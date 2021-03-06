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

#include <windows.h>

#include <rutabaga/rutabaga.h>
#include <rutabaga/platform.h>

#include "win_rtb.h"

int64_t
rtb_mouse_double_click_interval(struct rtb_window *win)
{
	return GetDoubleClickTime() * 1000000;
}

void
rtb_mouse_pointer_warp(struct rtb_window *win, struct rtb_point pt)
{
	struct win_rtb_window *self = RTB_WINDOW_AS(win, win_rtb_window);
	struct rtb_mouse *m = &win->mouse;
	struct rtb_phy_point phy;
	POINT wpt;

	/* we'll get recursively called from the rtb__platform_mouse_motion() at
	 * the bottom of this function, so this is the termination condition to
	 * prevent infinite recursion. */
	if (pt.x == m->x && pt.y == m->y)
		return;

	phy = rtb_point_to_phy(win, pt);

	wpt.x = phy.x;
	wpt.y = phy.y;

	ClientToScreen(self->hwnd, &wpt);
	SetCursorPos(wpt.x, wpt.y);

	m->previous.x += pt.x - m->x;
	m->previous.y += pt.y - m->y;

	rtb__platform_mouse_motion(win, pt);
}

void
rtb__platform_set_cursor(struct rtb_window *win, struct rtb_mouse *mouse,
		rtb_mouse_cursor_t cursor)
{
	struct win_rtb *wrtb = (void *) win->rtb;

	switch (cursor) {
	case RTB_MOUSE_CURSOR_DEFAULT:
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		break;

	case RTB_MOUSE_CURSOR_COPY:
		SetCursor(wrtb->copy_cursor);
		break;

	case RTB_MOUSE_CURSOR_HIDDEN:
		SetCursor(NULL);
		break;
	}
}
