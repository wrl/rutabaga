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
#include <stdio.h>
#include <wctype.h>
#include <time.h>
#include <poll.h>
#include <math.h>

#include <xcb/xcb.h>
#include <xcb/xkb.h>
#include <xkbcommon/xkbcommon.h>

#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#include <X11/Xlibint.h>

#include "rutabaga/rutabaga.h"
#include "rutabaga/window.h"
#include "rutabaga/event.h"
#include "rutabaga/platform.h"
#include "rutabaga/keyboard.h"

#include "private/util.h"

#include "xrtb.h"

#define FPS 60
#define CAST_EVENT_TO(type) type *ev = (type *) _ev
#define SET_IF_TRUE(w, m, f) (w = (w & ~m) | (-f & m))

/**
 * mouse events
 */

static void handle_mouse_enter(struct xcb_window *win,
		const xcb_generic_event_t *_ev)
{
	CAST_EVENT_TO(xcb_enter_notify_event_t);
	rtb_platform_mouse_enter_window(RTB_WINDOW(win), ev->event_x, ev->event_y);
}

static void handle_mouse_leave(struct xcb_window *win,
		const xcb_generic_event_t *_ev)
{
	CAST_EVENT_TO(xcb_leave_notify_event_t);
	rtb_platform_mouse_leave_window(RTB_WINDOW(win), ev->event_x, ev->event_y);
}

