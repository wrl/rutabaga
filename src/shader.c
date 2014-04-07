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

static void
print_shader_error(GLuint shader)
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

static void
print_program_error(GLuint program)
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

static GLuint
shader_link(struct rtb_shader *shader)
{
	GLuint program;
	GLint status;

	program = glCreateProgram();

	glAttachShader(program, shader->vertex_shader);
	glAttachShader(program, shader->fragment_shader);

	if (shader->geometry_shader)
		glAttachShader(program, shader->geometry_shader);

	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &status);

	if (status == GL_TRUE) {
		shader->program = program;
		return program;
	}

	print_program_error(program);
	glDeleteProgram(program);
	return 0;
}

static GLuint
glsl_compile(GLenum type, const char *source)
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

/**
 * public API
 */

int
rtb_shader_create(struct rtb_shader *shader,
		const char *vertex_src, const char *geometry_src,
		const char *fragment_src)
{
	GLuint program;
	int status;

	shader->vertex_shader = glsl_compile(GL_VERTEX_SHADER, vertex_src);
	shader->fragment_shader = glsl_compile(GL_FRAGMENT_SHADER, fragment_src);

	if (geometry_src)
		shader->geometry_shader = glsl_compile(GL_GEOMETRY_SHADER, geometry_src);
	else
		shader->geometry_shader = 0;

	if (!shader->vertex_shader || !shader->fragment_shader
			|| (geometry_src && !shader->geometry_shader))
		return 0;

	status = shader_link(shader);
	if (!status)
		return 0;

	program = shader->program;

#define CACHE_ATTRIBUTE(NAME) \
	shader->NAME = glGetAttribLocation(program, #NAME)
#define CACHE_UNIFORM(TO, NAME) \
	shader->TO = glGetUniformLocation(program, NAME)
#define CACHE_SIMPLE_UNIFORM(NAME) CACHE_UNIFORM(NAME, #NAME)
#define CACHE_MATRIX_UNIFORM(NAME) CACHE_UNIFORM(matrices.NAME, #NAME)

	CACHE_MATRIX_UNIFORM(modelview);
	CACHE_MATRIX_UNIFORM(projection);

	CACHE_SIMPLE_UNIFORM(offset);
	CACHE_SIMPLE_UNIFORM(color);
	CACHE_SIMPLE_UNIFORM(texture);
	CACHE_SIMPLE_UNIFORM(texture_size);

	CACHE_ATTRIBUTE(vertex);
	CACHE_ATTRIBUTE(tex_coord);

	return status;
}

void
rtb_shader_free(struct rtb_shader *shader)
{
	glDetachShader(shader->program, shader->vertex_shader);
	glDetachShader(shader->program, shader->fragment_shader);

	glDeleteShader(shader->vertex_shader);
	glDeleteShader(shader->fragment_shader);

	if (shader->geometry_shader)
		glDeleteShader(shader->geometry_shader);

	glDeleteProgram(shader->program);
}
