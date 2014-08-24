/* cairo - a vector graphics library with display and print output
 *
 * Copyright © 2011 Intel Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it either under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * (the "LGPL") or, at your option, under the terms of the Mozilla
 * Public License Version 1.1 (the "MPL"). If you do not alter this
 * notice, a recipient may use your version of this file under either
 * the MPL or the LGPL.
 *
 * You should have received a copy of the LGPL along with this library
 * in the file COPYING-LGPL-2.1; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA 02110-1335, USA
 * You should have received a copy of the MPL along with this library
 * in the file COPYING-MPL-1.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.og/MPL/
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY
 * OF ANY KIND, either express or implied. See the LGPL or the MPL for
 * the specific language governing rights and limitations.
 *
 * Contributor(s):
 *      Robert Bragg <robert@linux.intel.com>
 */

/* so long as we can verify that the ctm doesn't change multiple times
 * during the construction of a path we can build a shadow
 * #cairo_path_fixed_t in user coordinates that we can use to create a
 * hash value for caching tessellations of that path.
 *
 * We need to hook into all the points where the ctm can be changed
 * so we can bump a cr->path_ctm_age counter.
 *
 * We need to hook into all the points where the path can be modified
 * so we can catch the start of a path and reset the cr->path_ctm_age
 * counter at that point.
 *
 * When a draw operation is hit we can then check that the
 * path_ctm_age == 0 and if so we create a hash of the path.
 *
 * We use this hash to lookup a #cairo_cogl_path_meta_t struct which
 * may contain tessellated triangles for the path or may just contain
 * a count of how many times the path has been re-seen (we only cache
 * tessellated triangles if there is evidence that the path is being
 * used multiple times because there is a cost involved in allocating
 * a separate buffer for the triangles).
 */

#include "cairoint.h"

#include "cairo-cogl-context-private.h"
#include "cairo-freed-pool-private.h"
#include "cairo-arc-private.h"
#include "cairo-path-fixed-private.h"

#include <glib.h>

static freed_pool_t context_pool;

void
_cairo_cogl_context_reset_static_data (void)
{
    _freed_pool_reset (&context_pool);
}

static cairo_status_t
_cairo_cogl_context_rectangle_real (cairo_cogl_context_t *cr,
				    double x, double y,
				    double width, double height)
{
    cairo_status_t status;
    status = cr->dev->backend_parent.rectangle (cr, x, y, width, height);
    if (unlikely (status))
	return status;

    return _cairo_cogl_path_fixed_rectangle (&cr->user_path,
					     _cairo_fixed_from_double (x),
					     _cairo_fixed_from_double (y),
					     _cairo_fixed_from_double (width),
					     _cairo_fixed_from_double (height));
}

/* The idea here is that we have a simplified way of tracking rectangle paths
 * because rectangles are perhaps the most common shape drawn with cairo.
 *
 * Basically we have a speculative store for a rectangle path that doesn't
 * need to use the #cairo_path_fixed_t api to describe a rectangle in terms of
 * (move_to,rel_line_to,rel_line_to,_rel_line_to,close) because if you profile
 * heavy rectangle drawing with Cairo that process can be overly expensive.
 *
 * If the user asks to add more than just a rectangle to their current path
 * then we "flush" any speculative rectangle stored into the current path
 * before continuing to append their operations.
 *
 * In addition to the speculative store cairo-cogl also has a fast-path
 * fill_rectangle drawing operation that further aims to minimize the cost
 * of drawing rectangles.
 */
static cairo_status_t
_flush_cr_rectangle (cairo_cogl_context_t *cr)
{
    if (!cr->path_is_rectangle)
	return CAIRO_STATUS_SUCCESS;

    cr->path_is_rectangle = FALSE;
    return _cairo_cogl_context_rectangle_real (cr, cr->x, cr->y, cr->width, cr->height);
}

static cairo_status_t
_cairo_cogl_context_restore (void *abstract_cr)
{
    cairo_cogl_context_t *cr = abstract_cr;

    if (cr->path_is_rectangle) {
	cairo_status_t status = _flush_cr_rectangle (cr);
	if (unlikely (status))
	    return status;
    }

    cr->path_ctm_age++;
    return cr->dev->backend_parent.restore (abstract_cr);
}

static cairo_status_t
_cairo_cogl_context_translate (void *abstract_cr, double tx, double ty)
{
    cairo_cogl_context_t *cr = abstract_cr;

    if (cr->path_is_rectangle) {
	cairo_status_t status = _flush_cr_rectangle (cr);
	if (unlikely (status))
	    return status;
    }

    cr->path_ctm_age++;
    return cr->dev->backend_parent.translate (abstract_cr, tx, ty);
}

