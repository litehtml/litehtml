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
#include "cairoint.h"

#include "cairo-cache-private.h"
#include "cairo-error-private.h"
#include "cairo-path-fixed-private.h"
#include "cairo-recording-surface-private.h"
#include "cairo-surface-clipper-private.h"
#include "cairo-fixed-private.h"
#include "cairo-device-private.h"
#include "cairo-composite-rectangles-private.h"
#include "cairo-image-surface-inline.h"
#include "cairo-cogl-private.h"
#include "cairo-cogl-gradient-private.h"
#include "cairo-arc-private.h"
#include "cairo-traps-private.h"
#include "cairo-cogl-context-private.h"
#include "cairo-cogl-utils-private.h"
#include "cairo-box-inline.h"
#include "cairo-surface-subsurface-inline.h"
#include "cairo-surface-fallback-private.h"
#include "cairo-surface-offset-private.h"

#include "cairo-cogl.h"

#include <cogl/cogl2-experimental.h>
#include <glib.h>

#define CAIRO_COGL_DEBUG 0
//#define FILL_WITH_COGL_PATH
//#define USE_CAIRO_PATH_FLATTENER
#define ENABLE_PATH_CACHE
//#define DISABLE_BATCHING
#define USE_COGL_RECTANGLE_API
#define ENABLE_RECTANGLES_FASTPATH

#if defined (USE_COGL_RECTANGLE_API) || defined (ENABLE_PATH_CACHE)
#define NEED_COGL_CONTEXT
#endif

#if CAIRO_COGL_DEBUG && __GNUC__
#define UNSUPPORTED(reason) ({ \
    g_warning ("cairo-cogl: hit unsupported operation: %s", reason); \
    CAIRO_INT_STATUS_UNSUPPORTED; \
})
#else
#define UNSUPPORTED(reason) CAIRO_INT_STATUS_UNSUPPORTED
#endif

#define CAIRO_COGL_PATH_META_CACHE_SIZE (1024 * 1024)

typedef struct _cairo_cogl_texture_attributes {
    /* nabbed from cairo_surface_attributes_t... */
    cairo_matrix_t	    matrix;
    cairo_extend_t	    extend;
    cairo_filter_t	    filter;
    cairo_bool_t	    has_component_alpha;

    CoglPipelineWrapMode    s_wrap;
    CoglPipelineWrapMode    t_wrap;
} cairo_cogl_texture_attributes_t;

typedef enum _cairo_cogl_journal_entry_type {
    CAIRO_COGL_JOURNAL_ENTRY_TYPE_RECTANGLE,
    CAIRO_COGL_JOURNAL_ENTRY_TYPE_PRIMITIVE,
    CAIRO_COGL_JOURNAL_ENTRY_TYPE_PATH,
    CAIRO_COGL_JOURNAL_ENTRY_TYPE_CLIP
} cairo_cogl_journal_entry_type_t;

typedef struct _cairo_cogl_journal_entry {
    cairo_cogl_journal_entry_type_t type;
} cairo_cogl_journal_entry_t;

typedef struct _cairo_cogl_journal_clip_entry {
    cairo_cogl_journal_entry_t base;
    cairo_clip_t *clip;
} cairo_cogl_journal_clip_entry_t;

typedef struct _cairo_cogl_journal_rect_entry {
    cairo_cogl_journal_entry_t base;
    CoglPipeline *pipeline;
    float x;
    float y;
    float width;
    float height;
    int n_layers;
    cairo_matrix_t ctm;
} cairo_cogl_journal_rect_entry_t;

typedef struct _cairo_cogl_journal_prim_entry {
    cairo_cogl_journal_entry_t base;
    CoglPipeline *pipeline;
    CoglPrimitive *primitive;
    gboolean has_transform;
    cairo_matrix_t transform;
} cairo_cogl_journal_prim_entry_t;

typedef struct _cairo_cogl_journal_path_entry {
    cairo_cogl_journal_entry_t base;
    CoglPipeline *pipeline;
    CoglPath *path;
} cairo_cogl_journal_path_entry_t;

typedef struct _cairo_cogl_path_fill_meta {
    cairo_cache_entry_t	cache_entry;
    cairo_reference_count_t ref_count;
    int counter;
    cairo_path_fixed_t *user_path;
    cairo_matrix_t ctm_inverse;

    /* TODO */
#if 0
    /* A cached path tessellation should be re-usable with different rotations
     * and translations but not for different scales.
     *
     * one idea is to track the diagonal lenghts of a unit rectangle
     * transformed through the original ctm use to tesselate the geometry
     * so we can check what the lengths are for any new ctm to know if
     * this geometry is compatible.
     */
#endif

    CoglPrimitive *prim;
} cairo_cogl_path_fill_meta_t;

typedef struct _cairo_cogl_path_stroke_meta {
    cairo_cache_entry_t	cache_entry;
    cairo_reference_count_t ref_count;
    int counter;
    cairo_path_fixed_t *user_path;
    cairo_matrix_t ctm_inverse;
    cairo_stroke_style_t style;
    double tolerance;

    /* TODO */
#if 0
    /* A cached path tessellation should be re-usable with different rotations
     * and translations but not for different scales.
     *
     * one idea is to track the diagonal lenghts of a unit rectangle
     * transformed through the original ctm use to tesselate the geometry
     * so we can check what the lengths are for any new ctm to know if
     * this geometry is compatible.
     */
#endif

    CoglPrimitive *prim;
} cairo_cogl_path_stroke_meta_t;

static cairo_surface_t *
_cairo_cogl_surface_create_full (cairo_cogl_device_t *dev,
				 cairo_bool_t ignore_alpha,
				 CoglFramebuffer *framebuffer,
				 CoglTexture *texture);

static cairo_int_status_t
_cairo_cogl_surface_fill (void			    *abstract_surface,
                          cairo_operator_t	     op,
                          const cairo_pattern_t	    *source,
                          const cairo_path_fixed_t  *path,
                          cairo_fill_rule_t	     fill_rule,
                          double		     tolerance,
                          cairo_antialias_t	     antialias,
                          const cairo_clip_t	    *clip);

static void
_cairo_cogl_journal_flush (cairo_cogl_surface_t *surface);

cairo_private extern const cairo_surface_backend_t _cairo_cogl_surface_backend;

slim_hidden_proto (cairo_cogl_device_create);
slim_hidden_proto (cairo_cogl_surface_create);
slim_hidden_proto (cairo_cogl_surface_get_framebuffer);
slim_hidden_proto (cairo_cogl_surface_get_texture);
slim_hidden_proto (cairo_cogl_surface_end_frame);

static cairo_cogl_device_t *
to_device (cairo_device_t *device)
{
    return (cairo_cogl_device_t *)device;
}

/* moves trap points such that they become the actual corners of the trapezoid */
static void
_sanitize_trap (cairo_trapezoid_t *t)
{
    cairo_trapezoid_t s = *t;

#define FIX(lr, tb, p) \
    if (t->lr.p.y != t->tb) { \
        t->lr.p.x = s.lr.p2.x + _cairo_fixed_mul_div_floor (s.lr.p1.x - s.lr.p2.x, s.tb - s.lr.p2.y, s.lr.p1.y - s.lr.p2.y); \
        t->lr.p.y = s.tb; \
    }
    FIX (left,  top,    p1);
    FIX (left,  bottom, p2);
    FIX (right, top,    p1);
    FIX (right, bottom, p2);
}

static cairo_status_t
_cairo_cogl_surface_ensure_framebuffer (cairo_cogl_surface_t *surface)
{
    GError *error = NULL;

    if (surface->framebuffer)
	return CAIRO_STATUS_SUCCESS;

    surface->framebuffer = COGL_FRAMEBUFFER (cogl_offscreen_new_to_texture (surface->texture));
    if (!cogl_framebuffer_allocate (surface->framebuffer, &error)) {
	g_error_free (error);
	cogl_object_unref (surface->framebuffer);
	surface->framebuffer = NULL;
	return CAIRO_STATUS_NO_MEMORY;
    }

    cogl_push_framebuffer (surface->framebuffer);
    cogl_ortho (0, surface->width,
		surface->height, 0,
		-1, 100);
    cogl_pop_framebuffer ();

    return CAIRO_STATUS_SUCCESS;
}

static cairo_surface_t *
_cairo_cogl_surface_create_similar (void            *abstract_surface,
				    cairo_content_t  content,
				    int              width,
				    int              height)
{
    cairo_cogl_surface_t *reference_surface = abstract_surface;
    cairo_cogl_surface_t *surface;
    CoglTexture *texture;
    cairo_status_t status;

    texture = cogl_texture_new_with_size (width, height,
					  COGL_TEXTURE_NO_SLICING,
					  (content & CAIRO_CONTENT_COLOR) ?
					  COGL_PIXEL_FORMAT_BGRA_8888_PRE :
					  COGL_PIXEL_FORMAT_A_8);
    if (!texture)
        return _cairo_surface_create_in_error (_cairo_error (CAIRO_STATUS_NO_MEMORY));

    surface = (cairo_cogl_surface_t *)
	_cairo_cogl_surface_create_full (to_device(reference_surface->base.device),
					 (content & CAIRO_CONTENT_ALPHA) == 0,
					 NULL,
					 texture);
    if (unlikely (surface->base.status))
	return &surface->base;

    status = _cairo_cogl_surface_ensure_framebuffer (surface);
    if (unlikely (status)) {
	cairo_surface_destroy (&surface->base);
	return _cairo_surface_create_in_error (status);
    }

    return &surface->base;
}

static cairo_bool_t
_cairo_cogl_surface_get_extents (void *abstract_surface,
                                 cairo_rectangle_int_t *extents)
{
    cairo_cogl_surface_t *surface = abstract_surface;

    extents->x = 0;
    extents->y = 0;
    extents->width  = surface->width;
    extents->height = surface->height;

    return TRUE;
}

static void
_cairo_cogl_journal_free (cairo_cogl_surface_t *surface)
{
    GList *l;

    for (l = surface->journal->head; l; l = l->next) {
	cairo_cogl_journal_entry_t *entry = l->data;

	if (entry->type == CAIRO_COGL_JOURNAL_ENTRY_TYPE_PRIMITIVE) {
	    cairo_cogl_journal_prim_entry_t *prim_entry =
		(cairo_cogl_journal_prim_entry_t *)entry;
	    cogl_object_unref (prim_entry->primitive);
	} else if (entry->type == CAIRO_COGL_JOURNAL_ENTRY_TYPE_PATH) {
	    cairo_cogl_journal_path_entry_t *path_entry =
		(cairo_cogl_journal_path_entry_t *)entry;
	    cogl_object_unref (path_entry->path);
	}
    }

    g_queue_free (surface->journal);
    surface->journal = NULL;
}

#ifdef FILL_WITH_COGL_PATH
static void
_cairo_cogl_journal_log_path (cairo_cogl_surface_t *surface,
			      CoglPipeline *pipeline,
			      CoglPath *path)
{
    cairo_cogl_journal_path_entry_t *entry;

    if (unlikely (surface->journal == NULL))
	surface->journal = g_queue_new ();

    /* FIXME: Instead of a GList here we should stack allocate the journal
     * entries so it would be cheaper to allocate and they can all be freed in
     * one go after flushing! */
    entry = g_slice_new (cairo_cogl_journal_path_entry_t);
    entry->base.type = CAIRO_COGL_JOURNAL_ENTRY_TYPE_PATH;

    entry->pipeline = cogl_object_ref (pipeline);
    entry->path = cogl_object_ref (path);

    g_queue_push_tail (surface->journal, entry);

#ifdef DISABLE_BATCHING
    _cairo_cogl_journal_flush (surface);
#endif
}
#endif /* FILL_WITH_COGL_PATH */

static void
_cairo_cogl_journal_log_primitive (cairo_cogl_surface_t *surface,
				   CoglPipeline *pipeline,
				   CoglPrimitive *primitive,
				   cairo_matrix_t *transform)
{
    cairo_cogl_journal_prim_entry_t *entry;

    if (unlikely (surface->journal == NULL))
	surface->journal = g_queue_new ();

    /* FIXME: Instead of a GList here we should stack allocate the journal
     * entries so it would be cheaper to allocate and they can all be freed in
     * one go after flushing! */
    entry = g_slice_new (cairo_cogl_journal_prim_entry_t);
    entry->base.type = CAIRO_COGL_JOURNAL_ENTRY_TYPE_PRIMITIVE;

    entry->pipeline = cogl_object_ref (pipeline);

    if (transform) {
	entry->transform = *transform;
	entry->has_transform = TRUE;
    } else
	entry->has_transform = FALSE;

    entry->primitive = cogl_object_ref (primitive);

    g_queue_push_tail (surface->journal, entry);

#ifdef DISABLE_BATCHING
    _cairo_cogl_journal_flush (surface);
#endif
}

