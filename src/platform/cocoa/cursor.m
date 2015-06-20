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

#include <rutabaga/rutabaga.h>
#include <rutabaga/platform.h>

#import <Cocoa/Cocoa.h>

#include "cocoa_rtb.h"

int64_t
rtb_mouse_double_click_interval(struct rtb_window *win)
{
	return [NSEvent doubleClickInterval] * 1e+09;
}

void
rtb_mouse_pointer_warp(struct rtb_window *win, int x, int y)
{
	struct cocoa_rtb_window *self = RTB_WINDOW_AS(win, cocoa_rtb_window);
	struct rtb_mouse *m = &win->mouse;
	NSRect rect, screen_frame;
	NSPoint point;

	/* we'll get recursively called from the rtb__platform_mouse_motion() at
	 * the bottom of this function, so this is the termination condition to
	 * prevent infinite recursion. */
	if (x == m->x && y == m->y)
		return;

	/* ok so...rutabaga coordinate space is top-left origin, which
	 * convertPoint handles properly here. window space is bottom-
	 * left origin, as is screen space. */
	point = [self->view convertPoint:NSMakePoint(x, y) toView:nil];
	rect = [self->view.window
		convertRectToScreen:NSMakeRect(point.x, point.y, 0.0, 0.0)];

	/* we've got a screen-space coord. let's convert to global space. */

	screen_frame = self->view.window.screen.frame;

	/* global space is top-left origin, so we need to flip y. */
	rect.origin.y = screen_frame.size.height - rect.origin.y;

	rect.origin.x += screen_frame.origin.x;
	rect.origin.y += screen_frame.origin.y;

	CGWarpMouseCursorPosition(rect.origin);
	CGAssociateMouseAndMouseCursorPosition(true);

	m->previous.x += x - m->x;
	m->previous.y += y - m->y;

	rtb__platform_mouse_motion(win, x, y);
}

void
rtb_set_cursor(struct rtb_window *rwin, rtb_mouse_cursor_t cursor)
{
}
