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

#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#include <xcb/xcb.h>
#include <xcb/xkb.h>
#include <xkbcommon/xkbcommon.h>

#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#include <X11/Xlibint.h>

#include <uv.h>

#include "rutabaga/rutabaga.h"
#include "rutabaga/window.h"
#include "rutabaga/event.h"
#include "rutabaga/platform.h"
#include "rutabaga/keyboard.h"

#include "private/util.h"

#include "xrtb.h"

#ifndef GL_ARB_timer_query
#define GL_TIME_ELAPSED 0x88BF
#define GL_TIMESTAMP 0x8E28
#endif

#define CAST_EVENT_TO(type) type *ev = (type *) _ev
#define SET_IF_TRUE(w, m, f) (w = (w & ~m) | (-f & m))

/**
 * mouse events
 */

static void
handle_mouse_enter(struct xrtb_window *win, const xcb_generic_event_t *_ev)
{
	CAST_EVENT_TO(xcb_enter_notify_event_t);
	rtb_platform_mouse_enter_window(RTB_WINDOW(win), ev->event_x, ev->event_y);
}

static void
handle_mouse_leave(struct xrtb_window *win, const xcb_generic_event_t *_ev)
{
	CAST_EVENT_TO(xcb_leave_notify_event_t);
	rtb_platform_mouse_leave_window(RTB_WINDOW(win), ev->event_x, ev->event_y);
}

static void
handle_mouse_button_press(struct xrtb_window *win,
		const xcb_generic_event_t *_ev)
{
	CAST_EVENT_TO(xcb_button_press_event_t);
	xcb_grab_pointer_cookie_t cookie;
	xcb_grab_pointer_reply_t *reply;

	int button;

	switch (ev->detail) {
	case 1: button = RTB_MOUSE_BUTTON1; break;
	case 2: button = RTB_MOUSE_BUTTON2; break;
	case 3: button = RTB_MOUSE_BUTTON3; break;

	default:
		goto dont_handle;
	}

	rtb_platform_mouse_press(RTB_WINDOW(win),
			button, ev->event_x, ev->event_y);

dont_handle:
	cookie = xcb_grab_pointer(
			win->xrtb->xcb_conn, 0, win->xcb_win,
			XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_MOTION |
			XCB_EVENT_MASK_BUTTON_RELEASE,
			XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE,
			XCB_NONE, XCB_CURRENT_TIME);

	if ((reply = xcb_grab_pointer_reply(win->xrtb->xcb_conn, cookie, NULL)))
		free(reply);
}

static void
handle_mouse_button_release(struct xrtb_window *xwin,
		const xcb_generic_event_t *_ev)
{
	CAST_EVENT_TO(xcb_button_release_event_t);
	xcb_void_cookie_t cookie;
	xcb_generic_error_t *err;
	int button;

	cookie = xcb_ungrab_pointer_checked(xwin->xrtb->xcb_conn,
			XCB_CURRENT_TIME);

	if ((err = xcb_request_check(xwin->xrtb->xcb_conn, cookie))) {
		ERR("can't ungrab pointer! (%d)\n", err->error_code);
		free(err);
	}

	switch (ev->detail) {
	case 1: button = RTB_MOUSE_BUTTON1; break;
	case 2: button = RTB_MOUSE_BUTTON2; break;
	case 3: button = RTB_MOUSE_BUTTON3; break;

	default:
		return;
	}

	rtb_platform_mouse_release(RTB_WINDOW(xwin),
			button, ev->event_x, ev->event_y);
}

static void
handle_mouse_motion(struct xrtb_window *win, const xcb_generic_event_t *_ev)
{
	CAST_EVENT_TO(xcb_motion_notify_event_t);

	rtb_platform_mouse_motion(RTB_WINDOW(win), ev->event_x, ev->event_y);
}

/**
 * keyboard events
 */

static rtb_modkey_t
modifier_state(struct xcb_rutabaga *xrtb)
{
#define MOD_ACTIVE(xkb_mod, rtb_mod) \
	((xkb_state_mod_index_is_active(xrtb->xkb_state, \
			xrtb->mod_indices.xkb_mod, XKB_STATE_MODS_EFFECTIVE) > 0) \
	 * rtb_mod)

	return
		MOD_ACTIVE(super,   RTB_KEY_MOD_SUPER)
		| MOD_ACTIVE(shift, RTB_KEY_MOD_SHIFT)
		| MOD_ACTIVE(ctrl,  RTB_KEY_MOD_CTRL)
		| MOD_ACTIVE(alt,   RTB_KEY_MOD_ALT);

#undef MOD_ACTIVE
}

