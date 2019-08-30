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

#include <assert.h>

#include <time.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <rutabaga/rutabaga.h>
#include <rutabaga/window.h>
#include <rutabaga/layout.h>
#include <rutabaga/event.h>

#include <rutabaga/widgets/button.h>
#include <rutabaga/widgets/label.h>

struct rtb_label click_counter;
unsigned clicks = 0;

static void
update_label(void)
{
	char buf[32];

	if (clicks == 1) {
		rtb_label_set_text(&click_counter, "clicked 1 time");
		return;
	}

	snprintf(buf, sizeof(buf), "clicked %u times", clicks);
	rtb_label_set_text(&click_counter, buf);
}

static int
button_clicked(struct rtb_element *tgt,
		const struct rtb_event *_e, void *ctx)
{
	const struct rtb_button_event *e = RTB_EVENT_AS(_e, rtb_button_event);

	if (e->mod_keys) {
		if (clicks > 0)
			clicks--;
	} else
		clicks++;

	update_label();

	return 0;
}

int
main(int argc, char **argv)
{
	struct rutabaga *delicious;
	struct rtb_window *win;

	struct rtb_button click_me;

	delicious = rtb_new();
	assert(delicious);
	win = rtb_window_open_ez(delicious, {
		.title = "rtb basic demo",

		.width  = 600,
		.height = 700,
	});
	assert(win);

	rtb_elem_set_size_cb(RTB_ELEMENT(win), rtb_size_hfit_children);
	rtb_elem_set_layout(RTB_ELEMENT(win), rtb_layout_hpack_center);

	rtb_label_init(&click_counter);
	rtb_button_init(&click_me);

	click_counter.align = RTB_ALIGN_MIDDLE;
	click_me.align = RTB_ALIGN_MIDDLE;
	click_me.flags |= RTB_ELEM_CLICK_FOCUS;

	rtb_button_set_label(&click_me, "click me");
	update_label();

	rtb_register_handler(RTB_ELEMENT(&click_me),
			RTB_BUTTON_CLICK, button_clicked, NULL);

	rtb_elem_add_child(RTB_ELEMENT(win), RTB_ELEMENT(&click_counter),
			RTB_ADD_TAIL);
	rtb_elem_add_child(RTB_ELEMENT(win), RTB_ELEMENT(&click_me),
			RTB_ADD_TAIL);

	rtb_event_loop(delicious);

	rtb_window_lock(win);

	rtb_label_fini(&click_counter);
	rtb_window_close(delicious->win);
	rtb_free(delicious);
}
