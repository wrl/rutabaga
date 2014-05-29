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

#include <uv.h>
#include <windows.h>
#include <windowsx.h>

#include "rutabaga/rutabaga.h"
#include "rutabaga/window.h"
#include "rutabaga/event.h"
#include "rtb_private/window_impl.h"

#include "win_rtb.h"

/* since resizing the window stalls our event loop, we register a system
 * timer so we can keep redrawing content during the window resize. */

#define WIN_RTB_SIZING_FRAME_TIMER 4242

static void
draw_frame(struct win_rtb_window *self)
{
	rtb_window_draw(RTB_WINDOW(self));
	SwapBuffers(self->dc);
}

/**
 * various message types
 */

static void
handle_mouse_motion(struct win_rtb_window *self, int x, int y)
{
}

static void
handle_resize(struct win_rtb_window *self)
{
	RECT wrect;

	GetClientRect(self->hwnd, &wrect);

	self->w = wrect.right;
	self->h = wrect.bottom;

	self->need_reconfigure = 1;
}

static void
handle_close(struct win_rtb_window *self)
{
	struct rtb_window_event rev = {
		.type   = RTB_WINDOW_CLOSE,
		.window = RTB_WINDOW(self)
	};

	rtb_dispatch_raw(RTB_ELEMENT(self), RTB_EVENT(&rev));
}

static void
handle_timer(struct win_rtb_window *self, int timer_id)
{
	switch (timer_id) {
	case WIN_RTB_SIZING_FRAME_TIMER:
		draw_frame(self);
		break;

	default:
		break;
	}
}

/**
 * message handling
 */

LRESULT
win_rtb_handle_message(struct win_rtb_window *self,
		UINT message, WPARAM wparam, LPARAM lparam)
{
	switch (message) {
	/**
	 * lifecycle
	 */
	case WM_CREATE:
	case WM_SHOWWINDOW:
	case WM_SIZE:
		handle_resize(self);
		return 0;

	case WM_CLOSE:
		handle_close(self);
		return 0;

	/**
	 * mouse events
	 */

	case WM_MOUSEMOVE:
		handle_mouse_motion(self,
				GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
		return 0;

	/**
	 * misc win32 bullshit
	 */

	case WM_TIMER:
		handle_timer(self, wparam);
		return 0;

	case WM_ENTERSIZEMOVE:
		SetTimer(self->hwnd, WIN_RTB_SIZING_FRAME_TIMER, 16, 0);
		return 0;

	case WM_EXITSIZEMOVE:
		KillTimer(self->hwnd, WIN_RTB_SIZING_FRAME_TIMER);
		return 0;

	/**
	 * reply hazy, ask bill
	 */

	default:
		return DefWindowProc(self->hwnd, message, wparam, lparam);
	}
}

static void
drain_windows_message_queue(struct win_rtb_window *self)
{
	MSG msg;

	while (PeekMessage(&msg, self->hwnd, 0, 0, PM_REMOVE))
		win_rtb_handle_message(self, msg.message, msg.wParam, msg.lParam);

	if (self->need_reconfigure)
		rtb_window_reinit(RTB_WINDOW(self));
}

/**
 * frame drawing
 */

static void
frame_cb(uv_timer_t *handle, int status)
{
	struct win_rtb_window *self = handle->data;

	drain_windows_message_queue(self);
	draw_frame(self);
}


/**
 * public API
 */

void
rtb_event_loop_init(struct rutabaga *r)
{
	struct win_rtb_window *win = RTB_WINDOW_AS(r->win, win_rtb_window);
	r->event_loop = uv_loop_new();

	uv_timer_init(r->event_loop, &win->frame_timer);
	win->frame_timer.data = win;

	uv_timer_start(&win->frame_timer, frame_cb, 0, 16);
}

void
rtb_event_loop_run(struct rutabaga *r)
{
	uv_run(r->event_loop, UV_RUN_DEFAULT);
}

void
rtb_event_loop_stop(struct rutabaga *r)
{
	uv_stop(r->event_loop);
}

void
rtb_event_loop_fini(struct rutabaga *r)
{
	uv_loop_t *rtb_loop;

	rtb_loop = r->event_loop;
	r->event_loop = NULL;
	uv_loop_delete(rtb_loop);
}
