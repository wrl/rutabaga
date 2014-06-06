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

#include "rutabaga/rutabaga.h"
#include "rutabaga/window.h"
#include "rutabaga/event.h"
#include "rutabaga/mouse.h"
#include "rutabaga/platform.h"

#define RTB_DBLCICK_WINDOW_NSEC 4e+08

/**
 * event dispatching
 */

static struct rtb_element *
dispatch_drag_event(struct rtb_window *window, rtb_ev_type_t type,
		struct rtb_element *also_dispatch_to, int button, int x, int y)
{
	struct rtb_mouse_button *b = &window->mouse.button[button];
	struct rtb_drag_event ev = {
		.type = type,
		.window = window,

		.mod_keys = rtb_get_modkeys(window),

		.button = button,
		.target = b->target,

		.cursor = {
			.x = x,
			.y = y},
		.start = {
			.x = b->drag_start.x,
			.y = b->drag_start.y},
		.delta = {
			.x = x - b->drag_last.x,
			.y = y - b->drag_last.y}
	};

	if (also_dispatch_to && also_dispatch_to != b->target)
		rtb_dispatch_raw(also_dispatch_to, RTB_EVENT(&ev));

	if (!b->target)
		return NULL;

	return rtb_dispatch_raw(b->target, RTB_EVENT(&ev));
}

static void
dispatch_drag_enter(struct rtb_window *window,
		struct rtb_element *dispatch_to, int x, int y)
{
	struct rtb_mouse_button *b;
	int i;

	struct rtb_drag_event ev = {
		.type = RTB_DRAG_ENTER,
		.window = window,

		.mod_keys = rtb_get_modkeys(window),

		.button = 0,
		.cursor = {
			.x = x,
			.y = y},

		.target = NULL
	};

	for (i = 0; i < RTB_MOUSE_BUTTON_MAX + 1; i++) {
		b = &window->mouse.button[i];

		if (b->state != RTB_MOUSE_BUTTON_STATE_DRAG)
			continue;

		ev.button = i;
		ev.target = b->target;

		rtb_dispatch_raw(dispatch_to, RTB_EVENT(&ev));
	}
}

static void
dispatch_drag_leave(struct rtb_window *window,
		struct rtb_element *dispatch_to, int x, int y)
{
	struct rtb_mouse_button *b;
	int i;

	struct rtb_drag_event ev = {
		.type = RTB_DRAG_LEAVE,
		.window = window,

		.mod_keys = rtb_get_modkeys(window),

		.button = 0,
		.cursor = {
			.x = x,
			.y = y},

		.target = NULL
	};

	for (i = 0; i < RTB_MOUSE_BUTTON_MAX + 1; i++) {
		b = &window->mouse.button[i];

		if (b->state != RTB_MOUSE_BUTTON_STATE_DRAG)
			continue;

		ev.button = i;
		ev.target = b->target;

		rtb_dispatch_raw(dispatch_to, RTB_EVENT(&ev));
	}
}

static struct rtb_element *
dispatch_click_event(struct rtb_window *window,
		struct rtb_element *target, int button, int x, int y)
{
	struct rtb_mouse_event ev = {
		.type = RTB_MOUSE_CLICK,
		.window = window,
		.target = target,

		.mod_keys = rtb_get_modkeys(window),

		.button = button,
		.click_count = window->mouse.button[button].click_count + 1,

		.cursor = {
			.x = x,
			.y = y},
	};

	return rtb_dispatch_raw(target, RTB_EVENT(&ev));
}

static struct rtb_element *
dispatch_simple_mouse_event(struct rtb_window *window,
		struct rtb_element *target, rtb_ev_type_t type,
		int button, int x, int y)
{
	struct rtb_mouse_event ev = {
		.type = type,
		.window = window,
		.target = target,

		.mod_keys = rtb_get_modkeys(window),

		.button = button,
		.cursor = {
			.x = x,
			.y = y}
	};

	return rtb_dispatch_raw(target, RTB_EVENT(&ev));
}

/**
 * state machine (one per mouse button)
 */

static void
mouse_down(struct rtb_window *window, struct rtb_element *target,
		int button, int x, int y)
{
	struct rtb_mouse *mouse = &window->mouse;
	struct rtb_mouse_button *b = &mouse->button[button];

	b->state  = RTB_MOUSE_BUTTON_STATE_DOWN;
	b->target = target;
	b->drag_last.x = x;
	b->drag_last.y = y;
	mouse->buttons_down |= 1 << button;
}

static void
mouse_up(struct rtb_window *window, struct rtb_element *target,
		int button, int x, int y)
{
	struct rtb_mouse *mouse = &window->mouse;
	struct rtb_mouse_button *b = &mouse->button[button];
	uint64_t now;

	if (rtb_elem_is_in_tree(b->target, target)) {
		now = uv_hrtime();

		if ((int64_t) (now - b->last_click) < RTB_DBLCICK_WINDOW_NSEC)
			b->click_count++;
		else
			b->click_count = 0;

		b->last_click = now;
		dispatch_click_event(window, target, button, x, y);
	} else if (b->state == RTB_MOUSE_BUTTON_STATE_DRAG)
		dispatch_drag_event(window, RTB_DRAG_DROP, target, button, x, y);

	b->state  = RTB_MOUSE_BUTTON_STATE_UP;
	b->target = NULL;
	mouse->buttons_down &= ~(1 << button);
}

