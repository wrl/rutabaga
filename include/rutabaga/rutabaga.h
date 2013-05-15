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

#include <glloadgen/gl_core.3.0.h>

#include "rutabaga/defaults.h"
#include "rutabaga/types.h"
#include "rutabaga/atom.h"
#include "rutabaga/dict.h"

#include "wwrl/allocator.h"

/**
 * general utility macros
 */

#define RTB_POINT_IN_RECT(pt, rect) \
	(((pt.x >= rect.p1.x) && (pt.x <= rect.p2.x)) \
	 && ((pt.y >= rect.p1.y) && (pt.y <= rect.p2.y)))

/**
 * structures
 */

struct rutabaga {
	/* private ********************************/
	/* XXX: need to be able to handle several of these */
	rtb_win_t *win;
	int run_event_loop;

	struct {
		RTB_DICT(rtb_atom_dict, rtb_atom_descriptor) type;
	} atoms;

	struct wwrl_allocator allocator;
};

void rtb_stop_event_loop(rtb_t *self);

rtb_t *rtb_init(void);
void rtb_destroy(rtb_t *rutabaga);
