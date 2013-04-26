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

#include <wchar.h>

#include "rutabaga/rutabaga.h"
#include "rutabaga/types.h"

#define RTB_EVENT_SYS_MASK (1 << ((sizeof(rtb_ev_type_t) * 8) - 1))
#define RTB_IS_SYS_EVENT(x) (!!(x & RTB_EVENT_SYS_MASK))

#define RTB_EVENT(x) (&(x)->_rtb_event)
#define RTB_EVENT_WINDOW(x) (&(x)->_rtb_event_window)

/**
 * system event types
 */

typedef unsigned int rtb_ev_type_t;

#define SYS(x) ((x) | RTB_EVENT_SYS_MASK)
enum {
	RTB_WINDOW_CLOSE   = SYS(0),

	RTB_KEY_PRESS      = SYS(1),
	RTB_KEY_RELEASE    = SYS(2),

	RTB_MOUSE_DOWN     = SYS(3),
	RTB_MOUSE_UP       = SYS(4),
	RTB_MOUSE_ENTER    = SYS(5),
	RTB_MOUSE_LEAVE    = SYS(6),

	RTB_MOUSE_CLICK    = SYS(7),
	RTB_MOUSE_DBLCLICK = SYS(8),

	RTB_DRAG_START     = SYS(9),
	RTB_DRAGGING       = SYS(10),
	RTB_DRAG_ENTER     = SYS(11),
	RTB_DRAG_LEAVE     = SYS(12),
	RTB_DRAG_DROP      = SYS(13)
};
#undef SYS

/**
 * structures
 */

struct rtb_event {
	rtb_ev_type_t type;
};

struct rtb_event_window {
	RTB_INHERIT(rtb_event);

	rtb_win_t *window;
};

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

int rtb_handle(rtb_obj_t *victim, const rtb_ev_t *event);
rtb_obj_t *rtb_dispatch_raw(rtb_obj_t *victim, rtb_ev_t *event);
rtb_obj_t *rtb_dispatch_simple(rtb_obj_t *victim, rtb_ev_type_t type);

int rtb_attach(rtb_obj_t *victim, rtb_ev_type_t type, rtb_event_cb_t handler,
		void *user_arg);
void rtb_detach(rtb_obj_t *victim, rtb_ev_type_t type);

void rtb_event_loop(rtb_t *rutabaga);
