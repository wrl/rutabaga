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

#include <rutabaga/rutabaga.h>
#include <rutabaga/window.h>
#include <rutabaga/keyboard.h>

#include <uv.h>

#include <xcb/xcb.h>
#include <xcb/xcb_cursor.h>
#include <xkbcommon/xkbcommon.h>

#include <GL/glx.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#define ERR(...) fprintf(stderr, "rutabaga XCB: " __VA_ARGS__)

struct xrtb_frame_timer {
	RTB_INHERIT(uv_timer_s);
	struct xrtb_window *xwin;
	unsigned int wait_msec;
};

struct xrtb_uv_poll {
	RTB_INHERIT(uv_poll_s);
	struct xcb_rutabaga *xrtb;
};

struct xcb_rutabaga {
	struct rutabaga rtb;

	Display *dpy;
	xcb_connection_t *xcb_conn;

	struct {
		xcb_atom_t
			wm_protocols,
			wm_delete_window,
			targets,
			timestamp,
			utf8_string,
			clipboard;
	} atoms;

	struct {
		xkb_mod_index_t shift;
		xkb_mod_index_t ctrl;
		xkb_mod_index_t alt;
		xkb_mod_index_t super;
	} mod_indices;

	struct xkb_context *xkb_ctx;
	struct xkb_keymap *xkb_keymap;
	struct xkb_state *xkb_state;

	int xkb_supported;
	int xkb_core_kbd_id;
	int xkb_event;

	int running_in_xwayland;

	xcb_cursor_t empty_cursor;

	struct xrtb_uv_poll xcb_poll;
	struct xrtb_frame_timer frame_timer;

	struct {
		xcb_connection_t *conn;
		xcb_window_t window;
		rtb_utf8_t *buffer;
		size_t nbytes;
	} clipboard;
};

struct xrtb_window {
	RTB_INHERIT(rtb_window);

	struct xcb_rutabaga *xrtb;

	xcb_screen_t *screen;
	xcb_window_t xcb_win;

	xcb_cursor_context_t *cursor_ctx;

	struct {
		xcb_cursor_t dfault;
		xcb_cursor_t copy;
	} cursor;

	EGLDisplay egl_dpy;
	EGLContext egl_ctx;
	EGLSurface egl_surface;

	uint16_t numlock_mask;
	uint16_t capslock_mask;
	uint16_t shiftlock_mask;
	uint16_t modeswitch_mask;
};

rtb_keysym_t xrtb_keyboard_translate_keysym(xcb_keysym_t xsym,
		rtb_utf32_t *chr);

int  xrtb_keyboard_reload(struct xcb_rutabaga *xrtb);
int  xrtb_keyboard_init(struct xcb_rutabaga *xrtb);
void xrtb_keyboard_fini(struct xcb_rutabaga *xrtb);
