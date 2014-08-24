/* cairo - a vector graphics library with display and print output
 *
 * Copyright © 2005 Red Hat, Inc.
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
 * The Initial Developer of the Original Code is Red Hat, Inc.
 *
 * Contributor(s):
 *	Carl D. Worth <cworth@redhat.com>
 */

#ifndef CAIRO_SKIA_CONTEXT_PRIVATE_H
#define CAIRO_SKIA_CONTEXT_PRIVATE_H

#include "cairo-private.h"
#include "cairo-image-surface-private.h"

#include <SkBitmap.h>
#include <SkCanvas.h>
#include <SkPaint.h>
#include <SkPath.h>

typedef struct _cairo_skia_context cairo_skia_context_t;
typedef struct _cairo_skia_surface cairo_skia_surface_t;

struct _cairo_skia_context {
    cairo_t base;

    cairo_skia_surface_t *original;
    cairo_skia_surface_t *target;

    cairo_matrix_t matrix;

    SkCanvas *canvas;
    SkPaint *paint;
    SkPath *path;

    cairo_surface_t *source;
    cairo_image_surface_t *source_image;
    void *source_extra;
};

struct _cairo_skia_surface {
    cairo_image_surface_t image;

    SkBitmap *bitmap;
};

static inline bool
format_to_sk_config (cairo_format_t format,
		     SkBitmap::Config& config,
		     bool& opaque)
{
    opaque = false;

    switch (format) {
    case CAIRO_FORMAT_ARGB32:
	config = SkBitmap::kARGB_8888_Config;
	break;
    case CAIRO_FORMAT_RGB24:
	config = SkBitmap::kARGB_8888_Config;
	opaque = true;
	break;
    case CAIRO_FORMAT_RGB16_565:
	config = SkBitmap::kRGB_565_Config;
	opaque = true;
	break;
    case CAIRO_FORMAT_A8:
	config = SkBitmap::kA8_Config;
	break;
    case CAIRO_FORMAT_A1:
	config = SkBitmap::kA1_Config;
	break;
    case CAIRO_FORMAT_RGB30:
    case CAIRO_FORMAT_INVALID:
    default:
	return false;
    }

    return true;
}

cairo_private cairo_t *
_cairo_skia_context_create (void *target);

#endif /* CAIRO_SKIA_CONTEXT_PRIVATE_H */
