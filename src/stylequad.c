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

#include "rutabaga/rutabaga.h"
#include "rutabaga/element.h"
#include "rutabaga/render.h"
#include "rutabaga/style.h"
#include "rutabaga/quad.h"
#include "rutabaga/stylequad.h"
#include "rutabaga/window.h"

#include "private/util.h"

static const GLubyte texture_indices[] = {
	/* top (l, c, r) */
	 0,  2,  1,  3,  2,  0,
	 1,  7,  4,  2,  7,  1,
	 4,  6,  5,  7,  6,  4,

	/* middle (l, c, r) */
	 3,  9,  2,  8,  9,  3,
	 2, 12,  7,  9, 12,  2,
	 7, 13,  6, 12, 13,  7,

	/* bottom (l, c, r) */
	 8, 10,  9, 11, 10,  8,
	 9, 15, 12, 10, 15,  9,
	12, 14, 13, 15, 14, 12
};

static const GLubyte solid_indices[] = {
	2, 7, 9, 12
};

static const GLubyte outline_indices[] = {
	2, 7, 12, 9
};

/**
 * drawing
 */

static void
draw_solid(struct rtb_stylequad *self, const struct rtb_shader *shader,
		GLenum mode, const GLubyte *indices, GLsizei count)
{
	glBindBuffer(GL_ARRAY_BUFFER, self->vertices);
	glEnableVertexAttribArray(shader->vertex);
	glVertexAttribPointer(shader->vertex, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glDrawElements(mode, count, GL_UNSIGNED_BYTE, indices);

	glDisableVertexAttribArray(shader->vertex);
}

static void
draw_textured(struct rtb_stylequad *self, const struct rtb_shader *shader,
		GLenum mode, const GLubyte *indices, GLsizei count)
{
	glBindBuffer(GL_ARRAY_BUFFER, self->tex_coords);
	glEnableVertexAttribArray(shader->tex_coord);
	glVertexAttribPointer(shader->tex_coord,
			2, GL_FLOAT, GL_FALSE, 0, 0);

	draw_solid(self, shader, mode, indices, count);

	glDisableVertexAttribArray(shader->tex_coord);
}

static void
draw(struct rtb_stylequad *self, struct rtb_shader *shader)
{
	struct rtb_element *owner = self->owner;
	const struct rtb_style_texture_definition *texture;

	rtb_render_set_position(owner, self->offset.x, self->offset.y);
	glUniform2f(shader->texture_size, 0.f, 0.f);

	if (self->texture.definition) {
		texture = self->texture.definition;

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, self->texture.gl_handle);
		glUniform1i(shader->texture, 0);
		glUniform2f(shader->texture_size,
				texture->w, texture->h);

		draw_textured(self, shader, GL_TRIANGLES,
				texture_indices, ARRAY_LENGTH(texture_indices));

		glBindTexture(GL_TEXTURE_2D, 0);

		glUniform2f(shader->texture_size, 0.f, 0.f);
	} else if (self->cached_style.bg_color) {
		rtb_render_set_color(owner,
				self->cached_style.bg_color->r,
				self->cached_style.bg_color->g,
				self->cached_style.bg_color->b,
				self->cached_style.bg_color->a);

		draw_solid(self, shader, GL_TRIANGLE_STRIP,
				solid_indices, ARRAY_LENGTH(solid_indices));
	}

	if (self->cached_style.border_color) {
		rtb_render_set_color(owner,
				self->cached_style.border_color->r,
				self->cached_style.border_color->g,
				self->cached_style.border_color->b,
				self->cached_style.border_color->a);

		glLineWidth(1.f);

		draw_solid(self, shader, GL_LINE_LOOP,
				outline_indices, ARRAY_LENGTH(outline_indices));
	}
}

void
rtb_stylequad_draw(struct rtb_stylequad *self)
{
	struct rtb_shader *shader = &self->owner->window->shaders.stylequad;

	rtb_render_reset(self->owner);
	rtb_render_use_shader(self->owner, shader);

	draw(self, shader);
}

void
rtb_stylequad_draw_with_modelview(struct rtb_stylequad *self, mat4 *modelview)
{
	struct rtb_shader *shader = &self->owner->window->shaders.stylequad;

	rtb_render_reset(self->owner);
	rtb_render_use_shader(self->owner, shader);
	rtb_render_set_modelview(self->owner, modelview->data);

	draw(self, shader);
}

/**
 * updating
 */

static int
load_one_texture(GLuint dst_handle,
		const struct rtb_style_texture_definition *src)
{
	glBindTexture(GL_TEXTURE_2D, dst_handle);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
			src->w, src->h,
			0, GL_BGRA, GL_UNSIGNED_BYTE,
			RTB_ASSET_DATA(RTB_ASSET(src)));

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0);
	return 0;
}

