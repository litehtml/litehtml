/* -*- Mode: c; tab-width: 8; c-basic-offset: 4; indent-tabs-mode: t; -*- */
/* cairo - a vector graphics library with display and print output
 *
 * Copyright © 2004 Red Hat, Inc
 * Copyright © 2006 Red Hat, Inc
 * Copyright © 2007, 2008 Adrian Johnson
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
 *	Kristian Høgsberg <krh@redhat.com>
 *	Carl Worth <cworth@cworth.org>
 *	Adrian Johnson <ajohnson@redneon.com>
 */

#define _BSD_SOURCE /* for snprintf() */
#include "cairoint.h"

#include "cairo-pdf.h"
#include "cairo-pdf-surface-private.h"
#include "cairo-pdf-operators-private.h"
#include "cairo-pdf-shading-private.h"

#include "cairo-array-private.h"
#include "cairo-analysis-surface-private.h"
#include "cairo-composite-rectangles-private.h"
#include "cairo-default-context-private.h"
#include "cairo-error-private.h"
#include "cairo-image-surface-private.h"
#include "cairo-image-info-private.h"
#include "cairo-recording-surface-private.h"
#include "cairo-output-stream-private.h"
#include "cairo-paginated-private.h"
#include "cairo-scaled-font-subsets-private.h"
#include "cairo-surface-clipper-private.h"
#include "cairo-surface-snapshot-inline.h"
#include "cairo-surface-subsurface-private.h"
#include "cairo-type3-glyph-surface-private.h"

#include <time.h>
#include <zlib.h>

/* Issues:
 *
 * - We embed an image in the stream each time it's composited.  We
 *   could add generation counters to surfaces and remember the stream
 *   ID for a particular generation for a particular surface.
 *
 * - Backend specific meta data.
 */

/*
 * Page Structure of the Generated PDF:
 *
 * Each page requiring fallbacks images contains a knockout group at
 * the top level. The first operation of the knockout group paints a
 * group containing all the supported drawing operations. Fallback
 * images (if any) are painted in the knockout group. This ensures
 * that fallback images do not composite with any content under the
 * fallback images.
 *
 * Streams:
 *
 * This PDF surface has three types of streams:
 *  - PDF Stream
 *  - Content Stream
 *  - Group Stream
 *
 * Calling _cairo_output_stream_printf (surface->output, ...) will
 * write to the currently open stream.
 *
 * PDF Stream:
 *   A PDF Stream may be opened and closed with the following functions:
 *     _cairo_pdf_surface_open stream ()
 *     _cairo_pdf_surface_close_stream ()
 *
 *   PDF Streams are written directly to the PDF file. They are used for
 *   fonts, images and patterns.
 *
 * Content Stream:
 *   The Content Stream is opened and closed with the following functions:
 *     _cairo_pdf_surface_open_content_stream ()
 *     _cairo_pdf_surface_close_content_stream ()
 *
 *   The Content Stream contains the text and graphics operators.
 *
 * Group Stream:
 *   A Group Stream may be opened and closed with the following functions:
 *     _cairo_pdf_surface_open_group ()
 *     _cairo_pdf_surface_close_group ()
 *
 *   A Group Stream is a Form XObject. It is used for short sequences
 *   of operators. As the content is very short the group is stored in
 *   memory until it is closed. This allows some optimization such as
 *   including the Resource dictionary and stream length inside the
 *   XObject instead of using an indirect object.
 */

/**
 * SECTION:cairo-pdf
 * @Title: PDF Surfaces
 * @Short_Description: Rendering PDF documents
 * @See_Also: #cairo_surface_t
 *
 * The PDF surface is used to render cairo graphics to Adobe
 * PDF files and is a multi-page vector surface backend.
 **/

static cairo_bool_t
_cairo_pdf_surface_get_extents (void		        *abstract_surface,
				cairo_rectangle_int_t   *rectangle);

/**
 * CAIRO_HAS_PDF_SURFACE:
 *
 * Defined if the PDF surface backend is available.
 * This macro can be used to conditionally compile backend-specific code.
 *
 * Since: 1.2
 **/

static const cairo_pdf_version_t _cairo_pdf_versions[] =
{
    CAIRO_PDF_VERSION_1_4,
    CAIRO_PDF_VERSION_1_5
};

#define CAIRO_PDF_VERSION_LAST ARRAY_LENGTH (_cairo_pdf_versions)

static const char * _cairo_pdf_version_strings[CAIRO_PDF_VERSION_LAST] =
{
    "PDF 1.4",
    "PDF 1.5"
};

static const char *_cairo_pdf_supported_mime_types[] =
{
    CAIRO_MIME_TYPE_JPEG,
    CAIRO_MIME_TYPE_JP2,
    CAIRO_MIME_TYPE_UNIQUE_ID,
    NULL
};

typedef struct _cairo_pdf_object {
    long offset;
} cairo_pdf_object_t;

typedef struct _cairo_pdf_font {
    unsigned int font_id;
    unsigned int subset_id;
    cairo_pdf_resource_t subset_resource;
} cairo_pdf_font_t;

typedef struct _cairo_pdf_rgb_linear_function {
    cairo_pdf_resource_t resource;
    double               color1[3];
    double               color2[3];
} cairo_pdf_rgb_linear_function_t;

typedef struct _cairo_pdf_alpha_linear_function {
    cairo_pdf_resource_t resource;
    double               alpha1;
    double               alpha2;
} cairo_pdf_alpha_linear_function_t;

static cairo_pdf_resource_t
_cairo_pdf_surface_new_object (cairo_pdf_surface_t *surface);

static void
_cairo_pdf_surface_clear (cairo_pdf_surface_t *surface);

static void
_cairo_pdf_smask_group_destroy (cairo_pdf_smask_group_t *group);

static cairo_status_t
_cairo_pdf_surface_add_font (unsigned int        font_id,
			     unsigned int        subset_id,
			     void		*closure);

static void
_cairo_pdf_group_resources_init (cairo_pdf_group_resources_t *res);

static cairo_status_t
_cairo_pdf_surface_open_stream (cairo_pdf_surface_t	*surface,
				cairo_pdf_resource_t    *resource,
                                cairo_bool_t             compressed,
				const char		*fmt,
				...) CAIRO_PRINTF_FORMAT(4, 5);
static cairo_status_t
_cairo_pdf_surface_close_stream (cairo_pdf_surface_t	*surface);

static cairo_status_t
_cairo_pdf_surface_write_page (cairo_pdf_surface_t *surface);

static void
_cairo_pdf_surface_write_pages (cairo_pdf_surface_t *surface);

static cairo_pdf_resource_t
_cairo_pdf_surface_write_info (cairo_pdf_surface_t *surface);

static cairo_pdf_resource_t
_cairo_pdf_surface_write_catalog (cairo_pdf_surface_t *surface);

static long
_cairo_pdf_surface_write_xref (cairo_pdf_surface_t *surface);

static cairo_status_t
_cairo_pdf_surface_write_page (cairo_pdf_surface_t *surface);

static cairo_status_t
_cairo_pdf_surface_emit_font_subsets (cairo_pdf_surface_t *surface);

static cairo_bool_t
_cairo_pdf_source_surface_equal (const void *key_a, const void *key_b);

static const cairo_surface_backend_t cairo_pdf_surface_backend;
static const cairo_paginated_surface_backend_t cairo_pdf_surface_paginated_backend;

static cairo_pdf_resource_t
_cairo_pdf_surface_new_object (cairo_pdf_surface_t *surface)
{
    cairo_pdf_resource_t resource;
    cairo_status_t status;
    cairo_pdf_object_t object;

    object.offset = _cairo_output_stream_get_position (surface->output);

    status = _cairo_array_append (&surface->objects, &object);
    if (unlikely (status)) {
	resource.id = 0;
	return resource;
    }

    resource = surface->next_available_resource;
    surface->next_available_resource.id++;

    return resource;
}

static void
_cairo_pdf_surface_update_object (cairo_pdf_surface_t	*surface,
				  cairo_pdf_resource_t	 resource)
{
    cairo_pdf_object_t *object;

    object = _cairo_array_index (&surface->objects, resource.id - 1);
    object->offset = _cairo_output_stream_get_position (surface->output);
}

static void
_cairo_pdf_surface_set_size_internal (cairo_pdf_surface_t *surface,
				      double		  width,
				      double		  height)
{
    surface->width = width;
    surface->height = height;
    cairo_matrix_init (&surface->cairo_to_pdf, 1, 0, 0, -1, 0, height);
    _cairo_pdf_operators_set_cairo_to_pdf_matrix (&surface->pdf_operators,
						  &surface->cairo_to_pdf);
}

static cairo_bool_t
_path_covers_bbox (cairo_pdf_surface_t *surface,
		   cairo_path_fixed_t *path)
{
    cairo_box_t box;

    return _cairo_path_fixed_is_box (path, &box) &&
	   box.p1.x <= 0 &&
	   box.p1.y <= 0 &&
	   box.p2.x >= _cairo_fixed_from_double (surface->width) &&
	   box.p2.y >= _cairo_fixed_from_double (surface->height);
}

static cairo_status_t
_cairo_pdf_surface_clipper_intersect_clip_path (cairo_surface_clipper_t *clipper,
						cairo_path_fixed_t	*path,
						cairo_fill_rule_t	fill_rule,
						double			tolerance,
						cairo_antialias_t	antialias)
{
    cairo_pdf_surface_t *surface = cairo_container_of (clipper,
						       cairo_pdf_surface_t,
						       clipper);
    cairo_int_status_t status;

    status = _cairo_pdf_operators_flush (&surface->pdf_operators);
    if (unlikely (status))
	return status;

    if (path == NULL) {
	_cairo_output_stream_printf (surface->output, "Q q\n");

	surface->current_pattern_is_solid_color = FALSE;
	_cairo_pdf_operators_reset (&surface->pdf_operators);

	return CAIRO_STATUS_SUCCESS;
    }

    if (_path_covers_bbox (surface, path))
	return CAIRO_STATUS_SUCCESS;

    return _cairo_pdf_operators_clip (&surface->pdf_operators, path, fill_rule);
}

static cairo_surface_t *
_cairo_pdf_surface_create_for_stream_internal (cairo_output_stream_t	*output,
					       double			 width,
					       double			 height)
{
    cairo_pdf_surface_t *surface;
    cairo_status_t status, status_ignored;

    surface = malloc (sizeof (cairo_pdf_surface_t));
    if (unlikely (surface == NULL)) {
	/* destroy stream on behalf of caller */
	status = _cairo_output_stream_destroy (output);
	return _cairo_surface_create_in_error (_cairo_error (CAIRO_STATUS_NO_MEMORY));
    }

    _cairo_surface_init (&surface->base,
			 &cairo_pdf_surface_backend,
			 NULL, /* device */
			 CAIRO_CONTENT_COLOR_ALPHA);

    surface->output = output;
    surface->width = width;
    surface->height = height;
    cairo_matrix_init (&surface->cairo_to_pdf, 1, 0, 0, -1, 0, height);

    _cairo_array_init (&surface->objects, sizeof (cairo_pdf_object_t));
    _cairo_array_init (&surface->pages, sizeof (cairo_pdf_resource_t));
    _cairo_array_init (&surface->rgb_linear_functions, sizeof (cairo_pdf_rgb_linear_function_t));
    _cairo_array_init (&surface->alpha_linear_functions, sizeof (cairo_pdf_alpha_linear_function_t));
    _cairo_array_init (&surface->fonts, sizeof (cairo_pdf_font_t));
    _cairo_array_init (&surface->smask_groups, sizeof (cairo_pdf_smask_group_t *));
    _cairo_array_init (&surface->knockout_group, sizeof (cairo_pdf_resource_t));

    _cairo_array_init (&surface->page_patterns, sizeof (cairo_pdf_pattern_t));
    _cairo_array_init (&surface->page_surfaces, sizeof (cairo_pdf_source_surface_t));
    surface->all_surfaces = _cairo_hash_table_create (_cairo_pdf_source_surface_equal);
    if (unlikely (surface->all_surfaces == NULL)) {
	status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	goto BAIL0;
    }

    _cairo_pdf_group_resources_init (&surface->resources);

    surface->font_subsets = _cairo_scaled_font_subsets_create_composite ();
    if (! surface->font_subsets) {
	status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	goto BAIL1;
    }

    _cairo_scaled_font_subsets_enable_latin_subset (surface->font_subsets, TRUE);

    surface->next_available_resource.id = 1;
    surface->pages_resource = _cairo_pdf_surface_new_object (surface);
    if (surface->pages_resource.id == 0) {
	status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
        goto BAIL2;
    }

    surface->pdf_version = CAIRO_PDF_VERSION_1_5;
    surface->compress_content = TRUE;
    surface->pdf_stream.active = FALSE;
    surface->pdf_stream.old_output = NULL;
    surface->group_stream.active = FALSE;
    surface->group_stream.stream = NULL;
    surface->group_stream.mem_stream = NULL;

    surface->paginated_mode = CAIRO_PAGINATED_MODE_ANALYZE;

    surface->force_fallbacks = FALSE;
    surface->select_pattern_gstate_saved = FALSE;
    surface->current_pattern_is_solid_color = FALSE;
    surface->current_operator = CAIRO_OPERATOR_OVER;
    surface->header_emitted = FALSE;

    _cairo_surface_clipper_init (&surface->clipper,
				 _cairo_pdf_surface_clipper_intersect_clip_path);

    _cairo_pdf_operators_init (&surface->pdf_operators,
			       surface->output,
			       &surface->cairo_to_pdf,
			       surface->font_subsets);
    _cairo_pdf_operators_set_font_subsets_callback (&surface->pdf_operators,
						    _cairo_pdf_surface_add_font,
						    surface);
    _cairo_pdf_operators_enable_actual_text(&surface->pdf_operators, TRUE);

    surface->paginated_surface =  _cairo_paginated_surface_create (
	                                  &surface->base,
					  CAIRO_CONTENT_COLOR_ALPHA,
					  &cairo_pdf_surface_paginated_backend);

    status = surface->paginated_surface->status;
    if (status == CAIRO_STATUS_SUCCESS) {
	/* paginated keeps the only reference to surface now, drop ours */
	cairo_surface_destroy (&surface->base);
	return surface->paginated_surface;
    }

BAIL2:
    _cairo_scaled_font_subsets_destroy (surface->font_subsets);
BAIL1:
    _cairo_hash_table_destroy (surface->all_surfaces);
BAIL0:
    _cairo_array_fini (&surface->objects);
    free (surface);

    /* destroy stream on behalf of caller */
    status_ignored = _cairo_output_stream_destroy (output);

    return _cairo_surface_create_in_error (status);
}

/**
 * cairo_pdf_surface_create_for_stream:
 * @write_func: a #cairo_write_func_t to accept the output data, may be %NULL
 *              to indicate a no-op @write_func. With a no-op @write_func,
 *              the surface may be queried or used as a source without
 *              generating any temporary files.
 * @closure: the closure argument for @write_func
 * @width_in_points: width of the surface, in points (1 point == 1/72.0 inch)
 * @height_in_points: height of the surface, in points (1 point == 1/72.0 inch)
 *
 * Creates a PDF surface of the specified size in points to be written
 * incrementally to the stream represented by @write_func and @closure.
 *
 * Return value: a pointer to the newly created surface. The caller
 * owns the surface and should call cairo_surface_destroy() when done
 * with it.
 *
 * This function always returns a valid pointer, but it will return a
 * pointer to a "nil" surface if an error such as out of memory
 * occurs. You can use cairo_surface_status() to check for this.
 *
 * Since: 1.2
 **/
cairo_surface_t *
cairo_pdf_surface_create_for_stream (cairo_write_func_t		 write_func,
				     void			*closure,
				     double			 width_in_points,
				     double			 height_in_points)
{
    cairo_output_stream_t *output;

    output = _cairo_output_stream_create (write_func, NULL, closure);
    if (_cairo_output_stream_get_status (output))
	return _cairo_surface_create_in_error (_cairo_output_stream_destroy (output));

    return _cairo_pdf_surface_create_for_stream_internal (output,
							  width_in_points,
							  height_in_points);
}

/**
 * cairo_pdf_surface_create:
 * @filename: a filename for the PDF output (must be writable), %NULL may be
 *            used to specify no output. This will generate a PDF surface that
 *            may be queried and used as a source, without generating a
 *            temporary file.
 * @width_in_points: width of the surface, in points (1 point == 1/72.0 inch)
 * @height_in_points: height of the surface, in points (1 point == 1/72.0 inch)
 *
 * Creates a PDF surface of the specified size in points to be written
 * to @filename.
 *
 * Return value: a pointer to the newly created surface. The caller
 * owns the surface and should call cairo_surface_destroy() when done
 * with it.
 *
 * This function always returns a valid pointer, but it will return a
 * pointer to a "nil" surface if an error such as out of memory
 * occurs. You can use cairo_surface_status() to check for this.
 *
 * Since: 1.2
 **/
cairo_surface_t *
cairo_pdf_surface_create (const char		*filename,
			  double		 width_in_points,
			  double		 height_in_points)
{
    cairo_output_stream_t *output;

    output = _cairo_output_stream_create_for_filename (filename);
    if (_cairo_output_stream_get_status (output))
	return _cairo_surface_create_in_error (_cairo_output_stream_destroy (output));

    return _cairo_pdf_surface_create_for_stream_internal (output,
							  width_in_points,
							  height_in_points);
}

static cairo_bool_t
_cairo_surface_is_pdf (cairo_surface_t *surface)
{
    return surface->backend == &cairo_pdf_surface_backend;
}

/* If the abstract_surface is a paginated surface, and that paginated
 * surface's target is a pdf_surface, then set pdf_surface to that
 * target. Otherwise return FALSE.
 */
static cairo_bool_t
_extract_pdf_surface (cairo_surface_t		 *surface,
		      cairo_pdf_surface_t	**pdf_surface)
{
    cairo_surface_t *target;
    cairo_status_t status_ignored;

    if (surface->status)
	return FALSE;
    if (surface->finished) {
	status_ignored = _cairo_surface_set_error (surface,
						   _cairo_error (CAIRO_STATUS_SURFACE_FINISHED));
        return FALSE;
    }

    if (! _cairo_surface_is_paginated (surface)) {
	status_ignored = _cairo_surface_set_error (surface,
						   _cairo_error (CAIRO_STATUS_SURFACE_TYPE_MISMATCH));
	return FALSE;
    }

    target = _cairo_paginated_surface_get_target (surface);
    if (target->status) {
	status_ignored = _cairo_surface_set_error (surface,
						   target->status);
	return FALSE;
    }
    if (target->finished) {
	status_ignored = _cairo_surface_set_error (surface,
						   _cairo_error (CAIRO_STATUS_SURFACE_FINISHED));
	return FALSE;
    }

    if (! _cairo_surface_is_pdf (target)) {
	status_ignored = _cairo_surface_set_error (surface,
						   _cairo_error (CAIRO_STATUS_SURFACE_TYPE_MISMATCH));
	return FALSE;
    }

    *pdf_surface = (cairo_pdf_surface_t *) target;
    return TRUE;
}

/**
 * cairo_pdf_surface_restrict_to_version:
 * @surface: a PDF #cairo_surface_t
 * @version: PDF version
 *
 * Restricts the generated PDF file to @version. See cairo_pdf_get_versions()
 * for a list of available version values that can be used here.
 *
 * This function should only be called before any drawing operations
 * have been performed on the given surface. The simplest way to do
 * this is to call this function immediately after creating the
 * surface.
 *
 * Since: 1.10
 **/
void
cairo_pdf_surface_restrict_to_version (cairo_surface_t 		*abstract_surface,
				       cairo_pdf_version_t  	 version)
{
    cairo_pdf_surface_t *surface = NULL; /* hide compiler warning */

    if (! _extract_pdf_surface (abstract_surface, &surface))
	return;

    if (version < CAIRO_PDF_VERSION_LAST)
	surface->pdf_version = version;

    _cairo_pdf_operators_enable_actual_text(&surface->pdf_operators,
					    version >= CAIRO_PDF_VERSION_1_5);
}

/**
 * cairo_pdf_get_versions:
 * @versions: supported version list
 * @num_versions: list length
 *
 * Used to retrieve the list of supported versions. See
 * cairo_pdf_surface_restrict_to_version().
 *
 * Since: 1.10
 **/
void
cairo_pdf_get_versions (cairo_pdf_version_t const	**versions,
                        int                     	 *num_versions)
{
    if (versions != NULL)
	*versions = _cairo_pdf_versions;

    if (num_versions != NULL)
	*num_versions = CAIRO_PDF_VERSION_LAST;
}

/**
 * cairo_pdf_version_to_string:
 * @version: a version id
 *
 * Get the string representation of the given @version id. This function
 * will return %NULL if @version isn't valid. See cairo_pdf_get_versions()
 * for a way to get the list of valid version ids.
 *
 * Return value: the string associated to given version.
 *
 * Since: 1.10
 **/
const char *
cairo_pdf_version_to_string (cairo_pdf_version_t version)
{
    if (version >= CAIRO_PDF_VERSION_LAST)
	return NULL;

    return _cairo_pdf_version_strings[version];
}

/**
 * cairo_pdf_surface_set_size:
 * @surface: a PDF #cairo_surface_t
 * @width_in_points: new surface width, in points (1 point == 1/72.0 inch)
 * @height_in_points: new surface height, in points (1 point == 1/72.0 inch)
 *
 * Changes the size of a PDF surface for the current (and
 * subsequent) pages.
 *
 * This function should only be called before any drawing operations
 * have been performed on the current page. The simplest way to do
 * this is to call this function immediately after creating the
 * surface or immediately after completing a page with either
 * cairo_show_page() or cairo_copy_page().
 *
 * Since: 1.2
 **/
void
cairo_pdf_surface_set_size (cairo_surface_t	*surface,
			    double		 width_in_points,
			    double		 height_in_points)
{
    cairo_pdf_surface_t *pdf_surface = NULL; /* hide compiler warning */
    cairo_status_t status;

    if (! _extract_pdf_surface (surface, &pdf_surface))
	return;

    _cairo_pdf_surface_set_size_internal (pdf_surface,
					  width_in_points,
					  height_in_points);
    status = _cairo_paginated_surface_set_size (pdf_surface->paginated_surface,
						width_in_points,
						height_in_points);
    if (status)
	status = _cairo_surface_set_error (surface, status);
}

static void
_cairo_pdf_surface_clear (cairo_pdf_surface_t *surface)
{
    int i, size;
    cairo_pdf_pattern_t *pattern;
    cairo_pdf_source_surface_t *src_surface;
    cairo_pdf_smask_group_t *group;

    size = _cairo_array_num_elements (&surface->page_patterns);
    for (i = 0; i < size; i++) {
	pattern = (cairo_pdf_pattern_t *) _cairo_array_index (&surface->page_patterns, i);
	cairo_pattern_destroy (pattern->pattern);
    }
    _cairo_array_truncate (&surface->page_patterns, 0);

    size = _cairo_array_num_elements (&surface->page_surfaces);
    for (i = 0; i < size; i++) {
	src_surface = (cairo_pdf_source_surface_t *) _cairo_array_index (&surface->page_surfaces, i);
	cairo_surface_destroy (src_surface->surface);
    }
    _cairo_array_truncate (&surface->page_surfaces, 0);

    size = _cairo_array_num_elements (&surface->smask_groups);
    for (i = 0; i < size; i++) {
	_cairo_array_copy_element (&surface->smask_groups, i, &group);
	_cairo_pdf_smask_group_destroy (group);
    }
    _cairo_array_truncate (&surface->smask_groups, 0);
    _cairo_array_truncate (&surface->knockout_group, 0);
}

static void
_cairo_pdf_group_resources_init (cairo_pdf_group_resources_t *res)
{
    int i;

    for (i = 0; i < CAIRO_NUM_OPERATORS; i++)
	res->operators[i] = FALSE;

    _cairo_array_init (&res->alphas, sizeof (double));
    _cairo_array_init (&res->smasks, sizeof (cairo_pdf_resource_t));
    _cairo_array_init (&res->patterns, sizeof (cairo_pdf_resource_t));
    _cairo_array_init (&res->shadings, sizeof (cairo_pdf_resource_t));
    _cairo_array_init (&res->xobjects, sizeof (cairo_pdf_resource_t));
    _cairo_array_init (&res->fonts, sizeof (cairo_pdf_font_t));
}

static void
_cairo_pdf_group_resources_fini (cairo_pdf_group_resources_t *res)
{
    _cairo_array_fini (&res->alphas);
    _cairo_array_fini (&res->smasks);
    _cairo_array_fini (&res->patterns);
    _cairo_array_fini (&res->shadings);
    _cairo_array_fini (&res->xobjects);
    _cairo_array_fini (&res->fonts);
}

static void
_cairo_pdf_group_resources_clear (cairo_pdf_group_resources_t *res)
{
    int i;

    for (i = 0; i < CAIRO_NUM_OPERATORS; i++)
	res->operators[i] = FALSE;

    _cairo_array_truncate (&res->alphas, 0);
    _cairo_array_truncate (&res->smasks, 0);
    _cairo_array_truncate (&res->patterns, 0);
    _cairo_array_truncate (&res->shadings, 0);
    _cairo_array_truncate (&res->xobjects, 0);
    _cairo_array_truncate (&res->fonts, 0);
}

static void
_cairo_pdf_surface_add_operator (cairo_pdf_surface_t *surface,
				 cairo_operator_t     op)
{
    cairo_pdf_group_resources_t *res = &surface->resources;

    res->operators[op] = TRUE;
}

