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
#include "rutabaga/object.h"
#include "rutabaga/shader.h"
#include "rutabaga/surface.h"
#include "rutabaga/mouse.h"

#define RTB_WINDOW(x) RTB_UPCAST(x, rtb_window)

struct rtb_window {
	RTB_INHERIT(rtb_surface);
	rtb_font_manager_t *font_manager;

	/* public *********************************/
	struct {
		struct rtb_shader dfault;
		struct rtb_shader surface;
	} shaders;

	rtb_style_t *style_list;

	mat4 identity;

	/* private ********************************/
	rtb_t *rtb;
	pthread_mutex_t lock;
	int need_reconfigure;
	rtb_mouse_t mouse;
	rtb_obj_t *focus;
};

void rtb_window_lock(rtb_win_t *);
void rtb_window_unlock(rtb_win_t *);

void rtb_window_draw(rtb_win_t *);
void rtb_window_reinit(rtb_win_t *);

void rtb_window_focus_object(rtb_win_t *, rtb_obj_t *focused);

rtb_win_t *rtb_window_open(rtb_t *r,
		int width, int height, const char *title);
void rtb_window_close(rtb_win_t *);

void window_impl_rtb_free(rtb_t *rtb);
rtb_t *window_impl_rtb_alloc(void);
void window_impl_swap_buffers(rtb_win_t *self);
void window_impl_close(rtb_win_t *self);
rtb_win_t *window_impl_open(rtb_t *r,
		int width, int height, const char *title);
