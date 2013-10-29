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

void
rtb_stylequad_draw(struct rtb_stylequad *self)
{
	int cleared = 0;
#define CLEAR() do {               \
	if (cleared)                   \
	break;                         \
	rtb_render_clear(self->owner); \
	cleared = 1;                   \
} while (0)

	if (self->cached_style.bg_color) {
		CLEAR();

		rtb_render_set_color(self->owner,
				self->cached_style.bg_color->r,
				self->cached_style.bg_color->g,
				self->cached_style.bg_color->b,
				self->cached_style.bg_color->a);

		rtb_render_quad(self->owner, RTB_QUAD(self));
	}

	if (self->cached_style.border_color) {
		CLEAR();

		rtb_render_set_color(self->owner,
				self->cached_style.border_color->r,
				self->cached_style.border_color->g,
				self->cached_style.border_color->b,
				self->cached_style.border_color->a);

		glLineWidth(2.f);
		rtb_render_quad_outline(self->owner, RTB_QUAD(self));
	}
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
#define CACHE_COLOR(dest, name) do {                       \
	prop = rtb_style_query_prop(elem->style,               \
			elem->state, name, RTB_STYLE_PROP_COLOR, 0);   \
	if (prop) {                                            \
		ASSIGN_AND_MAYBE_MARK_DIRTY(                       \
				self->cached_style.dest, &prop->color);    \
	} else                                                 \
		self->cached_style.dest = NULL;                    \
} while (0)
#define CACHE_TEXTURE(dest, name) do {                     \
	prop = rtb_style_query_prop(elem->style,               \
			elem->state, name, RTB_STYLE_PROP_TEXTURE, 0); \
	if (prop)                                              \
		ASSIGN_AND_MAYBE_MARK_DIRTY(                       \
				self->cached_style.dest, &prop->texture);  \
	else                                                   \
		self->cached_style.dest = NULL;                    \
} while (0)

	CACHE_COLOR(bg_color, "background-color");
	CACHE_COLOR(fg_color, "color");
	CACHE_COLOR(border_color, "border-color");

#undef CACHE_TEXTURE
#undef CACHE_COLOR
#undef ASSIGN_AND_MAYBE_MARK_DIRTY
}

void
rtb_stylequad_update_geometry(struct rtb_stylequad *self)
{
	rtb_quad_set_vertices(RTB_QUAD(self), &self->owner->rect);
}

void
rtb_stylequad_init(struct rtb_stylequad *self, struct rtb_element *owner)
{
	self->owner = owner;
	rtb_quad_init(RTB_QUAD(self));
}

void rtb_stylequad_fini(struct rtb_stylequad *self)
{
	rtb_quad_fini(RTB_QUAD(self));
}
