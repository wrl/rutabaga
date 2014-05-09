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

#define ARRAY_LENGTH(a) (sizeof(a) / sizeof(*a))

static rtb_utf8_t *labels[] = {
	"hello",
	"we are",
	"buttons",
	"rendered",
	"in",
	"opengl"
};

static rtb_utf8_t *rlabels[] = {
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

static struct rtb_button *last_button = NULL;
static struct rtb_text_input *input;
static struct rtb_label time_label;
static float speed = 1.f;

#ifdef __MACH__
# include <mach/mach.h>
# include <mach/mach_time.h>

static int
time_monotonic(struct timespec *ts)
{
	uint64_t t;

	t = mach_absolute_time();
	ts->tv_sec  = t / 1000000000;
	ts->tv_nsec = t % 1000000000;

	return 0;
}
#else
# include <time.h>

static int
time_monotonic(struct timespec *ts)
{
	return clock_gettime(CLOCK_MONOTONIC, ts);
}
#endif

static int
print_streeng(struct rtb_element *victim,
		const struct rtb_event *e, void *ctx)
{
	const struct rtb_mouse_event *mv = (void *) e;
	int i;

	printf("(%f, %f) click!\n", mv->cursor.x, mv->cursor.y);

	i = rand() % (ARRAY_LENGTH(rlabels));
	rtb_button_set_label((void *) victim, rlabels[i]);
	last_button = (void *) victim;

	rtb_text_input_set_text(input, rlabels[i], -1);

	return 0;
}

static int
knob_value(struct rtb_element *victim,
		const struct rtb_event *_e, void *unused)
{
	const struct rtb_knob_event *e = RTB_EVENT_AS(_e, rtb_knob_event);

	speed = e->value;
	printf(" :: speed: %f\n", e->value);
	return 0;
}

static void
distribute_demo(rtb_container_t *root)
{
	rtb_container_t *containers[10];
	struct rtb_knob *knob;
	int i, j;

	for (i = 0; i < (int) ARRAY_LENGTH(containers); i++) {
		containers[i] = rtb_container_new();

		rtb_elem_set_size_cb(containers[i], rtb_size_hfill);
		rtb_elem_set_layout(containers[i], rtb_layout_hdistribute);

		/* XXX: lol memory leak */
		for (j = 0; j < i + 1; j++) {
			knob = rtb_knob_new();

			if (!i) {
				knob->origin = 1.f;
				knob->min = 0.f;
				knob->max = 2.f;

				rtb_register_handler(RTB_ELEMENT(knob),
						KNOB_VALUE_CHANGE, knob_value, NULL);
			} else {
				knob->origin = 0.5f;
				knob->min = -1.f;
				knob->max = 1.f;
			}

			rtb_container_add(containers[i], RTB_ELEMENT(knob));
		}

		knob = NULL;

		rtb_container_add(root, containers[i]);
	}
}

static void
setup_ui(rtb_container_t *root)
{
	rtb_container_t *upper, *lower;
	struct rtb_button *buttons[6];
	int i;

	upper = rtb_container_new();
	lower = rtb_container_new();

	rtb_elem_set_layout(upper, rtb_layout_hpack_center);
	rtb_elem_set_size_cb(upper, rtb_size_hfill);

	rtb_elem_set_layout(lower, rtb_layout_hpack_right);
	rtb_elem_set_size_cb(lower, rtb_size_hfill);

	upper->outer_pad.x =
		upper->outer_pad.y =
		lower->outer_pad.x =
		lower->outer_pad.y = 5.f;

	for (i = 0; i < (int) ARRAY_LENGTH(buttons); i++) {
		buttons[i] = rtb_button_new(NULL);

		buttons[i]->flags |= RTB_ELEM_CLICK_FOCUS;
		rtb_button_set_label(buttons[i], labels[i]);

		rtb_register_handler(RTB_ELEMENT(buttons[i]),
				RTB_BUTTON_CLICK, print_streeng, NULL);
	}

	rtb_container_add(upper, RTB_ELEMENT(buttons[0]));
	rtb_container_add(upper, RTB_ELEMENT(buttons[1]));
	rtb_container_add(upper, RTB_ELEMENT(buttons[2]));

	rtb_container_add(lower, RTB_ELEMENT(buttons[3]));
	rtb_container_add(lower, RTB_ELEMENT(buttons[4]));
	rtb_container_add(lower, RTB_ELEMENT(buttons[5]));

	rtb_container_add(root, upper);
	rtb_container_add(root, lower);
}

static int
handle_input_key(struct rtb_element *obj,
		const struct rtb_event *_ev, void *ctx)
{
	struct rtb_text_input *input = RTB_ELEMENT_AS(obj, rtb_text_input);
	const struct rtb_key_event *ev = RTB_EVENT_AS(_ev, rtb_key_event);

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

void
add_input(struct rutabaga *rtb, rtb_container_t *root)
{
	input = rtb_text_input_new(rtb);

	rtb_text_input_set_text(input, "hey ßßßß man §§§", -1);

	input->min_size.w = 200.f;

	rtb_register_handler(RTB_ELEMENT(input),
			RTB_KEY_PRESS, handle_input_key, NULL);
	rtb_elem_add_child(root, RTB_ELEMENT(input), RTB_ADD_TAIL);
}

static void
print_modkeys(const struct rtb_key_event *ev)
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

static int
handle_key_press(struct rtb_element *victim,
		const struct rtb_event *e, void *ctx)
{
	const struct rtb_key_event *ev = RTB_EVENT_AS(e, rtb_key_event);

	print_modkeys(ev);

	switch (ev->keysym) {
	case RTB_KEY_NUMPAD:
		printf("numpad: %lc\n", ev->character);

		if (ev->character == L'-')
			rtb_event_loop_stop((struct rutabaga *) ctx);

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

static float timer, last;

static int
frame_start(struct rtb_element *elem, const struct rtb_event *e, void *ctx)
{
	struct timespec ts;
	float now;
	char buf[32];

	time_monotonic(&ts);

	now = (ts.tv_sec + (ts.tv_nsec / 1e+09));
	timer += (now - last) * speed;

	snprintf(buf, sizeof(buf), "%.2f", timer);
	last = now;

	rtb_label_set_text(&time_label, buf);
	return 1;
}

static int
frame_end(struct rtb_element *elem, const struct rtb_event *e, void *ctx)
{
	rtb_elem_mark_dirty(RTB_ELEMENT(&time_label));
	return 1;
}

static void
init_timer(void)
{
	struct timespec ts;
	time_monotonic(&ts);
	last = ts.tv_sec + (ts.tv_nsec / 1e+09);
	timer = 0.0;
}

int main(int argc, char **argv)
{
	struct rutabaga *delicious;
	struct rtb_window *win;

	srand(time(NULL));

	delicious = rtb_new();
	assert(delicious);
	win = rtb_window_open(delicious, 600, 700, "~delicious~");
	assert(win);

	win->outer_pad.x = 5.f;
	win->outer_pad.y = 5.f;

	rtb_elem_set_size_cb(RTB_ELEMENT(win), rtb_size_hfit_children);
	rtb_elem_set_layout(RTB_ELEMENT(win), rtb_layout_vpack_bottom);

	rtb_register_handler(RTB_ELEMENT(win),
			RTB_KEY_PRESS, handle_key_press, delicious);

	rtb_label_init(&time_label);
	rtb_elem_add_child(RTB_ELEMENT(win), RTB_ELEMENT(&time_label),
			RTB_ADD_HEAD);

	rtb_register_handler(RTB_ELEMENT(win),
			RTB_FRAME_START, frame_start, NULL);
	rtb_register_handler(RTB_ELEMENT(win),
			RTB_FRAME_END, frame_end, NULL);

	distribute_demo(RTB_ELEMENT(delicious->win));
	setup_ui(RTB_ELEMENT(delicious->win));
	add_input(delicious, RTB_ELEMENT(delicious->win));

	init_timer();
	rtb_event_loop(delicious);

	rtb_label_fini(&time_label);
	rtb_window_close(delicious->win);
	rtb_free(delicious);
}
