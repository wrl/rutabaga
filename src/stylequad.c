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

/**
 * drawing
 */

void
rtb_stylequad_draw(struct rtb_stylequad *self)
{
	struct rtb_shader *shader = &self->owner->window->shaders.stylequad;
	const struct rtb_style_texture_definition *texture;

	rtb_render_reset(self->owner);
	rtb_render_use_shader(self->owner, shader);

	glUniform2f(shader->texture_size, 0.f, 0.f);

	if (self->texture.currently_loaded) {
		texture = self->texture.currently_loaded;

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, self->texture.gl_handle);
		glUniform1i(shader->texture, 0);
		glUniform2f(shader->texture_size,
				texture->w, texture->h);

		rtb_render_quad(self->owner, RTB_QUAD(self));

		glBindTexture(GL_TEXTURE_2D, 0);
	} else if (self->cached_style.bg_color) {
		rtb_render_set_color(self->owner,
				self->cached_style.bg_color->r,
				self->cached_style.bg_color->g,
				self->cached_style.bg_color->b,
				self->cached_style.bg_color->a);

		rtb_render_quad(self->owner, RTB_QUAD(self));
	}

	if (self->cached_style.border_color) {
		rtb_render_set_color(self->owner,
				self->cached_style.border_color->r,
				self->cached_style.border_color->g,
				self->cached_style.border_color->b,
				self->cached_style.border_color->a);

		glLineWidth(2.f);
		rtb_render_quad_outline(self->owner, RTB_QUAD(self));
	}
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
load_textures(struct rtb_stylequad *self)
{
	struct rtb_rect tex_coords = {
		.as_float = {
			0.f, 1.f,
			1.f, 0.f
		}
	};

	if (self->cached_style.background_image == self->texture.currently_loaded)
		return;

	if (self->cached_style.background_image) {
		load_one_texture(self->texture.gl_handle,
				self->cached_style.background_image);
		rtb_quad_set_tex_coords(RTB_QUAD(self), &tex_coords);
	}

	self->texture.currently_loaded = self->cached_style.background_image;
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

	CACHE_TEXTURE(background_image, "background-image");
	load_textures(self);

#undef CACHE_TEXTURE
#undef CACHE_COLOR
#undef CACHE_PROP
#undef ASSIGN_AND_MAYBE_MARK_DIRTY
}

void
rtb_stylequad_update_geometry(struct rtb_stylequad *self)
{
	rtb_quad_set_vertices(RTB_QUAD(self), &self->owner->rect);
}

/**
 * lifecycle
 */

void
rtb_stylequad_init(struct rtb_stylequad *self, struct rtb_element *owner)
{
	self->owner = owner;
	rtb_quad_init(RTB_QUAD(self));

	glGenTextures(1, &self->texture.gl_handle);
}

void rtb_stylequad_fini(struct rtb_stylequad *self)
{
	glDeleteTextures(1, &self->texture.gl_handle);
	rtb_quad_fini(RTB_QUAD(self));
}
