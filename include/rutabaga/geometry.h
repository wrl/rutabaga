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
#include <rutabaga/rutabaga.h>

#define RTB_POINT(x) RTB_UPCAST(x, rtb_point)
#define RTB_POINT_IN_RECT(pt, rect) \
	((pt).x >= (rect).x && (pt).x <= (rect).x2 \
	 && (pt).y >= (rect).y && (pt).y <= (rect).y2)

#define RTB_MAKE_PHY_POINT(X, Y) ((struct rtb_phy_point) {.x = (X), .y = (Y)})

struct rtb_phy_point {
	int x;
	int y;
};

struct rtb_phy_size {
	int w;
	int h;
};

#define RTB_MAKE_POINT(X, Y) ((struct rtb_point) {.x = (X), .y = (Y)})

struct rtb_point {
	GLfloat x;
	GLfloat y;
};

struct rtb_size {
	GLfloat w;
	GLfloat h;
};

struct rtb_padding {
	struct rtb_point;
};

struct rtb_rect {
	/**
	 * nasty looking union here, but supports accessing members through
	 * several different ways:
	 *
	 *     struct rtb_rect rect;
	 *
	 *     rect.x  == rect.p[0].x
	 *     rect.y  == rect.p[0].y
	 *     rect.x2 == rect.p[1].x
	 *     rect.y2 == rect.p[1].y
	 *
	 *     rect.x  == rect.as_float[0]
	 *     rect.y  == rect.as_float[1]
	 *     rect.x2 == rect.as_float[2]
	 *     rect.y2 == rect.as_float[3]
	 */

	union {
		struct rtb_point pts[2];

		struct {
			GLfloat x, y;
			GLfloat x2, y2;
		};

		GLfloat as_float[4];
	};

	RTB_INHERIT_AS(rtb_size, size);
};

static inline void
rtb_rect_update_size_from_points(struct rtb_rect *rect)
{
	rect->w = rect->x2 - rect->x;
	rect->h = rect->y2 - rect->y;
}

static inline void
rtb_rect_update_points_from_size(struct rtb_rect *rect)
{
	rect->x2 = rect->x + rect->w;
	rect->y2 = rect->y + rect->h;
}

struct rtb_window;

struct rtb_phy_size rtb_size_to_phy(struct rtb_window *,
		struct rtb_size sz);
struct rtb_size rtb_phy_to_size(struct rtb_window *,
		struct rtb_phy_size phy);
struct rtb_phy_point rtb_point_to_phy(struct rtb_window *,
		struct rtb_point pt);
struct rtb_point rtb_phy_to_point(struct rtb_window *,
		struct rtb_phy_point phy);
