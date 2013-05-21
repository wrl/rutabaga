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

#include <stdio.h>
#include <math.h>

#include "rutabaga/rutabaga.h"
#include "rutabaga/object.h"
#include "rutabaga/container.h"
#include "rutabaga/layout.h"
#include "rutabaga/layout-helpers.h"

/**
 * size
 */

void rtb_size_self(rtb_obj_t *obj, const struct rtb_size *avail,
		struct rtb_size *want)
{
	want->w = fmax(obj->w, obj->min_size.w);
	want->h = fmax(obj->h, obj->min_size.h);
}

void rtb_size_hfit_children(rtb_obj_t *obj, const struct rtb_size *avail,
		struct rtb_size *want)
{
	rtb_obj_t *iter;
	struct rtb_size child, need = {-obj->inner_pad.x, 0.f}, zero = {0.f, 0.f};

	TAILQ_FOREACH(iter, &obj->children, child) {
		iter->size_cb(iter, &zero, &child);

		need.w += child.w + obj->inner_pad.x;
		need.h  = fmax(need.h, child.h);
	}

	need.w  = need.w + (obj->outer_pad.x * 2);
	need.h  = need.h + (obj->outer_pad.y * 2);

	want->w = fmax(need.w, obj->min_size.w);
	want->h = fmax(need.h, obj->min_size.h);
}

void rtb_size_vfit_children(rtb_obj_t *obj, const struct rtb_size *avail,
		struct rtb_size *want)
{
	rtb_obj_t *iter;
	struct rtb_size child, need = {0.f, -obj->inner_pad.y}, zero = {0.f, 0.f};

	TAILQ_FOREACH(iter, &obj->children, child) {
		iter->size_cb(iter, &zero, &child);

		need.w  = fmax(need.w, child.w);
		need.h += child.h + obj->inner_pad.y;
	}

	need.w  = need.w + (obj->outer_pad.x * 2);
	need.h  = need.h + (obj->outer_pad.y * 2);

	want->w = fmax(need.w, obj->min_size.w);
	want->h = fmax(need.h, obj->min_size.h);
}

void rtb_size_hfill(rtb_obj_t *obj, const struct rtb_size *avail,
		struct rtb_size *want)
{
	rtb_size_hfit_children(obj, avail, want);
	want->w = fmax(want->w, avail->w);
}

void rtb_size_vfill(rtb_obj_t *obj, const struct rtb_size *avail,
		struct rtb_size *want)
{
	rtb_size_vfit_children(obj, avail, want);
	want->h = fmax(want->h, avail->h);
}

void rtb_size_fill(rtb_obj_t *obj, const struct rtb_size *avail,
		struct rtb_size *want)
{
	want->w = avail->w;
	want->h = avail->h;
}

/**
 * static layout (i.e. don't change anything)
 */

void rtb_layout_unmanaged(rtb_obj_t *obj)
{
	struct rtb_size avail, child;
	rtb_obj_t *iter;

	avail.w = obj->w - (obj->outer_pad.x * 2);
	avail.h = obj->h - (obj->outer_pad.y * 2);

	TAILQ_FOREACH(iter, &obj->children, child) {
		iter->size_cb(iter, &avail, &child);
		rtb_obj_set_size(iter, &child);
	}
}

/**
 * vertical pack
 */

void rtb_layout_vpack_top(rtb_obj_t *obj)
{
	struct rtb_size avail, child;
	struct rtb_point position;
	rtb_obj_t *iter;
	float xstart;

	avail.w = obj->w - (obj->outer_pad.x * 2);
	avail.h = obj->h - (obj->outer_pad.y * 2);

	xstart = obj->x + obj->outer_pad.x;
	position.y = obj->y + obj->outer_pad.y;

	TAILQ_FOREACH(iter, &obj->children, child) {
		iter->size_cb(iter, &avail, &child);
		position.x = xstart + halign(avail.w, child.w, iter->align);

		rtb_obj_set_position_from_point(iter, &position);
		rtb_obj_set_size(iter, &child);

		/* XXX: what if a child in the middle wants all the space,
		 *      and there are still children left to layout? */
		avail.h    -= child.h + obj->inner_pad.y;
		position.y += child.h + obj->inner_pad.y;
	}
}

