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
#include <rutabaga/quad.h>

void
rtb_quad_set_vertices(struct rtb_quad *self, struct rtb_rect *from)
{
	GLfloat v[4][2] = {
		{from->x,  from->y},
		{from->x2, from->y},
		{from->x2, from->y2},
		{from->x,  from->y2}
	};

	glBindBuffer(GL_ARRAY_BUFFER, self->vertices);
	glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void
rtb_quad_set_tex_coords(struct rtb_quad *self, struct rtb_rect *from)
{
	GLfloat v[4][2] = {
		{from->x,  from->y},
		{from->x2, from->y},
		{from->x2, from->y2},
		{from->x,  from->y2}
	};

	if (!self->tex_coords)
		glGenBuffers(1, &self->tex_coords);

	glBindBuffer(GL_ARRAY_BUFFER, self->tex_coords);
	glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void
rtb_quad_init(struct rtb_quad *self)
{
	glGenBuffers(1, &self->vertices);
	self->tex_coords = 0;
}

void
rtb_quad_fini(struct rtb_quad *self)
{
#define FREE_BUFFER_IF_USED(buf) if (self->buf) glDeleteBuffers(1, &self->buf)

	FREE_BUFFER_IF_USED(tex_coords);
	FREE_BUFFER_IF_USED(vertices);
}
