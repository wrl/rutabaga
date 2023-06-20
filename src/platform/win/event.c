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

#include <uv.h>
#include <windows.h>
#include <windowsx.h>
#include <winsock2.h>

#include <rutabaga/rutabaga.h>
#include <rutabaga/window.h>
#include <rutabaga/event.h>
#include <rutabaga/platform.h>

#include "rtb_private/window_impl.h"

#include "win_rtb.h"

/* since resizing the window stalls our event loop, we register a system
 * timer so we can keep redrawing content during the window resize. */

#define WIN_RTB_FRAME_TIMER 4242

#define LOCK(s) rtb_window_lock(RTB_WINDOW(s))
#define UNLOCK(s) rtb_window_unlock(RTB_WINDOW(s))

/**
 * frame drawing
 */

static void
draw_frame(struct win_rtb_window *self, int force)
{
	LOCK(self);

	if (rtb_window_draw(RTB_WINDOW(self), force))
		SwapBuffers(self->dc);

	UNLOCK(self);
}

/**
 * window events
 */

static void
handle_resize(struct win_rtb_window *self)
{
	RECT wrect;

	GetClientRect(self->hwnd, &wrect);

	self->phy_size.w = wrect.right;
	self->phy_size.h = wrect.bottom;

	LOCK(self);
	rtb_window_reinit(RTB_WINDOW(self));
	UNLOCK(self);
}

static void
handle_close(struct win_rtb_window *self)
{
	struct rtb_window_event rev = {
		.type   = RTB_WINDOW_SHOULD_CLOSE,
		.source = RTB_EVENT_SOURCE_USER_DIRECT,
		.window = RTB_WINDOW(self)
	};

	LOCK(self);
	rtb_dispatch_raw(RTB_ELEMENT(self), RTB_EVENT(&rev));
	UNLOCK(self);
}

static void
handle_timer(struct win_rtb_window *self, int timer_id)
{
	switch (timer_id) {
	case WIN_RTB_FRAME_TIMER:
		uv_run(&self->rtb->event_loop, UV_RUN_NOWAIT);
		draw_frame(self, 0);
		break;

	default:
		break;
	}
}

/**
 * mouse events
 */

static void
setup_mouse_tracking(struct win_rtb_window *self)
{
	TRACKMOUSEEVENT t = {
		sizeof(t),
		TME_LEAVE,
		self->hwnd,
		0
	};

	TrackMouseEvent(&t);
}

static void
handle_mouse_motion(struct win_rtb_window *self, int x, int y)
{
	struct rtb_window *rwin = RTB_WINDOW(self);

	if (!self->mouse_in)
		setup_mouse_tracking(self);

	LOCK(self);
	rtb__platform_mouse_motion(rwin, rtb_phy_to_point(rwin,
				RTB_MAKE_PHY_POINT(x, y)));
	UNLOCK(self);
}

static void
handle_mouse_leave(struct win_rtb_window *self)
{
	struct rtb_window *rwin = RTB_WINDOW(self);
	POINT p;

	GetCursorPos(&p);
	ScreenToClient(self->hwnd, &p);

	LOCK(self);
	rtb__platform_mouse_leave_window(rwin, rtb_phy_to_point(rwin,
				RTB_MAKE_PHY_POINT(p.x, p.y)));
	UNLOCK(self);
}

static void
handle_mouse_down(struct win_rtb_window *self, int button, LPARAM lparam)
{
	struct rtb_window *rwin = RTB_WINDOW(self);

	LOCK(self);
	rtb__platform_mouse_press(rwin, button, rtb_phy_to_point(rwin,
				RTB_MAKE_PHY_POINT(GET_X_LPARAM(lparam),
					GET_Y_LPARAM(lparam))));
	UNLOCK(self);

	if (!self->capture_depth++)
		SetCapture(self->hwnd);
}

static void
handle_mouse_up(struct win_rtb_window *self, int button, LPARAM lparam)
{
	struct rtb_window *rwin = RTB_WINDOW(self);

	LOCK(self);
	rtb__platform_mouse_release(rwin, button, rtb_phy_to_point(rwin,
				RTB_MAKE_PHY_POINT(GET_X_LPARAM(lparam),
					GET_Y_LPARAM(lparam))));
	UNLOCK(self);

	if (--self->capture_depth <= 0) {
		ReleaseCapture();
		self->capture_depth = 0;
	}
}

static void
handle_mouse_wheel(struct win_rtb_window *self, WPARAM wparam, LPARAM lparam)
{
	struct rtb_window *rwin = RTB_WINDOW(self);

	LOCK(self);
	rtb__platform_mouse_wheel(rwin, rtb_phy_to_point(rwin,
				RTB_MAKE_PHY_POINT(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam))),
			GET_WHEEL_DELTA_WPARAM(wparam) / 120.f);
	UNLOCK(self);
}

