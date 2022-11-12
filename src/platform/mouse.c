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

#include <rutabaga/rutabaga.h>
#include <rutabaga/window.h>
#include <rutabaga/event.h>
#include <rutabaga/mouse.h>
#include <rutabaga/platform.h>

/**
 * event dispatching
 */

static struct rtb_element *
dispatch_drag_event(struct rtb_window *window, rtb_ev_type_t type,
		struct rtb_element *also_dispatch_to, int button,
		struct rtb_point cursor, struct rtb_size delta)
{
	struct rtb_mouse_button *b = &window->mouse.button[button];
	struct rtb_drag_event ev = {
		.type = type,
		.window = window,

		.mod_keys = rtb_get_modkeys(window),

		.button = button,
		.target = b->target,

		.cursor = cursor,
		.start = {
			.x = b->drag_start.x,
			.y = b->drag_start.y},
		.start_mod_keys = b->drag_start_mod_keys,
		.delta = {
			.x = delta.w,
			.y = delta.h}
	};

	if (also_dispatch_to && also_dispatch_to != b->target)
		rtb_dispatch_raw(also_dispatch_to, RTB_EVENT(&ev));

	if (!b->target)
		return NULL;

	return rtb_dispatch_raw(b->target, RTB_EVENT(&ev));
}

static void
dispatch_drag_enter(struct rtb_window *window,
		struct rtb_element *dispatch_to, struct rtb_point cursor)
{
	struct rtb_mouse_button *b;
	int i;

	struct rtb_drag_event ev = {
		.type = RTB_DRAG_ENTER,
		.window = window,

		.mod_keys = rtb_get_modkeys(window),

		.button = 0,
		.cursor = cursor,

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
		struct rtb_element *dispatch_to, struct rtb_point cursor)
{
	struct rtb_mouse_button *b;
	int i;

	struct rtb_drag_event ev = {
		.type = RTB_DRAG_LEAVE,
		.window = window,

		.mod_keys = rtb_get_modkeys(window),

		.button = 0,
		.cursor = cursor,

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
		struct rtb_element *target, int button,
		rtb_mouse_button_state_t button_state, struct rtb_point cursor)
{
	struct rtb_mouse_event ev = {
		.type = RTB_MOUSE_CLICK,
		.window = window,
		.target = target,

		.mod_keys = rtb_get_modkeys(window),

		.button = button,
		.button_state = button_state,

		.click_number = window->mouse.button[button].click_count,

		.cursor = cursor
	};

	return rtb_dispatch_raw(target, RTB_EVENT(&ev));
}

static struct rtb_element *
dispatch_simple_mouse_event(struct rtb_window *window,
		struct rtb_element *target, rtb_ev_type_t type,
		int button, struct rtb_point cursor)
{
	struct rtb_mouse_event ev = {
		.type = type,
		.window = window,
		.target = target,

		.mod_keys = rtb_get_modkeys(window),

		.button = button,
		.cursor = cursor,
	};

	return rtb_dispatch_raw(target, RTB_EVENT(&ev));
}

/**
 * state machine (one per mouse button)
 */

static void
mouse_down(struct rtb_window *window, struct rtb_element *target,
		int button)
{
	struct rtb_mouse *mouse = &window->mouse;
	struct rtb_mouse_button *b = &mouse->button[button];

	b->state  = RTB_MOUSE_BUTTON_STATE_DOWN;
	b->target = target;
	mouse->buttons_down |= 1 << button;
	b->last_mouse_down = uv_hrtime();
}

static void
mouse_up(struct rtb_window *window, struct rtb_element *target,
		int button, struct rtb_point cursor)
{
	int64_t double_click_interval, time_between_clicks, time_in_click;
	struct rtb_mouse *mouse = &window->mouse;
	struct rtb_mouse_button *b = &mouse->button[button];
	struct rtb_size drag_delta;
	uint64_t now;

	rtb_mouse_button_state_t b_state = b->state;
	struct rtb_element *b_target = b->target;

	b->state  = RTB_MOUSE_BUTTON_STATE_UP;
	b->target = NULL;
	mouse->buttons_down &= ~(1 << button);

	/* are we releasing the mouse on the same element that we pressed the mouse
	 * down on? */
	if (rtb_elem_is_in_tree(b_target, target)) {
		double_click_interval = rtb_mouse_double_click_interval(window);
		now = uv_hrtime();

		time_in_click = now - b->last_mouse_down;
		time_between_clicks = now - b->last_click;

		/* only do double (+ triple, + more) click updating if this click
		 * wasn't a drag. */
		if (b_state != RTB_MOUSE_BUTTON_STATE_DRAG) {
			if (time_between_clicks < double_click_interval)
				b->click_count++;
			else
				b->click_count = 0;

			/* this is so that a click which takes a long time to complete (for
			 * example, one in which where was hesitation or dragging between the
			 * mousedown and mouseup) doesn't count toward number of clicks.
			 *
			 * mostly just an aesthetic concern. feels better this way. otherwise
			 * you can take a long time with a click and then immediately follow it
			 * up with a short click and it counts as a double click. feels weird
			 * and wrong. */
			if (time_in_click < double_click_interval)
				b->last_click = now;
		} else
			b->click_count = 0;

		dispatch_click_event(window, target, button, b_state, cursor);
	} else if (b_state == RTB_MOUSE_BUTTON_STATE_DRAG) {
		drag_delta.w = cursor.x - mouse->previous.x;
		drag_delta.h = cursor.y - mouse->previous.y;

		dispatch_drag_event(window, RTB_DRAG_DROP, target, button, cursor,
				drag_delta);
	}
}

static void
drag(struct rtb_window *win, struct rtb_point cursor, struct rtb_size delta)
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
			b->drag_start = cursor;

