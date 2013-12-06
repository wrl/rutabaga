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
#include "rutabaga/geometry.h"
#include "rutabaga/element.h"
#include "rutabaga/container.h"
#include "rutabaga/layout.h"
#include "rutabaga/layout-helpers.h"

/**
 * size
 */

void
rtb_size_self(struct rtb_element *elem,
		const struct rtb_size *avail, struct rtb_size *want)
{
	want->w = fmax(elem->w, elem->min_size.w);
	want->h = fmax(elem->h, elem->min_size.h);
}

void
rtb_size_hfit_children(struct rtb_element *elem,
		const struct rtb_size *avail, struct rtb_size *want)
{
	struct rtb_element *iter;
	struct rtb_size child, need = {-elem->inner_pad.x, 0.f}, zero = {0.f, 0.f};

	TAILQ_FOREACH(iter, &elem->children, child) {
		iter->size_cb(iter, &zero, &child);

		need.w += child.w + elem->inner_pad.x;
		need.h  = fmax(need.h, child.h);
	}

	need.w  = need.w + (elem->outer_pad.x * 2);
	need.h  = need.h + (elem->outer_pad.y * 2);

	want->w = fmax(need.w, elem->min_size.w);
	want->h = fmax(need.h, elem->min_size.h);
}

void
rtb_size_vfit_children(struct rtb_element *elem,
		const struct rtb_size *avail, struct rtb_size *want)
{
	struct rtb_element *iter;
	struct rtb_size child, need = {0.f, -elem->inner_pad.y}, zero = {0.f, 0.f};

	TAILQ_FOREACH(iter, &elem->children, child) {
		iter->size_cb(iter, &zero, &child);

		need.w  = fmax(need.w, child.w);
		need.h += child.h + elem->inner_pad.y;
	}

	need.w  = need.w + (elem->outer_pad.x * 2);
	need.h  = need.h + (elem->outer_pad.y * 2);

	want->w = fmax(need.w, elem->min_size.w);
	want->h = fmax(need.h, elem->min_size.h);
}

void
rtb_size_hfill(struct rtb_element *elem,
		const struct rtb_size *avail, struct rtb_size *want)
{
	rtb_size_hfit_children(elem, avail, want);
	want->w = fmax(want->w, avail->w);
}

void
rtb_size_vfill(struct rtb_element *elem,
		const struct rtb_size *avail, struct rtb_size *want)
{
	rtb_size_vfit_children(elem, avail, want);
	want->h = fmax(want->h, avail->h);
}

void
rtb_size_fill(struct rtb_element *elem,
		const struct rtb_size *avail, struct rtb_size *want)
{
	want->w = avail->w;
	want->h = avail->h;
}

/**
 * static layout (i.e. don't change anything)
 */

void
rtb_layout_unmanaged(struct rtb_element *elem)
{
	struct rtb_size avail, child;
	struct rtb_element *iter;

	avail = elem->inner_rect.size;

	TAILQ_FOREACH(iter, &elem->children, child) {
		iter->size_cb(iter, &avail, &child);
		rtb_elem_set_size(iter, &child);
	}
}

/**
 * vertical pack
 */

void
rtb_layout_vpack_top(struct rtb_element *elem)
{
	struct rtb_size avail, child;
	struct rtb_point position;
	struct rtb_element *iter;
	float xstart;

	avail = elem->inner_rect.size;
	xstart = elem->inner_rect.x;
	position.y = elem->inner_rect.y;

	TAILQ_FOREACH(iter, &elem->children, child) {
		iter->size_cb(iter, &avail, &child);
		position.x = xstart + halign(avail.w, child.w, iter->align);

		rtb_elem_set_position_from_point(iter, &position);
		rtb_elem_set_size(iter, &child);

		/* XXX: what if a child in the middle wants all the space,
		 *      and there are still children left to layout? */
		avail.h    -= child.h + elem->inner_pad.y;
		position.y += child.h + elem->inner_pad.y;
	}
}

void
rtb_layout_vpack_middle(struct rtb_element *elem)
{
	float children_height, xstart;
	struct rtb_size avail, child;
	struct rtb_point position;
	struct rtb_element *iter;

	avail = elem->inner_rect.size;
	xstart = elem->inner_rect.x;
	position.y = elem->inner_rect.y;

	children_height = -elem->inner_pad.y;
	TAILQ_FOREACH(iter, &elem->children, child) {
		iter->size_cb(iter, &avail, &child);
		children_height += child.h + elem->inner_pad.y;
	}

	position.y += (avail.h - children_height) / 2.f;

	if (avail.h < children_height)
		return rtb_layout_vpack_top(elem);

	TAILQ_FOREACH(iter, &elem->children, child) {
		iter->size_cb(iter, &avail, &child);
		position.x = xstart + halign(avail.w, child.w, iter->align);

		rtb_elem_set_position_from_point(iter, &position);
		rtb_elem_set_size(iter, &child);

		position.y += child.h + elem->inner_pad.y;
		avail.h    -= child.h + elem->inner_pad.y;
	}
}