void rtb_layout_vpack_middle(rtb_obj_t *obj)
{
	float children_height, xstart;
	struct rtb_size avail, child;
	struct rtb_point position;
	rtb_obj_t *iter;

	avail.w = obj->w - (obj->outer_pad.x * 2);
	avail.h = obj->h - (obj->outer_pad.y * 2);

	xstart = obj->x + obj->outer_pad.x;
	position.y = obj->y + obj->outer_pad.y;

	children_height = -obj->inner_pad.y;
	TAILQ_FOREACH(iter, &obj->children, child) {
		iter->size_cb(iter, &avail, &child);
		children_height += child.h + obj->inner_pad.y;
	}

	position.y += (avail.h - children_height) / 2.f;

	if (avail.h < children_height)
		return rtb_layout_vpack_top(obj);

	TAILQ_FOREACH(iter, &obj->children, child) {
		iter->size_cb(iter, &avail, &child);
		position.x = xstart + halign(avail.w, child.w, iter->align);

		rtb_obj_set_position_from_point(iter, &position);
		rtb_obj_set_size(iter, &child);

		position.y += child.h + obj->inner_pad.y;
		avail.h    -= child.h + obj->inner_pad.y;
	}
}

void rtb_layout_vpack_bottom(rtb_obj_t *obj)
{
	struct rtb_size avail, child;
	struct rtb_point position;
	rtb_obj_t *iter;
	float xstart;

	avail.w = obj->w - (obj->outer_pad.x * 2);
	avail.h = obj->h - (obj->outer_pad.y * 2);

	xstart = obj->x + obj->outer_pad.x;
	position.y = obj->y + obj->outer_pad.y + obj->inner_pad.y + avail.h;

	TAILQ_FOREACH_REVERSE(iter, &obj->children, children, child) {
		iter->size_cb(iter, &avail, &child);
		position.x = xstart + halign(avail.w, child.w, iter->align);
		position.y -= child.h + obj->inner_pad.y;

		if (position.y < (obj->y + obj->outer_pad.y))
			return rtb_layout_vpack_top(obj);

		rtb_obj_set_position_from_point(iter, &position);
		rtb_obj_set_size(iter, &child);

		avail.h -= child.h + obj->inner_pad.y;
	}
}

/**
 * horizontal pack
 */

void rtb_layout_hpack_left(rtb_obj_t *obj)
{
	struct rtb_size avail, child;
	struct rtb_point position;
	rtb_obj_t *iter;
	float ystart;

	avail.w = obj->w - (obj->outer_pad.x * 2);
	avail.h = obj->h - (obj->outer_pad.y * 2);

	position.x = obj->x + obj->outer_pad.x;
	ystart = obj->y + obj->outer_pad.y;

	TAILQ_FOREACH(iter, &obj->children, child) {
		iter->size_cb(iter, &avail, &child);
		position.y = ystart + valign(avail.h, child.h, iter->align);

		rtb_obj_set_position_from_point(iter, &position);
		rtb_obj_set_size(iter, &child);

		avail.w    -= child.w + obj->inner_pad.x;
		position.x += child.w + obj->inner_pad.x;
	}
}

void rtb_layout_hpack_center(rtb_obj_t *obj)
{
	struct rtb_size avail, child;
	float children_width, ystart;
	struct rtb_point position;
	rtb_obj_t *iter;

	avail.w = obj->w - (obj->outer_pad.x * 2);
	avail.h = obj->h - (obj->outer_pad.y * 2);

	position.x = obj->x + obj->outer_pad.x;
	ystart = obj->y + obj->outer_pad.y;

	children_width = -obj->inner_pad.x;
	TAILQ_FOREACH(iter, &obj->children, child) {
		iter->size_cb(iter, &avail, &child);
		children_width += child.w + obj->inner_pad.x;
	}

	position.x += (avail.w - children_width) / 2.f;

	if (avail.w < children_width)
		return rtb_layout_hpack_left(obj);

	TAILQ_FOREACH(iter, &obj->children, child) {
		iter->size_cb(iter, &avail, &child);
		position.y = ystart + valign(avail.h, child.h, iter->align);

		rtb_obj_set_position_from_point(iter, &position);
		rtb_obj_set_size(iter, &child);

		avail.w    -= child.w + obj->inner_pad.x;
		position.x += child.w + obj->inner_pad.x;
	}
}

