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
#include <time.h>

#include "rutabaga/rutabaga.h"
#include "rutabaga/container.h"
#include "rutabaga/window.h"
#include "rutabaga/layout.h"
#include "rutabaga/keyboard.h"

#include "rutabaga/widgets/button.h"
#include "rutabaga/widgets/text-input.h"
#include "rutabaga/widgets/knob.h"

#define ARRAY_LENGTH(a) (sizeof(a) / sizeof(*a))

rtb_utf8_t *labels[] = {
	"hello",
	"we are",
	"buttons",
	"rendered",
	"in",
	"opengl"
};

rtb_utf8_t *rlabels[] = {
	"i'm",
	"gonna",
	"pop",
	"some",
	"taaaags",
	"only got",
	"20 dollars",
	"in my pocket",
	"i i i'm huntin",
	"looking for a come-up",
	"this is fucking awesome"
};

rtb_knob_t *knob;
rtb_button_t *last_button = NULL;
rtb_text_input_t *input;

int print_streeng(rtb_obj_t *victim, const rtb_ev_t *e, void *ctx)
{
	rtb_ev_mouse_t *mv = (void *) e;
	int i;

	printf("(%f, %f) click!\n", mv->cursor.x, mv->cursor.y);

	i = rand() % (ARRAY_LENGTH(rlabels));
	rtb_button_set_label((void *) victim, rlabels[i]);
	last_button = (void *) victim;

	rtb_text_input_set_text(input, rlabels[i], -1);

	return 0;
}

int knob_value(rtb_obj_t *victim, const rtb_ev_t *e, void *unused)
{
	if (victim != RTB_OBJECT(knob)) {
		knob = (void *) knob;
		printf("\n");
	}

	printf(" :: value: %f\n", ((rtb_ev_knob_t *) e)->value);
	return 0;
}

int report(rtb_obj_t *victim, const rtb_ev_t *e, void *user_data)
{
	puts("calc");
	return 0;
}

void distribute_demo(rtb_container_t *root)
{
	int i, j;
	rtb_container_t *containers[10];

	for (i = 0; i < ARRAY_LENGTH(containers); i++) {
		containers[i] = rtb_container_new();

		rtb_obj_set_size_cb(containers[i], rtb_size_hfill);
		rtb_obj_set_layout(containers[i], rtb_layout_hdistribute);

		/* XXX: lol memory leak */
		for (j = 0; j < i + 1; j++) {
			knob = rtb_knob_new();

			knob->origin = .7f;
			knob->min = -1.f;
			knob->max = 1.f;

			rtb_attach(RTB_OBJECT(knob), KNOB_VALUE_CHANGE, knob_value, NULL);
			rtb_container_add(containers[i], RTB_OBJECT(knob));
		}

		knob = NULL;

		rtb_container_add(root, containers[i]);
	}
}

void setup_ui(rtb_container_t *root)
{
	rtb_container_t *upper, *lower;
	rtb_button_t *buttons[6];
	int i;

	upper = rtb_container_new();
	lower = rtb_container_new();

	rtb_obj_set_layout(upper, rtb_layout_hpack_center);
	rtb_obj_set_size_cb(upper, rtb_size_hfill);

	rtb_obj_set_layout(lower, rtb_layout_hpack_right);
	rtb_obj_set_size_cb(lower, rtb_size_hfill);

	upper->outer_pad.x =
		upper->outer_pad.y =
		lower->outer_pad.x =
		lower->outer_pad.y = 5.f;

	for (i = 0; i < sizeof(buttons) / sizeof(*buttons); i++) {
		buttons[i] = rtb_button_new(NULL);
		rtb_button_set_label(buttons[i], labels[i]);

		rtb_attach(RTB_OBJECT(buttons[i]), BUTTON_CLICK, print_streeng, NULL);
	}

	rtb_container_add(upper, RTB_OBJECT(buttons[0]));
	rtb_container_add(upper, RTB_OBJECT(buttons[1]));
	rtb_container_add(upper, RTB_OBJECT(buttons[2]));

	rtb_container_add(lower, RTB_OBJECT(buttons[3]));
	rtb_container_add(lower, RTB_OBJECT(buttons[4]));
	rtb_container_add(lower, RTB_OBJECT(buttons[5]));

	rtb_container_add(root, upper);
	rtb_container_add(root, lower);
}

static int handle_input_key(struct rtb_object *obj,
		const struct rtb_event *_ev, void *ctx)
{
	rtb_text_input_t *input = RTB_OBJECT_AS(obj, rtb_text_input);
	rtb_ev_key_t *ev = (void *) _ev;

	switch (ev->keysym) {
	case RTB_KEY_ENTER:
		if (!last_button)
			break;

		rtb_button_set_label(last_button, rtb_text_input_get_text(input));
		break;

	default:
		break;
	}

	return 1;
}

void add_input(rtb_container_t *root)
{
	input = rtb_text_input_new();

	rtb_text_input_set_text(input, "hey ßßßß man §§§", -1);

	rtb_attach(RTB_OBJECT(input), RTB_KEY_PRESS, handle_input_key, NULL);
	rtb_obj_add_child(root, RTB_OBJECT(input), RTB_ADD_TAIL);
}

static void print_modkeys(const rtb_ev_key_t *ev)
{
	if (!ev->modkeys)
		return;

	printf("modkeys:");

#define PRINT_MOD(constant, name) \
	if (ev->modkeys & constant)   \
		printf(" " name)

	PRINT_MOD(RTB_KEY_MOD_SHIFT, "shift");
	PRINT_MOD(RTB_KEY_MOD_CTRL,  "ctrl");
	PRINT_MOD(RTB_KEY_MOD_ALT,   "alt");
	PRINT_MOD(RTB_KEY_MOD_SUPER, "super");

#undef PRINT_MOD

	printf("\n");
}

static int handle_key_press(struct rtb_object *victim,
		const struct rtb_event *e, void *ctx)
{
	struct rtb_event_key *ev = (void *) e;

	print_modkeys(ev);

	switch (ev->keysym) {
	case RTB_KEY_NUMPAD:
		printf("numpad: %lc\n", ev->character);

		if (ev->character == L'-')
			rtb_stop_event_loop((rtb_t *) ctx);

		break;

	case RTB_KEY_NORMAL:
		printf("normal: %lc\n", ev->character);
		break;

	case RTB_KEY_UP:    puts("^"); break;
	case RTB_KEY_LEFT:  puts("<"); break;
	case RTB_KEY_DOWN:  puts("v"); break;
	case RTB_KEY_RIGHT: puts(">"); break;

	default:
		break;
	}

	return 1;
}

int main(int argc, char **argv)
{
	rtb_t *delicious;
	rtb_win_t *win;

	srand(time(NULL));

	assert(delicious = rtb_init());
	assert((win = rtb_window_open(delicious, 450, 600, "~delicious~")));

	win->outer_pad.x = 5.f;
	win->outer_pad.y = 5.f;

	rtb_obj_set_size_cb(RTB_OBJECT(win), rtb_size_hfit_children);
	rtb_obj_set_layout(RTB_OBJECT(win), rtb_layout_vpack_bottom);

	rtb_attach(RTB_OBJECT(win), RTB_KEY_PRESS, handle_key_press, delicious);

	printf("%ld\n", sizeof(struct rtb_object));

	distribute_demo(RTB_OBJECT(delicious->win));
	setup_ui(RTB_OBJECT(delicious->win));
	add_input(RTB_OBJECT(delicious->win));

	rtb_event_loop(delicious);

	rtb_window_close(delicious->win);
	rtb_destroy(delicious);
}
