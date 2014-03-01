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

#include <assert.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "rutabaga/rutabaga.h"
#include "rutabaga/container.h"
#include "rutabaga/window.h"
#include "rutabaga/layout.h"
#include "rutabaga/keyboard.h"

#include "rutabaga/widgets/button.h"
#include "rutabaga/widgets/text-input.h"
#include "rutabaga/widgets/knob.h"

static int
frame_start(struct rtb_element *elem, const struct rtb_event *e, void *ctx)
{
	return 1;
}

static int
frame_end(struct rtb_element *elem, const struct rtb_event *e, void *ctx)
{
	return 1;
}

int main(int argc, char **argv)
{
	struct rutabaga *delicious;
	struct rtb_window *win;

	delicious = rtb_new();
	assert(delicious);
	win = rtb_window_open(delicious, 450, 600, "~delicious~");
	assert(win);

	win->outer_pad.x = 5.f;
	win->outer_pad.y = 5.f;

	rtb_elem_set_size_cb(RTB_ELEMENT(win), rtb_size_hfit_children);
	rtb_elem_set_layout(RTB_ELEMENT(win), rtb_layout_vpack_bottom);

	rtb_register_handler(RTB_ELEMENT(win),
			RTB_FRAME_START, frame_start, NULL);
	rtb_register_handler(RTB_ELEMENT(win),
			RTB_FRAME_END, frame_end, NULL);

	rtb_event_loop(delicious);

	rtb_window_close(delicious->win);
	rtb_free(delicious);
}
