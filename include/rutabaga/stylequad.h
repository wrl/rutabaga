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

#include "rutabaga/geometry.h"
#include "rutabaga/quad.h"
#include "rutabaga/mat4.h"

struct rtb_stylequad {
	struct rtb_element *owner;
	struct rtb_point offset;

	GLuint tex_coords;
	GLuint vertices;

	struct {
		const struct rtb_rgb_color *bg_color;
		const struct rtb_rgb_color *fg_color;
		const struct rtb_rgb_color *border_color;

		const struct rtb_style_texture_definition *border_image;
	} cached_style;

	struct {
		const struct rtb_style_texture_definition *definition;
		GLuint gl_handle;
	} texture;
};

void rtb_stylequad_update_style(struct rtb_stylequad *);
void rtb_stylequad_update_geometry(struct rtb_stylequad *);

void rtb_stylequad_draw(struct rtb_stylequad *);
void rtb_stylequad_draw_with_modelview(struct rtb_stylequad *,
		mat4 *modelview);

void rtb_stylequad_init(struct rtb_stylequad *, struct rtb_element *owner);
void rtb_stylequad_fini(struct rtb_stylequad *);
