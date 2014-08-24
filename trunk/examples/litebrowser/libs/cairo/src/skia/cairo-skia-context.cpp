/* -*- Mode: c; c-basic-offset: 4; indent-tabs-mode: t; tab-width: 8; -*- */
/* cairo - a vector graphics library with display and print output
 *
 * Copyright © 2002 University of Southern California
 * Copyright © 2005 Red Hat, Inc.
 * Copyright © 2010 Intel Corporation
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
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
 *	Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "cairoint.h"

#include "cairo-private.h"
#include "cairo-error-private.h"
#include "cairo-arc-private.h"
#include "cairo-backend-private.h"
#include "cairo-default-context-private.h"
#include "cairo-freed-pool-private.h"
#include "cairo-gstate-private.h"
#include "cairo-image-surface-inline.h"
#include "cairo-path-private.h"
#include "cairo-pattern-private.h"
#include "cairo-skia-private.h"
#include "cairo-surface-backend-private.h"

#include <SkShader.h>
#include <SkColorShader.h>
#include <SkGradientShader.h>
#include <SkDashPathEffect.h>

#if !defined(INFINITY)
#define INFINITY HUGE_VAL
#endif

#if (CAIRO_FIXED_BITS == 32) && (CAIRO_FIXED_FRAC_BITS == 16) && defined(SK_SCALAR_IS_FIXED)
# define CAIRO_FIXED_TO_SK_SCALAR(x)  (x)
#elif defined(SK_SCALAR_IS_FIXED)
/* This can be done better, but this will do for now */
# define CAIRO_FIXED_TO_SK_SCALAR(x)  SkFloatToScalar(_cairo_fixed_to_double(x))
#else
# define CAIRO_FIXED_TO_SK_SCALAR(x)  SkFloatToScalar(_cairo_fixed_to_double(x))
#endif

#define UNSUPPORTED


static freed_pool_t context_pool;

static void
_cairo_skia_context_destroy (void *abstract_cr)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    cr->path->reset ();
    cr->paint->reset ();

    delete cr->canvas;

    cairo_surface_destroy (&cr->target->image.base);
    cairo_surface_destroy (&cr->original->image.base);

    if (cr->source != NULL) {
	if (cr->source_image != NULL) {
	    _cairo_surface_release_source_image (cr->source, cr->source_image, cr->source_extra);
	    cr->source_image = NULL;
	}
	cairo_surface_destroy (cr->source);
	cr->source = NULL;
    }

    _cairo_fini (&cr->base);

    _freed_pool_put (&context_pool, cr);
}

static cairo_surface_t *
_cairo_skia_context_get_original_target (void *abstract_cr)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    return &cr->original->image.base;
}

static cairo_surface_t *
_cairo_skia_context_get_current_target (void *abstract_cr)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    return &cr->target->image.base;
}

static cairo_status_t
_cairo_skia_context_save (void *abstract_cr)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    cr->canvas->save ();
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_restore (void *abstract_cr)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    cr->canvas->restore ();
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_push_group (void *abstract_cr, cairo_content_t content)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;
    cairo_surface_t *group_surface;
    cairo_status_t status;
    int width, height;

    //clip = _cairo_gstate_get_clip (cr->gstate);
    width = cr->target->image.width;
    height = cr->target->image.height;
    group_surface = cr->target->image.base.backend->create_similar (&cr->target->image.base,
								    content, width, height);

#if 0
    /* Set device offsets on the new surface so that logically it appears at
     * the same location on the parent surface -- when we pop_group this,
     * the source pattern will get fixed up for the appropriate target surface
     * device offsets, so we want to set our own surface offsets from /that/,
     * and not from the device origin. */
    cairo_surface_set_device_offset (group_surface,
				     parent_surface->device_transform.x0 - extents.x,
				     parent_surface->device_transform.y0 - extents.y);

    /* If we have a current path, we need to adjust it to compensate for
     * the device offset just applied. */
    _cairo_path_fixed_transform (cr->path,
				 &group_surface->device_transform);
#endif

    status = _cairo_skia_context_save (cr);
    if (unlikely (status)) {
	cairo_surface_destroy (group_surface);
	return status;
    }

    cairo_surface_destroy (&cr->target->image.base);
    cr->target = (cairo_skia_surface_t *) group_surface;

    return CAIRO_STATUS_SUCCESS;
}

static cairo_pattern_t *
_cairo_skia_context_pop_group (void *abstract_cr)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;
    cairo_surface_t *group_surface;
    cairo_pattern_t *group_pattern;
    cairo_status_t status;

    group_surface = cairo_surface_reference (&cr->target->image.base);

    status = _cairo_skia_context_restore (cr);
    if (unlikely (status)) {
	group_pattern = _cairo_pattern_create_in_error (status);
	goto done;
    }

    group_pattern = cairo_pattern_create_for_surface (group_surface);
    status = group_pattern->status;
    if (unlikely (status))
        goto done;

#if 0
    _cairo_gstate_get_matrix (cr->gstate, &group_matrix);
    /* Transform by group_matrix centered around device_transform so that when
     * we call _cairo_gstate_copy_transformed_pattern the result is a pattern
     * with a matrix equivalent to the device_transform of group_surface. */
    if (_cairo_surface_has_device_transform (group_surface)) {
	cairo_pattern_set_matrix (group_pattern, &group_surface->device_transform);
	_cairo_pattern_transform (group_pattern, &group_matrix);
	_cairo_pattern_transform (group_pattern, &group_surface->device_transform_inverse);
    } else {
	cairo_pattern_set_matrix (group_pattern, &group_matrix);
    }

    /* If we have a current path, we need to adjust it to compensate for
     * the device offset just removed. */
    _cairo_path_fixed_transform (cr->path,
				 &group_surface->device_transform_inverse);