static void
set_tex_coords(struct rtb_stylequad *self,
		const struct rtb_style_texture_definition *tx)
{
	GLfloat
		hpxl    = 1.f / tx->w,
		vpxl    = 1.f / tx->h,
		bdr_top = (tx->border.top    * vpxl),
		bdr_rgt = (tx->border.right  * hpxl),
		bdr_btm = (tx->border.bottom * vpxl),
		bdr_lft = (tx->border.left   * hpxl);

	GLfloat v[16][2] = {
		{0.f,           1.f},
		{0.f + bdr_lft, 1.f},
		{0.f + bdr_lft, 1.f - bdr_top},
		{0.f,           1.f - bdr_top},

		{1.f - bdr_rgt, 1.f},
		{1.f,           1.f},
		{1.f,           1.f - bdr_top},
		{1.f - bdr_rgt, 1.f - bdr_top},

		{0.f,           bdr_btm},
		{0.f + bdr_lft, bdr_btm},
		{0.f + bdr_lft, 0.f},
		{0.f,           0.f},

		{1.f - bdr_rgt, bdr_btm},
		{1.f,           bdr_btm},
		{1.f,           0.f},
		{1.f - bdr_rgt, 0.f},
	};

	glBindBuffer(GL_ARRAY_BUFFER, self->tex_coords);
	glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static void
load_textures(struct rtb_stylequad *self)
{
	if (self->cached_style.border_image == self->texture.definition)
		return;

	if (self->cached_style.border_image) {
		load_one_texture(self->texture.gl_handle,
				self->cached_style.border_image);
		set_tex_coords(self, self->cached_style.border_image);
	}

	self->texture.definition = self->cached_style.border_image;
}

void
rtb_stylequad_update_style(struct rtb_stylequad *self)
{
	const struct rtb_style_property_definition *prop;
	struct rtb_element *elem = self->owner;

#define ASSIGN_AND_MAYBE_MARK_DIRTY(dest, val) do {        \
	if (dest != val) rtb_elem_mark_dirty(elem);            \
	dest = val;                                            \
} while (0)

#define CACHE_PROP(dest, name, type, member) do {          \
	prop = rtb_style_query_prop(elem->style,               \
			elem->state, name, type, 0);                   \
	if (prop)                                              \
		ASSIGN_AND_MAYBE_MARK_DIRTY(                       \
				self->cached_style.dest, &prop->member);   \
	else                                                   \
		ASSIGN_AND_MAYBE_MARK_DIRTY(                       \
				self->cached_style.dest, NULL);            \
} while (0)

#define CACHE_COLOR(dest, name) \
		CACHE_PROP(dest, name, RTB_STYLE_PROP_COLOR, color)
#define CACHE_TEXTURE(dest, name) \
		CACHE_PROP(dest, name, RTB_STYLE_PROP_TEXTURE, texture)

	CACHE_COLOR(bg_color, "background-color");
	CACHE_COLOR(fg_color, "color");
	CACHE_COLOR(border_color, "border-color");

	CACHE_TEXTURE(border_image, "border-image");
	load_textures(self);

#undef CACHE_TEXTURE
#undef CACHE_COLOR
#undef CACHE_PROP
#undef ASSIGN_AND_MAYBE_MARK_DIRTY
}

void
rtb_stylequad_update_geometry(struct rtb_stylequad *self)
{
	struct rtb_element *owner = self->owner;
	struct rtb_rect r;

	r.x  = -(owner->w / 2.f);
	r.y  = -(owner->h / 2.f);
	r.x2 = -r.x;
	r.y2 = -r.y;

	self->offset.x = owner->x + r.x2;
	self->offset.y = owner->y + r.y2;

	glBindBuffer(GL_ARRAY_BUFFER, self->vertices);

	if (self->texture.definition) {
		const struct rtb_style_texture_definition *tx =
			self->texture.definition;
		unsigned int
			bdr_top = tx->border.top,
			bdr_rgt = tx->border.right,
			bdr_btm = tx->border.bottom,
			bdr_lft = tx->border.left;

		GLfloat v[16][2] = {
			{r.x,            r.y},
			{r.x  + bdr_lft, r.y},
			{r.x  + bdr_lft, r.y + bdr_top},
			{r.x,            r.y + bdr_top},

			{r.x2 - bdr_rgt, r.y},
			{r.x2,           r.y},
			{r.x2,           r.y + bdr_top},
			{r.x2 - bdr_rgt, r.y + bdr_top},

			{r.x,            r.y2 - bdr_btm},
			{r.x  + bdr_lft, r.y2 - bdr_btm},
			{r.x  + bdr_lft, r.y2},
			{r.x,            r.y2},

			{r.x2 - bdr_rgt, r.y2 - bdr_btm},
			{r.x2,           r.y2 - bdr_btm},
			{r.x2,           r.y2},
			{r.x2 - bdr_rgt, r.y2}
		};

		glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
	} else {
		GLfloat v[16][2] = {
			[2]  = {r.x,  r.y},
			[7]  = {r.x2, r.y},
			[12] = {r.x2, r.y2},
			[9]  = {r.x,  r.y2}
		};

		glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

/**
 * lifecycle
 */

void
rtb_stylequad_init(struct rtb_stylequad *self, struct rtb_element *owner)
{
	self->owner = owner;

	glGenBuffers(1, &self->vertices);
	glGenBuffers(1, &self->tex_coords);
	glGenTextures(1, &self->texture.gl_handle);
}

void rtb_stylequad_fini(struct rtb_stylequad *self)
{
	glDeleteTextures(1, &self->texture.gl_handle);
	glDeleteBuffers(1, &self->tex_coords);
	glDeleteBuffers(1, &self->vertices);
}