static void
_cairo_cogl_journal_log_rectangle (cairo_cogl_surface_t *surface,
				   CoglPipeline *pipeline,
				   float x,
				   float y,
				   float width,
				   float height,
				   int n_layers,
				   cairo_matrix_t *ctm)
{
    cairo_cogl_journal_rect_entry_t *entry;

    if (unlikely (surface->journal == NULL))
	surface->journal = g_queue_new ();

    /* FIXME: Instead of a GList here we should stack allocate the journal
     * entries so it would be cheaper to allocate and they can all be freed in
     * one go after flushing! */
    entry = g_slice_new (cairo_cogl_journal_rect_entry_t);
    entry->base.type = CAIRO_COGL_JOURNAL_ENTRY_TYPE_RECTANGLE;

    entry->pipeline = cogl_object_ref (pipeline);

    entry->x = x;
    entry->y = y;
    entry->width = width;
    entry->height = height;
    entry->ctm = *ctm;

    entry->n_layers = n_layers;

    g_queue_push_tail (surface->journal, entry);

#ifdef DISABLE_BATCHING
    _cairo_cogl_journal_flush (surface);
#endif
}

static void
_cairo_cogl_journal_log_clip (cairo_cogl_surface_t *surface,
			      const cairo_clip_t *clip)
{
    cairo_cogl_journal_clip_entry_t *entry;

    if (unlikely (surface->journal == NULL))
	surface->journal = g_queue_new ();

    /* FIXME: Instead of a GList here we should stack allocate the journal
     * entries so it would be cheaper to allocate and they can all be freed in
     * one go after flushing! */
    entry = g_slice_new (cairo_cogl_journal_clip_entry_t);
    entry->base.type = CAIRO_COGL_JOURNAL_ENTRY_TYPE_CLIP;
    entry->clip = _cairo_clip_copy (clip);

    g_queue_push_tail (surface->journal, entry);
}

static void
_cairo_cogl_journal_discard (cairo_cogl_surface_t *surface)
{
    GList *l;

    if (!surface->journal) {
	assert (surface->last_clip == NULL);
	return;
    }

    if (surface->buffer_stack && surface->buffer_stack_offset) {
	cogl_buffer_unmap (COGL_BUFFER (surface->buffer_stack));
	cogl_object_unref (surface->buffer_stack);
	surface->buffer_stack = NULL;
    }

    for (l = surface->journal->head; l; l = l->next) {
	cairo_cogl_journal_entry_t *entry = l->data;
	gsize entry_size;

	switch (entry->type)
	{
	case CAIRO_COGL_JOURNAL_ENTRY_TYPE_CLIP: {
	    cairo_cogl_journal_clip_entry_t *clip_entry =
		(cairo_cogl_journal_clip_entry_t *)entry;
	    _cairo_clip_destroy (clip_entry->clip);
	    entry_size = sizeof (cairo_cogl_journal_clip_entry_t);
	    break;
	}
	case CAIRO_COGL_JOURNAL_ENTRY_TYPE_RECTANGLE: {
	    cairo_cogl_journal_rect_entry_t *rect_entry =
		(cairo_cogl_journal_rect_entry_t *)entry;
	    cogl_object_unref (rect_entry->pipeline);
	    entry_size = sizeof (cairo_cogl_journal_rect_entry_t);
	    break;
	}
	case CAIRO_COGL_JOURNAL_ENTRY_TYPE_PRIMITIVE: {
	    cairo_cogl_journal_prim_entry_t *prim_entry =
		(cairo_cogl_journal_prim_entry_t *)entry;
	    cogl_object_unref (prim_entry->pipeline);
	    cogl_object_unref (prim_entry->primitive);
	    entry_size = sizeof (cairo_cogl_journal_prim_entry_t);
	    break;
	}
	case CAIRO_COGL_JOURNAL_ENTRY_TYPE_PATH: {
	    cairo_cogl_journal_path_entry_t *path_entry =
		(cairo_cogl_journal_path_entry_t *)entry;
	    cogl_object_unref (path_entry->pipeline);
	    cogl_object_unref (path_entry->path);
	    entry_size = sizeof (cairo_cogl_journal_path_entry_t);
	    break;
	}
	default:
	    assert (0); /* not reached! */
	    entry_size = 0; /* avoid compiler warning */
	}
	g_slice_free1 (entry_size, entry);
    }

    g_queue_clear (surface->journal);

    if (surface->last_clip) {
	_cairo_clip_destroy (surface->last_clip);
	surface->last_clip = NULL;
    }
}

static CoglAttributeBuffer *
_cairo_cogl_surface_allocate_buffer_space (cairo_cogl_surface_t *surface,
					   size_t size,
					   size_t *offset,
					   void **pointer)
{
    /* XXX: In the Cogl journal we found it more efficient to have a pool of
     * buffers that we re-cycle but for now we simply thow away our stack
     * buffer each time we flush. */
    if (unlikely (surface->buffer_stack &&
		  (surface->buffer_stack_size - surface->buffer_stack_offset) < size)) {
	cogl_buffer_unmap (COGL_BUFFER (surface->buffer_stack));
	cogl_object_unref (surface->buffer_stack);
	surface->buffer_stack = NULL;
	surface->buffer_stack_size *= 2;
    }

    if (unlikely (surface->buffer_stack_size < size))
	surface->buffer_stack_size = size * 2;

    if (unlikely (surface->buffer_stack == NULL)) {
	surface->buffer_stack = cogl_attribute_buffer_new (surface->buffer_stack_size, NULL);
	surface->buffer_stack_pointer =
	    cogl_buffer_map (COGL_BUFFER (surface->buffer_stack),
			     COGL_BUFFER_ACCESS_WRITE,
			     COGL_BUFFER_MAP_HINT_DISCARD);
	surface->buffer_stack_offset = 0;
    }

    *pointer = surface->buffer_stack_pointer + surface->buffer_stack_offset;
    *offset = surface->buffer_stack_offset;

    surface->buffer_stack_offset += size;
    return cogl_object_ref (surface->buffer_stack);
}


static CoglAttributeBuffer *
_cairo_cogl_traps_to_triangles_buffer (cairo_cogl_surface_t *surface,
				       cairo_traps_t *traps,
				       size_t *offset,
				       gboolean one_shot)
{
    CoglAttributeBuffer *buffer;
    int n_traps = traps->num_traps;
    int i;
    CoglVertexP2 *triangles;

    if (one_shot) {
	buffer = _cairo_cogl_surface_allocate_buffer_space (surface,
							    n_traps * sizeof (CoglVertexP2) * 6,
							    offset,
							    (void **)&triangles);
	if (!buffer)
	    return NULL;
    } else {
	buffer = cogl_attribute_buffer_new (n_traps * sizeof (CoglVertexP2) * 6, NULL);
	if (!buffer)
	    return NULL;
	triangles = cogl_buffer_map (COGL_BUFFER (buffer),
				     COGL_BUFFER_ACCESS_WRITE,
				     COGL_BUFFER_MAP_HINT_DISCARD);
	if (!triangles)
	    return NULL;
	*offset = 0;
    }

    /* XXX: This is can be very expensive. I'm not sure a.t.m if it's
     * predominantly the bandwidth required or the cost of the fixed_to_float
     * conversions but either way we should try using an index buffer to
     * reduce the amount we upload by 1/3 (offset by allocating and uploading
     * indices though) sadly though my experience with the intel mesa drivers
     * is that slow paths can easily be hit when starting to use indices.
     */
    for (i = 0; i < n_traps; i++)
    {
	CoglVertexP2 *p = &triangles[i * 6];
	cairo_trapezoid_t *trap = &traps->traps[i];

	p[0].x = _cairo_cogl_util_fixed_to_float (trap->left.p1.x);
	p[0].y = _cairo_cogl_util_fixed_to_float (trap->left.p1.y);

	p[1].x = _cairo_cogl_util_fixed_to_float (trap->left.p2.x);
	p[1].y = _cairo_cogl_util_fixed_to_float (trap->left.p2.y);

	p[2].x = _cairo_cogl_util_fixed_to_float (trap->right.p2.x);
	p[2].y = _cairo_cogl_util_fixed_to_float (trap->right.p2.y);

	p[3].x = _cairo_cogl_util_fixed_to_float (trap->left.p1.x);
	p[3].y = _cairo_cogl_util_fixed_to_float (trap->left.p1.y);

	p[4].x = _cairo_cogl_util_fixed_to_float (trap->right.p2.x);
	p[4].y = _cairo_cogl_util_fixed_to_float (trap->right.p2.y);

	p[5].x = _cairo_cogl_util_fixed_to_float (trap->right.p1.x);
	p[5].y = _cairo_cogl_util_fixed_to_float (trap->right.p1.y);
    }

    if (!one_shot)
	cogl_buffer_unmap (COGL_BUFFER (buffer));

    return buffer;
}

/* Used for solid fills, in this case we just need a mesh made of
 * a single (2-component) position attribute. */
static CoglPrimitive *
_cairo_cogl_traps_to_composite_prim_p2 (cairo_cogl_surface_t *surface,
					cairo_traps_t *traps,
					gboolean one_shot)
{
    size_t offset;
    CoglAttributeBuffer *buffer = _cairo_cogl_traps_to_triangles_buffer (surface, traps, &offset, one_shot);
    CoglAttribute *pos = cogl_attribute_new (buffer,
					     "cogl_position_in",
					     sizeof (CoglVertexP2),
					     offset,
					     2,
					     COGL_ATTRIBUTE_TYPE_FLOAT);
    CoglPrimitive *prim;

    /* The attribute will have taken a reference on the buffer */
    cogl_object_unref (buffer);

    prim = cogl_primitive_new (COGL_VERTICES_MODE_TRIANGLES,
			       traps->num_traps * 6, pos, NULL);

    /* The primitive will now keep the attribute alive... */
    cogl_object_unref (pos);

    return prim;
}

/* Used for surface fills, in this case we need a mesh made of a single
 * (2-component) position attribute + we also alias the same attribute as
 * (2-component) texture coordinates */
static CoglPrimitive *
_cairo_cogl_traps_to_composite_prim_p2t2 (cairo_cogl_surface_t *surface,
					  cairo_traps_t *traps,
					  gboolean one_shot)
{
    size_t offset;
    CoglAttributeBuffer *buffer = _cairo_cogl_traps_to_triangles_buffer (surface, traps, &offset, one_shot);
    CoglAttribute *pos = cogl_attribute_new (buffer,
					     "cogl_position_in",
					     sizeof (CoglVertexP2),
					     offset,
					     2,
					     COGL_ATTRIBUTE_TYPE_FLOAT);
    CoglAttribute *tex_coords = cogl_attribute_new (buffer,
						    "cogl_tex_coord0_in",
						    sizeof (CoglVertexP2),
						    0,
						    2,
						    COGL_ATTRIBUTE_TYPE_FLOAT);
    CoglPrimitive *prim;

    /* The attributes will have taken references on the buffer */
    cogl_object_unref (buffer);

    prim = cogl_primitive_new (COGL_VERTICES_MODE_TRIANGLES,
			       traps->num_traps * 6, pos, tex_coords, NULL);

    /* The primitive will now keep the attributes alive... */
    cogl_object_unref (pos);
    cogl_object_unref (tex_coords);

    return prim;
}

static CoglPrimitive *
_cairo_cogl_traps_to_composite_prim (cairo_cogl_surface_t *surface,
				     cairo_traps_t *traps,
				     int n_layers,
				     gboolean one_shot)
{
    int n_traps = traps->num_traps;
    int i;

    /* XXX: Ideally we would skip tessellating to traps entirely since
     * given their representation, conversion to triangles is quite expensive.
     *
     * This simplifies the conversion to triangles by making the end points of
     * the two side lines actually just correspond to the corners of the
     * traps.
     */
    for (i = 0; i < n_traps; i++)
	_sanitize_trap (&traps->traps[i]);

    if (n_layers == 0)
	return _cairo_cogl_traps_to_composite_prim_p2 (surface, traps, one_shot);
    else {
	assert (n_layers == 1);
	return _cairo_cogl_traps_to_composite_prim_p2t2 (surface, traps, one_shot);
    }
}

