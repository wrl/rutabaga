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

#include <xcb/xcb.h>
#include <xcb/xcb_cursor.h>

#include <rutabaga/rutabaga.h>
#include <rutabaga/platform.h>
#include <rutabaga/window.h>

#include "xrtb.h"

#define FALLBACK_DOUBLE_CLICK_MS 300

int64_t
rtb_mouse_double_click_interval(struct rtb_window *win)
{
	/* XXX: should get this from the desktop environment or Xdefaults */
	return FALLBACK_DOUBLE_CLICK_MS * 1000000;
}

void
rtb_mouse_pointer_warp(struct rtb_window *rwin, struct rtb_point pt)
{
	struct xrtb_window *self = RTB_WINDOW_AS(rwin, xrtb_window);
	struct rtb_mouse *m = &rwin->mouse;
	struct rtb_phy_point phy;

	phy = rtb_point_to_phy(rwin, pt);

	if (pt.x == m->x && pt.y == m->y)
		return;

	xcb_warp_pointer(self->xrtb->xcb_conn, XCB_NONE, self->xcb_win,
			0, 0, 0, 0, phy.x, phy.y);

	/* since xorg sends us a motion notify, we have to fix the previous
	 * cursor position so that dragging still works as expected. */
	m->previous.x += pt.x - m->x;
	m->previous.y += pt.y - m->y;
}

void
rtb__platform_set_cursor(struct rtb_window *rwin, struct rtb_mouse *mouse,
		rtb_mouse_cursor_t cursor)
{
	struct xrtb_window *self = RTB_WINDOW_AS(rwin, xrtb_window);
	struct xcb_rutabaga *xrtb = self->xrtb;
	xcb_cursor_t xc;

#define USE_CURSOR(C, NAME) do {											\
	if (!self->cursor.C)													\
		self->cursor.C = xcb_cursor_load_cursor(self->cursor_ctx, NAME);	\
	xc = self->cursor.C;													\
} while(0)

	switch (cursor) {
	case RTB_MOUSE_CURSOR_DEFAULT:
		xc = self->cursor.dfault;
		break;

	case RTB_MOUSE_CURSOR_HIDDEN:
		xc = xrtb->empty_cursor;
		break;

	case RTB_MOUSE_CURSOR_COPY:
		USE_CURSOR(copy, "copy");
		break;

	default:
		return;
	}

	xcb_change_window_attributes(xrtb->xcb_conn, self->xcb_win,
			XCB_CW_CURSOR, &xc);

	xcb_flush(xrtb->xcb_conn);
}
