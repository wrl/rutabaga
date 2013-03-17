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

#include "rutabaga/rutabaga.h"
#include "rutabaga/window.h"
#include "rutabaga/render.h"

#define CONTEXT (obj->window->render_ctx)

void rtb_render_use_program(rtb_obj_t *obj, rtb_shader_program_t *p)
{
	GLuint program;

	if (!p)
		p = &obj->window->default_shader;

	program = p->program;
	CONTEXT.shader = p;

	glUseProgram(program);

	glUniformMatrix4fv(p->matrices.projection,
		1, GL_FALSE, obj->surface->projection.data);
	glUniformMatrix4fv(p->matrices.modelview,
		1, GL_FALSE, obj->window->identity.data);
}

void rtb_render_set_position(rtb_obj_t *obj, float x, float y)
{
	glUniform2f(CONTEXT.shader->offset, x, y);
}

void rtb_render_set_color(rtb_obj_t *obj,
		GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
	glUniform4f(CONTEXT.shader->color, r, g, b, a);
}

void rtb_render_set_modelview(rtb_obj_t *obj, const GLfloat *matrix)
{
	glUniformMatrix4fv(CONTEXT.shader->matrices.modelview,
		1, GL_FALSE, matrix);
}

void rtb_render_push(rtb_obj_t *obj)
{
	rtb_render_use_program(obj, NULL);

	glScissor(obj->x - obj->surface->x,
			obj->surface->y + obj->surface->h - obj->h - obj->y,
			obj->w, obj->h);

	glEnable(GL_SCISSOR_TEST);
}

void rtb_render_clear(rtb_obj_t *obj)
{
	glClearColor(0.f, 0.f, 0.f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT);
}

void rtb_render_pop(rtb_obj_t *obj)
{
	glUseProgram(0);
}
