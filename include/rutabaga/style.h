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

#include "rutabaga/types.h"
#include "rutabaga/geometry.h"
#include "rutabaga/asset.h"
#include "rutabaga/atom.h"

typedef enum {
	RTB_STYLE_NORMAL = 1 << RTB_DRAW_NORMAL,
	RTB_STYLE_FOCUS  = 1 << RTB_DRAW_FOCUS,
	RTB_STYLE_HOVER  = 1 << RTB_DRAW_HOVER,
	RTB_STYLE_ACTIVE = 1 << RTB_DRAW_ACTIVE
} rtb_style_states_t;

/**
 * property types
 */

struct rtb_style_texture_definition {
	RTB_INHERIT(rtb_asset);

	RTB_INHERIT_AS(rtb_size, size);
	struct {
		unsigned int left, right, top, bottom;
	} padding;
};

struct rtb_rgb_color {
	GLfloat r, g, b, a;
};

struct rtb_style_property {
	char *name;

	enum {
		RTB_STYLE_PROP_COLOR,
		RTB_STYLE_PROP_FLOAT,
		RTB_STYLE_PROP_INT,
		RTB_STYLE_PROP_FONT,
		RTB_STYLE_PROP_TEXTURE
	} type;
};

struct rtb_style_property_definition {
	/* public *********************************/
	char *for_prop;

	union {
		struct rtb_rgb_color color;
		float flt;
		int i;
		struct rtb_style_texture_definition texture;
	};

	/* private ********************************/
	struct rtb_style_property *prop;
};

struct rtb_style_props {
	struct rtb_rgb_color fg, bg;
};

struct rtb_style {
	/* public *********************************/
	char *for_type;
	rtb_style_states_t available_styles;

	struct rtb_style_props states[RTB_DRAW_STATE_MAX + 1];

	/* private ********************************/
	rtb_type_atom_descriptor_t *resolved_type;
};

/**
 * public API
 */

void rtb_style_apply_to_tree(struct rtb_object *root,
		struct rtb_style *style_list);
struct rtb_style *rtb_style_for_object(struct rtb_object *obj,
		struct rtb_style *style_list);

int rtb_style_resolve_list(struct rtb_window *,
		struct rtb_style *style_list);
struct rtb_style *rtb_style_get_defaults(void);
