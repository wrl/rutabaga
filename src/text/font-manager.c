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

#include <stdlib.h>
#include <string.h>

#include "rutabaga/rutabaga.h"
#include "rutabaga/font-manager.h"
#include "rutabaga/window.h"
#include "rutabaga/shader.h"

#include "shaders/text.glsl.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#define ERR(...) fprintf(stderr, "rutabaga: " __VA_ARGS__)

static const uint8_t lcd_weights[] = {
	0x00,
	0x55,
	0x56,
	0x55,
	0x00
};

/* characters we cache by default in the texture.
 *
 * since nobody can agree on what a wchar_t is (and, by extension, wchar
 * string literals), we have to do this disgusting-ass list. thanks,
 * microsoft. whoever decided to use utf-16 in windows can get fucked. */
static const rtb_utf32_t cache[] = {
	' ', ',', '.', '!', '?', ';', '[', '\\', ']', '^', '_', '@', '{', '|', '}', '~', '\"', '#', '$', '%', '&', '\'', '(', ')',
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '*', '+', '-', '/', ':', '<', '=', '>', '`'
};

static int
init_font(struct rtb_font *font)
{
	if (0)
		memcpy(font->txfont->lcd_weights, lcd_weights, sizeof(lcd_weights));

	texture_font_load_glyphs(font->txfont, cache);
	return 0;
}

/**
 * emebedded font
 */

int
rtb_font_manager_load_embedded_font(struct rtb_font_manager *fm,
		struct rtb_font *font, int pt_size, const void *base, size_t size)
{
	font->txfont =
		texture_font_new_from_memory(fm->atlas, pt_size, base, size);

	if (!font->txfont)
		return -1;

	font->size = pt_size;
	font->fm   = fm;

	init_font(font);
	return 0;
}

void
rtb_font_manager_free_embedded_font(struct rtb_font *font)
{
	texture_font_delete(font->txfont);
}

/**
 * external font
 */

int
rtb_font_manager_load_external_font(struct rtb_font_manager *fm,
		struct rtb_external_font *font, int pt_size, const char *path)
{
	font->txfont = texture_font_new_from_file(fm->atlas, pt_size, path);
	if (!font->txfont) {
		ERR("couldn't load font \"%s\"\n", path);
		return -1;
	}

	font->path = strdup(path);
	font->size = pt_size;
	font->fm   = fm;

	init_font(RTB_FONT(font));
	return 0;
}

void
rtb_font_manager_free_external_font(struct rtb_external_font *font)
{
	free(font->path);
	texture_font_delete(font->txfont);
}

int
rtb_font_manager_init(struct rtb_font_manager *fm, int dpi_x, int dpi_y)
{
	if (!rtb_shader_create(RTB_SHADER(&fm->shader),
				TEXT_VERT_SHADER, NULL, TEXT_FRAG_SHADER)) {
		ERR("couldn't compile text shader.\n");
		goto err_shader;
	}

#define CACHE_UNIFORM(UNIFORM) \
	fm->shader.UNIFORM = glGetUniformLocation(fm->shader.program, #UNIFORM)

	CACHE_UNIFORM(offset);
	CACHE_UNIFORM(texture);
	CACHE_UNIFORM(atlas_pixel);
	CACHE_UNIFORM(gamma);

#undef CACHE_UNIFORM

#ifdef FT_CONFIG_OPTION_SUBPIXEL_RENDERING
	fm->atlas = texture_atlas_new(512, 512, 3, dpi_x, dpi_y);
#else
	fm->atlas = texture_atlas_new(512, 512, 1, dpi_x, dpi_y);
#endif

	return 0;

err_shader:
	return -1;
}

void
rtb_font_manager_fini(struct rtb_font_manager *fm)
{
	texture_atlas_delete(fm->atlas);
	rtb_shader_free(RTB_SHADER(&fm->shader));
}
