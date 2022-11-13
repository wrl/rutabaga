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

#pragma once

#include <uv.h>

#include <rutabaga/types.h>
#include <rutabaga/element.h>
#include <rutabaga/shader.h>
#include <rutabaga/surface.h>
#include <rutabaga/mouse.h>
#include <rutabaga/event.h>
#include <rutabaga/font-manager.h>

#define RTB_WINDOW(x) RTB_UPCAST(x, rtb_window)
#define RTB_WINDOW_AS(x, type) RTB_DOWNCAST(x, type, rtb_window)

#define RTB_WINDOW_EVENT(x) RTB_UPCAST(x, rtb_window_event)

struct rtb_window_event {
	RTB_INHERIT(rtb_event);
	struct rtb_window *window;
};

struct rtb_window_local_storage {
	struct {
		struct rtb_shader dfault;
		struct rtb_shader surface;
		struct rtb_shader stylequad;
	} shader;

	struct {
		struct {
			GLuint border;
			GLuint solid;
			GLuint outline;
		} stylequad;

		struct {
			GLuint solid;
			GLuint outline;
		} quad;
	} ibo;
};

struct rtb_window {
	RTB_INHERIT(rtb_surface);
	struct rtb_font_manager font_manager;

	/* public *********************************/
	struct rtb_window_local_storage local_storage;

	struct rtb_style *style_list;
	struct rtb_font *style_fonts;

	/* private ********************************/
	int finished_initialising;
	int need_reconfigure;
	int dpi_changed;
	int dirty;

	struct {
		int x;
		int y;
	} dpi;

	struct rtb_point scale;
	struct rtb_point scale_recip;

	struct rutabaga *rtb;

	GLuint vao;

	uv_mutex_t lock;

	struct rtb_mouse mouse;
	struct rtb_element *focus;

	int mouse_in_overlay;
	struct rtb_surface overlay_surface;
};

/**
 * return value indicates whether anything was actually drawn.
 * returning 0 means that nothing was marked dirty and/or the window isn't
 * visible. returning 1 means that drawing occurred.
 *
 * passing a 1 for force_redraw skips the window dirtiness check.
 */
int rtb_window_draw(struct rtb_window *, int force_redraw);

void rtb_window_focus_element(struct rtb_window *,
		struct rtb_element *focused);

void rtb_window_lock(struct rtb_window *);
void rtb_window_unlock(struct rtb_window *);

void rtb_window_reinit(struct rtb_window *);
void rtb_window_set_scale(struct rtb_window *, struct rtb_point new_scale);

/**
 * overlays
 */

void rtb_window_add_overlay(struct rtb_window *, struct rtb_element *,
		rtb_child_add_loc_t where);

void rtb_window_remove_overlay(struct rtb_window *, struct rtb_element *);

/**
 * opening/closing
 */

typedef enum {
	RTB_WINDOW_OPEN_AS_IF_PARENTED = 1
} rtb_window_open_option_flags_t;

struct rtb_window_open_options {
	rtb_window_open_option_flags_t flags;

	const char *title;

	int width;
	int height;

	intptr_t parent;
};

#define rtb_window_open_ez(RTB, ...) \
	rtb_window_open(RTB, &((struct rtb_window_open_options) __VA_ARGS__));

struct rtb_window *rtb_window_open(struct rutabaga *,
		const struct rtb_window_open_options *);
void rtb_window_close(struct rtb_window *);

intptr_t rtb_window_get_native_handle(struct rtb_window *);