#endif

done:
    cairo_surface_destroy (group_surface);

    return group_pattern;
}

static inline cairo_surface_t *
surface_from_pattern (const cairo_pattern_t *pattern)
{
    return (reinterpret_cast <const cairo_surface_pattern_t *> (pattern))->surface;
}

static inline bool
surface_to_sk_bitmap (cairo_surface_t *surface, SkBitmap& bitmap)
{
    cairo_image_surface_t *img = (cairo_image_surface_t *) surface;
    SkBitmap::Config config;
    bool opaque;

    if (unlikely (! format_to_sk_config (img->format, config, opaque)))
	return false;

    bitmap.reset ();
    bitmap.setConfig (config, img->width, img->height, img->stride);
    bitmap.setIsOpaque (opaque);
    bitmap.setPixels (img->data);

    return true;
}

static inline SkMatrix
matrix_to_sk (const cairo_matrix_t& mat)
{
    SkMatrix skm;

    skm.reset ();
    skm.set (SkMatrix::kMScaleX, SkFloatToScalar (mat.xx));
    skm.set (SkMatrix::kMSkewX,  SkFloatToScalar (mat.xy));
    skm.set (SkMatrix::kMTransX, SkFloatToScalar (mat.x0));
    skm.set (SkMatrix::kMSkewY,  SkFloatToScalar (mat.yx));
    skm.set (SkMatrix::kMScaleY, SkFloatToScalar (mat.yy));
    skm.set (SkMatrix::kMTransY, SkFloatToScalar (mat.y0));

    /*
    skm[6] = SkFloatToScalar (0.0);
    skm[7] = SkFloatToScalar (0.0);
    skm[8] = SkFloatToScalar (1.0); -- this isn't right, it wants a magic value in there that it'll set itself.  It wants Sk_Fract1 (2.30), not Sk_Scalar1
    */

    return skm;
}

static inline SkMatrix
matrix_inverse_to_sk (const cairo_matrix_t& mat)
{
    cairo_matrix_t inv = mat;
    cairo_status_t status = cairo_matrix_invert (&inv);
    assert (status == CAIRO_STATUS_SUCCESS);
    return matrix_to_sk (inv);
}

static SkShader::TileMode
extend_to_sk (cairo_extend_t extend)
{
    static const SkShader::TileMode modeMap[] = {
	SkShader::kClamp_TileMode,  // NONE behaves like PAD, because noone wants NONE
	SkShader::kRepeat_TileMode,
	SkShader::kMirror_TileMode,
	SkShader::kClamp_TileMode
    };

    return modeMap[extend];
}

static inline SkColor
color_to_sk (const cairo_color_t& c)
{
    /* Need unpremultiplied 1-byte values */
    return SkColorSetARGB ((U8CPU) (c.alpha * 255),
			   (U8CPU) (c.red * 255),
			   (U8CPU) (c.green * 255),
			   (U8CPU) (c.blue * 255));
}

static inline SkColor
color_stop_to_sk (const cairo_color_stop_t& c)
{
    /* Need unpremultiplied 1-byte values */
    return SkColorSetARGB ((U8CPU) (c.alpha * 255),
			   (U8CPU) (c.red * 255),
			   (U8CPU) (c.green * 255),
			   (U8CPU) (c.blue * 255));
}

static SkShader*
source_to_sk_shader (cairo_skia_context_t *cr,
		     const cairo_pattern_t *pattern)
{
    SkShader *shader = NULL;

    if (pattern->type == CAIRO_PATTERN_TYPE_SOLID) {
	cairo_solid_pattern_t *solid = (cairo_solid_pattern_t *) pattern;
	return new SkColorShader (color_to_sk (solid->color));
    } else if (pattern->type == CAIRO_PATTERN_TYPE_SURFACE) {
	cairo_surface_t *surface = surface_from_pattern (pattern);

	cr->source = cairo_surface_reference (surface);

	if (surface->type == CAIRO_SURFACE_TYPE_SKIA) {
	    cairo_skia_surface_t *esurf = (cairo_skia_surface_t *) surface;

	    shader = SkShader::CreateBitmapShader (*esurf->bitmap,
						   extend_to_sk (pattern->extend),
						   extend_to_sk (pattern->extend));
	} else {
	    SkBitmap bitmap;

	    if (! _cairo_surface_is_image (surface)) {
		cairo_status_t status;

		status = _cairo_surface_acquire_source_image (surface,
							      &cr->source_image,
							      &cr->source_extra);
		if (status)
		    return NULL;

		surface = &cr->source_image->base;
	    }

	    if (unlikely (! surface_to_sk_bitmap (surface, bitmap)))
		return NULL;

	    shader = SkShader::CreateBitmapShader (bitmap,
						   extend_to_sk (pattern->extend),
						   extend_to_sk (pattern->extend));
	}
    } else if (pattern->type == CAIRO_PATTERN_TYPE_LINEAR
	       /* || pattern->type == CAIRO_PATTERN_TYPE_RADIAL */)
    {
	cairo_gradient_pattern_t *gradient = (cairo_gradient_pattern_t *) pattern;
	SkColor colors_stack[10];
	SkScalar pos_stack[10];
	SkColor *colors = colors_stack;
	SkScalar *pos = pos_stack;

	if (gradient->n_stops > 10) {
	    colors = new SkColor[gradient->n_stops];
	    pos = new SkScalar[gradient->n_stops];
	}

	for (unsigned int i = 0; i < gradient->n_stops; i++) {
	    pos[i] = CAIRO_FIXED_TO_SK_SCALAR (gradient->stops[i].offset);
	    colors[i] = color_stop_to_sk (gradient->stops[i].color);
	}

	if (pattern->type == CAIRO_PATTERN_TYPE_LINEAR) {
	    cairo_linear_pattern_t *linear = (cairo_linear_pattern_t *) gradient;
	    SkPoint points[2];

	    points[0].set (SkFloatToScalar (linear->pd1.x),
			   SkFloatToScalar (linear->pd1.y));
	    points[1].set (SkFloatToScalar (linear->pd2.x),
			   SkFloatToScalar (linear->pd2.y));
	    shader = SkGradientShader::CreateLinear (points, colors, pos, gradient->n_stops,
						     extend_to_sk (pattern->extend));
	} else {
	    // XXX todo -- implement real radial shaders in Skia
	}

	if (gradient->n_stops > 10) {
	    delete [] colors;
	    delete [] pos;
	}
    }

    if (shader && ! _cairo_matrix_is_identity (&pattern->matrix))
	shader->setLocalMatrix (matrix_inverse_to_sk (pattern->matrix));

    return shader;
}