static void
dispatch_key_event(struct xrtb_window *win,
		const xcb_key_press_event_t *ev, rtb_ev_type_t type)
{
	struct rtb_key_event rtb_ev = {.type = type};
	xcb_keysym_t sym;

	sym = xkb_state_key_get_one_sym(win->xrtb->xkb_state, ev->detail);

	/* first, look the keysym up in our internal mod key
	 * translation table. */
	rtb_ev.keysym = xrtb_keyboard_translate_keysym(sym, &rtb_ev.character);

	/* if we don't find it there, treat it like an alphanumeric key
	 * and get the UTF-32 value. */
	if (rtb_ev.keysym == RTB_KEY_UNKNOWN) {
		rtb_ev.keysym    = RTB_KEY_NORMAL;
		rtb_ev.character = xkb_keysym_to_utf32(sym);

		if (!rtb_ev.character)
			return;
	}

	rtb_ev.modkeys = modifier_state(win->xrtb);
	rtb_dispatch_raw(RTB_ELEMENT(win), RTB_EVENT(&rtb_ev));
}

static int
handle_key_press(struct xrtb_window *win, const xcb_generic_event_t *_ev)
{
	CAST_EVENT_TO(xcb_key_press_event_t);

	dispatch_key_event(win, ev, RTB_KEY_PRESS);
	xkb_state_update_key(win->xrtb->xkb_state, ev->detail, XKB_KEY_DOWN);
	return 0;
}

static int
handle_key_release(struct xrtb_window *win, const xcb_generic_event_t *_ev)
{
	CAST_EVENT_TO(xcb_key_release_event_t);

	dispatch_key_event(win, ev, RTB_KEY_RELEASE);
	xkb_state_update_key(win->xrtb->xkb_state, ev->detail, XKB_KEY_UP);
	return 0;
}

static void
handle_mapping_notify(struct xrtb_window *xwin, xcb_generic_event_t *_ev)
{
	xrtb_keyboard_reload(xwin->xrtb);
}

/**
 * window structure events
 */

static void
handle_visibility_notify(struct xrtb_window *win, xcb_generic_event_t *_ev)
{
	CAST_EVENT_TO(xcb_visibility_notify_event_t);

	switch (ev->state) {
	case XCB_VISIBILITY_UNOBSCURED:
		win->visibility = RTB_UNOBSCURED;
		break;

	case XCB_VISIBILITY_PARTIALLY_OBSCURED:
		win->visibility = RTB_PARTIALLY_OBSCURED;
		break;

	case XCB_VISIBILITY_FULLY_OBSCURED:
		win->visibility = RTB_FULLY_OBSCURED;
		break;
	}
}

static void
handle_configure_notify(struct xrtb_window *win, xcb_generic_event_t *_ev)
{
	CAST_EVENT_TO(xcb_configure_notify_event_t);

	if (ev->width  != win->w ||
		ev->height != win->h) {
		win->w = ev->width;
		win->h = ev->height;

		win->need_reconfigure = 1;
	}
}

static void
handle_client_message(struct xrtb_window *xwin, xcb_generic_event_t *_ev)
{
	CAST_EVENT_TO(xcb_client_message_event_t);
	struct rtb_window_event rev = {
		.type   = RTB_WINDOW_CLOSE,
		.window = RTB_WINDOW(xwin)
	};

	if (ev->data.data32[0] == xwin->xrtb->atoms.wm_delete_window)
		rtb_dispatch_raw(RTB_ELEMENT(xwin), RTB_EVENT(&rev));
}

/**
 * bullshit xkb fuck
 */

static int
handle_xkb_new_keyboard(struct xrtb_window *win, xcb_generic_event_t *_ev)
{
	CAST_EVENT_TO(xcb_xkb_new_keyboard_notify_event_t);

	if (ev->deviceID == win->xrtb->xkb_core_kbd_id)
		xrtb_keyboard_reload(win->xrtb);

	return 0;
}

static int
handle_xkb_state_notify(struct xrtb_window *win, xcb_generic_event_t *_ev)
{
	CAST_EVENT_TO(xcb_xkb_state_notify_event_t);

	xkb_state_update_mask(win->xrtb->xkb_state,
			ev->baseMods,
			ev->latchedMods,
			ev->lockedMods,
			ev->baseGroup,
			ev->latchedGroup,
			ev->lockedGroup);

	return 0;
}

