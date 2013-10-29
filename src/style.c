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
#include "rutabaga/asset.h"

#include "private/util.h"

/* generated as part of the build process */
#include "styles/default/style.h"

const static struct rtb_style_property_definition fallbacks[RTB_STYLE_PROP_TYPE_COUNT] = {
	[RTB_STYLE_PROP_COLOR] = {
		.property_name = "<fallback>",
		.type = RTB_STYLE_PROP_COLOR,
		.color = {RGB(0xFF0000), 1.f}
	},

	/* XXX: have a basic fallback font? */
	[RTB_STYLE_PROP_FONT] = {
		.property_name = "<fallback>",
		.type = RTB_STYLE_PROP_FONT,
		.font.face = NULL
	}
};

static rtb_draw_state_t
draw_state_for_elem_state(unsigned int state)
{
	switch (state) {
	case RTB_STATE_NORMAL:       return RTB_DRAW_NORMAL;
	case RTB_STATE_HOVER:        return RTB_DRAW_HOVER;
	case RTB_STATE_ACTIVE:       return RTB_DRAW_ACTIVE;

	case RTB_STATE_FOCUS:        return RTB_DRAW_FOCUS;
	case RTB_STATE_FOCUS_HOVER:  return RTB_DRAW_HOVER;
	case RTB_STATE_FOCUS_ACTIVE: return RTB_DRAW_ACTIVE;
	}

	return RTB_DRAW_NORMAL;
}

/**
 * style initialization
 */

static int
load_texture(struct rtb_style_texture_definition *def)
{
	struct rtb_asset *asset = RTB_ASSET(def);

	if (!RTB_ASSET_IS_LOADED(asset))
		if (rtb_asset_load(asset))
			return -1;

	return 0;
}

static int
load_font_face(struct rtb_style_font_face *face)
{
	struct rtb_asset *asset = RTB_ASSET(face);

	if (!RTB_ASSET_IS_LOADED(asset))
		if (rtb_asset_load(asset))
			return -1;

	return 0;
}

static int
load_assets(struct rtb_window *window,
		struct rtb_style_property_definition *property)
{
	int assets_loaded = 0;

	for(; property->property_name; property++) {
		switch (property->type) {
		case RTB_STYLE_PROP_TEXTURE:
			if (load_texture(&property->texture))
				return -1;

			assets_loaded++;
			break;

		case RTB_STYLE_PROP_FONT:
			if (load_font_face(property->font.face))
				return -1;

			if (rtb_font_manager_load_embedded_font(&window->font_manager,
						&property->font.font_internal, property->font.size,
						property->font.face->embedded.base,
						property->font.face->embedded.size))
				return -1;

			assets_loaded++;
			break;

		default:
			break;
		}
	}

	return assets_loaded;
}

static struct rtb_style *
style_for_type(struct rtb_type_atom *atom, struct rtb_style *s)
{
	for (; s->for_type; s++) {
		if (s->resolved_type && rtb_is_type(s->resolved_type, atom)) {
			return s;
		}
	}

	return NULL;
}

static struct rtb_style *
inherits_from(struct rtb_type_atom_descriptor *type,
		struct rtb_style *style_list)
{
	struct rtb_type_atom_descriptor *super;

	for (; style_list->for_type; style_list++) {
		for (super = type->super[0]; super; super = super->super[0]) {
			if (style_list->resolved_type == super)
				return style_list;
		}
	}

	return NULL;
}

static int
style_resolve(struct rtb_window *window, struct rtb_style *style)
{
	rtb_draw_state_t state;

	style->resolved_type = rtb_type_lookup(window, style->for_type);
	if (!style->resolved_type)
		return -1;

	style->inherit_from = inherits_from(style->resolved_type, style);

	for (state = 0; state < RTB_DRAW_STATE_COUNT; state++) {
		if (load_assets(window, style->properties[state]) < 0)
			printf("rutabaga: error loading assets for %s\n",
					style->for_type);
	}

	return 0;
}

/**
 * queries
 */

const struct rtb_style_property_definition *query_no_fallback(
		struct rtb_style *style_list, rtb_elem_state_t elem_state,
		const char *property_name, rtb_style_prop_type_t type)
{
	struct rtb_style_property_definition *prop;
	rtb_draw_state_t draw_state;

	draw_state = draw_state_for_elem_state(elem_state);

	for (; style_list; style_list = style_list->inherit_from) {
		prop = style_list->properties[draw_state];

		for (; !!prop->property_name; prop++)
			if (!strcmp(prop->property_name, property_name)
					&& prop->type == type)
				return prop;
	}

	return NULL;
}

const struct rtb_style_property_definition *query(
		struct rtb_style *style_list, rtb_elem_state_t elem_state,
		const char *property_name, rtb_style_prop_type_t type,
		int return_fallback)
{
	const struct rtb_style_property_definition *prop;

	if ((prop = query_no_fallback(style_list,
					elem_state, property_name, type)))
		return prop;

	switch (elem_state) {
	case RTB_STATE_FOCUS_HOVER:
	case RTB_STATE_FOCUS_ACTIVE:
		if ((prop = query_no_fallback(style_list,
						RTB_STATE_FOCUS, property_name, type)))
			return prop;

		/* fall-through */

	case RTB_STATE_FOCUS:
	case RTB_STATE_HOVER:
	case RTB_STATE_ACTIVE:
		if ((prop = query_no_fallback(style_list,
						RTB_STATE_NORMAL, property_name, type)))
			return prop;

	default:
		break;
	}

	if (return_fallback)
		return &fallbacks[type];
	return NULL;
}

const struct rtb_style_property_definition *rtb_style_query_prop(
		struct rtb_style *style_list, rtb_elem_state_t state,
		const char *property_name, rtb_style_prop_type_t type,
		int return_fallback)
{
	return query(style_list, state, property_name, type, return_fallback);
}

const struct rtb_style_property_definition *rtb_style_query_prop_in_tree(
		struct rtb_element *leaf, rtb_elem_state_t state,
		const char *property_name, rtb_style_prop_type_t type,
		int return_fallback)
{
	const struct rtb_style_property_definition *prop;
	struct rtb_element *root;

	root = RTB_ELEMENT(leaf->window);

	for (prop = NULL; !prop && leaf != root; leaf = leaf->parent)
		prop = query(leaf->style, state, property_name, type, return_fallback);

	return prop;
}

int
rtb_style_elem_has_properties_for_state(struct rtb_element *elem,
		rtb_elem_state_t state)
{
#define HAS_PROPS_FOR(elem_state) \
	(elem->style->properties[draw_state_for_elem_state(elem_state)]->property_name)

	/* exact match */
	if (HAS_PROPS_FOR(state))
		return 1;

	/* some fallbacks */
	switch (state) {
	case RTB_STATE_FOCUS_ACTIVE:
		if (HAS_PROPS_FOR(RTB_STATE_FOCUS))
			return 1;

	default:
		break;
	}

	return 0;
}

/**
 * public API
 */

int
rtb_style_resolve_list(struct rtb_window *win, struct rtb_style *style_list)
{
	int i, unresolved_styles;
	struct rtb_style *s;

	unresolved_styles = 0;

	for (i = 0; style_list[i].for_type; i++) {
		s = &style_list[i];

		if (style_resolve(win, s))
			unresolved_styles++;
	}

	for (i = 0; style_list[i].for_type; i++) {
		s = &style_list[i];
		if (!s->resolved_type)
			continue;

		s->inherit_from = inherits_from(s->resolved_type, style_list);
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
