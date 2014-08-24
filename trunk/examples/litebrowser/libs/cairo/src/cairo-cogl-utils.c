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
#include "cairo-cogl-utils-private.h"

#include <cogl/cogl.h>
#include <glib.h>

static cairo_status_t
_cogl_move_to (void		   *closure,
	       const cairo_point_t *point)
{
    cogl_path_move_to (closure,
		       _cairo_cogl_util_fixed_to_float (point->x),
		       _cairo_cogl_util_fixed_to_float (point->y));
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cogl_line_to (void		   *closure,
	       const cairo_point_t *point)
{
    cogl_path_line_to (closure,
		       _cairo_cogl_util_fixed_to_float (point->x),
		       _cairo_cogl_util_fixed_to_float (point->y));
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cogl_curve_to (void                *closure,
		const cairo_point_t *p0,
		const cairo_point_t *p1,
		const cairo_point_t *p2)
{
    cogl_path_curve_to (closure,
			_cairo_cogl_util_fixed_to_float (p0->x),
			_cairo_cogl_util_fixed_to_float (p0->y),
			_cairo_cogl_util_fixed_to_float (p1->x),
			_cairo_cogl_util_fixed_to_float (p1->y),
			_cairo_cogl_util_fixed_to_float (p2->x),
			_cairo_cogl_util_fixed_to_float (p2->y));
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cogl_close_path (void *closure)
{
    cogl_path_close (closure);
    return CAIRO_STATUS_SUCCESS;
}

CoglPath *
_cairo_cogl_util_path_from_cairo (const cairo_path_fixed_t *path,
				  cairo_fill_rule_t fill_rule,
				  float tolerance)
{
    CoglPath *cogl_path = cogl_path_new ();
    cairo_status_t status;

    if (fill_rule == CAIRO_FILL_RULE_EVEN_ODD)
	cogl_path_set_fill_rule (cogl_path, COGL_PATH_FILL_RULE_EVEN_ODD);
    else
	cogl_path_set_fill_rule (cogl_path, COGL_PATH_FILL_RULE_NON_ZERO);

#ifdef USE_CAIRO_PATH_FLATTENER
    /* XXX: rely on cairo to do path flattening, since it seems Cogl's
     * curve_to flattening is much slower */
    status = _cairo_path_fixed_interpret_flat (path,
					       _cogl_move_to,
					       _cogl_line_to,
					       _cogl_close_path,
					       cogl_path,
					       tolerance);
#else
    status = _cairo_path_fixed_interpret (path,
					  _cogl_move_to,
					  _cogl_line_to,
					  _cogl_curve_to,
					  _cogl_close_path,
					  cogl_path);
#endif

    assert (status == CAIRO_STATUS_SUCCESS);
    return cogl_path;
}

int
_cairo_cogl_util_next_p2 (int a)
{
  int rval = 1;

  while (rval < a)
    rval <<= 1;

  return rval;
}

