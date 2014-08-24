/* -*- Mode: c; tab-width: 8; c-basic-offset: 4; indent-tabs-mode: t; -*- */
/* cairo - a vector graphics library with display and print output
 *
 * Copyright © 2002 University of Southern California
 * Copyright © 2005 Red Hat, Inc.
 * Copyright © 2009 Chris Wilson
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
 * http://www.mozilla.org/MPL/
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY
 * OF ANY KIND, either express or implied. See the LGPL or the MPL for
 * the specific language governing rights and limitations.
 *
 * The Original Code is the cairo graphics library.
 *
 * The Initial Developer of the Original Code is University of Southern
 * California.
 *
 * Contributor(s):
 *	Carl D. Worth <cworth@cworth.org>
 *	Kristian Høgsberg <krh@redhat.com>
 *	Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "cairoint.h"
#include "cairo-clip-private.h"
#include "cairo-error-private.h"
#include "cairo-freed-pool-private.h"
#include "cairo-gstate-private.h"
#include "cairo-path-fixed-private.h"
#include "cairo-pattern-private.h"
#include "cairo-composite-rectangles-private.h"
#include "cairo-region-private.h"

cairo_status_t
_cairo_clip_combine_with_surface (const cairo_clip_t *clip,
				  cairo_surface_t *dst,
				  int dst_x, int dst_y)
{
    cairo_clip_path_t *copy_path;
    cairo_clip_path_t *clip_path;
    cairo_clip_t *copy;
    cairo_status_t status = CAIRO_STATUS_SUCCESS;

    copy = _cairo_clip_copy_with_translation (clip, -dst_x, -dst_y);
    copy_path = copy->path;
    copy->path = NULL;

    if (copy->boxes) {
	status = _cairo_surface_paint (dst,
				       CAIRO_OPERATOR_IN,
				       &_cairo_pattern_white.base,
				       copy);
    }

    clip = NULL;
    if (_cairo_clip_is_region (copy))
	clip = copy;
    clip_path = copy_path;
    while (status == CAIRO_STATUS_SUCCESS && clip_path) {
	status = _cairo_surface_fill (dst,
				      CAIRO_OPERATOR_IN,
				      &_cairo_pattern_white.base,
				      &clip_path->path,
				      clip_path->fill_rule,
				      clip_path->tolerance,
				      clip_path->antialias,
				      clip);
	clip_path = clip_path->prev;
    }

    copy->path = copy_path;
    _cairo_clip_destroy (copy);
    return status;
}

cairo_surface_t *
_cairo_clip_get_surface (const cairo_clip_t *clip,
			 cairo_surface_t *target,
			 int *tx, int *ty)
{
    cairo_surface_t *surface;
    cairo_status_t status;
    cairo_clip_t *copy;
    cairo_clip_path_t *copy_path, *clip_path;

    surface = _cairo_surface_create_similar_solid (target,
						   CAIRO_CONTENT_ALPHA,
						   clip->extents.width,
						   clip->extents.height,
						   CAIRO_COLOR_WHITE);
    if (unlikely (surface->status))
	return surface;

    copy = _cairo_clip_copy_with_translation (clip,
					      -clip->extents.x,
					      -clip->extents.y);
    copy_path = copy->path;
    copy->path = NULL;

    status = CAIRO_STATUS_SUCCESS;
    clip_path = copy_path;
    while (status == CAIRO_STATUS_SUCCESS && clip_path) {
	status = _cairo_surface_fill (surface,
				      CAIRO_OPERATOR_IN,
				      &_cairo_pattern_white.base,
				      &clip_path->path,
				      clip_path->fill_rule,
				      clip_path->tolerance,
				      clip_path->antialias,
				      copy);
	clip_path = clip_path->prev;
    }

    copy->path = copy_path;
    _cairo_clip_destroy (copy);

    if (unlikely (status)) {
	cairo_surface_destroy (surface);
	return _cairo_surface_create_in_error (status);
    }

    *tx = clip->extents.x;
    *ty = clip->extents.y;
    return surface;
}

cairo_surface_t *
_cairo_clip_get_image (const cairo_clip_t *clip,
		       cairo_surface_t *target,
		       const cairo_rectangle_int_t *extents)
{
    cairo_surface_t *surface;
    cairo_status_t status;

    surface = cairo_surface_create_similar_image (target,
						  CAIRO_FORMAT_A8,
						  extents->width,
						  extents->height);
    if (unlikely (surface->status))
	return surface;

    status = _cairo_surface_paint (surface, CAIRO_OPERATOR_SOURCE,
				   &_cairo_pattern_white.base, NULL);
    if (likely (status == CAIRO_STATUS_SUCCESS))
	status = _cairo_clip_combine_with_surface (clip, surface,
						   extents->x, extents->y);

    if (unlikely (status)) {
	cairo_surface_destroy (surface);
	surface = _cairo_surface_create_in_error (status);
    }

    return surface;
}