static inline bool
pattern_filter_to_sk (const cairo_pattern_t *pattern)
{
    switch (pattern->filter) {
    case CAIRO_FILTER_GOOD:
    case CAIRO_FILTER_BEST:
    case CAIRO_FILTER_BILINEAR:
    case CAIRO_FILTER_GAUSSIAN:
	return true;
    default:
    case CAIRO_FILTER_FAST:
    case CAIRO_FILTER_NEAREST:
	return false;
    }
}

static inline bool
pattern_to_sk_color (const cairo_pattern_t *pattern, SkColor& color)
{
    if (pattern->type != CAIRO_PATTERN_TYPE_SOLID)
	return false;

    color = color_to_sk (((cairo_solid_pattern_t *) pattern)->color);
    return true;
}

static cairo_status_t
_cairo_skia_context_set_source (void *abstract_cr,
				cairo_pattern_t *source)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;
    SkColor color;

    if (cr->source != NULL) {
	if (cr->source_image != NULL) {
	    _cairo_surface_release_source_image (cr->source, cr->source_image, cr->source_extra);
	    cr->source_image = NULL;
	}
	cairo_surface_destroy (cr->source);
	cr->source = NULL;
    }

    if (pattern_to_sk_color (source, color)) {
	cr->paint->setColor (color);
    } else {
	SkShader *shader = source_to_sk_shader (cr, source);
	if (shader == NULL) {
	    UNSUPPORTED;
	    return CAIRO_STATUS_SUCCESS;
	}

	cr->paint->setShader (shader);
	shader->unref ();

	cr->paint->setFilterBitmap (pattern_filter_to_sk (source));
    }

    /* XXX change notification */
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_set_source_rgba (void *abstract_cr, double red, double green, double blue, double alpha)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    /* Need unpremultiplied 1-byte values */
    cr->paint->setARGB ((U8CPU) (alpha * 255),
			(U8CPU) (red * 255),
			(U8CPU) (green * 255),
			(U8CPU) (blue * 255));
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_set_source_surface (void *abstract_cr,
					cairo_surface_t *surface,
					double	   x,
					double	   y)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;
    cairo_pattern_t *pattern;
    cairo_matrix_t matrix;
    cairo_status_t status;

    if (surface->type == CAIRO_SURFACE_TYPE_SKIA) {
	cairo_skia_surface_t *esurf = (cairo_skia_surface_t *) surface;
	SkShader *shader;

	shader = SkShader::CreateBitmapShader (*esurf->bitmap,
					       SkShader::kClamp_TileMode, /* XXX */
					       SkShader::kClamp_TileMode);

	cr->paint->setShader (shader);
	shader->unref ();

	cr->paint->setFilterBitmap (true);

	return CAIRO_STATUS_SUCCESS;
    }

    pattern = cairo_pattern_create_for_surface (surface);
    if (unlikely (pattern->status))
	return pattern->status;

    cairo_matrix_init_translate (&matrix, -x, -y);
    cairo_pattern_set_matrix (pattern, &matrix);

    status = _cairo_skia_context_set_source (cr, pattern);
    cairo_pattern_destroy (pattern);

    return status;
}

static cairo_pattern_t *
_cairo_skia_context_get_source (void *abstract_cr)
{
    //cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    UNSUPPORTED;
    return NULL;
}

static cairo_status_t
_cairo_skia_context_set_tolerance (void *abstract_cr,
				   double tolerance)
{
    //cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    /* XXX ignored */
    return CAIRO_STATUS_SUCCESS;
}

static inline SkXfermode::Mode
operator_to_sk (cairo_operator_t op)
{
    static const SkXfermode::Mode modeMap[] = {
	SkXfermode::kClear_Mode,

	SkXfermode::kSrc_Mode,
	SkXfermode::kSrcOver_Mode,
	SkXfermode::kSrcIn_Mode,
	SkXfermode::kSrcOut_Mode,
	SkXfermode::kSrcATop_Mode,

	SkXfermode::kDst_Mode,
	SkXfermode::kDstOver_Mode,
	SkXfermode::kDstIn_Mode,
	SkXfermode::kDstOut_Mode,
	SkXfermode::kDstATop_Mode,

	SkXfermode::kXor_Mode,
	SkXfermode::kPlus_Mode, // XXX Add?
	SkXfermode::kPlus_Mode, // XXX SATURATE

	SkXfermode::kPlus_Mode,
	SkXfermode::kMultiply_Mode,
	SkXfermode::kScreen_Mode,
	SkXfermode::kOverlay_Mode,
	SkXfermode::kDarken_Mode,
	SkXfermode::kLighten_Mode,
	SkXfermode::kColorDodge_Mode,
	SkXfermode::kColorBurn_Mode,
	SkXfermode::kHardLight_Mode,
	SkXfermode::kSoftLight_Mode,
	SkXfermode::kDifference_Mode,
	SkXfermode::kExclusion_Mode,

	SkXfermode::kSrcOver_Mode, // XXX: CAIRO_OPERATOR_HSL_HUE
	SkXfermode::kSrcOver_Mode, // XXX: CAIRO_OPERATOR_HSL_SATURATION,
	SkXfermode::kSrcOver_Mode, // XXX: CAIRO_OPERATOR_HSL_COLOR,
	SkXfermode::kSrcOver_Mode, // XXX: CAIRO_OPERATOR_HSL_LUMINOSITY
    };

    return modeMap[op];
}