static cairo_status_t
_cairo_cogl_context_scale (void *abstract_cr, double sx, double sy)
{
    cairo_cogl_context_t *cr = abstract_cr;

    if (cr->path_is_rectangle) {
	cairo_status_t status = _flush_cr_rectangle (cr);
	if (unlikely (status))
	    return status;
    }

    cr->path_ctm_age++;
    return cr->dev->backend_parent.scale (abstract_cr, sx, sy);
}

static cairo_status_t
_cairo_cogl_context_rotate (void *abstract_cr, double theta)
{
    cairo_cogl_context_t *cr = abstract_cr;

    if (cr->path_is_rectangle) {
	cairo_status_t status = _flush_cr_rectangle (cr);
	if (unlikely (status))
	    return status;
    }

    cr->path_ctm_age++;
    return cr->dev->backend_parent.rotate (abstract_cr, theta);
}

static cairo_status_t
_cairo_cogl_context_transform (void *abstract_cr, const cairo_matrix_t *matrix)
{
    cairo_cogl_context_t *cr = abstract_cr;

    if (cr->path_is_rectangle) {
	cairo_status_t status = _flush_cr_rectangle (cr);
	if (unlikely (status))
	    return status;
    }

    cr->path_ctm_age++;
    return cr->dev->backend_parent.transform (abstract_cr, matrix);
}

static cairo_status_t
_cairo_cogl_context_set_matrix (void *abstract_cr, const cairo_matrix_t *matrix)
{
    cairo_cogl_context_t *cr = abstract_cr;

    if (cr->path_is_rectangle) {
	cairo_status_t status = _flush_cr_rectangle (cr);
	if (unlikely (status))
	    return status;
    }

    cr->path_ctm_age++;
    return cr->dev->backend_parent.set_matrix (abstract_cr, matrix);
}

static cairo_status_t
_cairo_cogl_context_set_identity_matrix (void *abstract_cr)
{
    cairo_cogl_context_t *cr = abstract_cr;

    if (cr->path_is_rectangle) {
	cairo_status_t status = _flush_cr_rectangle (cr);
	if (unlikely (status))
	    return status;
    }

    cr->path_ctm_age++;
    return cr->dev->backend_parent.set_identity_matrix (abstract_cr);
}