void rtb_layout_hpack_right(rtb_obj_t *obj)
{
	struct rtb_size avail, child;
	float ystart;
	struct rtb_point position;
	rtb_obj_t *iter;

	avail.w = obj->w - (obj->outer_pad.x * 2);
	avail.h = obj->h - (obj->outer_pad.y * 2);

	position.x = obj->x + obj->outer_pad.x + avail.w + obj->inner_pad.x;
	ystart = obj->y + obj->outer_pad.y;

	TAILQ_FOREACH_REVERSE(iter, &obj->children, children, child) {
		iter->size_cb(iter, &avail, &child);
		position.x -= child.w + obj->inner_pad.x;
		position.y = ystart + valign(avail.h, child.h, iter->align);

		if (position.x < (obj->x + obj->outer_pad.x))
			return rtb_layout_hpack_left(obj);

		rtb_obj_set_position_from_point(iter, &position);
		rtb_obj_set_size(iter, &child);

		avail.w -= child.w + obj->inner_pad.x;
	}
}

/**
 * distribute
 */

void hdistribute_one(rtb_obj_t *obj, int child_width)
{
	rtb_obj_t *only_child;
	struct rtb_point position;
	float w;

	w = obj->w;
	position.x = obj->x + (w / 2.f) - (child_width / 2.f);
	position.y = obj->y + obj->outer_pad.y;

	if (position.x < (obj->x + obj->outer_pad.x))
		return rtb_layout_hpack_left(obj);

	only_child = TAILQ_FIRST(&obj->children);
	rtb_obj_set_position_from_point(only_child, &position);
}

static void hdistribute_many(rtb_obj_t *obj,
		int children, int child0_width, int children_width)
{
	float xoff, yoff, w, pad;
	struct rtb_size avail;
	struct rtb_point position;
	rtb_obj_t *iter;

	avail.w = obj->w - (obj->outer_pad.x * 2);
	avail.h = obj->h - (obj->outer_pad.y * 2);

	w = children_width + ((children - 1) * obj->inner_pad.x);

	if (avail.w < w)
		return rtb_layout_hpack_left(obj);

	xoff = obj->x + obj->outer_pad.x;
	yoff = obj->y + obj->outer_pad.y;

	pad = (avail.w - children_width) / (float) (children - 1);

	TAILQ_FOREACH(iter, &obj->children, child) {
		position.x = xoff;
		position.y = yoff + valign(avail.h, iter->h, iter->align);

		rtb_obj_set_position_from_point(iter, &position);

		xoff += iter->w + pad;
	}

	iter = TAILQ_LAST(&obj->children, children);
	iter->x = obj->x + avail.w + obj->outer_pad.x - iter->w;
}

void rtb_layout_hdistribute(rtb_obj_t *obj)
{
	int children, children_width, child0_width;
	struct rtb_size avail, child;
	rtb_obj_t *iter;

	children = children_width = child0_width = 0;

	avail.w = obj->w - (obj->outer_pad.x * 2);
	avail.h = obj->h - (obj->outer_pad.y * 2);

	TAILQ_FOREACH(iter, &obj->children, child) {
		iter->size_cb(iter, &avail, &child);
		rtb_obj_set_size(iter, &child);

		if (!child0_width)
			child0_width = child.w;

		children++;
		children_width += child.w;
		avail.w        -= child.w;
	}

	switch (children) {
	case 0:
		return;

	case 1:
		return hdistribute_one(obj, child0_width);

	default:
		return hdistribute_many(obj, children, child0_width, children_width);
	}
}
