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

#include <pthread.h>

#include "rutabaga/types.h"
#include "rutabaga/element.h"
#include "rutabaga/shader.h"
#include "rutabaga/surface.h"
#include "rutabaga/mouse.h"
#include "rutabaga/event.h"
#include "rutabaga/font-manager.h"

#define RTB_WINDOW(x) RTB_UPCAST(x, rtb_window)
#define RTB_WINDOW_EVENT(x) RTB_UPCAST(x, rtb_event_window)

struct rtb_event_window {
	RTB_INHERIT(rtb_event);
	struct rtb_window *window;
};

struct rtb_window {
	RTB_INHERIT(rtb_surface);
	struct rtb_font_manager font_manager;

	/* public *********************************/
	struct {
		struct rtb_shader dfault;
		struct rtb_shader surface;
	} shaders;

	struct rtb_style *style_list;

	mat4 identity;

	/* private ********************************/
	struct rutabaga *rtb;

	int need_reconfigure;
	pthread_mutex_t lock;

	struct rtb_mouse mouse;
	struct rtb_element *focus;
};

void rtb_window_lock(struct rtb_window *);
void rtb_window_unlock(struct rtb_window *);

void rtb_window_draw(struct rtb_window *);
void rtb_window_reinit(struct rtb_window *);

void rtb_window_focus_element(struct rtb_window *,
		struct rtb_element *focused);

struct rtb_window *rtb_window_open(struct rutabaga *r,
		int width, int height, const char *title);
void rtb_window_close(struct rtb_window *);

void window_impl_rtb_free(struct rutabaga *rtb);
struct rutabaga *window_impl_rtb_alloc(void);
void window_impl_swap_buffers(struct rtb_window *self);
void window_impl_close(struct rtb_window *self);
struct rtb_window *window_impl_open(struct rutabaga *r,
		int width, int height, const char *title);