static cairo_status_t
_cairo_pdf_surface_add_alpha (cairo_pdf_surface_t *surface,
			      double               alpha,
			      int                 *index)
{
    int num_alphas, i;
    double other;
    cairo_status_t status;
    cairo_pdf_group_resources_t *res = &surface->resources;

    num_alphas = _cairo_array_num_elements (&res->alphas);
    for (i = 0; i < num_alphas; i++) {
	_cairo_array_copy_element (&res->alphas, i, &other);
	if (alpha == other) {
	    *index = i;
	    return CAIRO_STATUS_SUCCESS;
	}
    }

    status = _cairo_array_append (&res->alphas, &alpha);
    if (unlikely (status))
	return status;

    *index = _cairo_array_num_elements (&res->alphas) - 1;

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_pdf_surface_add_smask (cairo_pdf_surface_t  *surface,
			      cairo_pdf_resource_t  smask)
{
    return _cairo_array_append (&(surface->resources.smasks), &smask);
}

static cairo_status_t
_cairo_pdf_surface_add_pattern (cairo_pdf_surface_t  *surface,
				cairo_pdf_resource_t  pattern)
{
    return _cairo_array_append (&(surface->resources.patterns), &pattern);
}

static cairo_status_t
_cairo_pdf_surface_add_shading (cairo_pdf_surface_t  *surface,
				cairo_pdf_resource_t  shading)
{
    return _cairo_array_append (&(surface->resources.shadings), &shading);
}


static cairo_status_t
_cairo_pdf_surface_add_xobject (cairo_pdf_surface_t  *surface,
				cairo_pdf_resource_t  xobject)
{
    return _cairo_array_append (&(surface->resources.xobjects), &xobject);
}

static cairo_status_t
_cairo_pdf_surface_add_font (unsigned int        font_id,
			     unsigned int        subset_id,
			     void		*closure)
{
    cairo_pdf_surface_t *surface = closure;
    cairo_pdf_font_t font;
    int num_fonts, i;
    cairo_status_t status;
    cairo_pdf_group_resources_t *res = &surface->resources;

    num_fonts = _cairo_array_num_elements (&res->fonts);
    for (i = 0; i < num_fonts; i++) {
	_cairo_array_copy_element (&res->fonts, i, &font);
	if (font.font_id == font_id &&
	    font.subset_id == subset_id)
	    return CAIRO_STATUS_SUCCESS;
    }

    num_fonts = _cairo_array_num_elements (&surface->fonts);
    for (i = 0; i < num_fonts; i++) {
	_cairo_array_copy_element (&surface->fonts, i, &font);
	if (font.font_id == font_id &&
	    font.subset_id == subset_id)
	    return _cairo_array_append (&res->fonts, &font);
    }

    font.font_id = font_id;
    font.subset_id = subset_id;
    font.subset_resource = _cairo_pdf_surface_new_object (surface);
    if (font.subset_resource.id == 0)
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    status = _cairo_array_append (&surface->fonts, &font);
    if (unlikely (status))
	return status;

    return _cairo_array_append (&res->fonts, &font);
}

static cairo_pdf_resource_t
_cairo_pdf_surface_get_font_resource (cairo_pdf_surface_t *surface,
				      unsigned int         font_id,
				      unsigned int         subset_id)
{
    cairo_pdf_font_t font;
    int num_fonts, i;

    num_fonts = _cairo_array_num_elements (&surface->fonts);
    for (i = 0; i < num_fonts; i++) {
	_cairo_array_copy_element (&surface->fonts, i, &font);
	if (font.font_id == font_id && font.subset_id == subset_id)
	    return font.subset_resource;
    }

    font.subset_resource.id = 0;
    return font.subset_resource;
}

static const char *
_cairo_operator_to_pdf_blend_mode (cairo_operator_t op)
{
    switch (op) {
    /* The extend blend mode operators */
    case CAIRO_OPERATOR_MULTIPLY:       return "Multiply";
    case CAIRO_OPERATOR_SCREEN:         return "Screen";
    case CAIRO_OPERATOR_OVERLAY:        return "Overlay";
    case CAIRO_OPERATOR_DARKEN:         return "Darken";
    case CAIRO_OPERATOR_LIGHTEN:        return "Lighten";
    case CAIRO_OPERATOR_COLOR_DODGE:    return "ColorDodge";
    case CAIRO_OPERATOR_COLOR_BURN:     return "ColorBurn";
    case CAIRO_OPERATOR_HARD_LIGHT:     return "HardLight";
    case CAIRO_OPERATOR_SOFT_LIGHT:     return "SoftLight";
    case CAIRO_OPERATOR_DIFFERENCE:     return "Difference";
    case CAIRO_OPERATOR_EXCLUSION:      return "Exclusion";
    case CAIRO_OPERATOR_HSL_HUE:        return "Hue";
    case CAIRO_OPERATOR_HSL_SATURATION: return "Saturation";
    case CAIRO_OPERATOR_HSL_COLOR:      return "Color";
    case CAIRO_OPERATOR_HSL_LUMINOSITY: return "Luminosity";

    default:
    /* The original Porter-Duff set */
    case CAIRO_OPERATOR_CLEAR:
    case CAIRO_OPERATOR_SOURCE:
    case CAIRO_OPERATOR_OVER:
    case CAIRO_OPERATOR_IN:
    case CAIRO_OPERATOR_OUT:
    case CAIRO_OPERATOR_ATOP:
    case CAIRO_OPERATOR_DEST:
    case CAIRO_OPERATOR_DEST_OVER:
    case CAIRO_OPERATOR_DEST_IN:
    case CAIRO_OPERATOR_DEST_OUT:
    case CAIRO_OPERATOR_DEST_ATOP:
    case CAIRO_OPERATOR_XOR:
    case CAIRO_OPERATOR_ADD:
    case CAIRO_OPERATOR_SATURATE:
	return "Normal";
    }
}

static void
_cairo_pdf_surface_emit_group_resources (cairo_pdf_surface_t         *surface,
					 cairo_pdf_group_resources_t *res)
{
    int num_alphas, num_smasks, num_resources, i;
    double alpha;
    cairo_pdf_resource_t *smask, *pattern, *shading, *xobject;
    cairo_pdf_font_t *font;

    _cairo_output_stream_printf (surface->output, "<<\n");

    num_alphas = _cairo_array_num_elements (&res->alphas);
    num_smasks = _cairo_array_num_elements (&res->smasks);
    if (num_alphas > 0 || num_smasks > 0) {
	_cairo_output_stream_printf (surface->output,
				     "   /ExtGState <<\n");

	for (i = 0; i < CAIRO_NUM_OPERATORS; i++) {
	    if (res->operators[i]) {
		_cairo_output_stream_printf (surface->output,
					     "      /b%d << /BM /%s >>\n",
					     i, _cairo_operator_to_pdf_blend_mode(i));
	    }
	}

	for (i = 0; i < num_alphas; i++) {
	    _cairo_array_copy_element (&res->alphas, i, &alpha);
	    _cairo_output_stream_printf (surface->output,
					 "      /a%d << /CA %f /ca %f >>\n",
					 i, alpha, alpha);
	}

	for (i = 0; i < num_smasks; i++) {
	    smask = _cairo_array_index (&res->smasks, i);
	    _cairo_output_stream_printf (surface->output,
					 "      /s%d %d 0 R\n",
					 smask->id, smask->id);
	}

	_cairo_output_stream_printf (surface->output,
				     "   >>\n");
    }

    num_resources = _cairo_array_num_elements (&res->patterns);
    if (num_resources > 0) {
	_cairo_output_stream_printf (surface->output,
				     "   /Pattern <<");
	for (i = 0; i < num_resources; i++) {
	    pattern = _cairo_array_index (&res->patterns, i);
	    _cairo_output_stream_printf (surface->output,
					 " /p%d %d 0 R",
					 pattern->id, pattern->id);
	}

	_cairo_output_stream_printf (surface->output,
				     " >>\n");
    }

    num_resources = _cairo_array_num_elements (&res->shadings);
    if (num_resources > 0) {
	_cairo_output_stream_printf (surface->output,
				     "   /Shading <<");
	for (i = 0; i < num_resources; i++) {
	    shading = _cairo_array_index (&res->shadings, i);
	    _cairo_output_stream_printf (surface->output,
					 " /sh%d %d 0 R",
					 shading->id, shading->id);
	}

	_cairo_output_stream_printf (surface->output,
				     " >>\n");
    }

    num_resources = _cairo_array_num_elements (&res->xobjects);
    if (num_resources > 0) {
	_cairo_output_stream_printf (surface->output,
				     "   /XObject <<");

	for (i = 0; i < num_resources; i++) {
	    xobject = _cairo_array_index (&res->xobjects, i);
	    _cairo_output_stream_printf (surface->output,
					 " /x%d %d 0 R",
					 xobject->id, xobject->id);
	}

	_cairo_output_stream_printf (surface->output,
				     " >>\n");
    }

    num_resources = _cairo_array_num_elements (&res->fonts);
    if (num_resources > 0) {
	_cairo_output_stream_printf (surface->output,"   /Font <<\n");
	for (i = 0; i < num_resources; i++) {
	    font = _cairo_array_index (&res->fonts, i);
	    _cairo_output_stream_printf (surface->output,
					 "      /f-%d-%d %d 0 R\n",
					 font->font_id,
					 font->subset_id,
					 font->subset_resource.id);
	}
	_cairo_output_stream_printf (surface->output, "   >>\n");
    }

    _cairo_output_stream_printf (surface->output,
				 ">>\n");
}

static cairo_pdf_smask_group_t *
_cairo_pdf_surface_create_smask_group (cairo_pdf_surface_t	    *surface,
				       const cairo_rectangle_int_t  *extents)
{
    cairo_pdf_smask_group_t	*group;

    group = calloc (1, sizeof (cairo_pdf_smask_group_t));
    if (unlikely (group == NULL)) {
	_cairo_error_throw (CAIRO_STATUS_NO_MEMORY);
	return NULL;
    }

    group->group_res = _cairo_pdf_surface_new_object (surface);
    if (group->group_res.id == 0) {
	_cairo_error_throw (CAIRO_STATUS_NO_MEMORY);
	free (group);
	return NULL;
    }
    group->width = surface->width;
    group->height = surface->height;
    if (extents != NULL) {
	group->extents = *extents;
    } else {
	group->extents.x = 0;
	group->extents.y = 0;
	group->extents.width = surface->width;
	group->extents.height = surface->height;
    }
    group->extents = *extents;

    return group;
}

static void
_cairo_pdf_smask_group_destroy (cairo_pdf_smask_group_t *group)
{
    if (group->operation == PDF_FILL ||	group->operation == PDF_STROKE)
	_cairo_path_fixed_fini (&group->path);
    if (group->source)
	cairo_pattern_destroy (group->source);
    if (group->mask)
	cairo_pattern_destroy (group->mask);
    free (group->utf8);
    free (group->glyphs);
    free (group->clusters);
    if (group->scaled_font)
	cairo_scaled_font_destroy (group->scaled_font);
    free (group);
}

static cairo_status_t
_cairo_pdf_surface_add_smask_group (cairo_pdf_surface_t     *surface,
				    cairo_pdf_smask_group_t *group)
{
    return _cairo_array_append (&surface->smask_groups, &group);
}

static cairo_bool_t
_cairo_pdf_source_surface_equal (const void *key_a, const void *key_b)
{
    const cairo_pdf_source_surface_entry_t *a = key_a;
    const cairo_pdf_source_surface_entry_t *b = key_b;

    if (a->interpolate != b->interpolate)
	return FALSE;

    if (a->unique_id && b->unique_id && a->unique_id_length == b->unique_id_length)
	return (memcmp (a->unique_id, b->unique_id, a->unique_id_length) == 0);

    return (a->id == b->id);
}

static void
_cairo_pdf_source_surface_init_key (cairo_pdf_source_surface_entry_t *key)
{
    if (key->unique_id && key->unique_id_length > 0) {
	key->base.hash = _cairo_hash_bytes (_CAIRO_HASH_INIT_VALUE,
					    key->unique_id, key->unique_id_length);
    } else {
	key->base.hash = key->id;
    }
}

static cairo_int_status_t
_cairo_pdf_surface_acquire_source_image_from_pattern (cairo_pdf_surface_t          *surface,
						      const cairo_pattern_t        *pattern,
						      const cairo_rectangle_int_t  *extents,
						      cairo_image_surface_t       **image,
						      void                        **image_extra)
{
    switch (pattern->type) {
    case CAIRO_PATTERN_TYPE_SURFACE: {
	cairo_surface_pattern_t *surf_pat = (cairo_surface_pattern_t *) pattern;
	return _cairo_surface_acquire_source_image (surf_pat->surface, image, image_extra);
    } break;

    case CAIRO_PATTERN_TYPE_RASTER_SOURCE: {
	cairo_surface_t *surf;
	surf = _cairo_raster_source_pattern_acquire (pattern, &surface->base, extents);
	if (!surf)
	    return CAIRO_INT_STATUS_UNSUPPORTED;
	assert (cairo_surface_get_type (surf) == CAIRO_SURFACE_TYPE_IMAGE);
	*image = (cairo_image_surface_t *) surf;
    } break;

    case CAIRO_PATTERN_TYPE_SOLID:
    case CAIRO_PATTERN_TYPE_LINEAR:
    case CAIRO_PATTERN_TYPE_RADIAL:
    case CAIRO_PATTERN_TYPE_MESH:
    default:
	ASSERT_NOT_REACHED;
	break;
    }

    return CAIRO_STATUS_SUCCESS;
}

static void
_cairo_pdf_surface_release_source_image_from_pattern (cairo_pdf_surface_t          *surface,
						      const cairo_pattern_t        *pattern,
						      cairo_image_surface_t        *image,
						      void                         *image_extra)
{
    switch (pattern->type) {
    case CAIRO_PATTERN_TYPE_SURFACE: {
	cairo_surface_pattern_t *surf_pat = (cairo_surface_pattern_t *) pattern;
	_cairo_surface_release_source_image (surf_pat->surface, image, image_extra);
    } break;

    case CAIRO_PATTERN_TYPE_RASTER_SOURCE:
	_cairo_raster_source_pattern_release (pattern, &image->base);
	break;

    case CAIRO_PATTERN_TYPE_SOLID:
    case CAIRO_PATTERN_TYPE_LINEAR:
    case CAIRO_PATTERN_TYPE_RADIAL:
    case CAIRO_PATTERN_TYPE_MESH:
    default:

	ASSERT_NOT_REACHED;
	break;
    }
}

static cairo_int_status_t
_get_jpx_image_info (cairo_surface_t		 *source,
		     cairo_image_info_t		*info,
		     const unsigned char	**mime_data,
		     unsigned long		 *mime_data_length)
{
    cairo_surface_get_mime_data (source, CAIRO_MIME_TYPE_JP2,
				 mime_data, mime_data_length);
    if (*mime_data == NULL)
	return CAIRO_INT_STATUS_UNSUPPORTED;

    return _cairo_image_info_get_jpx_info (info, *mime_data, *mime_data_length);
}

static cairo_int_status_t
_get_jpeg_image_info (cairo_surface_t		 *source,
		      cairo_image_info_t	 *info,
		      const unsigned char	**mime_data,
		      unsigned long		 *mime_data_length)
{
    cairo_surface_get_mime_data (source, CAIRO_MIME_TYPE_JPEG,
				 mime_data, mime_data_length);
    if (*mime_data == NULL)
	return CAIRO_INT_STATUS_UNSUPPORTED;

    return _cairo_image_info_get_jpeg_info (info, *mime_data, *mime_data_length);
}

static cairo_int_status_t
_get_source_surface_size (cairo_surface_t         *source,
			  int                     *width,
			  int                     *height,
			  cairo_rectangle_int_t   *extents)
{
    cairo_int_status_t status;
    cairo_image_info_t info;
    const unsigned char *mime_data;
    unsigned long mime_data_length;

    if (source->type == CAIRO_SURFACE_TYPE_RECORDING) {
	if (source->backend->type == CAIRO_SURFACE_TYPE_SUBSURFACE) {
	     cairo_surface_subsurface_t *sub = (cairo_surface_subsurface_t *) source;

	     *extents = sub->extents;
	     *width  = extents->width;
	     *height = extents->height;
	} else {
	    cairo_surface_t *free_me = NULL;
	    cairo_rectangle_int_t surf_extents;
	    cairo_box_t box;
	    cairo_bool_t bounded;

	    if (_cairo_surface_is_snapshot (source))
		free_me = source = _cairo_surface_snapshot_get_target (source);

	    status = _cairo_recording_surface_get_ink_bbox ((cairo_recording_surface_t *)source,
							    &box, NULL);
	    if (unlikely (status)) {
		cairo_surface_destroy (free_me);
		return status;
	    }

	    bounded = _cairo_surface_get_extents (source, &surf_extents);
	    cairo_surface_destroy (free_me);

	    *width = surf_extents.width;
	    *height = surf_extents.height;

	    _cairo_box_round_to_rectangle (&box, extents);
	}

	return CAIRO_STATUS_SUCCESS;
    }

    extents->x = 0;
    extents->y = 0;

    status = _get_jpx_image_info (source, &info, &mime_data, &mime_data_length);
    if (status != CAIRO_INT_STATUS_UNSUPPORTED) {
	*width = info.width;
	*height = info.height;
	extents->width = info.width;
	extents->height = info.height;
	return status;
    }

    status = _get_jpeg_image_info (source, &info, &mime_data, &mime_data_length);
    if (status != CAIRO_INT_STATUS_UNSUPPORTED) {
	*width = info.width;
	*height = info.height;
	extents->width = info.width;
	extents->height = info.height;
	return status;
    }

    if (! _cairo_surface_get_extents (source, extents))
	return CAIRO_INT_STATUS_UNSUPPORTED;

    *width = extents->width;
    *height = extents->height;

    return CAIRO_STATUS_SUCCESS;
}

/**
 * _cairo_pdf_surface_add_source_surface:
 * @surface: the pdf surface
 * @source_surface: A #cairo_surface_t to use as the source surface
 * @source_pattern: A #cairo_pattern_t of type SURFACE or RASTER_SOURCE to use as the source
 * @filter: filter type of the source pattern
 * @stencil_mask: if true, the surface will be written to the PDF as an /ImageMask
 * @extents: extents of the operation that is using this source
 * @surface_res: return PDF resource number of the surface
 * @width: returns width of surface
 * @height: returns height of surface
 * @x_offset: x offset of surface
 * @t_offset: y offset of surface
 * @source_extents: returns extents of source (either ink extents or extents needed to cover @extents)
 *
 * Add surface or raster_source pattern to list of surfaces to be
 * written to the PDF file when the current page is finished. Returns
 * a PDF resource to reference the image. A hash table of all images
 * in the PDF files (keyed by CAIRO_MIME_TYPE_UNIQUE_ID or surface
 * unique_id) to ensure surfaces with the same id are only written
 * once to the PDF file.
 *
 * Only one of @source_pattern or @source_surface is to be
 * specified. Set the other to NULL.
 **/
static cairo_status_t
_cairo_pdf_surface_add_source_surface (cairo_pdf_surface_t	    *surface,
				       cairo_surface_t	            *source_surface,
				       const cairo_pattern_t	    *source_pattern,
				       cairo_filter_t		     filter,
				       cairo_bool_t                  stencil_mask,
				       const cairo_rectangle_int_t  *extents,
				       cairo_pdf_resource_t	    *surface_res,
				       int                          *width,
				       int                          *height,
				       double                       *x_offset,
				       double                       *y_offset,
				       cairo_rectangle_int_t        *source_extents)
{
    cairo_pdf_source_surface_t src_surface;
    cairo_pdf_source_surface_entry_t surface_key;
    cairo_pdf_source_surface_entry_t *surface_entry;
    cairo_status_t status;
    cairo_bool_t interpolate;
    unsigned char *unique_id;
    unsigned long unique_id_length = 0;
    cairo_box_t box;
    cairo_rectangle_int_t rect;
    cairo_image_surface_t *image;
    void *image_extra;

    switch (filter) {
    default:
    case CAIRO_FILTER_GOOD:
    case CAIRO_FILTER_BEST:
    case CAIRO_FILTER_BILINEAR:
	interpolate = TRUE;
	break;
    case CAIRO_FILTER_FAST:
    case CAIRO_FILTER_NEAREST:
    case CAIRO_FILTER_GAUSSIAN:
	interpolate = FALSE;
	break;
    }

    *x_offset = 0;
    *y_offset = 0;
    if (source_pattern) {
	if (source_pattern->type == CAIRO_PATTERN_TYPE_RASTER_SOURCE) {
	    /* get the operation extents in pattern space */
	    _cairo_box_from_rectangle (&box, extents);
	    _cairo_matrix_transform_bounding_box_fixed (&source_pattern->matrix, &box, NULL);
	    _cairo_box_round_to_rectangle (&box, &rect);
	    status = _cairo_pdf_surface_acquire_source_image_from_pattern (surface, source_pattern,
									   &rect, &image,
									   &image_extra);
	    if (unlikely (status))
		return status;
	    source_surface = &image->base;
	    cairo_surface_get_device_offset (source_surface, x_offset, y_offset);
	} else {
	    cairo_surface_pattern_t *surface_pattern = (cairo_surface_pattern_t *) source_pattern;
	    source_surface = surface_pattern->surface;
	}
    }

    surface_key.id  = source_surface->unique_id;
    surface_key.interpolate = interpolate;
    cairo_surface_get_mime_data (source_surface, CAIRO_MIME_TYPE_UNIQUE_ID,
				 (const unsigned char **) &surface_key.unique_id,
				 &surface_key.unique_id_length);
    _cairo_pdf_source_surface_init_key (&surface_key);
    surface_entry = _cairo_hash_table_lookup (surface->all_surfaces, &surface_key.base);
    if (surface_entry) {
	*surface_res = surface_entry->surface_res;
	*width = surface_entry->width;
	*height = surface_entry->height;
	*source_extents = surface_entry->extents;
	status = CAIRO_STATUS_SUCCESS;
    } else {
	status = _get_source_surface_size (source_surface,
					   width,
					   height,
					   source_extents);
	if (unlikely(status))
	    goto release_source;

	if (surface_key.unique_id && surface_key.unique_id_length > 0) {
	    unique_id = _cairo_malloc (surface_key.unique_id_length);
	    if (unique_id == NULL) {
		status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
		goto release_source;
	    }

	    unique_id_length = surface_key.unique_id_length;
	    memcpy (unique_id, surface_key.unique_id, unique_id_length);
	} else {
	    unique_id = NULL;
	    unique_id_length = 0;
	}
    }

release_source:
    if (source_pattern && source_pattern->type == CAIRO_PATTERN_TYPE_RASTER_SOURCE)
	_cairo_pdf_surface_release_source_image_from_pattern (surface, source_pattern, image, image_extra);

    if (status || surface_entry)
	return status;

    surface_entry = malloc (sizeof (cairo_pdf_source_surface_entry_t));
    if (surface_entry == NULL) {
	status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	goto fail1;
    }

    surface_entry->id = surface_key.id;
    surface_entry->interpolate = interpolate;
    surface_entry->stencil_mask = stencil_mask;
    surface_entry->unique_id_length = unique_id_length;
    surface_entry->unique_id = unique_id;
    surface_entry->width = *width;
    surface_entry->height = *height;
    surface_entry->x_offset = *x_offset;
    surface_entry->y_offset = *y_offset;
    surface_entry->extents = *source_extents;
    _cairo_pdf_source_surface_init_key (surface_entry);

    src_surface.hash_entry = surface_entry;
    if (source_pattern && source_pattern->type == CAIRO_PATTERN_TYPE_RASTER_SOURCE) {
	src_surface.type = CAIRO_PATTERN_TYPE_RASTER_SOURCE;
	src_surface.surface = NULL;
	status = _cairo_pattern_create_copy (&src_surface.raster_pattern, source_pattern);
	if (unlikely (status))
	    goto fail2;

    } else {
	src_surface.type = CAIRO_PATTERN_TYPE_SURFACE;
	src_surface.surface = cairo_surface_reference (source_surface);
	src_surface.raster_pattern = NULL;
    }

    surface_entry->surface_res = _cairo_pdf_surface_new_object (surface);
    if (surface_entry->surface_res.id == 0) {
	status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	goto fail3;
    }

    status = _cairo_array_append (&surface->page_surfaces, &src_surface);
    if (unlikely (status))
	goto fail3;

    status = _cairo_hash_table_insert (surface->all_surfaces,
				       &surface_entry->base);
    if (unlikely(status))
	goto fail3;

    *surface_res = surface_entry->surface_res;

    return status;

fail3:
    if (source_pattern && source_pattern->type == CAIRO_PATTERN_TYPE_RASTER_SOURCE)
	cairo_pattern_destroy (src_surface.raster_pattern);
    else
	cairo_surface_destroy (src_surface.surface);

fail2:
    free (surface_entry);

fail1:
    free (unique_id);

    return status;
}

static cairo_status_t
_cairo_pdf_surface_add_pdf_pattern_or_shading (cairo_pdf_surface_t	   *surface,
					       const cairo_pattern_t	   *pattern,
					       const cairo_rectangle_int_t *extents,
					       cairo_bool_t                 is_shading,
					       cairo_pdf_resource_t	   *pattern_res,
					       cairo_pdf_resource_t	   *gstate_res)
{
    cairo_pdf_pattern_t pdf_pattern;
    cairo_status_t status;

    pdf_pattern.is_shading = is_shading;

    /* Solid colors are emitted into the content stream */
    if (pattern->type == CAIRO_PATTERN_TYPE_SOLID) {
	pattern_res->id = 0;
	gstate_res->id = 0;
	return CAIRO_STATUS_SUCCESS;
    }

    status = _cairo_pattern_create_copy (&pdf_pattern.pattern, pattern);
    if (unlikely (status))
	return status;

    pdf_pattern.pattern_res = _cairo_pdf_surface_new_object (surface);
    if (pdf_pattern.pattern_res.id == 0) {
	cairo_pattern_destroy (pdf_pattern.pattern);
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);
    }

    pdf_pattern.gstate_res.id = 0;

    /* gradient patterns require an smask object to implement transparency */
    if (pattern->type == CAIRO_PATTERN_TYPE_LINEAR ||
	pattern->type == CAIRO_PATTERN_TYPE_RADIAL ||
	pattern->type == CAIRO_PATTERN_TYPE_MESH)
    {
	double min_alpha;

	_cairo_pattern_alpha_range (pattern, &min_alpha, NULL);
	if (! CAIRO_ALPHA_IS_OPAQUE (min_alpha)) {
            pdf_pattern.gstate_res = _cairo_pdf_surface_new_object (surface);
	    if (pdf_pattern.gstate_res.id == 0) {
		cairo_pattern_destroy (pdf_pattern.pattern);
		return _cairo_error (CAIRO_STATUS_NO_MEMORY);
	    }
        }
    }

    pdf_pattern.width  = surface->width;
    pdf_pattern.height = surface->height;
    if (extents != NULL) {
	pdf_pattern.extents = *extents;
    } else {
	pdf_pattern.extents.x = 0;
	pdf_pattern.extents.y = 0;
	pdf_pattern.extents.width  = surface->width;
	pdf_pattern.extents.height = surface->height;
    }

    *pattern_res = pdf_pattern.pattern_res;
    *gstate_res = pdf_pattern.gstate_res;

    status = _cairo_array_append (&surface->page_patterns, &pdf_pattern);
    if (unlikely (status)) {
	cairo_pattern_destroy (pdf_pattern.pattern);
	return status;
    }

    return CAIRO_STATUS_SUCCESS;
}

/* Get BBox in PDF coordinates from extents in cairo coordinates */
static void
_get_bbox_from_extents (double                       surface_height,
		       const cairo_rectangle_int_t *extents,
		       cairo_box_double_t          *bbox)
{
    bbox->p1.x = extents->x;
    bbox->p1.y = surface_height - (extents->y + extents->height);
    bbox->p2.x = extents->x + extents->width;
    bbox->p2.y = surface_height - extents->y;
}

static cairo_status_t
_cairo_pdf_surface_add_pdf_shading (cairo_pdf_surface_t		*surface,
				    const cairo_pattern_t	*pattern,
				    const cairo_rectangle_int_t	*extents,
				    cairo_pdf_resource_t	*shading_res,
				    cairo_pdf_resource_t	*gstate_res)
{
    return _cairo_pdf_surface_add_pdf_pattern_or_shading (surface,
							  pattern,
							  extents,
							  TRUE,
							  shading_res,
							  gstate_res);
}

static cairo_status_t
_cairo_pdf_surface_add_pdf_pattern (cairo_pdf_surface_t		*surface,
				    const cairo_pattern_t	*pattern,
				    const cairo_rectangle_int_t	*extents,
				    cairo_pdf_resource_t	*pattern_res,
				    cairo_pdf_resource_t	*gstate_res)
{
    return _cairo_pdf_surface_add_pdf_pattern_or_shading (surface,
							  pattern,
							  extents,
							  FALSE,
							  pattern_res,
							  gstate_res);
}

static cairo_status_t
_cairo_pdf_surface_open_stream (cairo_pdf_surface_t	*surface,
				cairo_pdf_resource_t    *resource,
				cairo_bool_t             compressed,
				const char		*fmt,
				...)
{
    va_list ap;
    cairo_pdf_resource_t self, length;
    cairo_output_stream_t *output = NULL;

    if (resource) {
	self = *resource;
	_cairo_pdf_surface_update_object (surface, self);
    } else {
	self = _cairo_pdf_surface_new_object (surface);
	if (self.id == 0)
	    return _cairo_error (CAIRO_STATUS_NO_MEMORY);
    }

    length = _cairo_pdf_surface_new_object (surface);
    if (length.id == 0)
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    if (compressed) {
	output = _cairo_deflate_stream_create (surface->output);
	if (_cairo_output_stream_get_status (output))
	    return _cairo_output_stream_destroy (output);
    }

    surface->pdf_stream.active = TRUE;
    surface->pdf_stream.self = self;
    surface->pdf_stream.length = length;
    surface->pdf_stream.compressed = compressed;
    surface->current_pattern_is_solid_color = FALSE;
    surface->current_operator = CAIRO_OPERATOR_OVER;
    _cairo_pdf_operators_reset (&surface->pdf_operators);

    _cairo_output_stream_printf (surface->output,
				 "%d 0 obj\n"
				 "<< /Length %d 0 R\n",
				 surface->pdf_stream.self.id,
				 surface->pdf_stream.length.id);
    if (compressed)
	_cairo_output_stream_printf (surface->output,
				     "   /Filter /FlateDecode\n");

    if (fmt != NULL) {
	va_start (ap, fmt);
	_cairo_output_stream_vprintf (surface->output, fmt, ap);
	va_end (ap);
    }

    _cairo_output_stream_printf (surface->output,
				 ">>\n"
				 "stream\n");

    surface->pdf_stream.start_offset = _cairo_output_stream_get_position (surface->output);

    if (compressed) {
	assert (surface->pdf_stream.old_output == NULL);
        surface->pdf_stream.old_output = surface->output;
        surface->output = output;
	_cairo_pdf_operators_set_stream (&surface->pdf_operators, surface->output);
    }

    return _cairo_output_stream_get_status (surface->output);
}

static cairo_status_t
_cairo_pdf_surface_close_stream (cairo_pdf_surface_t *surface)
{
    cairo_status_t status;
    long length;

    if (! surface->pdf_stream.active)
	return CAIRO_STATUS_SUCCESS;

    status = _cairo_pdf_operators_flush (&surface->pdf_operators);

    if (surface->pdf_stream.compressed) {
	cairo_status_t status2;

	status2 = _cairo_output_stream_destroy (surface->output);
	if (likely (status == CAIRO_STATUS_SUCCESS))
	    status = status2;

	surface->output = surface->pdf_stream.old_output;
	_cairo_pdf_operators_set_stream (&surface->pdf_operators, surface->output);
	surface->pdf_stream.old_output = NULL;
    }

    length = _cairo_output_stream_get_position (surface->output) -
	surface->pdf_stream.start_offset;
    _cairo_output_stream_printf (surface->output,
				 "\n"
				 "endstream\n"
				 "endobj\n");

    _cairo_pdf_surface_update_object (surface,
				      surface->pdf_stream.length);
    _cairo_output_stream_printf (surface->output,
				 "%d 0 obj\n"
				 "   %ld\n"
				 "endobj\n",
				 surface->pdf_stream.length.id,
				 length);

    surface->pdf_stream.active = FALSE;

    if (likely (status == CAIRO_STATUS_SUCCESS))
	status = _cairo_output_stream_get_status (surface->output);

    return status;
}

static void
_cairo_pdf_surface_write_memory_stream (cairo_pdf_surface_t         *surface,
					cairo_output_stream_t       *mem_stream,
					cairo_pdf_resource_t         resource,
					cairo_pdf_group_resources_t *resources,
					cairo_bool_t                 is_knockout_group,
					const cairo_box_double_t    *bbox)
{
    _cairo_pdf_surface_update_object (surface, resource);

    _cairo_output_stream_printf (surface->output,
				 "%d 0 obj\n"
				 "<< /Type /XObject\n"
				 "   /Length %d\n",
				 resource.id,
				 _cairo_memory_stream_length (mem_stream));

    if (surface->compress_content) {
	_cairo_output_stream_printf (surface->output,
				     "   /Filter /FlateDecode\n");
    }

    _cairo_output_stream_printf (surface->output,
				 "   /Subtype /Form\n"
				 "   /BBox [ %f %f %f %f ]\n"
				 "   /Group <<\n"
				 "      /Type /Group\n"
				 "      /S /Transparency\n"
				 "      /I true\n"
				 "      /CS /DeviceRGB\n",
				 bbox->p1.x, bbox->p1.y, bbox->p2.x, bbox->p2.y);

    if (is_knockout_group)
        _cairo_output_stream_printf (surface->output,
                                     "      /K true\n");

    _cairo_output_stream_printf (surface->output,
				 "   >>\n"
				 "   /Resources\n");
    _cairo_pdf_surface_emit_group_resources (surface, resources);
    _cairo_output_stream_printf (surface->output,
				 ">>\n"
				 "stream\n");
    _cairo_memory_stream_copy (mem_stream, surface->output);
    _cairo_output_stream_printf (surface->output,
				 "endstream\n"
				 "endobj\n");
}

static cairo_status_t
_cairo_pdf_surface_open_group (cairo_pdf_surface_t         *surface,
			       const cairo_box_double_t    *bbox,
			       cairo_pdf_resource_t        *resource)
{
    cairo_status_t status;

    assert (surface->pdf_stream.active == FALSE);
    assert (surface->group_stream.active == FALSE);

    surface->group_stream.active = TRUE;
    surface->current_pattern_is_solid_color = FALSE;
    surface->current_operator = CAIRO_OPERATOR_OVER;
    _cairo_pdf_operators_reset (&surface->pdf_operators);

    surface->group_stream.mem_stream = _cairo_memory_stream_create ();

    if (surface->compress_content) {
	surface->group_stream.stream =
	    _cairo_deflate_stream_create (surface->group_stream.mem_stream);
    } else {
	surface->group_stream.stream = surface->group_stream.mem_stream;
    }
    status = _cairo_output_stream_get_status (surface->group_stream.stream);

    surface->group_stream.old_output = surface->output;
    surface->output = surface->group_stream.stream;
    _cairo_pdf_operators_set_stream (&surface->pdf_operators, surface->output);
    _cairo_pdf_group_resources_clear (&surface->resources);

    if (resource) {
	surface->group_stream.resource = *resource;
    } else {
	surface->group_stream.resource = _cairo_pdf_surface_new_object (surface);
	if (surface->group_stream.resource.id == 0)
	    return _cairo_error (CAIRO_STATUS_NO_MEMORY);
    }
    surface->group_stream.is_knockout = FALSE;
    surface->group_stream.bbox = *bbox;

    return status;
}

