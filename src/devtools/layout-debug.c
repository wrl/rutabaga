/**
 * rutabaga: an OpenGL widget toolkit
 * Copyright (c) 2013-2017 William Light.
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

#include <rutabaga/rutabaga.h>
#include <rutabaga/render.h>
#include <rutabaga/quad.h>

#include "rtb_private/layout-debug.h"
#include "rtb_private/util.h"

static struct rtb_quad quad;

void
rtb_debug_draw_bounding_box(struct rtb_element *self)
{
	struct rtb_render_context *ctx;
	struct rtb_rect rect = {
		.x  = self->rect.x,
		.y  = self->rect.y,
		.x2 = self->rect.x2 - 1,
		.y2 = self->rect.y2 - 1
	};

	rtb_quad_set_vertices(&quad, &rect);

	rtb_render_reset(self);
	ctx = rtb_render_get_context(self);

	rtb_render_set_position(ctx, 0.f, 0.f);

	rtb_render_set_color(ctx, 1.f, 0.f, 0.f, .4f);
	glLineWidth(1.f);
	rtb_render_quad_outline(ctx, &quad);
}

void
rtb_debug_init(void)
{
	rtb_quad_init(&quad);
}
