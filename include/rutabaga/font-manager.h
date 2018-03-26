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

#include <bsd/queue.h>

#include <rutabaga/shader.h>

#include "freetype-gl/freetype-gl.h"
#include "freetype-gl/vertex-buffer.h"

#define RTB_FONT(x) RTB_UPCAST(x, rtb_font)
#define RTB_FONT_AS(x, type) RTB_DOWNCAST(x, type, rtb_font)

struct rtb_font {
	int size;
	float lcd_gamma;

	texture_font_t *txfont;
	struct rtb_font_manager *fm;

	TAILQ_ENTRY(rtb_font) manager_entry;
};

struct rtb_external_font {
	RTB_INHERIT(rtb_font);
	char *path;
};

struct rtb_font_manager {
	struct rtb_font_shader {
		RTB_INHERIT(rtb_shader);

		GLint atlas_pixel;
		GLint gamma;
	} shader;

	texture_atlas_t *atlas;

	TAILQ_HEAD(managed_fonts, rtb_font) managed_fonts;
};

int rtb_font_manager_load_embedded_font(struct rtb_font_manager *fm,
		struct rtb_font *font, int pt_size, const void *base, size_t size);
void rtb_font_manager_free_embedded_font(struct rtb_font *font);

int rtb_font_manager_load_external_font(struct rtb_font_manager *fm,
		struct rtb_external_font *font, int pt_size, const char *path);
void rtb_font_manager_free_external_font(struct rtb_external_font *font);

int rtb_font_manager_init(struct rtb_font_manager *, int dpi_x, int dpi_y);
void rtb_font_manager_fini(struct rtb_font_manager *);