			b->drag_start_mod_keys = rtb_get_modkeys(win);

			b->target = dispatch_drag_event(win,
					RTB_DRAG_START, NULL, i, cursor, delta);
			break;

		case RTB_MOUSE_BUTTON_STATE_DRAG:
			dispatch_drag_event(win, RTB_DRAG_MOTION, NULL, i, cursor, delta);
			break;
		}
	}
}

static struct rtb_element *
element_underneath_mouse(struct rtb_window *win)
{
	if (win->mouse.element_underneath)
		return win->mouse.element_underneath;
	return RTB_ELEMENT(win);
}

static inline int
point_in_visible_element(const struct rtb_element *elem, struct rtb_point pt)
{
	return RTB_POINT_IN_RECT(pt, *elem) && elem->visibility != RTB_FULLY_OBSCURED;
}

static struct rtb_element *
retarget_descend(struct rtb_element *ret, struct rtb_window *win,
		struct rtb_point cursor)
{
	struct rtb_element *iter;
	int hit = 0;

descend:
	TAILQ_FOREACH_REVERSE(iter, &ret->children, rtb_elem_children, child) {
		if (point_in_visible_element(iter, cursor)) {
			hit = 1;

			ret = iter;
			ret->mouse_in = 1;

			dispatch_simple_mouse_event(win, ret, RTB_MOUSE_ENTER, -1, cursor);

			if (win->mouse.buttons_down)
				dispatch_drag_enter(win, ret, cursor);

			goto descend;
		}
	}

	if (hit)
		return ret;
	return NULL;
}

static inline void
mouse_leave_element(struct rtb_window *win, struct rtb_element *elem,
		struct rtb_point cursor)
{
	elem->mouse_in = 0;
	dispatch_simple_mouse_event(
			win, elem, RTB_MOUSE_LEAVE, -1, cursor);

	if (win->mouse.buttons_down)
		dispatch_drag_leave(win, elem, cursor);
}

static void
retarget(struct rtb_window *win, struct rtb_point cursor)
{
	struct rtb_element *found, *ret = element_underneath_mouse(win);

	while (ret != RTB_ELEMENT(win)) {
		if (point_in_visible_element(ret, cursor))
			break;

		mouse_leave_element(win, ret, cursor);
		ret = ret->parent;
	}

	if (ret == RTB_ELEMENT(&win->overlay_surface)
			|| (ret == RTB_ELEMENT(win) && win->mouse_in_overlay)) {
		win->mouse_in_overlay = 0;
		ret = RTB_ELEMENT(win);
	}

	found = NULL;

	if (!win->mouse_in_overlay) {
		found = retarget_descend(RTB_ELEMENT(&win->overlay_surface), win, cursor);

		if (found) {
			while (ret != (struct rtb_element *) win) {
				mouse_leave_element(win, ret, cursor);
				ret = ret->parent;
			}

			win->mouse_in_overlay = 1;
			ret = found;
		}
	}

	if (!found && (found = retarget_descend(ret, win, cursor)))
		ret = found;

	win->mouse.element_underneath = ret;
}