static cairo_int_status_t
_cairo_cogl_fill_to_primitive (cairo_cogl_surface_t	*surface,
			       const cairo_path_fixed_t	*path,
			       cairo_fill_rule_t	 fill_rule,
			       double			 tolerance,
			       int			 n_layers,
			       cairo_bool_t		 one_shot,
			       CoglPrimitive	       **primitive,
			       size_t			*size)
{
    cairo_traps_t traps;
    cairo_int_status_t status;

    _cairo_traps_init (&traps);
    status = _cairo_path_fixed_fill_to_traps (path, fill_rule, tolerance, &traps);
    if (unlikely (status))
	goto BAIL;

    if (traps.num_traps == 0) {
	status = CAIRO_INT_STATUS_NOTHING_TO_DO;
	goto BAIL;
    }

    *size = traps.num_traps * sizeof (CoglVertexP2) * 6;

    *primitive = _cairo_cogl_traps_to_composite_prim (surface, &traps, n_layers, one_shot);
    if (!*primitive) {
	status = CAIRO_INT_STATUS_NO_MEMORY;
	goto BAIL;
    }

BAIL:
    _cairo_traps_fini (&traps);
    return status;
}

static void
_cairo_cogl_clip_push_box (const cairo_box_t *box)
{
    if (_cairo_box_is_pixel_aligned (box)) {
	cairo_rectangle_int_t rect;
	_cairo_box_round_to_rectangle (box, &rect);
	cogl_clip_push_window_rectangle (rect.x, rect.y,
					 rect.width, rect.height);
    } else {
	double x1, y1, x2, y2;
	_cairo_box_to_doubles (box, &x1, &y1, &x2, &y2);
	cogl_clip_push_rectangle (x1, y1, x2, y2);
    }
}

static void
_cairo_cogl_journal_flush (cairo_cogl_surface_t *surface)
{
    GList *l;
    int clip_stack_depth = 0;
    int i;

    if (!surface->journal)
	return;

    if (surface->buffer_stack && surface->buffer_stack_offset) {
	cogl_buffer_unmap (COGL_BUFFER (surface->buffer_stack));
	cogl_object_unref (surface->buffer_stack);
	surface->buffer_stack = NULL;
    }

    cogl_set_framebuffer (surface->framebuffer);

    cogl_push_matrix ();

    for (l = surface->journal->head; l; l = l->next) {
	cairo_cogl_journal_entry_t *entry = l->data;

	switch (entry->type)
	{
	case CAIRO_COGL_JOURNAL_ENTRY_TYPE_CLIP: {
	    cairo_cogl_journal_clip_entry_t *clip_entry =
		(cairo_cogl_journal_clip_entry_t *)entry;
	    cairo_clip_path_t *path;
#if 0
	    cairo_bool_t checked_for_primitives = FALSE;
	    cairo_cogl_clip_primitives_t *clip_primitives;
#endif

	    for (i = 0; i < clip_stack_depth; i++)
		cogl_clip_pop ();
	    clip_stack_depth = 0;

	    for (path = clip_entry->clip->path, i = 0; path; path = path->prev, i++) {
		cairo_rectangle_int_t extents;
		cairo_int_status_t status;
		CoglPrimitive *prim;
		size_t prim_size;

		_cairo_path_fixed_approximate_clip_extents (&path->path, &extents);

		/* TODO - maintain a fifo of the last 10 used clips with cached
		 * primitives to see if we can avoid tesselating the path and
		 * uploading the vertices...
		 */
#if 0
		if (!checked_for_primitives) {
		    clip_primitives = find_clip_primitives (clip);
		    checked_for_primitives = TRUE;
		}
		if (clip_primitives)
		    prim = clip_primitives->primitives[i];
#endif
		status = _cairo_cogl_fill_to_primitive (surface,
							&path->path,
							path->fill_rule,
							path->tolerance,
							0,
							TRUE,
							&prim,
							&prim_size);
		if (unlikely (status)) {
		    g_warning ("Failed to get primitive for clip path while flushing journal");
		    continue;
		}
		clip_stack_depth++;
		cogl_clip_push_primitive (prim,
					  extents.x, extents.y,
					  extents.x + extents.width,
					  extents.y + extents.height);
		cogl_object_unref (prim);
	    }

	    for (i = 0; i < clip_entry->clip->num_boxes; i++) {
		clip_stack_depth++;
		_cairo_cogl_clip_push_box (&clip_entry->clip->boxes[i]);
	    }

	    surface->n_clip_updates_per_frame++;
	    break;
	}
	case CAIRO_COGL_JOURNAL_ENTRY_TYPE_RECTANGLE: {
	    cairo_cogl_journal_rect_entry_t *rect_entry =
		(cairo_cogl_journal_rect_entry_t *)entry;
	    float tex_coords[8];
	    float x1 = rect_entry->x;
	    float y1 = rect_entry->y;
	    float x2 = rect_entry->x + rect_entry->width;
	    float y2 = rect_entry->y + rect_entry->height;
	    cairo_matrix_t *ctm = &rect_entry->ctm;
	    float ctmfv[16] = {
		ctm->xx, ctm->yx, 0, 0,
		ctm->xy, ctm->yy, 0, 0,
		0,	     0,	      1, 0,
		ctm->x0, ctm->y0, 0, 1
	    };
	    CoglMatrix transform;

	    cogl_matrix_init_from_array (&transform, ctmfv);

	    if (rect_entry->n_layers) {
		g_assert (rect_entry->n_layers <= 2);
		tex_coords[0] = x1;
		tex_coords[1] = y1;
		tex_coords[2] = x2;
		tex_coords[3] = y2;
		if (rect_entry->n_layers > 1)
		    memcpy (&tex_coords[4], tex_coords, sizeof (float) * 4);
	    }

	    cogl_set_source (rect_entry->pipeline);
	    cogl_push_matrix ();
	    cogl_transform (&transform);
	    cogl_rectangle_with_multitexture_coords (x1, y1, x2, y2,
						     tex_coords, 4 * rect_entry->n_layers);
	    cogl_pop_matrix ();
	    break;
	}
	case CAIRO_COGL_JOURNAL_ENTRY_TYPE_PRIMITIVE: {
	    cairo_cogl_journal_prim_entry_t *prim_entry =
		(cairo_cogl_journal_prim_entry_t *)entry;
	    CoglMatrix transform;

	    cogl_push_matrix ();
	    if (prim_entry->has_transform) {
		cairo_matrix_t *ctm = &prim_entry->transform;
		float ctmfv[16] = {
		    ctm->xx, ctm->yx, 0, 0,
		    ctm->xy, ctm->yy, 0, 0,
		    0,	     0,	      1, 0,
		    ctm->x0, ctm->y0, 0, 1
		};
		cogl_matrix_init_from_array (&transform, ctmfv);
		cogl_transform (&transform);
	    } else {
		cogl_matrix_init_identity (&transform);
		cogl_set_modelview_matrix (&transform);
	    }

	    cogl_set_source (prim_entry->pipeline);
	    cogl_primitive_draw (prim_entry->primitive);
	    cogl_pop_matrix ();
	    break;
	}
	case CAIRO_COGL_JOURNAL_ENTRY_TYPE_PATH: {
	    cairo_cogl_journal_path_entry_t *path_entry =
		(cairo_cogl_journal_path_entry_t *)entry;

	    cogl_set_source (path_entry->pipeline);
	    cogl_path_fill (path_entry->path);
	    break;
	}
	default:
	    assert (0); /* not reached! */
	}
    }

    cogl_pop_matrix ();

    for (i = 0; i < clip_stack_depth; i++)
	cogl_clip_pop ();

    _cairo_cogl_journal_discard (surface);
}

