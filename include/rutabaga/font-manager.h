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

#include "rutabaga/shader.h"

#include "freetype-gl/freetype-gl.h"
#include "freetype-gl/vertex-buffer.h"

typedef struct rtb_font rtb_font_t;

struct rtb_font {
	char *path;
	int size;

	float lcd_gamma;

	texture_font_t *txfont;
	rtb_font_manager_t *fm;
};

struct rtb_font_manager {
	rtb_win_t *win;

	struct rtb_font_shader {
		RTB_INHERIT(rtb_shader_program);

		GLint texture;
		GLint atlas_pixel;
		GLint gamma;
	} shader;

	struct {
		rtb_font_t main;
		rtb_font_t big;
		rtb_font_t monospace;
	} fonts;

	texture_atlas_t *atlas;
};

int rtb_font_manager_load_font(rtb_font_manager_t *fm, rtb_font_t *font,
		const char *path, int size);
void rtb_font_manager_free_font(rtb_font_t *font);

int  rtb_font_manager_init(rtb_win_t *win);
void rtb_font_manager_fini(rtb_win_t *win);