static cairo_status_t
_cairo_skia_context_set_operator (void *abstract_cr, cairo_operator_t op)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    cr->paint->setXfermodeMode (operator_to_sk (op));
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_set_opacity (void *abstract_cr, double opacity)
{
    //cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    /* XXX */
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_set_antialias (void *abstract_cr, cairo_antialias_t antialias)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    cr->paint->setAntiAlias (antialias != CAIRO_ANTIALIAS_NONE);
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_set_fill_rule (void *abstract_cr,
				   cairo_fill_rule_t fill_rule)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    cr->path->setFillType (fill_rule == CAIRO_FILL_RULE_WINDING ? SkPath::kWinding_FillType : SkPath::kEvenOdd_FillType);
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_set_line_width (void *abstract_cr,
				    double line_width)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    cr->paint->setStrokeWidth (SkFloatToScalar (line_width));
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_set_line_cap (void *abstract_cr,
				  cairo_line_cap_t line_cap)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;
    static const SkPaint::Cap map[] = {
	SkPaint::kButt_Cap,
	SkPaint::kRound_Cap,
	SkPaint::kSquare_Cap
    };
    cr->paint->setStrokeCap (map[line_cap]);
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_set_line_join (void *abstract_cr,
				   cairo_line_join_t line_join)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;
    static const SkPaint::Join map[] = {
	SkPaint::kMiter_Join,
	SkPaint::kRound_Join,
	SkPaint::kBevel_Join
    };
    cr->paint->setStrokeJoin (map[line_join]);
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_set_dash (void *abstract_cr,
			      const double *dashes,
			      int	      num_dashes,
			      double	      offset)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;
    SkScalar intervals_static[20];
    SkScalar *intervals = intervals_static;

    if (num_dashes == 0) {
	cr->paint->setPathEffect (NULL);
	return CAIRO_STATUS_SUCCESS;
    }

    int loop = 0;
    if ((num_dashes & 1) != 0) {
	loop = 1;
	num_dashes <<= 1;
    }

    if (num_dashes > 20)
	intervals = new SkScalar[num_dashes];

    int i = 0;
    do {
	for (int j = 0; i < num_dashes; j++)
	    intervals[i++] = SkFloatToScalar (dashes[j]);
    } while (loop--);

    SkDashPathEffect *dash = new SkDashPathEffect (intervals, num_dashes, SkFloatToScalar (offset));

    cr->paint->setPathEffect (dash);
    dash->unref ();

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_set_miter_limit (void *abstract_cr, double limit)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    cr->paint->setStrokeMiter (SkFloatToScalar (limit));
    return CAIRO_STATUS_SUCCESS;
}

static cairo_antialias_t
_cairo_skia_context_get_antialias (void *abstract_cr)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    return cr->paint->isAntiAlias () ? CAIRO_ANTIALIAS_DEFAULT : CAIRO_ANTIALIAS_NONE;
}

static void
_cairo_skia_context_get_dash (void *abstract_cr,
			      double *dashes,
			      int *num_dashes,
			      double *offset)
{
    //cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    *num_dashes = 0;
    /* XXX */
}

static cairo_fill_rule_t
_cairo_skia_context_get_fill_rule (void *abstract_cr)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;
    SkPath::FillType ft;

    ft = cr->path->getFillType ();
    if (ft == SkPath::kWinding_FillType)
	return CAIRO_FILL_RULE_WINDING;
    if (ft == SkPath::kEvenOdd_FillType)
	return CAIRO_FILL_RULE_EVEN_ODD;;

    UNSUPPORTED;
    return CAIRO_FILL_RULE_WINDING;
}

static double
_cairo_skia_context_get_line_width (void *abstract_cr)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    return /* ScalarToFloat */ cr->paint->getStrokeWidth ();
}

static cairo_line_cap_t
_cairo_skia_context_get_line_cap (void *abstract_cr)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;
    static const cairo_line_cap_t map[] = {
	CAIRO_LINE_CAP_BUTT,
	CAIRO_LINE_CAP_ROUND,
	CAIRO_LINE_CAP_SQUARE
    };
    return map[cr->paint->getStrokeCap ()];
}

static cairo_line_join_t
_cairo_skia_context_get_line_join (void *abstract_cr)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;
    static const cairo_line_join_t map[] = {
	CAIRO_LINE_JOIN_MITER,
	CAIRO_LINE_JOIN_ROUND,
	CAIRO_LINE_JOIN_BEVEL
    };
    return map[cr->paint->getStrokeJoin ()];
}

static double
_cairo_skia_context_get_miter_limit (void *abstract_cr)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    return /* SkScalarToFloat */ cr->paint->getStrokeMiter ();
}

static cairo_operator_t
_cairo_skia_context_get_operator (void *abstract_cr)
{
    //cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    UNSUPPORTED;
    //cr->paint->getXfermode ();
    return CAIRO_OPERATOR_OVER;
}

