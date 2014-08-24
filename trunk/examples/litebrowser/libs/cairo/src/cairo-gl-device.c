/* cairo - a vector graphics library with display and print output
 *
 * Copyright © 2009 Eric Anholt
 * Copyright © 2009 Chris Wilson
 * Copyright © 2005,2010 Red Hat, Inc
 * Copyright © 2010 Linaro Limited
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
 * The Initial Developer of the Original Code is Red Hat, Inc.
 *
 * Contributor(s):
 *	Benjamin Otte <otte@gnome.org>
 *	Carl Worth <cworth@cworth.org>
 *	Chris Wilson <chris@chris-wilson.co.uk>
 *	Eric Anholt <eric@anholt.net>
 *	Alexandros Frantzis <alexandros.frantzis@linaro.org>
 */

#include "cairoint.h"

#include "cairo-error-private.h"
#include "cairo-gl-private.h"

static void
_gl_lock (void *device)
{
    cairo_gl_context_t *ctx = (cairo_gl_context_t *) device;

    ctx->acquire (ctx);
}

static void
_gl_unlock (void *device)
{
    cairo_gl_context_t *ctx = (cairo_gl_context_t *) device;

    ctx->release (ctx);
}

static cairo_status_t
_gl_flush (void *device)
{
    cairo_gl_context_t *ctx;
    cairo_status_t status;

    status = _cairo_gl_context_acquire (device, &ctx);
    if (unlikely (status))
        return status;

    _cairo_gl_composite_flush (ctx);

    _cairo_gl_context_destroy_operand (ctx, CAIRO_GL_TEX_SOURCE);
    _cairo_gl_context_destroy_operand (ctx, CAIRO_GL_TEX_MASK);

    if (ctx->clip_region) {
        cairo_region_destroy (ctx->clip_region);
        ctx->clip_region = NULL;
    }

    ctx->current_target = NULL;
    ctx->current_operator = -1;
    ctx->vertex_size = 0;
    ctx->pre_shader = NULL;
    _cairo_gl_set_shader (ctx, NULL);

    ctx->dispatch.BindBuffer (GL_ARRAY_BUFFER, 0);

    glDisable (GL_SCISSOR_TEST);
    glDisable (GL_BLEND);

    return _cairo_gl_context_release (ctx, status);
}

static void
_gl_finish (void *device)
{
    cairo_gl_context_t *ctx = device;

    _gl_lock (device);

    _cairo_cache_fini (&ctx->gradients);

    _cairo_gl_context_fini_shaders (ctx);

    _gl_unlock (device);
}

static void
_gl_destroy (void *device)
{
    cairo_gl_context_t *ctx = device;
    int n;

    ctx->acquire (ctx);

    while (! cairo_list_is_empty (&ctx->fonts)) {
	cairo_gl_font_t *font;

	font = cairo_list_first_entry (&ctx->fonts,
				       cairo_gl_font_t,
				       link);

	cairo_list_del (&font->base.link);
	cairo_list_del (&font->link);
	free (font);
    }

    for (n = 0; n < ARRAY_LENGTH (ctx->glyph_cache); n++)
	_cairo_gl_glyph_cache_fini (ctx, &ctx->glyph_cache[n]);

    _cairo_array_fini (&ctx->tristrip_indices);

    cairo_region_destroy (ctx->clip_region);

    free (ctx->vb_mem);

    ctx->destroy (ctx);

    free (ctx);
}

static const cairo_device_backend_t _cairo_gl_device_backend = {
    CAIRO_DEVICE_TYPE_GL,

    _gl_lock,
    _gl_unlock,

    _gl_flush, /* flush */
    _gl_finish,
    _gl_destroy,
};

