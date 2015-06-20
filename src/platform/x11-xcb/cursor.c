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

#include <xcb/xcb.h>

#include <rutabaga/rutabaga.h>
#include <rutabaga/window.h>

#include "xrtb.h"

#define FALLBACK_DOUBLE_CLICK_MS 300

int64_t
rtb__mouse_double_click_interval(struct rtb_window *win)
{
	/* XXX: should get this from the desktop environment or Xdefaults */
	return FALLBACK_DOUBLE_CLICK_MS * 1000000;
}

void
rtb__mouse_pointer_warp(struct rtb_window *rwin, int x, int y)
{
	struct xrtb_window *self = RTB_WINDOW_AS(rwin, xrtb_window);
	struct rtb_mouse *m = &rwin->mouse;

	if (self->xrtb->running_in_xwayland || (x == m->x && y == m->y))
		return;

	xcb_warp_pointer(self->xrtb->xcb_conn, XCB_NONE, self->xcb_win,
			0, 0, 0, 0, x, y);

	/* since xorg sends us a motion notify, we have to fix the previous
	 * cursor position so that dragging still works as expected. */
	m->previous.x += x - m->x;
	m->previous.y += y - m->y;
}

void
rtb_set_cursor(struct rtb_window *rwin, rtb_mouse_cursor_t cursor)
{
	struct xrtb_window *self = RTB_WINDOW_AS(rwin, xrtb_window);
	struct xcb_rutabaga *xrtb = self->xrtb;
	uint32_t val_mask, val_list;

	switch (cursor) {
	case RTB_MOUSE_CURSOR_DEFAULT:
		val_list = 0;
		break;

	case RTB_MOUSE_CURSOR_HIDDEN:
		val_list = xrtb->empty_cursor;
		break;

	default:
		return;
	}

	val_mask = XCB_CW_CURSOR;
	xcb_change_window_attributes(xrtb->xcb_conn, self->xcb_win,
			val_mask, &val_list);

	xcb_flush(xrtb->xcb_conn);
}
