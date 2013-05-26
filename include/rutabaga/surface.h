/**
 * rutabaga: an OpenGL widget toolkit
 * Copyright (c) 2013 William Light.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "rutabaga/rutabaga.h"
#include "rutabaga/types.h"
#include "rutabaga/object.h"
#include "rutabaga/render.h"
#include "rutabaga/mat4.h"

#define RTB_SURFACE(x) RTB_UPCAST(x, rtb_surface)

typedef enum {
	RTB_SURFACE_VALID,
	RTB_SURFACE_INVALID
} rtb_surface_state_t;

struct rtb_surface {
	RTB_INHERIT(rtb_object);

	/* private ********************************/
	GLuint fbo;
	GLuint texture;
	struct rtb_quad quad;

	rtb_surface_state_t surface_state;

	mat4 projection;

	struct rtb_render_context render_ctx;
};

void rtb_surface_blit(struct rtb_surface *);
void rtb_surface_draw_children(struct rtb_surface *, rtb_draw_state_t state);
void rtb_surface_invalidate(struct rtb_surface *);

int rtb_surface_init(struct rtb_surface *,
		struct rtb_object_implementation *impl);
void rtb_surface_fini(struct rtb_surface *);