cairo_status_t
_cairo_gl_context_init (cairo_gl_context_t *ctx)
{
    cairo_status_t status;
    cairo_gl_dispatch_t *dispatch = &ctx->dispatch;
    int gl_version = _cairo_gl_get_version ();
    cairo_gl_flavor_t gl_flavor = _cairo_gl_get_flavor ();
    const char *env;
    int n;

    _cairo_device_init (&ctx->base, &_cairo_gl_device_backend);

    ctx->compositor = _cairo_gl_span_compositor_get ();

    /* XXX The choice of compositor should be made automatically at runtime.
     * However, it is useful to force one particular compositor whilst
     * testing.
     */
    env = getenv ("CAIRO_GL_COMPOSITOR");
    if (env) {
	if (strcmp(env, "msaa") == 0)
	    ctx->compositor = _cairo_gl_msaa_compositor_get ();
    }

    memset (ctx->glyph_cache, 0, sizeof (ctx->glyph_cache));
    cairo_list_init (&ctx->fonts);

    /* Support only GL version >= 1.3 */
    if (gl_version < CAIRO_GL_VERSION_ENCODE (1, 3))
	return _cairo_error (CAIRO_STATUS_DEVICE_ERROR);

    /* Check for required extensions */
    if (gl_flavor == CAIRO_GL_FLAVOR_DESKTOP) {
	if (_cairo_gl_has_extension ("GL_ARB_texture_non_power_of_two"))
	    ctx->tex_target = GL_TEXTURE_2D;
	else if (_cairo_gl_has_extension ("GL_ARB_texture_rectangle"))
	    ctx->tex_target = GL_TEXTURE_RECTANGLE;
	else
	    return _cairo_error (CAIRO_STATUS_DEVICE_ERROR);
    }
    else {
	if (_cairo_gl_has_extension ("GL_OES_texture_npot"))
	    ctx->tex_target = GL_TEXTURE_2D;
	else
	    return _cairo_error (CAIRO_STATUS_DEVICE_ERROR);
    }

    if (gl_flavor == CAIRO_GL_FLAVOR_DESKTOP &&
	gl_version < CAIRO_GL_VERSION_ENCODE (2, 1) &&
	! _cairo_gl_has_extension ("GL_ARB_pixel_buffer_object"))
	return _cairo_error (CAIRO_STATUS_DEVICE_ERROR);

    if (gl_flavor == CAIRO_GL_FLAVOR_ES &&
	! _cairo_gl_has_extension ("GL_EXT_texture_format_BGRA8888"))
	return _cairo_error (CAIRO_STATUS_DEVICE_ERROR);

    ctx->has_map_buffer = (gl_flavor == CAIRO_GL_FLAVOR_DESKTOP ||
			   (gl_flavor == CAIRO_GL_FLAVOR_ES &&
			    _cairo_gl_has_extension ("GL_OES_mapbuffer")));

    ctx->has_mesa_pack_invert =
	_cairo_gl_has_extension ("GL_MESA_pack_invert");

    ctx->has_packed_depth_stencil =
	((gl_flavor == CAIRO_GL_FLAVOR_DESKTOP &&
	 _cairo_gl_has_extension ("GL_EXT_packed_depth_stencil")) ||
	(gl_flavor == CAIRO_GL_FLAVOR_ES &&
	 _cairo_gl_has_extension ("GL_OES_packed_depth_stencil")));

    ctx->current_operator = -1;
    ctx->gl_flavor = gl_flavor;

    status = _cairo_gl_context_init_shaders (ctx);
    if (unlikely (status))
        return status;

    status = _cairo_cache_init (&ctx->gradients,
                                _cairo_gl_gradient_equal,
                                NULL,
                                (cairo_destroy_func_t) _cairo_gl_gradient_destroy,
                                CAIRO_GL_GRADIENT_CACHE_SIZE);
    if (unlikely (status))
        return status;

    if (! ctx->has_map_buffer) {
	ctx->vb_mem = _cairo_malloc_ab (CAIRO_GL_VBO_SIZE, 1);
	if (unlikely (ctx->vb_mem == NULL)) {
	    _cairo_cache_fini (&ctx->gradients);
	    return _cairo_error (CAIRO_STATUS_NO_MEMORY);
	}
    }

    _cairo_array_init (&ctx->tristrip_indices, sizeof(int));

    /* PBO for any sort of texture upload */
    dispatch->GenBuffers (1, &ctx->texture_load_pbo);
    dispatch->GenBuffers (1, &ctx->vbo);

    ctx->max_framebuffer_size = 0;
    glGetIntegerv (GL_MAX_RENDERBUFFER_SIZE, &ctx->max_framebuffer_size);
    ctx->max_texture_size = 0;
    glGetIntegerv (GL_MAX_TEXTURE_SIZE, &ctx->max_texture_size);
    ctx->max_textures = 0;
    glGetIntegerv (GL_MAX_TEXTURE_IMAGE_UNITS, &ctx->max_textures);

    for (n = 0; n < ARRAY_LENGTH (ctx->glyph_cache); n++)
	_cairo_gl_glyph_cache_init (&ctx->glyph_cache[n]);

    return CAIRO_STATUS_SUCCESS;
}

void
_cairo_gl_context_activate (cairo_gl_context_t *ctx,
                            cairo_gl_tex_t      tex_unit)
{
    if (ctx->max_textures <= (GLint) tex_unit) {
        if (tex_unit < 2) {
            _cairo_gl_composite_flush (ctx);
            _cairo_gl_context_destroy_operand (ctx, ctx->max_textures - 1);
        }
        glActiveTexture (ctx->max_textures - 1);
    } else {
        glActiveTexture (GL_TEXTURE0 + tex_unit);
    }
}