static void handle_mouse_button_press(struct xcb_window *win,
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

static void handle_mouse_button_release(struct xcb_window *win,
		const xcb_generic_event_t *_ev)
{
	CAST_EVENT_TO(xcb_button_release_event_t);
	struct xcb_window *xwin = (void *) win;
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

	rtb_platform_mouse_release(RTB_WINDOW(win),
			button, ev->event_x, ev->event_y);
}

static void handle_mouse_motion(struct xcb_window *win,
		const xcb_generic_event_t *_ev)
{
	CAST_EVENT_TO(xcb_motion_notify_event_t);

	rtb_platform_mouse_motion(RTB_WINDOW(win), ev->event_x, ev->event_y);
}

/**
 * keyboard events
 */

static rtb_modkey_t modifier_state(struct xcb_rutabaga *xrtb)
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

static void dispatch_key_event(struct xcb_window *win,
		const xcb_key_press_event_t *ev, rtb_ev_type_t type)
{
	struct rtb_event_key rtb_ev = {.type = type};
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
	rtb_dispatch_raw(RTB_OBJECT(win), RTB_EVENT(&rtb_ev));
}

static int handle_key_press(struct xcb_window *win,
		const xcb_generic_event_t *_ev)
{
	CAST_EVENT_TO(xcb_key_press_event_t);

	dispatch_key_event(win, ev, RTB_KEY_PRESS);
	xkb_state_update_key(win->xrtb->xkb_state, ev->detail, XKB_KEY_DOWN);
	return 0;
}

static int handle_key_release(struct xcb_window *win,
		const xcb_generic_event_t *_ev)
{
	CAST_EVENT_TO(xcb_key_release_event_t);

	dispatch_key_event(win, ev, RTB_KEY_RELEASE);
	xkb_state_update_key(win->xrtb->xkb_state, ev->detail, XKB_KEY_UP);
	return 0;
}

static void handle_mapping_notify(struct xcb_window *win,
		xcb_generic_event_t *_ev)
{
	struct xcb_window *xwin = (void *) win;

	xrtb_keyboard_reload(xwin->xrtb);
}

/**
 * window structure events
 */

static void handle_visibility_notify(struct xcb_window *win,
		xcb_generic_event_t *_ev)
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

static void handle_configure_notify(struct xcb_window *win,
		xcb_generic_event_t *_ev)
{
	CAST_EVENT_TO(xcb_configure_notify_event_t);

	if (ev->width  != win->w ||
		ev->height != win->h) {
		win->w = ev->width;
		win->h = ev->height;

		win->need_reconfigure = 1;
	}
}

static void handle_client_message(struct xcb_window *win,
		xcb_generic_event_t *_ev)
{
	CAST_EVENT_TO(xcb_client_message_event_t);
	struct xcb_window *xwin = (void *) win;
	struct rtb_event_window rev = {
		.type   = RTB_WINDOW_CLOSE,
		.window = RTB_WINDOW(win)
	};

	if (ev->data.data32[0] == xwin->xrtb->atoms.wm_delete_window)
		rtb_dispatch_raw(RTB_OBJECT(win), RTB_EVENT(&rev));
}

/**
 * bullshit xkb fuck
 */

static int handle_xkb_new_keyboard(struct xcb_window *win,
		xcb_generic_event_t *_ev)
{
	CAST_EVENT_TO(xcb_xkb_new_keyboard_notify_event_t);

	if (ev->deviceID == win->xrtb->xkb_core_kbd_id)
		xrtb_keyboard_reload(win->xrtb);

	return 0;
}

static int handle_xkb_state_notify(struct xcb_window *win,
		xcb_generic_event_t *_ev)
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

static int handle_xkb_event(struct xcb_window *win,
		xcb_generic_event_t *_ev)
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

static int handle_secret_xlib_event(Display *dpy, xcb_generic_event_t *ev)
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

int handle_generic_event(struct xcb_window *win,
		xcb_generic_event_t *ev)
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

static int drain_xcb_event_queue(rtb_win_t *win, xcb_connection_t *conn)
{
	xcb_generic_event_t *ev;
	int ret;

	while ((ev = xcb_poll_for_event(conn))) {
		ret = handle_generic_event((struct xcb_window *) win, ev);
		free(ev);

		if (ret)
			return -1;
	}

	if (win->need_reconfigure) {
		rtb_window_reinit(win);
		win->need_reconfigure = 0;
	}

	return 0;
}

/**
 * timespec utils
 */

static void timespec_copy_inc(struct timespec *dst, struct timespec *src,
		long by_nsec)
{
	dst->tv_sec  = src->tv_sec;
	dst->tv_nsec = src->tv_nsec + by_nsec;

	if (dst->tv_nsec > 999999999) {
		dst->tv_sec++;
		dst->tv_nsec -= 999999999;
	} else if (dst->tv_nsec < 0) {
		dst->tv_sec--;
		dst->tv_nsec += 999999999;
	}
}

static void timespec_diff(struct timespec *diff,
		struct timespec *a, struct timespec *b)
{
	diff->tv_sec  = a->tv_sec  - b->tv_sec;
	diff->tv_nsec = a->tv_nsec - b->tv_nsec;
}

/**
 * granularity specified in fraction of a second. i.e. pass `1`
 * for a granularity of 1 second, `1000` for a millisecond, etc.
 *
 * finest granularity is 1 nanosecond, coarsest is 1 second.
 */
static int timespec_cmp(struct timespec *a, struct timespec *b,
		long granularity)
{
	struct timespec diff;
	int64_t nsec_diff;

	timespec_diff(&diff, a, b);

	if (granularity < 1 || granularity > 1000000000L)
		granularity = 1;
	else
		granularity = 1000000000L / granularity;

	nsec_diff = (diff.tv_sec * 1000000000L) +
		((diff.tv_nsec / (granularity)) * granularity);

	if (nsec_diff > 0)
		return 1;
	else if (nsec_diff < 0)
		return -1;

	return 0;
}

#define FRAME_NSEC (999999999 / FPS)
#define FRAME_MSEC (999 / FPS)

void rtb_event_loop(rtb_t *r)
{
	struct xcb_rutabaga *xrtb = (void *) r;
	struct timespec next_frame, now, diff;
	struct pollfd fds[1];
	rtb_win_t *win = r->win;
	int timeout_ms;

	clock_gettime(CLOCK_MONOTONIC, &now);
	timespec_copy_inc(&next_frame, &now, -1);

	r->run_event_loop = 1;

	fds[0].fd = xcb_get_file_descriptor(xrtb->xcb_conn);
	fds[0].events = POLLIN;

	while (r->run_event_loop) {
		clock_gettime(CLOCK_MONOTONIC, &now);
		timespec_diff(&diff, &next_frame, &now);
		timeout_ms = (diff.tv_sec * 1000) + (diff.tv_nsec / 1000000);
		if (timeout_ms < 0 || timeout_ms >= FRAME_MSEC)
			timeout_ms = 0;

		poll(fds, ARRAY_LENGTH(fds), timeout_ms);

		rtb_window_lock(win);

		if (drain_xcb_event_queue(win, xrtb->xcb_conn) < 0)
			return;

		clock_gettime(CLOCK_MONOTONIC, &now);
		if ((timespec_cmp(&next_frame, &now, 1000L) < 1) &&
		    win->visibility != RTB_FULLY_OBSCURED) {
			rtb_window_draw(win);
			timespec_copy_inc(&next_frame, &now, FRAME_NSEC);
		}

		rtb_window_unlock(win);
	}
}
