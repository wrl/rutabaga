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

#include <rutabaga/rutabaga.h>
#include <rutabaga/element.h>
#include <rutabaga/window.h>
#include <rutabaga/render.h>
#include <rutabaga/shader.h>
#include <rutabaga/style.h>
#include <rutabaga/geometry.h>

#include <rutabaga/text-object.h>

#include "freetype-gl/freetype-gl.h"
#include "freetype-gl/vertex-buffer.h"

#include "rtb_private/utf8.h"

struct text_vertex {
	float x, y;
	float s, t;
	float shift;
};

int
rtb_text_object_get_glyph_rect(struct rtb_text_object *self, int idx,
		struct rtb_rect *rect)
{
	vector_t *vertices = self->vertices->vertices;
	struct text_vertex *v;

	if (idx < 0 || ((size_t) idx * 4) > vector_size(vertices))
		return -1;

	v = (void *) vector_get(vertices, (idx * 4) - 4);

	/* upper left corner */
	rect->x = v->x;
	rect->y = v->y;

	/* lower right corner */
	v += 2;
	rect->x2 = v->x;
	rect->y2 = v->y;

	return 0;
}

int
rtb_text_object_count_glyphs(struct rtb_text_object *self)
{
	return vector_size(self->vertices->vertices) / 4;
}

static float
quantize(float x, float modulo, float modulo_recip, float *remainder)
{
	float ret = x * modulo_recip;
	float floored = floorf(ret);

	*remainder = (ret - floored) * modulo;

	return floored * modulo;
}

int
rtb_text_object_update(struct rtb_text_object *self,
		struct rtb_font *rfont, struct rtb_window *win,
		const rtb_utf8_t *text, float line_height_multiplier)
{
	float x, y, line_height, x0, y0, x1, y1, max_w, scale_x_recip;
	struct rtb_point scale = win->scale_recip;
	rtb_utf32_t codepoint, prev_codepoint;
	uint32_t state, prev_state;
	texture_font_t *font;
	unsigned lines;

	if (!rfont || !text)
		return -1;

	font = rfont->txfont;
	self->font = rfont;
	scale_x_recip = win->scale.x;

	vertex_buffer_clear(self->vertices);

	line_height = (font->height * line_height_multiplier) * scale.y;

	x  = 0.f;
	x1 = 0.f;
	y  = ceilf(line_height / 2.f)
		- (font->descender * scale.y)
		+ win->scale.y;

	max_w = 0.f;
	lines = 1;

	state = prev_state = UTF8_ACCEPT;
	prev_codepoint = 0;

	for (; *text; prev_state = state, text++) {
		texture_glyph_t *glyph;
		float s0, t0, s1, t1, x0_shift, x1_shift;

		switch(u8dec(&state, &codepoint, *text)) {
		case UTF8_ACCEPT:
			break;

		case UTF8_REJECT:
			if (prev_state != UTF8_ACCEPT)
				text--;

			codepoint = 0xFFFD;
			state = UTF8_ACCEPT;
			break;

		default:
			continue;
		}

		if (codepoint == '\n') {
			lines++;

			if (x > max_w)
				max_w = x;

			y += line_height;
			x = x1 = 0.f;
			continue;
		}

		glyph = texture_font_get_glyph(font, codepoint);
		if (!glyph)
			continue;

		if (prev_codepoint)
			x += (texture_glyph_get_kerning(glyph, prev_codepoint) * scale.x);

		s0 = glyph->s0;
		s1 = glyph->s1;

		t0 = glyph->t0;
		t1 = glyph->t1;

		x0 = x  + (glyph->offset_x * scale.x);
		x1 = x0 + (glyph->width * scale.x);
		y0 = y  - (glyph->offset_y * scale.y);
		y1 = y0 + (glyph->height * scale.y);

		x0 = quantize(x0, scale.x, win->scale.x, &x0_shift);
		x1 = quantize(x1, scale.x, win->scale.x, &x1_shift);

		GLuint indices[6] = {0, 1, 2, 0, 2, 3};
		struct text_vertex vertices[4] = {
			{x0, y0, s0, t0, x0_shift},
			{x0, y1, s0, t1, x0_shift},
			{x1, y1, s1, t1, x1_shift},
			{x1, y0, s1, t0, x1_shift}
		};

		vertex_buffer_push_back(self->vertices, vertices, 4, indices, 6);

		x += glyph->advance_x * scale.x;
		prev_codepoint = codepoint;
	}

	vertex_buffer_upload(self->vertices);
	self->h = line_height * lines;
	self->w = roundf((x > max_w) ? x : max_w);

	return 0;
}

void
rtb_text_object_render(struct rtb_text_object *self,
		struct rtb_render_context *ctx, float x, float y,
		const struct rtb_rgb_color *color)
{
	struct rtb_font_shader *shader;
	struct rtb_font_manager *fm;
	texture_atlas_t *atlas;

	if (!vertex_buffer_size(self->vertices))
		return;

	fm = self->fm;
	shader = &fm->shader;
	atlas = fm->atlas;

	rtb_render_use_shader(ctx, RTB_SHADER(shader));
	glBindTexture(GL_TEXTURE_2D, atlas->id);

	glUniform1i(shader->texture, 0);
	glUniform1f(shader->gamma, self->font->lcd_gamma);

	glUniform3f(shader->atlas_pixel,
			1.f / atlas->width, 1.f / atlas->height, atlas->depth);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUniform2f(shader->offset, x, y);
	rtb_render_set_color(ctx,
			color->r, color->g, color->b, color->a);
	vertex_buffer_render(self->vertices, GL_TRIANGLES);
}

struct rtb_text_object *
rtb_text_object_new(struct rtb_font_manager *fm)
{
	struct rtb_text_object *self = calloc(1, sizeof(*self));

	self->fm = fm;
	self->vertices = vertex_buffer_new("vertex:2f,tex_coord:2f,subpixel_shift:1f");

	return self;
}

void
rtb_text_object_free(struct rtb_text_object *self)
{
	vertex_buffer_delete(self->vertices);
	free(self);
}
