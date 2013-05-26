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
#include "rutabaga/style.h"
#include "rutabaga/quad.h"

#include "private/util.h"

#define CONTEXT (obj->window->render_ctx)

static const GLubyte quad_tri_indices[] = {
	0, 1, 3, 2
};

static const GLubyte quad_outline_indices[] = {
	0, 1, 2, 3
};

static struct rtb_style_props *get_styleprops(struct rtb_object *obj,
		rtb_draw_state_t state)
{
	if (obj->style->available_styles & (1 << state))
		return &obj->style->states[state];
	else
		return &obj->style->states[RTB_DRAW_NORMAL];
}

/**
 * public API
 *
 * shader variables
 */

void rtb_render_use_style_bg(struct rtb_object *obj, rtb_draw_state_t state)
{
	struct rtb_rgb_color *color = &(get_styleprops(obj, state))->bg;

	rtb_render_set_color(obj,
			color->r,
			color->g,
			color->b,
			color->a);
}

void rtb_render_use_style_fg(struct rtb_object *obj, rtb_draw_state_t state)
{
	struct rtb_rgb_color *color = &(get_styleprops(obj, state))->fg;

	rtb_render_set_color(obj,
			color->r,
			color->g,
			color->b,
			color->a);
}

void rtb_render_set_color(struct rtb_object *obj,
		GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
	glUniform4f(CONTEXT.shader->color, r, g, b, a);
}

void rtb_render_set_position(struct rtb_object *obj, float x, float y)
{
	glUniform2f(CONTEXT.shader->offset, x, y);
}

void rtb_render_set_modelview(struct rtb_object *obj, const GLfloat *matrix)
{
	glUniformMatrix4fv(CONTEXT.shader->matrices.modelview,
		1, GL_FALSE, matrix);
}

/**
 * quad drawing
 */

static void render_quad(struct rtb_object *obj, struct rtb_quad *quad,
		GLenum mode, const GLubyte *indices, GLsizei count)
{
	struct rtb_shader *shader = CONTEXT.shader;

	if (!quad->vertices)
		return;

	glBindBuffer(GL_ARRAY_BUFFER, quad->vertices);
	glEnableVertexAttribArray(shader->vertex);
	glVertexAttribPointer(shader->vertex, 2, GL_FLOAT, GL_FALSE, 0, 0);

	if (quad->tex_coords) {
		glBindBuffer(GL_ARRAY_BUFFER, quad->tex_coords);
		glEnableVertexAttribArray(shader->tex_coord);
		glVertexAttribPointer(shader->tex_coord,
				2, GL_FLOAT, GL_FALSE, 0, 0);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glDrawElements(mode, count, GL_UNSIGNED_BYTE, indices);

	glDisableVertexAttribArray(shader->vertex);

	if (quad->tex_coords)
		glDisableVertexAttribArray(shader->tex_coord);
}

void rtb_render_quad_outline(struct rtb_object *obj, struct rtb_quad *quad)
{
	render_quad(obj, quad, GL_LINE_LOOP,
			quad_outline_indices, ARRAY_LENGTH(quad_outline_indices));
}

void rtb_render_quad(struct rtb_object *obj, struct rtb_quad *quad)
{
	render_quad(obj, quad, GL_TRIANGLE_STRIP,
			quad_tri_indices, ARRAY_LENGTH(quad_tri_indices));
}

void rtb_render_clear(struct rtb_object *obj)
{
	glClearColor(0.f, 0.f, 0.f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT);
}

/**
 * state changes
 */

void rtb_render_use_shader(struct rtb_object *obj, struct rtb_shader *shader)
{
	GLuint program;

	if (!shader)
		shader = &obj->window->shaders.dfault;

	program = shader->program;
	CONTEXT.shader = shader;

	glUseProgram(program);

	glUniformMatrix4fv(shader->matrices.projection,
		1, GL_FALSE, obj->surface->projection.data);
	glUniformMatrix4fv(shader->matrices.modelview,
		1, GL_FALSE, obj->window->identity.data);
}

void rtb_render_reset(struct rtb_object *obj)
{
	rtb_render_use_shader(obj, NULL);

	glScissor(obj->x - obj->surface->x,
			obj->surface->y + obj->surface->h - obj->h - obj->y,
			obj->w, obj->h);

	glEnable(GL_SCISSOR_TEST);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void rtb_render_push(struct rtb_object *obj)
{
	rtb_render_reset(obj);
}

void rtb_render_pop(struct rtb_object *obj)
{
	glUseProgram(0);
}
