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

#include "rutabaga/types.h"
#include "rutabaga/shader.h"
#include "rutabaga/quad.h"

#include "bsd/queue.h"

struct rtb_render_context {
	struct rtb_shader *shader;

	struct {
		TAILQ_HEAD(rtb_render_tailq, rtb_element) every_frame;
		struct rtb_render_tailq next_frame;
	} queues;
};

void rtb_render_use_style_bg(struct rtb_element *);
void rtb_render_use_style_fg(struct rtb_element *);

void rtb_render_set_color(struct rtb_element *,
		GLfloat r, GLfloat g, GLfloat b, GLfloat a);
void rtb_render_set_position(struct rtb_element *, float x, float y);
void rtb_render_set_modelview(struct rtb_element *, const GLfloat *matrix);

void rtb_render_quad_outline(struct rtb_element *, struct rtb_quad *);
void rtb_render_quad(struct rtb_element *, struct rtb_quad *);
void rtb_render_clear(struct rtb_element *);

void rtb_render_use_shader(struct rtb_element *, struct rtb_shader *);
void rtb_render_reset(struct rtb_element *);
void rtb_render_push(struct rtb_element *);
void rtb_render_pop(struct rtb_element *);