static double
_cairo_skia_context_get_opacity (void *abstract_cr)
{
    //cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    UNSUPPORTED;
    return 1.;
}

static double
_cairo_skia_context_get_tolerance (void *abstract_cr)
{
    //cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    /* XXX */
    return CAIRO_GSTATE_TOLERANCE_DEFAULT;
}


/* Current tranformation matrix */

static cairo_status_t
_cairo_skia_context_translate (void *abstract_cr,
			       double tx,
			       double ty)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    cairo_matrix_translate (&cr->matrix, tx, ty);
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_scale (void *abstract_cr,
			   double sx,
			      double sy)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    cairo_matrix_scale (&cr->matrix, sx, sy);
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_rotate (void *abstract_cr,
			    double theta)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    cairo_matrix_rotate (&cr->matrix, theta);
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_transform (void *abstract_cr,
			       const cairo_matrix_t *matrix)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    cairo_matrix_multiply (&cr->matrix, &cr->matrix, matrix);
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_set_matrix (void *abstract_cr,
				const cairo_matrix_t *matrix)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    cr->matrix = *matrix;
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_set_identity_matrix (void *abstract_cr)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    cairo_matrix_init_identity (&cr->matrix);
    return CAIRO_STATUS_SUCCESS;
}

static void
_cairo_skia_context_get_matrix (void *abstract_cr,
				cairo_matrix_t *matrix)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    *matrix = cr->matrix;
}

static void
_cairo_skia_context_user_to_device (void *abstract_cr,
				    double *x,
				    double *y)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    cairo_matrix_transform_point (&cr->matrix, x, y);
}

static void
_cairo_skia_context_user_to_device_distance (void *abstract_cr,
					     double *dx,
					     double *dy)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    cairo_matrix_transform_distance (&cr->matrix, dx, dy);
}

static void
_cairo_skia_context_device_to_user (void *abstract_cr,
				    double *x,
				    double *y)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;
    cairo_matrix_t inverse;
    cairo_status_t status;

    inverse = cr->matrix;
    status = cairo_matrix_invert (&inverse);
    assert (CAIRO_STATUS_SUCCESS == status);

    cairo_matrix_transform_point (&inverse, x, y);
}

static void
_cairo_skia_context_device_to_user_distance (void *abstract_cr,
					     double *dx,
					     double *dy)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;
    cairo_matrix_t inverse;
    cairo_status_t status;

    inverse = cr->matrix;
    status = cairo_matrix_invert (&inverse);
    assert (CAIRO_STATUS_SUCCESS == status);

    cairo_matrix_transform_distance (&inverse, dx, dy);
}

/* Path constructor */

static cairo_status_t
_cairo_skia_context_new_path (void *abstract_cr)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    cr->path->reset ();
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_new_sub_path (void *abstract_cr)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    cr->path->rMoveTo (0, 0); /* XXX */
    return CAIRO_STATUS_SUCCESS;
}

static void
user_to_device_point (cairo_skia_context_t *cr, double *x, double *y)
{
    cairo_matrix_transform_point (&cr->matrix, x, y);
    cairo_matrix_transform_point (&cr->target->image.base.device_transform, x, y);
}

static void
user_to_device_distance (cairo_skia_context_t *cr, double *dx, double *dy)
{
    cairo_matrix_transform_distance (&cr->matrix, dx, dy);
    cairo_matrix_transform_distance (&cr->target->image.base.device_transform, dx, dy);
}

static cairo_status_t
_cairo_skia_context_move_to (void *abstract_cr, double x, double y)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    user_to_device_point (cr, &x, &y);
    cr->path->moveTo (SkFloatToScalar (x), SkFloatToScalar (y));
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_line_to (void *abstract_cr, double x, double y)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    user_to_device_point (cr, &x, &y);
    cr->path->lineTo (SkFloatToScalar (x), SkFloatToScalar (y));
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_curve_to (void *abstract_cr,
			      double x1, double y1,
			      double x2, double y2,
			      double x3, double y3)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    user_to_device_point (cr, &x1, &y1);
    user_to_device_point (cr, &x2, &y2);
    user_to_device_point (cr, &x3, &y3);
    cr->path->cubicTo (SkFloatToScalar (x1), SkFloatToScalar (y1),
		       SkFloatToScalar (x2), SkFloatToScalar (y2),
		       SkFloatToScalar (x3), SkFloatToScalar (y3));
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_arc_to (void *abstract_cr,
			    double x1, double y1,
			    double x2, double y2,
			    double radius)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

#if 0
    user_to_device_point (cr, &x1, &y1);
    user_to_device_point (cr, &x2, &y2);
    user_to_device_distance (cr, &radius, &radius);
#endif

    cr->path->arcTo (SkFloatToScalar (x1), SkFloatToScalar (y1),
		     SkFloatToScalar (x2), SkFloatToScalar (y2),
		     SkFloatToScalar (radius));
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_rel_move_to (void *abstract_cr, double dx, double dy)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    user_to_device_distance (cr, &dx, &dy);
    cr->path->rMoveTo (SkFloatToScalar (dx), SkFloatToScalar (dy));
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_rel_line_to (void *abstract_cr, double dx, double dy)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    user_to_device_distance (cr, &dx, &dy);
    cr->path->rLineTo (SkFloatToScalar (dx), SkFloatToScalar (dy));
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_rel_curve_to (void *abstract_cr,
				  double dx1, double dy1,
				  double dx2, double dy2,
				  double dx3, double dy3)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    user_to_device_distance (cr, &dx1, &dy1);
    user_to_device_distance (cr, &dx2, &dy2);
    user_to_device_distance (cr, &dx3, &dy3);
    cr->path->rCubicTo (SkFloatToScalar (dx1), SkFloatToScalar (dy1),
			SkFloatToScalar (dx2), SkFloatToScalar (dy2),
			SkFloatToScalar (dx3), SkFloatToScalar (dy3));
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_rel_arc_to (void *abstract_cr,
				double dx1, double dy1,
				double dx2, double dy2,
				double radius)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

