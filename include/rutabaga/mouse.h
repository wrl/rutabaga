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

#define RTB_EV_MOUSE_T(x) ((rtb_ev_mouse_t *) x)
#define RTB_EV_DRAG_T(x) ((rtb_ev_drag_t *) x)

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

/**
 * events
 */

struct rtb_event_mouse {
	rtb_ev_t;
	rtb_win_t *window;
	rtb_obj_t *target;

	rtb_pt_t cursor;
	rtb_mouse_buttons_t button;
};

struct rtb_event_drag {
	rtb_ev_mouse_t;
	rtb_pt_t start;

	struct {
		int x;
		int y;
	} delta;
};

/**
 * internal mouse structure
 */

struct rtb_mouse {
	rtb_pt_t;
	rtb_obj_t *object_underneath;

	struct rtb_mouse_button {
		rtb_pt_t drag_last;
		rtb_pt_t drag_start;

		rtb_obj_t *target;

		enum {
			UP,
			DOWN,
			DRAG
		} state;
	} button[RTB_MOUSE_BUTTON_MAX + 1];
	rtb_mouse_button_mask_t buttons_down;
};

/**
 * platform API
 */

void rtb_mouse_press(rtb_win_t *win, int buttons, int x, int y);
void rtb_mouse_release(rtb_win_t *win, int buttons, int x, int y);
void rtb_mouse_motion(rtb_win_t *win, int x, int y);
void rtb_mouse_enter_window(rtb_win_t *win, int x, int y);
void rtb_mouse_leave_window(rtb_win_t *win, int x, int y);