void
rtb_layout_vpack_bottom(struct rtb_element *elem)
{
	struct rtb_size avail, child;
	struct rtb_point position;
	struct rtb_element *iter;
	float xstart;

	avail = elem->inner_rect.size;

	xstart = elem->inner_rect.x;
	position.y = elem->inner_rect.y2 + elem->inner_pad.y;

	TAILQ_FOREACH_REVERSE(iter, &elem->children, children, child) {
		iter->size_cb(iter, &avail, &child);
		position.x = xstart + halign(avail.w, child.w, iter->align);
		position.y -= child.h + elem->inner_pad.y;

		if (position.y < (elem->inner_rect.y))
			return rtb_layout_vpack_top(elem);

		rtb_elem_set_position_from_point(iter, &position);
		rtb_elem_set_size(iter, &child);

		avail.h -= child.h + elem->inner_pad.y;
	}
}

/**
 * horizontal pack
 */

void
rtb_layout_hpack_left(struct rtb_element *elem)
{
	struct rtb_size avail, child;
	struct rtb_point position;
	struct rtb_element *iter;
	float ystart;

	avail = elem->inner_rect.size;
	position.x = elem->inner_rect.x;
	ystart = elem->inner_rect.y;

	TAILQ_FOREACH(iter, &elem->children, child) {
		iter->size_cb(iter, &avail, &child);
		position.y = ystart + valign(avail.h, child.h, iter->align);

		rtb_elem_set_position_from_point(iter, &position);
		rtb_elem_set_size(iter, &child);

		avail.w    -= child.w + elem->inner_pad.x;
		position.x += child.w + elem->inner_pad.x;
	}
}

void
rtb_layout_hpack_center(struct rtb_element *elem)
{
	struct rtb_size avail, child;
	float children_width, ystart;
	struct rtb_point position;
	struct rtb_element *iter;

	avail = elem->inner_rect.size;
	position.x = elem->inner_rect.x;
	ystart = elem->inner_rect.y;

	children_width = -elem->inner_pad.x;
	TAILQ_FOREACH(iter, &elem->children, child) {
		iter->size_cb(iter, &avail, &child);
		children_width += child.w + elem->inner_pad.x;
	}

	position.x += (avail.w - children_width) / 2.f;

	if (avail.w < children_width)
		return rtb_layout_hpack_left(elem);

	TAILQ_FOREACH(iter, &elem->children, child) {
		iter->size_cb(iter, &avail, &child);
		position.y = ystart + valign(avail.h, child.h, iter->align);

		rtb_elem_set_position_from_point(iter, &position);
		rtb_elem_set_size(iter, &child);

		avail.w    -= child.w + elem->inner_pad.x;
		position.x += child.w + elem->inner_pad.x;
	}
}

void
rtb_layout_hpack_right(struct rtb_element *elem)
{
	struct rtb_size avail, child;
	float ystart;
	struct rtb_point position;
	struct rtb_element *iter;

	avail = elem->inner_rect.size;
	position.x = elem->inner_rect.x2 + elem->inner_pad.x;
	ystart = elem->inner_rect.y;

	TAILQ_FOREACH_REVERSE(iter, &elem->children, children, child) {
		iter->size_cb(iter, &avail, &child);
		position.x -= child.w + elem->inner_pad.x;
		position.y = ystart + valign(avail.h, child.h, iter->align);

		if (position.x < (elem->inner_rect.x))
			return rtb_layout_hpack_left(elem);

		rtb_elem_set_position_from_point(iter, &position);
		rtb_elem_set_size(iter, &child);

		avail.w -= child.w + elem->inner_pad.x;
	}
}

/**
 * distribute
 */

static void
hdistribute_one(struct rtb_element *elem, int child_width)
{
	struct rtb_element *only_child;
	struct rtb_point position;
	float w;

	w = elem->w;
	position.x = elem->x + (w / 2.f) - (child_width / 2.f);
	position.y = elem->inner_rect.y;

	if (position.x < (elem->inner_rect.x))
		return rtb_layout_hpack_left(elem);

	only_child = TAILQ_FIRST(&elem->children);
	rtb_elem_set_position_from_point(only_child, &position);
}

static void
hdistribute_many(struct rtb_element *elem,
		int children, int child0_width, int children_width)
{
	float xoff, yoff, w, pad;
	struct rtb_size avail;
	struct rtb_point position;
	struct rtb_element *iter;

	avail = elem->inner_rect.size;

	w = children_width + ((children - 1) * elem->inner_pad.x);

	if (avail.w < w)
		return rtb_layout_hpack_left(elem);

	xoff = elem->inner_rect.x;
	yoff = elem->inner_rect.y;

	pad = (avail.w - children_width) / (float) (children - 1);

	TAILQ_FOREACH(iter, &elem->children, child) {
		position.x = xoff;
		position.y = yoff + valign(avail.h, iter->h, iter->align);

		rtb_elem_set_position_from_point(iter, &position);

		xoff += iter->w + pad;
	}

	iter = TAILQ_LAST(&elem->children, children);
	iter->x = elem->inner_rect.x2 - iter->w;
}

void
rtb_layout_hdistribute(struct rtb_element *elem)
{
	int children, children_width, child0_width;
	struct rtb_size avail, child;
	struct rtb_element *iter;

	children = children_width = child0_width = 0;

	avail = elem->inner_rect.size;

	TAILQ_FOREACH(iter, &elem->children, child) {
		iter->size_cb(iter, &avail, &child);
		rtb_elem_set_size(iter, &child);

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
		return hdistribute_one(elem, child0_width);

	default:
		return hdistribute_many(elem, children, child0_width, children_width);
	}
}