static cairo_status_t
_cairo_cogl_surface_flush (void *abstract_surface)
{
    cairo_cogl_surface_t *surface = (cairo_cogl_surface_t *)abstract_surface;

    _cairo_cogl_journal_flush (surface);

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_cogl_surface_finish (void *abstract_surface)
{
    cairo_cogl_surface_t *surface = abstract_surface;

    if (surface->texture)
	cogl_object_unref (surface->texture);

    if (surface->framebuffer)
	cogl_object_unref (surface->framebuffer);

    if (surface->journal)
	_cairo_cogl_journal_free (surface);

    /*XXX wtf */
    cairo_device_release (surface->base.device);

    return CAIRO_STATUS_SUCCESS;
}

static CoglPixelFormat
get_cogl_format_from_cairo_format (cairo_format_t cairo_format);

/* XXX: We often use RGBA format for onscreen framebuffers so make sure
 * to handle CAIRO_FORMAT_INVALID sensibly */
static cairo_format_t
get_cairo_format_from_cogl_format (CoglPixelFormat format)
{
    switch ((int)format)
    {
    case COGL_PIXEL_FORMAT_A_8:
	return CAIRO_FORMAT_A8;
    case COGL_PIXEL_FORMAT_RGB_565:
	return CAIRO_FORMAT_RGB16_565;

    case COGL_PIXEL_FORMAT_BGRA_8888_PRE:
    case COGL_PIXEL_FORMAT_ARGB_8888_PRE:
    case COGL_PIXEL_FORMAT_RGBA_8888_PRE:
	/* Note: this is ambiguous since CAIRO_FORMAT_RGB24
	 * would also map to the same CoglPixelFormat */
	return CAIRO_FORMAT_ARGB32;

    default:
	g_warning("bad format: %x a? %d, bgr? %d, pre %d, format: %d\n",
		  format,
		  format & COGL_A_BIT,
		  format & COGL_BGR_BIT,
		  format & COGL_PREMULT_BIT,
		  format & ~(COGL_A_BIT | COGL_BGR_BIT | COGL_PREMULT_BIT));
	return CAIRO_FORMAT_INVALID;
    }
}

static CoglPixelFormat
get_cogl_format_from_cairo_format (cairo_format_t cairo_format)
{
    switch (cairo_format)
    {
    case CAIRO_FORMAT_ARGB32:
    case CAIRO_FORMAT_RGB24:
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
	return COGL_PIXEL_FORMAT_BGRA_8888_PRE;
#else
	return COGL_PIXEL_FORMAT_ARGB_8888_PRE;
#endif
    case CAIRO_FORMAT_A8:
	return COGL_PIXEL_FORMAT_A_8;
    case CAIRO_FORMAT_RGB16_565:
	return COGL_PIXEL_FORMAT_RGB_565;
    case CAIRO_FORMAT_INVALID:
    case CAIRO_FORMAT_A1:
    case CAIRO_FORMAT_RGB30:
	return 0;
    }

    g_warn_if_reached ();
    return 0;
}

static cairo_status_t
_cairo_cogl_surface_read_rect_to_image_surface (cairo_cogl_surface_t   *surface,
						cairo_rectangle_int_t  *interest,
						cairo_image_surface_t **image_out)
{
    cairo_image_surface_t *image;
    cairo_status_t status;
    cairo_format_t cairo_format;
    CoglPixelFormat cogl_format;

    /* TODO: Add cogl_texture_get_region() API so we don't have to ensure the
     * surface is bound to an fbo to read back pixels */
    status = _cairo_cogl_surface_ensure_framebuffer (surface);
    if (unlikely (status))
	return status;

    cairo_format = get_cairo_format_from_cogl_format (surface->cogl_format);
    if (cairo_format == CAIRO_FORMAT_INVALID) {
	cairo_format = CAIRO_FORMAT_ARGB32;
	cogl_format = get_cogl_format_from_cairo_format (cairo_format);
    } else {
	cogl_format = cogl_framebuffer_get_color_format (surface->framebuffer);
    }

    image = (cairo_image_surface_t *)
	cairo_image_surface_create (cairo_format, surface->width, surface->height);
    if (image->base.status)
	return image->base.status;

    /* TODO: Add cogl_framebuffer_read_pixels() API */
    cogl_push_framebuffer (surface->framebuffer);
    cogl_read_pixels (0, 0, surface->width, surface->height,
		      COGL_READ_PIXELS_COLOR_BUFFER,
		      cogl_format,
		      image->data);
    cogl_pop_framebuffer ();

    *image_out = image;

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_cogl_surface_acquire_source_image (void		         *abstract_surface,
					  cairo_image_surface_t **image_out,
					  void		        **image_extra)
{
    cairo_cogl_surface_t *surface = abstract_surface;
    cairo_status_t status;

    if (surface->texture) {
	cairo_format_t format = get_cairo_format_from_cogl_format (surface->cogl_format);
	cairo_image_surface_t *image = (cairo_image_surface_t *)
	    cairo_image_surface_create (format, surface->width, surface->height);
	if (image->base.status)
	    return image->base.status;

	cogl_texture_get_data (surface->texture,
			       cogl_texture_get_format (surface->texture),
			       0,
			       image->data);

	image->base.is_clear = FALSE;
	*image_out = image;
    } else {
	cairo_rectangle_int_t extents = {
	    0, 0, surface->width, surface->height
	};
	status = _cairo_cogl_surface_read_rect_to_image_surface (surface, &extents,
								 image_out);
	if (unlikely (status))
	    return status;
    }

    *image_extra = NULL;

    return CAIRO_STATUS_SUCCESS;
}

static void
_cairo_cogl_surface_release_source_image (void			*abstract_surface,
					  cairo_image_surface_t *image,
					  void			*image_extra)
{
    cairo_surface_destroy (&image->base);
}

static cairo_status_t
_cairo_cogl_surface_clear (cairo_cogl_surface_t *surface,
			   const cairo_color_t *color)
{
    /* Anything batched in the journal up until now is redundant... */
    _cairo_cogl_journal_discard (surface);

    /* XXX: we currently implicitly clear the depth and stencil buffer here
     * but since we use the framebuffer_discard extension when available I
     * suppose this doesn't matter too much.
     *
     * The main concern is that we want to avoid re-loading an external z
     * buffer at the start of each frame, but also many gpu architectures have
     * optimizations for how they handle the depth/stencil buffers and can get
     * upset if they aren't cleared together at the start of the frame.
     *
     * FIXME: we need a way to assert that the clip stack currently isn't
     * using the stencil buffer before clearing it here!
     */
    cogl_framebuffer_clear4f (surface->framebuffer,
			      COGL_BUFFER_BIT_COLOR |
			      COGL_BUFFER_BIT_DEPTH |
			      COGL_BUFFER_BIT_STENCIL,
			      color->red * color->alpha,
			      color->green * color->alpha,
			      color->blue * color->alpha,
			      color->alpha);
    return CAIRO_STATUS_SUCCESS;
}

cairo_status_t
_cairo_cogl_path_fixed_rectangle (cairo_path_fixed_t *path,
				  cairo_fixed_t x,
				  cairo_fixed_t y,
				  cairo_fixed_t width,
				  cairo_fixed_t height)
{
    cairo_status_t status;

    status = _cairo_path_fixed_move_to (path, x, y);
    if (unlikely (status))
	return status;

    status = _cairo_path_fixed_rel_line_to (path, width, 0);
    if (unlikely (status))
	return status;

    status = _cairo_path_fixed_rel_line_to (path, 0, height);
    if (unlikely (status))
	return status;

    status = _cairo_path_fixed_rel_line_to (path, -width, 0);
    if (unlikely (status))
	return status;

    status = _cairo_path_fixed_close_path (path);
    if (unlikely (status))
	return status;

    return CAIRO_STATUS_SUCCESS;
}

static cairo_int_status_t
_cairo_cogl_surface_paint (void                  *abstract_surface,
                           cairo_operator_t       op,
                           const cairo_pattern_t *source,
                           const cairo_clip_t    *clip)
{
    cairo_cogl_surface_t *surface;
    cairo_path_fixed_t path;
    cairo_status_t status;
    cairo_matrix_t identity;

    if (clip == NULL) {
	if (op == CAIRO_OPERATOR_CLEAR)
            return _cairo_cogl_surface_clear (abstract_surface, CAIRO_COLOR_TRANSPARENT);
	else if (source->type == CAIRO_PATTERN_TYPE_SOLID &&
                (op == CAIRO_OPERATOR_SOURCE ||
                 (op == CAIRO_OPERATOR_OVER && (((cairo_surface_t *)abstract_surface)->is_clear || _cairo_pattern_is_opaque_solid (source))))) {
            return _cairo_cogl_surface_clear (abstract_surface,
					      &((cairo_solid_pattern_t *) source)->color);
        }
    }

    /* fallback to handling the paint in terms of a fill... */

    surface = abstract_surface;

    _cairo_path_fixed_init (&path);

    status = _cairo_cogl_path_fixed_rectangle (&path, 0, 0, surface->width, surface->height);
    if (unlikely (status))
	goto BAIL;

#ifdef NEED_COGL_CONTEXT
    /* XXX: in cairo-cogl-context.c we set some sideband data on the
     * surface before issuing a fill so we need to do that here too... */
    surface->user_path = &path;
    cairo_matrix_init_identity (&identity);
    surface->ctm = &identity;
    surface->ctm_inverse = &identity;
    surface->path_is_rectangle = TRUE;
    surface->path_rectangle_x = 0;
    surface->path_rectangle_y = 0;
    surface->path_rectangle_width = surface->width;
    surface->path_rectangle_height = surface->height;
#endif

    status = _cairo_cogl_surface_fill (abstract_surface,
				       op,
				       source,
				       &path,
				       CAIRO_FILL_RULE_WINDING,
				       1,
				       CAIRO_ANTIALIAS_DEFAULT,
				       clip);
BAIL:
    _cairo_path_fixed_fini (&path);
    return status;
}

static CoglPipelineWrapMode
get_cogl_wrap_mode_for_extend (cairo_extend_t extend_mode)
{
    switch (extend_mode)
    {
    case CAIRO_EXTEND_NONE:
	return COGL_PIPELINE_WRAP_MODE_CLAMP_TO_EDGE;
    case CAIRO_EXTEND_PAD:
	return COGL_PIPELINE_WRAP_MODE_CLAMP_TO_EDGE;
    case CAIRO_EXTEND_REPEAT:
	return COGL_PIPELINE_WRAP_MODE_REPEAT;
    case CAIRO_EXTEND_REFLECT:
	/* TODO: return COGL_PIPELINE_WRAP_MODE_MIRROR; */
	return CAIRO_EXTEND_REPEAT;
    }
    assert (0); /* not reached */
    return COGL_PIPELINE_WRAP_MODE_CLAMP_TO_EDGE;
}

#if 0
/* Given an arbitrary texture, check if it's already a pot texture and simply
 * return it back if so. If not create a new pot texture, scale the old to
 * fill it, unref the old and return a pointer to the new pot texture. */
static cairo_int_status_t
_cairo_cogl_get_pot_texture (CoglContext *context,
			     CoglTexture *texture,
			     CoglTexture **pot_texture)
{
    int width = cogl_texture_get_width (texture);
    int height = cogl_texture_get_height (texture);
    int pot_width;
    int pot_height;
    CoglHandle offscreen = NULL;
    CoglTexture2D *pot = NULL;
    GError *error;

    pot_width = _cairo_cogl_util_next_p2 (width);
    pot_height = _cairo_cogl_util_next_p2 (height);

    if (pot_width == width && pot_height == height)
	return CAIRO_INT_STATUS_SUCCESS;

    for (;;) {
	error = NULL;
	pot = cogl_texture_2d_new_with_size (context,
					     pot_width,
					     pot_height,
					     cogl_texture_get_format (texture),
					     &error);
	if (pot)
	    break;
	else
	    g_error_free (error);

	if (pot_width > pot_height)
	    pot_width >>= 1;
	else
	    pot_height >>= 1;

	if (!pot_width || !pot_height)
	    break;
    }

    *pot_texture = COGL_TEXTURE (pot);

    if (!pot)
	return CAIRO_INT_STATUS_NO_MEMORY;

    /* Use the GPU to do a bilinear filtered scale from npot to pot... */
    offscreen = cogl_offscreen_new_to_texture (COGL_TEXTURE (pot));
    error = NULL;
    if (!cogl_framebuffer_allocate (COGL_FRAMEBUFFER (offscreen), &error)) {
	/* NB: if we don't pass an error then Cogl is allowed to simply abort
	 * automatically. */
	g_error_free (error);
	cogl_object_unref (pot);
	*pot_texture = NULL;
	return CAIRO_INT_STATUS_NO_MEMORY;
    }

    cogl_push_framebuffer (COGL_FRAMEBUFFER (offscreen));
    cogl_set_source_texture (texture);
    cogl_rectangle (-1, 1, 1, -1);
    cogl_pop_framebuffer ();

    cogl_object_unref (offscreen);
}
#endif

/* NB: a reference for the texture is transferred to the caller which should
 * be unrefed */
static CoglTexture *
_cairo_cogl_acquire_surface_texture (cairo_cogl_surface_t  *reference_surface,
				     cairo_surface_t	   *abstract_surface)
{
    cairo_image_surface_t *image;
    cairo_image_surface_t *acquired_image = NULL;
    void *image_extra;
    CoglPixelFormat format;
    cairo_image_surface_t *image_clone = NULL;
    CoglTexture2D *texture;
    GError *error = NULL;
    cairo_surface_t *clone;

    if (abstract_surface->device == reference_surface->base.device) {
	cairo_cogl_surface_t *surface = (cairo_cogl_surface_t *)abstract_surface;
	_cairo_cogl_surface_flush (surface);
	return surface->texture ? cogl_object_ref (surface->texture) : NULL;
    }

    if (abstract_surface->type == CAIRO_SURFACE_TYPE_COGL) {
	if (_cairo_surface_is_subsurface (abstract_surface)) {
	    cairo_cogl_surface_t *surface;

	    surface = (cairo_cogl_surface_t *)
		_cairo_surface_subsurface_get_target (abstract_surface);
	    if (surface->base.device == reference_surface->base.device)
		return surface->texture ? cogl_object_ref (surface->texture) : NULL;
	}
    }

    clone = _cairo_surface_has_snapshot (abstract_surface, &_cairo_cogl_surface_backend);
    if (clone) {
	cairo_cogl_surface_t *surface = (cairo_cogl_surface_t *)clone;
	return surface->texture ? cogl_object_ref (surface->texture) : NULL;
    }

    g_warning ("Uploading image surface to texture");

    if (_cairo_surface_is_image (abstract_surface)) {
	image = (cairo_image_surface_t *)abstract_surface;
    } else {
	cairo_status_t status = _cairo_surface_acquire_source_image (abstract_surface,
								     &acquired_image, &image_extra);
	if (unlikely (status)) {
	    g_warning ("acquire_source_image failed: %s [%d]\n",
		       cairo_status_to_string (status), status);
	    return NULL;
	}
	image = acquired_image;
    }

    format = get_cogl_format_from_cairo_format (image->format);
    if (!format)
    {
	image_clone = _cairo_image_surface_coerce (image);
	if (unlikely (image_clone->base.status)) {
	    g_warning ("image_surface_coerce failed");
	    texture = NULL;
	    goto BAIL;
	}

	format = get_cogl_format_from_cairo_format (image_clone->format);
	assert (format);
    }

    texture = cogl_texture_2d_new_from_data (to_device(reference_surface->base.device)->cogl_context,
					     image->width,
					     image->height,
					     format, /* incoming */
					     format, /* desired */
					     image->stride,
					     image->data,
					     &error);
    if (!texture) {
	g_warning ("Failed to allocate texture: %s", error->message);
	g_error_free (error);
	goto BAIL;
    }

    clone = _cairo_cogl_surface_create_full (to_device(reference_surface->base.device),
					     reference_surface->ignore_alpha,
					     NULL, COGL_TEXTURE (texture));

    _cairo_surface_attach_snapshot (abstract_surface, clone, NULL);

    /* Attaching the snapshot will take a reference on the clone surface... */
    cairo_surface_destroy (clone);

BAIL:
    if (image_clone)
	cairo_surface_destroy (&image_clone->base);
    if (acquired_image)
	_cairo_surface_release_source_image (abstract_surface, acquired_image, image_extra);

    return COGL_TEXTURE (texture);
}

/* NB: a reference for the texture is transferred to the caller which should
 * be unrefed */
static CoglTexture *
_cairo_cogl_acquire_pattern_texture (const cairo_pattern_t *pattern,
				     cairo_cogl_surface_t *destination,
				     const cairo_rectangle_int_t *extents,
				     const cairo_rectangle_int_t *sample,
				     cairo_cogl_texture_attributes_t *attributes)
{
    CoglTexture *texture = NULL;

    switch ((int)pattern->type)
    {
    case CAIRO_PATTERN_TYPE_SURFACE: {
	cairo_surface_t *surface = ((cairo_surface_pattern_t *)pattern)->surface;
	texture = _cairo_cogl_acquire_surface_texture (destination, surface);
	if (!texture)
	    return NULL;

	/* XXX: determine if it would have no effect to change the
	 * extend mode to EXTEND_PAD instead since we can simply map
	 * EXTEND_PAD to CLAMP_TO_EDGE without needing fragment shader
	 * tricks or extra border texels. */
#if 0
	/* TODO: We still need to consider HW such as SGX which doesn't have
	 * full support for NPOT textures. */
	if (pattern->extend == CAIRO_EXTEND_REPEAT || pattern->extend == CAIRO_EXTEND_REFLECT) {
	    _cairo_cogl_get_pot_texture ();
	}
#endif

	cairo_matrix_init_identity (&attributes->matrix);

	/* Convert from un-normalized source coordinates in backend
	 * coordinates to normalized texture coordinates */
	cairo_matrix_scale (&attributes->matrix,
			    1.0f / cogl_texture_get_width (texture),
			    1.0f / cogl_texture_get_height (texture));

	/* XXX: need to multiply in the pattern->matrix */

	attributes->extend = pattern->extend;
	attributes->filter = CAIRO_FILTER_BILINEAR;
	attributes->has_component_alpha = pattern->has_component_alpha;

	attributes->s_wrap = get_cogl_wrap_mode_for_extend (pattern->extend);
	attributes->t_wrap = attributes->s_wrap;

	return texture;
    }
    case CAIRO_PATTERN_TYPE_RADIAL:
    case CAIRO_PATTERN_TYPE_MESH: {
	cairo_surface_t *surface;
	cairo_matrix_t texture_matrix;

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					      extents->width, extents->height);
	if (_cairo_surface_offset_paint (surface,
					 extents->x, extents->y,
					 CAIRO_OPERATOR_SOURCE,
					 pattern, NULL)) {
	    cairo_surface_destroy (surface);
	    return NULL;
	}

	texture = _cairo_cogl_acquire_surface_texture (destination, surface);
	if (!texture)
	    goto BAIL;

	cairo_matrix_init_identity (&texture_matrix);

	/* Convert from un-normalized source coordinates in backend
	 * coordinates to normalized texture coordinates */
	cairo_matrix_scale (&texture_matrix,
			    1.0f / cogl_texture_get_width (texture),
			    1.0f / cogl_texture_get_height (texture));

	cairo_matrix_translate (&texture_matrix, -extents->x, -extents->y);

	attributes->matrix = texture_matrix;
	attributes->extend = pattern->extend;
	attributes->filter = CAIRO_FILTER_NEAREST;
	attributes->has_component_alpha = pattern->has_component_alpha;

	/* any pattern extend modes have already been dealt with... */
	attributes->s_wrap = COGL_PIPELINE_WRAP_MODE_CLAMP_TO_EDGE;
	attributes->t_wrap = attributes->s_wrap;

BAIL:
	cairo_surface_destroy (surface);

	return texture;
    }
    case CAIRO_PATTERN_TYPE_LINEAR: {
	cairo_linear_pattern_t *linear_pattern = (cairo_linear_pattern_t *)pattern;
	cairo_cogl_linear_gradient_t *gradient;
	cairo_cogl_linear_texture_entry_t *linear_texture;
	cairo_int_status_t status;
	float a, b;
	float dist;
	float scale;
	float angle;

	status = _cairo_cogl_get_linear_gradient (to_device(destination->base.device),
						  pattern->extend,
						  linear_pattern->base.n_stops,
						  linear_pattern->base.stops,
						  &gradient);
	if (unlikely (status))
	    return NULL;

	linear_texture = _cairo_cogl_linear_gradient_texture_for_extend (gradient, pattern->extend);

	attributes->extend = pattern->extend;
	attributes->filter = CAIRO_FILTER_BILINEAR;
	attributes->has_component_alpha = pattern->has_component_alpha;
	attributes->s_wrap = get_cogl_wrap_mode_for_extend (pattern->extend);
	attributes->t_wrap = COGL_PIPELINE_WRAP_MODE_REPEAT;

	cairo_matrix_init_identity (&attributes->matrix);

	a = linear_pattern->pd2.x - linear_pattern->pd1.x;
	b = linear_pattern->pd2.y - linear_pattern->pd1.y;
	dist = sqrtf (a*a + b*b);
	scale = 1.0f / dist;
	angle = - atan2f (b, a);

	cairo_matrix_rotate (&attributes->matrix, angle);
	cairo_matrix_scale (&attributes->matrix, scale, scale);

	cairo_matrix_translate (&attributes->matrix,
				-linear_pattern->pd1.x,
				-linear_pattern->pd1.y);

	/* XXX: this caught me out: cairo doesn't follow the standard
	 * maths convention for multiplying two matrices A x B - cairo
	 * does B x A so the final matrix is as if A's transforms were
	 * applied first.
	 */
	cairo_matrix_multiply (&attributes->matrix,
			       &pattern->matrix,
			       &attributes->matrix);

	return cogl_object_ref (linear_texture->texture);
    }
    default:
	g_warning ("Un-supported source type");
	return NULL;
    }
}

static void
set_layer_texture_with_attributes (CoglPipeline *pipeline,
				   int layer_index,
				   CoglTexture *texture,
				   cairo_cogl_texture_attributes_t *attributes)
{
    cogl_pipeline_set_layer_texture (pipeline, layer_index, texture);

    if (!_cairo_matrix_is_identity (&attributes->matrix)) {
	cairo_matrix_t *m = &attributes->matrix;
	float texture_matrixfv[16] = {
	    m->xx, m->yx, 0, 0,
	    m->xy, m->yy, 0, 0,
	    0, 0, 1, 0,
	    m->x0, m->y0, 0, 1
	};
	CoglMatrix texture_matrix;
	cogl_matrix_init_from_array (&texture_matrix, texture_matrixfv);
	cogl_pipeline_set_layer_matrix (pipeline, layer_index, &texture_matrix);
    }

    if (attributes->s_wrap != attributes->t_wrap) {
	cogl_pipeline_set_layer_wrap_mode_s (pipeline, layer_index, attributes->s_wrap);
	cogl_pipeline_set_layer_wrap_mode_t (pipeline, layer_index, attributes->t_wrap);
    } else
	cogl_pipeline_set_layer_wrap_mode (pipeline, layer_index, attributes->s_wrap);
}

static CoglPipeline *
get_source_mask_operator_destination_pipeline (const cairo_pattern_t *mask,
					       const cairo_pattern_t *source,
					       cairo_operator_t op,
					       cairo_cogl_surface_t *destination,
					       cairo_composite_rectangles_t *extents)
{
    cairo_cogl_template_type template_type;
    CoglPipeline *pipeline;

    switch ((int)source->type)
    {
    case CAIRO_PATTERN_TYPE_SOLID:
	template_type = mask ?
	    CAIRO_COGL_TEMPLATE_TYPE_MASK_SOLID : CAIRO_COGL_TEMPLATE_TYPE_SOLID;
	break;
    case CAIRO_PATTERN_TYPE_SURFACE:
    case CAIRO_PATTERN_TYPE_LINEAR:
    case CAIRO_PATTERN_TYPE_RADIAL:
    case CAIRO_PATTERN_TYPE_MESH:
	template_type = mask ?
	    CAIRO_COGL_TEMPLATE_TYPE_MASK_TEXTURE : CAIRO_COGL_TEMPLATE_TYPE_TEXTURE;
	break;
    default:
	g_warning ("Un-supported source type");
	return NULL;
    }

    pipeline = cogl_pipeline_copy (to_device(destination->base.device)->template_pipelines[op][template_type]);

    if (source->type == CAIRO_PATTERN_TYPE_SOLID) {
	cairo_solid_pattern_t *solid_pattern = (cairo_solid_pattern_t *)source;
	cogl_pipeline_set_color4f (pipeline,
				   solid_pattern->color.red * solid_pattern->color.alpha,
				   solid_pattern->color.green * solid_pattern->color.alpha,
				   solid_pattern->color.blue * solid_pattern->color.alpha,
				   solid_pattern->color.alpha);
    } else {
	cairo_cogl_texture_attributes_t attributes;
	CoglTexture *texture =
	    _cairo_cogl_acquire_pattern_texture (source, destination,
						 &extents->bounded,
						 &extents->source_sample_area,
						 &attributes);
	if (!texture)
	    goto BAIL;
	set_layer_texture_with_attributes (pipeline, 0, texture, &attributes);
	cogl_object_unref (texture);
    }

    if (mask) {
	if (mask->type == CAIRO_PATTERN_TYPE_SOLID) {
	    cairo_solid_pattern_t *solid_pattern = (cairo_solid_pattern_t *)mask;
	    CoglColor color;
	    cogl_color_init_from_4f (&color,
				     solid_pattern->color.red * solid_pattern->color.alpha,
				     solid_pattern->color.green * solid_pattern->color.alpha,
				     solid_pattern->color.blue * solid_pattern->color.alpha,
				     solid_pattern->color.alpha);
	    cogl_pipeline_set_layer_combine_constant (pipeline, 1, &color);
	} else {
	    cairo_cogl_texture_attributes_t attributes;
	    CoglTexture *texture =
		_cairo_cogl_acquire_pattern_texture (mask, destination,
						     &extents->bounded,
						     &extents->mask_sample_area,
						     &attributes);
	    if (!texture)
		goto BAIL;
	    set_layer_texture_with_attributes (pipeline, 1, texture, &attributes);
	    cogl_object_unref (texture);
	}
    }

    return pipeline;

BAIL:
    cogl_object_unref (pipeline);
    return NULL;
}

#if 0
CoglPrimitive *
_cairo_cogl_rectangle_new_p2t2t2 (float x,
				  float y,
				  float width,
				  float height)
{
    CoglVertexP2 vertices[] = {
	{x, y}, {x, y + height}, {x + width, y + height},
	{x, y}, {x + width, y + height}, {x + width, y}
    };
    CoglAttributeBuffer *buffer = cogl_attribute_buffer_new (sizeof (vertices));
    CoglAttribute *pos = cogl_attribute_new (buffer,
					     "cogl_position_in",
					     sizeof (CoglVertexP2),
					     0,
					     2,
					     COGL_ATTRIBUTE_TYPE_FLOAT);
    CoglAttribute *tex_coords0 = cogl_attribute_new (buffer,
						     "cogl_tex_coord0_in",
						     sizeof (CoglVertexP2),
						     0,
						     2,
						     COGL_ATTRIBUTE_TYPE_FLOAT);
    CoglAttribute *tex_coords0 = cogl_attribute_new (buffer,
						     "cogl_tex_coord0_in",
						     sizeof (CoglVertexP2),
						     0,
						     2,
						     COGL_ATTRIBUTE_TYPE_FLOAT);
    CoglPrimitive *prim;

    cogl_buffer_set_data (COGL_BUFFER (buffer), 0, vertices, sizeof (vertices));

    /* The attributes will now keep the buffer alive... */
    cogl_object_unref (buffer);

    prim = cogl_primitive_new (COGL_VERTICES_MODE_TRIANGLES,
			       6, pos, tex_coords, NULL);

    /* The primitive will now keep the attribute alive... */
    cogl_object_unref (pos);

    return prim;
}
#endif

static void
_cairo_cogl_log_clip (cairo_cogl_surface_t *surface,
		      const cairo_clip_t *clip)
{
    if (!_cairo_clip_equal (clip, surface->last_clip)) {
	_cairo_cogl_journal_log_clip (surface, clip);
	_cairo_clip_destroy (surface->last_clip);
	surface->last_clip = _cairo_clip_copy (clip);
    }
}

static void
_cairo_cogl_maybe_log_clip (cairo_cogl_surface_t *surface,
			    cairo_composite_rectangles_t *composite)
{
    cairo_clip_t *clip = composite->clip;

    if (_cairo_composite_rectangles_can_reduce_clip (composite, clip))
	clip = NULL;

    if (clip == NULL) {
	if (_cairo_composite_rectangles_can_reduce_clip (composite,
							 surface->last_clip))
	    return;
    }

    _cairo_cogl_log_clip (surface, clip);
}

static cairo_bool_t
is_operator_supported (cairo_operator_t op)
{
    switch ((int)op) {
    case CAIRO_OPERATOR_SOURCE:
    case CAIRO_OPERATOR_OVER:
    case CAIRO_OPERATOR_IN:
    case CAIRO_OPERATOR_DEST_OVER:
    case CAIRO_OPERATOR_DEST_IN:
    case CAIRO_OPERATOR_ADD:
	return TRUE;

    default:
	return FALSE;
    }
}

static cairo_int_status_t
_cairo_cogl_surface_mask (void                    *abstract_surface,
                          cairo_operator_t         op,
                          const cairo_pattern_t   *source,
                          const cairo_pattern_t   *mask,
                          const cairo_clip_t      *clip)
{
    cairo_cogl_surface_t *surface = abstract_surface;
    cairo_composite_rectangles_t extents;
    cairo_status_t status;
    CoglPipeline *pipeline;
    cairo_matrix_t identity;

    /* XXX: Use this to smoke test the acquire_source/dest_image fallback
     * paths... */
    //return CAIRO_INT_STATUS_UNSUPPORTED;

    if (!is_operator_supported (op))
	return CAIRO_INT_STATUS_UNSUPPORTED;

    status = _cairo_composite_rectangles_init_for_mask (&extents,
							&surface->base,
							op, source, mask, clip);
    if (unlikely (status))
	return status;

    pipeline = get_source_mask_operator_destination_pipeline (mask, source,
							      op, surface, &extents);
    if (!pipeline){
	status = CAIRO_INT_STATUS_UNSUPPORTED;
	goto BAIL;
    }

    _cairo_cogl_maybe_log_clip (surface, &extents);

    cairo_matrix_init_identity (&identity);
    _cairo_cogl_journal_log_rectangle (surface, pipeline,
				       extents.bounded.x,
				       extents.bounded.y,
				       extents.bounded.width,
				       extents.bounded.height,
				       2,
				       &identity);

    /* The journal will take a reference on the pipeline and clip_path... */
    cogl_object_unref (pipeline);

BAIL:
    return status;
}

static int
_cairo_cogl_source_n_layers (const cairo_pattern_t *source)
{
    switch ((int)source->type)
    {
    case CAIRO_PATTERN_TYPE_SOLID:
	return 0;
    case CAIRO_PATTERN_TYPE_LINEAR:
    case CAIRO_PATTERN_TYPE_RADIAL:
    case CAIRO_PATTERN_TYPE_MESH:
    case CAIRO_PATTERN_TYPE_SURFACE:
	return 1;
    default:
	g_warning ("Unsupported source type");
	return 0;
    }
}

static cairo_bool_t
_cairo_cogl_path_fill_meta_equal (const void *key_a, const void *key_b)
{
    const cairo_cogl_path_fill_meta_t *meta0 = key_a;
    const cairo_cogl_path_fill_meta_t *meta1 = key_b;

    return _cairo_path_fixed_equal (meta0->user_path, meta1->user_path);
}

static cairo_bool_t
_cairo_cogl_stroke_style_equal (const cairo_stroke_style_t *a,
			        const cairo_stroke_style_t *b)
{
    if (a->line_width == b->line_width &&
	a->line_cap == b->line_cap &&
	a->line_join == b->line_join &&
	a->miter_limit == b->miter_limit &&
	a->num_dashes == b->num_dashes &&
	a->dash_offset == b->dash_offset)
    {
	unsigned int i;
	for (i = 0; i < a->num_dashes; i++) {
	    if (a->dash[i] != b->dash[i])
		return FALSE;
	}
    }
    return TRUE;
}

static cairo_bool_t
_cairo_cogl_path_stroke_meta_equal (const void *key_a, const void *key_b)
{
    const cairo_cogl_path_stroke_meta_t *meta0 = key_a;
    const cairo_cogl_path_stroke_meta_t *meta1 = key_b;

    return _cairo_cogl_stroke_style_equal (&meta0->style, &meta1->style) &&
	_cairo_path_fixed_equal (meta0->user_path, meta1->user_path);
}

static cairo_cogl_path_stroke_meta_t *
_cairo_cogl_path_stroke_meta_reference (cairo_cogl_path_stroke_meta_t *meta)
{
    assert (CAIRO_REFERENCE_COUNT_HAS_REFERENCE (&meta->ref_count));

    _cairo_reference_count_inc (&meta->ref_count);

    return meta;
}

static void
_cairo_cogl_path_stroke_meta_destroy (cairo_cogl_path_stroke_meta_t *meta)
{
    assert (CAIRO_REFERENCE_COUNT_HAS_REFERENCE (&meta->ref_count));

    if (! _cairo_reference_count_dec_and_test (&meta->ref_count))
	return;

    _cairo_path_fixed_fini (meta->user_path);
    free (meta->user_path);

    _cairo_stroke_style_fini (&meta->style);

    if (meta->prim)
	cogl_object_unref (meta->prim);

    free (meta);
}

static cairo_cogl_path_stroke_meta_t *
_cairo_cogl_path_stroke_meta_lookup (cairo_cogl_device_t	*ctx,
				     unsigned long		 hash,
				     cairo_path_fixed_t		*user_path,
				     const cairo_stroke_style_t *style,
				     double			 tolerance)
{
    cairo_cogl_path_stroke_meta_t *ret;
    cairo_cogl_path_stroke_meta_t lookup;

    lookup.cache_entry.hash = hash;
    lookup.user_path = user_path;
    lookup.style = *style;
    lookup.tolerance = tolerance;

    ret = _cairo_cache_lookup (&ctx->path_stroke_staging_cache, &lookup.cache_entry);
    if (!ret)
	ret = _cairo_cache_lookup (&ctx->path_stroke_prim_cache, &lookup.cache_entry);
    return ret;
}

static void
_cairo_cogl_path_stroke_meta_set_prim_size (cairo_cogl_surface_t *surface,
					    cairo_cogl_path_stroke_meta_t *meta,
					    size_t size)
{
    /* now that we know the meta structure is associated with a primitive
     * we promote it from the staging cache into the primitive cache.
     */

    /* XXX: _cairo_cache borks if you try and remove an entry that's already
     * been evicted so we explicitly look it up first... */
    if (_cairo_cache_lookup (&to_device(surface->base.device)->path_stroke_staging_cache, &meta->cache_entry)) {
	_cairo_cogl_path_stroke_meta_reference (meta);
	_cairo_cache_remove (&to_device(surface->base.device)->path_stroke_staging_cache, &meta->cache_entry);
    }

    meta->cache_entry.size = size;
    if (_cairo_cache_insert (&to_device(surface->base.device)->path_stroke_prim_cache, &meta->cache_entry) !=
	CAIRO_STATUS_SUCCESS)
	_cairo_cogl_path_stroke_meta_destroy (meta);
}

static unsigned int
_cairo_cogl_stroke_style_hash (unsigned int hash,
			       const cairo_stroke_style_t *style)
{
    unsigned int i;
    hash = _cairo_hash_bytes (hash, &style->line_width, sizeof (style->line_width));
    hash = _cairo_hash_bytes (hash, &style->line_cap, sizeof (style->line_cap));
    hash = _cairo_hash_bytes (hash, &style->line_join, sizeof (style->line_join));
    hash = _cairo_hash_bytes (hash, &style->miter_limit, sizeof (style->miter_limit));
    hash = _cairo_hash_bytes (hash, &style->num_dashes, sizeof (style->num_dashes));
    hash = _cairo_hash_bytes (hash, &style->dash_offset, sizeof (style->dash_offset));
    for (i = 0; i < style->num_dashes; i++)
	hash = _cairo_hash_bytes (hash, &style->dash[i], sizeof (double));
    return hash;
}

static cairo_cogl_path_stroke_meta_t *
_cairo_cogl_get_path_stroke_meta (cairo_cogl_surface_t *surface,
				  const cairo_stroke_style_t *style,
				  double tolerance)
{
    unsigned long hash;
    cairo_cogl_path_stroke_meta_t *meta = NULL;
    cairo_path_fixed_t *meta_path = NULL;
    cairo_status_t status;

    if (!surface->user_path)
	return NULL;

    hash = _cairo_path_fixed_hash (surface->user_path);
    hash = _cairo_cogl_stroke_style_hash (hash, style);
    hash = _cairo_hash_bytes (hash, &tolerance, sizeof (tolerance));

    meta = _cairo_cogl_path_stroke_meta_lookup (to_device(surface->base.device), hash,
						surface->user_path, style, tolerance);
    if (meta)
	return meta;

    meta = calloc (1, sizeof (cairo_cogl_path_stroke_meta_t));
    if (!meta)
	goto BAIL;
    CAIRO_REFERENCE_COUNT_INIT (&meta->ref_count, 1);
    meta->cache_entry.hash = hash;
    meta->counter = 0;
    meta_path = malloc (sizeof (cairo_path_fixed_t));
    if (!meta_path)
	goto BAIL;
    /* FIXME: we should add a ref-counted wrapper for our user_paths
     * so we don't have to keep copying them here! */
    status = _cairo_path_fixed_init_copy (meta_path, surface->user_path);
    if (unlikely (status))
	goto BAIL;
    meta->user_path = meta_path;
    meta->ctm_inverse = *surface->ctm_inverse;

    status = _cairo_stroke_style_init_copy (&meta->style, style);
    if (unlikely (status)) {
	_cairo_path_fixed_fini (meta_path);
	goto BAIL;
    }
    meta->tolerance = tolerance;

    return meta;

BAIL:
    free (meta_path);
    free (meta);
    return NULL;
}

static cairo_int_status_t
_cairo_cogl_stroke_to_primitive (cairo_cogl_surface_t	    *surface,
				 const cairo_path_fixed_t   *path,
				 const cairo_stroke_style_t *style,
				 const cairo_matrix_t	    *ctm,
				 const cairo_matrix_t	    *ctm_inverse,
				 double			     tolerance,
				 int			     n_layers,
				 cairo_bool_t		     one_shot,
				 CoglPrimitive		   **primitive,
				 size_t			    *size)
{
    cairo_traps_t traps;
    cairo_int_status_t status;

    _cairo_traps_init (&traps);

    status = _cairo_path_fixed_stroke_to_traps (path, style, ctm, ctm_inverse, tolerance,
						&traps);
    if (unlikely (status))
	goto BAIL;

    if (traps.num_traps == 0) {
	status = CAIRO_INT_STATUS_NOTHING_TO_DO;
	goto BAIL;
    }

    *size = traps.num_traps * sizeof (CoglVertexP2) * 6;

    //g_print ("new stroke prim\n");
    *primitive = _cairo_cogl_traps_to_composite_prim (surface, &traps, n_layers, one_shot);
    if (!*primitive) {
	status = CAIRO_INT_STATUS_NO_MEMORY;
	goto BAIL;
    }

BAIL:
    _cairo_traps_fini (&traps);
    return status;
}

static cairo_int_status_t
_cairo_cogl_surface_stroke (void                       *abstract_surface,
			    cairo_operator_t            op,
			    const cairo_pattern_t      *source,
			    const cairo_path_fixed_t   *path,
			    const cairo_stroke_style_t *style,
			    const cairo_matrix_t       *ctm,
			    const cairo_matrix_t       *ctm_inverse,
			    double                      tolerance,
			    cairo_antialias_t           antialias,
			    const cairo_clip_t         *clip)
{
    cairo_cogl_surface_t *surface = (cairo_cogl_surface_t *)abstract_surface;
    cairo_composite_rectangles_t extents;
    CoglPipeline *pipeline;
    cairo_status_t status;
#ifdef ENABLE_PATH_CACHE
    cairo_cogl_path_stroke_meta_t *meta = NULL;
    cairo_matrix_t transform_matrix;
#endif
    cairo_matrix_t *transform = NULL;
    gboolean one_shot = TRUE;
    CoglPrimitive *prim = NULL;
    cairo_bool_t new_prim = FALSE;

    if (! is_operator_supported (op))
	return CAIRO_INT_STATUS_UNSUPPORTED;

    /* FIXME - support unbounded operators */
    if (!_cairo_operator_bounded_by_mask (op)) {
	/* Currently IN this is the only unbounded operator we aim to support
	 * in cairo-cogl. */
	assert (op == CAIRO_OPERATOR_IN);
	g_warning ("FIXME: handle stroking with unbounded operators!");
	return CAIRO_INT_STATUS_UNSUPPORTED;
    }

    status = _cairo_composite_rectangles_init_for_stroke (&extents,
							  &surface->base,
							  op, source, path,
							  style,
							  ctm,
							  clip);
    if (unlikely (status))
	return status;

#ifdef ENABLE_PATH_CACHE
    /* FIXME: we are currently leaking the meta state if we don't reach
     * the cache_insert at the end. */
    meta = _cairo_cogl_get_path_stroke_meta (surface, style, tolerance);
    if (meta) {
	prim = meta->prim;
	if (prim) {
	    cairo_matrix_multiply (&transform_matrix, &meta->ctm_inverse, surface->ctm);
	    transform = &transform_matrix;
	} else if (meta->counter++ > 10)
	    one_shot = FALSE;
    }
#endif

    if (!prim) {
	int n_layers = _cairo_cogl_source_n_layers (source);
	size_t prim_size = 0;
	status = _cairo_cogl_stroke_to_primitive (surface, path, style,
						  ctm, ctm_inverse, tolerance,
						  n_layers, one_shot,
						  &prim, &prim_size);
	if (unlikely (status))
	    return status;
	new_prim = TRUE;
#if defined (ENABLE_PATH_CACHE)
	if (meta) {
	    meta->prim = cogl_object_ref (prim);
	    _cairo_cogl_path_stroke_meta_set_prim_size (surface, meta, prim_size);
	}
#endif
    }

    pipeline = get_source_mask_operator_destination_pipeline (NULL, source,
							      op, surface, &extents);
    if (!pipeline)
	return CAIRO_INT_STATUS_UNSUPPORTED;

    _cairo_cogl_maybe_log_clip (surface, &extents);

    _cairo_cogl_journal_log_primitive (surface, pipeline, prim, transform);

    /* The journal will take a reference on the pipeline and primitive... */
    cogl_object_unref (pipeline);
    if (new_prim)
	cogl_object_unref (prim);

    return CAIRO_INT_STATUS_SUCCESS;
}

static cairo_cogl_path_fill_meta_t *
_cairo_cogl_path_fill_meta_reference (cairo_cogl_path_fill_meta_t *meta)
{
    assert (CAIRO_REFERENCE_COUNT_HAS_REFERENCE (&meta->ref_count));

    _cairo_reference_count_inc (&meta->ref_count);

    return meta;
}

static void
_cairo_cogl_path_fill_meta_destroy (cairo_cogl_path_fill_meta_t *meta)
{
    assert (CAIRO_REFERENCE_COUNT_HAS_REFERENCE (&meta->ref_count));

    if (! _cairo_reference_count_dec_and_test (&meta->ref_count))
	return;

    _cairo_path_fixed_fini (meta->user_path);
    free (meta->user_path);

    if (meta->prim)
	cogl_object_unref (meta->prim);

    free (meta);
}

static cairo_cogl_path_fill_meta_t *
_cairo_cogl_path_fill_meta_lookup (cairo_cogl_device_t	*ctx,
				   unsigned long	 hash,
				   cairo_path_fixed_t	*user_path)
{
    cairo_cogl_path_fill_meta_t *ret;
    cairo_cogl_path_fill_meta_t lookup;

    lookup.cache_entry.hash = hash;
    lookup.user_path = user_path;

    ret = _cairo_cache_lookup (&ctx->path_fill_staging_cache, &lookup.cache_entry);
    if (!ret)
	ret = _cairo_cache_lookup (&ctx->path_fill_prim_cache, &lookup.cache_entry);
    return ret;
}

static void
_cairo_cogl_path_fill_meta_set_prim_size (cairo_cogl_surface_t *surface,
					  cairo_cogl_path_fill_meta_t *meta,
					  size_t size)
{
    /* now that we know the meta structure is associated with a primitive
     * we promote it from the staging cache into the primitive cache.
     */

    /* XXX: _cairo_cache borks if you try and remove an entry that's already
     * been evicted so we explicitly look it up first... */
    if (_cairo_cache_lookup (&to_device(surface->base.device)->path_fill_staging_cache, &meta->cache_entry)) {
	_cairo_cogl_path_fill_meta_reference (meta);
	_cairo_cache_remove (&to_device(surface->base.device)->path_fill_staging_cache, &meta->cache_entry);
    }

    meta->cache_entry.size = size;
    if (_cairo_cache_insert (&to_device(surface->base.device)->path_fill_prim_cache, &meta->cache_entry) !=
	CAIRO_STATUS_SUCCESS)
	_cairo_cogl_path_fill_meta_destroy (meta);
}

static cairo_cogl_path_fill_meta_t *
_cairo_cogl_get_path_fill_meta (cairo_cogl_surface_t *surface)
{
    unsigned long hash;
    cairo_cogl_path_fill_meta_t *meta = NULL;
    cairo_path_fixed_t *meta_path = NULL;
    cairo_status_t status;

    if (!surface->user_path)
	return NULL;

    hash = _cairo_path_fixed_hash (surface->user_path);

    meta = _cairo_cogl_path_fill_meta_lookup (to_device(surface->base.device),
					      hash, surface->user_path);
    if (meta)
	return meta;

    meta = calloc (1, sizeof (cairo_cogl_path_fill_meta_t));
    if (!meta)
	goto BAIL;
    meta->cache_entry.hash = hash;
    meta->counter = 0;
    CAIRO_REFERENCE_COUNT_INIT (&meta->ref_count, 1);
    meta_path = malloc (sizeof (cairo_path_fixed_t));
    if (!meta_path)
	goto BAIL;
    /* FIXME: we should add a ref-counted wrapper for our user_paths
     * so we don't have to keep copying them here! */
    status = _cairo_path_fixed_init_copy (meta_path, surface->user_path);
    if (unlikely (status))
	goto BAIL;
    meta->user_path = meta_path;
    meta->ctm_inverse = *surface->ctm_inverse;

    /* To start with - until we associate a CoglPrimitive with the meta
     * structure - we keep the meta in a staging structure until we
     * see whether it actually gets re-used. */
    meta->cache_entry.size = 1;
    if (_cairo_cache_insert (&to_device(surface->base.device)->path_fill_staging_cache, &meta->cache_entry) !=
	CAIRO_STATUS_SUCCESS)
	_cairo_cogl_path_fill_meta_destroy (meta);

    return meta;

BAIL:
    free (meta_path);
    free (meta);
    return NULL;
}

static cairo_int_status_t
_cairo_cogl_surface_fill (void			    *abstract_surface,
                          cairo_operator_t	     op,
                          const cairo_pattern_t	    *source,
                          const cairo_path_fixed_t  *path,
                          cairo_fill_rule_t	     fill_rule,
                          double		     tolerance,
                          cairo_antialias_t	     antialias,
                          const cairo_clip_t	    *clip)
{
    cairo_cogl_surface_t *surface = abstract_surface;
    cairo_composite_rectangles_t extents;
    cairo_status_t status;
#ifdef ENABLE_PATH_CACHE
    cairo_cogl_path_fill_meta_t *meta = NULL;
    cairo_matrix_t transform_matrix;
#endif
    cairo_matrix_t *transform = NULL;
    cairo_bool_t one_shot = TRUE;
    CoglPrimitive *prim = NULL;
    cairo_bool_t new_prim = FALSE;
    CoglPipeline *pipeline;

    if (! is_operator_supported (op))
	return CAIRO_INT_STATUS_UNSUPPORTED;

    /* FIXME - support unbounded operators */
    if (!_cairo_operator_bounded_by_mask (op)) {
	/* Currently IN this is the only unbounded operator we aim to support
	 * in cairo-cogl. */
	assert (op == CAIRO_OPERATOR_IN);
	g_warning ("FIXME: handle filling with unbounded operators!");
	return CAIRO_INT_STATUS_UNSUPPORTED;
    }

    status = _cairo_composite_rectangles_init_for_fill (&extents,
							&surface->base,
							op, source, path,
							clip);
    if (unlikely (status))
	return status;

#ifndef FILL_WITH_COGL_PATH
#ifdef ENABLE_PATH_CACHE
    meta = _cairo_cogl_get_path_fill_meta (surface);
    if (meta) {
	prim = meta->prim;
	if (prim) {
	    cairo_matrix_multiply (&transform_matrix, &meta->ctm_inverse, surface->ctm);
	    transform = &transform_matrix;
	} else if (meta->counter++ > 10)
	    one_shot = FALSE;
    }
#endif /* ENABLE_PATH_CACHE */

    if (!prim) {
	int n_layers = _cairo_cogl_source_n_layers (source);
	size_t prim_size;
	status = _cairo_cogl_fill_to_primitive (surface, path, fill_rule, tolerance,
						one_shot, n_layers, &prim, &prim_size);
	if (unlikely (status))
	    return status;
	new_prim = TRUE;
#ifdef ENABLE_PATH_CACHE
	if (meta) {
	    meta->prim = cogl_object_ref (prim);
	    _cairo_cogl_path_fill_meta_set_prim_size (surface, meta, prim_size);
	}
#endif /* ENABLE_PATH_CACHE */
    }

#endif /* !FILL_WITH_COGL_PATH */

    pipeline = get_source_mask_operator_destination_pipeline (NULL, source,
							      op, surface, &extents);
    if (!pipeline)
	return CAIRO_INT_STATUS_UNSUPPORTED;

    _cairo_cogl_maybe_log_clip (surface, &extents);

#ifndef FILL_WITH_COGL_PATH
    _cairo_cogl_journal_log_primitive (surface, pipeline, prim, transform);
    /* The journal will take a reference on the prim */
    if (new_prim)
	cogl_object_unref (prim);
#else
    CoglPath * cogl_path = _cairo_cogl_util_path_from_cairo (path, fill_rule, tolerance);
    _cairo_cogl_journal_log_path (surface, pipeline, cogl_path);
    cogl_object_unref (cogl_path);
#endif

    /* The journal will take a reference on the pipeline... */
    cogl_object_unref (pipeline);

    return CAIRO_INT_STATUS_SUCCESS;
}

cairo_int_status_t
_cairo_cogl_surface_fill_rectangle (void		     *abstract_surface,
				    cairo_operator_t	      op,
				    const cairo_pattern_t    *source,
				    double		      x,
				    double		      y,
				    double		      width,
				    double		      height,
				    cairo_matrix_t	     *ctm,
				    const cairo_clip_t	     *clip)
{
    cairo_cogl_surface_t *surface = abstract_surface;
    CoglPipeline *pipeline;

    if (! is_operator_supported (op))
	return CAIRO_INT_STATUS_UNSUPPORTED;

    /* FIXME - support unbounded operators */
    if (!_cairo_operator_bounded_by_mask (op)) {
	/* Currently IN this is the only unbounded operator we aim to support
	 * in cairo-cogl. */
	assert (op == CAIRO_OPERATOR_IN);
	g_warning ("FIXME: handle filling with unbounded operators!");
	return CAIRO_INT_STATUS_UNSUPPORTED;
    }

    /* FIXME */
#if 0
    status = _cairo_composite_rectangles_init_for_fill_rectangle (&extents,
								  &surface->base,
								  op, source, path,
								  clip);
    if (unlikely (status))
	return status;
#endif

    if (source->type == CAIRO_PATTERN_TYPE_SOLID) {
	double x1 = x;
	double y1 = y;
	double x2 = x1 + width;
	double y2 = y1 + height;

	pipeline = get_source_mask_operator_destination_pipeline (NULL, source,
								  op, surface, NULL);
	if (!pipeline)
	    return CAIRO_INT_STATUS_UNSUPPORTED;

	_cairo_cogl_log_clip (surface, clip);

	_cairo_cogl_journal_log_rectangle (surface,
					   pipeline,
					   x1, y1, x2, y2,
					   0,
					   ctm);
	return CAIRO_INT_STATUS_SUCCESS;
    } else
	return CAIRO_INT_STATUS_UNSUPPORTED;

    /* TODO:
     * We need to acquire the textures here, look at the corresponding
     * attributes and see if this can be trivially handled by logging
     * a textured rectangle only needing simple scaling or translation
     * of texture coordinates.
     *
     * At this point we should also aim to remap the default
     * EXTEND_NONE mode to EXTEND_PAD which is more efficient if we
     * know it makes no difference either way since we can map that to
     * CLAMP_TO_EDGE.
     */
}

static cairo_int_status_t
_cairo_cogl_surface_show_glyphs (void			*surface,
                                 cairo_operator_t        op,
                                 const cairo_pattern_t	*source,
                                 cairo_glyph_t          *glyphs,
                                 int                     num_glyphs,
                                 cairo_scaled_font_t	*scaled_font,
                                 const cairo_clip_t     *clip)
{
    return CAIRO_INT_STATUS_UNSUPPORTED;
}

const cairo_surface_backend_t _cairo_cogl_surface_backend = {
    CAIRO_SURFACE_TYPE_COGL,
    _cairo_cogl_surface_finish,
#ifdef NEED_COGL_CONTEXT
    _cairo_cogl_context_create,
#else
    _cairo_default_context_create,
#endif

    _cairo_cogl_surface_create_similar,
    NULL, /* create similar image */
    NULL, /* map to image */
    NULL, /* unmap image */

    _cairo_surface_default_source,
    _cairo_cogl_surface_acquire_source_image,
    _cairo_cogl_surface_release_source_image,
    NULL, /* snapshot */

    NULL, /* copy_page */
    NULL, /* show_page */

    _cairo_cogl_surface_get_extents,
    NULL, /* get_font_options */

    _cairo_cogl_surface_flush, /* flush */
    NULL, /* mark_dirty_rectangle */

    _cairo_cogl_surface_paint,
    _cairo_cogl_surface_mask,
    _cairo_cogl_surface_stroke,
    _cairo_cogl_surface_fill,
    NULL, /* fill_stroke*/
    _cairo_surface_fallback_glyphs,
};

static cairo_surface_t *
_cairo_cogl_surface_create_full (cairo_cogl_device_t *dev,
				 cairo_bool_t ignore_alpha,
				 CoglFramebuffer *framebuffer,
				 CoglTexture *texture)
{
    cairo_cogl_surface_t *surface;
    cairo_status_t status;

    status = cairo_device_acquire (&dev->base);
    if (unlikely (status))
	return _cairo_surface_create_in_error (status);

    surface = malloc (sizeof (cairo_cogl_surface_t));
    if (unlikely (surface == NULL))
        return _cairo_surface_create_in_error (_cairo_error (CAIRO_STATUS_NO_MEMORY));

    surface->ignore_alpha = ignore_alpha;

    surface->framebuffer = framebuffer;
    if (framebuffer) {
	surface->width = cogl_framebuffer_get_width (framebuffer);
	surface->height = cogl_framebuffer_get_height (framebuffer);
	surface->cogl_format = cogl_framebuffer_get_color_format (framebuffer);
	cogl_object_ref (framebuffer);
    }

    /* FIXME: If texture == NULL and we are given an offscreen framebuffer
     * then we want a way to poke inside the framebuffer to get a texture */
    surface->texture = texture;
    if (texture) {
	if (!framebuffer) {
	    surface->width = cogl_texture_get_width (texture);
	    surface->height = cogl_texture_get_height (texture);
	    surface->cogl_format = cogl_texture_get_format (texture);
	}
	cogl_object_ref (texture);
    }

    assert(surface->width && surface->height);

    surface->journal = NULL;

    surface->buffer_stack = NULL;
    surface->buffer_stack_size = 4096;

    surface->last_clip = NULL;

    surface->n_clip_updates_per_frame = 0;

    _cairo_surface_init (&surface->base,
                         &_cairo_cogl_surface_backend,
                         &dev->base,
                         CAIRO_CONTENT_COLOR_ALPHA);

    return &surface->base;
}

cairo_surface_t *
cairo_cogl_surface_create (cairo_device_t  *abstract_device,
			   CoglFramebuffer *framebuffer)
{
    cairo_cogl_device_t *dev = (cairo_cogl_device_t *)abstract_device;

    if (abstract_device == NULL)
	return _cairo_surface_create_in_error (CAIRO_STATUS_DEVICE_ERROR);

    if (abstract_device->status)
	return _cairo_surface_create_in_error (abstract_device->status);

    if (abstract_device->backend->type != CAIRO_DEVICE_TYPE_COGL)
	return _cairo_surface_create_in_error (_cairo_error (CAIRO_STATUS_SURFACE_TYPE_MISMATCH));

    return _cairo_cogl_surface_create_full (dev, FALSE, framebuffer, NULL);
}
slim_hidden_def (cairo_cogl_surface_create);

CoglFramebuffer *
cairo_cogl_surface_get_framebuffer (cairo_surface_t *abstract_surface)
{
    cairo_cogl_surface_t *surface;

    if (abstract_surface->backend != &_cairo_cogl_surface_backend) {
        _cairo_error_throw (CAIRO_STATUS_SURFACE_TYPE_MISMATCH);
        return NULL;
    }

    surface = (cairo_cogl_surface_t *) abstract_surface;

    return surface->framebuffer;
}
slim_hidden_def (cairo_cogl_surface_get_framebuffer);

CoglTexture *
cairo_cogl_surface_get_texture (cairo_surface_t *abstract_surface)
{
    cairo_cogl_surface_t *surface;

    if (abstract_surface->backend != &_cairo_cogl_surface_backend) {
        _cairo_error_throw (CAIRO_STATUS_SURFACE_TYPE_MISMATCH);
        return NULL;
    }

    surface = (cairo_cogl_surface_t *) abstract_surface;

    return surface->texture;
}
slim_hidden_def (cairo_cogl_surface_get_texture);

static cairo_status_t
_cairo_cogl_device_flush (void *device)
{
    cairo_status_t status;

    status = cairo_device_acquire (device);
    if (unlikely (status))
	return status;

    /* XXX: we don't need to flush Cogl here, we just need to flush
     * any batching we do of compositing primitives. */

    cairo_device_release (device);

    return CAIRO_STATUS_SUCCESS;
}

static void
_cairo_cogl_device_finish (void *device)
{
    cairo_status_t status;

    status = cairo_device_acquire (device);
    if (unlikely (status))
	return;

    /* XXX: Drop references to external resources */

    cairo_device_release (device);
}

static void
_cairo_cogl_device_destroy (void *device)
{
    cairo_cogl_device_t *dev = device;

    /* FIXME: Free stuff! */

    g_free (dev);
}

static const cairo_device_backend_t _cairo_cogl_device_backend = {
    CAIRO_DEVICE_TYPE_COGL,

    NULL, /* lock */
    NULL, /* unlock */

    _cairo_cogl_device_flush,
    _cairo_cogl_device_finish,
    _cairo_cogl_device_destroy,
};

static cairo_bool_t
set_blend (CoglPipeline *pipeline, const char *blend_string)
{
    GError *error = NULL;
    if (!cogl_pipeline_set_blend (pipeline, blend_string, &error)) {
	g_warning ("Unsupported blend string with current gpu/driver: %s", blend_string);
	g_error_free (error);
	return FALSE;
    }
    return TRUE;
}

static cairo_bool_t
_cairo_cogl_setup_op_state (CoglPipeline *pipeline, cairo_operator_t op)
{
    cairo_bool_t status = FALSE;

    switch ((int)op)
    {
    case CAIRO_OPERATOR_SOURCE:
	status = set_blend (pipeline, "RGBA = ADD (SRC_COLOR, 0)");
	break;
    case CAIRO_OPERATOR_OVER:
	status = set_blend (pipeline, "RGBA = ADD (SRC_COLOR, DST_COLOR * (1 - SRC_COLOR[A]))");
	break;
    case CAIRO_OPERATOR_IN:
	status = set_blend (pipeline, "RGBA = ADD (SRC_COLOR * DST_COLOR[A], 0)");
	break;
    case CAIRO_OPERATOR_DEST_OVER:
	status = set_blend (pipeline, "RGBA = ADD (SRC_COLOR * (1 - DST_COLOR[A]), DST_COLOR)");
	break;
    case CAIRO_OPERATOR_DEST_IN:
	status = set_blend (pipeline, "RGBA = ADD (0, DST_COLOR * SRC_COLOR[A])");
	break;
    case CAIRO_OPERATOR_ADD:
	status = set_blend (pipeline, "RGBA = ADD (SRC_COLOR, DST_COLOR)");
	break;
    }

    return status;
}

static void
create_templates_for_op (cairo_cogl_device_t *dev, cairo_operator_t op)
{
    CoglPipeline *base = cogl_pipeline_new ();
    CoglPipeline *pipeline;
    CoglColor color;

    if (!_cairo_cogl_setup_op_state (base, op)) {
	cogl_object_unref (base);
	return;
    }

    dev->template_pipelines[op][CAIRO_COGL_TEMPLATE_TYPE_SOLID] = base;

    pipeline = cogl_pipeline_copy (base);
    cogl_pipeline_set_layer_texture (pipeline, 0, dev->dummy_texture);
    dev->template_pipelines[op][CAIRO_COGL_TEMPLATE_TYPE_TEXTURE] = pipeline;

    pipeline = cogl_pipeline_copy (base);
    cogl_pipeline_set_layer_combine (pipeline, 1,
                                     "RGBA = MODULATE (PREVIOUS, CONSTANT[A])",
                                     NULL);
    cogl_pipeline_set_layer_combine_constant (pipeline, 1, &color);
    cogl_pipeline_set_layer_texture (pipeline, 1, dev->dummy_texture);
    dev->template_pipelines[op][CAIRO_COGL_TEMPLATE_TYPE_MASK_SOLID] = pipeline;

    pipeline = cogl_pipeline_copy (base);
    cogl_pipeline_set_layer_combine (pipeline, 1,
                                     "RGBA = MODULATE (PREVIOUS, TEXTURE[A])",
                                     NULL);
    cogl_pipeline_set_layer_texture (pipeline, 1, dev->dummy_texture);
    dev->template_pipelines[op][CAIRO_COGL_TEMPLATE_TYPE_MASK_TEXTURE] = pipeline;
}

cairo_device_t *
cairo_cogl_device_create (CoglContext *cogl_context)
{
    cairo_cogl_device_t *dev = g_new0 (cairo_cogl_device_t, 1);
    cairo_status_t status;

    dev->backend_vtable_initialized = FALSE;

    dev->cogl_context = cogl_context;

    dev->dummy_texture = cogl_texture_new_with_size (1, 1,
						     COGL_TEXTURE_NO_SLICING,
						     COGL_PIXEL_FORMAT_ANY);
    if (!dev->dummy_texture)
	goto ERROR;

    memset (dev->template_pipelines, 0, sizeof (dev->template_pipelines));
    create_templates_for_op (dev, CAIRO_OPERATOR_SOURCE);
    create_templates_for_op (dev, CAIRO_OPERATOR_OVER);
    create_templates_for_op (dev, CAIRO_OPERATOR_IN);
    create_templates_for_op (dev, CAIRO_OPERATOR_DEST_OVER);
    create_templates_for_op (dev, CAIRO_OPERATOR_DEST_IN);
    create_templates_for_op (dev, CAIRO_OPERATOR_ADD);

    status = _cairo_cache_init (&dev->linear_cache,
                                _cairo_cogl_linear_gradient_equal,
                                NULL,
                                (cairo_destroy_func_t) _cairo_cogl_linear_gradient_destroy,
                                CAIRO_COGL_LINEAR_GRADIENT_CACHE_SIZE);
    if (unlikely (status))
	return _cairo_device_create_in_error(status);

    status = _cairo_cache_init (&dev->path_fill_staging_cache,
                                _cairo_cogl_path_fill_meta_equal,
                                NULL,
                                (cairo_destroy_func_t) _cairo_cogl_path_fill_meta_destroy,
                                1000);

    status = _cairo_cache_init (&dev->path_stroke_staging_cache,
                                _cairo_cogl_path_stroke_meta_equal,
                                NULL,
                                (cairo_destroy_func_t) _cairo_cogl_path_stroke_meta_destroy,
                                1000);

    status = _cairo_cache_init (&dev->path_fill_prim_cache,
                                _cairo_cogl_path_fill_meta_equal,
                                NULL,
                                (cairo_destroy_func_t) _cairo_cogl_path_fill_meta_destroy,
                                CAIRO_COGL_PATH_META_CACHE_SIZE);

    status = _cairo_cache_init (&dev->path_stroke_prim_cache,
                                _cairo_cogl_path_stroke_meta_equal,
                                NULL,
                                (cairo_destroy_func_t) _cairo_cogl_path_stroke_meta_destroy,
                                CAIRO_COGL_PATH_META_CACHE_SIZE);

    _cairo_device_init (&dev->base, &_cairo_cogl_device_backend);
    return &dev->base;

ERROR:
    g_free (dev);
    return NULL;
}
slim_hidden_def (cairo_cogl_device_create);

void
cairo_cogl_surface_end_frame (cairo_surface_t *abstract_surface)
{
    cairo_cogl_surface_t *surface = (cairo_cogl_surface_t *)abstract_surface;
    cairo_surface_flush (abstract_surface);

    //g_print ("n_clip_update_per_frame = %d\n", surface->n_clip_updates_per_frame);
    surface->n_clip_updates_per_frame = 0;
}
slim_hidden_def (cairo_cogl_surface_end_frame);