static void
drag(struct rtb_window *win, int x, int y)
{
	struct rtb_mouse_button *b;
	int i;

	for (i = 0; i < RTB_MOUSE_BUTTON_MAX + 1; i++) {
		b = &win->mouse.button[i];

		switch (b->state) {
		case RTB_MOUSE_BUTTON_STATE_UP:
			continue;

		case RTB_MOUSE_BUTTON_STATE_DOWN:
			b->state = RTB_MOUSE_BUTTON_STATE_DRAG;
			b->drag_start.x = x;
			b->drag_start.y = y;

			b->target = dispatch_drag_event(win,
					RTB_DRAG_START, NULL, i, x, y);
			break;

		case RTB_MOUSE_BUTTON_STATE_DRAG:
			dispatch_drag_event(win, RTB_DRAGGING, NULL, i, x, y);
			break;
		}

		b->drag_last.x = x;
		b->drag_last.y = y;
	}
}

static struct rtb_element *
element_underneath_mouse(struct rtb_window *win)
{
	if (win->mouse.element_underneath)
		return win->mouse.element_underneath;
	return RTB_ELEMENT(win);
}

static void
retarget(struct rtb_window *win, int x, int y)
{
	struct rtb_element *iter, *ret = element_underneath_mouse(win);
	struct rtb_point cursor = {x, y};

	while (ret != (struct rtb_element *) win) {
		if (RTB_POINT_IN_RECT(cursor, *ret))
			break;

		ret->mouse_in = 0;
		dispatch_simple_mouse_event(win, ret, RTB_MOUSE_LEAVE, -1, x, y);

		if (win->mouse.buttons_down)
			dispatch_drag_leave(win, ret, x, y);

		ret = ret->parent;
	}

descend:
	TAILQ_FOREACH_REVERSE(iter, &ret->children, children, child) {
		if (RTB_POINT_IN_RECT(cursor, *iter)) {
			ret = iter;
			ret->mouse_in = 1;

			dispatch_simple_mouse_event(win, ret, RTB_MOUSE_ENTER, -1, x, y);

			if (win->mouse.buttons_down)
				dispatch_drag_enter(win, ret, x, y);

			goto descend;
		}
	}

	win->mouse.element_underneath = ret;
}

/**
 * platform API
 */

void
rtb_platform_mouse_press(struct rtb_window *win,
		int button, int x, int y)
{
	struct rtb_element *target;

	if (button > RTB_MOUSE_BUTTON_MAX)
		return;

	target = element_underneath_mouse(win);

	mouse_down(win, target, button, x, y);

	target = dispatch_simple_mouse_event(win,
			target, RTB_MOUSE_DOWN, button, x, y);

	if (button == RTB_MOUSE_BUTTON1
			&& (target->flags & RTB_ELEM_CLICK_FOCUS))
		rtb_window_focus_element(win, target);
}

void
rtb_platform_mouse_release(struct rtb_window *win,
		int button, int x, int y)
{
	struct rtb_element *target;

	if (button > RTB_MOUSE_BUTTON_MAX)
		return;

	target = element_underneath_mouse(win);

	mouse_up(win, target, button, x, y);
	dispatch_simple_mouse_event(win, target, RTB_MOUSE_UP, button, x, y);
}

void
rtb_platform_mouse_motion(struct rtb_window *win, int x, int y)
{
	if (!win->mouse_in) {
		if ((0 < x && x < win->w) && (0 < y && y < win->h)) {
			rtb_platform_mouse_enter_window(win, x, y);
			return;
		} else if (!win->mouse.buttons_down)
			return;
	}

	retarget(win, x, y);

	if (win->mouse.buttons_down)
		drag(win, x, y);

	win->mouse.x = x;
	win->mouse.y = y;
}

void
rtb_platform_mouse_enter_window(struct rtb_window *win, int x, int y)
{
	if (win->mouse_in)
		rtb_platform_mouse_leave_window(win, x, y);

	win->mouse_in = 1;
	win->mouse.element_underneath = RTB_ELEMENT(win);

	dispatch_simple_mouse_event(win, RTB_ELEMENT(win),
			RTB_MOUSE_ENTER, -1, x, y);

	/* XXX: only on x11-xcb? */
	rtb_platform_mouse_motion(win, x, y);
}

void
rtb_platform_mouse_leave_window(struct rtb_window *win, int x, int y)
{
	struct rtb_element *underneath = element_underneath_mouse(win);

	while (underneath) {
		underneath->mouse_in = 0;
		dispatch_simple_mouse_event(
				win, underneath, RTB_MOUSE_LEAVE, -1, x, y);

		if (win->mouse.buttons_down)
			dispatch_drag_leave(win, underneath, x, y);

		underneath = underneath->parent;
	}

	win->mouse.element_underneath = NULL;
	win->mouse_in = 0;
}