static cairo_status_t
_cairo_pdf_surface_open_knockout_group (cairo_pdf_surface_t         *surface,
					const cairo_box_double_t    *bbox)
{
    cairo_status_t status;

    status = _cairo_pdf_surface_open_group (surface, bbox, NULL);
    if (unlikely (status))
	return status;

    surface->group_stream.is_knockout = TRUE;

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_pdf_surface_close_group (cairo_pdf_surface_t *surface,
				cairo_pdf_resource_t *group)
{
    cairo_status_t status = CAIRO_STATUS_SUCCESS, status2;

    assert (surface->pdf_stream.active == FALSE);
    assert (surface->group_stream.active == TRUE);

    status = _cairo_pdf_operators_flush (&surface->pdf_operators);
    if (unlikely (status))
	return status;

    if (surface->compress_content) {
	status = _cairo_output_stream_destroy (surface->group_stream.stream);
	surface->group_stream.stream = NULL;

	_cairo_output_stream_printf (surface->group_stream.mem_stream,
				     "\n");
    }
    surface->output = surface->group_stream.old_output;
    _cairo_pdf_operators_set_stream (&surface->pdf_operators, surface->output);
    surface->group_stream.active = FALSE;
    _cairo_pdf_surface_write_memory_stream (surface,
					    surface->group_stream.mem_stream,
					    surface->group_stream.resource,
					    &surface->resources,
					    surface->group_stream.is_knockout,
					    &surface->group_stream.bbox);
    if (group)
	*group = surface->group_stream.resource;

    status2 = _cairo_output_stream_destroy (surface->group_stream.mem_stream);
    if (status == CAIRO_STATUS_SUCCESS)
	status = status2;

    surface->group_stream.mem_stream = NULL;
    surface->group_stream.stream = NULL;

    return status;
}

static cairo_status_t
_cairo_pdf_surface_open_content_stream (cairo_pdf_surface_t       *surface,
					const cairo_box_double_t  *bbox,
					cairo_pdf_resource_t      *resource,
					cairo_bool_t               is_form)
{
    cairo_status_t status;

    assert (surface->pdf_stream.active == FALSE);
    assert (surface->group_stream.active == FALSE);

    surface->content_resources = _cairo_pdf_surface_new_object (surface);
    if (surface->content_resources.id == 0)
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    if (is_form) {
	assert (bbox != NULL);

	status =
	    _cairo_pdf_surface_open_stream (surface,
					    resource,
					    surface->compress_content,
					    "   /Type /XObject\n"
					    "   /Subtype /Form\n"
					    "   /BBox [ %f %f %f %f ]\n"
					    "   /Group <<\n"
					    "      /Type /Group\n"
					    "      /S /Transparency\n"
					    "      /I true\n"
					    "      /CS /DeviceRGB\n"
					    "   >>\n"
					    "   /Resources %d 0 R\n",
					    bbox->p1.x,
					    bbox->p1.y,
					    bbox->p2.x,
					    bbox->p2.y,
					    surface->content_resources.id);
    } else {
	status =
	    _cairo_pdf_surface_open_stream (surface,
					    resource,
					    surface->compress_content,
					    NULL);
    }
    if (unlikely (status))
	return status;

    surface->content = surface->pdf_stream.self;

    _cairo_output_stream_printf (surface->output, "q\n");

    return _cairo_output_stream_get_status (surface->output);
}

static cairo_status_t
_cairo_pdf_surface_close_content_stream (cairo_pdf_surface_t *surface)
{
    cairo_status_t status;

    assert (surface->pdf_stream.active == TRUE);
    assert (surface->group_stream.active == FALSE);

    status = _cairo_pdf_operators_flush (&surface->pdf_operators);
    if (unlikely (status))
	return status;

    _cairo_output_stream_printf (surface->output, "Q\n");
    status = _cairo_pdf_surface_close_stream (surface);
    if (unlikely (status))
	return status;

    _cairo_pdf_surface_update_object (surface, surface->content_resources);
    _cairo_output_stream_printf (surface->output,
				 "%d 0 obj\n",
				 surface->content_resources.id);
    _cairo_pdf_surface_emit_group_resources (surface, &surface->resources);
    _cairo_output_stream_printf (surface->output,
				 "endobj\n");

    return _cairo_output_stream_get_status (surface->output);
}

static void
_cairo_pdf_source_surface_entry_pluck (void *entry, void *closure)
{
    cairo_pdf_source_surface_entry_t *surface_entry = entry;
    cairo_hash_table_t *patterns = closure;

    _cairo_hash_table_remove (patterns, &surface_entry->base);
    free (surface_entry->unique_id);

    free (surface_entry);
}

static cairo_status_t
_cairo_pdf_surface_finish (void *abstract_surface)
{
    cairo_pdf_surface_t *surface = abstract_surface;
    long offset;
    cairo_pdf_resource_t info, catalog;
    cairo_status_t status, status2;

    status = surface->base.status;
    if (status == CAIRO_STATUS_SUCCESS)
	status = _cairo_pdf_surface_emit_font_subsets (surface);

    _cairo_pdf_surface_write_pages (surface);

    info = _cairo_pdf_surface_write_info (surface);
    if (info.id == 0 && status == CAIRO_STATUS_SUCCESS)
	status = _cairo_error (CAIRO_STATUS_NO_MEMORY);

    catalog = _cairo_pdf_surface_write_catalog (surface);
    if (catalog.id == 0 && status == CAIRO_STATUS_SUCCESS)
	status = _cairo_error (CAIRO_STATUS_NO_MEMORY);

    offset = _cairo_pdf_surface_write_xref (surface);

    _cairo_output_stream_printf (surface->output,
				 "trailer\n"
				 "<< /Size %d\n"
				 "   /Root %d 0 R\n"
				 "   /Info %d 0 R\n"
				 ">>\n",
				 surface->next_available_resource.id,
				 catalog.id,
				 info.id);

    _cairo_output_stream_printf (surface->output,
				 "startxref\n"
				 "%ld\n"
				 "%%%%EOF\n",
				 offset);

    /* pdf_operators has already been flushed when the last stream was
     * closed so we should never be writing anything here - however,
     * the stream may itself be in an error state. */
    status2 = _cairo_pdf_operators_fini (&surface->pdf_operators);
    if (status == CAIRO_STATUS_SUCCESS)
	status = status2;

    /* close any active streams still open due to fatal errors */
    status2 = _cairo_pdf_surface_close_stream (surface);
    if (status == CAIRO_STATUS_SUCCESS)
	status = status2;

    if (surface->group_stream.stream != NULL) {
	status2 = _cairo_output_stream_destroy (surface->group_stream.stream);
	if (status == CAIRO_STATUS_SUCCESS)
	    status = status2;
    }
    if (surface->group_stream.mem_stream != NULL) {
	status2 = _cairo_output_stream_destroy (surface->group_stream.mem_stream);
	if (status == CAIRO_STATUS_SUCCESS)
	    status = status2;
    }
    if (surface->pdf_stream.active)
	surface->output = surface->pdf_stream.old_output;
    if (surface->group_stream.active)
	surface->output = surface->group_stream.old_output;

    /* and finish the pdf surface */
    status2 = _cairo_output_stream_destroy (surface->output);
    if (status == CAIRO_STATUS_SUCCESS)
	status = status2;

    _cairo_pdf_surface_clear (surface);
    _cairo_pdf_group_resources_fini (&surface->resources);

    _cairo_array_fini (&surface->objects);
    _cairo_array_fini (&surface->pages);
    _cairo_array_fini (&surface->rgb_linear_functions);
    _cairo_array_fini (&surface->alpha_linear_functions);
    _cairo_array_fini (&surface->page_patterns);
    _cairo_array_fini (&surface->page_surfaces);
    _cairo_hash_table_foreach (surface->all_surfaces,
			       _cairo_pdf_source_surface_entry_pluck,
			       surface->all_surfaces);
    _cairo_hash_table_destroy (surface->all_surfaces);
    _cairo_array_fini (&surface->smask_groups);
    _cairo_array_fini (&surface->fonts);
    _cairo_array_fini (&surface->knockout_group);

    if (surface->font_subsets) {
	_cairo_scaled_font_subsets_destroy (surface->font_subsets);
	surface->font_subsets = NULL;
    }

    _cairo_surface_clipper_reset (&surface->clipper);

    return status;
}

static cairo_int_status_t
_cairo_pdf_surface_start_page (void *abstract_surface)
{
    cairo_pdf_surface_t *surface = abstract_surface;

    /* Document header */
    if (! surface->header_emitted) {
	const char *version;

	switch (surface->pdf_version) {
	case CAIRO_PDF_VERSION_1_4:
	    version = "1.4";
	    break;
	default:
	case CAIRO_PDF_VERSION_1_5:
	    version = "1.5";
	    break;
	}

	_cairo_output_stream_printf (surface->output,
				     "%%PDF-%s\n", version);
	_cairo_output_stream_printf (surface->output,
				     "%%%c%c%c%c\n", 181, 237, 174, 251);
	surface->header_emitted = TRUE;
    }

    _cairo_pdf_group_resources_clear (&surface->resources);

    return CAIRO_STATUS_SUCCESS;
}

static cairo_int_status_t
_cairo_pdf_surface_has_fallback_images (void		*abstract_surface,
					cairo_bool_t	 has_fallbacks)
{
    cairo_status_t status;
    cairo_pdf_surface_t *surface = abstract_surface;
    cairo_box_double_t bbox;

    surface->has_fallback_images = has_fallbacks;
    bbox.p1.x = 0;
    bbox.p1.y = 0;
    bbox.p2.x = surface->width;
    bbox.p2.y = surface->height;
    status = _cairo_pdf_surface_open_content_stream (surface, &bbox, NULL, has_fallbacks);
    if (unlikely (status))
	return status;

    return CAIRO_STATUS_SUCCESS;
}

static cairo_bool_t
_cairo_pdf_surface_supports_fine_grained_fallbacks (void *abstract_surface)
{
    return TRUE;
}

static cairo_status_t
_cairo_pdf_surface_add_padded_image_surface (cairo_pdf_surface_t          *surface,
					     const cairo_pattern_t        *source,
					     const cairo_rectangle_int_t  *extents,
					     cairo_pdf_resource_t         *surface_res,
					     int                          *width,
					     int                          *height,
					     double                       *x_offset,
					     double                       *y_offset)
{
    cairo_image_surface_t *image;
    cairo_surface_t *pad_image;
    void *image_extra;
    cairo_int_status_t status;
    int w, h;
    cairo_rectangle_int_t extents2;
    cairo_box_t box;
    cairo_rectangle_int_t rect;
    cairo_surface_pattern_t pad_pattern;

    status = _cairo_pdf_surface_acquire_source_image_from_pattern (surface, source, extents,
								   &image, &image_extra);
    if (unlikely (status))
        return status;

    pad_image = &image->base;

    /* get the operation extents in pattern space */
    _cairo_box_from_rectangle (&box, extents);
    _cairo_matrix_transform_bounding_box_fixed (&source->matrix, &box, NULL);
    _cairo_box_round_to_rectangle (&box, &rect);

    /* Check if image needs padding to fill extents */
    w = image->width;
    h = image->height;
    if (_cairo_fixed_integer_ceil(box.p1.x) < 0 ||
	_cairo_fixed_integer_ceil(box.p1.y) < 0 ||
	_cairo_fixed_integer_floor(box.p2.y) > w ||
	_cairo_fixed_integer_floor(box.p2.y) > h)
    {
	pad_image = _cairo_image_surface_create_with_content (cairo_surface_get_content (&image->base),
							      rect.width,
							      rect.height);
	if (pad_image->status) {
	    status = pad_image->status;
	    goto BAIL;
	}

	_cairo_pattern_init_for_surface (&pad_pattern, &image->base);
	cairo_matrix_init_translate (&pad_pattern.base.matrix, rect.x, rect.y);
	pad_pattern.base.extend = CAIRO_EXTEND_PAD;
	status = _cairo_surface_paint (pad_image,
				       CAIRO_OPERATOR_SOURCE, &pad_pattern.base,
				       NULL);
        _cairo_pattern_fini (&pad_pattern.base);
        if (unlikely (status))
            goto BAIL;

	cairo_surface_set_device_offset (pad_image, rect.x, rect.y);
    }

    status = _cairo_pdf_surface_add_source_surface (surface,
						    pad_image,
						    NULL,
						    source->filter,
						    FALSE,
						    extents,
						    surface_res,
						    width,
						    height,
						    x_offset,
						    y_offset,
						    &extents2);
    if (unlikely (status))
        goto BAIL;

BAIL:
    if (pad_image != &image->base)
        cairo_surface_destroy (pad_image);

    _cairo_pdf_surface_release_source_image_from_pattern (surface, source, image, image_extra);

    return status;
}

/* Emit alpha channel from the image into the given data, providing
 * an id that can be used to reference the resulting SMask object.
 *
 * In the case that the alpha channel happens to be all opaque, then
 * no SMask object will be emitted and *id_ret will be set to 0.
 *
 * When stencil_mask is TRUE, stream_res is an an input specifying the
 * resource to use. When stencil_mask is FALSE, a new resource will be
 * created and returned in stream_res.
 */
static cairo_status_t
_cairo_pdf_surface_emit_smask (cairo_pdf_surface_t	*surface,
			       cairo_image_surface_t	*image,
			       cairo_bool_t              stencil_mask,
			       const char               *interpolate,
			       cairo_pdf_resource_t	*stream_res)
{
    cairo_status_t status = CAIRO_STATUS_SUCCESS;
    char *alpha;
    unsigned long alpha_size;
    uint32_t *pixel32;
    uint8_t *pixel8;
    int i, x, y, bit, a;
    cairo_image_transparency_t transparency;

    /* This is the only image format we support, which simplifies things. */
    assert (image->format == CAIRO_FORMAT_ARGB32 ||
	    image->format == CAIRO_FORMAT_A8 ||
	    image->format == CAIRO_FORMAT_A1 );

    transparency = _cairo_image_analyze_transparency (image);
    if (stencil_mask) {
	assert (transparency == CAIRO_IMAGE_IS_OPAQUE ||
		transparency == CAIRO_IMAGE_HAS_BILEVEL_ALPHA);
    } else {
	if (transparency == CAIRO_IMAGE_IS_OPAQUE)
	    return status;
    }

    if (transparency == CAIRO_IMAGE_HAS_BILEVEL_ALPHA) {
	alpha_size = (image->width + 7) / 8 * image->height;
	alpha = _cairo_malloc_ab ((image->width+7) / 8, image->height);
    } else {
	alpha_size = image->height * image->width;
	alpha = _cairo_malloc_ab (image->height, image->width);
    }

    if (unlikely (alpha == NULL)) {
	status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	goto CLEANUP;
    }

    i = 0;
    for (y = 0; y < image->height; y++) {
	if (image->format == CAIRO_FORMAT_A1) {
	    pixel8 = (uint8_t *) (image->data + y * image->stride);

	    for (x = 0; x < (image->width + 7) / 8; x++, pixel8++) {
		a = *pixel8;
		a = CAIRO_BITSWAP8_IF_LITTLE_ENDIAN (a);
		alpha[i++] = a;
	    }
	} else {
	    pixel8 = (uint8_t *) (image->data + y * image->stride);
	    pixel32 = (uint32_t *) (image->data + y * image->stride);
	    bit = 7;
	    for (x = 0; x < image->width; x++) {
		if (image->format == CAIRO_FORMAT_ARGB32) {
		    a = (*pixel32 & 0xff000000) >> 24;
		    pixel32++;
		} else {
		    a = *pixel8;
		    pixel8++;
		}

		if (transparency == CAIRO_IMAGE_HAS_ALPHA) {
		    alpha[i++] = a;
		} else { /* transparency == CAIRO_IMAGE_HAS_BILEVEL_ALPHA or CAIRO_IMAGE_IS_OPAQUE */
		    if (bit == 7)
			alpha[i] = 0;
		    if (a != 0)
			alpha[i] |= (1 << bit);
		    bit--;
		    if (bit < 0) {
			bit = 7;
			i++;
		    }
		}
	    }
	    if (bit != 7)
		i++;
	}
    }

    if (stencil_mask) {
	status = _cairo_pdf_surface_open_stream (surface,
						 stream_res,
						 TRUE,
						 "   /Type /XObject\n"
						 "   /Subtype /Image\n"
						 "   /ImageMask true\n"
						 "   /Width %d\n"
						 "   /Height %d\n"
						 "   /Interpolate %s\n"
						 "   /BitsPerComponent 1\n"
						 "   /Decode [1 0]\n",
						 image->width, image->height, interpolate);
    } else {
	stream_res->id = 0;
	status = _cairo_pdf_surface_open_stream (surface,
						 NULL,
						 TRUE,
						 "   /Type /XObject\n"
						 "   /Subtype /Image\n"
						 "   /Width %d\n"
						 "   /Height %d\n"
						 "   /ColorSpace /DeviceGray\n"
						 "   /Interpolate %s\n"
						 "   /BitsPerComponent %d\n",
						 image->width, image->height, interpolate,
						 transparency == CAIRO_IMAGE_HAS_ALPHA ? 8 : 1);
    }
    if (unlikely (status))
	goto CLEANUP_ALPHA;

    if (!stencil_mask)
	*stream_res = surface->pdf_stream.self;

    _cairo_output_stream_write (surface->output, alpha, alpha_size);
    status = _cairo_pdf_surface_close_stream (surface);

 CLEANUP_ALPHA:
    free (alpha);
 CLEANUP:
    return status;
}

/* Emit image data into the given surface, providing a resource that
 * can be used to reference the data in image_ret. */
static cairo_status_t
_cairo_pdf_surface_emit_image (cairo_pdf_surface_t     *surface,
                               cairo_image_surface_t   *image_surf,
                               cairo_pdf_resource_t    *image_res,
			       cairo_filter_t           filter,
			       cairo_bool_t             stencil_mask)
{
    cairo_status_t status = CAIRO_STATUS_SUCCESS;
    char *data;
    unsigned long data_size;
    uint32_t *pixel;
    int i, x, y, bit;
    cairo_pdf_resource_t smask = {0}; /* squelch bogus compiler warning */
    cairo_bool_t need_smask;
    const char *interpolate = "true";
    cairo_image_color_t color;
    cairo_image_surface_t *image;

    image  = image_surf;
    if (image->format != CAIRO_FORMAT_RGB24 &&
	image->format != CAIRO_FORMAT_ARGB32 &&
	image->format != CAIRO_FORMAT_A8 &&
	image->format != CAIRO_FORMAT_A1)
    {
	cairo_surface_t *surf;
	cairo_surface_pattern_t pattern;

	surf = _cairo_image_surface_create_with_content (cairo_surface_get_content (&image_surf->base),
							 image_surf->width,
							 image_surf->height);
	image = (cairo_image_surface_t *) surf;
	if (surf->status) {
	    status = surf->status;
	    goto CLEANUP;
	}

	_cairo_pattern_init_for_surface (&pattern, &image_surf->base);
	status = _cairo_surface_paint (surf,
				       CAIRO_OPERATOR_SOURCE, &pattern.base,
				       NULL);
        _cairo_pattern_fini (&pattern.base);
        if (unlikely (status))
            goto CLEANUP;
    }

    switch (filter) {
    case CAIRO_FILTER_GOOD:
    case CAIRO_FILTER_BEST:
    case CAIRO_FILTER_BILINEAR:
	interpolate = "true";
	break;
    case CAIRO_FILTER_FAST:
    case CAIRO_FILTER_NEAREST:
    case CAIRO_FILTER_GAUSSIAN:
	interpolate = "false";
	break;
    }

    if (stencil_mask)
	return _cairo_pdf_surface_emit_smask (surface, image, stencil_mask, interpolate, image_res);

    color = _cairo_image_analyze_color (image);
    switch (color) {
	case CAIRO_IMAGE_IS_COLOR:
	case CAIRO_IMAGE_UNKNOWN_COLOR:
	    data_size = image->height * image->width * 3;
	    data = _cairo_malloc_abc (image->width, image->height, 3);
	    break;

	case CAIRO_IMAGE_IS_GRAYSCALE:
	    data_size = image->height * image->width;
	    data = _cairo_malloc_ab (image->width, image->height);
	    break;
	case CAIRO_IMAGE_IS_MONOCHROME:
	    data_size = (image->width + 7) / 8 * image->height;
	    data = _cairo_malloc_ab ((image->width+7) / 8, image->height);
	    break;
    }
    if (unlikely (data == NULL)) {
	status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	goto CLEANUP;
    }

    i = 0;
    for (y = 0; y < image->height; y++) {
	pixel = (uint32_t *) (image->data + y * image->stride);

	bit = 7;
	for (x = 0; x < image->width; x++, pixel++) {
	    int r, g, b;

	    /* XXX: We're un-premultiplying alpha here. My reading of the PDF
	     * specification suggests that we should be able to avoid having
	     * to do this by filling in the SMask's Matte dictionary
	     * appropriately, but my attempts to do that so far have
	     * failed. */
	    if (image->format == CAIRO_FORMAT_ARGB32) {
		uint8_t a;
		a = (*pixel & 0xff000000) >> 24;
		if (a == 0) {
		    r = g = b = 0;
		} else {
		    r = (((*pixel & 0xff0000) >> 16) * 255 + a / 2) / a;
		    g = (((*pixel & 0x00ff00) >>  8) * 255 + a / 2) / a;
		    b = (((*pixel & 0x0000ff) >>  0) * 255 + a / 2) / a;
		}
	    } else if (image->format == CAIRO_FORMAT_RGB24) {
		r = (*pixel & 0x00ff0000) >> 16;
		g = (*pixel & 0x0000ff00) >>  8;
		b = (*pixel & 0x000000ff) >>  0;
	    } else {
		r = g = b = 0;
	    }

	    switch (color) {
		case CAIRO_IMAGE_IS_COLOR:
		case CAIRO_IMAGE_UNKNOWN_COLOR:
		    data[i++] = r;
		    data[i++] = g;
		    data[i++] = b;
		    break;

		case CAIRO_IMAGE_IS_GRAYSCALE:
		    data[i++] = r;
		    break;

		case CAIRO_IMAGE_IS_MONOCHROME:
		    if (bit == 7)
			data[i] = 0;
		    if (r != 0)
			data[i] |= (1 << bit);
		    bit--;
		    if (bit < 0) {
			bit = 7;
			i++;
		    }
		    break;
	    }
	}
	if (bit != 7)
	    i++;
    }

    need_smask = FALSE;
    if (image->format == CAIRO_FORMAT_ARGB32 ||
	image->format == CAIRO_FORMAT_A8 ||
	image->format == CAIRO_FORMAT_A1) {
	status = _cairo_pdf_surface_emit_smask (surface, image, FALSE, interpolate, &smask);
	if (unlikely (status))
	    goto CLEANUP_RGB;

	if (smask.id)
	    need_smask = TRUE;
    }

#define IMAGE_DICTIONARY	"   /Type /XObject\n"		\
				"   /Subtype /Image\n"	\
				"   /Width %d\n"		\
				"   /Height %d\n"		\
				"   /ColorSpace %s\n"	\
	                        "   /Interpolate %s\n" \
				"   /BitsPerComponent %d\n"

    if (need_smask)
	status = _cairo_pdf_surface_open_stream (surface,
						 image_res,
						 TRUE,
						 IMAGE_DICTIONARY
						 "   /SMask %d 0 R\n",
						 image->width, image->height,
						 color == CAIRO_IMAGE_IS_COLOR ? "/DeviceRGB" : "/DeviceGray",
						 interpolate,
						 color == CAIRO_IMAGE_IS_MONOCHROME? 1 : 8,
						 smask.id);
    else
	status = _cairo_pdf_surface_open_stream (surface,
						 image_res,
						 TRUE,
						 IMAGE_DICTIONARY,
						 image->width, image->height,
						 color == CAIRO_IMAGE_IS_COLOR ? "/DeviceRGB" : "/DeviceGray",
						 interpolate,
						 color == CAIRO_IMAGE_IS_MONOCHROME? 1 : 8);
    if (unlikely (status))
	goto CLEANUP_RGB;

#undef IMAGE_DICTIONARY

    _cairo_output_stream_write (surface->output, data, data_size);
    status = _cairo_pdf_surface_close_stream (surface);

CLEANUP_RGB:
    free (data);
CLEANUP:
    if (image != image_surf)
	cairo_surface_destroy (&image->base);

    return status;
}

static cairo_int_status_t
_cairo_pdf_surface_emit_jpx_image (cairo_pdf_surface_t   *surface,
				   cairo_surface_t	 *source,
				   cairo_pdf_resource_t   res)
{
    cairo_status_t status;
    const unsigned char *mime_data;
    unsigned long mime_data_length;
    cairo_image_info_t info;

    if (surface->pdf_version < CAIRO_PDF_VERSION_1_5)
	return CAIRO_INT_STATUS_UNSUPPORTED;

    cairo_surface_get_mime_data (source, CAIRO_MIME_TYPE_JP2,
				 &mime_data, &mime_data_length);
    if (mime_data == NULL)
	return CAIRO_INT_STATUS_UNSUPPORTED;

    status = _cairo_image_info_get_jpx_info (&info, mime_data, mime_data_length);
    if (status)
	return status;

    status = _cairo_pdf_surface_open_stream (surface,
					     &res,
					     FALSE,
					     "   /Type /XObject\n"
					     "   /Subtype /Image\n"
					     "   /Width %d\n"
					     "   /Height %d\n"
					     "   /Filter /JPXDecode\n",
					     info.width,
					     info.height);
    if (status)
	return status;

    _cairo_output_stream_write (surface->output, mime_data, mime_data_length);
    status = _cairo_pdf_surface_close_stream (surface);

    return status;
}

static cairo_int_status_t
_cairo_pdf_surface_emit_jpeg_image (cairo_pdf_surface_t   *surface,
				    cairo_surface_t	  *source,
				    cairo_pdf_resource_t   res)
{
    cairo_status_t status;
    const unsigned char *mime_data;
    unsigned long mime_data_length;
    cairo_image_info_t info;
    const char *colorspace;

    cairo_surface_get_mime_data (source, CAIRO_MIME_TYPE_JPEG,
				 &mime_data, &mime_data_length);
    if (unlikely (source->status))
	return source->status;
    if (mime_data == NULL)
	return CAIRO_INT_STATUS_UNSUPPORTED;

    status = _cairo_image_info_get_jpeg_info (&info, mime_data, mime_data_length);
    if (unlikely (status))
	return status;

    switch (info.num_components) {
	case 1:
	    colorspace = "/DeviceGray";
	    break;
	case 3:
	    colorspace = "/DeviceRGB";
	    break;
	case 4:
	    colorspace = "/DeviceCMYK";
	    break;
	default:
	    return CAIRO_INT_STATUS_UNSUPPORTED;
    }

    status = _cairo_pdf_surface_open_stream (surface,
					     &res,
					     FALSE,
					     "   /Type /XObject\n"
					     "   /Subtype /Image\n"
					     "   /Width %d\n"
					     "   /Height %d\n"
					     "   /ColorSpace %s\n"
					     "   /BitsPerComponent %d\n"
					     "   /Filter /DCTDecode\n",
					     info.width,
					     info.height,
					     colorspace,
					     info.bits_per_component);
    if (unlikely (status))
	return status;

    _cairo_output_stream_write (surface->output, mime_data, mime_data_length);
    status = _cairo_pdf_surface_close_stream (surface);

    return status;
}

static cairo_status_t
_cairo_pdf_surface_emit_image_surface (cairo_pdf_surface_t        *surface,
				       cairo_pdf_source_surface_t *source)
{
    cairo_image_surface_t *image;
    void *image_extra;
    cairo_int_status_t status;

    if (source->type == CAIRO_PATTERN_TYPE_SURFACE) {
	status = _cairo_surface_acquire_source_image (source->surface, &image, &image_extra);
    } else {
	status = _cairo_pdf_surface_acquire_source_image_from_pattern (surface, source->raster_pattern,
								       &source->hash_entry->extents,
								       &image, &image_extra);
    }
    if (unlikely (status))
	return status;

    if (!source->hash_entry->stencil_mask) {
	status = _cairo_pdf_surface_emit_jpx_image (surface, &image->base, source->hash_entry->surface_res);
	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	    goto release_source;

	status = _cairo_pdf_surface_emit_jpeg_image (surface, &image->base, source->hash_entry->surface_res);
	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	    goto release_source;
    }

    status = _cairo_pdf_surface_emit_image (surface, image,
					    &source->hash_entry->surface_res,
					    source->hash_entry->interpolate,
					    source->hash_entry->stencil_mask);

release_source:
    if (source->type == CAIRO_PATTERN_TYPE_SURFACE)
	_cairo_surface_release_source_image (source->surface, image, image_extra);
    else
	_cairo_pdf_surface_release_source_image_from_pattern (surface, source->raster_pattern,
							      image, image_extra);

    return status;
}

static cairo_status_t
_cairo_pdf_surface_emit_recording_surface (cairo_pdf_surface_t        *surface,
					   cairo_pdf_source_surface_t *pdf_source)
{
    double old_width, old_height;
    cairo_paginated_mode_t old_paginated_mode;
    cairo_surface_clipper_t old_clipper;
    cairo_box_double_t bbox;
    cairo_int_status_t status;
    int alpha = 0;
    cairo_surface_t *free_me = NULL;
    cairo_surface_t *source;

    assert (pdf_source->type == CAIRO_PATTERN_TYPE_SURFACE);
    source = pdf_source->surface;
    if (_cairo_surface_is_snapshot (source))
	free_me = source = _cairo_surface_snapshot_get_target (source);

    old_width = surface->width;
    old_height = surface->height;
    old_paginated_mode = surface->paginated_mode;
    old_clipper = surface->clipper;
    _cairo_surface_clipper_init (&surface->clipper,
				 _cairo_pdf_surface_clipper_intersect_clip_path);

    _cairo_pdf_surface_set_size_internal (surface,
					  pdf_source->hash_entry->width,
					  pdf_source->hash_entry->height);
    /* Patterns are emitted after fallback images. The paginated mode
     * needs to be set to _RENDER while the recording surface is replayed
     * back to this surface.
     */
    surface->paginated_mode = CAIRO_PAGINATED_MODE_RENDER;
    _cairo_pdf_group_resources_clear (&surface->resources);
    _get_bbox_from_extents (pdf_source->hash_entry->height, &pdf_source->hash_entry->extents, &bbox);
    status = _cairo_pdf_surface_open_content_stream (surface, &bbox, &pdf_source->hash_entry->surface_res, TRUE);
    if (unlikely (status))
	goto err;

    if (cairo_surface_get_content (source) == CAIRO_CONTENT_COLOR) {
	status = _cairo_pdf_surface_add_alpha (surface, 1.0, &alpha);
	if (unlikely (status))
	    goto err;

	_cairo_output_stream_printf (surface->output,
				     "q /a%d gs 0 0 0 rg 0 0 %f %f re f Q\n",
				     alpha,
				     surface->width,
				     surface->height);
    }

    status = _cairo_recording_surface_replay_region (source,
						     NULL,
						     &surface->base,
						     CAIRO_RECORDING_REGION_NATIVE);
    assert (status != CAIRO_INT_STATUS_UNSUPPORTED);
    if (unlikely (status))
	goto err;

    status = _cairo_pdf_surface_close_content_stream (surface);

    _cairo_surface_clipper_reset (&surface->clipper);
    surface->clipper = old_clipper;
    _cairo_pdf_surface_set_size_internal (surface,
					  old_width,
					  old_height);
    surface->paginated_mode = old_paginated_mode;

err:
    cairo_surface_destroy (free_me);
    return status;
}

static cairo_status_t
_cairo_pdf_surface_emit_recording_subsurface (cairo_pdf_surface_t  *surface,
					      cairo_surface_t      *recording_surface,
					      const cairo_rectangle_int_t *extents,
					      cairo_pdf_resource_t  resource)
{
    double old_width, old_height;
    cairo_paginated_mode_t old_paginated_mode;
    cairo_surface_clipper_t old_clipper;
    cairo_box_double_t bbox;
    cairo_int_status_t status;
    int alpha = 0;

    old_width = surface->width;
    old_height = surface->height;
    old_paginated_mode = surface->paginated_mode;
    old_clipper = surface->clipper;
    _cairo_surface_clipper_init (&surface->clipper,
				 _cairo_pdf_surface_clipper_intersect_clip_path);

    _cairo_pdf_surface_set_size_internal (surface,
					  extents->width,
					  extents->height);
    /* Patterns are emitted after fallback images. The paginated mode
     * needs to be set to _RENDER while the recording surface is replayed
     * back to this surface.
     */
    surface->paginated_mode = CAIRO_PAGINATED_MODE_RENDER;
    _cairo_pdf_group_resources_clear (&surface->resources);
    _get_bbox_from_extents (extents->height, extents, &bbox);
    status = _cairo_pdf_surface_open_content_stream (surface, &bbox, &resource, TRUE);
    if (unlikely (status))
	return status;

    if (cairo_surface_get_content (recording_surface) == CAIRO_CONTENT_COLOR) {
	status = _cairo_pdf_surface_add_alpha (surface, 1.0, &alpha);
	if (unlikely (status))
	    return status;

	_cairo_output_stream_printf (surface->output,
				     "q /a%d gs 0 0 0 rg 0 0 %f %f re f Q\n",
				     alpha,
				     surface->width,
				     surface->height);
    }

    status = _cairo_recording_surface_replay_region (recording_surface,
						     extents,
						     &surface->base,
						     CAIRO_RECORDING_REGION_NATIVE);
    assert (status != CAIRO_INT_STATUS_UNSUPPORTED);
    if (unlikely (status))
	return status;

    status = _cairo_pdf_surface_close_content_stream (surface);

    _cairo_surface_clipper_reset (&surface->clipper);
    surface->clipper = old_clipper;
    _cairo_pdf_surface_set_size_internal (surface,
					  old_width,
					  old_height);
    surface->paginated_mode = old_paginated_mode;

    return status;
}

static cairo_status_t
_cairo_pdf_surface_emit_surface (cairo_pdf_surface_t        *surface,
				 cairo_pdf_source_surface_t *src_surface)
{
    if (src_surface->type == CAIRO_PATTERN_TYPE_SURFACE) {
	if (src_surface->surface->type == CAIRO_SURFACE_TYPE_RECORDING) {
	    if (src_surface->surface->backend->type == CAIRO_SURFACE_TYPE_SUBSURFACE) {
		cairo_surface_subsurface_t *sub = (cairo_surface_subsurface_t *) src_surface->surface;
		return _cairo_pdf_surface_emit_recording_subsurface (surface,
								     sub->target,
								     &sub->extents,
								     src_surface->hash_entry->surface_res);
	    } else {
		return _cairo_pdf_surface_emit_recording_surface (surface,
								  src_surface);
	    }
	}
    }
    return _cairo_pdf_surface_emit_image_surface (surface, src_surface);
}

static cairo_status_t
_cairo_pdf_surface_emit_surface_pattern (cairo_pdf_surface_t	*surface,
					 cairo_pdf_pattern_t	*pdf_pattern)
{
    cairo_pattern_t *pattern = pdf_pattern->pattern;
    cairo_status_t status;
    cairo_pdf_resource_t pattern_resource = {0};
    cairo_matrix_t cairo_p2d, pdf_p2d;
    cairo_extend_t extend = cairo_pattern_get_extend (pattern);
    double xstep, ystep;
    cairo_rectangle_int_t pattern_extents;
    int pattern_width = 0; /* squelch bogus compiler warning */
    int pattern_height = 0; /* squelch bogus compiler warning */
    double x_offset;
    double y_offset;
    char draw_surface[200];
    cairo_box_double_t     bbox;

    if (pattern->extend == CAIRO_EXTEND_PAD) {
	status = _cairo_pdf_surface_add_padded_image_surface (surface,
							      pattern,
							      &pdf_pattern->extents,
							      &pattern_resource,
							      &pattern_width,
							      &pattern_height,
							      &x_offset,
							      &y_offset);
	pattern_extents.x = 0;
	pattern_extents.y = 0;
	pattern_extents.width = pattern_width;
	pattern_extents.height = pattern_height;
    } else {
	status = _cairo_pdf_surface_add_source_surface (surface,
							NULL,
							pattern,
							pattern->filter,
							FALSE,
							&pdf_pattern->extents,
							&pattern_resource,
							&pattern_width,
							&pattern_height,
							&x_offset,
							&y_offset,
							&pattern_extents);
    }
    if (unlikely (status))
	return status;

    switch (extend) {
    case CAIRO_EXTEND_PAD:
    case CAIRO_EXTEND_NONE:
    {
	/* In PS/PDF, (as far as I can tell), all patterns are
	 * repeating. So we support cairo's EXTEND_NONE semantics
	 * by setting the repeat step size to a size large enough
	 * to guarantee that no more than a single occurrence will
	 * be visible.
	 *
	 * First, map the surface extents into pattern space (since
	 * xstep and ystep are in pattern space).  Then use an upper
	 * bound on the length of the diagonal of the pattern image
	 * and the surface as repeat size.  This guarantees to never
	 * repeat visibly.
	 */
	double x1 = 0.0, y1 = 0.0;
	double x2 = surface->width, y2 = surface->height;
	_cairo_matrix_transform_bounding_box (&pattern->matrix,
					      &x1, &y1, &x2, &y2,
					      NULL);

	/* Rather than computing precise bounds of the union, just
	 * add the surface extents unconditionally. We only
	 * required an answer that's large enough, we don't really
	 * care if it's not as tight as possible.*/
	xstep = ystep = ceil ((x2 - x1) + (y2 - y1) +
			      pattern_width + pattern_height);
    }
    break;
    case CAIRO_EXTEND_REPEAT:
	xstep = pattern_width;
	ystep = pattern_height;
	break;
    case CAIRO_EXTEND_REFLECT:
	pattern_extents.x = 0;
	pattern_extents.y = 0;
	pattern_extents.width = pattern_width*2;
	pattern_extents.height = pattern_height*2;
	xstep = pattern_width*2;
	ystep = pattern_height*2;
	break;
	/* All the rest (if any) should have been analyzed away, so this
	 * case should be unreachable. */
    default:
	ASSERT_NOT_REACHED;
	xstep = 0;
	ystep = 0;
    }

    /* At this point, (that is, within the surface backend interface),
     * the pattern's matrix maps from cairo's device space to cairo's
     * pattern space, (both with their origin at the upper-left, and
     * cairo's pattern space of size width,height).
     *
     * Then, we must emit a PDF pattern object that maps from its own
     * pattern space, (which has a size that we establish in the BBox
     * dictionary entry), to the PDF page's *initial* space, (which
     * does not benefit from the Y-axis flipping matrix that we emit
     * on each page). So the PDF patterns matrix maps from a
     * (width,height) pattern space to a device space with the origin
     * in the lower-left corner.
     *
     * So to handle all of that, we start with an identity matrix for
     * the PDF pattern to device matrix. We translate it up by the
     * image height then flip it in the Y direction, (moving us from
     * the PDF origin to cairo's origin). We then multiply in the
     * inverse of the cairo pattern matrix, (since it maps from device
     * to pattern, while we're setting up pattern to device). Finally,
     * we translate back down by the image height and flip again to
     * end up at the lower-left origin that PDF expects.
     *
     * Additionally, within the stream that paints the pattern itself,
     * we are using a PDF image object that has a size of (1,1) so we
     * have to scale it up by the image width and height to fill our
     * pattern cell.
     */
    cairo_p2d = pattern->matrix;
    status = cairo_matrix_invert (&cairo_p2d);
    /* cairo_pattern_set_matrix ensures the matrix is invertible */
    assert (status == CAIRO_STATUS_SUCCESS);

    cairo_matrix_multiply (&pdf_p2d, &cairo_p2d, &surface->cairo_to_pdf);
    cairo_matrix_translate (&pdf_p2d, -x_offset, -y_offset);
    cairo_matrix_translate (&pdf_p2d, 0.0, pattern_height);
    cairo_matrix_scale (&pdf_p2d, 1.0, -1.0);

    _get_bbox_from_extents (pattern_height, &pattern_extents, &bbox);
    _cairo_pdf_surface_update_object (surface, pdf_pattern->pattern_res);
    status = _cairo_pdf_surface_open_stream (surface,
				             &pdf_pattern->pattern_res,
					     FALSE,
					     "   /PatternType 1\n"
					     "   /BBox [ %f %f %f %f ]\n"
					     "   /XStep %f\n"
					     "   /YStep %f\n"
					     "   /TilingType 1\n"
					     "   /PaintType 1\n"
					     "   /Matrix [ %f %f %f %f %f %f ]\n"
					     "   /Resources << /XObject << /x%d %d 0 R >> >>\n",
					     bbox.p1.x, bbox.p1.y, bbox.p2.x, bbox.p2.y,
					     xstep, ystep,
					     pdf_p2d.xx, pdf_p2d.yx,
					     pdf_p2d.xy, pdf_p2d.yy,
					     pdf_p2d.x0, pdf_p2d.y0,
					     pattern_resource.id,
					     pattern_resource.id);
    if (unlikely (status))
	return status;

    if (pattern->type == CAIRO_PATTERN_TYPE_SURFACE &&
	((cairo_surface_pattern_t *) pattern)->surface->type == CAIRO_SURFACE_TYPE_RECORDING) {
	snprintf(draw_surface,
		 sizeof (draw_surface),
		 "/x%d Do\n",
		 pattern_resource.id);
    } else {
	snprintf(draw_surface,
		 sizeof (draw_surface),
		 "q %d 0 0 %d 0 0 cm /x%d Do Q",
		 pattern_width,
		 pattern_height,
		 pattern_resource.id);
    }

    if (extend == CAIRO_EXTEND_REFLECT) {
	_cairo_output_stream_printf (surface->output,
				     "q 0 0 %d %d re W n %s Q\n"
				     "q -1 0 0 1 %d 0 cm 0 0 %d %d re W n %s Q\n"
				     "q 1 0 0 -1 0 %d cm 0 0 %d %d re W n %s Q\n"
				     "q -1 0 0 -1 %d %d cm 0 0 %d %d re W n %s Q\n",
				     pattern_width, pattern_height,
				     draw_surface,
				     pattern_width*2, pattern_width, pattern_height,
				     draw_surface,
				     pattern_height*2, pattern_width, pattern_height,
				     draw_surface,
				     pattern_width*2, pattern_height*2, pattern_width, pattern_height,
				     draw_surface);
    } else {
	_cairo_output_stream_printf (surface->output,
				     " %s \n",
				     draw_surface);
    }

    status = _cairo_pdf_surface_close_stream (surface);
    if (unlikely (status))
	return status;

    return _cairo_output_stream_get_status (surface->output);
}

typedef struct _cairo_pdf_color_stop {
    double offset;
    double color[4];
    cairo_pdf_resource_t resource;
} cairo_pdf_color_stop_t;

static cairo_status_t
cairo_pdf_surface_emit_rgb_linear_function (cairo_pdf_surface_t    *surface,
                                            cairo_pdf_color_stop_t *stop1,
                                            cairo_pdf_color_stop_t *stop2,
                                            cairo_pdf_resource_t   *function)
{
    int num_elems, i;
    cairo_pdf_rgb_linear_function_t elem;
    cairo_pdf_resource_t res;
    cairo_status_t status;

    num_elems = _cairo_array_num_elements (&surface->rgb_linear_functions);
    for (i = 0; i < num_elems; i++) {
	_cairo_array_copy_element (&surface->rgb_linear_functions, i, &elem);
        if (memcmp (&elem.color1[0], &stop1->color[0], sizeof (double)*3) != 0)
            continue;
        if (memcmp (&elem.color2[0], &stop2->color[0], sizeof (double)*3) != 0)
            continue;
        *function =  elem.resource;
        return CAIRO_STATUS_SUCCESS;
    }

    res = _cairo_pdf_surface_new_object (surface);
    if (res.id == 0)
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    _cairo_output_stream_printf (surface->output,
				 "%d 0 obj\n"
				 "<< /FunctionType 2\n"
				 "   /Domain [ 0 1 ]\n"
				 "   /C0 [ %f %f %f ]\n"
				 "   /C1 [ %f %f %f ]\n"
				 "   /N 1\n"
				 ">>\n"
				 "endobj\n",
				 res.id,
                                 stop1->color[0],
                                 stop1->color[1],
                                 stop1->color[2],
                                 stop2->color[0],
                                 stop2->color[1],
                                 stop2->color[2]);

    elem.resource = res;
    memcpy (&elem.color1[0], &stop1->color[0], sizeof (double)*3);
    memcpy (&elem.color2[0], &stop2->color[0], sizeof (double)*3);

    status = _cairo_array_append (&surface->rgb_linear_functions, &elem);
    *function = res;

    return status;
}

static cairo_status_t
cairo_pdf_surface_emit_alpha_linear_function (cairo_pdf_surface_t    *surface,
                                              cairo_pdf_color_stop_t *stop1,
                                              cairo_pdf_color_stop_t *stop2,
                                              cairo_pdf_resource_t   *function)
{
    int num_elems, i;
    cairo_pdf_alpha_linear_function_t elem;
    cairo_pdf_resource_t res;
    cairo_status_t status;

    num_elems = _cairo_array_num_elements (&surface->alpha_linear_functions);
    for (i = 0; i < num_elems; i++) {
	_cairo_array_copy_element (&surface->alpha_linear_functions, i, &elem);
        if (elem.alpha1 != stop1->color[3])
            continue;
        if (elem.alpha2 != stop2->color[3])
            continue;
        *function =  elem.resource;
        return CAIRO_STATUS_SUCCESS;
    }

    res = _cairo_pdf_surface_new_object (surface);
    if (res.id == 0)
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    _cairo_output_stream_printf (surface->output,
				 "%d 0 obj\n"
				 "<< /FunctionType 2\n"
				 "   /Domain [ 0 1 ]\n"
				 "   /C0 [ %f ]\n"
				 "   /C1 [ %f ]\n"
				 "   /N 1\n"
				 ">>\n"
				 "endobj\n",
				 res.id,
                                 stop1->color[3],
                                 stop2->color[3]);

    elem.resource = res;
    elem.alpha1 = stop1->color[3];
    elem.alpha2 = stop2->color[3];

    status = _cairo_array_append (&surface->alpha_linear_functions, &elem);
    *function = res;

    return status;
}

static cairo_status_t
_cairo_pdf_surface_emit_stitched_colorgradient (cairo_pdf_surface_t    *surface,
                                                unsigned int	        n_stops,
                                                cairo_pdf_color_stop_t *stops,
                                                cairo_bool_t	        is_alpha,
                                                cairo_pdf_resource_t   *function)
{
    cairo_pdf_resource_t res;
    unsigned int i;
    cairo_status_t status;

    /* emit linear gradients between pairs of subsequent stops... */
    for (i = 0; i < n_stops-1; i++) {
        if (is_alpha) {
            status = cairo_pdf_surface_emit_alpha_linear_function (surface,
                                                                   &stops[i],
                                                                   &stops[i+1],
                                                                   &stops[i].resource);
            if (unlikely (status))
                return status;
        } else {
            status = cairo_pdf_surface_emit_rgb_linear_function (surface,
                                                                 &stops[i],
                                                                 &stops[i+1],
                                                                 &stops[i].resource);
            if (unlikely (status))
                return status;
        }
    }

    /* ... and stitch them together */
    res = _cairo_pdf_surface_new_object (surface);
    if (res.id == 0)
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    _cairo_output_stream_printf (surface->output,
				 "%d 0 obj\n"
				 "<< /FunctionType 3\n"
				 "   /Domain [ %f %f ]\n",
				 res.id,
                                 stops[0].offset,
                                 stops[n_stops - 1].offset);

    _cairo_output_stream_printf (surface->output,
				 "   /Functions [ ");
    for (i = 0; i < n_stops-1; i++)
        _cairo_output_stream_printf (surface->output,
                                     "%d 0 R ", stops[i].resource.id);
    _cairo_output_stream_printf (surface->output,
				 "]\n");

    _cairo_output_stream_printf (surface->output,
				 "   /Bounds [ ");
    for (i = 1; i < n_stops-1; i++)
        _cairo_output_stream_printf (surface->output,
				     "%f ", stops[i].offset);
    _cairo_output_stream_printf (surface->output,
				 "]\n");

    _cairo_output_stream_printf (surface->output,
				 "   /Encode [ ");
    for (i = 1; i < n_stops; i++)
        _cairo_output_stream_printf (surface->output,
				     "0 1 ");
    _cairo_output_stream_printf (surface->output,
				 "]\n");

    _cairo_output_stream_printf (surface->output,
				 ">>\n"
				 "endobj\n");

    *function = res;

    return _cairo_output_stream_get_status (surface->output);
}


static void
calc_gradient_color (cairo_pdf_color_stop_t *new_stop,
		     cairo_pdf_color_stop_t *stop1,
		     cairo_pdf_color_stop_t *stop2)
{
    int i;
    double offset = stop1->offset / (stop1->offset + 1.0 - stop2->offset);

    for (i = 0; i < 4; i++)
	new_stop->color[i] = stop1->color[i] + offset*(stop2->color[i] - stop1->color[i]);
}

#define COLOR_STOP_EPSILON 1e-6

static cairo_status_t
_cairo_pdf_surface_emit_pattern_stops (cairo_pdf_surface_t      *surface,
                                       cairo_gradient_pattern_t *pattern,
                                       cairo_pdf_resource_t     *color_function,
                                       cairo_pdf_resource_t     *alpha_function)
{
    cairo_pdf_color_stop_t *allstops, *stops;
    unsigned int n_stops;
    unsigned int i;
    cairo_bool_t emit_alpha = FALSE;
    cairo_status_t status;

    color_function->id = 0;
    alpha_function->id = 0;

    allstops = _cairo_malloc_ab ((pattern->n_stops + 2), sizeof (cairo_pdf_color_stop_t));
    if (unlikely (allstops == NULL))
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    stops = &allstops[1];
    n_stops = pattern->n_stops;

    for (i = 0; i < n_stops; i++) {
	stops[i].color[0] = pattern->stops[i].color.red;
	stops[i].color[1] = pattern->stops[i].color.green;
	stops[i].color[2] = pattern->stops[i].color.blue;
	stops[i].color[3] = pattern->stops[i].color.alpha;
        if (!CAIRO_ALPHA_IS_OPAQUE (stops[i].color[3]))
            emit_alpha = TRUE;
	stops[i].offset = pattern->stops[i].offset;
    }

    if (pattern->base.extend == CAIRO_EXTEND_REPEAT ||
	pattern->base.extend == CAIRO_EXTEND_REFLECT) {
	if (stops[0].offset > COLOR_STOP_EPSILON) {
	    if (pattern->base.extend == CAIRO_EXTEND_REFLECT)
		memcpy (allstops, stops, sizeof (cairo_pdf_color_stop_t));
	    else
		calc_gradient_color (&allstops[0], &stops[0], &stops[n_stops-1]);
	    stops = allstops;
	    n_stops++;
	}
	stops[0].offset = 0.0;

	if (stops[n_stops-1].offset < 1.0 - COLOR_STOP_EPSILON) {
	    if (pattern->base.extend == CAIRO_EXTEND_REFLECT) {
		memcpy (&stops[n_stops],
			&stops[n_stops - 1],
			sizeof (cairo_pdf_color_stop_t));
	    } else {
		calc_gradient_color (&stops[n_stops], &stops[0], &stops[n_stops-1]);
	    }
	    n_stops++;
	}
	stops[n_stops-1].offset = 1.0;
    }

    if (stops[0].offset == stops[n_stops - 1].offset) {
	/*
	 * The first and the last stops have the same offset, but we
	 * don't want a function with an empty domain, because that
	 * would provoke underdefined behaviour from rasterisers.
	 * This can only happen with EXTEND_PAD, because EXTEND_NONE
	 * is optimised into a clear pattern in cairo-gstate, and
	 * REFLECT/REPEAT are always transformed to have the first
	 * stop at t=0 and the last stop at t=1.  Thus we want a step
	 * function going from the first color to the last one.
	 *
	 * This can be accomplished by stitching three functions:
	 *  - a constant first color function,
	 *  - a step from the first color to the last color (with empty domain)
	 *  - a constant last color function
	 */
	cairo_pdf_color_stop_t pad_stops[4];

	assert (pattern->base.extend == CAIRO_EXTEND_PAD);

	pad_stops[0] = pad_stops[1] = stops[0];
	pad_stops[2] = pad_stops[3] = stops[n_stops - 1];

	pad_stops[0].offset = 0;
	pad_stops[3].offset = 1;

        status = _cairo_pdf_surface_emit_stitched_colorgradient (surface,
                                                                 4,
                                                                 pad_stops,
                                                                 FALSE,
                                                                 color_function);
        if (unlikely (status))
            goto BAIL;

        if (emit_alpha) {
            status = _cairo_pdf_surface_emit_stitched_colorgradient (surface,
                                                                     4,
                                                                     pad_stops,
                                                                     TRUE,
                                                                     alpha_function);
            if (unlikely (status))
                goto BAIL;
        }
    } else if (n_stops == 2) {
        /* no need for stitched function */
        status = cairo_pdf_surface_emit_rgb_linear_function (surface,
                                                             &stops[0],
                                                             &stops[n_stops - 1],
                                                             color_function);
        if (unlikely (status))
            goto BAIL;

        if (emit_alpha) {
            status = cairo_pdf_surface_emit_alpha_linear_function (surface,
                                                                   &stops[0],
                                                                   &stops[n_stops - 1],
                                                                   alpha_function);
            if (unlikely (status))
                goto BAIL;
        }
    } else {
        /* multiple stops: stitch. XXX possible optimization: regularly spaced
         * stops do not require stitching. XXX */
        status = _cairo_pdf_surface_emit_stitched_colorgradient (surface,
                                                                 n_stops,
                                                                 stops,
                                                                 FALSE,
                                                                 color_function);
        if (unlikely (status))
            goto BAIL;

        if (emit_alpha) {
            status = _cairo_pdf_surface_emit_stitched_colorgradient (surface,
                                                                     n_stops,
                                                                     stops,
                                                                     TRUE,
                                                                     alpha_function);
            if (unlikely (status))
                goto BAIL;
        }
    }

BAIL:
    free (allstops);
    return status;
}

static cairo_status_t
_cairo_pdf_surface_emit_repeating_function (cairo_pdf_surface_t      *surface,
					    cairo_gradient_pattern_t *pattern,
					    cairo_pdf_resource_t     *function,
					    int                       begin,
					    int                       end)
{
    cairo_pdf_resource_t res;
    int i;

    res = _cairo_pdf_surface_new_object (surface);
    if (res.id == 0)
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    _cairo_output_stream_printf (surface->output,
				 "%d 0 obj\n"
				 "<< /FunctionType 3\n"
				 "   /Domain [ %d %d ]\n",
				 res.id,
                                 begin,
                                 end);

    _cairo_output_stream_printf (surface->output,
				 "   /Functions [ ");
    for (i = begin; i < end; i++)
        _cairo_output_stream_printf (surface->output,
                                     "%d 0 R ", function->id);
    _cairo_output_stream_printf (surface->output,
				 "]\n");

    _cairo_output_stream_printf (surface->output,
				 "   /Bounds [ ");
    for (i = begin + 1; i < end; i++)
        _cairo_output_stream_printf (surface->output,
				     "%d ", i);
    _cairo_output_stream_printf (surface->output,
				 "]\n");

    _cairo_output_stream_printf (surface->output,
				 "   /Encode [ ");
    for (i = begin; i < end; i++) {
	if ((i % 2) && pattern->base.extend == CAIRO_EXTEND_REFLECT) {
	    _cairo_output_stream_printf (surface->output,
					 "1 0 ");
	} else {
	    _cairo_output_stream_printf (surface->output,
					 "0 1 ");
	}
    }
    _cairo_output_stream_printf (surface->output,
				 "]\n");

    _cairo_output_stream_printf (surface->output,
				 ">>\n"
				 "endobj\n");

    *function = res;

    return _cairo_output_stream_get_status (surface->output);
}

static cairo_status_t
cairo_pdf_surface_emit_transparency_group (cairo_pdf_surface_t  *surface,
					   cairo_pdf_pattern_t  *pdf_pattern,
					   cairo_pdf_resource_t  gstate_resource,
					   cairo_pdf_resource_t  gradient_mask)
{
    cairo_pdf_resource_t smask_resource;
    cairo_status_t status;
    char buf[100];

    if (pdf_pattern->is_shading) {
	snprintf(buf, sizeof(buf),
		 "         /Shading\n"
		 "            << /sh%d %d 0 R >>\n",
		 gradient_mask.id,
		 gradient_mask.id);
    } else {
	snprintf(buf, sizeof(buf),
		 "         /Pattern\n"
		 "            << /p%d %d 0 R >>\n",
		 gradient_mask.id,
		 gradient_mask.id);
    }

    status = _cairo_pdf_surface_open_stream (surface,
					     NULL,
					     surface->compress_content,
					     "   /Type /XObject\n"
					     "   /Subtype /Form\n"
					     "   /FormType 1\n"
					     "   /BBox [ 0 0 %f %f ]\n"
					     "   /Resources\n"
					     "      << /ExtGState\n"
					     "            << /a0 << /ca 1 /CA 1 >>"
					     "      >>\n"
					     "%s"
					     "      >>\n"
					     "   /Group\n"
					     "      << /Type /Group\n"
					     "         /S /Transparency\n"
					     "         /I true\n"
					     "         /CS /DeviceGray\n"
					     "      >>\n",
					     surface->width,
					     surface->height,
					     buf);
    if (unlikely (status))
	return status;

    if (pdf_pattern->is_shading) {
	_cairo_output_stream_printf (surface->output,
				     "/a0 gs /sh%d sh\n",
				     gradient_mask.id);
    } else {
	_cairo_output_stream_printf (surface->output,
				     "q\n"
				     "/a0 gs\n"
				     "/Pattern cs /p%d scn\n"
				     "0 0 %f %f re\n"
				     "f\n"
				     "Q\n",
				     gradient_mask.id,
				     surface->width,
				     surface->height);
    }

    status = _cairo_pdf_surface_close_stream (surface);
    if (unlikely (status))
	return status;

    smask_resource = _cairo_pdf_surface_new_object (surface);
    if (smask_resource.id == 0)
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    _cairo_output_stream_printf (surface->output,
                                 "%d 0 obj\n"
                                 "<< /Type /Mask\n"
                                 "   /S /Luminosity\n"
                                 "   /G %d 0 R\n"
                                 ">>\n"
                                 "endobj\n",
                                 smask_resource.id,
                                 surface->pdf_stream.self.id);

    /* Create GState which uses the transparency group as an SMask. */
    _cairo_pdf_surface_update_object (surface, gstate_resource);

    _cairo_output_stream_printf (surface->output,
                                 "%d 0 obj\n"
                                 "<< /Type /ExtGState\n"
                                 "   /SMask %d 0 R\n"
                                 "   /ca 1\n"
                                 "   /CA 1\n"
                                 "   /AIS false\n"
                                 ">>\n"
                                 "endobj\n",
                                 gstate_resource.id,
                                 smask_resource.id);

    return _cairo_output_stream_get_status (surface->output);
}

static void
_cairo_pdf_surface_output_gradient (cairo_pdf_surface_t        *surface,
				    const cairo_pdf_pattern_t  *pdf_pattern,
				    cairo_pdf_resource_t        pattern_resource,
				    const cairo_matrix_t       *pat_to_pdf,
				    const cairo_circle_double_t*start,
				    const cairo_circle_double_t*end,
				    const double               *domain,
				    const char                 *colorspace,
				    cairo_pdf_resource_t        color_function)
{
    _cairo_output_stream_printf (surface->output,
                                 "%d 0 obj\n",
				 pattern_resource.id);

    if (!pdf_pattern->is_shading) {
	_cairo_output_stream_printf (surface->output,
				     "<< /Type /Pattern\n"
				     "   /PatternType 2\n"
				     "   /Matrix [ %f %f %f %f %f %f ]\n"
				     "   /Shading\n",
				     pat_to_pdf->xx, pat_to_pdf->yx,
				     pat_to_pdf->xy, pat_to_pdf->yy,
				     pat_to_pdf->x0, pat_to_pdf->y0);
    }

    if (pdf_pattern->pattern->type == CAIRO_PATTERN_TYPE_LINEAR) {
	_cairo_output_stream_printf (surface->output,
				     "      << /ShadingType 2\n"
				     "         /ColorSpace %s\n"
				     "         /Coords [ %f %f %f %f ]\n",
				     colorspace,
				     start->center.x, start->center.y,
				     end->center.x, end->center.y);
    } else {
	_cairo_output_stream_printf (surface->output,
				     "      << /ShadingType 3\n"
				     "         /ColorSpace %s\n"
				     "         /Coords [ %f %f %f %f %f %f ]\n",
				     colorspace,
				     start->center.x, start->center.y,
				     MAX (start->radius, 0),
				     end->center.x, end->center.y,
				     MAX (end->radius, 0));
    }

    _cairo_output_stream_printf (surface->output,
				 "         /Domain [ %f %f ]\n",
				 domain[0], domain[1]);

    if (pdf_pattern->pattern->extend != CAIRO_EXTEND_NONE) {
        _cairo_output_stream_printf (surface->output,
                                     "         /Extend [ true true ]\n");
    } else {
        _cairo_output_stream_printf (surface->output,
                                     "         /Extend [ false false ]\n");
    }

    _cairo_output_stream_printf (surface->output,
				 "         /Function %d 0 R\n"
                                 "      >>\n",
				 color_function.id);

    if (!pdf_pattern->is_shading) {
	_cairo_output_stream_printf (surface->output,
				     ">>\n"
				     "endobj\n");
    }
}

static cairo_status_t
_cairo_pdf_surface_emit_gradient (cairo_pdf_surface_t    *surface,
				  cairo_pdf_pattern_t    *pdf_pattern)
{
    cairo_gradient_pattern_t *pattern = (cairo_gradient_pattern_t *) pdf_pattern->pattern;
    cairo_pdf_resource_t color_function, alpha_function;
    cairo_matrix_t pat_to_pdf;
    cairo_circle_double_t start, end;
    double domain[2];
    cairo_status_t status;

    assert (pattern->n_stops != 0);

    status = _cairo_pdf_surface_emit_pattern_stops (surface,
                                                    pattern,
                                                    &color_function,
                                                    &alpha_function);
    if (unlikely (status))
	return status;

    pat_to_pdf = pattern->base.matrix;
    status = cairo_matrix_invert (&pat_to_pdf);
    /* cairo_pattern_set_matrix ensures the matrix is invertible */
    assert (status == CAIRO_STATUS_SUCCESS);
    cairo_matrix_multiply (&pat_to_pdf, &pat_to_pdf, &surface->cairo_to_pdf);

    if (pattern->base.extend == CAIRO_EXTEND_REPEAT ||
	pattern->base.extend == CAIRO_EXTEND_REFLECT)
    {
	double bounds_x1, bounds_x2, bounds_y1, bounds_y2;
	double x_scale, y_scale, tolerance;

	/* TODO: use tighter extents */
	bounds_x1 = 0;
	bounds_y1 = 0;
	bounds_x2 = surface->width;
	bounds_y2 = surface->height;
	_cairo_matrix_transform_bounding_box (&pattern->base.matrix,
					      &bounds_x1, &bounds_y1,
					      &bounds_x2, &bounds_y2,
					      NULL);

	x_scale = surface->base.x_resolution / surface->base.x_fallback_resolution;
	y_scale = surface->base.y_resolution / surface->base.y_fallback_resolution;

	tolerance = fabs (_cairo_matrix_compute_determinant (&pattern->base.matrix));
	tolerance /= _cairo_matrix_transformed_circle_major_axis (&pattern->base.matrix, 1);
	tolerance *= MIN (x_scale, y_scale);

	_cairo_gradient_pattern_box_to_parameter (pattern,
						  bounds_x1, bounds_y1,
						  bounds_x2, bounds_y2,
						  tolerance, domain);
    } else if (pattern->stops[0].offset == pattern->stops[pattern->n_stops - 1].offset) {
	/*
	 * If the first and the last stop offset are the same, then
	 * the color function is a step function.
	 * _cairo_ps_surface_emit_pattern_stops emits it as a stitched
	 * function no matter how many stops the pattern has.  The
	 * domain of the stitched function will be [0 1] in this case.
	 *
	 * This is done to avoid emitting degenerate gradients for
	 * EXTEND_PAD patterns having a step color function.
	 */
	domain[0] = 0.0;
	domain[1] = 1.0;

	assert (pattern->base.extend == CAIRO_EXTEND_PAD);
    } else {
	domain[0] = pattern->stops[0].offset;
	domain[1] = pattern->stops[pattern->n_stops - 1].offset;
    }

    /* PDF requires the first and last stop to be the same as the
     * extreme coordinates. For repeating patterns this moves the
     * extreme coordinates out to the begin/end of the repeating
     * function. For non repeating patterns this may move the extreme
     * coordinates in if there are not stops at offset 0 and 1. */
    _cairo_gradient_pattern_interpolate (pattern, domain[0], &start);
    _cairo_gradient_pattern_interpolate (pattern, domain[1], &end);

    if (pattern->base.extend == CAIRO_EXTEND_REPEAT ||
	pattern->base.extend == CAIRO_EXTEND_REFLECT)
    {
	int repeat_begin, repeat_end;

	repeat_begin = floor (domain[0]);
	repeat_end = ceil (domain[1]);

	status = _cairo_pdf_surface_emit_repeating_function (surface,
							     pattern,
							     &color_function,
							     repeat_begin,
							     repeat_end);
	if (unlikely (status))
	    return status;

	if (alpha_function.id != 0) {
	    status = _cairo_pdf_surface_emit_repeating_function (surface,
								 pattern,
								 &alpha_function,
								 repeat_begin,
								 repeat_end);
	    if (unlikely (status))
		return status;
	}
    } else if (pattern->n_stops <= 2) {
	/* For EXTEND_NONE and EXTEND_PAD if there are only two stops a
	 * Type 2 function is used by itself without a stitching
	 * function. Type 2 functions always have the domain [0 1] */
	domain[0] = 0.0;
	domain[1] = 1.0;
    }

    _cairo_pdf_surface_update_object (surface, pdf_pattern->pattern_res);
    _cairo_pdf_surface_output_gradient (surface, pdf_pattern,
					pdf_pattern->pattern_res,
					&pat_to_pdf, &start, &end, domain,
					"/DeviceRGB", color_function);

    if (alpha_function.id != 0) {
	cairo_pdf_resource_t mask_resource;

	assert (pdf_pattern->gstate_res.id != 0);

        /* Create pattern for SMask. */
        mask_resource = _cairo_pdf_surface_new_object (surface);
	if (mask_resource.id == 0)
	    return _cairo_error (CAIRO_STATUS_NO_MEMORY);

	_cairo_pdf_surface_output_gradient (surface, pdf_pattern,
					    mask_resource,
					    &pat_to_pdf, &start, &end, domain,
					    "/DeviceGray", alpha_function);

	status = cairo_pdf_surface_emit_transparency_group (surface,
							    pdf_pattern,
						            pdf_pattern->gstate_res,
							    mask_resource);
	if (unlikely (status))
	    return status;
    }

    return _cairo_output_stream_get_status (surface->output);
}

static cairo_status_t
_cairo_pdf_surface_emit_mesh_pattern (cairo_pdf_surface_t    *surface,
				      cairo_pdf_pattern_t    *pdf_pattern)
{
    cairo_matrix_t pat_to_pdf;
    cairo_status_t status;
    cairo_pattern_t *pattern = pdf_pattern->pattern;
    cairo_pdf_shading_t shading;
    int i;
    cairo_pdf_resource_t res;

    pat_to_pdf = pattern->matrix;
    status = cairo_matrix_invert (&pat_to_pdf);
    /* cairo_pattern_set_matrix ensures the matrix is invertible */
    assert (status == CAIRO_STATUS_SUCCESS);

    cairo_matrix_multiply (&pat_to_pdf, &pat_to_pdf, &surface->cairo_to_pdf);

    status = _cairo_pdf_shading_init_color (&shading, (cairo_mesh_pattern_t *) pattern);
    if (unlikely (status))
	return status;

    res = _cairo_pdf_surface_new_object (surface);
    if (unlikely (res.id == 0))
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    _cairo_output_stream_printf (surface->output,
				 "%d 0 obj\n"
                                 "<< /ShadingType %d\n"
                                 "   /ColorSpace /DeviceRGB\n"
				 "   /BitsPerCoordinate %d\n"
				 "   /BitsPerComponent %d\n"
				 "   /BitsPerFlag %d\n"
				 "   /Decode [",
				 res.id,
				 shading.shading_type,
				 shading.bits_per_coordinate,
				 shading.bits_per_component,
				 shading.bits_per_flag);

    for (i = 0; i < shading.decode_array_length; i++)
	_cairo_output_stream_printf (surface->output, "%f ", shading.decode_array[i]);

    _cairo_output_stream_printf (surface->output,
				 "]\n"
				 "   /Length %ld\n"
				 ">>\n"
				 "stream\n",
				 shading.data_length);

    _cairo_output_stream_write (surface->output, shading.data, shading.data_length);

    _cairo_output_stream_printf (surface->output,
				 "\nendstream\n"
				 "endobj\n");

    _cairo_pdf_shading_fini (&shading);

    _cairo_pdf_surface_update_object (surface, pdf_pattern->pattern_res);
    _cairo_output_stream_printf (surface->output,
                                 "%d 0 obj\n"
                                 "<< /Type /Pattern\n"
                                 "   /PatternType 2\n"
                                 "   /Matrix [ %f %f %f %f %f %f ]\n"
                                 "   /Shading %d 0 R\n"
				 ">>\n"
				 "endobj\n",
				 pdf_pattern->pattern_res.id,
                                 pat_to_pdf.xx, pat_to_pdf.yx,
                                 pat_to_pdf.xy, pat_to_pdf.yy,
                                 pat_to_pdf.x0, pat_to_pdf.y0,
				 res.id);

    if (pdf_pattern->gstate_res.id != 0) {
	cairo_pdf_resource_t mask_resource;

	/* Create pattern for SMask. */
        res = _cairo_pdf_surface_new_object (surface);
	if (unlikely (res.id == 0))
	    return _cairo_error (CAIRO_STATUS_NO_MEMORY);

	status = _cairo_pdf_shading_init_alpha (&shading, (cairo_mesh_pattern_t *) pattern);
	if (unlikely (status))
	    return status;

	_cairo_output_stream_printf (surface->output,
				 "%d 0 obj\n"
                                 "<< /ShadingType %d\n"
                                 "   /ColorSpace /DeviceGray\n"
				 "   /BitsPerCoordinate %d\n"
				 "   /BitsPerComponent %d\n"
				 "   /BitsPerFlag %d\n"
				 "   /Decode [",
				 res.id,
				 shading.shading_type,
				 shading.bits_per_coordinate,
				 shading.bits_per_component,
				 shading.bits_per_flag);

	for (i = 0; i < shading.decode_array_length; i++)
	    _cairo_output_stream_printf (surface->output, "%f ", shading.decode_array[i]);

	_cairo_output_stream_printf (surface->output,
				     "]\n"
				     "   /Length %ld\n"
				     ">>\n"
				     "stream\n",
				     shading.data_length);

	_cairo_output_stream_write (surface->output, shading.data, shading.data_length);

	_cairo_output_stream_printf (surface->output,
				     "\nendstream\n"
				     "endobj\n");
	_cairo_pdf_shading_fini (&shading);

        mask_resource = _cairo_pdf_surface_new_object (surface);
	if (unlikely (mask_resource.id == 0))
	    return _cairo_error (CAIRO_STATUS_NO_MEMORY);

	_cairo_output_stream_printf (surface->output,
				     "%d 0 obj\n"
				     "<< /Type /Pattern\n"
				     "   /PatternType 2\n"
				     "   /Matrix [ %f %f %f %f %f %f ]\n"
				     "   /Shading %d 0 R\n"
				     ">>\n"
				     "endobj\n",
				     mask_resource.id,
				     pat_to_pdf.xx, pat_to_pdf.yx,
				     pat_to_pdf.xy, pat_to_pdf.yy,
				     pat_to_pdf.x0, pat_to_pdf.y0,
				     res.id);

	status = cairo_pdf_surface_emit_transparency_group (surface,
							    pdf_pattern,
						            pdf_pattern->gstate_res,
							    mask_resource);
	if (unlikely (status))
	    return status;
    }

    return _cairo_output_stream_get_status (surface->output);
}

static cairo_status_t
_cairo_pdf_surface_emit_pattern (cairo_pdf_surface_t *surface, cairo_pdf_pattern_t *pdf_pattern)
{
    double old_width, old_height;
    cairo_status_t status;

    old_width = surface->width;
    old_height = surface->height;
    _cairo_pdf_surface_set_size_internal (surface,
					  pdf_pattern->width,
					  pdf_pattern->height);

    switch (pdf_pattern->pattern->type) {
    case CAIRO_PATTERN_TYPE_SOLID:
	ASSERT_NOT_REACHED;
	status = _cairo_error (CAIRO_STATUS_PATTERN_TYPE_MISMATCH);
	break;

    case CAIRO_PATTERN_TYPE_SURFACE:
    case CAIRO_PATTERN_TYPE_RASTER_SOURCE:
	status = _cairo_pdf_surface_emit_surface_pattern (surface, pdf_pattern);
	break;

    case CAIRO_PATTERN_TYPE_LINEAR:
    case CAIRO_PATTERN_TYPE_RADIAL:
	status = _cairo_pdf_surface_emit_gradient (surface, pdf_pattern);
	break;

    case CAIRO_PATTERN_TYPE_MESH:
	status = _cairo_pdf_surface_emit_mesh_pattern (surface, pdf_pattern);
	break;

    default:
	ASSERT_NOT_REACHED;
	status = _cairo_error (CAIRO_STATUS_PATTERN_TYPE_MISMATCH);
	break;
    }

    _cairo_pdf_surface_set_size_internal (surface,
					  old_width,
					  old_height);

    return status;
}

static cairo_status_t
_cairo_pdf_surface_paint_surface_pattern (cairo_pdf_surface_t          *surface,
					  const cairo_pattern_t        *source,
					  const cairo_rectangle_int_t  *extents,
					  cairo_bool_t                  stencil_mask)
{
    cairo_pdf_resource_t surface_res;
    int width, height;
    cairo_matrix_t cairo_p2d, pdf_p2d;
    cairo_status_t status;
    int alpha;
    cairo_rectangle_int_t extents2;
    double x_offset;
    double y_offset;

    if (source->extend == CAIRO_EXTEND_PAD &&
	!(source->type == CAIRO_PATTERN_TYPE_SURFACE &&
	  ((cairo_surface_pattern_t *)source)->surface->type == CAIRO_SURFACE_TYPE_RECORDING))
    {
	status = _cairo_pdf_surface_add_padded_image_surface (surface,
							      source,
							      extents,
							      &surface_res,
							      &width,
							      &height,
							      &x_offset,
							      &y_offset);
    } else {
	status = _cairo_pdf_surface_add_source_surface (surface,
							NULL,
							source,
							source->filter,
							stencil_mask,
							extents,
							&surface_res,
							&width,
							&height,
							&x_offset,
							&y_offset,
							&extents2);
    }
    if (unlikely (status))
	return status;

    cairo_p2d = source->matrix;
    status = cairo_matrix_invert (&cairo_p2d);
    /* cairo_pattern_set_matrix ensures the matrix is invertible */
    assert (status == CAIRO_STATUS_SUCCESS);

    pdf_p2d = surface->cairo_to_pdf;
    cairo_matrix_multiply (&pdf_p2d, &cairo_p2d, &pdf_p2d);
    cairo_matrix_translate (&pdf_p2d, x_offset, y_offset);
    cairo_matrix_translate (&pdf_p2d, 0.0, height);
    cairo_matrix_scale (&pdf_p2d, 1.0, -1.0);
    if (!(source->type == CAIRO_PATTERN_TYPE_SURFACE &&
	  ((cairo_surface_pattern_t *)source)->surface->type == CAIRO_SURFACE_TYPE_RECORDING))
    {
	cairo_matrix_scale (&pdf_p2d, width, height);
    }

    status = _cairo_pdf_operators_flush (&surface->pdf_operators);
    if (unlikely (status))
	return status;

    if (! _cairo_matrix_is_identity (&pdf_p2d)) {
	_cairo_output_stream_printf (surface->output,
				     "%f %f %f %f %f %f cm\n",
				     pdf_p2d.xx, pdf_p2d.yx,
				     pdf_p2d.xy, pdf_p2d.yy,
				     pdf_p2d.x0, pdf_p2d.y0);
    }

    status = _cairo_pdf_surface_add_alpha (surface, 1.0, &alpha);
    if (unlikely (status))
	return status;

    if (stencil_mask) {
	_cairo_output_stream_printf (surface->output,
				     "/x%d Do\n",
				     surface_res.id);
    } else {
	_cairo_output_stream_printf (surface->output,
				     "/a%d gs /x%d Do\n",
				     alpha,
				     surface_res.id);
    }

    return _cairo_pdf_surface_add_xobject (surface, surface_res);
}

static cairo_status_t
_cairo_pdf_surface_paint_gradient (cairo_pdf_surface_t         *surface,
				   const cairo_pattern_t       *source,
				   const cairo_rectangle_int_t *extents)
{
    cairo_pdf_resource_t shading_res, gstate_res;
    cairo_matrix_t pat_to_pdf;
    cairo_status_t status;
    int alpha;

    status = _cairo_pdf_surface_add_pdf_shading (surface, source,
						 extents,
						 &shading_res, &gstate_res);
    if (unlikely (status == CAIRO_INT_STATUS_NOTHING_TO_DO))
	return CAIRO_STATUS_SUCCESS;
    if (unlikely (status))
	return status;

    pat_to_pdf = source->matrix;
    status = cairo_matrix_invert (&pat_to_pdf);
    /* cairo_pattern_set_matrix ensures the matrix is invertible */
    assert (status == CAIRO_STATUS_SUCCESS);
    cairo_matrix_multiply (&pat_to_pdf, &pat_to_pdf, &surface->cairo_to_pdf);

    status = _cairo_pdf_operators_flush (&surface->pdf_operators);
    if (unlikely (status))
	return status;

    if (! _cairo_matrix_is_identity (&pat_to_pdf)) {
	_cairo_output_stream_printf (surface->output,
				     "%f %f %f %f %f %f cm\n",
				     pat_to_pdf.xx, pat_to_pdf.yx,
				     pat_to_pdf.xy, pat_to_pdf.yy,
				     pat_to_pdf.x0, pat_to_pdf.y0);
    }

    status = _cairo_pdf_surface_add_shading (surface, shading_res);
    if (unlikely (status))
	return status;

    if (gstate_res.id != 0) {
	status = _cairo_pdf_surface_add_smask (surface, gstate_res);
	if (unlikely (status))
	    return status;

	_cairo_output_stream_printf (surface->output,
				     "/s%d gs /sh%d sh\n",
				     gstate_res.id,
				     shading_res.id);
    } else {
	status = _cairo_pdf_surface_add_alpha (surface, 1.0, &alpha);
	if (unlikely (status))
	    return status;

	_cairo_output_stream_printf (surface->output,
				     "/a%d gs /sh%d sh\n",
				     alpha,
				     shading_res.id);
    }

    return status;
}

static cairo_status_t
_cairo_pdf_surface_paint_pattern (cairo_pdf_surface_t          *surface,
				  const cairo_pattern_t        *source,
				  const cairo_rectangle_int_t  *extents,
				  cairo_bool_t                  mask)
{
    switch (source->type) {
    case CAIRO_PATTERN_TYPE_SURFACE:
    case CAIRO_PATTERN_TYPE_RASTER_SOURCE:
	return _cairo_pdf_surface_paint_surface_pattern (surface,
							 source,
							 extents,
							 mask);
    case CAIRO_PATTERN_TYPE_LINEAR:
    case CAIRO_PATTERN_TYPE_RADIAL:
    case CAIRO_PATTERN_TYPE_MESH:
	return _cairo_pdf_surface_paint_gradient (surface,
						  source,
						  extents);

    case CAIRO_PATTERN_TYPE_SOLID:
    default:
	ASSERT_NOT_REACHED;
	return CAIRO_STATUS_SUCCESS;
    }
}

static cairo_bool_t
_can_paint_pattern (const cairo_pattern_t *pattern)
{
    switch (pattern->type) {
    case CAIRO_PATTERN_TYPE_SOLID:
	return FALSE;

    case CAIRO_PATTERN_TYPE_SURFACE:
    case CAIRO_PATTERN_TYPE_RASTER_SOURCE:
	return (pattern->extend == CAIRO_EXTEND_NONE ||
		pattern->extend == CAIRO_EXTEND_PAD);

    case CAIRO_PATTERN_TYPE_LINEAR:
    case CAIRO_PATTERN_TYPE_RADIAL:
	return TRUE;

    case CAIRO_PATTERN_TYPE_MESH:
	return FALSE;

    default:
	ASSERT_NOT_REACHED;
	return FALSE;
    }
}

static cairo_status_t
_cairo_pdf_surface_select_operator (cairo_pdf_surface_t *surface,
				    cairo_operator_t     op)
{
    cairo_status_t status;

    if (op == surface->current_operator)
	return CAIRO_STATUS_SUCCESS;

    status = _cairo_pdf_operators_flush (&surface->pdf_operators);
    if (unlikely (status))
	return status;

    _cairo_output_stream_printf (surface->output,
				 "/b%d gs\n", op);
    surface->current_operator = op;
    _cairo_pdf_surface_add_operator (surface, op);

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_pdf_surface_select_pattern (cairo_pdf_surface_t *surface,
				   const cairo_pattern_t     *pattern,
				   cairo_pdf_resource_t pattern_res,
				   cairo_bool_t         is_stroke)
{
    cairo_status_t status;
    int alpha;
    const cairo_color_t *solid_color = NULL;

    if (pattern->type == CAIRO_PATTERN_TYPE_SOLID) {
	const cairo_solid_pattern_t *solid = (const cairo_solid_pattern_t *) pattern;

	solid_color = &solid->color;
    }

    if (solid_color != NULL) {
	if (surface->current_pattern_is_solid_color == FALSE ||
	    surface->current_color_red != solid_color->red ||
	    surface->current_color_green != solid_color->green ||
	    surface->current_color_blue != solid_color->blue ||
	    surface->current_color_is_stroke != is_stroke)
	{
	    status = _cairo_pdf_operators_flush (&surface->pdf_operators);
	    if (unlikely (status))
		return status;

	    _cairo_output_stream_printf (surface->output,
					 "%f %f %f ",
					 solid_color->red,
					 solid_color->green,
					 solid_color->blue);

	    if (is_stroke)
		_cairo_output_stream_printf (surface->output, "RG ");
	    else
		_cairo_output_stream_printf (surface->output, "rg ");

	    surface->current_color_red = solid_color->red;
	    surface->current_color_green = solid_color->green;
	    surface->current_color_blue = solid_color->blue;
	    surface->current_color_is_stroke = is_stroke;
	}

	if (surface->current_pattern_is_solid_color == FALSE ||
	    surface->current_color_alpha != solid_color->alpha)
	{
	    status = _cairo_pdf_surface_add_alpha (surface, solid_color->alpha, &alpha);
	    if (unlikely (status))
		return status;

	    status = _cairo_pdf_operators_flush (&surface->pdf_operators);
	    if (unlikely (status))
		return status;

	    _cairo_output_stream_printf (surface->output,
					 "/a%d gs\n",
					 alpha);
	    surface->current_color_alpha = solid_color->alpha;
	}

	surface->current_pattern_is_solid_color = TRUE;
    } else {
	status = _cairo_pdf_surface_add_alpha (surface, 1.0, &alpha);
	if (unlikely (status))
	    return status;

	status = _cairo_pdf_surface_add_pattern (surface, pattern_res);
	if (unlikely (status))
	    return status;

	status = _cairo_pdf_operators_flush (&surface->pdf_operators);
	if (unlikely (status))
	    return status;

	/* fill-stroke calls select_pattern twice. Don't save if the
	 * gstate is already saved. */
	if (!surface->select_pattern_gstate_saved)
	    _cairo_output_stream_printf (surface->output, "q ");

	if (is_stroke) {
	    _cairo_output_stream_printf (surface->output,
					 "/Pattern CS /p%d SCN ",
					 pattern_res.id);
	} else {
	    _cairo_output_stream_printf (surface->output,
					 "/Pattern cs /p%d scn ",
					 pattern_res.id);
	}
	_cairo_output_stream_printf (surface->output,
				     "/a%d gs\n",
				     alpha);
	surface->select_pattern_gstate_saved = TRUE;
	surface->current_pattern_is_solid_color = FALSE;
    }

    return _cairo_output_stream_get_status (surface->output);
}

static cairo_int_status_t
_cairo_pdf_surface_unselect_pattern (cairo_pdf_surface_t *surface)
{
    cairo_int_status_t status;

    if (surface->select_pattern_gstate_saved) {
	status = _cairo_pdf_operators_flush (&surface->pdf_operators);
	if (unlikely (status))
	    return status;

	_cairo_output_stream_printf (surface->output, "Q\n");
	_cairo_pdf_operators_reset (&surface->pdf_operators);
	surface->current_pattern_is_solid_color = FALSE;
    }
    surface->select_pattern_gstate_saved = FALSE;

    return CAIRO_STATUS_SUCCESS;
}

static cairo_int_status_t
_cairo_pdf_surface_show_page (void *abstract_surface)
{
    cairo_pdf_surface_t *surface = abstract_surface;
    cairo_int_status_t status;

    status = _cairo_pdf_surface_close_content_stream (surface);
    if (unlikely (status))
	return status;

    _cairo_surface_clipper_reset (&surface->clipper);

    status = _cairo_pdf_surface_write_page (surface);
    if (unlikely (status))
	return status;

    _cairo_pdf_surface_clear (surface);

    return CAIRO_STATUS_SUCCESS;
}

static cairo_bool_t
_cairo_pdf_surface_get_extents (void		        *abstract_surface,
				cairo_rectangle_int_t   *rectangle)
{
    cairo_pdf_surface_t *surface = abstract_surface;

    rectangle->x = 0;
    rectangle->y = 0;

    /* XXX: The conversion to integers here is pretty bogus, (not to
     * mention the arbitrary limitation of width to a short(!). We
     * may need to come up with a better interface for get_size.
     */
    rectangle->width  = ceil (surface->width);
    rectangle->height = ceil (surface->height);

    return TRUE;
}

static void
_cairo_pdf_surface_get_font_options (void                  *abstract_surface,
				     cairo_font_options_t  *options)
{
    _cairo_font_options_init_default (options);

    cairo_font_options_set_hint_style (options, CAIRO_HINT_STYLE_NONE);
    cairo_font_options_set_hint_metrics (options, CAIRO_HINT_METRICS_OFF);
    cairo_font_options_set_antialias (options, CAIRO_ANTIALIAS_GRAY);
    _cairo_font_options_set_round_glyph_positions (options, CAIRO_ROUND_GLYPH_POS_OFF);
}

static cairo_pdf_resource_t
_cairo_pdf_surface_write_info (cairo_pdf_surface_t *surface)
{
    cairo_pdf_resource_t info;

    info = _cairo_pdf_surface_new_object (surface);
    if (info.id == 0)
	return info;

    _cairo_output_stream_printf (surface->output,
				 "%d 0 obj\n"
				 "<< /Creator (cairo %s (http://cairographics.org))\n"
				 "   /Producer (cairo %s (http://cairographics.org))\n"
				 ">>\n"
				 "endobj\n",
				 info.id,
                                 cairo_version_string (),
                                 cairo_version_string ());

    return info;
}

static void
_cairo_pdf_surface_write_pages (cairo_pdf_surface_t *surface)
{
    cairo_pdf_resource_t page;
    int num_pages, i;

    _cairo_pdf_surface_update_object (surface, surface->pages_resource);
    _cairo_output_stream_printf (surface->output,
				 "%d 0 obj\n"
				 "<< /Type /Pages\n"
				 "   /Kids [ ",
				 surface->pages_resource.id);

    num_pages = _cairo_array_num_elements (&surface->pages);
    for (i = 0; i < num_pages; i++) {
	_cairo_array_copy_element (&surface->pages, i, &page);
	_cairo_output_stream_printf (surface->output, "%d 0 R ", page.id);
    }

    _cairo_output_stream_printf (surface->output, "]\n");
    _cairo_output_stream_printf (surface->output, "   /Count %d\n", num_pages);


    /* TODO: Figure out which other defaults to be inherited by /Page
     * objects. */
    _cairo_output_stream_printf (surface->output,
				 ">>\n"
				 "endobj\n");
}

static cairo_status_t
_utf8_to_pdf_string (const char *utf8, char **str_out)
{
    int i;
    int len;
    cairo_bool_t ascii;
    char *str;
    cairo_status_t status = CAIRO_STATUS_SUCCESS;

    ascii = TRUE;
    len = strlen (utf8);
    for (i = 0; i < len; i++) {
	unsigned c = utf8[i];
	if (c < 32 || c > 126 || c == '(' || c == ')' || c == '\\') {
	    ascii = FALSE;
	    break;
	}
    }

    if (ascii) {
	str = malloc (len + 3);
	if (str == NULL)
	    return _cairo_error (CAIRO_STATUS_NO_MEMORY);

	str[0] = '(';
	for (i = 0; i < len; i++)
	    str[i+1] = utf8[i];
	str[i+1] = ')';
	str[i+2] = 0;
    } else {
	uint16_t *utf16 = NULL;
	int utf16_len = 0;

	status = _cairo_utf8_to_utf16 (utf8, -1, &utf16, &utf16_len);
	if (unlikely (status))
	    return status;

	str = malloc (utf16_len*4 + 7);
	if (str == NULL) {
	    free (utf16);
	    return _cairo_error (CAIRO_STATUS_NO_MEMORY);
	}

	strcpy (str, "<FEFF");
	for (i = 0; i < utf16_len; i++)
	    snprintf (str + 4*i + 5, 5, "%04X", utf16[i]);

	strcat (str, ">");
	free (utf16);
    }
    *str_out = str;

    return status;
}

static cairo_status_t
_cairo_pdf_surface_emit_unicode_for_glyph (cairo_pdf_surface_t	*surface,
					   const char 		*utf8)
{
    uint16_t *utf16 = NULL;
    int utf16_len = 0;
    cairo_status_t status;
    int i;

    if (utf8 && *utf8) {
	status = _cairo_utf8_to_utf16 (utf8, -1, &utf16, &utf16_len);
	if (unlikely (status))
	    return status;
    }

    _cairo_output_stream_printf (surface->output, "<");
    if (utf16 == NULL || utf16_len == 0) {
	/* According to the "ToUnicode Mapping File Tutorial"
	 * http://www.adobe.com/devnet/acrobat/pdfs/5411.ToUnicode.pdf
	 *
	 * Glyphs that do not map to a Unicode code point must be
	 * mapped to 0xfffd "REPLACEMENT CHARACTER".
	 */
	_cairo_output_stream_printf (surface->output,
				     "fffd");
    } else {
	for (i = 0; i < utf16_len; i++)
	    _cairo_output_stream_printf (surface->output,
					 "%04x", (int) (utf16[i]));
    }
    _cairo_output_stream_printf (surface->output, ">");

    free (utf16);

    return CAIRO_STATUS_SUCCESS;
}

/* Bob Jenkins hash
 *
 * Public domain code from:
 *   http://burtleburtle.net/bob/hash/doobs.html
 */

#define HASH_MIX(a,b,c) 		\
{ 					\
    a -= b; a -= c; a ^= (c>>13);	\
    b -= c; b -= a; b ^= (a<<8);	\
    c -= a; c -= b; c ^= (b>>13);	\
    a -= b; a -= c; a ^= (c>>12);	\
    b -= c; b -= a; b ^= (a<<16);	\
    c -= a; c -= b; c ^= (b>>5);	\
    a -= b; a -= c; a ^= (c>>3);	\
    b -= c; b -= a; b ^= (a<<10);	\
    c -= a; c -= b; c ^= (b>>15);	\
}

static uint32_t
_hash_data (const unsigned char *data, int length, uint32_t initval)
{
    uint32_t a, b, c, len;

    len = length;
    a = b = 0x9e3779b9;  /* the golden ratio; an arbitrary value */
    c = initval;         /* the previous hash value */

    while (len >= 12) {
	a += (data[0] + ((uint32_t)data[1]<<8) + ((uint32_t)data[2]<<16) + ((uint32_t)data[3]<<24));
	b += (data[4] + ((uint32_t)data[5]<<8) + ((uint32_t)data[6]<<16) + ((uint32_t)data[7]<<24));
	c += (data[8] + ((uint32_t)data[9]<<8) + ((uint32_t)data[10]<<16)+ ((uint32_t)data[11]<<24));
	HASH_MIX (a,b,c);
	data += 12;
	len -= 12;
    }

    c += length;
    switch(len) {
    case 11: c+= ((uint32_t) data[10] << 24);
    case 10: c+= ((uint32_t) data[9] << 16);
    case 9 : c+= ((uint32_t) data[8] << 8);
    case 8 : b+= ((uint32_t) data[7] << 24);
    case 7 : b+= ((uint32_t) data[6] << 16);
    case 6 : b+= ((uint32_t) data[5] << 8);
    case 5 : b+= data[4];
    case 4 : a+= ((uint32_t) data[3] << 24);
    case 3 : a+= ((uint32_t) data[2] << 16);
    case 2 : a+= ((uint32_t) data[1] << 8);
    case 1 : a+= data[0];
    }
    HASH_MIX (a,b,c);

    return c;
}

static void
_create_font_subset_tag (cairo_scaled_font_subset_t	*font_subset,
			 const char 			*font_name,
			 char				*tag)
{
    uint32_t hash;
    int i;
    long numerator;
    ldiv_t d;

    hash = _hash_data ((unsigned char *) font_name, strlen(font_name), 0);
    hash = _hash_data ((unsigned char *) (font_subset->glyphs),
		       font_subset->num_glyphs * sizeof(unsigned long), hash);

    numerator = abs (hash);
    for (i = 0; i < 6; i++) {
	d = ldiv (numerator, 26);
	numerator = d.quot;
        tag[i] = 'A' + d.rem;
    }
    tag[i] = 0;
}

static cairo_int_status_t
_cairo_pdf_surface_emit_to_unicode_stream (cairo_pdf_surface_t		*surface,
					   cairo_scaled_font_subset_t	*font_subset,
					   cairo_pdf_resource_t         *stream)
{
    unsigned int i, num_bfchar;
    cairo_int_status_t status;

    stream->id = 0;

    status = _cairo_pdf_surface_open_stream (surface,
					      NULL,
					      surface->compress_content,
					      NULL);
    if (unlikely (status))
	return status;

    _cairo_output_stream_printf (surface->output,
                                 "/CIDInit /ProcSet findresource begin\n"
                                 "12 dict begin\n"
                                 "begincmap\n"
                                 "/CIDSystemInfo\n"
                                 "<< /Registry (Adobe)\n"
                                 "   /Ordering (UCS)\n"
                                 "   /Supplement 0\n"
                                 ">> def\n"
                                 "/CMapName /Adobe-Identity-UCS def\n"
                                 "/CMapType 2 def\n"
                                 "1 begincodespacerange\n");

    if (font_subset->is_composite && !font_subset->is_latin) {
        _cairo_output_stream_printf (surface->output,
                                     "<0000> <ffff>\n");
    } else {
        _cairo_output_stream_printf (surface->output,
                                     "<00> <ff>\n");
    }

    _cairo_output_stream_printf (surface->output,
                                  "endcodespacerange\n");

    if (font_subset->is_scaled) {
	/* Type 3 fonts include glyph 0 in the subset */
	num_bfchar = font_subset->num_glyphs;

	/* The CMap specification has a limit of 100 characters per beginbfchar operator */
	_cairo_output_stream_printf (surface->output,
				     "%d beginbfchar\n",
				     num_bfchar > 100 ? 100 : num_bfchar);

	for (i = 0; i < num_bfchar; i++) {
	    if (i != 0 && i % 100 == 0) {
		_cairo_output_stream_printf (surface->output,
					     "endbfchar\n"
					     "%d beginbfchar\n",
					     num_bfchar - i > 100 ? 100 : num_bfchar - i);
	    }
	    _cairo_output_stream_printf (surface->output, "<%02x> ", i);
	    status = _cairo_pdf_surface_emit_unicode_for_glyph (surface,
								font_subset->utf8[i]);
	    if (unlikely (status))
		return status;

	    _cairo_output_stream_printf (surface->output,
					 "\n");
	}
    } else {
	/* Other fonts reserve glyph 0 for .notdef. Omit glyph 0 from the /ToUnicode map */
	num_bfchar = font_subset->num_glyphs - 1;

	/* The CMap specification has a limit of 100 characters per beginbfchar operator */
	_cairo_output_stream_printf (surface->output,
				     "%d beginbfchar\n",
				     num_bfchar > 100 ? 100 : num_bfchar);

	for (i = 0; i < num_bfchar; i++) {
	    if (i != 0 && i % 100 == 0) {
		_cairo_output_stream_printf (surface->output,
					     "endbfchar\n"
					     "%d beginbfchar\n",
					     num_bfchar - i > 100 ? 100 : num_bfchar - i);
	    }
	    if (font_subset->is_latin)
		_cairo_output_stream_printf (surface->output, "<%02x> ", font_subset->to_latin_char[i + 1]);
	    else if (font_subset->is_composite)
		_cairo_output_stream_printf (surface->output, "<%04x> ", i + 1);
	    else
		_cairo_output_stream_printf (surface->output, "<%02x> ", i + 1);

	    status = _cairo_pdf_surface_emit_unicode_for_glyph (surface,
								font_subset->utf8[i + 1]);
	    if (unlikely (status))
		return status;

	    _cairo_output_stream_printf (surface->output,
					 "\n");
	}
    }

    _cairo_output_stream_printf (surface->output,
                                 "endbfchar\n");

    _cairo_output_stream_printf (surface->output,
                                 "endcmap\n"
                                 "CMapName currentdict /CMap defineresource pop\n"
                                 "end\n"
                                 "end\n");

    *stream = surface->pdf_stream.self;
    return _cairo_pdf_surface_close_stream (surface);
}

#define PDF_UNITS_PER_EM 1000

static cairo_status_t
_cairo_pdf_surface_emit_cff_font (cairo_pdf_surface_t		*surface,
                                  cairo_scaled_font_subset_t	*font_subset,
                                  cairo_cff_subset_t            *subset)
{
    cairo_pdf_resource_t stream, descriptor, cidfont_dict;
    cairo_pdf_resource_t subset_resource, to_unicode_stream;
    cairo_pdf_font_t font;
    unsigned int i, last_glyph;
    cairo_status_t status;
    char tag[10];

    _create_font_subset_tag (font_subset, subset->ps_name, tag);

    subset_resource = _cairo_pdf_surface_get_font_resource (surface,
							    font_subset->font_id,
							    font_subset->subset_id);
    if (subset_resource.id == 0)
	return CAIRO_STATUS_SUCCESS;

    status = _cairo_pdf_surface_open_stream (surface,
					     NULL,
					     TRUE,
					     font_subset->is_latin ?
					     "   /Subtype /Type1C\n" :
					     "   /Subtype /CIDFontType0C\n");
    if (unlikely (status))
	return status;

    stream = surface->pdf_stream.self;
    _cairo_output_stream_write (surface->output,
				subset->data, subset->data_length);
    status = _cairo_pdf_surface_close_stream (surface);
    if (unlikely (status))
	return status;

    status = _cairo_pdf_surface_emit_to_unicode_stream (surface,
	                                                font_subset,
							&to_unicode_stream);
    if (_cairo_status_is_error (status))
	return status;

    descriptor = _cairo_pdf_surface_new_object (surface);
    if (descriptor.id == 0)
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    _cairo_output_stream_printf (surface->output,
				 "%d 0 obj\n"
				 "<< /Type /FontDescriptor\n"
				 "   /FontName /%s+%s\n",
				 descriptor.id,
				 tag,
				 subset->ps_name);

    if (subset->family_name_utf8) {
	char *pdf_str;

	status = _utf8_to_pdf_string (subset->family_name_utf8, &pdf_str);
	if (unlikely (status))
	    return status;

	_cairo_output_stream_printf (surface->output,
				     "   /FontFamily %s\n",
				     pdf_str);
	free (pdf_str);
    }

    _cairo_output_stream_printf (surface->output,
				 "   /Flags 4\n"
				 "   /FontBBox [ %ld %ld %ld %ld ]\n"
				 "   /ItalicAngle 0\n"
				 "   /Ascent %ld\n"
				 "   /Descent %ld\n"
				 "   /CapHeight %ld\n"
				 "   /StemV 80\n"
				 "   /StemH 80\n"
				 "   /FontFile3 %u 0 R\n"
				 ">>\n"
				 "endobj\n",
				 (long)(subset->x_min*PDF_UNITS_PER_EM),
				 (long)(subset->y_min*PDF_UNITS_PER_EM),
				 (long)(subset->x_max*PDF_UNITS_PER_EM),
				 (long)(subset->y_max*PDF_UNITS_PER_EM),
				 (long)(subset->ascent*PDF_UNITS_PER_EM),
				 (long)(subset->descent*PDF_UNITS_PER_EM),
				 (long)(subset->y_max*PDF_UNITS_PER_EM),
				 stream.id);

    if (font_subset->is_latin) {
	/* find last glyph used */
	for (i = 255; i >= 32; i--)
	    if (font_subset->latin_to_subset_glyph_index[i] > 0)
		break;

	last_glyph = i;
	_cairo_pdf_surface_update_object (surface, subset_resource);
	_cairo_output_stream_printf (surface->output,
				     "%d 0 obj\n"
				     "<< /Type /Font\n"
				     "   /Subtype /Type1\n"
				     "   /BaseFont /%s+%s\n"
				     "   /FirstChar 32\n"
				     "   /LastChar %d\n"
				     "   /FontDescriptor %d 0 R\n"
				     "   /Encoding /WinAnsiEncoding\n"
				     "   /Widths [",
				     subset_resource.id,
				     tag,
				     subset->ps_name,
				     last_glyph,
				     descriptor.id);

	for (i = 32; i < last_glyph + 1; i++) {
	    int glyph = font_subset->latin_to_subset_glyph_index[i];
	    if (glyph > 0) {
		_cairo_output_stream_printf (surface->output,
					     " %ld",
					     (long)(subset->widths[glyph]*PDF_UNITS_PER_EM));
	    } else {
		_cairo_output_stream_printf (surface->output, " 0");
	    }
	}

	_cairo_output_stream_printf (surface->output,
				     " ]\n");

	if (to_unicode_stream.id != 0)
	    _cairo_output_stream_printf (surface->output,
					 "    /ToUnicode %d 0 R\n",
					 to_unicode_stream.id);

	_cairo_output_stream_printf (surface->output,
				     ">>\n"
				     "endobj\n");
    } else {
	cidfont_dict = _cairo_pdf_surface_new_object (surface);
	if (cidfont_dict.id == 0)
	    return _cairo_error (CAIRO_STATUS_NO_MEMORY);

	_cairo_output_stream_printf (surface->output,
				     "%d 0 obj\n"
				     "<< /Type /Font\n"
				     "   /Subtype /CIDFontType0\n"
				     "   /BaseFont /%s+%s\n"
				     "   /CIDSystemInfo\n"
				     "   << /Registry (Adobe)\n"
				     "      /Ordering (Identity)\n"
				     "      /Supplement 0\n"
				     "   >>\n"
				     "   /FontDescriptor %d 0 R\n"
				     "   /W [0 [",
				     cidfont_dict.id,
				     tag,
				     subset->ps_name,
				     descriptor.id);

	for (i = 0; i < font_subset->num_glyphs; i++)
	    _cairo_output_stream_printf (surface->output,
					 " %ld",
					 (long)(subset->widths[i]*PDF_UNITS_PER_EM));

	_cairo_output_stream_printf (surface->output,
				     " ]]\n"
				     ">>\n"
				     "endobj\n");

	_cairo_pdf_surface_update_object (surface, subset_resource);
	_cairo_output_stream_printf (surface->output,
				     "%d 0 obj\n"
				     "<< /Type /Font\n"
				     "   /Subtype /Type0\n"
				     "   /BaseFont /%s+%s\n"
				     "   /Encoding /Identity-H\n"
				     "   /DescendantFonts [ %d 0 R]\n",
				     subset_resource.id,
				     tag,
				     subset->ps_name,
				     cidfont_dict.id);

	if (to_unicode_stream.id != 0)
	    _cairo_output_stream_printf (surface->output,
					 "   /ToUnicode %d 0 R\n",
					 to_unicode_stream.id);

	_cairo_output_stream_printf (surface->output,
				     ">>\n"
				     "endobj\n");
    }

    font.font_id = font_subset->font_id;
    font.subset_id = font_subset->subset_id;
    font.subset_resource = subset_resource;
    status = _cairo_array_append (&surface->fonts, &font);

    return status;
}

static cairo_status_t
_cairo_pdf_surface_emit_cff_font_subset (cairo_pdf_surface_t	     *surface,
                                         cairo_scaled_font_subset_t  *font_subset)
{
    cairo_status_t status;
    cairo_cff_subset_t subset;
    char name[64];

    snprintf (name, sizeof name, "CairoFont-%d-%d",
              font_subset->font_id, font_subset->subset_id);
    status = _cairo_cff_subset_init (&subset, name, font_subset);
    if (unlikely (status))
        return status;

    status = _cairo_pdf_surface_emit_cff_font (surface, font_subset, &subset);

    _cairo_cff_subset_fini (&subset);

    return status;
}

static cairo_status_t
_cairo_pdf_surface_emit_cff_fallback_font (cairo_pdf_surface_t	       *surface,
                                           cairo_scaled_font_subset_t  *font_subset)
{
    cairo_status_t status;
    cairo_cff_subset_t subset;
    char name[64];

    /* CFF fallback subsetting does not work with 8-bit glyphs unless
     * they are a latin subset */
    if (!font_subset->is_composite && !font_subset->is_latin)
	return CAIRO_INT_STATUS_UNSUPPORTED;

    snprintf (name, sizeof name, "CairoFont-%d-%d",
              font_subset->font_id, font_subset->subset_id);
    status = _cairo_cff_fallback_init (&subset, name, font_subset);
    if (unlikely (status))
        return status;

    status = _cairo_pdf_surface_emit_cff_font (surface, font_subset, &subset);

    _cairo_cff_fallback_fini (&subset);

    return status;
}

static cairo_status_t
_cairo_pdf_surface_emit_type1_font (cairo_pdf_surface_t		*surface,
                                    cairo_scaled_font_subset_t	*font_subset,
                                    cairo_type1_subset_t        *subset)
{
    cairo_pdf_resource_t stream, descriptor, subset_resource, to_unicode_stream;
    cairo_pdf_font_t font;
    cairo_status_t status;
    unsigned long length;
    unsigned int i, last_glyph;
    char tag[10];

    _create_font_subset_tag (font_subset, subset->base_font, tag);

    subset_resource = _cairo_pdf_surface_get_font_resource (surface,
							    font_subset->font_id,
							    font_subset->subset_id);
    if (subset_resource.id == 0)
	return CAIRO_STATUS_SUCCESS;

    length = subset->header_length + subset->data_length + subset->trailer_length;
    status = _cairo_pdf_surface_open_stream (surface,
					     NULL,
					     TRUE,
					     "   /Length1 %lu\n"
					     "   /Length2 %lu\n"
					     "   /Length3 %lu\n",
					     subset->header_length,
					     subset->data_length,
					     subset->trailer_length);
    if (unlikely (status))
	return status;

    stream = surface->pdf_stream.self;
    _cairo_output_stream_write (surface->output, subset->data, length);
    status = _cairo_pdf_surface_close_stream (surface);
    if (unlikely (status))
	return status;

    status = _cairo_pdf_surface_emit_to_unicode_stream (surface,
	                                                font_subset,
							&to_unicode_stream);
    if (_cairo_status_is_error (status))
	return status;

    last_glyph = font_subset->num_glyphs - 1;
    if (font_subset->is_latin) {
	/* find last glyph used */
	for (i = 255; i >= 32; i--)
	    if (font_subset->latin_to_subset_glyph_index[i] > 0)
		break;

	last_glyph = i;
    }

    descriptor = _cairo_pdf_surface_new_object (surface);
    if (descriptor.id == 0)
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    _cairo_output_stream_printf (surface->output,
				 "%d 0 obj\n"
				 "<< /Type /FontDescriptor\n"
				 "   /FontName /%s+%s\n"
				 "   /Flags 4\n"
				 "   /FontBBox [ %ld %ld %ld %ld ]\n"
				 "   /ItalicAngle 0\n"
				 "   /Ascent %ld\n"
				 "   /Descent %ld\n"
				 "   /CapHeight %ld\n"
				 "   /StemV 80\n"
				 "   /StemH 80\n"
				 "   /FontFile %u 0 R\n"
				 ">>\n"
				 "endobj\n",
				 descriptor.id,
				 tag,
				 subset->base_font,
				 (long)(subset->x_min*PDF_UNITS_PER_EM),
				 (long)(subset->y_min*PDF_UNITS_PER_EM),
				 (long)(subset->x_max*PDF_UNITS_PER_EM),
				 (long)(subset->y_max*PDF_UNITS_PER_EM),
				 (long)(subset->ascent*PDF_UNITS_PER_EM),
				 (long)(subset->descent*PDF_UNITS_PER_EM),
				 (long)(subset->y_max*PDF_UNITS_PER_EM),
				 stream.id);

    _cairo_pdf_surface_update_object (surface, subset_resource);
    _cairo_output_stream_printf (surface->output,
				 "%d 0 obj\n"
				 "<< /Type /Font\n"
				 "   /Subtype /Type1\n"
				 "   /BaseFont /%s+%s\n"
				 "   /FirstChar %d\n"
				 "   /LastChar %d\n"
				 "   /FontDescriptor %d 0 R\n",
				 subset_resource.id,
				 tag,
				 subset->base_font,
				 font_subset->is_latin ? 32 : 0,
				 last_glyph,
				 descriptor.id);

    if (font_subset->is_latin)
	_cairo_output_stream_printf (surface->output, "   /Encoding /WinAnsiEncoding\n");

    _cairo_output_stream_printf (surface->output, "   /Widths [");
    if (font_subset->is_latin) {
	for (i = 32; i < last_glyph + 1; i++) {
	    int glyph = font_subset->latin_to_subset_glyph_index[i];
	    if (glyph > 0) {
		_cairo_output_stream_printf (surface->output,
					     " %ld",
					     (long)(subset->widths[glyph]*PDF_UNITS_PER_EM));
	    } else {
		_cairo_output_stream_printf (surface->output, " 0");
	    }
	}
    } else {
	for (i = 0; i < font_subset->num_glyphs; i++)
	    _cairo_output_stream_printf (surface->output,
					 " %ld",
					 (long)(subset->widths[i]*PDF_UNITS_PER_EM));
    }

    _cairo_output_stream_printf (surface->output,
				 " ]\n");

    if (to_unicode_stream.id != 0)
        _cairo_output_stream_printf (surface->output,
                                     "    /ToUnicode %d 0 R\n",
                                     to_unicode_stream.id);

    _cairo_output_stream_printf (surface->output,
				 ">>\n"
				 "endobj\n");

    font.font_id = font_subset->font_id;
    font.subset_id = font_subset->subset_id;
    font.subset_resource = subset_resource;
    return _cairo_array_append (&surface->fonts, &font);
}

static cairo_status_t
_cairo_pdf_surface_emit_type1_font_subset (cairo_pdf_surface_t		*surface,
					   cairo_scaled_font_subset_t	*font_subset)
{
    cairo_status_t status;
    cairo_type1_subset_t subset;
    char name[64];

    /* 16-bit glyphs not compatible with Type 1 fonts */
    if (font_subset->is_composite && !font_subset->is_latin)
	return CAIRO_INT_STATUS_UNSUPPORTED;

    snprintf (name, sizeof name, "CairoFont-%d-%d",
	      font_subset->font_id, font_subset->subset_id);
    status = _cairo_type1_subset_init (&subset, name, font_subset, FALSE);
    if (unlikely (status))
	return status;

    status = _cairo_pdf_surface_emit_type1_font (surface, font_subset, &subset);

    _cairo_type1_subset_fini (&subset);
    return status;
}

static cairo_status_t
_cairo_pdf_surface_emit_type1_fallback_font (cairo_pdf_surface_t	*surface,
                                             cairo_scaled_font_subset_t	*font_subset)
{
    cairo_status_t status;
    cairo_type1_subset_t subset;
    char name[64];

    /* 16-bit glyphs not compatible with Type 1 fonts */
    if (font_subset->is_composite && !font_subset->is_latin)
	return CAIRO_INT_STATUS_UNSUPPORTED;

    snprintf (name, sizeof name, "CairoFont-%d-%d",
	      font_subset->font_id, font_subset->subset_id);
    status = _cairo_type1_fallback_init_binary (&subset, name, font_subset);
    if (unlikely (status))
	return status;

    status = _cairo_pdf_surface_emit_type1_font (surface, font_subset, &subset);

    _cairo_type1_fallback_fini (&subset);
    return status;
}

static cairo_status_t
_cairo_pdf_surface_emit_truetype_font_subset (cairo_pdf_surface_t		*surface,
					      cairo_scaled_font_subset_t	*font_subset)
{
    cairo_pdf_resource_t stream, descriptor, cidfont_dict;
    cairo_pdf_resource_t subset_resource, to_unicode_stream;
    cairo_status_t status;
    cairo_pdf_font_t font;
    cairo_truetype_subset_t subset;
    unsigned int i, last_glyph;
    char tag[10];

    subset_resource = _cairo_pdf_surface_get_font_resource (surface,
							    font_subset->font_id,
							    font_subset->subset_id);
    if (subset_resource.id == 0)
	return CAIRO_STATUS_SUCCESS;

    status = _cairo_truetype_subset_init_pdf (&subset, font_subset);
    if (unlikely (status))
	return status;

    _create_font_subset_tag (font_subset, subset.ps_name, tag);

    status = _cairo_pdf_surface_open_stream (surface,
					     NULL,
					     TRUE,
					     "   /Length1 %lu\n",
					     subset.data_length);
    if (unlikely (status)) {
	_cairo_truetype_subset_fini (&subset);
	return status;
    }

    stream = surface->pdf_stream.self;
    _cairo_output_stream_write (surface->output,
				subset.data, subset.data_length);
    status = _cairo_pdf_surface_close_stream (surface);
    if (unlikely (status)) {
	_cairo_truetype_subset_fini (&subset);
	return status;
    }

    status = _cairo_pdf_surface_emit_to_unicode_stream (surface,
	                                                font_subset,
							&to_unicode_stream);
    if (_cairo_status_is_error (status)) {
	_cairo_truetype_subset_fini (&subset);
	return status;
    }

    descriptor = _cairo_pdf_surface_new_object (surface);
    if (descriptor.id == 0) {
	_cairo_truetype_subset_fini (&subset);
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);
    }

    _cairo_output_stream_printf (surface->output,
				 "%d 0 obj\n"
				 "<< /Type /FontDescriptor\n"
				 "   /FontName /%s+%s\n",
				 descriptor.id,
				 tag,
				 subset.ps_name);

    if (subset.family_name_utf8) {
	char *pdf_str;

	status = _utf8_to_pdf_string (subset.family_name_utf8, &pdf_str);
	if (unlikely (status))
	    return status;

	_cairo_output_stream_printf (surface->output,
				     "   /FontFamily %s\n",
				     pdf_str);
	free (pdf_str);
    }

    _cairo_output_stream_printf (surface->output,
				 "   /Flags %d\n"
				 "   /FontBBox [ %ld %ld %ld %ld ]\n"
				 "   /ItalicAngle 0\n"
				 "   /Ascent %ld\n"
				 "   /Descent %ld\n"
				 "   /CapHeight %ld\n"
				 "   /StemV 80\n"
				 "   /StemH 80\n"
				 "   /FontFile2 %u 0 R\n"
				 ">>\n"
				 "endobj\n",
				 font_subset->is_latin ? 32 : 4,
				 (long)(subset.x_min*PDF_UNITS_PER_EM),
				 (long)(subset.y_min*PDF_UNITS_PER_EM),
                                 (long)(subset.x_max*PDF_UNITS_PER_EM),
				 (long)(subset.y_max*PDF_UNITS_PER_EM),
				 (long)(subset.ascent*PDF_UNITS_PER_EM),
				 (long)(subset.descent*PDF_UNITS_PER_EM),
				 (long)(subset.y_max*PDF_UNITS_PER_EM),
				 stream.id);

    if (font_subset->is_latin) {
	/* find last glyph used */
	for (i = 255; i >= 32; i--)
	    if (font_subset->latin_to_subset_glyph_index[i] > 0)
		break;

	last_glyph = i;
	_cairo_pdf_surface_update_object (surface, subset_resource);
	_cairo_output_stream_printf (surface->output,
				     "%d 0 obj\n"
				     "<< /Type /Font\n"
				     "   /Subtype /TrueType\n"
				     "   /BaseFont /%s+%s\n"
				     "   /FirstChar 32\n"
				     "   /LastChar %d\n"
				     "   /FontDescriptor %d 0 R\n"
				     "   /Encoding /WinAnsiEncoding\n"
				     "   /Widths [",
				     subset_resource.id,
				     tag,
				     subset.ps_name,
				     last_glyph,
				     descriptor.id);

	for (i = 32; i < last_glyph + 1; i++) {
	    int glyph = font_subset->latin_to_subset_glyph_index[i];
	    if (glyph > 0) {
		_cairo_output_stream_printf (surface->output,
					     " %ld",
					     (long)(subset.widths[glyph]*PDF_UNITS_PER_EM));
	    } else {
		_cairo_output_stream_printf (surface->output, " 0");
	    }
	}

	_cairo_output_stream_printf (surface->output,
				     " ]\n");

	if (to_unicode_stream.id != 0)
	    _cairo_output_stream_printf (surface->output,
					 "    /ToUnicode %d 0 R\n",
					 to_unicode_stream.id);

	_cairo_output_stream_printf (surface->output,
				     ">>\n"
				     "endobj\n");
    } else {
	cidfont_dict = _cairo_pdf_surface_new_object (surface);
	if (cidfont_dict.id == 0) {
	    _cairo_truetype_subset_fini (&subset);
	    return _cairo_error (CAIRO_STATUS_NO_MEMORY);
	}

	_cairo_output_stream_printf (surface->output,
				     "%d 0 obj\n"
				     "<< /Type /Font\n"
				     "   /Subtype /CIDFontType2\n"
				     "   /BaseFont /%s+%s\n"
				     "   /CIDSystemInfo\n"
				     "   << /Registry (Adobe)\n"
				     "      /Ordering (Identity)\n"
				     "      /Supplement 0\n"
				     "   >>\n"
				     "   /FontDescriptor %d 0 R\n"
				     "   /W [0 [",
				     cidfont_dict.id,
				     tag,
				     subset.ps_name,
				     descriptor.id);

	for (i = 0; i < font_subset->num_glyphs; i++)
	    _cairo_output_stream_printf (surface->output,
					 " %ld",
					 (long)(subset.widths[i]*PDF_UNITS_PER_EM));

	_cairo_output_stream_printf (surface->output,
				     " ]]\n"
				     ">>\n"
				     "endobj\n");

	_cairo_pdf_surface_update_object (surface, subset_resource);
	_cairo_output_stream_printf (surface->output,
				     "%d 0 obj\n"
				     "<< /Type /Font\n"
				     "   /Subtype /Type0\n"
				     "   /BaseFont /%s+%s\n"
				     "   /Encoding /Identity-H\n"
				     "   /DescendantFonts [ %d 0 R]\n",
				     subset_resource.id,
				     tag,
				     subset.ps_name,
				     cidfont_dict.id);

	if (to_unicode_stream.id != 0)
	    _cairo_output_stream_printf (surface->output,
					 "   /ToUnicode %d 0 R\n",
					 to_unicode_stream.id);

	_cairo_output_stream_printf (surface->output,
				     ">>\n"
				     "endobj\n");
    }

    font.font_id = font_subset->font_id;
    font.subset_id = font_subset->subset_id;
    font.subset_resource = subset_resource;
    status = _cairo_array_append (&surface->fonts, &font);

    _cairo_truetype_subset_fini (&subset);

    return status;
}

static cairo_status_t
_cairo_pdf_emit_imagemask (cairo_image_surface_t *image,
			     cairo_output_stream_t *stream)
{
    uint8_t *byte, output_byte;
    int row, col, num_cols;

    /* The only image type supported by Type 3 fonts are 1-bit image
     * masks */
    assert (image->format == CAIRO_FORMAT_A1);

    _cairo_output_stream_printf (stream,
				 "BI\n"
				 "/IM true\n"
				 "/W %d\n"
				 "/H %d\n"
				 "/BPC 1\n"
				 "/D [1 0]\n",
				 image->width,
				 image->height);

    _cairo_output_stream_printf (stream,
				 "ID ");

    num_cols = (image->width + 7) / 8;
    for (row = 0; row < image->height; row++) {
	byte = image->data + row * image->stride;
	for (col = 0; col < num_cols; col++) {
	    output_byte = CAIRO_BITSWAP8_IF_LITTLE_ENDIAN (*byte);
	    _cairo_output_stream_write (stream, &output_byte, 1);
	    byte++;
	}
    }

    _cairo_output_stream_printf (stream,
				 "\nEI\n");

    return _cairo_output_stream_get_status (stream);
}

static cairo_int_status_t
_cairo_pdf_surface_analyze_user_font_subset (cairo_scaled_font_subset_t *font_subset,
					     void		        *closure)
{
    cairo_pdf_surface_t *surface = closure;
    cairo_status_t status = CAIRO_STATUS_SUCCESS;
    cairo_status_t status2;
    unsigned int i;
    cairo_surface_t *type3_surface;
    cairo_output_stream_t *null_stream;

    null_stream = _cairo_null_stream_create ();
    type3_surface = _cairo_type3_glyph_surface_create (font_subset->scaled_font,
						       null_stream,
						       _cairo_pdf_emit_imagemask,
						       surface->font_subsets);
    if (unlikely (type3_surface->status)) {
	status2 = _cairo_output_stream_destroy (null_stream);
	return type3_surface->status;
    }

    _cairo_type3_glyph_surface_set_font_subsets_callback (type3_surface,
							  _cairo_pdf_surface_add_font,
							  surface);

    for (i = 0; i < font_subset->num_glyphs; i++) {
	status = _cairo_type3_glyph_surface_analyze_glyph (type3_surface,
							   font_subset->glyphs[i]);
	if (unlikely (status))
	    break;
    }

    cairo_surface_destroy (type3_surface);
    status2 = _cairo_output_stream_destroy (null_stream);
    if (status == CAIRO_STATUS_SUCCESS)
	status = status2;

    return status;
}

static cairo_int_status_t
_cairo_pdf_surface_emit_type3_font_subset (cairo_pdf_surface_t		*surface,
					   cairo_scaled_font_subset_t	*font_subset)
{
    cairo_status_t status = CAIRO_STATUS_SUCCESS;
    cairo_pdf_resource_t *glyphs, encoding, char_procs, subset_resource, to_unicode_stream;
    cairo_pdf_font_t font;
    double *widths;
    unsigned int i;
    cairo_box_t font_bbox = {{0,0},{0,0}};
    cairo_box_t bbox = {{0,0},{0,0}};
    cairo_surface_t *type3_surface;

    if (font_subset->num_glyphs == 0)
	return CAIRO_STATUS_SUCCESS;

    subset_resource = _cairo_pdf_surface_get_font_resource (surface,
							    font_subset->font_id,
							    font_subset->subset_id);
    if (subset_resource.id == 0)
	return CAIRO_STATUS_SUCCESS;

    glyphs = _cairo_malloc_ab (font_subset->num_glyphs, sizeof (cairo_pdf_resource_t));
    if (unlikely (glyphs == NULL))
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    widths = _cairo_malloc_ab (font_subset->num_glyphs, sizeof (double));
    if (unlikely (widths == NULL)) {
        free (glyphs);
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);
    }

    _cairo_pdf_group_resources_clear (&surface->resources);
    type3_surface = _cairo_type3_glyph_surface_create (font_subset->scaled_font,
						       NULL,
						       _cairo_pdf_emit_imagemask,
						       surface->font_subsets);
    if (unlikely (type3_surface->status)) {
        free (glyphs);
        free (widths);
	return type3_surface->status;
    }

    _cairo_type3_glyph_surface_set_font_subsets_callback (type3_surface,
							  _cairo_pdf_surface_add_font,
							  surface);

    for (i = 0; i < font_subset->num_glyphs; i++) {
	status = _cairo_pdf_surface_open_stream (surface,
						 NULL,
						 surface->compress_content,
						 NULL);
	if (unlikely (status))
	    break;

	glyphs[i] = surface->pdf_stream.self;
	status = _cairo_type3_glyph_surface_emit_glyph (type3_surface,
							surface->output,
							font_subset->glyphs[i],
							&bbox,
							&widths[i]);
	if (unlikely (status))
	    break;

	status = _cairo_pdf_surface_close_stream (surface);
	if (unlikely (status))
	    break;

        if (i == 0) {
            font_bbox.p1.x = bbox.p1.x;
            font_bbox.p1.y = bbox.p1.y;
            font_bbox.p2.x = bbox.p2.x;
            font_bbox.p2.y = bbox.p2.y;
        } else {
            if (bbox.p1.x < font_bbox.p1.x)
                font_bbox.p1.x = bbox.p1.x;
            if (bbox.p1.y < font_bbox.p1.y)
                font_bbox.p1.y = bbox.p1.y;
            if (bbox.p2.x > font_bbox.p2.x)
                font_bbox.p2.x = bbox.p2.x;
            if (bbox.p2.y > font_bbox.p2.y)
                font_bbox.p2.y = bbox.p2.y;
        }
    }
    cairo_surface_destroy (type3_surface);
    if (unlikely (status)) {
	free (glyphs);
	free (widths);
	return status;
    }

    encoding = _cairo_pdf_surface_new_object (surface);
    if (encoding.id == 0) {
	free (glyphs);
	free (widths);
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);
    }

    _cairo_output_stream_printf (surface->output,
				 "%d 0 obj\n"
				 "<< /Type /Encoding\n"
				 "   /Differences [0", encoding.id);
    for (i = 0; i < font_subset->num_glyphs; i++)
	_cairo_output_stream_printf (surface->output,
				     " /%d", i);
    _cairo_output_stream_printf (surface->output,
				 "]\n"
				 ">>\n"
				 "endobj\n");

    char_procs = _cairo_pdf_surface_new_object (surface);
    if (char_procs.id == 0) {
	free (glyphs);
	free (widths);
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);
    }

    _cairo_output_stream_printf (surface->output,
				 "%d 0 obj\n"
				 "<<\n", char_procs.id);
    for (i = 0; i < font_subset->num_glyphs; i++)
	_cairo_output_stream_printf (surface->output,
				     " /%d %d 0 R\n",
				     i, glyphs[i].id);
    _cairo_output_stream_printf (surface->output,
				 ">>\n"
				 "endobj\n");

    free (glyphs);

    status = _cairo_pdf_surface_emit_to_unicode_stream (surface,
	                                                font_subset,
							&to_unicode_stream);
    if (_cairo_status_is_error (status)) {
	free (widths);
	return status;
    }

    _cairo_pdf_surface_update_object (surface, subset_resource);
    _cairo_output_stream_printf (surface->output,
				 "%d 0 obj\n"
				 "<< /Type /Font\n"
				 "   /Subtype /Type3\n"
				 "   /FontBBox [%f %f %f %f]\n"
				 "   /FontMatrix [ 1 0 0 1 0 0 ]\n"
				 "   /Encoding %d 0 R\n"
				 "   /CharProcs %d 0 R\n"
				 "   /FirstChar 0\n"
				 "   /LastChar %d\n",
				 subset_resource.id,
				 _cairo_fixed_to_double (font_bbox.p1.x),
				 - _cairo_fixed_to_double (font_bbox.p2.y),
				 _cairo_fixed_to_double (font_bbox.p2.x),
				 - _cairo_fixed_to_double (font_bbox.p1.y),
				 encoding.id,
				 char_procs.id,
				 font_subset->num_glyphs - 1);

    _cairo_output_stream_printf (surface->output,
				 "   /Widths [");
    for (i = 0; i < font_subset->num_glyphs; i++)
	_cairo_output_stream_printf (surface->output, " %f", widths[i]);
    _cairo_output_stream_printf (surface->output,
				 "]\n");
    free (widths);

    _cairo_output_stream_printf (surface->output,
				 "   /Resources\n");
    _cairo_pdf_surface_emit_group_resources (surface, &surface->resources);

    if (to_unicode_stream.id != 0)
        _cairo_output_stream_printf (surface->output,
                                     "    /ToUnicode %d 0 R\n",
                                     to_unicode_stream.id);

    _cairo_output_stream_printf (surface->output,
				 ">>\n"
				 "endobj\n");

    font.font_id = font_subset->font_id;
    font.subset_id = font_subset->subset_id;
    font.subset_resource = subset_resource;
    return _cairo_array_append (&surface->fonts, &font);
}