/**
 * message handling
 */

LRESULT
win_rtb_handle_message(struct win_rtb_window *self,
		UINT message, WPARAM wparam, LPARAM lparam)
{
	if (self->closing)
		return DefWindowProcW(self->hwnd, message, wparam, lparam);

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

	case WM_MOUSELEAVE:
		handle_mouse_leave(self);
		return 0;

	case WM_MOUSEWHEEL:
		handle_mouse_wheel(self, wparam, lparam);
		return 0;

	case WM_LBUTTONDOWN:
		handle_mouse_down(self, RTB_MOUSE_BUTTON1, lparam);
		return 0;

	case WM_LBUTTONUP:
		handle_mouse_up(self, RTB_MOUSE_BUTTON1, lparam);
		return 0;

	case WM_MBUTTONDOWN:
		handle_mouse_down(self, RTB_MOUSE_BUTTON2, lparam);
		return 0;

	case WM_MBUTTONUP:
		handle_mouse_up(self, RTB_MOUSE_BUTTON2, lparam);
		return 0;

	case WM_RBUTTONDOWN:
		handle_mouse_down(self, RTB_MOUSE_BUTTON3, lparam);
		return 0;

	case WM_RBUTTONUP:
		handle_mouse_up(self, RTB_MOUSE_BUTTON3, lparam);
		return 0;

	/**
	 * misc win32 bullshit
	 */

	case WM_PAINT:
		if (self->in_size_move)
			draw_frame(self, 1);
		else
			self->dirty = 1;

		ValidateRect(self->hwnd, NULL);
		return 0;

	case WM_TIMER:
		handle_timer(self, wparam);
		return 0;

	case WM_ENTERSIZEMOVE:
		self->in_size_move = 1;
		return 0;

	case WM_EXITSIZEMOVE:
		self->in_size_move = 0;
		return 0;

	/**
	 * reply hazy, ask bill
	 */

	default:
		return DefWindowProcW(self->hwnd, message, wparam, lparam);
	}
}

static void
run_application_event_loop(struct rutabaga *r)
{
	struct win_rtb_window *rwin = RTB_WINDOW_AS(r->win, win_rtb_window);

	int status;
	MSG msg;

	while (rwin->event_loop_running &&
			(status = GetMessage(&msg, rwin->hwnd, 0, 0))) {
		if (status == -1)
			break;

		TranslateMessage(&msg);
		win_rtb_handle_message(rwin, msg.message, msg.wParam, msg.lParam);
	}
}

/**
 * public API
 */

void
rtb_event_loop_init(struct rutabaga *r)
{
	uv_run(&r->event_loop, UV_RUN_NOWAIT);
}

void
rtb_event_loop_run(struct rutabaga *r)
{
	struct win_rtb_window *self = RTB_WINDOW_AS(r->win, win_rtb_window);

	self->capture_depth = 0;

	SetTimer(self->hwnd, WIN_RTB_FRAME_TIMER, 13, 0);
	self->event_loop_running = 1;

	if (r->owns_application_event_loop)
		run_application_event_loop(r);
}

void
rtb_event_loop_stop(struct rutabaga *r)
{
	struct win_rtb_window *self = RTB_WINDOW_AS(r->win, win_rtb_window);

	self->event_loop_running = 0;
	KillTimer(self->hwnd, WIN_RTB_FRAME_TIMER);

	if (self->capture_depth > 0) {
		ReleaseCapture();
		self->capture_depth = 0;
	}
}

void
rtb_event_loop_fini(struct rutabaga *r)
{
	/* once to run the event handlers for the last time, and once more
	 * to run the endgames for all of them. */
	uv_run(&r->event_loop, UV_RUN_NOWAIT);
	uv_run(&r->event_loop, UV_RUN_NOWAIT);
}

rtb_modkey_t
rtb_get_modkeys(struct rtb_window *rwin)
{
#define MOD_ACTIVE(win_key, rtb_mod) \
	(!!(GetAsyncKeyState(win_key) >> 15) * rtb_mod)

	return
		MOD_ACTIVE(VK_MENU,      RTB_KEY_MOD_ALT)
		| MOD_ACTIVE(VK_SHIFT,   RTB_KEY_MOD_SHIFT)
		| MOD_ACTIVE(VK_CONTROL, RTB_KEY_MOD_CTRL)
		| MOD_ACTIVE(VK_LWIN,    RTB_KEY_MOD_SUPER)
		| MOD_ACTIVE(VK_RWIN,    RTB_KEY_MOD_SUPER);

#undef MOD_ACTIVE
}
