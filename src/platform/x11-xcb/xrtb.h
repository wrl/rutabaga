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

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <GL/glx.h>

#include "rutabaga/rutabaga.h"
#include "rutabaga/window.h"

#define ERR(...) fprintf(stderr, "rutabaga XCB: " __VA_ARGS__)

struct xcb_rutabaga {
	rtb_t rtb;

	Display *dpy;
	xcb_connection_t *xcb_conn;

	struct {
		xcb_atom_t wm_protocols;
		xcb_atom_t wm_delete_window;
	} atoms;

	/* oh man, this typedef. it's in glxext.h */
	PFNGLXCOPYSUBBUFFERMESAPROC copy_sub_buffer;
};

struct xcb_window {
	RTB_INHERIT(rtb_window);

	struct xcb_rutabaga *xrtb;

	xcb_screen_t *screen;
	xcb_window_t xcb_win;

	GLXDrawable gl_draw;
	GLXContext gl_ctx;
	GLXWindow gl_win;

	xcb_key_symbols_t *keysyms;

	uint16_t numlock_mask;
	uint16_t capslock_mask;
	uint16_t shiftlock_mask;
	uint16_t modeswitch_mask;
};
