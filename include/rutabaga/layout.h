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

#include "rutabaga/rutabaga.h"

void rtb_size_self(rtb_obj_t *self, const struct rtb_size *avail,
		struct rtb_size *want);

void rtb_size_hfit_children(rtb_obj_t *self, const struct rtb_size *avail,
		struct rtb_size *want);

void rtb_size_vfit_children(rtb_obj_t *self, const struct rtb_size *avail,
		struct rtb_size *want);

void rtb_size_hfill(rtb_obj_t *self, const struct rtb_size *avail,
		struct rtb_size *want);

void rtb_size_vfill(rtb_obj_t *self, const struct rtb_size *avail,
		struct rtb_size *want);

void rtb_size_fill(rtb_obj_t *self, const struct rtb_size *avail,
		struct rtb_size *want);

void rtb_layout_unmanaged(rtb_obj_t *obj);

void rtb_layout_vpack_top(rtb_obj_t *self);
void rtb_layout_vpack_middle(rtb_obj_t *self);
void rtb_layout_vpack_bottom(rtb_obj_t *self);

void rtb_layout_hpack_left(rtb_obj_t *self);
void rtb_layout_hpack_center(rtb_obj_t *self);
void rtb_layout_hpack_right(rtb_obj_t *self);

void rtb_layout_hdistribute(rtb_obj_t *);