static cairo_int_status_t
_cairo_pdf_surface_emit_unscaled_font_subset (cairo_scaled_font_subset_t *font_subset,
                                              void			 *closure)
{
    cairo_pdf_surface_t *surface = closure;
    cairo_int_status_t status;

    status = _cairo_pdf_surface_emit_cff_font_subset (surface, font_subset);
    if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	return status;

    status = _cairo_pdf_surface_emit_truetype_font_subset (surface, font_subset);
    if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	return status;

    status = _cairo_pdf_surface_emit_type1_font_subset (surface, font_subset);
    if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	return status;

    status = _cairo_pdf_surface_emit_cff_fallback_font (surface, font_subset);
    if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	return status;

    status = _cairo_pdf_surface_emit_type1_fallback_font (surface, font_subset);
    if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	return status;

    ASSERT_NOT_REACHED;
    return CAIRO_INT_STATUS_SUCCESS;
}

static cairo_int_status_t
_cairo_pdf_surface_emit_scaled_font_subset (cairo_scaled_font_subset_t *font_subset,
                                            void		       *closure)
{
    cairo_pdf_surface_t *surface = closure;
    cairo_int_status_t status;

    status = _cairo_pdf_surface_emit_type3_font_subset (surface, font_subset);
    if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	return status;

    ASSERT_NOT_REACHED;
    return CAIRO_INT_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_pdf_surface_emit_font_subsets (cairo_pdf_surface_t *surface)
{
    cairo_status_t status;

    status = _cairo_scaled_font_subsets_foreach_user (surface->font_subsets,
						      _cairo_pdf_surface_analyze_user_font_subset,
						      surface);
    if (unlikely (status))
	goto BAIL;

    status = _cairo_scaled_font_subsets_foreach_unscaled (surface->font_subsets,
                                                          _cairo_pdf_surface_emit_unscaled_font_subset,
                                                          surface);
    if (unlikely (status))
	goto BAIL;

    status = _cairo_scaled_font_subsets_foreach_scaled (surface->font_subsets,
                                                        _cairo_pdf_surface_emit_scaled_font_subset,
                                                        surface);
    if (unlikely (status))
	goto BAIL;

    status = _cairo_scaled_font_subsets_foreach_user (surface->font_subsets,
						      _cairo_pdf_surface_emit_scaled_font_subset,
						      surface);

BAIL:
    _cairo_scaled_font_subsets_destroy (surface->font_subsets);
    surface->font_subsets = NULL;

    return status;
}

static cairo_pdf_resource_t
_cairo_pdf_surface_write_catalog (cairo_pdf_surface_t *surface)
{
    cairo_pdf_resource_t catalog;

    catalog = _cairo_pdf_surface_new_object (surface);
    if (catalog.id == 0)
	return catalog;

    _cairo_output_stream_printf (surface->output,
				 "%d 0 obj\n"
				 "<< /Type /Catalog\n"
				 "   /Pages %d 0 R\n"
				 ">>\n"
				 "endobj\n",
				 catalog.id,
				 surface->pages_resource.id);

    return catalog;
}

static long
_cairo_pdf_surface_write_xref (cairo_pdf_surface_t *surface)
{
    cairo_pdf_object_t *object;
    int num_objects, i;
    long offset;
    char buffer[11];

    num_objects = _cairo_array_num_elements (&surface->objects);

    offset = _cairo_output_stream_get_position (surface->output);
    _cairo_output_stream_printf (surface->output,
				 "xref\n"
				 "%d %d\n",
				 0, num_objects + 1);

    _cairo_output_stream_printf (surface->output,
				 "0000000000 65535 f \n");
    for (i = 0; i < num_objects; i++) {
	object = _cairo_array_index (&surface->objects, i);
	snprintf (buffer, sizeof buffer, "%010ld", object->offset);
	_cairo_output_stream_printf (surface->output,
				     "%s 00000 n \n", buffer);
    }

    return offset;
}

static cairo_status_t
_cairo_pdf_surface_write_mask_group (cairo_pdf_surface_t	*surface,
				     cairo_pdf_smask_group_t	*group)
{
    cairo_pdf_resource_t mask_group;
    cairo_pdf_resource_t smask;
    cairo_pdf_smask_group_t *smask_group;
    cairo_pdf_resource_t pattern_res, gstate_res;
    cairo_status_t status;
    cairo_box_double_t bbox;

    /* Create mask group */
    _get_bbox_from_extents (group->height, &group->extents, &bbox);
    status = _cairo_pdf_surface_open_group (surface, &bbox, NULL);
    if (unlikely (status))
	return status;

    if (_can_paint_pattern (group->mask)) {
	_cairo_output_stream_printf (surface->output, "q\n");
	status = _cairo_pdf_surface_paint_pattern (surface,
						   group->mask,
						   &group->extents,
						   FALSE);
	if (unlikely (status))
	    return status;

	_cairo_output_stream_printf (surface->output, "Q\n");
    } else {
	pattern_res.id = 0;
	gstate_res.id = 0;
	status = _cairo_pdf_surface_add_pdf_pattern (surface, group->mask, NULL,
						     &pattern_res, &gstate_res);
	if (unlikely (status))
	    return status;

	if (gstate_res.id != 0) {
	    smask_group = _cairo_pdf_surface_create_smask_group (surface, &group->extents);
	    if (unlikely (smask_group == NULL))
		return _cairo_error (CAIRO_STATUS_NO_MEMORY);

	    smask_group->width = group->width;
	    smask_group->height = group->height;
	    smask_group->operation = PDF_PAINT;
	    smask_group->source = cairo_pattern_reference (group->mask);
	    smask_group->source_res = pattern_res;
	    status = _cairo_pdf_surface_add_smask_group (surface, smask_group);
	    if (unlikely (status)) {
		_cairo_pdf_smask_group_destroy (smask_group);
		return status;
	    }

	    status = _cairo_pdf_surface_add_smask (surface, gstate_res);
	    if (unlikely (status))
		return status;

	    status = _cairo_pdf_surface_add_xobject (surface, smask_group->group_res);
	    if (unlikely (status))
		return status;

	    _cairo_output_stream_printf (surface->output,
					 "q /s%d gs /x%d Do Q\n",
					 gstate_res.id,
					 smask_group->group_res.id);
	} else {
	    status = _cairo_pdf_surface_select_pattern (surface, group->mask, pattern_res, FALSE);
	    if (unlikely (status))
		return status;

	    _cairo_output_stream_printf (surface->output,
					 "%f %f %f %f re f\n",
					 bbox.p1.x,
					 bbox.p1.y,
					 bbox.p2.x - bbox.p1.x,
					 bbox.p2.y - bbox.p1.y);

	    status = _cairo_pdf_surface_unselect_pattern (surface);
	    if (unlikely (status))
		return status;
	}
    }

    status = _cairo_pdf_surface_close_group (surface, &mask_group);
    if (unlikely (status))
	return status;

    /* Create source group */
    status = _cairo_pdf_surface_open_group (surface, &bbox, &group->source_res);
    if (unlikely (status))
	return status;

    if (_can_paint_pattern (group->source)) {
	_cairo_output_stream_printf (surface->output, "q\n");
	status = _cairo_pdf_surface_paint_pattern (surface,
						   group->source,
						   &group->extents,
						   FALSE);
	if (unlikely (status))
	    return status;

	_cairo_output_stream_printf (surface->output, "Q\n");
    } else {
	pattern_res.id = 0;
	gstate_res.id = 0;
	status = _cairo_pdf_surface_add_pdf_pattern (surface, group->source, NULL,
						     &pattern_res, &gstate_res);
	if (unlikely (status))
	    return status;

	if (gstate_res.id != 0) {
	    smask_group = _cairo_pdf_surface_create_smask_group (surface, &group->extents);
	    if (unlikely (smask_group == NULL))
		return _cairo_error (CAIRO_STATUS_NO_MEMORY);

	    smask_group->operation = PDF_PAINT;
	    smask_group->source = cairo_pattern_reference (group->source);
	    smask_group->source_res = pattern_res;
	    status = _cairo_pdf_surface_add_smask_group (surface, smask_group);
	    if (unlikely (status)) {
		_cairo_pdf_smask_group_destroy (smask_group);
		return status;
	    }

	    status = _cairo_pdf_surface_add_smask (surface, gstate_res);
	    if (unlikely (status))
		return status;

	    status = _cairo_pdf_surface_add_xobject (surface, smask_group->group_res);
	    if (unlikely (status))
		return status;

	    _cairo_output_stream_printf (surface->output,
					 "q /s%d gs /x%d Do Q\n",
					 gstate_res.id,
					 smask_group->group_res.id);
	} else {
	    status = _cairo_pdf_surface_select_pattern (surface, group->source, pattern_res, FALSE);
	    if (unlikely (status))
		return status;

	    _cairo_output_stream_printf (surface->output,
					 "%f %f %f %f re f\n",
					 bbox.p1.x,
					 bbox.p1.y,
					 bbox.p2.x - bbox.p1.x,
					 bbox.p2.y - bbox.p1.y);

	    status = _cairo_pdf_surface_unselect_pattern (surface);
	    if (unlikely (status))
		return status;
	}
    }

    status = _cairo_pdf_surface_close_group (surface, NULL);
    if (unlikely (status))
	return status;

    /* Create an smask based on the alpha component of mask_group */
    smask = _cairo_pdf_surface_new_object (surface);
    if (smask.id == 0)
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    _cairo_output_stream_printf (surface->output,
				 "%d 0 obj\n"
				 "<< /Type /Mask\n"
				 "   /S /Alpha\n"
				 "   /G %d 0 R\n"
				 ">>\n"
				 "endobj\n",
				 smask.id,
				 mask_group.id);

    /* Create a GState that uses the smask */
    _cairo_pdf_surface_update_object (surface, group->group_res);
    _cairo_output_stream_printf (surface->output,
				 "%d 0 obj\n"
				 "<< /Type /ExtGState\n"
				 "   /SMask %d 0 R\n"
				 "   /ca 1\n"
				 "   /CA 1\n"
				 "   /AIS false\n"
				 ">>\n"
				 "endobj\n",
				 group->group_res.id,
				 smask.id);

    return _cairo_output_stream_get_status (surface->output);
}

static cairo_status_t
_cairo_pdf_surface_write_smask_group (cairo_pdf_surface_t     *surface,
				      cairo_pdf_smask_group_t *group)
{
    double old_width, old_height;
    cairo_status_t status;
    cairo_box_double_t bbox;

    old_width = surface->width;
    old_height = surface->height;
    _cairo_pdf_surface_set_size_internal (surface,
					  group->width,
					  group->height);
    /* _mask is a special case that requires two groups - source
     * and mask as well as a smask and gstate dictionary */
    if (group->operation == PDF_MASK) {
	status = _cairo_pdf_surface_write_mask_group (surface, group);
	goto RESTORE_SIZE;
    }

    _get_bbox_from_extents (group->height, &group->extents, &bbox);
    status = _cairo_pdf_surface_open_group (surface, &bbox, &group->group_res);
    if (unlikely (status))
	return status;

    status = _cairo_pdf_surface_select_pattern (surface,
						group->source,
						group->source_res,
						group->operation == PDF_STROKE);
    if (unlikely (status))
	return status;

    switch (group->operation) {
    case PDF_PAINT:
	_cairo_output_stream_printf (surface->output,
				     "0 0 %f %f re f\n",
				     surface->width, surface->height);
	break;
    case PDF_MASK:
	ASSERT_NOT_REACHED;
	break;
    case PDF_FILL:
	status = _cairo_pdf_operators_fill (&surface->pdf_operators,
					    &group->path,
					    group->fill_rule);
	break;
    case PDF_STROKE:
	status = _cairo_pdf_operators_stroke (&surface->pdf_operators,
					      &group->path,
					      &group->style,
					      &group->ctm,
					      &group->ctm_inverse);
	break;
    case PDF_SHOW_GLYPHS:
	status = _cairo_pdf_operators_show_text_glyphs (&surface->pdf_operators,
							group->utf8, group->utf8_len,
							group->glyphs, group->num_glyphs,
							group->clusters, group->num_clusters,
							group->cluster_flags,
							group->scaled_font);
	break;
    }
    if (unlikely (status))
	return status;

    status = _cairo_pdf_surface_unselect_pattern (surface);
    if (unlikely (status))
	return status;

    status = _cairo_pdf_surface_close_group (surface, NULL);

RESTORE_SIZE:
    _cairo_pdf_surface_set_size_internal (surface,
					  old_width,
					  old_height);

    return status;
}

static cairo_status_t
_cairo_pdf_surface_write_patterns_and_smask_groups (cairo_pdf_surface_t *surface)
{
    cairo_pdf_pattern_t pattern;
    cairo_pdf_smask_group_t *group;
    cairo_pdf_source_surface_t src_surface;
    unsigned int pattern_index, group_index, surface_index;
    cairo_status_t status;

    /* Writing out PDF_MASK groups will cause additional smask groups
     * to be appended to surface->smask_groups. Additional patterns
     * may also be appended to surface->patterns.
     *
     * Writing recording surface patterns will cause additional patterns
     * and groups to be appended.
     */
    pattern_index = 0;
    group_index = 0;
    surface_index = 0;
    while ((pattern_index < _cairo_array_num_elements (&surface->page_patterns)) ||
	   (group_index < _cairo_array_num_elements (&surface->smask_groups)) ||
	   (surface_index < _cairo_array_num_elements (&surface->page_surfaces)))
    {
	for (; group_index < _cairo_array_num_elements (&surface->smask_groups); group_index++) {
	    _cairo_array_copy_element (&surface->smask_groups, group_index, &group);
	    status = _cairo_pdf_surface_write_smask_group (surface, group);
	    if (unlikely (status))
		return status;
	}

	for (; pattern_index < _cairo_array_num_elements (&surface->page_patterns); pattern_index++) {
	    _cairo_array_copy_element (&surface->page_patterns, pattern_index, &pattern);
	    status = _cairo_pdf_surface_emit_pattern (surface, &pattern);
	    if (unlikely (status))
		return status;
	}

	for (; surface_index < _cairo_array_num_elements (&surface->page_surfaces); surface_index++) {
	    _cairo_array_copy_element (&surface->page_surfaces, surface_index, &src_surface);
	    status = _cairo_pdf_surface_emit_surface (surface, &src_surface);
	    if (unlikely (status))
		return status;
	}
    }

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_pdf_surface_write_page (cairo_pdf_surface_t *surface)
{
    cairo_pdf_resource_t page, knockout, res;
    cairo_status_t status;
    unsigned int i, len;

    _cairo_pdf_group_resources_clear (&surface->resources);
    if (surface->has_fallback_images) {
	cairo_rectangle_int_t extents;
	cairo_box_double_t    bbox;

	extents.x = 0;
	extents.y = 0;
	extents.width = ceil (surface->width);
	extents.height = ceil (surface->height);
	_get_bbox_from_extents (surface->height, &extents, &bbox);
	status = _cairo_pdf_surface_open_knockout_group (surface, &bbox);
	if (unlikely (status))
	    return status;

	len = _cairo_array_num_elements (&surface->knockout_group);
	for (i = 0; i < len; i++) {
	    _cairo_array_copy_element (&surface->knockout_group, i, &res);
	    _cairo_output_stream_printf (surface->output,
					 "/x%d Do\n",
					 res.id);
	    status = _cairo_pdf_surface_add_xobject (surface, res);
	    if (unlikely (status))
		return status;
	}
	_cairo_output_stream_printf (surface->output,
				     "/x%d Do\n",
				     surface->content.id);
	status = _cairo_pdf_surface_add_xobject (surface, surface->content);
	if (unlikely (status))
	    return status;

	status = _cairo_pdf_surface_close_group (surface, &knockout);
	if (unlikely (status))
	    return status;

	_cairo_pdf_group_resources_clear (&surface->resources);
	status = _cairo_pdf_surface_open_content_stream (surface, NULL, NULL, FALSE);
	if (unlikely (status))
	    return status;

	_cairo_output_stream_printf (surface->output,
				     "/x%d Do\n",
				     knockout.id);
	status = _cairo_pdf_surface_add_xobject (surface, knockout);
	if (unlikely (status))
	    return status;

	status = _cairo_pdf_surface_close_content_stream (surface);
	if (unlikely (status))
	    return status;
    }

    page = _cairo_pdf_surface_new_object (surface);
    if (page.id == 0)
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    _cairo_output_stream_printf (surface->output,
				 "%d 0 obj\n"
				 "<< /Type /Page\n"
				 "   /Parent %d 0 R\n"
				 "   /MediaBox [ 0 0 %f %f ]\n"
				 "   /Contents %d 0 R\n"
				 "   /Group <<\n"
				 "      /Type /Group\n"
				 "      /S /Transparency\n"
				 "      /I true\n"
				 "      /CS /DeviceRGB\n"
				 "   >>\n"
				 "   /Resources %d 0 R\n"
				 ">>\n"
				 "endobj\n",
				 page.id,
				 surface->pages_resource.id,
				 surface->width,
				 surface->height,
				 surface->content.id,
				 surface->content_resources.id);

    status = _cairo_array_append (&surface->pages, &page);
    if (unlikely (status))
	return status;

    status = _cairo_pdf_surface_write_patterns_and_smask_groups (surface);
    if (unlikely (status))
	return status;

    return CAIRO_STATUS_SUCCESS;
}

static cairo_int_status_t
_cairo_pdf_surface_analyze_surface_pattern_transparency (cairo_pdf_surface_t      *surface,
							 cairo_surface_pattern_t *pattern)
{
    cairo_image_surface_t  *image;
    void		   *image_extra;
    cairo_int_status_t      status;
    cairo_image_transparency_t transparency;

    status = _cairo_surface_acquire_source_image (pattern->surface,
						  &image,
						  &image_extra);
    if (unlikely (status))
	return status;

    if (image->base.status)
	return image->base.status;

    transparency = _cairo_image_analyze_transparency (image);
    if (transparency == CAIRO_IMAGE_IS_OPAQUE)
	status = CAIRO_STATUS_SUCCESS;
    else
	status = CAIRO_INT_STATUS_FLATTEN_TRANSPARENCY;

    _cairo_surface_release_source_image (pattern->surface, image, image_extra);

    return status;
}

static cairo_bool_t
_surface_pattern_supported (cairo_surface_pattern_t *pattern)
{
    cairo_extend_t extend;

    if (pattern->surface->type == CAIRO_SURFACE_TYPE_RECORDING)
	return TRUE;

    if (pattern->surface->backend->acquire_source_image == NULL)
	return FALSE;

    /* Does an ALPHA-only source surface even make sense? Maybe, but I
     * don't think it's worth the extra code to support it. */

/* XXX: Need to write this function here...
    content = cairo_surface_get_content (pattern->surface);
    if (content == CAIRO_CONTENT_ALPHA)
	return FALSE;
*/

    extend = cairo_pattern_get_extend (&pattern->base);
    switch (extend) {
    case CAIRO_EXTEND_NONE:
    case CAIRO_EXTEND_REPEAT:
    case CAIRO_EXTEND_REFLECT:
    /* There's no point returning FALSE for EXTEND_PAD, as the image
     * surface does not currently implement it either */
    case CAIRO_EXTEND_PAD:
	return TRUE;
    }

    ASSERT_NOT_REACHED;
    return FALSE;
}

static cairo_bool_t
_pattern_supported (const cairo_pattern_t *pattern)
{
    switch (pattern->type) {
    case CAIRO_PATTERN_TYPE_SOLID:
    case CAIRO_PATTERN_TYPE_LINEAR:
    case CAIRO_PATTERN_TYPE_RADIAL:
    case CAIRO_PATTERN_TYPE_MESH:
    case CAIRO_PATTERN_TYPE_RASTER_SOURCE:
	return TRUE;

    case CAIRO_PATTERN_TYPE_SURFACE:
	return _surface_pattern_supported ((cairo_surface_pattern_t *) pattern);

    default:
	ASSERT_NOT_REACHED;
	return FALSE;
    }
}

static cairo_bool_t
_pdf_operator_supported (cairo_operator_t op)
{
    switch (op) {
    case CAIRO_OPERATOR_OVER:
    case CAIRO_OPERATOR_MULTIPLY:
    case CAIRO_OPERATOR_SCREEN:
    case CAIRO_OPERATOR_OVERLAY:
    case CAIRO_OPERATOR_DARKEN:
    case CAIRO_OPERATOR_LIGHTEN:
    case CAIRO_OPERATOR_COLOR_DODGE:
    case CAIRO_OPERATOR_COLOR_BURN:
    case CAIRO_OPERATOR_HARD_LIGHT:
    case CAIRO_OPERATOR_SOFT_LIGHT:
    case CAIRO_OPERATOR_DIFFERENCE:
    case CAIRO_OPERATOR_EXCLUSION:
    case CAIRO_OPERATOR_HSL_HUE:
    case CAIRO_OPERATOR_HSL_SATURATION:
    case CAIRO_OPERATOR_HSL_COLOR:
    case CAIRO_OPERATOR_HSL_LUMINOSITY:
	return TRUE;

    default:
    case CAIRO_OPERATOR_CLEAR:
    case CAIRO_OPERATOR_SOURCE:
    case CAIRO_OPERATOR_IN:
    case CAIRO_OPERATOR_OUT:
    case CAIRO_OPERATOR_ATOP:
    case CAIRO_OPERATOR_DEST:
    case CAIRO_OPERATOR_DEST_OVER:
    case CAIRO_OPERATOR_DEST_IN:
    case CAIRO_OPERATOR_DEST_OUT:
    case CAIRO_OPERATOR_DEST_ATOP:
    case CAIRO_OPERATOR_XOR:
    case CAIRO_OPERATOR_ADD:
    case CAIRO_OPERATOR_SATURATE:
	return FALSE;
    }
}

static cairo_int_status_t
_cairo_pdf_surface_analyze_operation (cairo_pdf_surface_t  *surface,
				      cairo_operator_t      op,
				      const cairo_pattern_t      *pattern,
				      const cairo_rectangle_int_t	 *extents)
{
    if (surface->force_fallbacks &&
	surface->paginated_mode == CAIRO_PAGINATED_MODE_ANALYZE)
    {
	return CAIRO_INT_STATUS_UNSUPPORTED;
    }

    if (! _pattern_supported (pattern))
	return CAIRO_INT_STATUS_UNSUPPORTED;

    if (_pdf_operator_supported (op)) {
	if (pattern->type == CAIRO_PATTERN_TYPE_SURFACE) {
	    cairo_surface_pattern_t *surface_pattern = (cairo_surface_pattern_t *) pattern;

	    if (surface_pattern->surface->type == CAIRO_SURFACE_TYPE_RECORDING) {
		if (pattern->extend == CAIRO_EXTEND_PAD)
		    return CAIRO_INT_STATUS_UNSUPPORTED;
		else
		    return CAIRO_INT_STATUS_ANALYZE_RECORDING_SURFACE_PATTERN;
	    }
	}

	return CAIRO_STATUS_SUCCESS;
    }


    /* The SOURCE operator is supported if the pattern is opaque or if
     * there is nothing painted underneath. */
    if (op == CAIRO_OPERATOR_SOURCE) {
	if (pattern->type == CAIRO_PATTERN_TYPE_SURFACE) {
	    cairo_surface_pattern_t *surface_pattern = (cairo_surface_pattern_t *) pattern;

	    if (surface_pattern->surface->type == CAIRO_SURFACE_TYPE_RECORDING) {
		if (_cairo_pattern_is_opaque (pattern, extents)) {
		    return CAIRO_INT_STATUS_ANALYZE_RECORDING_SURFACE_PATTERN;
		} else {
		    /* FIXME: The analysis surface does not yet have
		     * the capability to analyze a non opaque recording
		     * surface and mark it supported if there is
		     * nothing underneath. For now recording surfaces of
		     * type CONTENT_COLOR_ALPHA painted with
		     * OPERATOR_SOURCE will result in a fallback
		     * image. */

		    return CAIRO_INT_STATUS_UNSUPPORTED;
		}
	    } else {
		return _cairo_pdf_surface_analyze_surface_pattern_transparency (surface,
										surface_pattern);
	    }
	}

	if (_cairo_pattern_is_opaque (pattern, extents))
	    return CAIRO_STATUS_SUCCESS;
	else
	    return CAIRO_INT_STATUS_FLATTEN_TRANSPARENCY;
    }

    return CAIRO_INT_STATUS_UNSUPPORTED;
}

static cairo_bool_t
_cairo_pdf_surface_operation_supported (cairo_pdf_surface_t  *surface,
					cairo_operator_t      op,
					const cairo_pattern_t      *pattern,
					const cairo_rectangle_int_t *extents)
{
    return _cairo_pdf_surface_analyze_operation (surface, op, pattern, extents) != CAIRO_INT_STATUS_UNSUPPORTED;
}

static cairo_int_status_t
_cairo_pdf_surface_start_fallback (cairo_pdf_surface_t *surface)
{
    cairo_box_double_t bbox;
    cairo_status_t status;

    status = _cairo_pdf_surface_close_content_stream (surface);
    if (unlikely (status))
	return status;

    status = _cairo_array_append (&surface->knockout_group, &surface->content);
    if (unlikely (status))
	return status;

    _cairo_pdf_group_resources_clear (&surface->resources);
    bbox.p1.x = 0;
    bbox.p1.y = 0;
    bbox.p2.x = surface->width;
    bbox.p2.y = surface->height;
    return _cairo_pdf_surface_open_content_stream (surface, &bbox, NULL, TRUE);
}

/* A PDF stencil mask is an A1 mask used with the current color */
static cairo_int_status_t
_cairo_pdf_surface_emit_stencil_mask (cairo_pdf_surface_t         *surface,
				      const cairo_pattern_t       *source,
				      const cairo_pattern_t       *mask,
				      const cairo_rectangle_int_t *extents)
{
    cairo_status_t status;
    cairo_image_surface_t  *image;
    void		   *image_extra;
    cairo_image_transparency_t transparency;
    cairo_pdf_resource_t pattern_res = {0};

    if (! (source->type == CAIRO_PATTERN_TYPE_SOLID &&
	   (mask->type == CAIRO_PATTERN_TYPE_SURFACE || mask->type == CAIRO_PATTERN_TYPE_RASTER_SOURCE)))
	return CAIRO_INT_STATUS_UNSUPPORTED;

    if (mask->type == CAIRO_PATTERN_TYPE_SURFACE &&
	((cairo_surface_pattern_t *) mask)->surface->type == CAIRO_SURFACE_TYPE_RECORDING)
    {
	return CAIRO_INT_STATUS_UNSUPPORTED;
    }

    status = _cairo_pdf_surface_acquire_source_image_from_pattern (surface, mask, extents,
								   &image, &image_extra);
    if (unlikely (status))
	return status;

    if (image->base.status)
	return image->base.status;

    transparency = _cairo_image_analyze_transparency (image);
    if (transparency != CAIRO_IMAGE_IS_OPAQUE &&
	transparency != CAIRO_IMAGE_HAS_BILEVEL_ALPHA)
    {
	status = CAIRO_INT_STATUS_UNSUPPORTED;
	goto cleanup;
    }

    status = _cairo_pdf_surface_select_pattern (surface, source,
						pattern_res, FALSE);
    if (unlikely (status))
	return status;

    status = _cairo_pdf_operators_flush (&surface->pdf_operators);
    if (unlikely (status))
	return status;

    _cairo_output_stream_printf (surface->output, "q\n");
    status = _cairo_pdf_surface_paint_surface_pattern (surface, mask, NULL, TRUE);
    if (unlikely (status))
	return status;

    _cairo_output_stream_printf (surface->output, "Q\n");

    status = _cairo_output_stream_get_status (surface->output);

cleanup:
    _cairo_pdf_surface_release_source_image_from_pattern (surface, mask, image, image_extra);

    return status;
}

static cairo_int_status_t
_cairo_pdf_surface_set_clip (cairo_pdf_surface_t *surface,
			     cairo_composite_rectangles_t *composite)
{
    cairo_clip_t *clip = composite->clip;

    if (_cairo_composite_rectangles_can_reduce_clip (composite, clip))
	clip = NULL;

    if (clip == NULL) {
	if (_cairo_composite_rectangles_can_reduce_clip (composite,
							 surface->clipper.clip))
	    return CAIRO_STATUS_SUCCESS;
    }

    return _cairo_surface_clipper_set_clip (&surface->clipper, clip);
}

static cairo_int_status_t
_cairo_pdf_surface_paint (void			*abstract_surface,
			  cairo_operator_t	 op,
			  const cairo_pattern_t	*source,
			  const cairo_clip_t	*clip)
{
    cairo_pdf_surface_t *surface = abstract_surface;
    cairo_pdf_smask_group_t *group;
    cairo_pdf_resource_t pattern_res, gstate_res;
    cairo_composite_rectangles_t extents;
    cairo_int_status_t status;

    status = _cairo_composite_rectangles_init_for_paint (&extents,
							 &surface->base,
							 op, source, clip);
    if (unlikely (status))
	return status;

    if (surface->paginated_mode == CAIRO_PAGINATED_MODE_ANALYZE) {
	status = _cairo_pdf_surface_analyze_operation (surface, op, source, &extents.bounded);
	goto cleanup;
    } else if (surface->paginated_mode == CAIRO_PAGINATED_MODE_FALLBACK) {
	status = _cairo_pdf_surface_start_fallback (surface);
	if (unlikely (status))
	    goto cleanup;
    }

    assert (_cairo_pdf_surface_operation_supported (surface, op, source, &extents.bounded));

    status = _cairo_pdf_surface_set_clip (surface, &extents);
    if (unlikely (status))
	goto cleanup;

    status = _cairo_pdf_surface_select_operator (surface, op);
    if (unlikely (status))
	goto cleanup;

    status = _cairo_pdf_operators_flush (&surface->pdf_operators);
    if (unlikely (status))
	goto cleanup;

    if (_can_paint_pattern (source)) {
	_cairo_output_stream_printf (surface->output, "q\n");
	status = _cairo_pdf_surface_paint_pattern (surface,
						   source,
						   &extents.bounded,
						   FALSE);
	if (unlikely (status))
	    goto cleanup;

	_cairo_output_stream_printf (surface->output, "Q\n");
	_cairo_composite_rectangles_fini (&extents);
	return _cairo_output_stream_get_status (surface->output);
    }

    pattern_res.id = 0;
    gstate_res.id = 0;
    status = _cairo_pdf_surface_add_pdf_pattern (surface, source,
						 &extents.bounded,
						 &pattern_res, &gstate_res);
    if (unlikely (status))
	goto cleanup;

    if (gstate_res.id != 0) {
	group = _cairo_pdf_surface_create_smask_group (surface, &extents.bounded);
	if (unlikely (group == NULL)) {
	    status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	    goto cleanup;
	}

	group->operation = PDF_PAINT;
	status = _cairo_pattern_create_copy (&group->source, source);
	if (unlikely (status)) {
	    _cairo_pdf_smask_group_destroy (group);
	    goto cleanup;
	}
	group->source_res = pattern_res;
	status = _cairo_pdf_surface_add_smask_group (surface, group);
	if (unlikely (status)) {
	    _cairo_pdf_smask_group_destroy (group);
	    goto cleanup;
	}

	status = _cairo_pdf_surface_add_smask (surface, gstate_res);
	if (unlikely (status))
	    goto cleanup;

	status = _cairo_pdf_surface_add_xobject (surface, group->group_res);
	if (unlikely (status))
	    goto cleanup;

	_cairo_output_stream_printf (surface->output,
				     "q /s%d gs /x%d Do Q\n",
				     gstate_res.id,
				     group->group_res.id);
    } else {
	status = _cairo_pdf_surface_select_pattern (surface, source,
						    pattern_res, FALSE);
	if (unlikely (status))
	    goto cleanup;

	_cairo_output_stream_printf (surface->output,
				     "0 0 %f %f re f\n",
				     surface->width, surface->height);

	status = _cairo_pdf_surface_unselect_pattern (surface);
	if (unlikely (status))
	    goto cleanup;
    }

    _cairo_composite_rectangles_fini (&extents);
    return _cairo_output_stream_get_status (surface->output);

cleanup:
    _cairo_composite_rectangles_fini (&extents);
    return status;
}

static cairo_int_status_t
_cairo_pdf_surface_mask (void			*abstract_surface,
			 cairo_operator_t	 op,
			 const cairo_pattern_t	*source,
			 const cairo_pattern_t	*mask,
			 const cairo_clip_t	*clip)
{
    cairo_pdf_surface_t *surface = abstract_surface;
    cairo_pdf_smask_group_t *group;
    cairo_composite_rectangles_t extents;
    cairo_int_status_t status;
    cairo_rectangle_int_t r;
    cairo_box_t box;

    status = _cairo_composite_rectangles_init_for_mask (&extents,
							&surface->base,
							op, source, mask, clip);
    if (unlikely (status))
	return status;

    if (surface->paginated_mode == CAIRO_PAGINATED_MODE_ANALYZE) {
	cairo_status_t source_status, mask_status;

	status = _cairo_pdf_surface_analyze_operation (surface, op, source, &extents.bounded);
	if (_cairo_int_status_is_error (status))
	    goto cleanup;
	source_status = status;

	if (mask->has_component_alpha) {
	    status = CAIRO_INT_STATUS_UNSUPPORTED;
	} else {
	    status = _cairo_pdf_surface_analyze_operation (surface, op, mask, &extents.bounded);
	    if (_cairo_int_status_is_error (status))
		goto cleanup;
	}
	mask_status = status;

	_cairo_composite_rectangles_fini (&extents);
	return _cairo_analysis_surface_merge_status (source_status,
						     mask_status);
    } else if (surface->paginated_mode == CAIRO_PAGINATED_MODE_FALLBACK) {
	status = _cairo_pdf_surface_start_fallback (surface);
	if (unlikely (status))
	    goto cleanup;
    }

    assert (_cairo_pdf_surface_operation_supported (surface, op, source, &extents.bounded));
    assert (_cairo_pdf_surface_operation_supported (surface, op, mask, &extents.bounded));

    /* get the accurate extents */
    status = _cairo_pattern_get_ink_extents (source, &r);
    if (unlikely (status))
	goto cleanup;

    /* XXX slight impedance mismatch */
    _cairo_box_from_rectangle (&box, &r);
    status = _cairo_composite_rectangles_intersect_source_extents (&extents,
								   &box);
    if (unlikely (status))
	goto cleanup;

    status = _cairo_pattern_get_ink_extents (mask, &r);
    if (unlikely (status))
	goto cleanup;

    _cairo_box_from_rectangle (&box, &r);
    status = _cairo_composite_rectangles_intersect_mask_extents (&extents,
								 &box);
    if (unlikely (status))
	goto cleanup;

    status = _cairo_pdf_surface_set_clip (surface, &extents);
    if (unlikely (status))
	goto cleanup;

    status = _cairo_pdf_surface_select_operator (surface, op);
    if (unlikely (status))
	goto cleanup;

    /* Check if we can use a stencil mask */
    status = _cairo_pdf_surface_emit_stencil_mask (surface, source, mask, &extents.bounded);
    if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	goto cleanup;

    group = _cairo_pdf_surface_create_smask_group (surface, &extents.bounded);
    if (unlikely (group == NULL)) {
	status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	goto cleanup;
    }

    group->operation = PDF_MASK;
    status = _cairo_pattern_create_copy (&group->source, source);
    if (unlikely (status)) {
	_cairo_pdf_smask_group_destroy (group);
	goto cleanup;
    }
    status = _cairo_pattern_create_copy (&group->mask, mask);
    if (unlikely (status)) {
	_cairo_pdf_smask_group_destroy (group);
	goto cleanup;
    }
    group->source_res = _cairo_pdf_surface_new_object (surface);
    if (group->source_res.id == 0) {
	_cairo_pdf_smask_group_destroy (group);
	status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	goto cleanup;
    }

    status = _cairo_pdf_surface_add_smask_group (surface, group);
    if (unlikely (status)) {
	_cairo_pdf_smask_group_destroy (group);
	goto cleanup;
    }

    status = _cairo_pdf_surface_add_smask (surface, group->group_res);
    if (unlikely (status))
	goto cleanup;

    status = _cairo_pdf_surface_add_xobject (surface, group->source_res);
    if (unlikely (status))
	goto cleanup;

    status = _cairo_pdf_operators_flush (&surface->pdf_operators);
    if (unlikely (status))
	goto cleanup;

    _cairo_output_stream_printf (surface->output,
				 "q /s%d gs /x%d Do Q\n",
				 group->group_res.id,
				 group->source_res.id);

    _cairo_composite_rectangles_fini (&extents);
    return _cairo_output_stream_get_status (surface->output);

cleanup:
    _cairo_composite_rectangles_fini (&extents);
    return status;
}

static cairo_int_status_t
_cairo_pdf_surface_stroke (void			*abstract_surface,
			   cairo_operator_t	 op,
			   const cairo_pattern_t *source,
			   const cairo_path_fixed_t	*path,
			   const cairo_stroke_style_t	*style,
			   const cairo_matrix_t	*ctm,
			   const cairo_matrix_t	*ctm_inverse,
			   double		 tolerance,
			   cairo_antialias_t	 antialias,
			   const cairo_clip_t	*clip)
{
    cairo_pdf_surface_t *surface = abstract_surface;
    cairo_pdf_smask_group_t *group;
    cairo_pdf_resource_t pattern_res, gstate_res;
    cairo_composite_rectangles_t extents;
    cairo_int_status_t status;

    status = _cairo_composite_rectangles_init_for_stroke (&extents,
							  &surface->base,
							  op, source,
							  path, style, ctm,
							  clip);
    if (unlikely (status))
	return status;

    /* use the more accurate extents */
    if (extents.is_bounded) {
	cairo_rectangle_int_t mask;
	cairo_box_t box;

	status = _cairo_path_fixed_stroke_extents (path, style,
						   ctm, ctm_inverse,
						   tolerance,
						   &mask);
	if (unlikely (status))
	    goto cleanup;

	_cairo_box_from_rectangle (&box, &mask);
	status = _cairo_composite_rectangles_intersect_mask_extents (&extents,
								     &box);
	if (unlikely (status))
	    goto cleanup;
    }

    if (surface->paginated_mode == CAIRO_PAGINATED_MODE_ANALYZE) {
	status = _cairo_pdf_surface_analyze_operation (surface, op, source, &extents.bounded);
	goto cleanup;
    }

    assert (_cairo_pdf_surface_operation_supported (surface, op, source, &extents.bounded));

    status = _cairo_pdf_surface_set_clip (surface, &extents);
    if (unlikely (status))
	goto cleanup;

    pattern_res.id = 0;
    gstate_res.id = 0;
    status = _cairo_pdf_surface_add_pdf_pattern (surface, source,
						 &extents.bounded,
						 &pattern_res, &gstate_res);
    if (unlikely (status))
	goto cleanup;

    status = _cairo_pdf_surface_select_operator (surface, op);
    if (unlikely (status))
	goto cleanup;

    if (gstate_res.id != 0) {
	group = _cairo_pdf_surface_create_smask_group (surface, &extents.bounded);
	if (unlikely (group == NULL)) {
	    status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	    goto cleanup;
	}

	group->operation = PDF_STROKE;
	status = _cairo_pattern_create_copy (&group->source, source);
	if (unlikely (status)) {
	    _cairo_pdf_smask_group_destroy (group);
	    goto cleanup;
	}
	group->source_res = pattern_res;
	status = _cairo_path_fixed_init_copy (&group->path, path);
	if (unlikely (status)) {
	    _cairo_pdf_smask_group_destroy (group);
	    goto cleanup;
	}

	group->style = *style;
	group->ctm = *ctm;
	group->ctm_inverse = *ctm_inverse;
	status = _cairo_pdf_surface_add_smask_group (surface, group);
	if (unlikely (status)) {
	    _cairo_pdf_smask_group_destroy (group);
	    goto cleanup;
	}

	status = _cairo_pdf_surface_add_smask (surface, gstate_res);
	if (unlikely (status))
	    goto cleanup;

	status = _cairo_pdf_surface_add_xobject (surface, group->group_res);
	if (unlikely (status))
	    goto cleanup;

	status = _cairo_pdf_operators_flush (&surface->pdf_operators);
	if (unlikely (status))
	    goto cleanup;

	_cairo_output_stream_printf (surface->output,
				     "q /s%d gs /x%d Do Q\n",
				     gstate_res.id,
				     group->group_res.id);
    } else {
	status = _cairo_pdf_surface_select_pattern (surface, source, pattern_res, TRUE);
	if (unlikely (status))
	    goto cleanup;

	status = _cairo_pdf_operators_stroke (&surface->pdf_operators,
					      path,
					      style,
					      ctm,
					      ctm_inverse);
	if (unlikely (status))
	    goto cleanup;

	status = _cairo_pdf_surface_unselect_pattern (surface);
	if (unlikely (status))
	    goto cleanup;
    }

    _cairo_composite_rectangles_fini (&extents);
    return _cairo_output_stream_get_status (surface->output);

cleanup:
    _cairo_composite_rectangles_fini (&extents);
    return status;
}

static cairo_int_status_t
_cairo_pdf_surface_fill (void			*abstract_surface,
			 cairo_operator_t	 op,
			 const cairo_pattern_t	*source,
			 const cairo_path_fixed_t*path,
			 cairo_fill_rule_t	 fill_rule,
			 double			 tolerance,
			 cairo_antialias_t	 antialias,
			 const cairo_clip_t	*clip)
{
    cairo_pdf_surface_t *surface = abstract_surface;
    cairo_int_status_t status;
    cairo_pdf_smask_group_t *group;
    cairo_pdf_resource_t pattern_res, gstate_res;
    cairo_composite_rectangles_t extents;

    status = _cairo_composite_rectangles_init_for_fill (&extents,
							&surface->base,
							op, source, path,
							clip);
    if (unlikely (status))
	return status;

    /* use the more accurate extents */
    if (extents.is_bounded) {
	cairo_rectangle_int_t mask;
	cairo_box_t box;

	_cairo_path_fixed_fill_extents (path,
					fill_rule,
					tolerance,
					&mask);

	_cairo_box_from_rectangle (&box, &mask);
	status = _cairo_composite_rectangles_intersect_mask_extents (&extents,
								     &box);
	if (unlikely (status))
	    goto cleanup;
    }

    if (surface->paginated_mode == CAIRO_PAGINATED_MODE_ANALYZE) {
	status = _cairo_pdf_surface_analyze_operation (surface, op, source, &extents.bounded);
	goto cleanup;
    } else if (surface->paginated_mode == CAIRO_PAGINATED_MODE_FALLBACK) {
	status = _cairo_pdf_surface_start_fallback (surface);
	if (unlikely (status))
	    goto cleanup;
    }

    assert (_cairo_pdf_surface_operation_supported (surface, op, source, &extents.bounded));

    status = _cairo_pdf_surface_set_clip (surface, &extents);
    if (unlikely (status))
	goto cleanup;

    status = _cairo_pdf_surface_select_operator (surface, op);
    if (unlikely (status))
	goto cleanup;

    if (_can_paint_pattern (source)) {
	status = _cairo_pdf_operators_flush (&surface->pdf_operators);
	if (unlikely (status))
	    goto cleanup;

	_cairo_output_stream_printf (surface->output, "q\n");
	status =  _cairo_pdf_operators_clip (&surface->pdf_operators,
					     path,
					     fill_rule);
	if (unlikely (status))
	    goto cleanup;

	status = _cairo_pdf_surface_paint_pattern (surface,
						   source,
						   &extents.bounded,
						   FALSE);
	if (unlikely (status))
	    goto cleanup;

	_cairo_output_stream_printf (surface->output, "Q\n");
	status = _cairo_output_stream_get_status (surface->output);
	goto cleanup;
    }

    pattern_res.id = 0;
    gstate_res.id = 0;
    status = _cairo_pdf_surface_add_pdf_pattern (surface, source,
						 &extents.bounded,
						 &pattern_res, &gstate_res);
    if (unlikely (status))
	goto cleanup;

    if (gstate_res.id != 0) {
	group = _cairo_pdf_surface_create_smask_group (surface, &extents.bounded);
	if (unlikely (group == NULL)) {
	    status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	    goto cleanup;
	}

	group->operation = PDF_FILL;
	status = _cairo_pattern_create_copy (&group->source, source);
	if (unlikely (status)) {
	    _cairo_pdf_smask_group_destroy (group);
	    goto cleanup;
	}
	group->source_res = pattern_res;
	status = _cairo_path_fixed_init_copy (&group->path, path);
	if (unlikely (status)) {
	    _cairo_pdf_smask_group_destroy (group);
	    goto cleanup;
	}

	group->fill_rule = fill_rule;
	status = _cairo_pdf_surface_add_smask_group (surface, group);
	if (unlikely (status)) {
	    _cairo_pdf_smask_group_destroy (group);
	    goto cleanup;
	}

	status = _cairo_pdf_surface_add_smask (surface, gstate_res);
	if (unlikely (status))
	    goto cleanup;

	status = _cairo_pdf_surface_add_xobject (surface, group->group_res);
	if (unlikely (status))
	    goto cleanup;

	status = _cairo_pdf_operators_flush (&surface->pdf_operators);
	if (unlikely (status))
	    goto cleanup;

	_cairo_output_stream_printf (surface->output,
				     "q /s%d gs /x%d Do Q\n",
				     gstate_res.id,
				     group->group_res.id);
    } else {
	status = _cairo_pdf_surface_select_pattern (surface, source, pattern_res, FALSE);
	if (unlikely (status))
	    goto cleanup;

	status = _cairo_pdf_operators_fill (&surface->pdf_operators,
					    path,
					    fill_rule);
	if (unlikely (status))
	    goto cleanup;

	status = _cairo_pdf_surface_unselect_pattern (surface);
	if (unlikely (status))
	    goto cleanup;
    }

    _cairo_composite_rectangles_fini (&extents);
    return _cairo_output_stream_get_status (surface->output);

cleanup:
    _cairo_composite_rectangles_fini (&extents);
    return status;
}

static cairo_int_status_t
_cairo_pdf_surface_fill_stroke (void			*abstract_surface,
				cairo_operator_t	 fill_op,
				const cairo_pattern_t	*fill_source,
				cairo_fill_rule_t	 fill_rule,
				double			 fill_tolerance,
				cairo_antialias_t	 fill_antialias,
				const cairo_path_fixed_t*path,
				cairo_operator_t	 stroke_op,
				const cairo_pattern_t	*stroke_source,
				const cairo_stroke_style_t *stroke_style,
				const cairo_matrix_t	*stroke_ctm,
				const cairo_matrix_t	*stroke_ctm_inverse,
				double			 stroke_tolerance,
				cairo_antialias_t	 stroke_antialias,
				const cairo_clip_t	*clip)
{
    cairo_pdf_surface_t *surface = abstract_surface;
    cairo_int_status_t status;
    cairo_pdf_resource_t fill_pattern_res, stroke_pattern_res, gstate_res;
    cairo_composite_rectangles_t extents;

    /* During analysis we return unsupported and let the _fill and
     * _stroke functions that are on the fallback path do the analysis
     * for us. During render we may still encounter unsupported
     * combinations of fill/stroke patterns. However we can return
     * unsupported anytime to let the _fill and _stroke functions take
     * over.
     */
    if (surface->paginated_mode == CAIRO_PAGINATED_MODE_ANALYZE)
	return CAIRO_INT_STATUS_UNSUPPORTED;

    /* PDF rendering of fill-stroke is not the same as cairo when
     * either the fill or stroke is not opaque.
     */
    if ( !_cairo_pattern_is_opaque (fill_source, NULL) ||
	 !_cairo_pattern_is_opaque (stroke_source, NULL))
    {
	return CAIRO_INT_STATUS_UNSUPPORTED;
    }

    if (fill_op != stroke_op)
	return CAIRO_INT_STATUS_UNSUPPORTED;

    /* Compute the operation extents using the stroke which will naturally
     * be larger than the fill extents.
     */
    status = _cairo_composite_rectangles_init_for_stroke (&extents,
							  &surface->base,
							  stroke_op, stroke_source,
							  path, stroke_style, stroke_ctm,
							  clip);
    if (unlikely (status))
	return status;

    /* use the more accurate extents */
    if (extents.is_bounded) {
	cairo_rectangle_int_t mask;
	cairo_box_t box;

	status = _cairo_path_fixed_stroke_extents (path, stroke_style,
						   stroke_ctm, stroke_ctm_inverse,
						   stroke_tolerance,
						   &mask);
	if (unlikely (status))
	    goto cleanup;

	_cairo_box_from_rectangle (&box, &mask);
	status = _cairo_composite_rectangles_intersect_mask_extents (&extents,
								     &box);
	if (unlikely (status))
	    goto cleanup;
    }

    status = _cairo_pdf_surface_set_clip (surface, &extents);
    if (unlikely (status))
	goto cleanup;

    status = _cairo_pdf_surface_select_operator (surface, fill_op);
    if (unlikely (status))
	goto cleanup;

    /* use the more accurate extents */
    if (extents.is_bounded) {
	cairo_rectangle_int_t mask;
	cairo_box_t box;

	_cairo_path_fixed_fill_extents (path,
					fill_rule,
					fill_tolerance,
					&mask);

	_cairo_box_from_rectangle (&box, &mask);
	status = _cairo_composite_rectangles_intersect_mask_extents (&extents,
								     &box);
	if (unlikely (status))
	    goto cleanup;
    }

    fill_pattern_res.id = 0;
    gstate_res.id = 0;
    status = _cairo_pdf_surface_add_pdf_pattern (surface, fill_source,
						 &extents.bounded,
						 &fill_pattern_res,
						 &gstate_res);
    if (unlikely (status))
	goto cleanup;

    assert (gstate_res.id == 0);

    stroke_pattern_res.id = 0;
    gstate_res.id = 0;
    status = _cairo_pdf_surface_add_pdf_pattern (surface,
						 stroke_source,
						 &extents.bounded,
						 &stroke_pattern_res,
						 &gstate_res);
    if (unlikely (status))
	goto cleanup;

    assert (gstate_res.id == 0);

    /* As PDF has separate graphics state for fill and stroke we can
     * select both at the same time */
    status = _cairo_pdf_surface_select_pattern (surface, fill_source,
						fill_pattern_res, FALSE);
    if (unlikely (status))
	goto cleanup;

    status = _cairo_pdf_surface_select_pattern (surface, stroke_source,
						stroke_pattern_res, TRUE);
    if (unlikely (status))
	goto cleanup;

    status = _cairo_pdf_operators_fill_stroke (&surface->pdf_operators,
					       path,
					       fill_rule,
					       stroke_style,
					       stroke_ctm,
					       stroke_ctm_inverse);
    if (unlikely (status))
	goto cleanup;

    status = _cairo_pdf_surface_unselect_pattern (surface);
    if (unlikely (status))
	goto cleanup;

    _cairo_composite_rectangles_fini (&extents);
    return _cairo_output_stream_get_status (surface->output);

cleanup:
    _cairo_composite_rectangles_fini (&extents);
    return status;
}

static cairo_bool_t
_cairo_pdf_surface_has_show_text_glyphs	(void			*abstract_surface)
{
    return TRUE;
}

static cairo_int_status_t
_cairo_pdf_surface_show_text_glyphs (void			*abstract_surface,
				     cairo_operator_t		 op,
				     const cairo_pattern_t	*source,
				     const char                 *utf8,
				     int                         utf8_len,
				     cairo_glyph_t		*glyphs,
				     int			 num_glyphs,
				     const cairo_text_cluster_t *clusters,
				     int                         num_clusters,
				     cairo_text_cluster_flags_t  cluster_flags,
				     cairo_scaled_font_t	*scaled_font,
				     const cairo_clip_t		*clip)
{
    cairo_pdf_surface_t *surface = abstract_surface;
    cairo_pdf_smask_group_t *group;
    cairo_pdf_resource_t pattern_res, gstate_res;
    cairo_composite_rectangles_t extents;
    cairo_bool_t overlap;
    cairo_int_status_t status;

    status = _cairo_composite_rectangles_init_for_glyphs (&extents,
							  &surface->base,
							  op, source,
							  scaled_font,
							  glyphs, num_glyphs,
							  clip,
							  &overlap);
    if (unlikely (status))
	return status;

    if (surface->paginated_mode == CAIRO_PAGINATED_MODE_ANALYZE) {
	status = _cairo_pdf_surface_analyze_operation (surface, op, source, &extents.bounded);
	goto cleanup;
    }

    assert (_cairo_pdf_surface_operation_supported (surface, op, source, &extents.bounded));

    status = _cairo_pdf_surface_set_clip (surface, &extents);
    if (unlikely (status))
	goto cleanup;

    pattern_res.id = 0;
    gstate_res.id = 0;
    status = _cairo_pdf_surface_add_pdf_pattern (surface, source,
						 &extents.bounded,
						 &pattern_res, &gstate_res);
    if (unlikely (status))
	goto cleanup;

    status = _cairo_pdf_surface_select_operator (surface, op);
    if (unlikely (status))
	goto cleanup;

    if (gstate_res.id != 0) {
	group = _cairo_pdf_surface_create_smask_group (surface, &extents.bounded);
	if (unlikely (group == NULL)) {
	    status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	    goto cleanup;
	}

	group->operation = PDF_SHOW_GLYPHS;
	status = _cairo_pattern_create_copy (&group->source, source);
	if (unlikely (status)) {
	    _cairo_pdf_smask_group_destroy (group);
	    goto cleanup;
	}
	group->source_res = pattern_res;

	if (utf8_len) {
	    group->utf8 = malloc (utf8_len);
	    if (unlikely (group->utf8 == NULL)) {
		_cairo_pdf_smask_group_destroy (group);
		status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
		goto cleanup;
	    }
	    memcpy (group->utf8, utf8, utf8_len);
	}
	group->utf8_len = utf8_len;

	if (num_glyphs) {
	    group->glyphs = _cairo_malloc_ab (num_glyphs, sizeof (cairo_glyph_t));
	    if (unlikely (group->glyphs == NULL)) {
		_cairo_pdf_smask_group_destroy (group);
		status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
		goto cleanup;
	    }
	    memcpy (group->glyphs, glyphs, sizeof (cairo_glyph_t) * num_glyphs);
	}
	group->num_glyphs = num_glyphs;

	if (num_clusters) {
	    group->clusters = _cairo_malloc_ab (num_clusters, sizeof (cairo_text_cluster_t));
	    if (unlikely (group->clusters == NULL)) {
		_cairo_pdf_smask_group_destroy (group);
		status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
		goto cleanup;
	    }
	    memcpy (group->clusters, clusters, sizeof (cairo_text_cluster_t) * num_clusters);
	}
	group->num_clusters = num_clusters;

	group->scaled_font = cairo_scaled_font_reference (scaled_font);
	status = _cairo_pdf_surface_add_smask_group (surface, group);
	if (unlikely (status)) {
	    _cairo_pdf_smask_group_destroy (group);
	    goto cleanup;
	}

	status = _cairo_pdf_surface_add_smask (surface, gstate_res);
	if (unlikely (status))
	    goto cleanup;

	status = _cairo_pdf_surface_add_xobject (surface, group->group_res);
	if (unlikely (status))
	    goto cleanup;

	status = _cairo_pdf_operators_flush (&surface->pdf_operators);
	if (unlikely (status))
	    goto cleanup;

	_cairo_output_stream_printf (surface->output,
				     "q /s%d gs /x%d Do Q\n",
				     gstate_res.id,
				     group->group_res.id);
    } else {
	status = _cairo_pdf_surface_select_pattern (surface, source, pattern_res, FALSE);
	if (unlikely (status))
	    goto cleanup;

	/* Each call to show_glyphs() with a transclucent pattern must
	 * be in a separate text object otherwise overlapping text
	 * from separate calls to show_glyphs will not composite with
	 * each other. */
	if (! _cairo_pattern_is_opaque (source, &extents.bounded)) {
	    status = _cairo_pdf_operators_flush (&surface->pdf_operators);
	    if (unlikely (status))
		goto cleanup;
	}

	status = _cairo_pdf_operators_show_text_glyphs (&surface->pdf_operators,
							utf8, utf8_len,
							glyphs, num_glyphs,
							clusters, num_clusters,
							cluster_flags,
							scaled_font);
	if (unlikely (status))
	    goto cleanup;

	status = _cairo_pdf_surface_unselect_pattern (surface);
	if (unlikely (status))
	    goto cleanup;
    }

    _cairo_composite_rectangles_fini (&extents);
    return _cairo_output_stream_get_status (surface->output);

cleanup:
    _cairo_composite_rectangles_fini (&extents);
    return status;
}

static const char **
_cairo_pdf_surface_get_supported_mime_types (void		 *abstract_surface)
{
    return _cairo_pdf_supported_mime_types;
}

static void
_cairo_pdf_surface_set_paginated_mode (void			*abstract_surface,
				       cairo_paginated_mode_t	 paginated_mode)
{
    cairo_pdf_surface_t *surface = abstract_surface;

    surface->paginated_mode = paginated_mode;
}

static const cairo_surface_backend_t cairo_pdf_surface_backend = {
    CAIRO_SURFACE_TYPE_PDF,
    _cairo_pdf_surface_finish,

    _cairo_default_context_create,

    NULL, /* create similar: handled by wrapper */
    NULL, /* create similar image */
    NULL, /* map to image */
    NULL, /* unmap image */

    _cairo_surface_default_source,
    NULL, /* acquire_source_image */
    NULL, /* release_source_image */
    NULL, /* snapshot */

    NULL,  /* _cairo_pdf_surface_copy_page */
    _cairo_pdf_surface_show_page,

    _cairo_pdf_surface_get_extents,
    _cairo_pdf_surface_get_font_options,

    NULL, /* flush */
    NULL, /* mark_dirty_rectangle */

    /* Here are the drawing functions */
    _cairo_pdf_surface_paint,
    _cairo_pdf_surface_mask,
    _cairo_pdf_surface_stroke,
    _cairo_pdf_surface_fill,
    _cairo_pdf_surface_fill_stroke,
    NULL, /* show_glyphs */
    _cairo_pdf_surface_has_show_text_glyphs,
    _cairo_pdf_surface_show_text_glyphs,
    _cairo_pdf_surface_get_supported_mime_types,
};

static const cairo_paginated_surface_backend_t
cairo_pdf_surface_paginated_backend = {
    _cairo_pdf_surface_start_page,
    _cairo_pdf_surface_set_paginated_mode,
    NULL, /* set_bounding_box */
    _cairo_pdf_surface_has_fallback_images,
    _cairo_pdf_surface_supports_fine_grained_fallbacks,
};
