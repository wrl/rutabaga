/**
 * rutabaga: an OpenGL widget toolkit
 * Copyright (c) 2013-2018 William Light.
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

#include <rutabaga/rutabaga.h>
#include <rutabaga/font-manager.h>
#include <rutabaga/window.h>
#include <rutabaga/shader.h>

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
static const rtb_utf32_t default_cache[] = {
	' ', ',', '.', '!', '?', ';', '[', '\\', ']', '^', '_', '@', '{', '|', '}', '~', '\"', '#', '$', '%', '&', '\'', '(', ')',
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '*', '+', '-', '/', ':', '<', '=', '>', '`', 0
};

static int
init_txfont(struct rtb_texture_font *txfont, const rtb_utf32_t *cache)
{
	if (0)
		memcpy(txfont->txfont->lcd_weights, lcd_weights, sizeof(lcd_weights));

	texture_font_load_glyphs(txfont->txfont, cache ? cache : default_cache);
	return 0;
}

/**
 * txfont refcounting
 */

static int
rtb_texture_font_unref(struct rtb_texture_font *f)
{
	unsigned rc = --f->refcount;

	if (!rc) {
		if (f->loaded_from == RTB_FONT_EXTERNAL)
			free(f->location.path);

		texture_font_delete(f->txfont);
		free(f);
	}

	return rc;
}

/**
 * emebedded font
 */

static struct rtb_texture_font *
find_duplicate_embedded_txfont(const struct rtb_font_manager *fm,
		int pt_size, const void *base)
{
	struct rtb_font *font;

	TAILQ_FOREACH(font, &fm->managed_fonts, manager_entry) {
		if (font->size == pt_size
			&& font->txfont->loaded_from == RTB_FONT_EMBEDDED
			&& font->txfont->location.mem.base == base)
			return font->txfont;
	}

	return NULL;
}

int
rtb_font_manager_load_embedded_font(struct rtb_font_manager *fm,
		struct rtb_font *font, int pt_size, const void *base, size_t size)
{
	struct rtb_texture_font *txfont;

	font->size   = pt_size;
	font->fm     = fm;

	if ((txfont = find_duplicate_embedded_txfont(fm, pt_size, base))) {
		txfont->refcount++;
	} else {
		txfont = calloc(1, sizeof(*txfont));
		if (!txfont)
			goto err_calloc;

		txfont->txfont = texture_font_new_from_memory(
				fm->atlas, pt_size, base, size);

		if (!txfont->txfont)
			goto err_txfont_new;

		init_txfont(txfont, fm->cache_glyphs);
		txfont->refcount = 1;

		txfont->loaded_from       = RTB_FONT_EMBEDDED;
		txfont->location.mem.base = base;
		txfont->location.mem.size = size;
	}

	font->txfont = txfont;

	TAILQ_INSERT_TAIL(&fm->managed_fonts, font, manager_entry);
	return 0;

err_txfont_new:
	free(txfont);
err_calloc:
	return -1;
}

void
rtb_font_manager_free_embedded_font(struct rtb_font *font)
{
	TAILQ_REMOVE(&font->fm->managed_fonts, font, manager_entry);
	rtb_texture_font_unref(font->txfont);

	font->manager_entry.tqe_next = NULL;
	font->manager_entry.tqe_prev = NULL;
}

/**
 * external font
 */

static struct rtb_texture_font *
find_duplicate_external_txfont(const struct rtb_font_manager *fm,
		int pt_size, const char *path)
{
	struct rtb_font *font;

	TAILQ_FOREACH(font, &fm->managed_fonts, manager_entry) {
		if (font->size == pt_size
			&& font->txfont->loaded_from == RTB_FONT_EXTERNAL
			&& !strcmp(font->txfont->location.path, path))
			return font->txfont;
	}

	return NULL;
}

int
rtb_font_manager_load_external_font(struct rtb_font_manager *fm,
		struct rtb_external_font *font, int pt_size, const char *path)
{
	struct rtb_texture_font *txfont;

	font->size = pt_size;
	font->fm   = fm;

	if ((txfont = find_duplicate_external_txfont(fm, pt_size, path))) {
		txfont->refcount++;
	} else {
		txfont = calloc(1, sizeof(*txfont));
		if (!txfont)
			goto err_calloc;

		txfont->txfont = texture_font_new_from_file(
				fm->atlas, pt_size, path);

		if (!txfont->txfont)
			goto err_txfont_new;

		init_txfont(txfont, fm->cache_glyphs);
		txfont->refcount = 1;

		txfont->loaded_from   = RTB_FONT_EXTERNAL;
		txfont->location.path = strdup(path);
	}

	font->txfont = txfont;
	return 0;

err_txfont_new:
	ERR("couldn't load font \"%s\"\n", path);
	free(txfont);
err_calloc:
	return -1;
}

void
rtb_font_manager_free_external_font(struct rtb_external_font *font)
{
	free(font->path);
	rtb_texture_font_unref(font->txfont);
}

void
rtb_font_manager_set_dpi(struct rtb_font_manager *fm, int dpi_x, int dpi_y)
{
	struct rtb_font *f;

	texture_atlas_clear(fm->atlas);

	fm->atlas->dpi.x = dpi_x;
	fm->atlas->dpi.y = dpi_y;

	TAILQ_FOREACH(f, &fm->managed_fonts, manager_entry) {
		texture_font_delete(f->txfont->txfont);
		f->txfont->txfont = texture_font_new_from_memory(
			fm->atlas, f->size,
			f->txfont->location.mem.base, f->txfont->location.mem.size);

		init_txfont(f->txfont, fm->cache_glyphs);
	}
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
	CACHE_UNIFORM(tex);
	CACHE_UNIFORM(atlas_pixel);
	CACHE_UNIFORM(gamma);

#undef CACHE_UNIFORM

	fm->cache_glyphs = NULL;

#if defined(FT_CONFIG_OPTION_SUBPIXEL_RENDERING) \
	|| (FREETYPE_MAJOR > 2 \
		|| (FREETYPE_MAJOR == 2 && (FREETYPE_MINOR > 8 \
			|| (FREETYPE_MINOR == 8 && FREETYPE_PATCH >= 1))))
# define TEXTURE_ATLAS_DEPTH 3
#else
# define TEXTURE_ATLAS_DEPTH 1
#endif

	fm->atlas = texture_atlas_new(1024, 1024,
			TEXTURE_ATLAS_DEPTH, dpi_x, dpi_y);

#undef TEXTURE_ATLAS_DEPTH

	TAILQ_INIT(&fm->managed_fonts);
	return 0;

err_shader:
	return -1;
}

void
rtb_font_manager_fini(struct rtb_font_manager *fm)
{
	struct rtb_font *font;

	TAILQ_FOREACH(font, &fm->managed_fonts, manager_entry)
		rtb_texture_font_unref(font->txfont);

	texture_atlas_delete(fm->atlas);

	rtb_shader_free(RTB_SHADER(&fm->shader));
}
