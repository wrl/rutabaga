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

#pragma once

#include <rutabaga/types.h>
#include <rutabaga/font-manager.h>
#include <rutabaga/geometry.h>
#include <rutabaga/element.h>
#include <rutabaga/asset.h>
#include <rutabaga/atom.h>

typedef enum {
	RTB_STYLE_PROP_COLOR = 0,
	RTB_STYLE_PROP_FLOAT,
	RTB_STYLE_PROP_INT,
	RTB_STYLE_PROP_FONT,
	RTB_STYLE_PROP_TEXTURE,

	RTB_STYLE_PROP_TYPE_COUNT
} rtb_style_prop_type_t;

typedef enum {
	RTB_TEXTURE_VERTICAL_STRETCH   = 0x0,
	RTB_TEXTURE_HORIZONTAL_STRETCH = 0x0,
	RTB_TEXTURE_VERTICAL_TILE      = 0x1,
	RTB_TEXTURE_HORIZONTAL_TILE    = 0x2,

	/* set if a border image should also draw its middle portion.
	 * ignored for background images. */
	RTB_TEXTURE_FILL               = 0x4
} rtb_style_texture_flags_t;

/**
 * property types
 */

struct rtb_style_font_face {
	RTB_INHERIT(rtb_asset);
	const char *family;
	const char *weight;
};

struct rtb_style_font_definition {
	const struct rtb_style_font_face *face;
	float lcd_gamma;
	int size;

	/* private ********************************/
	size_t slot;
};

struct rtb_style_texture_definition {
	RTB_INHERIT(rtb_asset);
	RTB_INHERIT_AS(rtb_size, size);

	rtb_style_texture_flags_t flags;

	struct {
		unsigned int top, right, bottom, left;
	} border;
};

struct rtb_rgb_color {
	GLfloat r, g, b, a;
};

/**
 * the meat and potatoes
 */

struct rtb_style_property_definition {
	/* public *********************************/
	const char *property_name;
	rtb_style_prop_type_t type;

	union {
		struct rtb_rgb_color color;
		float flt;
		int i;
		struct rtb_style_texture_definition texture;
		struct rtb_style_font_definition font;
	};
};

struct rtb_style {
	/* public *********************************/
	const char *for_type;
	const struct rtb_style_property_definition *properties[RTB_DRAW_STATE_COUNT];

	/* private ********************************/
	struct rtb_style *inherit_from;
	struct rtb_type_atom_descriptor *resolved_type;
};

struct rtb_style_data {
	struct rtb_style *style;
	size_t nfonts;
};

/**
 * public API
 */

const struct rtb_style_property_definition *rtb_style_query_prop(
		struct rtb_element *elem, const char *property_name,
		rtb_style_prop_type_t type, int should_return_fallback);
const struct rtb_style_property_definition *rtb_style_query_prop_in_tree(
		struct rtb_element *leaf, const char *property_name,
		rtb_style_prop_type_t type, int should_return_fallback);
int rtb_style_elem_has_properties_for_state(struct rtb_element *elem,
		rtb_elem_state_t state);

void rtb_style_apply_to_tree(struct rtb_element *root,
		struct rtb_style *style_list);
struct rtb_style *rtb_style_for_element(struct rtb_element *elem,
		struct rtb_style *style_list);

int rtb_style_resolve_list(struct rtb_window *,
		struct rtb_style *style_list);

struct rtb_font *rtb_style_get_font_for_def(struct rtb_window *,
		const struct rtb_style_font_definition *);

struct rtb_style_data rtb_style_get_defaults(void);
