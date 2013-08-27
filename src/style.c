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
#include "rutabaga/window.h"
#include "rutabaga/style.h"

#include "private/util.h"

#include "style/definition.h"

static struct rtb_style no_style = {
	.for_type = "couldn't find a style for this element",
	.available_styles = RTB_STYLE_NORMAL,

	.states = {
		[RTB_DRAW_NORMAL] = {
			.fg = {RGB(0x00FF00), 1.f},
			.bg = {RGB(0xFF0000), 1.f}}
	}
};


static int
style_resolve(struct rtb_window *window, struct rtb_style *style)
{
	style->resolved_type = rtb_type_lookup(window, style->for_type);
	if (style->resolved_type)
		return 0;
	return -1;
}

static struct rtb_style *
style_for_type(struct rtb_type_atom *atom, struct rtb_style *style_list)
{
	for (; style_list->for_type; style_list++) {
		if (style_list->resolved_type &&
				rtb_is_type(style_list->resolved_type, atom))
			return style_list;
	}

	return &no_style;
}

/**
 * public API
 */

int
rtb_style_resolve_list(struct rtb_window *win, struct rtb_style *style_list)
{
	int unresolved_styles = 0;

	for (; style_list->for_type; style_list++) {
		if (style_resolve(win, style_list))
			unresolved_styles++;
	}

	return unresolved_styles;
}

void
rtb_style_apply_to_tree(struct rtb_element *root, struct rtb_style *style_list)
{
	struct rtb_element *iter;

	if (!root->style)
		root->style = style_for_type(RTB_TYPE_ATOM(root), style_list);

	TAILQ_FOREACH(iter, &root->children, child)
		rtb_style_apply_to_tree(iter, style_list);
}

struct rtb_style *
rtb_style_for_element(struct rtb_element *elem, struct rtb_style *style_list)
{
	return style_for_type(RTB_TYPE_ATOM(elem), style_list);
}

struct rtb_style *
rtb_style_get_defaults(void)
{
	return default_style;
}