static void
_cairo_gl_ensure_framebuffer (cairo_gl_context_t *ctx,
                              cairo_gl_surface_t *surface)
{
    GLenum status;
    cairo_gl_dispatch_t *dispatch = &ctx->dispatch;

    if (likely (surface->fb))
        return;

    /* Create a framebuffer object wrapping the texture so that we can render
     * to it.
     */
    dispatch->GenFramebuffers (1, &surface->fb);
    dispatch->BindFramebuffer (GL_FRAMEBUFFER, surface->fb);
    dispatch->FramebufferTexture2D (GL_FRAMEBUFFER,
				    GL_COLOR_ATTACHMENT0,
				    ctx->tex_target,
				    surface->tex,
				    0);
#if CAIRO_HAS_GL_SURFACE
    glDrawBuffer (GL_COLOR_ATTACHMENT0);
    glReadBuffer (GL_COLOR_ATTACHMENT0);
#endif

    status = dispatch->CheckFramebufferStatus (GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
	const char *str;
	switch (status) {
	//case GL_FRAMEBUFFER_UNDEFINED: str= "undefined"; break;
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: str= "incomplete attachment"; break;
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: str= "incomplete/missing attachment"; break;
	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: str= "incomplete draw buffer"; break;
	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: str= "incomplete read buffer"; break;
	case GL_FRAMEBUFFER_UNSUPPORTED: str= "unsupported"; break;
	case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: str= "incomplete multiple"; break;
	default: str = "unknown error"; break;
	}

	fprintf (stderr,
		 "destination is framebuffer incomplete: %s [%#x]\n",
		 str, status);
    }
}

cairo_bool_t
_cairo_gl_ensure_stencil (cairo_gl_context_t *ctx,
			  cairo_gl_surface_t *surface)
{
	cairo_gl_dispatch_t *dispatch = &ctx->dispatch;
#if CAIRO_HAS_GL_SURFACE
	GLenum internal_format = GL_DEPTH_STENCIL;
#elif CAIRO_HAS_GLESV2_SURFACE
	GLenum internal_format = GL_DEPTH24_STENCIL8_OES;
#endif

	if (! _cairo_gl_surface_is_texture (surface))
		return TRUE; /* best guess for now, will check later */

	if (surface->depth_stencil)
		return TRUE;

	if (! ctx->has_packed_depth_stencil)
		return FALSE;

	_cairo_gl_ensure_framebuffer (ctx, surface);

	dispatch->GenRenderbuffers (1, &surface->depth_stencil);
	dispatch->BindRenderbuffer (GL_RENDERBUFFER, surface->depth_stencil);
	dispatch->RenderbufferStorage (GL_RENDERBUFFER, internal_format,
				       surface->width, surface->height);

	ctx->dispatch.FramebufferRenderbuffer (GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
					       GL_RENDERBUFFER, surface->depth_stencil);
	ctx->dispatch.FramebufferRenderbuffer (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
					       GL_RENDERBUFFER, surface->depth_stencil);
	if (dispatch->CheckFramebufferStatus (GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		ctx->dispatch.DeleteRenderbuffers (1, &surface->depth_stencil);
		surface->depth_stencil = 0;
		return FALSE;
	}

	return TRUE;
}

/*
 * Stores a parallel projection transformation in matrix 'm',
 * using column-major order.
 *
 * This is equivalent to:
 *
 * glLoadIdentity()
 * gluOrtho2D()
 *
 * The calculation for the ortho tranformation was taken from the
 * mesa source code.
 */
static void
_gl_identity_ortho (GLfloat *m,
		    GLfloat left, GLfloat right,
		    GLfloat bottom, GLfloat top)
{
#define M(row,col)  m[col*4+row]
    M(0,0) = 2.f / (right - left);
    M(0,1) = 0.f;
    M(0,2) = 0.f;
    M(0,3) = -(right + left) / (right - left);

    M(1,0) = 0.f;
    M(1,1) = 2.f / (top - bottom);
    M(1,2) = 0.f;
    M(1,3) = -(top + bottom) / (top - bottom);

    M(2,0) = 0.f;
    M(2,1) = 0.f;
    M(2,2) = -1.f;
    M(2,3) = 0.f;

    M(3,0) = 0.f;
    M(3,1) = 0.f;
    M(3,2) = 0.f;
    M(3,3) = 1.f;
#undef M
}

void
_cairo_gl_context_set_destination (cairo_gl_context_t *ctx,
                                   cairo_gl_surface_t *surface)
{
    if (ctx->current_target == surface && ! surface->needs_update)
        return;

    _cairo_gl_composite_flush (ctx);

    ctx->current_target = surface;
    surface->needs_update = FALSE;

    if (_cairo_gl_surface_is_texture (surface)) {
        _cairo_gl_ensure_framebuffer (ctx, surface);
        ctx->dispatch.BindFramebuffer (GL_FRAMEBUFFER, surface->fb);
    } else {
        ctx->make_current (ctx, surface);
        ctx->dispatch.BindFramebuffer (GL_FRAMEBUFFER, 0);

#if CAIRO_HAS_GL_SURFACE
        glDrawBuffer (GL_BACK_LEFT);
        glReadBuffer (GL_BACK_LEFT);
#endif
    }

    glViewport (0, 0, surface->width, surface->height);

    if (_cairo_gl_surface_is_texture (surface))
	_gl_identity_ortho (ctx->modelviewprojection_matrix,
			    0, surface->width, 0, surface->height);
    else
	_gl_identity_ortho (ctx->modelviewprojection_matrix,
			    0, surface->width, surface->height, 0);
}