static int
handle_xkb_event(struct xrtb_window *win, xcb_generic_event_t *_ev)
{
	int type = ((xcb_xkb_new_keyboard_notify_event_t *) _ev)->xkbType;

	switch (type) {
	case XCB_XKB_NEW_KEYBOARD_NOTIFY:
		handle_xkb_new_keyboard(win, _ev);
		break;

	case XCB_XKB_STATE_NOTIFY:
		handle_xkb_state_notify(win, _ev);
		break;
	}

	return 0;
}

/**
 * ~mystery~ events
 */

static int
handle_secret_xlib_event(Display *dpy, xcb_generic_event_t *ev)
{
	unsigned int response_type = ev->response_type & ~0x80;
	int (*proc)(Display *, XEvent *, xEvent *);

	XLockDisplay(dpy);

	proc = XESetWireToEvent(dpy, response_type, 0);
	if (proc) {
		XESetWireToEvent(dpy, response_type, proc);
		XEvent xev;

		ev->sequence = LastKnownRequestProcessed(dpy);
		proc(dpy, &xev, (xEvent *) ev);
	}

	XUnlockDisplay(dpy);

	if (proc)
		return 1;
	return 0;
}

/**
 * actual event loop
 */

static int
handle_generic_event(struct xrtb_window *win, xcb_generic_event_t *ev)
{
	int type = ev->response_type & ~0x80;

	switch (type) {
	/**
	 * mouse events
	 */

	case XCB_ENTER_NOTIFY:
		handle_mouse_enter(win, ev);
		break;

	case XCB_LEAVE_NOTIFY:
		handle_mouse_leave(win, ev);
		break;

	case XCB_BUTTON_PRESS:
		handle_mouse_button_press(win, ev);
		break;

	case XCB_BUTTON_RELEASE:
		handle_mouse_button_release(win, ev);
		break;

	case XCB_MOTION_NOTIFY:
		handle_mouse_motion(win, ev);
		break;

	/**
	 * keyboard events
	 */

	case XCB_KEY_PRESS:
		handle_key_press(win, ev);
		break;

	case XCB_KEY_RELEASE:
		handle_key_release(win, ev);
		break;

	/**
	 * window structure events
	 */

	case XCB_KEYMAP_NOTIFY:
		/* what */
		break;

	case XCB_MAPPING_NOTIFY:
		handle_mapping_notify(win, ev);
		break;

	case XCB_MAP_NOTIFY:
		break;

	case XCB_EXPOSE:
		win->dirty = 1;
		break;

	case XCB_VISIBILITY_NOTIFY:
		handle_visibility_notify(win, ev);
		break;

	case XCB_CONFIGURE_NOTIFY:
		handle_configure_notify(win, ev);
		break;

	case XCB_CLIENT_MESSAGE:
		handle_client_message(win, ev);
		break;

	/**
	 * ~mystery~ events
	 */

	default:
		if (type == win->xrtb->xkb_event)
			handle_xkb_event(win, ev);
		else
			handle_secret_xlib_event(win->xrtb->dpy, ev);
		break;
	}

	return 0;
}

static int
drain_xcb_event_queue(xcb_connection_t *conn, struct rtb_window *win)
{
	xcb_generic_event_t *ev;
	int ret;

	while ((ev = xcb_poll_for_event(conn))) {
		rtb_window_lock(win);
		ret = handle_generic_event(RTB_WINDOW_AS(win, xrtb_window), ev);
		rtb_window_unlock(win);

		free(ev);

		if (ret)
			return -1;
	}

	if (win->need_reconfigure) {
		rtb_window_lock(win);

		rtb_window_reinit(win);
		win->need_reconfigure = 0;

		rtb_window_unlock(win);
	}

	return 0;
}

/**
 * event loop
 */

struct video_sync {
	int functions_valid;

	PFNGLXSWAPBUFFERSMSCOMLPROC swap_buffers_msc;
	PFNGLXGETSYNCVALUESOMLPROC get_values;
	PFNGLXGETMSCRATEOMLPROC get_msc_rate;

	/**
	 * The Unadjusted System Time (or UST) is a 64-bit monotonically
	 * increasing counter that is available throughout the system. A UST
	 * timestamp is obtained each time the graphics MSC is incremented.
	 */
	int64_t ust;

	/**
	 * The graphics Media Stream Counter (or graphics MSC) is a counter
	 * that is unique to the graphics subsystem and increments for each
	 * vertical retrace that occurs.
	 */
	int64_t msc;

	/**
	 * The Swap Buffer Counter (SBC) is an attribute of a GLXDrawable
	 * and is incremented each time a swap buffer action is performed on
	 * the associated drawable.
	 */
	int64_t sbc;
};

