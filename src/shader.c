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

#include <stdio.h>
#include <stdlib.h>

#ifdef NEED_ALLOCA_H
#include <alloca.h>
#endif

#include "rutabaga/rutabaga.h"
#include "rutabaga/shader.h"

static void print_shader_error(GLuint shader)
{
	GLint log_length;
	char *buf;

	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
	buf = alloca(log_length + 1);
	glGetShaderInfoLog(shader, log_length, NULL, buf);

	if (buf[log_length - 2] == '\n')
		buf[log_length - 2] = '\0';

	fprintf(stderr,
			"\nrtb_shader_compile(): couldn't compile shader. "
			"openGL said:\n     %s\n\n", buf);
}

static void print_program_error(GLuint program)
{
	GLint log_length;
	char *buf;

	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
	buf = alloca(log_length + 1);
	glGetProgramInfoLog(program, log_length, NULL, buf);

	if (buf[log_length - 2] == '\n')
		buf[log_length - 2] = '\0';

	fprintf(stderr,
			"\nrtb_shader_program_link(): couldn't link. "
			"openGL said:\n     %s\n\n", buf);
}

/**
 * public API
 */

GLuint rtb_shader_program_link(rtb_shader_program_t *p)
{
	GLuint program;
	GLint status;

	program = glCreateProgram();

	glAttachShader(program, p->vertex_shader);
	glAttachShader(program, p->fragment_shader);

	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &status);

	if (status == GL_TRUE) {
		p->program = program;
		return program;
	}

	print_program_error(program);
	glDeleteProgram(program);
	return 0;
}

GLuint rtb_shader_compile(GLenum type, const char *source)
{
	GLuint shader;
	GLint status;

	shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_TRUE)
		return shader;

	print_shader_error(shader);
	glDeleteShader(shader);
	return 0;
}

int rtb_shader_program_create(rtb_shader_program_t *p,
		const char *vertex_src, const char *fragment_src)
{
	int status;

	p->vertex_shader = rtb_shader_compile(GL_VERTEX_SHADER, vertex_src);
	p->fragment_shader = rtb_shader_compile(GL_FRAGMENT_SHADER, fragment_src);

	if (!p->vertex_shader || !p->fragment_shader)
		return 0;

	status = rtb_shader_program_link(p);
	if (!status)
		return 0;

	p->matrices.modelview  = glGetUniformLocation(p->program, "modelview");
	p->matrices.projection = glGetUniformLocation(p->program, "projection");

	p->offset = glGetUniformLocation(p->program, "offset");
	p->color  = glGetUniformLocation(p->program, "color");

	return status;
}

void rtb_shader_program_free(rtb_shader_program_t *p)
{
	glDetachShader(p->program, p->vertex_shader);
	glDetachShader(p->program, p->fragment_shader);

	glDeleteShader(p->vertex_shader);
	glDeleteShader(p->fragment_shader);
	glDeleteProgram(p->program);
}
