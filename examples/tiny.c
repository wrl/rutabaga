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

struct fuck {
	GLuint vbo, vao, ibo;
	struct rtb_rect rect;
};

static const GLubyte quad_tri_indices[] = {
	0, 1, 3, 2
};

static const GLfloat identity_matrix[] = {
	1.f, 0.f, 0.f, 0.f,
	0.f, 1.f, 0.f, 0.f,
	0.f, 0.f, 1.f, 0.f,
	0.f, 0.f, 0.f, 1.f
};

static void
fuck_init(struct fuck *fuck)
{
	glGenBuffers(1, &fuck->ibo);
	glGenBuffers(1, &fuck->vbo);
	glGenVertexArrays(1, &fuck->vao);

	printf(" fuck %d %d %d\n",
			fuck->ibo,
			fuck->vbo,
			fuck->vao);
}

#define GLE(exp) do {														\
	GLuint err;																\
	exp;																	\
	if ((err = glGetError()))												\
		printf(" :: " #exp " = %d\n", err);									\
} while(0)

static void
fuck_draw(struct rtb_window *win, struct fuck *fuck)
{
	GLint vertex, color, offset, projection, modelview;

	GLuint program;
	GLfloat v[4][2] = {
		{fuck->rect.x,  fuck->rect.y},
		{fuck->rect.x2, fuck->rect.y},
		{fuck->rect.x2, fuck->rect.y2},
		{fuck->rect.x,  fuck->rect.y2}
	};

	printf(" :: start: %d\n", glGetError());

	program = win->local_storage.shader.dfault.program;
	GLE(vertex     = glGetAttribLocation(program, "vertex"));
	GLE(color      = glGetUniformLocation(program, "color"));
	GLE(offset     = glGetUniformLocation(program, "offset"));
	GLE(projection = glGetUniformLocation(program, "projection"));
	GLE(modelview  = glGetUniformLocation(program, "modelview"));

	GLE(glUseProgram(program));

	GLE(glBindBuffer(GL_ARRAY_BUFFER, fuck->vbo));
	GLE(glBindVertexArray(fuck->vao));

	GLE(glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW));
	GLE(glEnableVertexAttribArray(vertex));
	GLE(glVertexAttribPointer(vertex, 2, GL_FLOAT, GL_FALSE, 0, 0));
	GLE(glBindBuffer(GL_ARRAY_BUFFER, 0));

	GLE(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, fuck->ibo));
	GLE(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_tri_indices), quad_tri_indices, GL_STATIC_DRAW));

	GLE(glUniform2f(offset, 0.f, 0.f));
	GLE(glUniformMatrix4fv(projection, 1, GL_FALSE, identity_matrix));
	GLE(glUniformMatrix4fv(modelview, 1, GL_FALSE, identity_matrix));
	GLE(glUniform4f(color, 1.f, 0.f, 0.f, 1.f));

	GLE(glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, 0));

	GLE(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
	GLE(glDisableVertexAttribArray(vertex));
	puts("");
}

/********************************/

static int
frame_start(struct rtb_element *elem, const struct rtb_event *e, void *ctx)
{
	return 1;
}

static int
frame_end(struct rtb_element *elem, const struct rtb_event *e, void *ctx)
{
	struct rtb_window *win = RTB_ELEMENT_AS(elem, rtb_window);
	struct fuck *fuck = ctx;

	fuck->rect = (struct rtb_rect) {
		.x  = 0.f,
		.y  = 0.f,
		.x2 = 0.5f,
		.y2 = 0.5f,
	};

	fuck_draw(win, fuck);
	return 1;
}

int main(int argc, char **argv)
{
	struct rutabaga *delicious;
	struct rtb_window *win;
	struct fuck fuck;

	delicious = rtb_new();
	assert(delicious);
	win = rtb_window_open(delicious, 450, 600, "~delicious~");
	assert(win);

	win->outer_pad.x = 5.f;
	win->outer_pad.y = 5.f;

	rtb_elem_set_size_cb(RTB_ELEMENT(win), rtb_size_hfit_children);
	rtb_elem_set_layout(RTB_ELEMENT(win), rtb_layout_vpack_bottom);

	rtb_register_handler(RTB_ELEMENT(win),
			RTB_FRAME_START, frame_start, NULL);
	rtb_register_handler(RTB_ELEMENT(win),
			RTB_FRAME_END, frame_end, &fuck);

	fuck_init(&fuck);

	rtb_event_loop(delicious);

	rtb_window_close(delicious->win);
	rtb_free(delicious);
}