static int
video_sync_init(struct video_sync *p)
{
	p->functions_valid = 0;

#define GET_PROC(dst, name) p->dst = (void *) glXGetProcAddress((GLubyte *) name);
	GET_PROC(swap_buffers_msc, "glXSwapBuffersMscOML");
	GET_PROC(get_values, "glXGetSyncValuesOML");
	GET_PROC(get_msc_rate, "glXGetMscRateOML");
#undef GET_PROC

	if (!p->swap_buffers_msc || !p->get_values || !p->get_msc_rate)
		return -1;

	p->functions_valid = 1;
	return 0;
}

struct xrtb_uv_poll {
	RTB_INHERIT(uv_poll_s);
	struct xcb_rutabaga *xrtb;
};

static void
xcb_poll_cb(uv_poll_t *_handle, int status, int events)
{
	struct xrtb_uv_poll *handle;
	struct xcb_rutabaga *xrtb;
	struct rtb_window *win;

	handle = RTB_DOWNCAST(_handle, xrtb_uv_poll, uv_poll_s);
	xrtb = handle->xrtb;
	win = ((struct rutabaga *) xrtb)->win;

	drain_xcb_event_queue(xrtb->xcb_conn, win);
}

struct xrtb_frame_timer {
	RTB_INHERIT(uv_timer_s);
	struct xrtb_window *xwin;
	unsigned int wait_msec;

	struct video_sync sync;
};

static void
frame_cb(uv_timer_t *_handle, int status)
{
	struct xrtb_frame_timer *timer;
	struct xrtb_window *xwin;
	struct rtb_window *win;
	struct video_sync *sync;

	timer = RTB_DOWNCAST(_handle, xrtb_frame_timer, uv_timer_s);
	xwin = timer->xwin;
	win = RTB_WINDOW(xwin);
	sync = &timer->sync;

	drain_xcb_event_queue(xwin->xrtb->xcb_conn, win);

	if (win->visibility == RTB_FULLY_OBSCURED || !win->dirty)
		return;

	rtb_window_lock(win);
	rtb_window_draw(win);

	if (sync->functions_valid) {
		sync->msc++;
		sync->swap_buffers_msc(xwin->xrtb->dpy, xwin->gl_draw,
				sync->msc, 0, 0);
		sync->get_values(xwin->xrtb->dpy, xwin->gl_draw,
				&sync->ust, &sync->msc, &sync->sbc);
	} else
		glXSwapBuffers(xwin->xrtb->dpy, xwin->gl_draw);
	rtb_window_unlock(win);
}

static int
frame_timer_init(struct xrtb_frame_timer *timer)
{
	int32_t fps_num, fps_denom;
	struct xcb_rutabaga *xrtb;
	struct xrtb_window *xwin;
	struct video_sync *sync;

	xwin = timer->xwin;
	xrtb = xwin->xrtb;
	sync = &timer->sync;

	if (video_sync_init(&timer->sync)) {
		/* oh well. */
		timer->wait_msec = 16;
		return -1;
	}

	rtb_window_lock(RTB_WINDOW(xwin));
	sync->get_values(xrtb->dpy, xwin->gl_draw,
			&sync->ust, &sync->msc, &sync->sbc);

	sync->get_msc_rate(xrtb->dpy, xwin->gl_draw, &fps_num, &fps_denom);
	rtb_window_unlock(RTB_WINDOW(xwin));

	timer->wait_msec =
		((1000 * (int64_t) fps_denom) / (int64_t) fps_num) - 1;

	return 0;
}

void
rtb_event_loop(struct rutabaga *r)
{
	struct xcb_rutabaga *xrtb = (void *) r;
	struct xrtb_window *xwin = (void *) r->win;
	struct xrtb_uv_poll xcb_poll;
	struct xrtb_frame_timer timer;
	uv_loop_t *rtb_loop;

	rtb_loop = uv_loop_new();
	r->event_loop = rtb_loop;

	xcb_poll.xrtb = xrtb;

	uv_poll_init(rtb_loop, RTB_UPCAST(&xcb_poll, uv_poll_s),
			xcb_get_file_descriptor(xrtb->xcb_conn));
	uv_poll_start(RTB_UPCAST(&xcb_poll, uv_poll_s), UV_READABLE,
			xcb_poll_cb);

	timer.xwin = xwin;
	frame_timer_init(&timer);

	uv_timer_init(rtb_loop, RTB_UPCAST(&timer, uv_timer_s));
	uv_timer_start(RTB_UPCAST(&timer, uv_timer_s),
			frame_cb, 0, timer.wait_msec);

	uv_run(rtb_loop, UV_RUN_DEFAULT);

	r->event_loop = NULL;
	uv_loop_delete(rtb_loop);
}
