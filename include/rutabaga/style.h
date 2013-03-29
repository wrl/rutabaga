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
#include "rutabaga/atom.h"

typedef enum {
	RTB_STYLE_NORMAL = 1 << RTB_DRAW_NORMAL,
	RTB_STYLE_FOCUS  = 1 << RTB_DRAW_FOCUS,
	RTB_STYLE_HOVER  = 1 << RTB_DRAW_HOVER,
	RTB_STYLE_ACTIVE = 1 << RTB_DRAW_ACTIVE
} rtb_style_states_t;

struct rtb_rgb_color {
	GLfloat r, g, b, a;
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

void rtb_style_apply_to_tree(rtb_obj_t *root, rtb_style_t *style_list);
rtb_style_t *rtb_style_for_object(rtb_obj_t *obj, rtb_style_t *style_list);

int rtb_style_resolve_list(rtb_win_t *, rtb_style_t *style_list);
rtb_style_t *rtb_style_get_defaults(void);
