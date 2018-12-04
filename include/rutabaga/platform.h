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

#include <rutabaga/types.h>
#include <rutabaga/geometry.h>
#include <rutabaga/window.h>
#include <rutabaga/mouse.h>

/******************************
 * from platform, to rutabaga
 ******************************/

/**
 * mouse
 */

void rtb__platform_mouse_press(struct rtb_window *,
		int buttons, struct rtb_phy_point);
void rtb__platform_mouse_release(struct rtb_window *,
		int buttons, struct rtb_phy_point);
void rtb__platform_mouse_motion(struct rtb_window *, struct rtb_phy_point);

void rtb__platform_mouse_wheel(struct rtb_window *, struct rtb_phy_point,
		float delta);

void rtb__platform_mouse_enter_window(struct rtb_window *, struct rtb_phy_point);
void rtb__platform_mouse_leave_window(struct rtb_window *, struct rtb_phy_point);

/******************************
 * from rutabaga, to platform
 ******************************/

/**
 * should return the number of nanoseconds inside of which two clicks
 * will be considered a double-click.
 *
 * will be called on every mouse click, so should be reasonably efficient.
 */
int64_t rtb_mouse_double_click_interval(struct rtb_window *);

void rtb__platform_set_cursor(struct rtb_window *, struct rtb_mouse *,
		rtb_mouse_cursor_t cursor);
void rtb_mouse_pointer_warp(struct rtb_window *, int x, int y);

void rtb_copy_to_clipboard(struct rtb_window *, const rtb_utf8_t *buf,
		size_t nbytes);
ssize_t rtb_paste_from_clipboard(struct rtb_window *, rtb_utf8_t **buf);