/**
 * platform API
 */

void
rtb__platform_mouse_press(struct rtb_window *win,
		int button, struct rtb_point pt)
{
	struct rtb_element *target;

	if (button > RTB_MOUSE_BUTTON_MAX)
		return;

	target = element_underneath_mouse(win);

	mouse_down(win, target, button);

	target = dispatch_simple_mouse_event(win,
			target, RTB_MOUSE_DOWN, button, pt);

	if (button == RTB_MOUSE_BUTTON1
			&& (target->flags & RTB_ELEM_CLICK_FOCUS))
		rtb_window_focus_element(win, target);
}

void
rtb__platform_mouse_release(struct rtb_window *win,
		int button, struct rtb_point pt)
{
	struct rtb_element *target;

	if (button > RTB_MOUSE_BUTTON_MAX)
		return;

	target = element_underneath_mouse(win);

	mouse_up(win, target, button, pt);
	dispatch_simple_mouse_event(win, target, RTB_MOUSE_UP, button, pt);
}

void
rtb__platform_mouse_motion(struct rtb_window *win, struct rtb_point pt)
{
	struct rtb_size delta;

	if (!win->mouse_in) {
		if ((0 < pt.x && pt.x < win->w) && (0 < pt.y && pt.y < win->h)) {
			rtb__platform_mouse_enter_window(win, pt);
			return;
		} else if (!win->mouse.buttons_down)
			return;
	}

	retarget(win, pt);

	*RTB_UPCAST(&win->mouse, rtb_point) = pt;

	if (win->mouse.buttons_down) {
		delta.w = pt.x - win->mouse.previous.x;
		delta.h = pt.y - win->mouse.previous.y;

		/* the positioning of this line is VERY important. see
		 * platform/x11-xcb/cursor.c function rtb_mouse_pointer_warp() */
		win->mouse.previous = *RTB_UPCAST(&win->mouse, rtb_point);

		drag(win, pt, delta);
	} else
		win->mouse.previous = *RTB_UPCAST(&win->mouse, rtb_point);
}

void
rtb__platform_mouse_wheel(struct rtb_window *win, struct rtb_point pt,
		float delta)
{
	struct rtb_element *target = element_underneath_mouse(win);

	struct rtb_mouse_event ev = {
		.type = RTB_MOUSE_WHEEL,
		.window = win,
		.target = target,

		.mod_keys = rtb_get_modkeys(win),

		.wheel.delta = delta,
		.cursor = pt,
	};

	rtb_dispatch_raw(target, RTB_EVENT(&ev));
}

void
rtb__platform_mouse_enter_window(struct rtb_window *win,
		struct rtb_point pt)
{
	if (win->mouse_in)
		rtb__platform_mouse_leave_window(win, pt);

	win->mouse_in = 1;
	win->mouse.element_underneath = RTB_ELEMENT(win);

	dispatch_simple_mouse_event(win, RTB_ELEMENT(win),
			RTB_MOUSE_ENTER, -1, pt);

	/* XXX: only on x11-xcb? */
	rtb__platform_mouse_motion(win, pt);
}

void
rtb__platform_mouse_leave_window(struct rtb_window *win,
		struct rtb_point pt)
{
	struct rtb_element *underneath = element_underneath_mouse(win);

	while (underneath) {
		mouse_leave_element(win, underneath, pt);
		underneath = underneath->parent;
	}

	win->mouse.element_underneath = NULL;
	win->mouse_in = 0;
}

/**
 * public API
 */

void
rtb_mouse_set_cursor(struct rtb_window *win, struct rtb_mouse *mouse,
		rtb_mouse_cursor_t cursor)
{
	mouse->current_cursor = cursor;
	rtb__platform_set_cursor(win, mouse, cursor);
}

void
rtb_mouse_unset_cursor(struct rtb_window *win, struct rtb_mouse *mouse)
{
	rtb_mouse_set_cursor(win, mouse, RTB_MOUSE_CURSOR_DEFAULT);
}
