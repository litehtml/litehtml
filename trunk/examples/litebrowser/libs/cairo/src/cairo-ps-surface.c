/* -*- Mode: c; c-basic-offset: 4; indent-tabs-mode: t; tab-width: 8; -*- */
/* cairo - a vector graphics library with display and print output
 *
 * Copyright © 2003 University of Southern California
 * Copyright © 2005 Red Hat, Inc
 * Copyright © 2007,2008 Adrian Johnson
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
 *	Keith Packard <keithp@keithp.com>
 *	Adrian Johnson <ajohnson@redneon.com>
 */


/*
 * Design of the PS output:
 *
 * The PS output is harmonised with the PDF operations using PS procedures
 * to emulate the PDF operators.
 *
 * This has a number of advantages:
 *   1. A large chunk of code is shared between the PDF and PS backends.
 *      See cairo-pdf-operators.
 *   2. Using gs to do PS -> PDF and PDF -> PS will always work well.
 */

#define _BSD_SOURCE /* for ctime_r(), snprintf(), strdup() */
#include "cairoint.h"

#include "cairo-ps.h"
#include "cairo-ps-surface-private.h"

#include "cairo-pdf-operators-private.h"
#include "cairo-pdf-shading-private.h"

#include "cairo-array-private.h"
#include "cairo-composite-rectangles-private.h"
#include "cairo-default-context-private.h"
#include "cairo-error-private.h"
#include "cairo-image-surface-private.h"
#include "cairo-list-inline.h"
#include "cairo-scaled-font-subsets-private.h"
#include "cairo-paginated-private.h"
#include "cairo-recording-surface-private.h"
#include "cairo-surface-clipper-private.h"
#include "cairo-surface-snapshot-inline.h"
#include "cairo-surface-subsurface-private.h"
#include "cairo-output-stream-private.h"
#include "cairo-type3-glyph-surface-private.h"
#include "cairo-image-info-private.h"

#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <zlib.h>
#include <errno.h>

#define DEBUG_PS 0

#if DEBUG_PS
#define DEBUG_FALLBACK(s) \
    fprintf (stderr, "%s::%d -- %s\n", __FUNCTION__, __LINE__, (s))
#else
#define DEBUG_FALLBACK(s)
#endif

#ifndef HAVE_CTIME_R
#define ctime_r(T, BUF) ctime (T)
#endif

/**
 * SECTION:cairo-ps
 * @Title: PostScript Surfaces
 * @Short_Description: Rendering PostScript documents
 * @See_Also: #cairo_surface_t
 *
 * The PostScript surface is used to render cairo graphics to Adobe
 * PostScript files and is a multi-page vector surface backend.
 **/

/**
 * CAIRO_HAS_PS_SURFACE:
 * 
 * Defined if the PostScript surface backend is available.
 * This macro can be used to conditionally compile backend-specific code.
 *
 * Since: 1.2
 **/

typedef enum {
    CAIRO_PS_COMPRESS_NONE,
    CAIRO_PS_COMPRESS_LZW,
    CAIRO_PS_COMPRESS_DEFLATE
 } cairo_ps_compress_t;

static const cairo_surface_backend_t cairo_ps_surface_backend;
static const cairo_paginated_surface_backend_t cairo_ps_surface_paginated_backend;

static cairo_bool_t
_cairo_ps_surface_get_extents (void		       *abstract_surface,
			       cairo_rectangle_int_t   *rectangle);

static const cairo_ps_level_t _cairo_ps_levels[] =
{
    CAIRO_PS_LEVEL_2,
    CAIRO_PS_LEVEL_3
};

#define CAIRO_PS_LEVEL_LAST ARRAY_LENGTH (_cairo_ps_levels)

static const char * _cairo_ps_level_strings[CAIRO_PS_LEVEL_LAST] =
{
    "PS Level 2",
    "PS Level 3"
};

static const char *_cairo_ps_supported_mime_types[] =
{
    CAIRO_MIME_TYPE_JPEG,
    NULL
};

typedef struct _cairo_page_standard_media {
    const char *name;
    int width;
    int height;
} cairo_page_standard_media_t;

static const cairo_page_standard_media_t _cairo_page_standard_media[] =
{
    { "A0",       2384, 3371 },
    { "A1",       1685, 2384 },
    { "A2",       1190, 1684 },
    { "A3",        842, 1190 },
    { "A4",        595,  842 },
    { "A5",        420,  595 },
    { "B4",        729, 1032 },
    { "B5",        516,  729 },
    { "Letter",    612,  792 },
    { "Tabloid",   792, 1224 },
    { "Ledger",   1224,  792 },
    { "Legal",     612, 1008 },
    { "Statement", 396,  612 },
    { "Executive", 540,  720 },
    { "Folio",     612,  936 },
    { "Quarto",    610,  780 },
    { "10x14",     720, 1008 },
};

typedef struct _cairo_page_media {
    char *name;
    int width;
    int height;
    cairo_list_t link;
} cairo_page_media_t;

static void
_cairo_ps_surface_emit_header (cairo_ps_surface_t *surface)
{
    char ctime_buf[26];
    time_t now;
    char **comments;
    int i, num_comments;
    int level;
    const char *eps_header = "";
    cairo_bool_t has_bbox;

    if (surface->has_creation_date)
	now = surface->creation_date;
    else
	now = time (NULL);

    if (surface->ps_level_used == CAIRO_PS_LEVEL_2)
	level = 2;
    else
	level = 3;

    if (surface->eps)
	eps_header = " EPSF-3.0";

    _cairo_output_stream_printf (surface->final_stream,
				 "%%!PS-Adobe-3.0%s\n"
				 "%%%%Creator: cairo %s (http://cairographics.org)\n"
				 "%%%%CreationDate: %s"
				 "%%%%Pages: %d\n",
				 eps_header,
				 cairo_version_string (),
				 ctime_r (&now, ctime_buf),
				 surface->num_pages);

    _cairo_output_stream_printf (surface->final_stream,
				 "%%%%DocumentData: Clean7Bit\n"
				 "%%%%LanguageLevel: %d\n",
				 level);

    if (!cairo_list_is_empty (&surface->document_media)) {
	cairo_page_media_t *page;
	cairo_bool_t first = TRUE;

	cairo_list_foreach_entry (page, cairo_page_media_t, &surface->document_media, link) {
	    if (first) {
		_cairo_output_stream_printf (surface->final_stream,
					     "%%%%DocumentMedia: ");
		first = FALSE;
	    } else {
		_cairo_output_stream_printf (surface->final_stream,
					     "%%%%+ ");
	    }
	    _cairo_output_stream_printf (surface->final_stream,
					 "%s %d %d 0 () ()\n",
					 page->name,
					 page->width,
					 page->height);
	}
    }

    has_bbox = FALSE;
    num_comments = _cairo_array_num_elements (&surface->dsc_header_comments);
    comments = _cairo_array_index (&surface->dsc_header_comments, 0);
    for (i = 0; i < num_comments; i++) {
	_cairo_output_stream_printf (surface->final_stream,
				     "%s\n", comments[i]);
	if (strncmp (comments[i], "%%BoundingBox:", 14) == 0)
	    has_bbox = TRUE;

	free (comments[i]);
	comments[i] = NULL;
    }

    if (!has_bbox) {
	_cairo_output_stream_printf (surface->final_stream,
				     "%%%%BoundingBox: %d %d %d %d\n",
				     surface->bbox_x1,
				     surface->bbox_y1,
				     surface->bbox_x2,
				     surface->bbox_y2);
    }

    _cairo_output_stream_printf (surface->final_stream,
				 "%%%%EndComments\n");

    _cairo_output_stream_printf (surface->final_stream,
				 "%%%%BeginProlog\n");

    if (surface->eps) {
	_cairo_output_stream_printf (surface->final_stream,
				     "save\n"
				     "50 dict begin\n");
    } else {
	_cairo_output_stream_printf (surface->final_stream,
				     "/languagelevel where\n"
				     "{ pop languagelevel } { 1 } ifelse\n"
				     "%d lt { /Helvetica findfont 12 scalefont setfont 50 500 moveto\n"
				     "  (This print job requires a PostScript Language Level %d printer.) show\n"
				     "  showpage quit } if\n",
				     level,
				     level);
    }

    _cairo_output_stream_printf (surface->final_stream,
				 "/q { gsave } bind def\n"
				 "/Q { grestore } bind def\n"
				 "/cm { 6 array astore concat } bind def\n"
				 "/w { setlinewidth } bind def\n"
				 "/J { setlinecap } bind def\n"
				 "/j { setlinejoin } bind def\n"
				 "/M { setmiterlimit } bind def\n"
				 "/d { setdash } bind def\n"
				 "/m { moveto } bind def\n"
				 "/l { lineto } bind def\n"
				 "/c { curveto } bind def\n"
				 "/h { closepath } bind def\n"
				 "/re { exch dup neg 3 1 roll 5 3 roll moveto 0 rlineto\n"
				 "      0 exch rlineto 0 rlineto closepath } bind def\n"
				 "/S { stroke } bind def\n"
				 "/f { fill } bind def\n"
				 "/f* { eofill } bind def\n"
				 "/n { newpath } bind def\n"
				 "/W { clip } bind def\n"
				 "/W* { eoclip } bind def\n"
				 "/BT { } bind def\n"
				 "/ET { } bind def\n"
				 "/pdfmark where { pop globaldict /?pdfmark /exec load put }\n"
				 "    { globaldict begin /?pdfmark /pop load def /pdfmark\n"
				 "    /cleartomark load def end } ifelse\n"
				 "/BDC { mark 3 1 roll /BDC pdfmark } bind def\n"
				 "/EMC { mark /EMC pdfmark } bind def\n"
				 "/cairo_store_point { /cairo_point_y exch def /cairo_point_x exch def } def\n"
				 "/Tj { show currentpoint cairo_store_point } bind def\n"
				 "/TJ {\n"
				 "  {\n"
				 "    dup\n"
				 "    type /stringtype eq\n"
				 "    { show } { -0.001 mul 0 cairo_font_matrix dtransform rmoveto } ifelse\n"
				 "  } forall\n"
				 "  currentpoint cairo_store_point\n"
				 "} bind def\n"
				 "/cairo_selectfont { cairo_font_matrix aload pop pop pop 0 0 6 array astore\n"
				 "    cairo_font exch selectfont cairo_point_x cairo_point_y moveto } bind def\n"
				 "/Tf { pop /cairo_font exch def /cairo_font_matrix where\n"
				 "      { pop cairo_selectfont } if } bind def\n"
				 "/Td { matrix translate cairo_font_matrix matrix concatmatrix dup\n"
				 "      /cairo_font_matrix exch def dup 4 get exch 5 get cairo_store_point\n"
				 "      /cairo_font where { pop cairo_selectfont } if } bind def\n"
				 "/Tm { 2 copy 8 2 roll 6 array astore /cairo_font_matrix exch def\n"
				 "      cairo_store_point /cairo_font where { pop cairo_selectfont } if } bind def\n"
				 "/g { setgray } bind def\n"
				 "/rg { setrgbcolor } bind def\n"
				 "/d1 { setcachedevice } bind def\n");

    _cairo_output_stream_printf (surface->final_stream,
				 "%%%%EndProlog\n");

    num_comments = _cairo_array_num_elements (&surface->dsc_setup_comments);
    if (num_comments) {
	_cairo_output_stream_printf (surface->final_stream,
				     "%%%%BeginSetup\n");

	comments = _cairo_array_index (&surface->dsc_setup_comments, 0);
	for (i = 0; i < num_comments; i++) {
	    _cairo_output_stream_printf (surface->final_stream,
					 "%s\n", comments[i]);
	    free (comments[i]);
	    comments[i] = NULL;
	}

	_cairo_output_stream_printf (surface->final_stream,
				     "%%%%EndSetup\n");
    }
}

static cairo_status_t
_cairo_ps_surface_emit_type1_font_subset (cairo_ps_surface_t		*surface,
					  cairo_scaled_font_subset_t	*font_subset)


{
    cairo_type1_subset_t subset;
    cairo_status_t status;
    int length;
    char name[64];

    snprintf (name, sizeof name, "f-%d-%d",
	      font_subset->font_id, font_subset->subset_id);
    status = _cairo_type1_subset_init (&subset, name, font_subset, TRUE);
    if (unlikely (status))
	return status;

    /* FIXME: Figure out document structure convention for fonts */

#if DEBUG_PS
    _cairo_output_stream_printf (surface->final_stream,
				 "%% _cairo_ps_surface_emit_type1_font_subset\n");
#endif

    length = subset.header_length + subset.data_length + subset.trailer_length;
    _cairo_output_stream_write (surface->final_stream, subset.data, length);

    _cairo_type1_subset_fini (&subset);

    return CAIRO_STATUS_SUCCESS;
}


