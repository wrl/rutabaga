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

#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "rutabaga/rutabaga.h"
#include "rutabaga/window.h"
#include "rutabaga/dict.h"

#include "rtb_private/stdlib-allocator.h"
#include "rtb_private/window_impl.h"

struct wwrl_allocator stdlib_allocator = {
	.malloc  = malloc,
	.free    = free,
	.calloc  = calloc,
	.realloc = realloc
};

struct rutabaga *
rtb_new(void)
{
	struct rutabaga *self = window_impl_rtb_alloc();

	if (!self)
		return NULL;

	RTB_DICT_INIT(&self->atoms.type);

	memcpy(&self->allocator, &stdlib_allocator,
			sizeof(self->allocator));

	uv_loop_init(&self->event_loop);
	return self;
}

void
rtb_free(struct rutabaga *self)
{
	uv_loop_close(&self->event_loop);
	window_impl_rtb_free(self);
}

void
rtb_event_loop(struct rutabaga *r)
{
	rtb_event_loop_init(r);
	rtb_event_loop_run(r);
	rtb_event_loop_fini(r);
}
