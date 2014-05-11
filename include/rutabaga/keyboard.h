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

#include <stdint.h>
#include "rutabaga/types.h"

/**
 * types
 */

typedef enum {
	RTB_KEY_NORMAL,
	RTB_KEY_NUMPAD,

	RTB_KEY_NUM_LOCK,
	RTB_KEY_SCROLL_LOCK,
	RTB_KEY_CAPS_LOCK,

	RTB_KEY_LEFT_SHIFT,
	RTB_KEY_LEFT_CTRL,
	RTB_KEY_LEFT_SUPER,
	RTB_KEY_LEFT_ALT,

	RTB_KEY_RIGHT_SHIFT,
	RTB_KEY_RIGHT_CTRL,
	RTB_KEY_RIGHT_SUPER,
	RTB_KEY_RIGHT_ALT,
	RTB_KEY_MENU,

	RTB_KEY_TAB,
	RTB_KEY_ESCAPE,
	RTB_KEY_BACKSPACE,
	RTB_KEY_ENTER,

	RTB_KEY_F1,
	RTB_KEY_F2,
	RTB_KEY_F3,
	RTB_KEY_F4,
	RTB_KEY_F5,
	RTB_KEY_F6,
	RTB_KEY_F7,
	RTB_KEY_F8,
	RTB_KEY_F9,
	RTB_KEY_F10,
	RTB_KEY_F11,
	RTB_KEY_F12,

	RTB_KEY_PRINT_SCREEN,
	RTB_KEY_PAUSE,

	RTB_KEY_INSERT,
	RTB_KEY_DELETE,
	RTB_KEY_HOME,
	RTB_KEY_END,
	RTB_KEY_PAGE_UP,
	RTB_KEY_PAGE_DOWN,

	RTB_KEY_UP,
	RTB_KEY_LEFT,
	RTB_KEY_DOWN,
	RTB_KEY_RIGHT,

	RTB_KEY_NUMPAD_HOME,
	RTB_KEY_NUMPAD_END,
	RTB_KEY_NUMPAD_PAGE_UP,
	RTB_KEY_NUMPAD_PAGE_DOWN,
	RTB_KEY_NUMPAD_UP,
	RTB_KEY_NUMPAD_LEFT,
	RTB_KEY_NUMPAD_DOWN,
	RTB_KEY_NUMPAD_RIGHT,
	RTB_KEY_NUMPAD_MIDDLE,

	RTB_KEY_NUMPAD_INSERT,
	RTB_KEY_NUMPAD_DELETE,
	RTB_KEY_NUMPAD_ENTER,

	RTB_KEY_UNKNOWN
} rtb_keysym_t;

typedef enum {
	RTB_KEY_MOD_SHIFT = 0x01,
	RTB_KEY_MOD_CTRL  = 0x02,
	RTB_KEY_MOD_ALT   = 0x04,
	RTB_KEY_MOD_SUPER = 0x08
} rtb_modkey_t;

typedef enum {
	RTB_KEY_LOCK_CAPS  = 0x01,
	RTB_KEY_LOCK_NUM   = 0x02,
	RTB_KEY_LOCK_SHIFT = 0x04
} rtb_lock_mask_t;

/**
 * events
 */

struct rtb_key_event {
	RTB_INHERIT(rtb_event);

	rtb_modkey_t mod_keys;
	rtb_keysym_t keysym;

	rtb_utf32_t character;
};
