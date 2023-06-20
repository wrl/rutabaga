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

#pragma once

#include <rutabaga/rutabaga.h>
#include <rutabaga/types.h>

#define RTB_EVENT_SYS_MASK (1 << ((sizeof(rtb_ev_type_t) * 8) - 1))
#define RTB_IS_SYS_EVENT(x) (!!(x & RTB_EVENT_SYS_MASK))

#define RTB_EVENT(x) RTB_UPCAST(x, rtb_event)
#define RTB_EVENT_AS(x, type) \
	RTB_CONTAINER_OF(x, const struct type, RTB_INHERITED_MEMBER(rtb_event))

/**
 * base event types
 */

typedef unsigned int rtb_ev_type_t;

#define SYS(x) ((x) | RTB_EVENT_SYS_MASK)
enum {
	// dispatched when the window should close.
	// if the handler returns `1`, the window stays open.
	RTB_WINDOW_SHOULD_CLOSE = SYS(0),

	// same as above, except there is no opportunity for the application to
	// indicate that the window should stay open.
	RTB_WINDOW_WILL_CLOSE = SYS(1),

	// dispatched from rtb_window_close() as the window is tearing down.
	RTB_WINDOW_CLOSING = SYS(2),

	/**
	 * dispatched before any drawing happens in a frame.
	 */
	RTB_FRAME_START = SYS(3),

	/**
	 * dispatched after all drawing in a frame has occurred,
	 * before the GL buffer swap.
	 */
	RTB_FRAME_END   = SYS(4),

	RTB_FOCUS       = SYS(5),
	RTB_UNFOCUS     = SYS(6),

	RTB_KEY_PRESS   = SYS(7),
	RTB_KEY_RELEASE = SYS(8),

	RTB_MOUSE_ENTER = SYS(9),
	RTB_MOUSE_LEAVE = SYS(10),

	RTB_MOUSE_DOWN  = SYS(11),
	RTB_MOUSE_UP    = SYS(12),
	RTB_MOUSE_CLICK = SYS(13),
	RTB_MOUSE_WHEEL = SYS(14),

	RTB_DRAG_START  = SYS(15),
	RTB_DRAG_MOTION = SYS(16),
	RTB_DRAG_ENTER  = SYS(17),
	RTB_DRAG_LEAVE  = SYS(18),
	RTB_DRAG_DROP   = SYS(19)
};
#undef SYS

typedef enum {
	RTB_DIRECTION_LEAFWARD,
	RTB_DIRECTION_ROOTWARD
} rtb_ev_direction_t;

/**
 * RTB_EVENT_SOURCE_USER_DIRECT:
 *     low level mouse events, keyboard events, etc. anything that has arrived
 *     directly from user interaction.
 *
 * RTB_EVENT_SOURCE_USER_DERIVED:
 *     higher-level events synthesised from direct events. e.g. a button click
 *     event or a value change that comes from a mouse move.
 *
 * RTB_EVENT_SOURCE_NON_USER:
 *     events which did not come from user interaction. for example, frame
 *     start/end events.
 */

typedef enum {
	RTB_EVENT_SOURCE_USER_DIRECT  = 0,
	RTB_EVENT_SOURCE_USER_DERIVED = 1,
	RTB_EVENT_SOURCE_NON_USER     = 2,

	RTB_EVENT_SOURCE_USER_MASK     = 0x01,
	RTB_EVENT_SOURCE_NON_USER_MASK = 0x2,
} rtb_ev_source_t;

struct rtb_event {
	rtb_ev_type_t type;
	rtb_ev_source_t source;
	const struct rtb_event *derived_from;
};

static inline int
rtb_event_is_from_user(const struct rtb_event *e)
{
	return e && (e->source & RTB_EVENT_SOURCE_USER_MASK);
}

static inline rtb_ev_source_t
rtb_event_derived_source(const struct rtb_event *e)
{
	if (e && (e->source == RTB_EVENT_SOURCE_USER_DIRECT
	            || e->source == RTB_EVENT_SOURCE_USER_DERIVED))
		return RTB_EVENT_SOURCE_USER_DERIVED;

	return RTB_EVENT_SOURCE_NON_USER;
}

/**
 * handling
 */

struct rtb_element;

typedef int (*rtb_event_cb_t)
	(struct rtb_element *elem, const struct rtb_event *event, void *ctx);

struct rtb_event_handler {
	rtb_ev_type_t type;

	struct {
		rtb_event_cb_t cb;
		void *ctx;
	} callback;
};

/**
 * public API
 */

// returns -1 if no handler for this event. otherwise, returns the return value
// of the handler.
int rtb_handle(struct rtb_element *target, const struct rtb_event *);

struct rtb_element *rtb_dispatch_raw(struct rtb_element *target,
		struct rtb_event *event);
struct rtb_element *rtb_dispatch_simple(struct rtb_element *target,
		rtb_ev_type_t type);

int rtb_register_handler(struct rtb_element *on_elem,
		rtb_ev_type_t for_type, rtb_event_cb_t handler, void *ctx);
void rtb_unregister_handler(struct rtb_element *on_elem,
		rtb_ev_type_t for_type, rtb_event_cb_t handler, void *ctx);

void rtb_event_loop_init(struct rutabaga *);
void rtb_event_loop_run(struct rutabaga *);
void rtb_event_loop_fini(struct rutabaga *);

void rtb_event_loop(struct rutabaga *);
void rtb_event_loop_stop(struct rutabaga *);