static cairo_status_t
_cairo_cogl_context_new_path (void *abstract_cr)
{
    cairo_cogl_context_t *cr = abstract_cr;
    cairo_status_t status;

    if (cr->path_is_rectangle) {
	status = _flush_cr_rectangle (cr);
	if (unlikely (status))
	    return status;
    }

    status = cr->dev->backend_parent.new_path (abstract_cr);
    if (unlikely (status))
	return status;

    _cairo_path_fixed_fini (&cr->user_path);
    _cairo_path_fixed_init (&cr->user_path);
    cr->path_is_rectangle = FALSE;

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_cogl_context_new_sub_path (void *abstract_cr)
{
    cairo_cogl_context_t *cr = abstract_cr;
    cairo_status_t status;

    if (cr->path_is_rectangle) {
	status = _flush_cr_rectangle (cr);
	if (unlikely (status))
	    return status;
    }

    status = cr->dev->backend_parent.new_sub_path (abstract_cr);
    if (unlikely (status))
	return status;

    _cairo_path_fixed_new_sub_path (&cr->user_path);

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_cogl_context_move_to (void *abstract_cr, double x, double y)
{
    cairo_cogl_context_t *cr = abstract_cr;
    cairo_status_t status;
    cairo_fixed_t x_fixed, y_fixed;

    if (cr->path_is_rectangle) {
	status = _flush_cr_rectangle (cr);
	if (unlikely (status))
	    return status;
    }

    status = cr->dev->backend_parent.move_to (abstract_cr, x, y);
    if (unlikely (status))
	return status;

    x_fixed = _cairo_fixed_from_double (x);
    y_fixed = _cairo_fixed_from_double (y);

    return _cairo_path_fixed_move_to (&cr->user_path, x_fixed, y_fixed);
}

static cairo_status_t
_cairo_cogl_context_line_to (void *abstract_cr, double x, double y)
{
    cairo_cogl_context_t *cr = abstract_cr;
    cairo_status_t status;
    cairo_fixed_t x_fixed, y_fixed;

    if (cr->path_is_rectangle) {
	status = _flush_cr_rectangle (cr);
	if (unlikely (status))
	    return status;
    }

    status = cr->dev->backend_parent.line_to (abstract_cr, x, y);
    if (unlikely (status))
	return status;

    x_fixed = _cairo_fixed_from_double (x);
    y_fixed = _cairo_fixed_from_double (y);

    if (cr->user_path.buf.base.num_ops == 0)
	cr->path_ctm_age = 0;

    return _cairo_path_fixed_line_to (&cr->user_path, x_fixed, y_fixed);
}

static cairo_status_t
_cairo_cogl_context_curve_to (void *abstract_cr,
			       double x1, double y1,
			       double x2, double y2,
			       double x3, double y3)
{
    cairo_cogl_context_t *cr = abstract_cr;
    cairo_status_t status;
    cairo_fixed_t x1_fixed, y1_fixed;
    cairo_fixed_t x2_fixed, y2_fixed;
    cairo_fixed_t x3_fixed, y3_fixed;

    if (cr->path_is_rectangle) {
	status = _flush_cr_rectangle (cr);
	if (unlikely (status))
	    return status;
    }

    status = cr->dev->backend_parent.curve_to (abstract_cr, x1, y1, x2, y2, x3, y3);
    if (unlikely (status))
	return status;

    x1_fixed = _cairo_fixed_from_double (x1);
    y1_fixed = _cairo_fixed_from_double (y1);

    x2_fixed = _cairo_fixed_from_double (x2);
    y2_fixed = _cairo_fixed_from_double (y2);

    x3_fixed = _cairo_fixed_from_double (x3);
    y3_fixed = _cairo_fixed_from_double (y3);

    if (cr->user_path.buf.base.num_ops == 0)
	cr->path_ctm_age = 0;

    return _cairo_path_fixed_curve_to (&cr->user_path,
				       x1_fixed, y1_fixed,
				       x2_fixed, y2_fixed,
				       x3_fixed, y3_fixed);
}

static cairo_status_t
_cairo_cogl_context_arc (void *abstract_cr,
			  double xc, double yc, double radius,
			  double angle1, double angle2,
			  cairo_bool_t forward)
{
    cairo_cogl_context_t *cr = abstract_cr;
    cairo_status_t status;

    if (cr->path_is_rectangle) {
	status = _flush_cr_rectangle (cr);
	if (unlikely (status))
	    return status;
    }

    status = cr->dev->backend_parent.arc (abstract_cr, xc, yc, radius, angle1, angle2, forward);
    if (unlikely (status))
	return status;

    if (cr->user_path.buf.base.num_ops == 0)
	cr->path_ctm_age = 0;

    /* Do nothing, successfully, if radius is <= 0 */
    if (radius <= 0.0) {
	cairo_fixed_t x_fixed, y_fixed;

	x_fixed = _cairo_fixed_from_double (xc);
	y_fixed = _cairo_fixed_from_double (yc);
	status = _cairo_path_fixed_line_to (&cr->user_path, x_fixed, y_fixed);
	if (unlikely (status))
	    return status;

	status = _cairo_path_fixed_line_to (&cr->user_path, x_fixed, y_fixed);
	if (unlikely (status))
	    return status;

	return CAIRO_STATUS_SUCCESS;
    }

    status = _cairo_cogl_context_line_to (cr,
					  xc + radius * cos (angle1),
					  yc + radius * sin (angle1));

    if (unlikely (status))
	return status;

    if (forward)
	_cairo_arc_path (&cr->base.base, xc, yc, radius, angle1, angle2);
    else
	_cairo_arc_path_negative (&cr->base.base, xc, yc, radius, angle1, angle2);

    return CAIRO_STATUS_SUCCESS; /* any error will have already been set on cr */
}

static cairo_status_t
_cairo_cogl_context_rel_move_to (void *abstract_cr, double dx, double dy)
{
    cairo_cogl_context_t *cr = abstract_cr;
    cairo_status_t status;
    cairo_fixed_t dx_fixed, dy_fixed;

    if (cr->path_is_rectangle) {
	status = _flush_cr_rectangle (cr);
	if (unlikely (status))
	    return status;
    }

    status = cr->dev->backend_parent.rel_move_to (abstract_cr, dx, dy);
    if (unlikely (status))
	return status;

    dx_fixed = _cairo_fixed_from_double (dx);
    dy_fixed = _cairo_fixed_from_double (dy);

    return _cairo_path_fixed_rel_move_to (&cr->user_path, dx_fixed, dy_fixed);
}

static cairo_status_t
_cairo_cogl_context_rel_line_to (void *abstract_cr, double dx, double dy)
{
    cairo_cogl_context_t *cr = abstract_cr;
    cairo_status_t status;
    cairo_fixed_t dx_fixed, dy_fixed;

    if (cr->path_is_rectangle) {
	status = _flush_cr_rectangle (cr);
	if (unlikely (status))
	    return status;
    }

    status = cr->dev->backend_parent.rel_line_to (abstract_cr, dx, dy);
    if (unlikely (status))
	return status;

    dx_fixed = _cairo_fixed_from_double (dx);
    dy_fixed = _cairo_fixed_from_double (dy);

    if (cr->user_path.buf.base.num_ops == 0)
	cr->path_ctm_age = 0;

    return _cairo_path_fixed_rel_line_to (&cr->user_path, dx_fixed, dy_fixed);
}


static cairo_status_t
_cairo_cogl_context_rel_curve_to (void *abstract_cr,
				   double dx1, double dy1,
				   double dx2, double dy2,
				   double dx3, double dy3)
{
    cairo_cogl_context_t *cr = abstract_cr;
    cairo_status_t status;
    cairo_fixed_t dx1_fixed, dy1_fixed;
    cairo_fixed_t dx2_fixed, dy2_fixed;
    cairo_fixed_t dx3_fixed, dy3_fixed;

    if (cr->path_is_rectangle) {
	status = _flush_cr_rectangle (cr);
	if (unlikely (status))
	    return status;
    }

    status = cr->dev->backend_parent.rel_curve_to (abstract_cr, dx1, dy1, dx2, dy2, dx3, dy3);
    if (unlikely (status))
	return status;

    dx1_fixed = _cairo_fixed_from_double (dx1);
    dy1_fixed = _cairo_fixed_from_double (dy1);

    dx2_fixed = _cairo_fixed_from_double (dx2);
    dy2_fixed = _cairo_fixed_from_double (dy2);

    dx3_fixed = _cairo_fixed_from_double (dx3);
    dy3_fixed = _cairo_fixed_from_double (dy3);

    if (cr->user_path.buf.base.num_ops == 0)
	cr->path_ctm_age = 0;

    return _cairo_path_fixed_rel_curve_to (&cr->user_path,
					   dx1_fixed, dy1_fixed,
					   dx2_fixed, dy2_fixed,
					   dx3_fixed, dy3_fixed);
}

#if 0
static cairo_status_t
_cairo_cogl_context_arc_to (void *abstract_cr,
			    double x1, double y1,
			    double x2, double y2,
			    double radius)
{
    cairo_cogl_context_t *cr = abstract_cr;
    cairo_status_t status;

    if (cr->path_is_rectangle) {
	status = _flush_cr_rectangle (cr);
	if (unlikely (status))
	    return status;
    }

    status = cr->dev->backend_parent.arc_to (abstract_cr, x1, y1, x2, y2, radius);
    if (unlikely (status))
	return status;
#warning "FIXME"
}

static cairo_status_t
_cairo_cogl_rel_arc_to (void *cr,
			double dx1, double dy1,
			double dx2, double dy2,
			double radius)
{
    cairo_cogl_context_t *cr = abstract_cr;
    cairo_status_t status;

    if (cr->path_is_rectangle) {
	status = _flush_cr_rectangle (cr);
	if (unlikely (status))
	    return status;
    }

    status = cr->dev->backend_parent.rel_arc_to (abstract_cr, dx1, dy2, dx2, dy2, radius);
    if (unlikely (status))
	return status;
#warning "FIXME"
}
#endif

static cairo_status_t
_cairo_cogl_context_close_path (void *abstract_cr)
{
    cairo_cogl_context_t *cr = abstract_cr;
    cairo_status_t status;

    if (cr->path_is_rectangle) {
	status = _flush_cr_rectangle (cr);
	if (unlikely (status))
	    return status;
    }

    status = cr->dev->backend_parent.close_path (abstract_cr);
    if (unlikely (status))
	return status;

    if (cr->user_path.buf.base.num_ops == 0)
	cr->path_ctm_age = 0;

    return _cairo_path_fixed_close_path (&cr->user_path);
}

static cairo_status_t
_cairo_cogl_context_rectangle (void *abstract_cr,
			       double x, double y,
			       double width, double height)
{
    cairo_cogl_context_t *cr = abstract_cr;

    if (cr->user_path.buf.base.num_ops == 0) {
	cr->path_ctm_age = 0;

#if 1
	/* XXX: Since drawing rectangles is so common we have a
	 * fast-path for drawing a single rectangle. */
	cr->x = x;
	cr->y = y;
	cr->width = width;
	cr->height = height;
	cr->path_is_rectangle = TRUE;
	return CAIRO_STATUS_SUCCESS;
#endif
    }

    if (cr->path_is_rectangle) {
	cairo_status_t status = _flush_cr_rectangle (cr);
	if (unlikely (status))
	    return status;
    }

    return _cairo_cogl_context_rectangle_real (cr, x, y, width, height);
}

/* Since the surface backend drawing operator functions don't get
 * passed the current #cairo_t context we don't have a good way
 * to get our user-coordinates path into our surface operator
 * functions.
 *
 * For now we use this function to set side band data on the surface
 * itself.
 */
static void
_cairo_cogl_surface_set_side_band_state (cairo_cogl_surface_t *surface,
					 cairo_cogl_context_t *cr)
{

    if (cr->path_ctm_age <= 1) {
	surface->user_path = &cr->user_path;
	surface->ctm = &cr->base.gstate->ctm;
	surface->ctm_inverse = &cr->base.gstate->ctm_inverse;
	surface->path_is_rectangle = cr->path_is_rectangle;
	if (surface->path_is_rectangle) {
	    surface->path_rectangle_x = cr->x;
	    surface->path_rectangle_y = cr->y;
	    surface->path_rectangle_width = cr->width;
	    surface->path_rectangle_height = cr->height;
	}
    } else {
	surface->user_path = NULL;
	surface->path_is_rectangle = FALSE;
    }
}

static cairo_status_t
_cairo_cogl_context_fill (void *abstract_cr)
{
    cairo_cogl_context_t *cr = abstract_cr;
    cairo_status_t status;
    cairo_cogl_surface_t *surface = (cairo_cogl_surface_t *)cr->base.gstate->target;

    if (cr->path_is_rectangle) {
	status = _cairo_cogl_surface_fill_rectangle (cr->base.gstate->target,
						     cr->base.gstate->op,
						     cr->base.gstate->source,
						     cr->x,
						     cr->y,
						     cr->width,
						     cr->height,
						     &cr->base.gstate->ctm,
						     cr->base.gstate->clip);
	if (status == CAIRO_STATUS_SUCCESS)
	    goto DONE;
	_flush_cr_rectangle (cr);
    }

    _cairo_cogl_surface_set_side_band_state (surface, cr);

    status = cr->dev->backend_parent.fill (abstract_cr);
    if (unlikely (status))
	return status;

DONE:
    _cairo_path_fixed_fini (&cr->user_path);
    _cairo_path_fixed_init (&cr->user_path);
    cr->path_is_rectangle = FALSE;

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_cogl_context_fill_preserve (void *abstract_cr)
{
    cairo_cogl_context_t *cr = abstract_cr;
    cairo_status_t status;
    cairo_cogl_surface_t *surface = (cairo_cogl_surface_t *)cr->base.gstate->target;

    _cairo_cogl_surface_set_side_band_state (surface, cr);

    status = cr->dev->backend_parent.fill_preserve (abstract_cr);
    if (unlikely (status))
	return status;

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_cogl_context_stroke (void *abstract_cr)
{
    cairo_cogl_context_t *cr = abstract_cr;
    cairo_status_t status;
    cairo_cogl_surface_t *surface = (cairo_cogl_surface_t *)cr->base.gstate->target;

    _cairo_cogl_surface_set_side_band_state (surface, cr);

    status = cr->dev->backend_parent.stroke (abstract_cr);
    if (unlikely (status))
	return status;

    _cairo_path_fixed_fini (&cr->user_path);
    _cairo_path_fixed_init (&cr->user_path);
    cr->path_is_rectangle = FALSE;

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_cogl_context_stroke_preserve (void *abstract_cr)
{
    cairo_cogl_context_t *cr = abstract_cr;
    cairo_status_t status;
    cairo_cogl_surface_t *surface = (cairo_cogl_surface_t *)cr->base.gstate->target;

    _cairo_cogl_surface_set_side_band_state (surface, cr);

    status = cr->dev->backend_parent.stroke_preserve (abstract_cr);
    if (unlikely (status))
	return status;

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_cogl_context_clip (void *abstract_cr)
{
    cairo_cogl_context_t *cr = abstract_cr;
    cairo_status_t status;

    status = cr->dev->backend_parent.clip (abstract_cr);
    if (unlikely (status))
	return status;

    _cairo_path_fixed_fini (&cr->user_path);
    _cairo_path_fixed_init (&cr->user_path);
    cr->path_is_rectangle = FALSE;

    return CAIRO_STATUS_SUCCESS;
}

static void
_cairo_cogl_context_destroy (void *abstract_cr)
{
    cairo_cogl_context_t *cr = abstract_cr;

    _cairo_default_context_fini (&cr->base);

    _cairo_path_fixed_fini (&cr->user_path);

    /* mark the context as invalid to protect against misuse */
    cr->base.base.status = CAIRO_STATUS_NULL_POINTER;
    _freed_pool_put (&context_pool, cr);
}

/* We want to hook into the frontend of the path construction APIs so
 * we can build up a path description in user coordinates instead of
 * backend coordinates so that we can recognize user coordinate
 * rectangles and so we can hash a user path independent of its
 * transform. (With some care to catch unusual cases where the ctm
 * changes mid-path) */
cairo_t *
_cairo_cogl_context_create (void *target)
{
    cairo_cogl_surface_t *surface = target;
    cairo_cogl_context_t *cr;
    cairo_status_t status;

    cr = _freed_pool_get (&context_pool);
    if (unlikely (cr == NULL)) {
	cr = malloc (sizeof (cairo_cogl_context_t));
	if (unlikely (cr == NULL))
	    return _cairo_create_in_error (_cairo_error (CAIRO_STATUS_NO_MEMORY));
    }

    status = _cairo_default_context_init (&cr->base, target);
    if (unlikely (status)) {
	_freed_pool_put (&context_pool, cr);
	return _cairo_create_in_error (status);
    }

    cr->dev = (cairo_cogl_device_t *)surface->base.device;

    if (unlikely (cr->dev->backend_vtable_initialized == FALSE)) {
	cairo_backend_t *backend = &cr->dev->backend;
	memcpy (backend, cr->base.base.backend, sizeof (cairo_backend_t));
	memcpy (&cr->dev->backend_parent, cr->base.base.backend, sizeof (cairo_backend_t));

	backend->destroy = _cairo_cogl_context_destroy;

	backend->restore = _cairo_cogl_context_restore;
	backend->translate = _cairo_cogl_context_translate;
	backend->scale = _cairo_cogl_context_scale;
	backend->rotate = _cairo_cogl_context_rotate;
	backend->transform = _cairo_cogl_context_transform;
	backend->set_matrix = _cairo_cogl_context_set_matrix;
	backend->set_identity_matrix = _cairo_cogl_context_set_identity_matrix;

	backend->new_path = _cairo_cogl_context_new_path;
	backend->new_sub_path = _cairo_cogl_context_new_sub_path;
	backend->move_to = _cairo_cogl_context_move_to;
	backend->rel_move_to = _cairo_cogl_context_rel_move_to;
	backend->line_to = _cairo_cogl_context_line_to;
	backend->rel_line_to = _cairo_cogl_context_rel_line_to;
	backend->curve_to = _cairo_cogl_context_curve_to;
	backend->rel_curve_to = _cairo_cogl_context_rel_curve_to;
#if 0
	backend->arc_to = _cairo_cogl_context_arc_to;
	backend->rel_arc_to = _cairo_cogl_context_rel_arc_to;
#endif
	backend->close_path = _cairo_cogl_context_close_path;
	//backend->arc = _cairo_cogl_context_arc;
	backend->rectangle = _cairo_cogl_context_rectangle;

	/* Try to automatically catch if any new path APIs are added that mean
	 * we may need to overload more functions... */
	assert (((char *)&backend->path_extents - (char *)&backend->device_to_user_distance)
		== (sizeof (void *) * 14));

	backend->fill = _cairo_cogl_context_fill;
	backend->fill_preserve = _cairo_cogl_context_fill_preserve;
	backend->stroke = _cairo_cogl_context_stroke;
	backend->stroke_preserve = _cairo_cogl_context_stroke_preserve;
	backend->clip = _cairo_cogl_context_clip;

	cr->dev->backend_vtable_initialized = TRUE;
    }

    cr->base.base.backend = &cr->dev->backend;

    _cairo_path_fixed_init (&cr->user_path);
    cr->path_is_rectangle = FALSE;

    return &cr->base.base;
}
