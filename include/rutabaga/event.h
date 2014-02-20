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

#pragma once

#include "rutabaga/rutabaga.h"
#include "rutabaga/types.h"

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
	/**
	 * dispatched when the window should close.
	 * if the handler returns `1`, the window stays open.
	 */
	RTB_WINDOW_CLOSE   = SYS(0),

	/**
	 * dispatched before any drawing happens in a frame.
	 */
	RTB_FRAME_START    = SYS(1),

	/**
	 * dispatched after all drawing in a frame has occurred,
	 * before the GL buffer swap.
	 */
	RTB_FRAME_END      = SYS(2),

	RTB_FOCUS          = SYS(3),
	RTB_UNFOCUS        = SYS(4),

	RTB_KEY_PRESS      = SYS(5),
	RTB_KEY_RELEASE    = SYS(6),

	RTB_MOUSE_DOWN     = SYS(7),
	RTB_MOUSE_UP       = SYS(8),
	RTB_MOUSE_ENTER    = SYS(9),
	RTB_MOUSE_LEAVE    = SYS(10),

	RTB_MOUSE_CLICK    = SYS(11),
	RTB_MOUSE_DBLCLICK = SYS(12),

	RTB_DRAG_START     = SYS(13),
	RTB_DRAGGING       = SYS(14),
	RTB_DRAG_ENTER     = SYS(15),
	RTB_DRAG_LEAVE     = SYS(16),
	RTB_DRAG_DROP      = SYS(17)
};
#undef SYS

typedef enum {
	RTB_DIRECTION_LEAFWARD,
	RTB_DIRECTION_ROOTWARD
} rtb_ev_direction_t;

/* take, for example, a mouse click event. if the click actually
 * originated from a mouse click, this would be RTB_EVENT_GENUINE.
 * if it originated from, say, a key press (i.e. when a button is
 * focused and the user presses <space> or <enter>), the event would
 * be marked RTB_EVENT_SYNTHETIC.  */

typedef enum {
	RTB_EVENT_GENUINE    = 0,
	RTB_EVENT_SYNTHETIC  = 1
} rtb_ev_source_t;

struct rtb_event {
	rtb_ev_type_t type;
	rtb_ev_source_t source;
};

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

int rtb_handle(struct rtb_element *target, const struct rtb_event *event);
struct rtb_element *rtb_dispatch_raw(struct rtb_element *target,
		struct rtb_event *event);
struct rtb_element *rtb_dispatch_simple(struct rtb_element *target,
		rtb_ev_type_t type);

int rtb_register_handler(struct rtb_element *on_elem,
		rtb_ev_type_t for_type, rtb_event_cb_t handler, void *context);
void rtb_unregister_handler(struct rtb_element *on_elem,
		rtb_ev_type_t for_type);

void rtb_event_loop_init(struct rutabaga *);
void rtb_event_loop_step(struct rutabaga *);
void rtb_event_loop_fini(struct rutabaga *);

void rtb_event_loop(struct rutabaga *);
