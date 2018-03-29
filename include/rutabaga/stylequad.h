/**
 * rutabaga: an OpenGL widget toolkit
 * Copyright (c) 2013-2017 William Light.
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

#include <rutabaga/geometry.h>
#include <rutabaga/render.h>
#include <rutabaga/shader.h>
#include <rutabaga/quad.h>
#include <rutabaga/mat4.h>

typedef enum {
	RTB_STYLEQUAD_DRAW_BG_COLOR     = 0x01,
	RTB_STYLEQUAD_DRAW_BG_IMAGE     = 0x02,
	RTB_STYLEQUAD_DRAW_BORDER_IMAGE = 0x04,
	RTB_STYLEQUAD_DRAW_BORDER_COLOR = 0x08,

	RTB_STYLEQUAD_DRAW_ALL =
		RTB_STYLEQUAD_DRAW_BG_COLOR
		| RTB_STYLEQUAD_DRAW_BG_IMAGE
		| RTB_STYLEQUAD_DRAW_BORDER_IMAGE
		| RTB_STYLEQUAD_DRAW_BORDER_COLOR,
} rtb_stylequad_draw_mode_t;

struct rtb_stylequad {
	struct rtb_point offset;

	GLuint vertices;

	struct {
		const struct rtb_rgb_color *bg_color;
		const struct rtb_rgb_color *border_color;
	} properties;

	struct rtb_stylequad_texture {
		const struct rtb_style_texture_definition *definition;
		GLuint gl_handle;
		GLuint coords;
	} border_image, background_image;
};

void rtb_stylequad_draw(const struct rtb_stylequad *,
		struct rtb_render_context *, const struct rtb_point *center,
		rtb_stylequad_draw_mode_t);
void rtb_stylequad_draw_solid(const struct rtb_stylequad *self,
		struct rtb_render_context *ctx, const struct rtb_point *center);
void rtb_stylequad_draw_on_element(struct rtb_stylequad *,
		struct rtb_element *, rtb_stylequad_draw_mode_t);
void rtb_stylequad_draw_with_modelview(struct rtb_stylequad *,
		struct rtb_element *, const mat4 *modelview,
		rtb_stylequad_draw_mode_t);

int rtb_stylequad_set_border_image(struct rtb_stylequad *,
		const struct rtb_style_texture_definition *);
int rtb_stylequad_set_background_image(struct rtb_stylequad *,
		const struct rtb_style_texture_definition *);
int rtb_stylequad_set_background_color(struct rtb_stylequad *,
		const struct rtb_rgb_color *);
int rtb_stylequad_set_border_color(struct rtb_stylequad *,
		const struct rtb_rgb_color *);

void rtb_stylequad_update_geometry(struct rtb_stylequad *,
		const struct rtb_rect *);

void rtb_stylequad_init(struct rtb_stylequad *);
void rtb_stylequad_fini(struct rtb_stylequad *);