#if 0
    user_to_device_point (cr, &x1, &y1);
    user_to_device_point (cr, &x2, &y2);
    user_to_device_distance (cr, &radius, &radius);
#endif

    cr->path->arcTo (SkFloatToScalar (dx1), SkFloatToScalar (dy1),
		     SkFloatToScalar (dx2), SkFloatToScalar (dy2),
		     SkFloatToScalar (radius));
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_close_path (void *abstract_cr)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    cr->path->close ();
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_rectangle (void *abstract_cr,
			       double x, double y,
			       double width, double height)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;
    double x1, y1, x2, y2;

    /* XXX assume no rotation! */
    x1 = x, y1 = y;
    user_to_device_point (cr, &x1, &y1);

    x2 = x + width, y2 = y + height;
    user_to_device_point (cr, &x2, &y2);

    cr->path->addRect (SkFloatToScalar (x1), SkFloatToScalar (y1),
		       SkFloatToScalar (x2), SkFloatToScalar (y2));
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_arc (void *abstract_cr,
			 double xc, double yc, double radius,
			 double angle1, double angle2,
			 cairo_bool_t forward)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;
    cairo_status_t status;

    /* XXX cr->path->arc() */

    /* Do nothing, successfully, if radius is <= 0 */
    if (radius <= 0.0) {
	status = _cairo_skia_context_line_to (cr, xc, yc);
	if (unlikely (status))
	    return status;

	status = _cairo_skia_context_line_to (cr, xc, yc);
	if (unlikely (status))
	    return status;

	return CAIRO_STATUS_SUCCESS;
    }

    status = _cairo_skia_context_line_to (cr,
					  xc + radius * cos (angle1),
					  yc + radius * sin (angle1));

    if (unlikely (status))
	return status;

    if (forward)
	_cairo_arc_path (&cr->base, xc, yc, radius, angle1, angle2);
    else
	_cairo_arc_path_negative (&cr->base, xc, yc, radius, angle1, angle2);

    return CAIRO_STATUS_SUCCESS;
}

static void
_cairo_skia_context_path_extents (void *abstract_cr,
				  double *x1,
				  double *y1,
				  double *x2,
				  double *y2)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;
    SkRect rect;

    rect = cr->path->getBounds ();

    UNSUPPORTED;
    /* XXX transform SkScalar rect to user */
}

static cairo_bool_t
_cairo_skia_context_has_current_point (void *abstract_cr)
{
    //cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    return TRUE;
}

static cairo_bool_t
_cairo_skia_context_get_current_point (void *abstract_cr,
				       double *x,
				       double *y)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;
    SkPoint pt;

    cr->path->getLastPt (&pt);
    //*x = SkScalarToFloat (pt.x);
    //*y = SkScalarToFloat (pt.y);
    //_cairo_gstate_backend_to_user (cr->gstate, x, y);

    return TRUE;
}

static cairo_path_t *
_cairo_skia_context_copy_path (void *abstract_cr)
{
    //cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    /* XXX iterate */
    UNSUPPORTED;
    return NULL;
}

static cairo_path_t *
_cairo_skia_context_copy_path_flat (void *abstract_cr)
{
    //cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    /* XXX iterate and decompose */
    UNSUPPORTED;
    return NULL;
}

static cairo_status_t
_cairo_skia_context_append_path (void *abstract_cr,
				 const cairo_path_t *path)
{
    //cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

   // return _cairo_path_append_to_context (path, cr);
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_stroke_to_path (void *abstract_cr)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    cr->paint->setStyle (SkPaint::kStroke_Style);
    cr->paint->getFillPath (*cr->path, cr->path);
    return CAIRO_STATUS_SUCCESS;
}


static cairo_status_t
_cairo_skia_context_paint (void *abstract_cr)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

#if 0
    if (cr->source != NULL) {
	SkBitmap bitmap;
	SkMatrix bitmapMatrix;

	if (cr->source->type == CAIRO_SURFACE_TYPE_SKIA) {
	    cairo_skia_surface_t *esurf = (cairo_skia_surface_t *) cr->source->type;

	    bitmap = *esurf->bitmap;
	} else {
	    surface_to_sk_bitmap (&cr->source_image->base, bitmap);
	}

	// XXX pattern->matrix, pattern->filter, pattern->extend
	cr->canvas->drawBitmapMatrix (bitmap, bitmapMatrix, cr->paint);
    } else {
	cr->canvas->drawPaint (*cr->paint);
    }
#else
    cr->canvas->drawPaint (*cr->paint);
#endif
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_paint_with_alpha (void *abstract_cr,
				      double alpha)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;
    cairo_status_t status;

    if (CAIRO_ALPHA_IS_OPAQUE (alpha))
	return _cairo_skia_context_paint (cr);

    cr->paint->setAlpha(SkScalarRound(255*alpha));
    status = _cairo_skia_context_paint (cr);
    cr->paint->setAlpha(255);

    return status;
}

static cairo_status_t
_cairo_skia_context_mask (void *abstract_cr,
			  cairo_pattern_t *mask)
{
    //cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    /* XXX */
    //UNSUPPORTED;
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_stroke_preserve (void *abstract_cr)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    cr->paint->setStyle (SkPaint::kStroke_Style);

    /* XXX pen transformation? */
    //assert (_cairo_matrix_is_identity (&cr->matrix));
    cr->canvas->drawPath (*cr->path, *cr->paint);
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_stroke (void *abstract_cr)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;
    cairo_status_t status;

    status = _cairo_skia_context_stroke_preserve (cr);
    if (unlikely (status))
	return status;

    return _cairo_skia_context_new_path (cr);
}