static cairo_status_t
_cairo_ps_surface_emit_type1_font_fallback (cairo_ps_surface_t		*surface,
                                            cairo_scaled_font_subset_t	*font_subset)
{
    cairo_type1_subset_t subset;
    cairo_status_t status;
    int length;
    char name[64];

    snprintf (name, sizeof name, "f-%d-%d",
	      font_subset->font_id, font_subset->subset_id);
    status = _cairo_type1_fallback_init_hex (&subset, name, font_subset);
    if (unlikely (status))
	return status;

    /* FIXME: Figure out document structure convention for fonts */

#if DEBUG_PS
    _cairo_output_stream_printf (surface->final_stream,
				 "%% _cairo_ps_surface_emit_type1_font_fallback\n");
#endif

    length = subset.header_length + subset.data_length + subset.trailer_length;
    _cairo_output_stream_write (surface->final_stream, subset.data, length);

    _cairo_type1_fallback_fini (&subset);

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_ps_surface_emit_truetype_font_subset (cairo_ps_surface_t		*surface,
					     cairo_scaled_font_subset_t	*font_subset)


{
    cairo_truetype_subset_t subset;
    cairo_status_t status;
    unsigned int i, begin, end;

    status = _cairo_truetype_subset_init_ps (&subset, font_subset);
    if (unlikely (status))
	return status;

    /* FIXME: Figure out document structure convention for fonts */

#if DEBUG_PS
    _cairo_output_stream_printf (surface->final_stream,
				 "%% _cairo_ps_surface_emit_truetype_font_subset\n");
#endif

    _cairo_output_stream_printf (surface->final_stream,
				 "11 dict begin\n"
				 "/FontType 42 def\n"
				 "/FontName /%s def\n"
				 "/PaintType 0 def\n"
				 "/FontMatrix [ 1 0 0 1 0 0 ] def\n"
				 "/FontBBox [ 0 0 0 0 ] def\n"
				 "/Encoding 256 array def\n"
				 "0 1 255 { Encoding exch /.notdef put } for\n",
				 subset.ps_name);

    /* FIXME: Figure out how subset->x_max etc maps to the /FontBBox */

    if (font_subset->is_latin) {
	for (i = 1; i < 256; i++) {
	    if (font_subset->latin_to_subset_glyph_index[i] > 0) {
		if (font_subset->glyph_names != NULL) {
		    _cairo_output_stream_printf (surface->final_stream,
						 "Encoding %d /%s put\n",
						 i, font_subset->glyph_names[font_subset->latin_to_subset_glyph_index[i]]);
		} else {
		    _cairo_output_stream_printf (surface->final_stream,
						 "Encoding %d /g%ld put\n", i, font_subset->latin_to_subset_glyph_index[i]);
		}
	    }
	}
    } else {
	for (i = 1; i < font_subset->num_glyphs; i++) {
	    if (font_subset->glyph_names != NULL) {
		_cairo_output_stream_printf (surface->final_stream,
					     "Encoding %d /%s put\n",
					     i, font_subset->glyph_names[i]);
	    } else {
		_cairo_output_stream_printf (surface->final_stream,
					     "Encoding %d /g%d put\n", i, i);
	    }
	}
    }

    _cairo_output_stream_printf (surface->final_stream,
				 "/CharStrings %d dict dup begin\n"
				 "/.notdef 0 def\n",
				 font_subset->num_glyphs);

    for (i = 1; i < font_subset->num_glyphs; i++) {
	if (font_subset->glyph_names != NULL) {
	    _cairo_output_stream_printf (surface->final_stream,
					 "/%s %d def\n",
					 font_subset->glyph_names[i], i);
	} else {
	    _cairo_output_stream_printf (surface->final_stream,
					 "/g%d %d def\n", i, i);
	}
    }

    _cairo_output_stream_printf (surface->final_stream,
				 "end readonly def\n");

    _cairo_output_stream_printf (surface->final_stream,
				 "/sfnts [\n");
    begin = 0;
    end = 0;
    for (i = 0; i < subset.num_string_offsets; i++) {
        end = subset.string_offsets[i];
        _cairo_output_stream_printf (surface->final_stream,"<");
        _cairo_output_stream_write_hex_string (surface->final_stream,
                                               subset.data + begin, end - begin);
        _cairo_output_stream_printf (surface->final_stream,"00>\n");
        begin = end;
    }
    if (subset.data_length > end) {
        _cairo_output_stream_printf (surface->final_stream,"<");
        _cairo_output_stream_write_hex_string (surface->final_stream,
                                               subset.data + end, subset.data_length - end);
        _cairo_output_stream_printf (surface->final_stream,"00>\n");
    }

    _cairo_output_stream_printf (surface->final_stream,
				 "] def\n"
				 "/f-%d-%d currentdict end definefont pop\n",
				 font_subset->font_id,
				 font_subset->subset_id);

    _cairo_truetype_subset_fini (&subset);

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_ps_emit_imagemask (cairo_image_surface_t *image,
			  cairo_output_stream_t *stream)
{
    uint8_t *row, *byte;
    int rows, cols;

    /* The only image type supported by Type 3 fonts are 1-bit image
     * masks */
    assert (image->format == CAIRO_FORMAT_A1);

    _cairo_output_stream_printf (stream,
				 "<<\n"
				 "   /ImageType 1\n"
				 "   /Width %d\n"
				 "   /Height %d\n"
				 "   /ImageMatrix [%d 0 0 %d 0 %d]\n"
				 "   /Decode [1 0]\n"
				 "   /BitsPerComponent 1\n",
				 image->width,
				 image->height,
				 image->width,
				 -image->height,
				 image->height);

    _cairo_output_stream_printf (stream,
				 "   /DataSource {<\n   ");
    for (row = image->data, rows = image->height; rows; row += image->stride, rows--) {
	for (byte = row, cols = (image->width + 7) / 8; cols; byte++, cols--) {
	    uint8_t output_byte = CAIRO_BITSWAP8_IF_LITTLE_ENDIAN (*byte);
	    _cairo_output_stream_printf (stream, "%02x ", output_byte);
	}
	_cairo_output_stream_printf (stream, "\n   ");
    }
    _cairo_output_stream_printf (stream, ">}\n>>\n");

    _cairo_output_stream_printf (stream,
				 "imagemask\n");

    return _cairo_output_stream_get_status (stream);
}

static cairo_int_status_t
_cairo_ps_surface_analyze_user_font_subset (cairo_scaled_font_subset_t *font_subset,
					    void		       *closure)
{
    cairo_ps_surface_t *surface = closure;
    cairo_status_t status = CAIRO_STATUS_SUCCESS;
    unsigned int i;
    cairo_surface_t *type3_surface;

    type3_surface = _cairo_type3_glyph_surface_create (font_subset->scaled_font,
						       NULL,
						       _cairo_ps_emit_imagemask,
						       surface->font_subsets);

    for (i = 0; i < font_subset->num_glyphs; i++) {
	status = _cairo_type3_glyph_surface_analyze_glyph (type3_surface,
							   font_subset->glyphs[i]);
	if (unlikely (status))
	    break;

    }
    cairo_surface_finish (type3_surface);
    cairo_surface_destroy (type3_surface);

    return status;
}

static cairo_status_t
_cairo_ps_surface_emit_type3_font_subset (cairo_ps_surface_t		*surface,
					  cairo_scaled_font_subset_t	*font_subset)


{
    cairo_status_t status;
    unsigned int i;
    cairo_box_t font_bbox = {{0,0},{0,0}};
    cairo_box_t bbox = {{0,0},{0,0}};
    cairo_surface_t *type3_surface;
    double width;

    if (font_subset->num_glyphs == 0)
	return CAIRO_STATUS_SUCCESS;

#if DEBUG_PS
    _cairo_output_stream_printf (surface->final_stream,
				 "%% _cairo_ps_surface_emit_type3_font_subset\n");
#endif

    _cairo_output_stream_printf (surface->final_stream,
				 "8 dict begin\n"
				 "/FontType 3 def\n"
				 "/FontMatrix [1 0 0 1 0 0] def\n"
				 "/Encoding 256 array def\n"
				 "0 1 255 { Encoding exch /.notdef put } for\n");

    type3_surface = _cairo_type3_glyph_surface_create (font_subset->scaled_font,
						       NULL,
						       _cairo_ps_emit_imagemask,
						       surface->font_subsets);
    status = type3_surface->status;
    if (unlikely (status))
	return status;

    for (i = 0; i < font_subset->num_glyphs; i++) {
	if (font_subset->glyph_names != NULL) {
	    _cairo_output_stream_printf (surface->final_stream,
					 "Encoding %d /%s put\n",
					 i, font_subset->glyph_names[i]);
	} else {
	    _cairo_output_stream_printf (surface->final_stream,
					 "Encoding %d /g%d put\n", i, i);
	}
    }

    _cairo_output_stream_printf (surface->final_stream,
				 "/Glyphs [\n");

    for (i = 0; i < font_subset->num_glyphs; i++) {
	_cairo_output_stream_printf (surface->final_stream,
				     "    { %% %d\n", i);
	status = _cairo_type3_glyph_surface_emit_glyph (type3_surface,
							surface->final_stream,
							font_subset->glyphs[i],
							&bbox,
							&width);
	if (unlikely (status))
	    break;

	_cairo_output_stream_printf (surface->final_stream,
				     "    }\n");
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
    cairo_surface_finish (type3_surface);
    cairo_surface_destroy (type3_surface);
    if (unlikely (status))
	return status;

    _cairo_output_stream_printf (surface->final_stream,
				 "] def\n"
				 "/FontBBox [%f %f %f %f] def\n"
				 "/BuildChar {\n"
				 "  exch /Glyphs get\n"
				 "  exch get\n"
				 "  10 dict begin exec end\n"
				 "} bind def\n"
				 "currentdict\n"
				 "end\n"
				 "/f-%d-%d exch definefont pop\n",
				 _cairo_fixed_to_double (font_bbox.p1.x),
				 - _cairo_fixed_to_double (font_bbox.p2.y),
				 _cairo_fixed_to_double (font_bbox.p2.x),
				 - _cairo_fixed_to_double (font_bbox.p1.y),
				 font_subset->font_id,
				 font_subset->subset_id);

    return CAIRO_STATUS_SUCCESS;
}

static cairo_int_status_t
_cairo_ps_surface_emit_unscaled_font_subset (cairo_scaled_font_subset_t	*font_subset,
				            void			*closure)
{
    cairo_ps_surface_t *surface = closure;
    cairo_int_status_t status;

    status = _cairo_scaled_font_subset_create_glyph_names (font_subset);
    if (_cairo_int_status_is_error (status))
	return status;

    status = _cairo_ps_surface_emit_type1_font_subset (surface, font_subset);
    if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	return status;

    status = _cairo_ps_surface_emit_truetype_font_subset (surface, font_subset);
    if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	return status;

    status = _cairo_ps_surface_emit_type1_font_fallback (surface, font_subset);
    if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	return status;

    ASSERT_NOT_REACHED;
    return CAIRO_STATUS_SUCCESS;
}

static cairo_int_status_t
_cairo_ps_surface_emit_scaled_font_subset (cairo_scaled_font_subset_t *font_subset,
                                           void			      *closure)
{
    cairo_ps_surface_t *surface = closure;
    cairo_int_status_t status;

    status = _cairo_scaled_font_subset_create_glyph_names (font_subset);
    if (_cairo_int_status_is_error (status))
	return status;

    status = _cairo_ps_surface_emit_type3_font_subset (surface, font_subset);
    if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	return status;

    ASSERT_NOT_REACHED;
    return CAIRO_INT_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_ps_surface_emit_font_subsets (cairo_ps_surface_t *surface)
{
    cairo_status_t status;

#if DEBUG_PS
    _cairo_output_stream_printf (surface->final_stream,
				 "%% _cairo_ps_surface_emit_font_subsets\n");
#endif

    status = _cairo_scaled_font_subsets_foreach_user (surface->font_subsets,
						      _cairo_ps_surface_analyze_user_font_subset,
						      surface);
    if (unlikely (status))
	return status;

    status = _cairo_scaled_font_subsets_foreach_unscaled (surface->font_subsets,
                                                          _cairo_ps_surface_emit_unscaled_font_subset,
                                                          surface);
    if (unlikely (status))
	return status;

    status = _cairo_scaled_font_subsets_foreach_scaled (surface->font_subsets,
                                                        _cairo_ps_surface_emit_scaled_font_subset,
                                                        surface);
    if (unlikely (status))
	return status;

    return _cairo_scaled_font_subsets_foreach_user (surface->font_subsets,
						    _cairo_ps_surface_emit_scaled_font_subset,
						    surface);
}

static cairo_status_t
_cairo_ps_surface_emit_body (cairo_ps_surface_t *surface)
{
    char    buf[4096];
    int	    n;

    if (ferror (surface->tmpfile) != 0)
	return _cairo_error (CAIRO_STATUS_TEMP_FILE_ERROR);

    rewind (surface->tmpfile);
    while ((n = fread (buf, 1, sizeof (buf), surface->tmpfile)) > 0)
	_cairo_output_stream_write (surface->final_stream, buf, n);

    if (ferror (surface->tmpfile) != 0)
	return _cairo_error (CAIRO_STATUS_TEMP_FILE_ERROR);

    return CAIRO_STATUS_SUCCESS;
}

static void
_cairo_ps_surface_emit_footer (cairo_ps_surface_t *surface)
{
    _cairo_output_stream_printf (surface->final_stream,
				 "%%%%Trailer\n");

    if (surface->eps) {
	_cairo_output_stream_printf (surface->final_stream,
				     "end restore\n");
    }

    _cairo_output_stream_printf (surface->final_stream,
				 "%%%%EOF\n");
}

static cairo_bool_t
_path_covers_bbox (cairo_ps_surface_t *surface,
		   cairo_path_fixed_t *path)
{
    cairo_box_t box;

    if (_cairo_path_fixed_is_box (path, &box)) {
	cairo_rectangle_int_t rect;

	_cairo_box_round_to_rectangle (&box, &rect);

	/* skip trivial whole-page clips */
	if (_cairo_rectangle_intersect (&rect, &surface->page_bbox)) {
	    if (rect.x == surface->page_bbox.x &&
		rect.width == surface->page_bbox.width &&
		rect.y == surface->page_bbox.y &&
		rect.height == surface->page_bbox.height)
	    {
		return TRUE;
	    }
	}
    }

    return FALSE;
}

static cairo_status_t
_cairo_ps_surface_clipper_intersect_clip_path (cairo_surface_clipper_t *clipper,
					       cairo_path_fixed_t *path,
					       cairo_fill_rule_t   fill_rule,
					       double		    tolerance,
					       cairo_antialias_t   antialias)
{
    cairo_ps_surface_t *surface = cairo_container_of (clipper,
						      cairo_ps_surface_t,
						      clipper);
    cairo_output_stream_t *stream = surface->stream;
    cairo_status_t status;

    assert (surface->paginated_mode != CAIRO_PAGINATED_MODE_ANALYZE);

#if DEBUG_PS
    _cairo_output_stream_printf (stream,
				 "%% _cairo_ps_surface_intersect_clip_path\n");
#endif

    if (path == NULL) {
	status = _cairo_pdf_operators_flush (&surface->pdf_operators);
	if (unlikely (status))
	    return status;

	_cairo_output_stream_printf (stream, "Q q\n");

	surface->current_pattern_is_solid_color = FALSE;
	_cairo_pdf_operators_reset (&surface->pdf_operators);

	return CAIRO_STATUS_SUCCESS;
    }

    if (_path_covers_bbox (surface, path))
	return CAIRO_STATUS_SUCCESS;

    return _cairo_pdf_operators_clip (&surface->pdf_operators,
				      path,
				      fill_rule);
}

/* PLRM specifies a tolerance of 5 points when matching page sizes */
static cairo_bool_t
_ps_page_dimension_equal (int a, int b)
{
    return (abs (a - b) < 5);
}

static const char *
_cairo_ps_surface_get_page_media (cairo_ps_surface_t     *surface)
{
    int width, height, i;
    char buf[50];
    cairo_page_media_t *page;
    const char *page_name;

    width = _cairo_lround (surface->width);
    height = _cairo_lround (surface->height);

    /* search previously used page sizes */
    cairo_list_foreach_entry (page, cairo_page_media_t, &surface->document_media, link) {
	if (_ps_page_dimension_equal (width, page->width) &&
	    _ps_page_dimension_equal (height, page->height))
	    return page->name;
    }

    /* search list of standard page sizes */
    page_name = NULL;
    for (i = 0; i < ARRAY_LENGTH (_cairo_page_standard_media); i++) {
	if (_ps_page_dimension_equal (width, _cairo_page_standard_media[i].width) &&
	    _ps_page_dimension_equal (height, _cairo_page_standard_media[i].height))
	{
	    page_name = _cairo_page_standard_media[i].name;
	    width = _cairo_page_standard_media[i].width;
	    height = _cairo_page_standard_media[i].height;
	    break;
	}
    }

    page = malloc (sizeof (cairo_page_media_t));
    if (unlikely (page == NULL)) {
	_cairo_error_throw (CAIRO_STATUS_NO_MEMORY);
	return NULL;
    }

    if (page_name) {
	page->name = strdup (page_name);
    } else {
	snprintf (buf, sizeof (buf), "%dx%dmm",
		  (int) _cairo_lround (surface->width * 25.4/72),
		  (int) _cairo_lround (surface->height * 25.4/72));
	page->name = strdup (buf);
    }

    if (unlikely (page->name == NULL)) {
	free (page);
	_cairo_error_throw (CAIRO_STATUS_NO_MEMORY);
	return NULL;
    }

    page->width = width;
    page->height = height;
    cairo_list_add_tail (&page->link, &surface->document_media);

    return page->name;
}

static cairo_surface_t *
_cairo_ps_surface_create_for_stream_internal (cairo_output_stream_t *stream,
					      double		     width,
					      double		     height)
{
    cairo_status_t status, status_ignored;
    cairo_ps_surface_t *surface;

    surface = malloc (sizeof (cairo_ps_surface_t));
    if (unlikely (surface == NULL)) {
	status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	goto CLEANUP;
    }

    _cairo_surface_init (&surface->base,
			 &cairo_ps_surface_backend,
			 NULL, /* device */
			 CAIRO_CONTENT_COLOR_ALPHA);

    surface->final_stream = stream;

    surface->tmpfile = tmpfile ();
    if (surface->tmpfile == NULL) {
	switch (errno) {
	case ENOMEM:
	    status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	    break;
	default:
	    status = _cairo_error (CAIRO_STATUS_TEMP_FILE_ERROR);
	    break;
	}
	goto CLEANUP_SURFACE;
    }

    surface->stream = _cairo_output_stream_create_for_file (surface->tmpfile);
    status = _cairo_output_stream_get_status (surface->stream);
    if (unlikely (status))
	goto CLEANUP_OUTPUT_STREAM;

    surface->font_subsets = _cairo_scaled_font_subsets_create_simple ();
    if (unlikely (surface->font_subsets == NULL)) {
	status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	goto CLEANUP_OUTPUT_STREAM;
    }

    _cairo_scaled_font_subsets_enable_latin_subset (surface->font_subsets, TRUE);
    surface->has_creation_date = FALSE;
    surface->eps = FALSE;
    surface->ps_level = CAIRO_PS_LEVEL_3;
    surface->ps_level_used = CAIRO_PS_LEVEL_2;
    surface->width  = width;
    surface->height = height;
    cairo_matrix_init (&surface->cairo_to_ps, 1, 0, 0, -1, 0, height);
    surface->paginated_mode = CAIRO_PAGINATED_MODE_ANALYZE;
    surface->force_fallbacks = FALSE;
    surface->content = CAIRO_CONTENT_COLOR_ALPHA;
    surface->use_string_datasource = FALSE;
    surface->current_pattern_is_solid_color = FALSE;

    surface->page_bbox.x = 0;
    surface->page_bbox.y = 0;
    surface->page_bbox.width  = width;
    surface->page_bbox.height = height;

    _cairo_surface_clipper_init (&surface->clipper,
				 _cairo_ps_surface_clipper_intersect_clip_path);

    _cairo_pdf_operators_init (&surface->pdf_operators,
			       surface->stream,
			       &surface->cairo_to_ps,
			       surface->font_subsets);
    surface->num_pages = 0;

    cairo_list_init (&surface->document_media);
    _cairo_array_init (&surface->dsc_header_comments, sizeof (char *));
    _cairo_array_init (&surface->dsc_setup_comments, sizeof (char *));
    _cairo_array_init (&surface->dsc_page_setup_comments, sizeof (char *));

    surface->dsc_comment_target = &surface->dsc_header_comments;

    surface->paginated_surface = _cairo_paginated_surface_create (
	                                   &surface->base,
					   CAIRO_CONTENT_COLOR_ALPHA,
					   &cairo_ps_surface_paginated_backend);
    status = surface->paginated_surface->status;
    if (status == CAIRO_STATUS_SUCCESS) {
	/* paginated keeps the only reference to surface now, drop ours */
	cairo_surface_destroy (&surface->base);
	return surface->paginated_surface;
    }

    _cairo_scaled_font_subsets_destroy (surface->font_subsets);
 CLEANUP_OUTPUT_STREAM:
    status_ignored = _cairo_output_stream_destroy (surface->stream);
    fclose (surface->tmpfile);
 CLEANUP_SURFACE:
    free (surface);
 CLEANUP:
    /* destroy stream on behalf of caller */
    status_ignored = _cairo_output_stream_destroy (stream);

    return _cairo_surface_create_in_error (status);
}

/**
 * cairo_ps_surface_create:
 * @filename: a filename for the PS output (must be writable), %NULL may be
 *            used to specify no output. This will generate a PS surface that
 *            may be queried and used as a source, without generating a
 *            temporary file.
 * @width_in_points: width of the surface, in points (1 point == 1/72.0 inch)
 * @height_in_points: height of the surface, in points (1 point == 1/72.0 inch)
 *
 * Creates a PostScript surface of the specified size in points to be
 * written to @filename. See cairo_ps_surface_create_for_stream() for
 * a more flexible mechanism for handling the PostScript output than
 * simply writing it to a named file.
 *
 * Note that the size of individual pages of the PostScript output can
 * vary. See cairo_ps_surface_set_size().
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
cairo_ps_surface_create (const char		*filename,
			 double			 width_in_points,
			 double			 height_in_points)
{
    cairo_output_stream_t *stream;

    stream = _cairo_output_stream_create_for_filename (filename);
    if (_cairo_output_stream_get_status (stream))
	return _cairo_surface_create_in_error (_cairo_output_stream_destroy (stream));

    return _cairo_ps_surface_create_for_stream_internal (stream,
							 width_in_points,
							 height_in_points);
}

/**
 * cairo_ps_surface_create_for_stream:
 * @write_func: a #cairo_write_func_t to accept the output data, may be %NULL
 *              to indicate a no-op @write_func. With a no-op @write_func,
 *              the surface may be queried or used as a source without
 *              generating any temporary files.
 * @closure: the closure argument for @write_func
 * @width_in_points: width of the surface, in points (1 point == 1/72.0 inch)
 * @height_in_points: height of the surface, in points (1 point == 1/72.0 inch)
 *
 * Creates a PostScript surface of the specified size in points to be
 * written incrementally to the stream represented by @write_func and
 * @closure. See cairo_ps_surface_create() for a more convenient way
 * to simply direct the PostScript output to a named file.
 *
 * Note that the size of individual pages of the PostScript
 * output can vary. See cairo_ps_surface_set_size().
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
cairo_ps_surface_create_for_stream (cairo_write_func_t	write_func,
				    void	       *closure,
				    double		width_in_points,
				    double		height_in_points)
{
    cairo_output_stream_t *stream;

    stream = _cairo_output_stream_create (write_func, NULL, closure);
    if (_cairo_output_stream_get_status (stream))
	return _cairo_surface_create_in_error (_cairo_output_stream_destroy (stream));

    return _cairo_ps_surface_create_for_stream_internal (stream,
							 width_in_points,
							 height_in_points);
}

static cairo_bool_t
_cairo_surface_is_ps (cairo_surface_t *surface)
{
    return surface->backend == &cairo_ps_surface_backend;
}

/* If the abstract_surface is a paginated surface, and that paginated
 * surface's target is a ps_surface, then set ps_surface to that
 * target. Otherwise return FALSE.
 */
static cairo_bool_t
_extract_ps_surface (cairo_surface_t	 *surface,
                     cairo_bool_t         set_error_on_failure,
		     cairo_ps_surface_t **ps_surface)
{
    cairo_surface_t *target;

    if (surface->status)
	return FALSE;
    if (surface->finished) {
        if (set_error_on_failure)
	    _cairo_surface_set_error (surface,
				      _cairo_error (CAIRO_STATUS_SURFACE_FINISHED));
	return FALSE;
    }

    if (! _cairo_surface_is_paginated (surface)) {
        if (set_error_on_failure)
	    _cairo_surface_set_error (surface,
				      _cairo_error (CAIRO_STATUS_SURFACE_TYPE_MISMATCH));
	return FALSE;
    }

    target = _cairo_paginated_surface_get_target (surface);
    if (target->status) {
        if (set_error_on_failure)
	    _cairo_surface_set_error (surface, target->status);
	return FALSE;
    }
    if (target->finished) {
        if (set_error_on_failure)
	    _cairo_surface_set_error (surface,
				      _cairo_error (CAIRO_STATUS_SURFACE_FINISHED));
	return FALSE;
    }

    if (! _cairo_surface_is_ps (target)) {
        if (set_error_on_failure)
	    _cairo_surface_set_error (surface,
				      _cairo_error (CAIRO_STATUS_SURFACE_TYPE_MISMATCH));
	return FALSE;
    }

    *ps_surface = (cairo_ps_surface_t *) target;
    return TRUE;
}

/**
 * cairo_ps_surface_restrict_to_level:
 * @surface: a PostScript #cairo_surface_t
 * @level: PostScript level
 *
 * Restricts the generated PostSript file to @level. See
 * cairo_ps_get_levels() for a list of available level values that
 * can be used here.
 *
 * This function should only be called before any drawing operations
 * have been performed on the given surface. The simplest way to do
 * this is to call this function immediately after creating the
 * surface.
 *
 * Since: 1.6
 **/
void
cairo_ps_surface_restrict_to_level (cairo_surface_t  *surface,
                                    cairo_ps_level_t  level)
{
    cairo_ps_surface_t *ps_surface = NULL;

    if (! _extract_ps_surface (surface, TRUE, &ps_surface))
	return;

    if (level < CAIRO_PS_LEVEL_LAST)
	ps_surface->ps_level = level;
}

/**
 * cairo_ps_get_levels:
 * @levels: supported level list
 * @num_levels: list length
 *
 * Used to retrieve the list of supported levels. See
 * cairo_ps_surface_restrict_to_level().
 *
 * Since: 1.6
 **/
void
cairo_ps_get_levels (cairo_ps_level_t const	**levels,
                     int                     	 *num_levels)
{
    if (levels != NULL)
	*levels = _cairo_ps_levels;

    if (num_levels != NULL)
	*num_levels = CAIRO_PS_LEVEL_LAST;
}

/**
 * cairo_ps_level_to_string:
 * @level: a level id
 *
 * Get the string representation of the given @level id. This function
 * will return %NULL if @level id isn't valid. See cairo_ps_get_levels()
 * for a way to get the list of valid level ids.
 *
 * Return value: the string associated to given level.
 *
 * Since: 1.6
 **/
const char *
cairo_ps_level_to_string (cairo_ps_level_t level)
{
    if (level >= CAIRO_PS_LEVEL_LAST)
	return NULL;

    return _cairo_ps_level_strings[level];
}

/**
 * cairo_ps_surface_set_eps:
 * @surface: a PostScript #cairo_surface_t
 * @eps: %TRUE to output EPS format PostScript
 *
 * If @eps is %TRUE, the PostScript surface will output Encapsulated
 * PostScript.
 *
 * This function should only be called before any drawing operations
 * have been performed on the current page. The simplest way to do
 * this is to call this function immediately after creating the
 * surface. An Encapsulated PostScript file should never contain more
 * than one page.
 *
 * Since: 1.6
 **/
void
cairo_ps_surface_set_eps (cairo_surface_t	*surface,
			  cairo_bool_t           eps)
{
    cairo_ps_surface_t *ps_surface = NULL;

    if (! _extract_ps_surface (surface, TRUE, &ps_surface))
	return;

    ps_surface->eps = eps;
}

/**
 * cairo_ps_surface_get_eps:
 * @surface: a PostScript #cairo_surface_t
 *
 * Check whether the PostScript surface will output Encapsulated PostScript.
 *
 * Return value: %TRUE if the surface will output Encapsulated PostScript.
 *
 * Since: 1.6
 **/
cairo_public cairo_bool_t
cairo_ps_surface_get_eps (cairo_surface_t	*surface)
{
    cairo_ps_surface_t *ps_surface = NULL;

    if (! _extract_ps_surface (surface, FALSE, &ps_surface))
	return FALSE;

    return ps_surface->eps;
}

/**
 * cairo_ps_surface_set_size:
 * @surface: a PostScript #cairo_surface_t
 * @width_in_points: new surface width, in points (1 point == 1/72.0 inch)
 * @height_in_points: new surface height, in points (1 point == 1/72.0 inch)
 *
 * Changes the size of a PostScript surface for the current (and
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
cairo_ps_surface_set_size (cairo_surface_t	*surface,
			   double		 width_in_points,
			   double		 height_in_points)
{
    cairo_ps_surface_t *ps_surface = NULL;
    cairo_status_t status;

    if (! _extract_ps_surface (surface, TRUE, &ps_surface))
	return;

    ps_surface->width = width_in_points;
    ps_surface->height = height_in_points;
    cairo_matrix_init (&ps_surface->cairo_to_ps, 1, 0, 0, -1, 0, height_in_points);
    _cairo_pdf_operators_set_cairo_to_pdf_matrix (&ps_surface->pdf_operators,
						  &ps_surface->cairo_to_ps);
    status = _cairo_paginated_surface_set_size (ps_surface->paginated_surface,
						width_in_points,
						height_in_points);
    if (status)
	status = _cairo_surface_set_error (surface, status);
}

/**
 * cairo_ps_surface_dsc_comment:
 * @surface: a PostScript #cairo_surface_t
 * @comment: a comment string to be emitted into the PostScript output
 *
 * Emit a comment into the PostScript output for the given surface.
 *
 * The comment is expected to conform to the PostScript Language
 * Document Structuring Conventions (DSC). Please see that manual for
 * details on the available comments and their meanings. In
 * particular, the \%\%IncludeFeature comment allows a
 * device-independent means of controlling printer device features. So
 * the PostScript Printer Description Files Specification will also be
 * a useful reference.
 *
 * The comment string must begin with a percent character (\%) and the
 * total length of the string (including any initial percent
 * characters) must not exceed 255 characters. Violating either of
 * these conditions will place @surface into an error state. But
 * beyond these two conditions, this function will not enforce
 * conformance of the comment with any particular specification.
 *
 * The comment string should not have a trailing newline.
 *
 * The DSC specifies different sections in which particular comments
 * can appear. This function provides for comments to be emitted
 * within three sections: the header, the Setup section, and the
 * PageSetup section.  Comments appearing in the first two sections
 * apply to the entire document while comments in the BeginPageSetup
 * section apply only to a single page.
 *
 * For comments to appear in the header section, this function should
 * be called after the surface is created, but before a call to
 * cairo_ps_surface_dsc_begin_setup().
 *
 * For comments to appear in the Setup section, this function should
 * be called after a call to cairo_ps_surface_dsc_begin_setup() but
 * before a call to cairo_ps_surface_dsc_begin_page_setup().
 *
 * For comments to appear in the PageSetup section, this function
 * should be called after a call to
 * cairo_ps_surface_dsc_begin_page_setup().
 *
 * Note that it is only necessary to call
 * cairo_ps_surface_dsc_begin_page_setup() for the first page of any
 * surface. After a call to cairo_show_page() or cairo_copy_page()
 * comments are unambiguously directed to the PageSetup section of the
 * current page. But it doesn't hurt to call this function at the
 * beginning of every page as that consistency may make the calling
 * code simpler.
 *
 * As a final note, cairo automatically generates several comments on
 * its own. As such, applications must not manually generate any of
 * the following comments:
 *
 * Header section: \%!PS-Adobe-3.0, \%\%Creator, \%\%CreationDate, \%\%Pages,
 * \%\%BoundingBox, \%\%DocumentData, \%\%LanguageLevel, \%\%EndComments.
 *
 * Setup section: \%\%BeginSetup, \%\%EndSetup
 *
 * PageSetup section: \%\%BeginPageSetup, \%\%PageBoundingBox, \%\%EndPageSetup.
 *
 * Other sections: \%\%BeginProlog, \%\%EndProlog, \%\%Page, \%\%Trailer, \%\%EOF
 *
 * Here is an example sequence showing how this function might be used:
 *
 * <informalexample><programlisting>
 * cairo_surface_t *surface = cairo_ps_surface_create (filename, width, height);
 * ...
 * cairo_ps_surface_dsc_comment (surface, "%%Title: My excellent document");
 * cairo_ps_surface_dsc_comment (surface, "%%Copyright: Copyright (C) 2006 Cairo Lover")
 * ...
 * cairo_ps_surface_dsc_begin_setup (surface);
 * cairo_ps_surface_dsc_comment (surface, "%%IncludeFeature: *MediaColor White");
 * ...
 * cairo_ps_surface_dsc_begin_page_setup (surface);
 * cairo_ps_surface_dsc_comment (surface, "%%IncludeFeature: *PageSize A3");
 * cairo_ps_surface_dsc_comment (surface, "%%IncludeFeature: *InputSlot LargeCapacity");
 * cairo_ps_surface_dsc_comment (surface, "%%IncludeFeature: *MediaType Glossy");
 * cairo_ps_surface_dsc_comment (surface, "%%IncludeFeature: *MediaColor Blue");
 * ... draw to first page here ..
 * cairo_show_page (cr);
 * ...
 * cairo_ps_surface_dsc_comment (surface, "%%IncludeFeature: *PageSize A5");
 * ...
 * </programlisting></informalexample>
 *
 * Since: 1.2
 **/
void
cairo_ps_surface_dsc_comment (cairo_surface_t	*surface,
			      const char	*comment)
{
    cairo_ps_surface_t *ps_surface = NULL;
    cairo_status_t status;
    char *comment_copy;

    if (! _extract_ps_surface (surface, TRUE, &ps_surface))
	return;

    /* A couple of sanity checks on the comment value. */
    if (comment == NULL) {
	status = _cairo_surface_set_error (surface, CAIRO_STATUS_NULL_POINTER);
	return;
    }

    if (comment[0] != '%' || strlen (comment) > 255) {
	status = _cairo_surface_set_error (surface, CAIRO_STATUS_INVALID_DSC_COMMENT);
	return;
    }

    /* Then, copy the comment and store it in the appropriate array. */
    comment_copy = strdup (comment);
    if (unlikely (comment_copy == NULL)) {
	status = _cairo_surface_set_error (surface, CAIRO_STATUS_NO_MEMORY);
	return;
    }

    status = _cairo_array_append (ps_surface->dsc_comment_target, &comment_copy);
    if (unlikely (status)) {
	free (comment_copy);
	status = _cairo_surface_set_error (surface, status);
	return;
    }
}

/**
 * cairo_ps_surface_dsc_begin_setup:
 * @surface: a PostScript #cairo_surface_t
 *
 * This function indicates that subsequent calls to
 * cairo_ps_surface_dsc_comment() should direct comments to the Setup
 * section of the PostScript output.
 *
 * This function should be called at most once per surface, and must
 * be called before any call to cairo_ps_surface_dsc_begin_page_setup()
 * and before any drawing is performed to the surface.
 *
 * See cairo_ps_surface_dsc_comment() for more details.
 *
 * Since: 1.2
 **/
void
cairo_ps_surface_dsc_begin_setup (cairo_surface_t *surface)
{
    cairo_ps_surface_t *ps_surface = NULL;

    if (! _extract_ps_surface (surface, TRUE, &ps_surface))
	return;

    if (ps_surface->dsc_comment_target == &ps_surface->dsc_header_comments)
	ps_surface->dsc_comment_target = &ps_surface->dsc_setup_comments;
}

/**
 * cairo_ps_surface_dsc_begin_page_setup:
 * @surface: a PostScript #cairo_surface_t
 *
 * This function indicates that subsequent calls to
 * cairo_ps_surface_dsc_comment() should direct comments to the
 * PageSetup section of the PostScript output.
 *
 * This function call is only needed for the first page of a
 * surface. It should be called after any call to
 * cairo_ps_surface_dsc_begin_setup() and before any drawing is
 * performed to the surface.
 *
 * See cairo_ps_surface_dsc_comment() for more details.
 *
 * Since: 1.2
 **/
void
cairo_ps_surface_dsc_begin_page_setup (cairo_surface_t *surface)
{
    cairo_ps_surface_t *ps_surface = NULL;

    if (! _extract_ps_surface (surface, TRUE, &ps_surface))
	return;

    if (ps_surface->dsc_comment_target == &ps_surface->dsc_header_comments ||
	ps_surface->dsc_comment_target == &ps_surface->dsc_setup_comments)
    {
	ps_surface->dsc_comment_target = &ps_surface->dsc_page_setup_comments;
    }
}

static cairo_status_t
_cairo_ps_surface_finish (void *abstract_surface)
{
    cairo_status_t status, status2;
    cairo_ps_surface_t *surface = abstract_surface;
    int i, num_comments;
    char **comments;

    status = surface->base.status;
    if (unlikely (status))
	goto CLEANUP;

    _cairo_ps_surface_emit_header (surface);

    status = _cairo_ps_surface_emit_font_subsets (surface);
    if (unlikely (status))
	goto CLEANUP;

    status = _cairo_ps_surface_emit_body (surface);
    if (unlikely (status))
	goto CLEANUP;

    _cairo_ps_surface_emit_footer (surface);

CLEANUP:
    _cairo_scaled_font_subsets_destroy (surface->font_subsets);

    status2 = _cairo_output_stream_destroy (surface->stream);
    if (status == CAIRO_STATUS_SUCCESS)
	status = status2;

    fclose (surface->tmpfile);

    status2 = _cairo_output_stream_destroy (surface->final_stream);
    if (status == CAIRO_STATUS_SUCCESS)
	status = status2;

    while (! cairo_list_is_empty (&surface->document_media)) {
        cairo_page_media_t *page;

        page = cairo_list_first_entry (&surface->document_media,
                                       cairo_page_media_t,
                                       link);
        cairo_list_del (&page->link);
	free (page->name);
	free (page);
    }

    num_comments = _cairo_array_num_elements (&surface->dsc_header_comments);
    comments = _cairo_array_index (&surface->dsc_header_comments, 0);
    for (i = 0; i < num_comments; i++)
	free (comments[i]);
    _cairo_array_fini (&surface->dsc_header_comments);

    num_comments = _cairo_array_num_elements (&surface->dsc_setup_comments);
    comments = _cairo_array_index (&surface->dsc_setup_comments, 0);
    for (i = 0; i < num_comments; i++)
	free (comments[i]);
    _cairo_array_fini (&surface->dsc_setup_comments);

    num_comments = _cairo_array_num_elements (&surface->dsc_page_setup_comments);
    comments = _cairo_array_index (&surface->dsc_page_setup_comments, 0);
    for (i = 0; i < num_comments; i++)
	free (comments[i]);
    _cairo_array_fini (&surface->dsc_page_setup_comments);

    _cairo_surface_clipper_reset (&surface->clipper);

    return status;
}

static cairo_int_status_t
_cairo_ps_surface_start_page (void *abstract_surface)
{
    cairo_ps_surface_t *surface = abstract_surface;

    /* Increment before print so page numbers start at 1. */
    surface->num_pages++;

    return CAIRO_STATUS_SUCCESS;
}

static cairo_int_status_t
_cairo_ps_surface_show_page (void *abstract_surface)
{
    cairo_ps_surface_t *surface = abstract_surface;
    cairo_int_status_t status;

    if (surface->clipper.clip != NULL)
	_cairo_surface_clipper_reset (&surface->clipper);

    status = _cairo_pdf_operators_flush (&surface->pdf_operators);
    if (unlikely (status))
	return status;

    _cairo_output_stream_printf (surface->stream,
				 "Q Q\n"
				 "showpage\n");

    return CAIRO_STATUS_SUCCESS;
}

static cairo_bool_t
color_is_gray (double red, double green, double blue)
{
    const double epsilon = 0.00001;

    return (fabs (red - green) < epsilon &&
	    fabs (red - blue) < epsilon);
}

/**
 * _cairo_ps_surface_acquire_source_surface_from_pattern:
 * @surface: the ps surface
 * @pattern: A #cairo_pattern_t of type SURFACE or RASTER_SOURCE to use as the source
 * @extents: extents of the operation that is using this source
 * @width: returns width of surface
 * @height: returns height of surface
 * @x_offset: returns x offset of surface
 * @y_offset: returns y offset of surface
 * @surface: returns surface of type image surface or recording surface
 * @image_extra: returns image extra for image type surface
 *
 * Acquire source surface or raster source pattern.
 **/
static cairo_status_t
_cairo_ps_surface_acquire_source_surface_from_pattern (cairo_ps_surface_t           *surface,
						       const cairo_pattern_t        *pattern,
						       const cairo_rectangle_int_t  *extents,
						       int                          *width,
						       int                          *height,
						       double                       *x_offset,
						       double                       *y_offset,
						       cairo_surface_t             **source_surface,
						       void                        **image_extra)
{
    cairo_status_t          status;
    cairo_image_surface_t  *image;

    *x_offset = *y_offset = 0;
    switch (pattern->type) {
    case CAIRO_PATTERN_TYPE_SURFACE: {
	cairo_surface_t *surf = ((cairo_surface_pattern_t *) pattern)->surface;

	if (surf->type == CAIRO_SURFACE_TYPE_RECORDING) {
	    if (surf->backend->type == CAIRO_SURFACE_TYPE_SUBSURFACE) {
		cairo_surface_subsurface_t *sub = (cairo_surface_subsurface_t *) surf;

		*width  = sub->extents.width;
		*height = sub->extents.height;
	    } else {
		cairo_surface_t *free_me = NULL;
		cairo_recording_surface_t *recording_surface;
		cairo_box_t bbox;
		cairo_rectangle_int_t extents;

		recording_surface = (cairo_recording_surface_t *) surf;
		if (_cairo_surface_is_snapshot (&recording_surface->base)) {
		    free_me = _cairo_surface_snapshot_get_target (&recording_surface->base);
		    recording_surface = (cairo_recording_surface_t *) free_me;
		}

		status = _cairo_recording_surface_get_bbox (recording_surface, &bbox, NULL);
		cairo_surface_destroy (free_me);
		if (unlikely (status))
		    return status;

		_cairo_box_round_to_rectangle (&bbox, &extents);
		*width  = extents.width;
		*height = extents.height;
	    }
	    *source_surface = surf;

	    return CAIRO_STATUS_SUCCESS;
	} else {
	    status =  _cairo_surface_acquire_source_image (surf, &image, image_extra);
	    if (unlikely (status))
		return status;
	}
    } break;

    case CAIRO_PATTERN_TYPE_RASTER_SOURCE: {
	cairo_surface_t *surf;
	cairo_box_t box;
	cairo_rectangle_int_t rect;

	/* get the operation extents in pattern space */
	_cairo_box_from_rectangle (&box, extents);
	_cairo_matrix_transform_bounding_box_fixed (&pattern->matrix, &box, NULL);
	_cairo_box_round_to_rectangle (&box, &rect);
	surf = _cairo_raster_source_pattern_acquire (pattern, &surface->base, &rect);
	if (!surf)
	    return CAIRO_INT_STATUS_UNSUPPORTED;
	assert (cairo_surface_get_type (surf) == CAIRO_SURFACE_TYPE_IMAGE);
	image = (cairo_image_surface_t *) surf;
    } break;

    case CAIRO_PATTERN_TYPE_SOLID:
    case CAIRO_PATTERN_TYPE_LINEAR:
    case CAIRO_PATTERN_TYPE_RADIAL:
    case CAIRO_PATTERN_TYPE_MESH:
    default:
	ASSERT_NOT_REACHED;
	break;
    }

    *width = image->width;
    *height = image->height;
    *source_surface = &image->base;
    return CAIRO_STATUS_SUCCESS;
}

static void
_cairo_ps_surface_release_source_surface_from_pattern (cairo_ps_surface_t           *surface,
						       const cairo_pattern_t        *pattern,
						       cairo_surface_t              *source,
						       void                         *image_extra)
{
    switch (pattern->type) {
    case CAIRO_PATTERN_TYPE_SURFACE: {
	cairo_surface_pattern_t *surf_pat = (cairo_surface_pattern_t *) pattern;
	if (surf_pat->surface->type != CAIRO_SURFACE_TYPE_RECORDING) {
	    cairo_image_surface_t *image  = (cairo_image_surface_t *) source;
	    _cairo_surface_release_source_image (surf_pat->surface, image, image_extra);
	}
    } break;

    case CAIRO_PATTERN_TYPE_RASTER_SOURCE:
	_cairo_raster_source_pattern_release (pattern, source);
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

/**
 * _cairo_ps_surface_create_padded_image_from_image:
 * @surface: the ps surface
 * @source: The source image
 * @extents: extents of the operation that is using this source
 * @width: returns width of padded image
 * @height: returns height of padded image
 * @x_offset: returns x offset of padded image
 * @y_offset: returns y offset of padded image
 * @image: returns the padded image or NULL if padding not required to fill @extents
 *
 * Creates a padded image if the source image does not fill the extents.
 **/
static cairo_status_t
_cairo_ps_surface_create_padded_image_from_image (cairo_ps_surface_t           *surface,
						  cairo_image_surface_t        *source,
						  const cairo_matrix_t         *source_matrix,
						  const cairo_rectangle_int_t  *extents,
						  int                          *width,
						  int                          *height,
						  double                       *x_offset,
						  double                       *y_offset,
						  cairo_image_surface_t       **image)
{
    cairo_box_t box;
    cairo_rectangle_int_t rect;
    cairo_surface_t	   *pad_image;
    cairo_surface_pattern_t pad_pattern;
    int w, h;
    cairo_int_status_t      status;

    /* get the operation extents in pattern space */
    _cairo_box_from_rectangle (&box, extents);
    _cairo_matrix_transform_bounding_box_fixed (source_matrix, &box, NULL);
    _cairo_box_round_to_rectangle (&box, &rect);

    /* Check if image needs padding to fill extents. */
    w = source->width;
    h = source->height;
    if (_cairo_fixed_integer_ceil(box.p1.x) < 0 ||
	_cairo_fixed_integer_ceil(box.p1.y) < 0 ||
	_cairo_fixed_integer_floor(box.p2.y) > w ||
	_cairo_fixed_integer_floor(box.p2.y) > h)
    {
	pad_image =
	    _cairo_image_surface_create_with_pixman_format (NULL,
							    source->pixman_format,
							    rect.width, rect.height,
							    0);
	if (pad_image->status)
	    return pad_image->status;

	_cairo_pattern_init_for_surface (&pad_pattern, &source->base);
	cairo_matrix_init_translate (&pad_pattern.base.matrix, rect.x, rect.y);
	pad_pattern.base.extend = CAIRO_EXTEND_PAD;
	status = _cairo_surface_paint (pad_image,
				       CAIRO_OPERATOR_SOURCE,
				       &pad_pattern.base,
				       NULL);
	_cairo_pattern_fini (&pad_pattern.base);
	*image = (cairo_image_surface_t *) pad_image;
	*width = rect.width;
	*height = rect.height;
	*x_offset = rect.x;
	*y_offset = rect.y;
    } else {
	*image = NULL;
	status = CAIRO_STATUS_SUCCESS;
    }

    return status;
}

static cairo_int_status_t
_cairo_ps_surface_analyze_surface_pattern_transparency (cairo_ps_surface_t            *surface,
							const cairo_pattern_t         *pattern,
							const cairo_rectangle_int_t   *extents)
{
    int width, height;
    double x_offset, y_offset;
    cairo_surface_t *source;
    cairo_image_surface_t *image;
    void *image_extra;
    cairo_int_status_t      status;
    cairo_image_transparency_t transparency;

    status = _cairo_ps_surface_acquire_source_surface_from_pattern (surface,
								    pattern,
								    extents,
								    &width,
								    &height,
								    &x_offset,
								    &y_offset,
								    &source,
								    &image_extra);
    if (unlikely (status))
	return status;

    image = (cairo_image_surface_t *) source;
    if (image->base.status)
	return image->base.status;

    transparency = _cairo_image_analyze_transparency (image);
    switch (transparency) {
    case CAIRO_IMAGE_IS_OPAQUE:
	status = CAIRO_STATUS_SUCCESS;
	break;

    case CAIRO_IMAGE_HAS_BILEVEL_ALPHA:
	if (surface->ps_level == CAIRO_PS_LEVEL_2) {
	    status = CAIRO_INT_STATUS_FLATTEN_TRANSPARENCY;
	} else {
	    surface->ps_level_used = CAIRO_PS_LEVEL_3;
	    status = CAIRO_STATUS_SUCCESS;
	}
	break;

    case CAIRO_IMAGE_HAS_ALPHA:
	status = CAIRO_INT_STATUS_FLATTEN_TRANSPARENCY;
	break;

    case CAIRO_IMAGE_UNKNOWN:
	ASSERT_NOT_REACHED;
    }

    _cairo_ps_surface_release_source_surface_from_pattern (surface, pattern, source, image_extra);

    return status;
}

static cairo_bool_t
surface_pattern_supported (const cairo_surface_pattern_t *pattern)
{
    if (pattern->surface->type == CAIRO_SURFACE_TYPE_RECORDING)
	return TRUE;

    if (pattern->surface->backend->acquire_source_image == NULL)
	return FALSE;

    /* Does an ALPHA-only source surface even make sense? Maybe, but I
     * don't think it's worth the extra code to support it. */

/* XXX: Need to write this function here...
    content = pattern->surface->content;
    if (content == CAIRO_CONTENT_ALPHA)
	return FALSE;
*/

    return TRUE;
}

static cairo_bool_t
_gradient_pattern_supported (cairo_ps_surface_t    *surface,
			     const cairo_pattern_t *pattern)
{
    double min_alpha, max_alpha;

    if (surface->ps_level == CAIRO_PS_LEVEL_2)
	return FALSE;

    /* Alpha gradients are only supported (by flattening the alpha)
     * if there is no variation in the alpha across the gradient. */
    _cairo_pattern_alpha_range (pattern, &min_alpha, &max_alpha);
    if (min_alpha != max_alpha)
	return FALSE;

    surface->ps_level_used = CAIRO_PS_LEVEL_3;

    return TRUE;
}

static cairo_bool_t
pattern_supported (cairo_ps_surface_t *surface, const cairo_pattern_t *pattern)
{
    switch (pattern->type) {
    case CAIRO_PATTERN_TYPE_SOLID:
	return TRUE;

    case CAIRO_PATTERN_TYPE_LINEAR:
    case CAIRO_PATTERN_TYPE_RADIAL:
    case CAIRO_PATTERN_TYPE_MESH:
	return _gradient_pattern_supported (surface, pattern);

    case CAIRO_PATTERN_TYPE_SURFACE:
	return surface_pattern_supported ((cairo_surface_pattern_t *) pattern);

    case CAIRO_PATTERN_TYPE_RASTER_SOURCE:
	return TRUE;

    default:
	ASSERT_NOT_REACHED;
	return FALSE;
    }
}

static cairo_bool_t
mask_supported (cairo_ps_surface_t *surface,
		const cairo_pattern_t *mask,
		const cairo_rectangle_int_t *extents)
{
    if (surface->ps_level == CAIRO_PS_LEVEL_2)
	return FALSE;

    if (mask->type == CAIRO_PATTERN_TYPE_SURFACE) {
	cairo_surface_pattern_t *surface_pattern = (cairo_surface_pattern_t *) mask;
	if (surface_pattern->surface->type == CAIRO_SURFACE_TYPE_IMAGE) {
	    /* check if mask if opaque or bilevel alpha */
	    if (_cairo_ps_surface_analyze_surface_pattern_transparency (surface, mask, extents) == CAIRO_INT_STATUS_SUCCESS) {
		surface->ps_level_used = CAIRO_PS_LEVEL_3;
		return TRUE;
	    }
	}
    }

    return FALSE;
}

static cairo_int_status_t
_cairo_ps_surface_analyze_operation (cairo_ps_surface_t    *surface,
				     cairo_operator_t       op,
				     const cairo_pattern_t       *pattern,
				     const cairo_pattern_t       *mask,
				     const cairo_rectangle_int_t *extents)
{
    double min_alpha;

    if (surface->force_fallbacks &&
	surface->paginated_mode == CAIRO_PAGINATED_MODE_ANALYZE)
    {
	return CAIRO_INT_STATUS_UNSUPPORTED;
    }

    if (! pattern_supported (surface, pattern))
	return CAIRO_INT_STATUS_UNSUPPORTED;

    if (! (op == CAIRO_OPERATOR_SOURCE || op == CAIRO_OPERATOR_OVER))
	return CAIRO_INT_STATUS_UNSUPPORTED;

    /* Mask is only supported when the mask is an image with opaque or bilevel alpha. */
    if (mask && !mask_supported (surface, mask, extents))
	return CAIRO_INT_STATUS_UNSUPPORTED;

    if (pattern->type == CAIRO_PATTERN_TYPE_SURFACE) {
	cairo_surface_pattern_t *surface_pattern = (cairo_surface_pattern_t *) pattern;

	if (surface_pattern->surface->type == CAIRO_SURFACE_TYPE_RECORDING) {
	    if (pattern->extend == CAIRO_EXTEND_PAD)
		return CAIRO_INT_STATUS_UNSUPPORTED;
	    else
		return CAIRO_INT_STATUS_ANALYZE_RECORDING_SURFACE_PATTERN;
	}
    }

    if (op == CAIRO_OPERATOR_SOURCE) {
	if (mask)
	    return CAIRO_INT_STATUS_UNSUPPORTED;
	else
	    return CAIRO_STATUS_SUCCESS;
    }

    /* CAIRO_OPERATOR_OVER is only supported for opaque patterns. If
     * the pattern contains transparency, we return
     * CAIRO_INT_STATUS_FLATTEN_TRANSPARENCY to the analysis
     * surface. If the analysis surface determines that there is
     * anything drawn under this operation, a fallback image will be
     * used. Otherwise the operation will be replayed during the
     * render stage and we blend the transparency into the white
     * background to convert the pattern to opaque.
     */
    if (pattern->type == CAIRO_PATTERN_TYPE_SURFACE || pattern->type == CAIRO_PATTERN_TYPE_RASTER_SOURCE)
	return _cairo_ps_surface_analyze_surface_pattern_transparency (surface, pattern, extents);

    /* Patterns whose drawn part is opaque are directly supported;
       those whose drawn part is partially transparent can be
       supported by flattening the alpha. */
    _cairo_pattern_alpha_range (pattern, &min_alpha, NULL);
    if (CAIRO_ALPHA_IS_OPAQUE (min_alpha))
	return CAIRO_STATUS_SUCCESS;

    return CAIRO_INT_STATUS_FLATTEN_TRANSPARENCY;
}

static cairo_bool_t
_cairo_ps_surface_operation_supported (cairo_ps_surface_t    *surface,
				       cairo_operator_t       op,
				       const cairo_pattern_t       *pattern,
				       const cairo_pattern_t       *mask,
				       const cairo_rectangle_int_t *extents)
{
    return _cairo_ps_surface_analyze_operation (surface, op, pattern, mask, extents) != CAIRO_INT_STATUS_UNSUPPORTED;
}

/* The "standard" implementation limit for PostScript string sizes is
 * 65535 characters (see PostScript Language Reference, Appendix
 * B). We go one short of that because we sometimes need two
 * characters in a string to represent a single ASCII85 byte, (for the
 * escape sequences "\\", "\(", and "\)") and we must not split these
 * across two strings. So we'd be in trouble if we went right to the
 * limit and one of these escape sequences just happened to land at
 * the end.
 */
#define STRING_ARRAY_MAX_STRING_SIZE (65535-1)
#define STRING_ARRAY_MAX_COLUMN	     72

typedef struct _string_array_stream {
    cairo_output_stream_t base;
    cairo_output_stream_t *output;
    int column;
    int string_size;
    cairo_bool_t use_strings;
} string_array_stream_t;

static cairo_status_t
_string_array_stream_write (cairo_output_stream_t *base,
			    const unsigned char   *data,
			    unsigned int	   length)
{
    string_array_stream_t *stream = (string_array_stream_t *) base;
    unsigned char c;
    const unsigned char backslash = '\\';

    if (length == 0)
	return CAIRO_STATUS_SUCCESS;

    while (length--) {
	if (stream->string_size == 0 && stream->use_strings) {
	    _cairo_output_stream_printf (stream->output, "(");
	    stream->column++;
	}

	c = *data++;
	if (stream->use_strings) {
	    switch (c) {
	    case '\\':
	    case '(':
	    case ')':
		_cairo_output_stream_write (stream->output, &backslash, 1);
		stream->column++;
		stream->string_size++;
		break;
	    }
	}
	/* Have to be careful to never split the final ~> sequence. */
        if (c == '~') {
	    _cairo_output_stream_write (stream->output, &c, 1);
	    stream->column++;
	    stream->string_size++;

	    if (length-- == 0)
		break;

	    c = *data++;
	}
	_cairo_output_stream_write (stream->output, &c, 1);
	stream->column++;
	stream->string_size++;

	if (stream->use_strings &&
	    stream->string_size >= STRING_ARRAY_MAX_STRING_SIZE)
	{
	    _cairo_output_stream_printf (stream->output, ")\n");
	    stream->string_size = 0;
	    stream->column = 0;
	}
	if (stream->column >= STRING_ARRAY_MAX_COLUMN) {
	    _cairo_output_stream_printf (stream->output, "\n ");
	    stream->string_size += 2;
	    stream->column = 1;
	}
    }

    return _cairo_output_stream_get_status (stream->output);
}

static cairo_status_t
_string_array_stream_close (cairo_output_stream_t *base)
{
    cairo_status_t status;
    string_array_stream_t *stream = (string_array_stream_t *) base;

    if (stream->use_strings)
	_cairo_output_stream_printf (stream->output, ")\n");

    status = _cairo_output_stream_get_status (stream->output);

    return status;
}

/* A string_array_stream wraps an existing output stream. It takes the
 * data provided to it and output one or more consecutive string
 * objects, each within the standard PostScript implementation limit
 * of 65k characters.
 *
 * The strings are each separated by a space character for easy
 * inclusion within an array object, (but the array delimiters are not
 * added by the string_array_stream).
 *
 * The string array stream is also careful to wrap the output within
 * STRING_ARRAY_MAX_COLUMN columns (+/- 1). The stream also adds
 * necessary escaping for special characters within a string,
 * (specifically '\', '(', and ')').
 */
static cairo_output_stream_t *
_string_array_stream_create (cairo_output_stream_t *output)
{
    string_array_stream_t *stream;

    stream = malloc (sizeof (string_array_stream_t));
    if (unlikely (stream == NULL)) {
	_cairo_error_throw (CAIRO_STATUS_NO_MEMORY);
	return (cairo_output_stream_t *) &_cairo_output_stream_nil;
    }

    _cairo_output_stream_init (&stream->base,
			       _string_array_stream_write,
			       NULL,
			       _string_array_stream_close);
    stream->output = output;
    stream->column = 0;
    stream->string_size = 0;
    stream->use_strings = TRUE;

    return &stream->base;
}

/* A base85_array_stream wraps an existing output stream. It wraps the
 * output within STRING_ARRAY_MAX_COLUMN columns (+/- 1). The output
 * is not enclosed in strings like string_array_stream.
 */
static cairo_output_stream_t *
_base85_array_stream_create (cairo_output_stream_t *output)
{
    string_array_stream_t *stream;

    stream = malloc (sizeof (string_array_stream_t));
    if (unlikely (stream == NULL)) {
	_cairo_error_throw (CAIRO_STATUS_NO_MEMORY);
	return (cairo_output_stream_t *) &_cairo_output_stream_nil;
    }

    _cairo_output_stream_init (&stream->base,
			       _string_array_stream_write,
			       NULL,
			       _string_array_stream_close);
    stream->output = output;
    stream->column = 0;
    stream->string_size = 0;
    stream->use_strings = FALSE;

    return &stream->base;
}


/* PS Output - this section handles output of the parts of the recording
 * surface we can render natively in PS. */

static cairo_status_t
_cairo_ps_surface_flatten_image_transparency (cairo_ps_surface_t    *surface,
					      cairo_image_surface_t *image,
					      cairo_image_surface_t **opaque_image)
{
    cairo_surface_t *opaque;
    cairo_surface_pattern_t pattern;
    cairo_status_t status;

    opaque = cairo_image_surface_create (CAIRO_FORMAT_RGB24,
					 image->width,
					 image->height);
    if (unlikely (opaque->status))
	return opaque->status;

    if (surface->content == CAIRO_CONTENT_COLOR_ALPHA) {
	status = _cairo_surface_paint (opaque,
				       CAIRO_OPERATOR_SOURCE,
				       &_cairo_pattern_white.base,
				       NULL);
	if (unlikely (status)) {
	    cairo_surface_destroy (opaque);
	    return status;
	}
    }

    _cairo_pattern_init_for_surface (&pattern, &image->base);
    pattern.base.filter = CAIRO_FILTER_NEAREST;
    status = _cairo_surface_paint (opaque, CAIRO_OPERATOR_OVER, &pattern.base, NULL);
    _cairo_pattern_fini (&pattern.base);
    if (unlikely (status)) {
	cairo_surface_destroy (opaque);
	return status;
    }

    *opaque_image = (cairo_image_surface_t *) opaque;
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_ps_surface_emit_base85_string (cairo_ps_surface_t    *surface,
				      const unsigned char   *data,
				      unsigned long	     length,
				      cairo_ps_compress_t    compress,
				      cairo_bool_t           use_strings)
{
    cairo_output_stream_t *base85_stream, *string_array_stream, *deflate_stream;
    unsigned char *data_compressed;
    unsigned long data_compressed_size;
    cairo_status_t status, status2;

    if (use_strings)
	string_array_stream = _string_array_stream_create (surface->stream);
    else
	string_array_stream = _base85_array_stream_create (surface->stream);

    status = _cairo_output_stream_get_status (string_array_stream);
    if (unlikely (status))
	return _cairo_output_stream_destroy (string_array_stream);

    base85_stream = _cairo_base85_stream_create (string_array_stream);
    status = _cairo_output_stream_get_status (base85_stream);
    if (unlikely (status)) {
	status2 = _cairo_output_stream_destroy (string_array_stream);
	return _cairo_output_stream_destroy (base85_stream);
    }

    switch (compress) {
	case CAIRO_PS_COMPRESS_NONE:
	    _cairo_output_stream_write (base85_stream, data, length);
	    break;

	case CAIRO_PS_COMPRESS_LZW:
	    /* XXX: Should fix cairo-lzw to provide a stream-based interface
	     * instead. */
	    data_compressed_size = length;
	    data_compressed = _cairo_lzw_compress ((unsigned char*)data, &data_compressed_size);
	    if (unlikely (data_compressed == NULL)) {
		status = _cairo_output_stream_destroy (string_array_stream);
		status = _cairo_output_stream_destroy (base85_stream);
		return _cairo_error (CAIRO_STATUS_NO_MEMORY);
	    }
	    _cairo_output_stream_write (base85_stream, data_compressed, data_compressed_size);
	    free (data_compressed);
	    break;

	case CAIRO_PS_COMPRESS_DEFLATE:
	    deflate_stream = _cairo_deflate_stream_create (base85_stream);
	    if (_cairo_output_stream_get_status (deflate_stream)) {
		return _cairo_output_stream_destroy (deflate_stream);
	    }
	    _cairo_output_stream_write (deflate_stream, data, length);
	    status = _cairo_output_stream_destroy (deflate_stream);
	    if (unlikely (status)) {
		status2 = _cairo_output_stream_destroy (string_array_stream);
		status2 = _cairo_output_stream_destroy (base85_stream);
		return _cairo_output_stream_destroy (deflate_stream);
	    }
	    break;
    }
    status = _cairo_output_stream_destroy (base85_stream);

    /* Mark end of base85 data */
    _cairo_output_stream_printf (string_array_stream, "~>");
    status2 = _cairo_output_stream_destroy (string_array_stream);
    if (status == CAIRO_STATUS_SUCCESS)
	status = status2;

    return status;
}

static cairo_status_t
_cairo_ps_surface_emit_image (cairo_ps_surface_t    *surface,
			      cairo_image_surface_t *image_surf,
			      cairo_operator_t	     op,
			      cairo_filter_t         filter,
			      cairo_bool_t           stencil_mask)
{
    cairo_status_t status;
    unsigned char *data;
    unsigned long data_size;
    cairo_image_surface_t *ps_image;
    int x, y, i, a;
    cairo_image_transparency_t transparency;
    cairo_bool_t use_mask;
    uint32_t *pixel32;
    uint8_t *pixel8;
    int bit;
    cairo_image_color_t color;
    const char *interpolate;
    cairo_ps_compress_t compress;
    const char *compress_filter;
    cairo_image_surface_t *image;

    if (image_surf->base.status)
	return image_surf->base.status;

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
	    goto bail0;
	}

	_cairo_pattern_init_for_surface (&pattern, &image_surf->base);
	status = _cairo_surface_paint (surf,
				       CAIRO_OPERATOR_SOURCE, &pattern.base,
				       NULL);
        _cairo_pattern_fini (&pattern.base);
        if (unlikely (status))
            goto bail0;
    }
    ps_image = image;

    switch (filter) {
    default:
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

    if (stencil_mask) {
	use_mask = FALSE;
	color = CAIRO_IMAGE_IS_MONOCHROME;
	transparency = CAIRO_IMAGE_HAS_BILEVEL_ALPHA;
    } else {
	transparency = _cairo_image_analyze_transparency (image);

	/* PostScript can not represent the alpha channel, so we blend the
	   current image over a white (or black for CONTENT_COLOR
	   surfaces) RGB surface to eliminate it. */

	if (op == CAIRO_OPERATOR_SOURCE ||
	    transparency == CAIRO_IMAGE_HAS_ALPHA ||
	    (transparency == CAIRO_IMAGE_HAS_BILEVEL_ALPHA &&
	     surface->ps_level == CAIRO_PS_LEVEL_2))
	{
	    status = _cairo_ps_surface_flatten_image_transparency (surface,
								   image,
								   &ps_image);
	    if (unlikely (status))
		return status;

	    use_mask = FALSE;
	} else if (transparency == CAIRO_IMAGE_IS_OPAQUE) {
	    use_mask = FALSE;
	} else { /* transparency == CAIRO_IMAGE_HAS_BILEVEL_ALPHA */
	    use_mask = TRUE;
	}

	color = _cairo_image_analyze_color (ps_image);
    }

    /* Type 2 (mask and image interleaved) has the mask and image
     * samples interleaved by row.  The mask row is first, one bit per
     * pixel with (bit 7 first). The row is padded to byte
     * boundaries. The image data is 3 bytes per pixel RGB format. */
    switch (color) {
    default:
    case CAIRO_IMAGE_UNKNOWN_COLOR:
	ASSERT_NOT_REACHED;
    case CAIRO_IMAGE_IS_COLOR:
	data_size = ps_image->width * 3;
	break;
    case CAIRO_IMAGE_IS_GRAYSCALE:
	data_size = ps_image->width;
	break;
    case CAIRO_IMAGE_IS_MONOCHROME:
	data_size = (ps_image->width + 7)/8;
	break;
    }
    if (use_mask)
	data_size += (ps_image->width + 7)/8;
    data_size *= ps_image->height;
    data = malloc (data_size);
    if (unlikely (data == NULL)) {
	status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	goto bail1;
    }

    i = 0;
    for (y = 0; y < ps_image->height; y++) {
	if (stencil_mask || use_mask) {
	    /* mask row */
	    if (ps_image->format == CAIRO_FORMAT_A1) {
		pixel8 = (uint8_t *) (ps_image->data + y * ps_image->stride);

		for (x = 0; x < (ps_image->width + 7) / 8; x++, pixel8++) {
		    a = *pixel8;
		    a = CAIRO_BITSWAP8_IF_LITTLE_ENDIAN (a);
		    data[i++] = a;
		}
	    } else {
		pixel8 = (uint8_t *) (ps_image->data + y * ps_image->stride);
		pixel32 = (uint32_t *) (ps_image->data + y * ps_image->stride);
		bit = 7;
		for (x = 0; x < ps_image->width; x++) {
		    if (ps_image->format == CAIRO_FORMAT_ARGB32) {
			a = (*pixel32 & 0xff000000) >> 24;
			pixel32++;
		    } else {
			a = *pixel8;
			pixel8++;
		    }

		    if (transparency == CAIRO_IMAGE_HAS_ALPHA) {
			data[i++] = a;
		    } else { /* transparency == CAIRO_IMAGE_HAS_BILEVEL_ALPHA or CAIRO_IMAGE_IS_OPAQUE */
			if (bit == 7)
			    data[i] = 0;
			if (a != 0)
			    data[i] |= (1 << bit);
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
	if (stencil_mask)
	    continue;

	/* image row*/
	pixel32 = (uint32_t *) (ps_image->data + y * ps_image->stride);
	bit = 7;
	for (x = 0; x < ps_image->width; x++, pixel32++) {
	    int r, g, b;

	    if (ps_image->format == CAIRO_FORMAT_ARGB32) {
		/* At this point ARGB32 images are either opaque or
		 * bilevel alpha so we don't need to unpremultiply. */
		if (((*pixel32 & 0xff000000) >> 24) == 0) {
		    r = g = b = 0;
		} else {
		    r = (*pixel32 & 0x00ff0000) >> 16;
		    g = (*pixel32 & 0x0000ff00) >>  8;
		    b = (*pixel32 & 0x000000ff) >>  0;
		}
	    } else if (ps_image->format == CAIRO_FORMAT_RGB24) {
		r = (*pixel32 & 0x00ff0000) >> 16;
		g = (*pixel32 & 0x0000ff00) >>  8;
		b = (*pixel32 & 0x000000ff) >>  0;
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

    if (surface->ps_level == CAIRO_PS_LEVEL_2) {
	compress = CAIRO_PS_COMPRESS_LZW;
	compress_filter = "LZWDecode";
    } else {
	compress = CAIRO_PS_COMPRESS_DEFLATE;
	compress_filter = "FlateDecode";
	surface->ps_level_used = CAIRO_PS_LEVEL_3;
    }

    if (surface->use_string_datasource) {
	/* Emit the image data as a base85-encoded string which will
	 * be used as the data source for the image operator later. */
	_cairo_output_stream_printf (surface->stream,
				     "/CairoImageData [\n");

	status = _cairo_ps_surface_emit_base85_string (surface,
						       data,
						       data_size,
						       compress,
						       TRUE);
	if (unlikely (status))
	    goto bail2;

	_cairo_output_stream_printf (surface->stream,
				     "] def\n");
	_cairo_output_stream_printf (surface->stream,
				     "/CairoImageDataIndex 0 def\n");
    }

    if (use_mask) {
	_cairo_output_stream_printf (surface->stream,
				     "%s setcolorspace\n"
				     "5 dict dup begin\n"
				     "  /ImageType 3 def\n"
				     "  /InterleaveType 2 def\n"
				     "  /DataDict 8 dict def\n"
				     "    DataDict begin\n"
				     "    /ImageType 1 def\n"
				     "    /Width %d def\n"
				     "    /Height %d def\n"
				     "    /Interpolate %s def\n"
				     "    /BitsPerComponent %d def\n"
				     "    /Decode [ %s ] def\n",
				     color == CAIRO_IMAGE_IS_COLOR ? "/DeviceRGB" : "/DeviceGray",
				     ps_image->width,
				     ps_image->height,
				     interpolate,
				     color == CAIRO_IMAGE_IS_MONOCHROME ? 1 : 8,
				     color == CAIRO_IMAGE_IS_COLOR ? "0 1 0 1 0 1" : "0 1");

	if (surface->use_string_datasource) {
	    _cairo_output_stream_printf (surface->stream,
					 "    /DataSource {\n"
					 "      CairoImageData CairoImageDataIndex get\n"
					 "	/CairoImageDataIndex CairoImageDataIndex 1 add def\n"
					 "	CairoImageDataIndex CairoImageData length 1 sub gt\n"
					 "       { /CairoImageDataIndex 0 def } if\n"
					 "    } /ASCII85Decode filter /%s filter def\n",
					 compress_filter);
	} else {
	    _cairo_output_stream_printf (surface->stream,
					 "    /DataSource currentfile /ASCII85Decode filter /%s filter def\n",
					 compress_filter);
	}

	_cairo_output_stream_printf (surface->stream,
				     "    /ImageMatrix [ 1 0 0 -1 0 %d ] def\n"
				     "  end\n"
				     "  /MaskDict 8 dict def\n"
				     "     MaskDict begin\n"
				     "    /ImageType 1 def\n"
				     "    /Width %d def\n"
				     "    /Height %d def\n"
				     "    /Interpolate %s def\n"
				     "    /BitsPerComponent 1 def\n"
				     "    /Decode [ 1 0 ] def\n"
				     "    /ImageMatrix [ 1 0 0 -1 0 %d ] def\n"
				     "  end\n"
				     "end\n"
				     "image\n",
				     ps_image->height,
				     ps_image->width,
				     ps_image->height,
				     interpolate,
				     ps_image->height);
    } else {
	if (!stencil_mask) {
	    _cairo_output_stream_printf (surface->stream,
					 "%s setcolorspace\n",
					 color == CAIRO_IMAGE_IS_COLOR ? "/DeviceRGB" : "/DeviceGray");
	}
	_cairo_output_stream_printf (surface->stream,
				     "8 dict dup begin\n"
				     "  /ImageType 1 def\n"
				     "  /Width %d def\n"
				     "  /Height %d def\n"
				     "  /Interpolate %s def\n"
				     "  /BitsPerComponent %d def\n"
				     "  /Decode [ %s ] def\n",
				     ps_image->width,
				     ps_image->height,
				     interpolate,
				     color == CAIRO_IMAGE_IS_MONOCHROME ? 1 : 8,
				     stencil_mask ? "1 0" : color == CAIRO_IMAGE_IS_COLOR ? "0 1 0 1 0 1" : "0 1");
	if (surface->use_string_datasource) {
	    _cairo_output_stream_printf (surface->stream,
					 "  /DataSource {\n"
					 "    CairoImageData CairoImageDataIndex get\n"
					 "    /CairoImageDataIndex CairoImageDataIndex 1 add def\n"
					 "    CairoImageDataIndex CairoImageData length 1 sub gt\n"
					 "     { /CairoImageDataIndex 0 def } if\n"
					 "  } /ASCII85Decode filter /%s filter def\n",
					 compress_filter);
	} else {
	    _cairo_output_stream_printf (surface->stream,
					 "  /DataSource currentfile /ASCII85Decode filter /%s filter def\n",
					 compress_filter);
	}

	_cairo_output_stream_printf (surface->stream,
				     "  /Interpolate %s def\n"
				     "  /ImageMatrix [ 1 0 0 -1 0 %d ] def\n"
				     "end\n"
				     "%s\n",
				     interpolate,
				     ps_image->height,
				     stencil_mask ? "imagemask" : "image");
    }

    if (!surface->use_string_datasource) {
	/* Emit the image data as a base85-encoded string which will
	 * be used as the data source for the image operator. */
	status = _cairo_ps_surface_emit_base85_string (surface,
						       data,
						       data_size,
						       compress,
						       FALSE);
	_cairo_output_stream_printf (surface->stream, "\n");
    } else {
	status = CAIRO_STATUS_SUCCESS;
    }

bail2:
    free (data);

bail1:
    if (!use_mask && ps_image != image)
	cairo_surface_destroy (&ps_image->base);

bail0:
    if (image != image_surf)
	cairo_surface_destroy (&image->base);

    return status;
}

static cairo_status_t
_cairo_ps_surface_emit_jpeg_image (cairo_ps_surface_t    *surface,
				   cairo_surface_t	 *source,
				   int                    width,
				   int                    height)
{
    cairo_status_t status;
    const unsigned char *mime_data;
    unsigned long mime_data_length;
    cairo_image_info_t info;
    const char *colorspace;
    const char *decode;

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
	    decode = "0 1";
	    break;
	case 3:
	    colorspace = "/DeviceRGB";
	    decode =  "0 1 0 1 0 1";
	    break;
	case 4:
	    colorspace = "/DeviceCMYK";
	    decode =  "0 1 0 1 0 1 0 1";
	    break;
	default:
	    return CAIRO_INT_STATUS_UNSUPPORTED;
    }

    if (surface->use_string_datasource) {
	/* Emit the image data as a base85-encoded string which will
	 * be used as the data source for the image operator later. */
	_cairo_output_stream_printf (surface->stream,
				     "/CairoImageData [\n");

	status = _cairo_ps_surface_emit_base85_string (surface,
						       mime_data,
						       mime_data_length,
						       CAIRO_PS_COMPRESS_NONE,
						       TRUE);
	if (unlikely (status))
	    return status;

	_cairo_output_stream_printf (surface->stream,
				     "] def\n");
	_cairo_output_stream_printf (surface->stream,
				     "/CairoImageDataIndex 0 def\n");
    }

    _cairo_output_stream_printf (surface->stream,
				 "%s setcolorspace\n"
				 "8 dict dup begin\n"
				 "  /ImageType 1 def\n"
				 "  /Width %d def\n"
				 "  /Height %d def\n"
				 "  /BitsPerComponent %d def\n"
				 "  /Decode [ %s ] def\n",
				 colorspace,
				 info.width,
				 info.height,
				 info.bits_per_component,
                                 decode);

    if (surface->use_string_datasource) {
	_cairo_output_stream_printf (surface->stream,
				     "  /DataSource {\n"
				     "    CairoImageData CairoImageDataIndex get\n"
				     "    /CairoImageDataIndex CairoImageDataIndex 1 add def\n"
				     "    CairoImageDataIndex CairoImageData length 1 sub gt\n"
				     "     { /CairoImageDataIndex 0 def } if\n"
				     "  } /ASCII85Decode filter /DCTDecode filter def\n");
    } else {
	_cairo_output_stream_printf (surface->stream,
				     "  /DataSource currentfile /ASCII85Decode filter /DCTDecode filter def\n");
    }

    _cairo_output_stream_printf (surface->stream,
				 "  /ImageMatrix [ 1 0 0 -1 0 %d ] def\n"
				 "end\n"
				 "image\n",
				 info.height);

    if (!surface->use_string_datasource) {
	/* Emit the image data as a base85-encoded string which will
	 * be used as the data source for the image operator. */
	status = _cairo_ps_surface_emit_base85_string (surface,
						       mime_data,
						       mime_data_length,
						       CAIRO_PS_COMPRESS_NONE,
						       FALSE);
    }

    return status;
}

static cairo_status_t
_cairo_ps_surface_emit_recording_surface (cairo_ps_surface_t *surface,
					  cairo_surface_t    *recording_surface)
{
    double old_width, old_height;
    cairo_matrix_t old_cairo_to_ps;
    cairo_content_t old_content;
    cairo_rectangle_int_t old_page_bbox;
    cairo_surface_t *free_me = NULL;
    cairo_surface_clipper_t old_clipper;
    cairo_box_t bbox;
    cairo_int_status_t status;

    old_content = surface->content;
    old_width = surface->width;
    old_height = surface->height;
    old_page_bbox = surface->page_bbox;
    old_cairo_to_ps = surface->cairo_to_ps;
    old_clipper = surface->clipper;
    _cairo_surface_clipper_init (&surface->clipper,
				 _cairo_ps_surface_clipper_intersect_clip_path);

    if (_cairo_surface_is_snapshot (recording_surface))
	free_me = recording_surface = _cairo_surface_snapshot_get_target (recording_surface);

    status =
	_cairo_recording_surface_get_bbox ((cairo_recording_surface_t *) recording_surface,
					   &bbox,
					   NULL);
    if (unlikely (status))
	goto err;

#if DEBUG_PS
    _cairo_output_stream_printf (surface->stream,
				 "%% _cairo_ps_surface_emit_recording_surface (%f, %f), (%f, %f)\n",
				 _cairo_fixed_to_double (bbox.p1.x),
				 _cairo_fixed_to_double (bbox.p1.y),
				 _cairo_fixed_to_double (bbox.p2.x),
				 _cairo_fixed_to_double (bbox.p2.y));
#endif

    surface->width = _cairo_fixed_to_double (bbox.p2.x - bbox.p1.x);
    surface->height = _cairo_fixed_to_double (bbox.p2.y - bbox.p1.y);
    _cairo_box_round_to_rectangle (&bbox, &surface->page_bbox);

    surface->current_pattern_is_solid_color = FALSE;
    _cairo_pdf_operators_reset (&surface->pdf_operators);
    cairo_matrix_init (&surface->cairo_to_ps, 1, 0, 0, -1, 0, surface->height);
    _cairo_pdf_operators_set_cairo_to_pdf_matrix (&surface->pdf_operators,
						  &surface->cairo_to_ps);
    _cairo_output_stream_printf (surface->stream, "  q\n");

    if (recording_surface->content == CAIRO_CONTENT_COLOR) {
	surface->content = CAIRO_CONTENT_COLOR;
	_cairo_output_stream_printf (surface->stream,
				     "  0 g %d %d %d %d rectfill\n",
				     surface->page_bbox.x,
				     surface->page_bbox.y,
				     surface->page_bbox.width,
				     surface->page_bbox.height);
    }

    status = _cairo_recording_surface_replay_region (recording_surface,
						     NULL,
						     &surface->base,
						     CAIRO_RECORDING_REGION_NATIVE);
    assert (status != CAIRO_INT_STATUS_UNSUPPORTED);
    if (unlikely (status))
	goto err;

    status = _cairo_pdf_operators_flush (&surface->pdf_operators);
    if (unlikely (status))
	goto err;

    _cairo_output_stream_printf (surface->stream, "  Q\n");

    _cairo_surface_clipper_reset (&surface->clipper);
    surface->clipper = old_clipper;
    surface->content = old_content;
    surface->width = old_width;
    surface->height = old_height;
    surface->page_bbox = old_page_bbox;
    surface->current_pattern_is_solid_color = FALSE;
    _cairo_pdf_operators_reset (&surface->pdf_operators);
    surface->cairo_to_ps = old_cairo_to_ps;

    _cairo_pdf_operators_set_cairo_to_pdf_matrix (&surface->pdf_operators,
						  &surface->cairo_to_ps);

err:
    cairo_surface_destroy (free_me);
    return status;
}

static cairo_int_status_t
_cairo_ps_surface_emit_recording_subsurface (cairo_ps_surface_t *surface,
					     cairo_surface_t    *recording_surface,
					     const cairo_rectangle_int_t *extents)
{
    double old_width, old_height;
    cairo_matrix_t old_cairo_to_ps;
    cairo_content_t old_content;
    cairo_rectangle_int_t old_page_bbox;
    cairo_surface_clipper_t old_clipper;
    cairo_surface_t *free_me = NULL;
    cairo_int_status_t status;

    old_content = surface->content;
    old_width = surface->width;
    old_height = surface->height;
    old_page_bbox = surface->page_bbox;
    old_cairo_to_ps = surface->cairo_to_ps;
    old_clipper = surface->clipper;
    _cairo_surface_clipper_init (&surface->clipper,
				 _cairo_ps_surface_clipper_intersect_clip_path);

#if DEBUG_PS
    _cairo_output_stream_printf (surface->stream,
				 "%% _cairo_ps_surface_emit_recording_subsurface (%d, %d), (%d, %d)\n",
				 extents->x, extents->y,
				 extents->width, extents->height);
#endif

    surface->page_bbox.x = surface->page_bbox.y = 0;
    surface->page_bbox.width = surface->width  = extents->width;
    surface->page_bbox.height = surface->height = extents->height;

    surface->current_pattern_is_solid_color = FALSE;
    _cairo_pdf_operators_reset (&surface->pdf_operators);
    cairo_matrix_init (&surface->cairo_to_ps, 1, 0, 0, -1, 0, surface->height);
    _cairo_pdf_operators_set_cairo_to_pdf_matrix (&surface->pdf_operators,
						  &surface->cairo_to_ps);
    _cairo_output_stream_printf (surface->stream, "  q\n");

    if (_cairo_surface_is_snapshot (recording_surface))
	free_me = recording_surface = _cairo_surface_snapshot_get_target (recording_surface);

    if (recording_surface->content == CAIRO_CONTENT_COLOR) {
	surface->content = CAIRO_CONTENT_COLOR;
	_cairo_output_stream_printf (surface->stream,
				     "  0 g %d %d %d %d rectfill\n",
				     surface->page_bbox.x,
				     surface->page_bbox.y,
				     surface->page_bbox.width,
				     surface->page_bbox.height);
    }

    status = _cairo_recording_surface_replay_region (recording_surface,
						     extents,
						     &surface->base,
						     CAIRO_RECORDING_REGION_NATIVE);
    assert (status != CAIRO_INT_STATUS_UNSUPPORTED);
    if (unlikely (status))
	goto err;

    status = _cairo_pdf_operators_flush (&surface->pdf_operators);
    if (unlikely (status))
	goto err;

    _cairo_output_stream_printf (surface->stream, "  Q\n");

    _cairo_surface_clipper_reset (&surface->clipper);
    surface->clipper = old_clipper;
    surface->content = old_content;
    surface->width = old_width;
    surface->height = old_height;
    surface->page_bbox = old_page_bbox;
    surface->current_pattern_is_solid_color = FALSE;
    _cairo_pdf_operators_reset (&surface->pdf_operators);
    surface->cairo_to_ps = old_cairo_to_ps;

    _cairo_pdf_operators_set_cairo_to_pdf_matrix (&surface->pdf_operators,
						  &surface->cairo_to_ps);

err:
    cairo_surface_destroy (free_me);
    return status;
}

static void
_cairo_ps_surface_flatten_transparency (cairo_ps_surface_t	*surface,
					const cairo_color_t	*color,
					double			*red,
					double			*green,
					double			*blue)
{
    *red   = color->red;
    *green = color->green;
    *blue  = color->blue;

    if (! CAIRO_COLOR_IS_OPAQUE (color)) {
	*red   *= color->alpha;
	*green *= color->alpha;
	*blue  *= color->alpha;
	if (surface->content == CAIRO_CONTENT_COLOR_ALPHA) {
	    double one_minus_alpha = 1. - color->alpha;
	    *red   += one_minus_alpha;
	    *green += one_minus_alpha;
	    *blue  += one_minus_alpha;
	}
    }
}

static void
_cairo_ps_surface_emit_solid_pattern (cairo_ps_surface_t    *surface,
				      cairo_solid_pattern_t *pattern)
{
    double red, green, blue;

    _cairo_ps_surface_flatten_transparency (surface, &pattern->color, &red, &green, &blue);

    if (color_is_gray (red, green, blue))
	_cairo_output_stream_printf (surface->stream,
				     "%f g\n",
				     red);
    else
	_cairo_output_stream_printf (surface->stream,
				     "%f %f %f rg\n",
				     red, green, blue);
}

static cairo_status_t
_cairo_ps_surface_emit_surface (cairo_ps_surface_t      *surface,
				cairo_pattern_t         *source_pattern,
				cairo_surface_t         *source_surface,
				cairo_operator_t	 op,
				int                      width,
				int                      height,
				cairo_bool_t             stencil_mask)
{
    cairo_int_status_t status;

    if (source_surface->type == CAIRO_SURFACE_TYPE_RECORDING) {
	if (source_surface->backend->type == CAIRO_SURFACE_TYPE_SUBSURFACE) {
	    cairo_surface_subsurface_t *sub = (cairo_surface_subsurface_t *) source_surface;
	    status = _cairo_ps_surface_emit_recording_subsurface (surface, sub->target, &sub->extents);
	} else {
	    status = _cairo_ps_surface_emit_recording_surface (surface, source_surface);
	}
    } else {
	cairo_image_surface_t *image = (cairo_image_surface_t *) source_surface;
	if (source_pattern->extend != CAIRO_EXTEND_PAD) {
	    status = _cairo_ps_surface_emit_jpeg_image (surface, source_surface,
							width, height);
	    if (status != CAIRO_INT_STATUS_UNSUPPORTED)
		return status;
	}

	status = _cairo_ps_surface_emit_image (surface, image,
					       op, source_pattern->filter, stencil_mask);
    }

    return status;
}


static void
_path_fixed_init_rectangle (cairo_path_fixed_t *path,
			    cairo_rectangle_int_t *rect)
{
    cairo_status_t status;

    _cairo_path_fixed_init (path);

    status = _cairo_path_fixed_move_to (path,
					_cairo_fixed_from_int (rect->x),
					_cairo_fixed_from_int (rect->y));
    assert (status == CAIRO_STATUS_SUCCESS);
    status = _cairo_path_fixed_rel_line_to (path,
					    _cairo_fixed_from_int (rect->width),
					    _cairo_fixed_from_int (0));
    assert (status == CAIRO_STATUS_SUCCESS);
    status = _cairo_path_fixed_rel_line_to (path,
					    _cairo_fixed_from_int (0),
					    _cairo_fixed_from_int (rect->height));
    assert (status == CAIRO_STATUS_SUCCESS);
    status = _cairo_path_fixed_rel_line_to (path,
					    _cairo_fixed_from_int (-rect->width),
					    _cairo_fixed_from_int (0));
    assert (status == CAIRO_STATUS_SUCCESS);

    status = _cairo_path_fixed_close_path (path);
    assert (status == CAIRO_STATUS_SUCCESS);
}

static cairo_status_t
_cairo_ps_surface_paint_surface (cairo_ps_surface_t     *surface,
				 cairo_pattern_t        *pattern,
				 cairo_rectangle_int_t  *extents,
				 cairo_operator_t	 op,
				 cairo_bool_t            stencil_mask)
{
    cairo_status_t status;
    int width, height;
    cairo_matrix_t cairo_p2d, ps_p2d;
    cairo_path_fixed_t path;
    double x_offset, y_offset;
    cairo_surface_t *source;
    cairo_image_surface_t *image = NULL;
    void *image_extra;

    status = _cairo_pdf_operators_flush (&surface->pdf_operators);
    if (unlikely (status))
	return status;

    status = _cairo_ps_surface_acquire_source_surface_from_pattern (surface,
								    pattern,
								    extents,
								    &width, &height,
								    &x_offset, &y_offset,
								    &source,
								    &image_extra);
    if (unlikely (status))
	return status;

    if (pattern->extend == CAIRO_EXTEND_PAD) {
	cairo_image_surface_t *img;

	assert (source->type == CAIRO_SURFACE_TYPE_IMAGE);
	img = (cairo_image_surface_t *) source;
	status = _cairo_ps_surface_create_padded_image_from_image (surface,
								   img,
								   &pattern->matrix,
								   extents,
								   &width, &height,
								   &x_offset, &y_offset,
								   &image);
	if (unlikely (status))
	    goto release_source;
    }

    _path_fixed_init_rectangle (&path, extents);
    status = _cairo_pdf_operators_clip (&surface->pdf_operators,
					&path,
					CAIRO_FILL_RULE_WINDING);
    _cairo_path_fixed_fini (&path);
    if (unlikely (status))
	goto release_source;

    cairo_p2d = pattern->matrix;

    if (surface->paginated_mode == CAIRO_PAGINATED_MODE_FALLBACK) {
	double x_scale = cairo_p2d.xx;
	double y_scale = cairo_p2d.yy;

	_cairo_output_stream_printf (surface->stream,
				     "%% Fallback Image: x=%f y=%f w=%d h=%d ",
				     -cairo_p2d.x0/x_scale,
				     -cairo_p2d.y0/y_scale,
				     (int)(width/x_scale),
				     (int)(height/y_scale));
	if (x_scale == y_scale) {
	    _cairo_output_stream_printf (surface->stream,
					 "res=%fppi ",
					 x_scale*72);
	} else {
	    _cairo_output_stream_printf (surface->stream,
					 "res=%fx%fppi ",
					 x_scale*72,
					 y_scale*72);
	}
	_cairo_output_stream_printf (surface->stream,
				     "size=%ld\n",
				     (long)width*height*3);
    } else {
	if (op == CAIRO_OPERATOR_SOURCE) {
	    _cairo_output_stream_printf (surface->stream,
					 "%d g 0 0 %f %f rectfill\n",
					 surface->content == CAIRO_CONTENT_COLOR ? 0 : 1,
					 surface->width,
					 surface->height);
	}
    }

    status = cairo_matrix_invert (&cairo_p2d);
    /* cairo_pattern_set_matrix ensures the matrix is invertible */
    assert (status == CAIRO_STATUS_SUCCESS);

    ps_p2d = surface->cairo_to_ps;
    cairo_matrix_multiply (&ps_p2d, &cairo_p2d, &ps_p2d);
    cairo_matrix_translate (&ps_p2d, x_offset, y_offset);
    cairo_matrix_translate (&ps_p2d, 0.0, height);
    cairo_matrix_scale (&ps_p2d, 1.0, -1.0);

    if (! _cairo_matrix_is_identity (&ps_p2d)) {
	_cairo_output_stream_printf (surface->stream,
				     "[ %f %f %f %f %f %f ] concat\n",
				     ps_p2d.xx, ps_p2d.yx,
				     ps_p2d.xy, ps_p2d.yy,
				     ps_p2d.x0, ps_p2d.y0);
    }

    status = _cairo_ps_surface_emit_surface (surface,
					     pattern,
					     image ? &image->base : source,
					     op,
					     width, height,
					     stencil_mask);

  release_source:
    if (image)
	cairo_surface_destroy (&image->base);

    _cairo_ps_surface_release_source_surface_from_pattern (surface, pattern, source, image_extra);

    return status;
}

static cairo_status_t
_cairo_ps_surface_emit_surface_pattern (cairo_ps_surface_t      *surface,
					cairo_pattern_t         *pattern,
					cairo_rectangle_int_t   *extents,
					cairo_operator_t	 op)
{
    cairo_status_t status;
    int pattern_width = 0; /* squelch bogus compiler warning */
    int pattern_height = 0; /* squelch bogus compiler warning */
    double xstep, ystep;
    cairo_matrix_t cairo_p2d, ps_p2d;
    cairo_bool_t old_use_string_datasource;
    double x_offset, y_offset;
    cairo_surface_t *source;
    cairo_image_surface_t *image = NULL;
    void *image_extra;

    cairo_p2d = pattern->matrix;
    status = cairo_matrix_invert (&cairo_p2d);
    /* cairo_pattern_set_matrix ensures the matrix is invertible */
    assert (status == CAIRO_STATUS_SUCCESS);

    status = _cairo_ps_surface_acquire_source_surface_from_pattern (surface,
								    pattern,
								    extents,
								    &pattern_width, &pattern_height,
								    &x_offset, &y_offset,
								    &source,
								    &image_extra);
    if (unlikely (status))
	return status;

    if (pattern->extend == CAIRO_EXTEND_PAD) {
	cairo_image_surface_t *img;

	assert (source->type == CAIRO_SURFACE_TYPE_IMAGE);
	img = (cairo_image_surface_t *) source;
	status = _cairo_ps_surface_create_padded_image_from_image (surface,
								   img,
								   &pattern->matrix,
								   extents,
								   &pattern_width, &pattern_height,
								   &x_offset, &y_offset,
								   &image);
	if (unlikely (status))
	    goto release_source;
    }
    if (unlikely (status))
	goto release_source;

    switch (pattern->extend) {
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
	break;
    }
    case CAIRO_EXTEND_REPEAT:
	xstep = pattern_width;
	ystep = pattern_height;
	break;
    case CAIRO_EXTEND_REFLECT:
	xstep = pattern_width*2;
	ystep = pattern_height*2;
	break;
	/* All the rest (if any) should have been analyzed away, so these
	 * cases should be unreachable. */
    default:
	ASSERT_NOT_REACHED;
	xstep = 0;
	ystep = 0;
    }

    _cairo_output_stream_printf (surface->stream,
				 "/CairoPattern {\n");

    old_use_string_datasource = surface->use_string_datasource;
    surface->use_string_datasource = TRUE;
    if (op == CAIRO_OPERATOR_SOURCE) {
	_cairo_output_stream_printf (surface->stream,
				     "%d g 0 0 %f %f rectfill\n",
				     surface->content == CAIRO_CONTENT_COLOR ? 0 : 1,
				     xstep, ystep);
    }
    status = _cairo_ps_surface_emit_surface (surface,
					     pattern,
					     image ? &image->base : source,
					     op,
					     pattern_width, pattern_height, FALSE);
    if (unlikely (status))
	goto release_source;

    surface->use_string_datasource = old_use_string_datasource;
    _cairo_output_stream_printf (surface->stream,
				 "} bind def\n");

    _cairo_output_stream_printf (surface->stream,
				 "<< /PatternType 1\n"
				 "   /PaintType 1\n"
				 "   /TilingType 1\n");
    _cairo_output_stream_printf (surface->stream,
				 "   /XStep %f /YStep %f\n",
				 xstep, ystep);

    if (pattern->extend == CAIRO_EXTEND_REFLECT) {
	_cairo_output_stream_printf (surface->stream,
				     "   /BBox [0 0 %d %d]\n"
				     "   /PaintProc {\n"
				     "      CairoPattern\n"
				     "      [-1 0 0  1 %d 0] concat CairoPattern\n"
				     "      [ 1 0 0 -1 0 %d] concat CairoPattern\n"
				     "      [-1 0 0  1 %d 0] concat CairoPattern\n"
				     "      CairoPattern\n"
				     "   } bind\n",
				     pattern_width*2, pattern_height*2,
				     pattern_width*2,
				     pattern_height*2,
				     pattern_width*2);
    } else {
	if (op == CAIRO_OPERATOR_SOURCE) {
	    _cairo_output_stream_printf (surface->stream,
					 "   /BBox [0 0 %f %f]\n",
					 xstep, ystep);
	} else {
	    _cairo_output_stream_printf (surface->stream,
					 "   /BBox [0 0 %d %d]\n",
					 pattern_width, pattern_height);
	}
	_cairo_output_stream_printf (surface->stream,
				     "   /PaintProc { CairoPattern }\n");
    }

    _cairo_output_stream_printf (surface->stream,
				 ">>\n");

    cairo_p2d = pattern->matrix;
    status = cairo_matrix_invert (&cairo_p2d);
    /* cairo_pattern_set_matrix ensures the matrix is invertible */
    assert (status == CAIRO_STATUS_SUCCESS);

    cairo_matrix_init_identity (&ps_p2d);
    cairo_matrix_translate (&ps_p2d, 0.0, surface->height);
    cairo_matrix_scale (&ps_p2d, 1.0, -1.0);
    cairo_matrix_multiply (&ps_p2d, &cairo_p2d, &ps_p2d);
    cairo_matrix_translate (&ps_p2d, 0.0, pattern_height);
    cairo_matrix_scale (&ps_p2d, 1.0, -1.0);

    _cairo_output_stream_printf (surface->stream,
				 "[ %f %f %f %f %f %f ]\n",
				 ps_p2d.xx, ps_p2d.yx,
				 ps_p2d.xy, ps_p2d.yy,
				 ps_p2d.x0, ps_p2d.y0);
    _cairo_output_stream_printf (surface->stream,
				 "makepattern setpattern\n");

  release_source:
    if (image)
	cairo_surface_destroy (&image->base);

    _cairo_ps_surface_release_source_surface_from_pattern (surface, pattern, source, image_extra);

    return status;
}

typedef struct _cairo_ps_color_stop {
    double offset;
    double color[4];
} cairo_ps_color_stop_t;

static void
_cairo_ps_surface_emit_linear_colorgradient (cairo_ps_surface_t     *surface,
					     cairo_ps_color_stop_t  *stop1,
					     cairo_ps_color_stop_t  *stop2)
{
    _cairo_output_stream_printf (surface->stream,
				 "   << /FunctionType 2\n"
				 "      /Domain [ 0 1 ]\n"
				 "      /C0 [ %f %f %f ]\n"
				 "      /C1 [ %f %f %f ]\n"
				 "      /N 1\n"
				 "   >>\n",
				 stop1->color[0],
				 stop1->color[1],
				 stop1->color[2],
				 stop2->color[0],
				 stop2->color[1],
				 stop2->color[2]);
}

static void
_cairo_ps_surface_emit_stitched_colorgradient (cairo_ps_surface_t    *surface,
					       unsigned int 	      n_stops,
					       cairo_ps_color_stop_t  stops[])
{
    unsigned int i;

    _cairo_output_stream_printf (surface->stream,
				 "<< /FunctionType 3\n"
				 "   /Domain [ 0 1 ]\n"
				 "   /Functions [\n");
    for (i = 0; i < n_stops - 1; i++)
	_cairo_ps_surface_emit_linear_colorgradient (surface, &stops[i], &stops[i+1]);

    _cairo_output_stream_printf (surface->stream, "   ]\n");

    _cairo_output_stream_printf (surface->stream, "   /Bounds [ ");
    for (i = 1; i < n_stops-1; i++)
	_cairo_output_stream_printf (surface->stream, "%f ", stops[i].offset);
    _cairo_output_stream_printf (surface->stream, "]\n");

    _cairo_output_stream_printf (surface->stream, "   /Encode [ 1 1 %d { pop 0 1 } for ]\n",
				 n_stops - 1);

    _cairo_output_stream_printf (surface->stream, ">>\n");
}

static void
calc_gradient_color (cairo_ps_color_stop_t *new_stop,
		     cairo_ps_color_stop_t *stop1,
		     cairo_ps_color_stop_t *stop2)
{
    int i;
    double offset = stop1->offset / (stop1->offset + 1.0 - stop2->offset);

    for (i = 0; i < 4; i++)
	new_stop->color[i] = stop1->color[i] + offset*(stop2->color[i] - stop1->color[i]);
}

#define COLOR_STOP_EPSILON 1e-6

static cairo_status_t
_cairo_ps_surface_emit_pattern_stops (cairo_ps_surface_t       *surface,
				      cairo_gradient_pattern_t *pattern)
{
    cairo_ps_color_stop_t *allstops, *stops;
    unsigned int i, n_stops;

    allstops = _cairo_malloc_ab ((pattern->n_stops + 2), sizeof (cairo_ps_color_stop_t));
    if (unlikely (allstops == NULL))
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    stops = &allstops[1];
    n_stops = pattern->n_stops;

    for (i = 0; i < n_stops; i++) {
	cairo_gradient_stop_t *stop = &pattern->stops[i];

	stops[i].color[0] = stop->color.red;
	stops[i].color[1] = stop->color.green;
	stops[i].color[2] = stop->color.blue;
	stops[i].color[3] = stop->color.alpha;
	stops[i].offset = pattern->stops[i].offset;
    }

    if (pattern->base.extend == CAIRO_EXTEND_REPEAT ||
	pattern->base.extend == CAIRO_EXTEND_REFLECT)
    {
	if (stops[0].offset > COLOR_STOP_EPSILON) {
	    if (pattern->base.extend == CAIRO_EXTEND_REFLECT)
		memcpy (allstops, stops, sizeof (cairo_ps_color_stop_t));
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
			sizeof (cairo_ps_color_stop_t));
	    } else {
		calc_gradient_color (&stops[n_stops], &stops[0], &stops[n_stops-1]);
	    }
	    n_stops++;
	}
	stops[n_stops-1].offset = 1.0;
    }

    for (i = 0; i < n_stops; i++) {
	double red, green, blue;
	cairo_color_t color;

	_cairo_color_init_rgba (&color,
				stops[i].color[0],
				stops[i].color[1],
				stops[i].color[2],
				stops[i].color[3]);
	_cairo_ps_surface_flatten_transparency (surface, &color,
						&red, &green, &blue);
	stops[i].color[0] = red;
	stops[i].color[1] = green;
	stops[i].color[2] = blue;
    }

    _cairo_output_stream_printf (surface->stream,
				 "/CairoFunction\n");
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
	cairo_ps_color_stop_t pad_stops[4];

	assert (pattern->base.extend == CAIRO_EXTEND_PAD);

	pad_stops[0] = pad_stops[1] = stops[0];
	pad_stops[2] = pad_stops[3] = stops[n_stops - 1];

	pad_stops[0].offset = 0;
	pad_stops[3].offset = 1;

	_cairo_ps_surface_emit_stitched_colorgradient (surface, 4, pad_stops);
    } else if (n_stops == 2) {
	/* no need for stitched function */
	_cairo_ps_surface_emit_linear_colorgradient (surface, &stops[0], &stops[1]);
    } else {
	/* multiple stops: stitch. XXX possible optimization: regulary spaced
	 * stops do not require stitching. XXX */
	_cairo_ps_surface_emit_stitched_colorgradient (surface, n_stops, stops);
    }
    _cairo_output_stream_printf (surface->stream,
				 "def\n");

    free (allstops);

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_ps_surface_emit_repeating_function (cairo_ps_surface_t       *surface,
					   cairo_gradient_pattern_t *pattern,
					   int                       begin,
					   int                       end)
{
    _cairo_output_stream_printf (surface->stream,
				 "/CairoFunction\n"
				 "<< /FunctionType 3\n"
				 "   /Domain [ %d %d ]\n"
				 "   /Functions [ %d {CairoFunction} repeat ]\n"
				 "   /Bounds [ %d 1 %d {} for ]\n",
				 begin,
                                 end,
				 end - begin,
				 begin + 1,
				 end - 1);

    if (pattern->base.extend == CAIRO_EXTEND_REFLECT) {
	_cairo_output_stream_printf (surface->stream, "   /Encode [ %d 1 %d { 2 mod 0 eq {0 1} {1 0} ifelse } for ]\n",
				     begin,
				     end - 1);
    } else {
	_cairo_output_stream_printf (surface->stream, "   /Encode [ %d 1 %d { pop 0 1 } for ]\n",
				     begin,
				     end - 1);
    }

    _cairo_output_stream_printf (surface->stream, ">> def\n");

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_ps_surface_emit_gradient (cairo_ps_surface_t       *surface,
				 cairo_gradient_pattern_t *pattern,
				 cairo_bool_t              is_ps_pattern)
{
    cairo_matrix_t pat_to_ps;
    cairo_circle_double_t start, end;
    double domain[2];
    cairo_status_t status;

    assert (pattern->n_stops != 0);

    status = _cairo_ps_surface_emit_pattern_stops (surface, pattern);
    if (unlikely (status))
	return status;

    pat_to_ps = pattern->base.matrix;
    status = cairo_matrix_invert (&pat_to_ps);
    /* cairo_pattern_set_matrix ensures the matrix is invertible */
    assert (status == CAIRO_STATUS_SUCCESS);
    cairo_matrix_multiply (&pat_to_ps, &pat_to_ps, &surface->cairo_to_ps);

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

    /* PS requires the first and last stop to be the same as the
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

	status = _cairo_ps_surface_emit_repeating_function (surface,
							    pattern,
							    repeat_begin,
							    repeat_end);
	if (unlikely (status))
	    return status;
    } else if (pattern->n_stops <= 2) {
	/* For EXTEND_NONE and EXTEND_PAD if there are only two stops a
	 * Type 2 function is used by itself without a stitching
	 * function. Type 2 functions always have the domain [0 1] */
	domain[0] = 0.0;
	domain[1] = 1.0;
    }

    if (is_ps_pattern) {
	_cairo_output_stream_printf (surface->stream,
				     "<< /PatternType 2\n"
				     "   /Shading\n");
    }

    if (pattern->base.type == CAIRO_PATTERN_TYPE_LINEAR) {
	_cairo_output_stream_printf (surface->stream,
				     "   << /ShadingType 2\n"
				     "      /ColorSpace /DeviceRGB\n"
				     "      /Coords [ %f %f %f %f ]\n",
				     start.center.x, start.center.y,
				     end.center.x, end.center.y);
    } else {
	_cairo_output_stream_printf (surface->stream,
				     "   << /ShadingType 3\n"
				     "      /ColorSpace /DeviceRGB\n"
				     "      /Coords [ %f %f %f %f %f %f ]\n",
				     start.center.x, start.center.y,
				     MAX (start.radius, 0),
				     end.center.x, end.center.y,
				     MAX (end.radius, 0));
    }

    if (pattern->base.extend != CAIRO_EXTEND_NONE) {
	_cairo_output_stream_printf (surface->stream,
                                     "      /Extend [ true true ]\n");
    } else {
	_cairo_output_stream_printf (surface->stream,
                                     "      /Extend [ false false ]\n");
    }

    if (domain[0] == 0.0 && domain[1] == 1.0) {
	_cairo_output_stream_printf (surface->stream,
				     "      /Function CairoFunction\n");
    } else {
	_cairo_output_stream_printf (surface->stream,
				     "      /Function <<\n"
				     "         /FunctionType 3\n"
				     "         /Domain [ 0 1 ]\n"
				     "         /Bounds [ ]\n"
				     "         /Encode [ %f %f ]\n"
				     "         /Functions [ CairoFunction ]\n"
				     "      >>\n",
				     domain[0], domain[1]);
    }

    _cairo_output_stream_printf (surface->stream,
				 "   >>\n");

    if (is_ps_pattern) {
	_cairo_output_stream_printf (surface->stream,
				     ">>\n"
				     "[ %f %f %f %f %f %f ]\n"
				     "makepattern setpattern\n",
				     pat_to_ps.xx, pat_to_ps.yx,
				     pat_to_ps.xy, pat_to_ps.yy,
				     pat_to_ps.x0, pat_to_ps.y0);
    } else {
	_cairo_output_stream_printf (surface->stream,
				     "shfill\n");
    }

    return status;
}

static cairo_status_t
_cairo_ps_surface_emit_mesh_pattern (cairo_ps_surface_t     *surface,
				     cairo_mesh_pattern_t   *pattern,
				     cairo_bool_t            is_ps_pattern)
{
    cairo_matrix_t pat_to_ps;
    cairo_status_t status;
    cairo_pdf_shading_t shading;
    int i;

    if (_cairo_array_num_elements (&pattern->patches) == 0)
        return CAIRO_INT_STATUS_NOTHING_TO_DO;

    pat_to_ps = pattern->base.matrix;
    status = cairo_matrix_invert (&pat_to_ps);
    /* cairo_pattern_set_matrix ensures the matrix is invertible */
    assert (status == CAIRO_STATUS_SUCCESS);

    cairo_matrix_multiply (&pat_to_ps, &pat_to_ps, &surface->cairo_to_ps);

    status = _cairo_pdf_shading_init_color (&shading, pattern);
    if (unlikely (status))
	return status;

    _cairo_output_stream_printf (surface->stream,
				 "currentfile\n"
				 "/ASCII85Decode filter /FlateDecode filter /ReusableStreamDecode filter\n");

    status = _cairo_ps_surface_emit_base85_string (surface,
						   shading.data,
						   shading.data_length,
						   CAIRO_PS_COMPRESS_DEFLATE,
						   FALSE);
    if (status)
	return status;

    _cairo_output_stream_printf (surface->stream,
				 "\n"
				 "/CairoData exch def\n");

    if (is_ps_pattern) {
	_cairo_output_stream_printf (surface->stream,
				     "<< /PatternType 2\n"
				     "   /Shading\n");
    }

    _cairo_output_stream_printf (surface->stream,
				 "   << /ShadingType %d\n"
				 "      /ColorSpace /DeviceRGB\n"
				 "      /DataSource CairoData\n"
				 "      /BitsPerCoordinate %d\n"
				 "      /BitsPerComponent %d\n"
				 "      /BitsPerFlag %d\n"
				 "      /Decode [",
				 shading.shading_type,
				 shading.bits_per_coordinate,
				 shading.bits_per_component,
				 shading.bits_per_flag);

    for (i = 0; i < shading.decode_array_length; i++)
	_cairo_output_stream_printf (surface->stream, "%f ", shading.decode_array[i]);

    _cairo_output_stream_printf (surface->stream,
				 "]\n"
				 "   >>\n");

    if (is_ps_pattern) {
	_cairo_output_stream_printf (surface->stream,
				     ">>\n"
				     "[ %f %f %f %f %f %f ]\n",
				     pat_to_ps.xx, pat_to_ps.yx,
				     pat_to_ps.xy, pat_to_ps.yy,
				     pat_to_ps.x0, pat_to_ps.y0);
	_cairo_output_stream_printf (surface->stream,
				     "makepattern\n"
				     "setpattern\n");
    } else {
	_cairo_output_stream_printf (surface->stream, "shfill\n");
    }

    _cairo_output_stream_printf (surface->stream,
				 "currentdict /CairoData undef\n");

    _cairo_pdf_shading_fini (&shading);

    return status;
}

static cairo_status_t
_cairo_ps_surface_emit_pattern (cairo_ps_surface_t *surface,
				const cairo_pattern_t *pattern,
				cairo_rectangle_int_t *extents,
				cairo_operator_t       op)
{
    cairo_status_t status;

    if (pattern->type == CAIRO_PATTERN_TYPE_SOLID) {
	cairo_solid_pattern_t *solid = (cairo_solid_pattern_t *) pattern;

	if (surface->current_pattern_is_solid_color == FALSE ||
	    ! _cairo_color_equal (&surface->current_color, &solid->color))
	{
	    status = _cairo_pdf_operators_flush (&surface->pdf_operators);
	    if (unlikely (status))
		return status;

	    _cairo_ps_surface_emit_solid_pattern (surface, (cairo_solid_pattern_t *) pattern);

	    surface->current_pattern_is_solid_color = TRUE;
	    surface->current_color = solid->color;
	}

	return CAIRO_STATUS_SUCCESS;
    }

    surface->current_pattern_is_solid_color = FALSE;
    status = _cairo_pdf_operators_flush (&surface->pdf_operators);
    if (unlikely (status))
	    return status;

    switch (pattern->type) {
    case CAIRO_PATTERN_TYPE_SOLID:

	_cairo_ps_surface_emit_solid_pattern (surface, (cairo_solid_pattern_t *) pattern);
	break;

    case CAIRO_PATTERN_TYPE_SURFACE:
    case CAIRO_PATTERN_TYPE_RASTER_SOURCE:
	status = _cairo_ps_surface_emit_surface_pattern (surface,
							 (cairo_pattern_t *)pattern,
							 extents,
							 op);
	if (unlikely (status))
	    return status;
	break;

    case CAIRO_PATTERN_TYPE_LINEAR:
    case CAIRO_PATTERN_TYPE_RADIAL:
	status = _cairo_ps_surface_emit_gradient (surface,
						  (cairo_gradient_pattern_t *) pattern,
						  TRUE);
	if (unlikely (status))
	    return status;
	break;

    case CAIRO_PATTERN_TYPE_MESH:
	status = _cairo_ps_surface_emit_mesh_pattern (surface,
						      (cairo_mesh_pattern_t *) pattern,
						      TRUE);
	if (unlikely (status))
	    return status;
	break;
    }

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_ps_surface_paint_gradient (cairo_ps_surface_t          *surface,
				  const cairo_pattern_t       *source,
				  const cairo_rectangle_int_t *extents)
{
    cairo_matrix_t pat_to_ps;
    cairo_status_t status;

    pat_to_ps = source->matrix;
    status = cairo_matrix_invert (&pat_to_ps);
    /* cairo_pattern_set_matrix ensures the matrix is invertible */
    assert (status == CAIRO_STATUS_SUCCESS);
    cairo_matrix_multiply (&pat_to_ps, &pat_to_ps, &surface->cairo_to_ps);

    if (! _cairo_matrix_is_identity (&pat_to_ps)) {
	_cairo_output_stream_printf (surface->stream,
				     "[%f %f %f %f %f %f] concat\n",
				     pat_to_ps.xx, pat_to_ps.yx,
				     pat_to_ps.xy, pat_to_ps.yy,
				     pat_to_ps.x0, pat_to_ps.y0);
    }

    if (source->type == CAIRO_PATTERN_TYPE_MESH) {
	status = _cairo_ps_surface_emit_mesh_pattern (surface,
						      (cairo_mesh_pattern_t *)source,
						      FALSE);
	if (unlikely (status))
	    return status;
    } else {
	status = _cairo_ps_surface_emit_gradient (surface,
						  (cairo_gradient_pattern_t *)source,
						  FALSE);
	if (unlikely (status))
	    return status;
    }

    return status;
}

static cairo_status_t
_cairo_ps_surface_paint_pattern (cairo_ps_surface_t           *surface,
				 const cairo_pattern_t        *source,
				 cairo_rectangle_int_t        *extents,
				 cairo_operator_t              op,
				 cairo_bool_t                  stencil_mask)
{
    switch (source->type) {
    case CAIRO_PATTERN_TYPE_SURFACE:
    case CAIRO_PATTERN_TYPE_RASTER_SOURCE:
       return _cairo_ps_surface_paint_surface (surface,
                                               (cairo_pattern_t *)source,
                                               extents,
                                               op,
					       stencil_mask);

    case CAIRO_PATTERN_TYPE_LINEAR:
    case CAIRO_PATTERN_TYPE_RADIAL:
    case CAIRO_PATTERN_TYPE_MESH:
	return _cairo_ps_surface_paint_gradient (surface,
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
    case CAIRO_PATTERN_TYPE_MESH:
	return TRUE;

    default:
	ASSERT_NOT_REACHED;
	return FALSE;
    }
}

static cairo_bool_t
_cairo_ps_surface_get_extents (void		       *abstract_surface,
			       cairo_rectangle_int_t   *rectangle)
{
    cairo_ps_surface_t *surface = abstract_surface;

    rectangle->x = 0;
    rectangle->y = 0;

    /* XXX: The conversion to integers here is pretty bogus, (not to
     * mention the aribitray limitation of width to a short(!). We
     * may need to come up with a better interface for get_extents.
     */
    rectangle->width  = ceil (surface->width);
    rectangle->height = ceil (surface->height);

    return TRUE;
}

static void
_cairo_ps_surface_get_font_options (void                  *abstract_surface,
				    cairo_font_options_t  *options)
{
    _cairo_font_options_init_default (options);

    cairo_font_options_set_hint_style (options, CAIRO_HINT_STYLE_NONE);
    cairo_font_options_set_hint_metrics (options, CAIRO_HINT_METRICS_OFF);
    cairo_font_options_set_antialias (options, CAIRO_ANTIALIAS_GRAY);
    _cairo_font_options_set_round_glyph_positions (options, CAIRO_ROUND_GLYPH_POS_OFF);
}

static cairo_int_status_t
_cairo_ps_surface_set_clip (cairo_ps_surface_t *surface,
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
_cairo_ps_surface_paint (void			*abstract_surface,
			 cairo_operator_t	 op,
			 const cairo_pattern_t	*source,
			 const cairo_clip_t	*clip)
{
    cairo_ps_surface_t *surface = abstract_surface;
    cairo_output_stream_t *stream = surface->stream;
    cairo_composite_rectangles_t extents;
    cairo_status_t status;

    status = _cairo_composite_rectangles_init_for_paint (&extents,
							 &surface->base,
							 op, source, clip);
    if (unlikely (status))
	return status;

    if (surface->paginated_mode == CAIRO_PAGINATED_MODE_ANALYZE) {
	status = _cairo_ps_surface_analyze_operation (surface, op, source, NULL, &extents.bounded);
	goto cleanup_composite;
    }

    assert (_cairo_ps_surface_operation_supported (surface, op, source, NULL, &extents.bounded));

#if DEBUG_PS
    _cairo_output_stream_printf (stream,
				 "%% _cairo_ps_surface_paint\n");
#endif

    status = _cairo_ps_surface_set_clip (surface, &extents);
    if (unlikely (status))
	goto cleanup_composite;

    if (_can_paint_pattern (source)) {
	status = _cairo_pdf_operators_flush (&surface->pdf_operators);
	if (unlikely (status))
	    goto cleanup_composite;

	_cairo_output_stream_printf (stream, "q\n");
	status = _cairo_ps_surface_paint_pattern (surface,
						  source,
						  &extents.bounded, op, FALSE);
	if (unlikely (status))
	    goto cleanup_composite;

	_cairo_output_stream_printf (stream, "Q\n");
    } else {
	status = _cairo_ps_surface_emit_pattern (surface, source, &extents.bounded, op);
	if (unlikely (status))
	    goto cleanup_composite;

	_cairo_output_stream_printf (stream, "0 0 %f %f rectfill\n",
				     surface->width, surface->height);
    }

cleanup_composite:
    _cairo_composite_rectangles_fini (&extents);
    return status;
}

static cairo_int_status_t
_cairo_ps_surface_mask (void			*abstract_surface,
			cairo_operator_t	 op,
			const cairo_pattern_t	*source,
			const cairo_pattern_t	*mask,
			const cairo_clip_t	*clip)
{
    cairo_ps_surface_t *surface = abstract_surface;
    cairo_output_stream_t *stream = surface->stream;
    cairo_composite_rectangles_t extents;
    cairo_status_t status;

    status = _cairo_composite_rectangles_init_for_mask (&extents,
							&surface->base,
							op, source, mask, clip);
    if (unlikely (status))
	return status;

    if (surface->paginated_mode == CAIRO_PAGINATED_MODE_ANALYZE) {
	status = _cairo_ps_surface_analyze_operation (surface, op, source, mask, &extents.bounded);
	goto cleanup_composite;
    }

    assert (_cairo_ps_surface_operation_supported (surface, op, source, mask, &extents.bounded));

#if DEBUG_PS
    _cairo_output_stream_printf (stream,
				 "%% _cairo_ps_surface_mask\n");
#endif

    status = _cairo_ps_surface_set_clip (surface, &extents);
    if (unlikely (status))
	goto cleanup_composite;

    status = _cairo_ps_surface_emit_pattern (surface, source, &extents.bounded, op);
    if (unlikely (status))
	goto cleanup_composite;

    _cairo_output_stream_printf (stream, "q\n");
    status = _cairo_ps_surface_paint_pattern (surface,
					      mask,
					      &extents.bounded, op, TRUE);
    if (unlikely (status))
	goto cleanup_composite;

    _cairo_output_stream_printf (stream, "Q\n");

cleanup_composite:
    _cairo_composite_rectangles_fini (&extents);
    return status;
}

static cairo_int_status_t
_cairo_ps_surface_stroke (void			*abstract_surface,
			  cairo_operator_t	 op,
			  const cairo_pattern_t	*source,
			  const cairo_path_fixed_t	*path,
			  const cairo_stroke_style_t	*style,
			  const cairo_matrix_t	*ctm,
			  const cairo_matrix_t	*ctm_inverse,
			  double		 tolerance,
			  cairo_antialias_t	 antialias,
			  const cairo_clip_t		*clip)
{
    cairo_ps_surface_t *surface = abstract_surface;
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
    {
	cairo_rectangle_int_t r;
	cairo_box_t b;

	status = _cairo_path_fixed_stroke_extents (path, style,
						   ctm, ctm_inverse,
						   tolerance,
						   &r);
	if (unlikely (status))
	    goto cleanup_composite;

	_cairo_box_from_rectangle (&b, &r);
	status = _cairo_composite_rectangles_intersect_mask_extents (&extents, &b);
	if (unlikely (status))
	    goto cleanup_composite;
    }

    if (surface->paginated_mode == CAIRO_PAGINATED_MODE_ANALYZE) {
	status = _cairo_ps_surface_analyze_operation (surface, op, source, NULL, &extents.bounded);
	goto cleanup_composite;
    }

    assert (_cairo_ps_surface_operation_supported (surface, op, source, NULL, &extents.bounded));

#if DEBUG_PS
    _cairo_output_stream_printf (surface->stream,
				 "%% _cairo_ps_surface_stroke\n");
#endif

    status = _cairo_ps_surface_set_clip (surface, &extents);
    if (unlikely (status))
	goto cleanup_composite;

    status = _cairo_ps_surface_emit_pattern (surface, source, &extents.bounded, op);
    if (unlikely (status))
	goto cleanup_composite;

    status = _cairo_pdf_operators_stroke (&surface->pdf_operators,
					  path,
					  style,
					  ctm,
					  ctm_inverse);

cleanup_composite:
    _cairo_composite_rectangles_fini (&extents);
    return status;
}

static cairo_int_status_t
_cairo_ps_surface_fill (void		*abstract_surface,
			cairo_operator_t	 op,
			const cairo_pattern_t	*source,
			const cairo_path_fixed_t*path,
			cairo_fill_rule_t	 fill_rule,
			double			 tolerance,
			cairo_antialias_t	 antialias,
			const cairo_clip_t		*clip)
{
    cairo_ps_surface_t *surface = abstract_surface;
    cairo_composite_rectangles_t extents;
    cairo_int_status_t status;

    status = _cairo_composite_rectangles_init_for_fill (&extents,
							&surface->base,
							op, source, path,
							clip);
    if (unlikely (status))
	return status;

    /* use the more accurate extents */
    {
	cairo_rectangle_int_t r;
	cairo_box_t b;

	_cairo_path_fixed_fill_extents (path,
					fill_rule,
					tolerance,
					&r);

	_cairo_box_from_rectangle (&b, &r);
	status = _cairo_composite_rectangles_intersect_mask_extents (&extents, &b);
	if (unlikely (status))
	    goto cleanup_composite;
    }

    if (surface->paginated_mode == CAIRO_PAGINATED_MODE_ANALYZE) {
	status = _cairo_ps_surface_analyze_operation (surface, op, source, NULL, &extents.bounded);
	goto cleanup_composite;
    }

    assert (_cairo_ps_surface_operation_supported (surface, op, source, NULL, &extents.bounded));

#if DEBUG_PS
    _cairo_output_stream_printf (surface->stream,
				 "%% _cairo_ps_surface_fill\n");
#endif

    status = _cairo_pdf_operators_flush (&surface->pdf_operators);
    if (unlikely (status))
	goto cleanup_composite;

    status = _cairo_ps_surface_set_clip (surface, &extents);
    if (unlikely (status))
	goto cleanup_composite;

    if (_can_paint_pattern (source)) {
	_cairo_output_stream_printf (surface->stream, "q\n");

	status =  _cairo_pdf_operators_clip (&surface->pdf_operators,
					     path,
					     fill_rule);
	if (unlikely (status))
	    goto cleanup_composite;

	status = _cairo_ps_surface_paint_pattern (surface,
						  source,
						  &extents.bounded, op, FALSE);
	if (unlikely (status))
	    goto cleanup_composite;

	_cairo_output_stream_printf (surface->stream, "Q\n");
	_cairo_pdf_operators_reset (&surface->pdf_operators);
    } else {
	status = _cairo_ps_surface_emit_pattern (surface, source, &extents.bounded, op);
	if (unlikely (status))
	    goto cleanup_composite;

	status = _cairo_pdf_operators_fill (&surface->pdf_operators,
					    path,
					    fill_rule);
    }

cleanup_composite:
    _cairo_composite_rectangles_fini (&extents);
    return status;
}

static cairo_bool_t
_cairo_ps_surface_has_show_text_glyphs	(void			*abstract_surface)
{
    return TRUE;
}

static cairo_int_status_t
_cairo_ps_surface_show_text_glyphs (void		       *abstract_surface,
				    cairo_operator_t	        op,
				    const cairo_pattern_t      *source,
				    const char                 *utf8,
				    int                         utf8_len,
				    cairo_glyph_t	       *glyphs,
				    int			        num_glyphs,
				    const cairo_text_cluster_t *clusters,
				    int                         num_clusters,
				    cairo_text_cluster_flags_t  cluster_flags,
				    cairo_scaled_font_t	       *scaled_font,
				    const cairo_clip_t	       *clip)
{
    cairo_ps_surface_t *surface = abstract_surface;
    cairo_composite_rectangles_t extents;
    cairo_bool_t overlap;
    cairo_status_t status;

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
	status = _cairo_ps_surface_analyze_operation (surface, op, source, NULL, &extents.bounded);
	goto cleanup_composite;
    }

    assert (_cairo_ps_surface_operation_supported (surface, op, source, NULL, &extents.bounded));

#if DEBUG_PS
    _cairo_output_stream_printf (surface->stream,
				 "%% _cairo_ps_surface_show_glyphs\n");
#endif

    status = _cairo_ps_surface_set_clip (surface, &extents);
    if (unlikely (status))
	goto cleanup_composite;

    status = _cairo_ps_surface_emit_pattern (surface, source, &extents.bounded, op);
    if (unlikely (status))
	goto cleanup_composite;

    status = _cairo_pdf_operators_show_text_glyphs (&surface->pdf_operators,
						    utf8, utf8_len,
						    glyphs, num_glyphs,
						    clusters, num_clusters,
						    cluster_flags,
						    scaled_font);

cleanup_composite:
    _cairo_composite_rectangles_fini (&extents);
    return status;
}

static const char **
_cairo_ps_surface_get_supported_mime_types (void		 *abstract_surface)
{
    return _cairo_ps_supported_mime_types;
}

static void
_cairo_ps_surface_set_paginated_mode (void			*abstract_surface,
				      cairo_paginated_mode_t	 paginated_mode)
{
    cairo_ps_surface_t *surface = abstract_surface;
    cairo_status_t status;

    surface->paginated_mode = paginated_mode;

    if (surface->clipper.clip != NULL) {
	status = _cairo_pdf_operators_flush (&surface->pdf_operators);

	_cairo_output_stream_printf (surface->stream, "Q q\n");
	_cairo_surface_clipper_reset (&surface->clipper);
    }
}

static cairo_int_status_t
_cairo_ps_surface_set_bounding_box (void		*abstract_surface,
				    cairo_box_t		*bbox)
{
    cairo_ps_surface_t *surface = abstract_surface;
    int i, num_comments;
    char **comments;
    int x1, y1, x2, y2;
    cairo_bool_t has_page_media, has_page_bbox;
    const char *page_media;

    x1 = floor (_cairo_fixed_to_double (bbox->p1.x));
    y1 = floor (surface->height - _cairo_fixed_to_double (bbox->p2.y));
    x2 = ceil (_cairo_fixed_to_double (bbox->p2.x));
    y2 = ceil (surface->height - _cairo_fixed_to_double (bbox->p1.y));

    surface->page_bbox.x = x1;
    surface->page_bbox.y = y1;
    surface->page_bbox.width  = x2 - x1;
    surface->page_bbox.height = y2 - y1;

    _cairo_output_stream_printf (surface->stream,
				 "%%%%Page: %d %d\n",
				 surface->num_pages,
				 surface->num_pages);

    _cairo_output_stream_printf (surface->stream,
				 "%%%%BeginPageSetup\n");

    has_page_media = FALSE;
    has_page_bbox = FALSE;
    num_comments = _cairo_array_num_elements (&surface->dsc_page_setup_comments);
    comments = _cairo_array_index (&surface->dsc_page_setup_comments, 0);
    for (i = 0; i < num_comments; i++) {
	_cairo_output_stream_printf (surface->stream,
				     "%s\n", comments[i]);
	if (strncmp (comments[i], "%%PageMedia:", 11) == 0)
	    has_page_media = TRUE;

	if (strncmp (comments[i], "%%PageBoundingBox:", 18) == 0)
	    has_page_bbox = TRUE;

	free (comments[i]);
	comments[i] = NULL;
    }
    _cairo_array_truncate (&surface->dsc_page_setup_comments, 0);

    if (!has_page_media && !surface->eps) {
	page_media = _cairo_ps_surface_get_page_media (surface);
	if (unlikely (page_media == NULL))
	    return _cairo_error (CAIRO_STATUS_NO_MEMORY);

	_cairo_output_stream_printf (surface->stream,
				     "%%%%PageMedia: %s\n",
				     page_media);
    }

    if (!has_page_bbox) {
	_cairo_output_stream_printf (surface->stream,
				     "%%%%PageBoundingBox: %d %d %d %d\n",
				     x1, y1, x2, y2);
    }

    _cairo_output_stream_printf (surface->stream,
                                 "%%%%EndPageSetup\n"
				 "q %d %d %d %d rectclip q\n",
				 surface->page_bbox.x,
				 surface->page_bbox.y,
				 surface->page_bbox.width,
				 surface->page_bbox.height);

    if (surface->num_pages == 1) {
	surface->bbox_x1 = x1;
	surface->bbox_y1 = y1;
	surface->bbox_x2 = x2;
	surface->bbox_y2 = y2;
    } else {
	if (x1 < surface->bbox_x1)
	    surface->bbox_x1 = x1;
	if (y1 < surface->bbox_y1)
	    surface->bbox_y1 = y1;
	if (x2 > surface->bbox_x2)
	    surface->bbox_x2 = x2;
	if (y2 > surface->bbox_y2)
	    surface->bbox_y2 = y2;
    }
    surface->current_pattern_is_solid_color = FALSE;
    _cairo_pdf_operators_reset (&surface->pdf_operators);

    return _cairo_output_stream_get_status (surface->stream);
}

static cairo_bool_t
_cairo_ps_surface_supports_fine_grained_fallbacks (void	    *abstract_surface)
{
    return TRUE;
}

static const cairo_surface_backend_t cairo_ps_surface_backend = {
    CAIRO_SURFACE_TYPE_PS,
    _cairo_ps_surface_finish,

    _cairo_default_context_create,

    NULL, /* create similar: handled by wrapper */
    NULL, /* create similar image */
    NULL, /* map to image */
    NULL, /* unmap image */

    _cairo_surface_default_source,
    NULL, /* acquire_source_image */
    NULL, /* release_source_image */
    NULL, /* snapshot */

    NULL, /* cairo_ps_surface_copy_page */
    _cairo_ps_surface_show_page,

    _cairo_ps_surface_get_extents,
    _cairo_ps_surface_get_font_options,

    NULL, /* flush */
    NULL, /* mark_dirty_rectangle */

    /* Here are the drawing functions */

    _cairo_ps_surface_paint, /* paint */
    _cairo_ps_surface_mask,
    _cairo_ps_surface_stroke,
    _cairo_ps_surface_fill,
    NULL, /* fill-stroke */
    NULL, /* show_glyphs */
    _cairo_ps_surface_has_show_text_glyphs,
    _cairo_ps_surface_show_text_glyphs,
    _cairo_ps_surface_get_supported_mime_types,
};

static const cairo_paginated_surface_backend_t cairo_ps_surface_paginated_backend = {
    _cairo_ps_surface_start_page,
    _cairo_ps_surface_set_paginated_mode,
    _cairo_ps_surface_set_bounding_box,
    NULL, /* _cairo_ps_surface_has_fallback_images, */
    _cairo_ps_surface_supports_fine_grained_fallbacks,
};
