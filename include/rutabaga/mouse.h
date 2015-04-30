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

#include "rutabaga/types.h"
#include "rutabaga/event.h"
#include "rutabaga/keyboard.h"

#define RTB_EVENT_MOUSE(x) RTB_UPCAST(x, rtb_event_mouse)
#define RTB_EVENT_DRAG(x) RTB_UPCAST(x, rtb_event_drag)

/**
 * types
 */

typedef enum {
	RTB_MOUSE_BUTTON1 = 0,
	RTB_MOUSE_BUTTON2 = 1,
	RTB_MOUSE_BUTTON3 = 2,

	RTB_MOUSE_BUTTON_MAX = RTB_MOUSE_BUTTON3
} rtb_mouse_buttons_t;

typedef enum {
	RTB_MOUSE_BUTTON1_MASK = 1 << RTB_MOUSE_BUTTON1,
	RTB_MOUSE_BUTTON2_MASK = 1 << RTB_MOUSE_BUTTON2,
	RTB_MOUSE_BUTTON3_MASK = 1 << RTB_MOUSE_BUTTON3
} rtb_mouse_button_mask_t;

typedef enum {
	RTB_MOUSE_CURSOR_DEFAULT,
	RTB_MOUSE_CURSOR_HIDDEN
} rtb_mouse_cursor_t;

/**
 * events
 */

struct rtb_mouse_event {
	RTB_INHERIT(rtb_event);

	struct rtb_window *window;
	struct rtb_element *target;
	rtb_modkey_t mod_keys;

	union {
		struct {
			rtb_mouse_buttons_t button;
			
			/* counts from 0, so 0 is the first click, 1 is a double
			 * click, etc. */
			int click_number;
		};

		struct {
			float delta;
		} wheel;
	};

	struct rtb_point cursor;
};

struct rtb_drag_event {
	RTB_INHERIT(rtb_mouse_event);

	struct rtb_point start;

	struct {
		int x;
		int y;
	} delta;
};

/**
 * internal mouse structure
 */

struct rtb_mouse {
	RTB_INHERIT(rtb_point);
	struct rtb_point previous;

	struct rtb_element *element_underneath;

	struct rtb_mouse_button {
		struct rtb_point drag_start;

		struct rtb_element *target;

		enum {
			RTB_MOUSE_BUTTON_STATE_UP,
			RTB_MOUSE_BUTTON_STATE_DOWN,
			RTB_MOUSE_BUTTON_STATE_DRAG
		} state;

		uint64_t last_click;
		int click_count;
	} button[RTB_MOUSE_BUTTON_MAX + 1];
	rtb_mouse_button_mask_t buttons_down;
};