static cairo_status_t
_cairo_skia_context_in_stroke (void *abstract_cr,
			       double x, double y,
			       cairo_bool_t *inside)
{
    //cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    UNSUPPORTED;
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_stroke_extents (void *abstract_cr,
				    double *x1, double *y1, double *x2, double *y2)
{
    //cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    UNSUPPORTED;
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_fill_preserve (void *abstract_cr)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    cr->paint->setStyle (SkPaint::kFill_Style);
    cr->canvas->drawPath (*cr->path, *cr->paint);
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_fill (void *abstract_cr)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;
    cairo_status_t status;

    status = _cairo_skia_context_fill_preserve (cr);
    if (unlikely (status))
	return status;

    return _cairo_skia_context_new_path (cr);
}

static cairo_status_t
_cairo_skia_context_in_fill (void *abstract_cr,
				double x, double y,
				cairo_bool_t *inside)
{
    //cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    UNSUPPORTED;
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_fill_extents (void *abstract_cr,
				     double *x1, double *y1, double *x2, double *y2)
{
    //cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    UNSUPPORTED;
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_clip_preserve (void *abstract_cr)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    cr->canvas->clipPath (*cr->path);
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_clip (void *abstract_cr)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;
    cairo_status_t status;

    status = _cairo_skia_context_clip_preserve (cr);
    if (unlikely (status))
	return status;

    return _cairo_skia_context_new_path (cr);
}

static cairo_status_t
_cairo_skia_context_in_clip (void *abstract_cr,
			     double x, double y,
			     cairo_bool_t *inside)
{
    //cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_reset_clip (void *abstract_cr)
{
    cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;
    SkRegion rgn(SkIRect::MakeWH (cr->target->bitmap->width (),
				  cr->target->bitmap->height ()));

    cr->canvas->setClipRegion(rgn);
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_clip_extents (void *abstract_cr,
				  double *x1, double *y1,
				  double *x2, double *y2)
{
    //cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    UNSUPPORTED;
    return CAIRO_STATUS_SUCCESS;
}

static cairo_rectangle_list_t *
_cairo_skia_context_copy_clip_rectangle_list (void *abstract_cr)
{
    //cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    UNSUPPORTED;
    return NULL;
}

static cairo_status_t
_cairo_skia_context_copy_page (void *abstract_cr)
{
    //cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    UNSUPPORTED;
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_show_page (void *abstract_cr)
{
    //cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;
    UNSUPPORTED;
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_set_font_face (void *abstract_cr,
				   cairo_font_face_t *font_face)
{
   // cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    //return _cairo_gstate_set_font_face (cr->gstate, font_face);
    return CAIRO_STATUS_SUCCESS;
}

static cairo_font_face_t *
_cairo_skia_context_get_font_face (void *abstract_cr)
{
    //cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    UNSUPPORTED;
    return NULL;
}

static cairo_status_t
_cairo_skia_context_font_extents (void *abstract_cr,
				  cairo_font_extents_t *extents)
{
    //cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    UNSUPPORTED;
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_set_font_size (void *abstract_cr,
				   double size)
{
    //cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    UNSUPPORTED;
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_set_font_matrix (void *abstract_cr,
				     const cairo_matrix_t *matrix)
{
    //cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    UNSUPPORTED;
    return CAIRO_STATUS_SUCCESS;
}

static void
_cairo_skia_context_get_font_matrix (void *abstract_cr,
				     cairo_matrix_t *matrix)
{
    //cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    UNSUPPORTED;
}

static cairo_status_t
_cairo_skia_context_set_font_options (void *abstract_cr,
				      const cairo_font_options_t *options)
{
    //cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    UNSUPPORTED;
    return CAIRO_STATUS_SUCCESS;
}

static void
_cairo_skia_context_get_font_options (void *abstract_cr,
				      cairo_font_options_t *options)
{
    //cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    UNSUPPORTED;
}

static cairo_status_t
_cairo_skia_context_set_scaled_font (void *abstract_cr,
					cairo_scaled_font_t *scaled_font)
{
    //cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    UNSUPPORTED;
    return CAIRO_STATUS_SUCCESS;
}

static cairo_scaled_font_t *
_cairo_skia_context_get_scaled_font (void *abstract_cr)
{
    //cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    UNSUPPORTED;
    return _cairo_scaled_font_create_in_error (CAIRO_STATUS_SURFACE_TYPE_MISMATCH);
}

static cairo_status_t
_cairo_skia_context_glyphs (void *abstract_cr,
			    const cairo_glyph_t *glyphs,
			    int num_glyphs,
			    cairo_glyph_text_info_t *info)
{
    //cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    /* XXX */
    //UNSUPPORTED;
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_glyph_path (void *abstract_cr,
				const cairo_glyph_t *glyphs,
				int num_glyphs)
{
    //cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    UNSUPPORTED;
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_skia_context_glyph_extents (void                *abstract_cr,
				   const cairo_glyph_t    *glyphs,
				   int                    num_glyphs,
				   cairo_text_extents_t   *extents)
{
    //cairo_skia_context_t *cr = (cairo_skia_context_t *) abstract_cr;

    UNSUPPORTED;
    return CAIRO_STATUS_SUCCESS;
}

static const cairo_backend_t _cairo_skia_context_backend = {
    CAIRO_TYPE_SKIA,
    _cairo_skia_context_destroy,

    _cairo_skia_context_get_original_target,
    _cairo_skia_context_get_current_target,

    _cairo_skia_context_save,
    _cairo_skia_context_restore,

    _cairo_skia_context_push_group,
    _cairo_skia_context_pop_group,

    _cairo_skia_context_set_source_rgba,
    _cairo_skia_context_set_source_surface,
    _cairo_skia_context_set_source,
    _cairo_skia_context_get_source,

    _cairo_skia_context_set_antialias,
    _cairo_skia_context_set_dash,
    _cairo_skia_context_set_fill_rule,
    _cairo_skia_context_set_line_cap,
    _cairo_skia_context_set_line_join,
    _cairo_skia_context_set_line_width,
    _cairo_skia_context_set_miter_limit,
    _cairo_skia_context_set_opacity,
    _cairo_skia_context_set_operator,
    _cairo_skia_context_set_tolerance,
    _cairo_skia_context_get_antialias,
    _cairo_skia_context_get_dash,
    _cairo_skia_context_get_fill_rule,
    _cairo_skia_context_get_line_cap,
    _cairo_skia_context_get_line_join,
    _cairo_skia_context_get_line_width,
    _cairo_skia_context_get_miter_limit,
    _cairo_skia_context_get_opacity,
    _cairo_skia_context_get_operator,
    _cairo_skia_context_get_tolerance,

    _cairo_skia_context_translate,
    _cairo_skia_context_scale,
    _cairo_skia_context_rotate,
    _cairo_skia_context_transform,
    _cairo_skia_context_set_matrix,
    _cairo_skia_context_set_identity_matrix,
    _cairo_skia_context_get_matrix,
    _cairo_skia_context_user_to_device,
    _cairo_skia_context_user_to_device_distance,
    _cairo_skia_context_device_to_user,
    _cairo_skia_context_device_to_user_distance,

    _cairo_skia_context_new_path,
    _cairo_skia_context_new_sub_path,
    _cairo_skia_context_move_to,
    _cairo_skia_context_rel_move_to,
    _cairo_skia_context_line_to,
    _cairo_skia_context_rel_line_to,
    _cairo_skia_context_curve_to,
    _cairo_skia_context_rel_curve_to,
    _cairo_skia_context_arc_to,
    _cairo_skia_context_rel_arc_to,
    _cairo_skia_context_close_path,
    _cairo_skia_context_arc,
    _cairo_skia_context_rectangle,
    _cairo_skia_context_path_extents,
    _cairo_skia_context_has_current_point,
    _cairo_skia_context_get_current_point,
    _cairo_skia_context_copy_path,
    _cairo_skia_context_copy_path_flat,
    _cairo_skia_context_append_path,

    _cairo_skia_stroke_to_path,

    _cairo_skia_context_clip,
    _cairo_skia_context_clip_preserve,
    _cairo_skia_context_in_clip,
    _cairo_skia_context_clip_extents,
    _cairo_skia_context_reset_clip,
    _cairo_skia_context_copy_clip_rectangle_list,

    _cairo_skia_context_paint,
    _cairo_skia_context_paint_with_alpha,
    _cairo_skia_context_mask,

    _cairo_skia_context_stroke,
    _cairo_skia_context_stroke_preserve,
    _cairo_skia_context_in_stroke,
    _cairo_skia_context_stroke_extents,

    _cairo_skia_context_fill,
    _cairo_skia_context_fill_preserve,
    _cairo_skia_context_in_fill,
    _cairo_skia_context_fill_extents,

    _cairo_skia_context_set_font_face,
    _cairo_skia_context_get_font_face,
    _cairo_skia_context_set_font_size,
    _cairo_skia_context_set_font_matrix,
    _cairo_skia_context_get_font_matrix,
    _cairo_skia_context_set_font_options,
    _cairo_skia_context_get_font_options,
    _cairo_skia_context_set_scaled_font,
    _cairo_skia_context_get_scaled_font,
    _cairo_skia_context_font_extents,

    _cairo_skia_context_glyphs,
    _cairo_skia_context_glyph_path,
    _cairo_skia_context_glyph_extents,

    _cairo_skia_context_copy_page,
    _cairo_skia_context_show_page,
};

cairo_t *
_cairo_skia_context_create (void *target)
{
    cairo_skia_surface_t *surface = (cairo_skia_surface_t *) target;
    cairo_skia_context_t *cr;

    cr = (cairo_skia_context_t *) _freed_pool_get (&context_pool);
    if (unlikely (cr == NULL)) {
	    cr = new cairo_skia_context_t;
	    if (unlikely (cr == NULL))
		return _cairo_create_in_error (_cairo_error (CAIRO_STATUS_NO_MEMORY));

	    cr->path = new SkPath;
	    cr->paint = new SkPaint;
    }

    _cairo_init (&cr->base, &_cairo_skia_context_backend);

    cr->source = NULL;
    cr->source_image = NULL;

    cr->paint->setStrokeWidth (SkFloatToScalar (2.0));

    cr->target = (cairo_skia_surface_t *) cairo_surface_reference ((cairo_surface_t *) target);
    cr->original = (cairo_skia_surface_t *) cairo_surface_reference ((cairo_surface_t *) target);
    cr->canvas = new SkCanvas (*surface->bitmap);
    cr->canvas->save ();

    cairo_matrix_init_identity (&cr->matrix);

    return &cr->base;
}

#if 0
void
_cairo_skia_context_set_SkPaint (cairo_t *cr, SkPaint paint)
{
    *cr->paint = paint;
}

void
_cairo_skia_context_set_SkPath (cairo_t *cr, SkPath path)
{
    *cr->path = path;
}
#endif
