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

#include <stdint.h>

/**
 * inheritance/composition helpers
 */

#define RTB_INHERITED_MEMBER(type) _parent_##type

#define RTB_INHERIT_AS(from, as)	\
	union {				\
		struct from;		\
		struct from as;		\
	}

#define RTB_INHERIT(from)		\
	RTB_INHERIT_AS(from, RTB_INHERITED_MEMBER(from))

/**
 * adapted from
 * http://stackoverflow.com/questions/10269685/kernels-container-of-any-way-to-make-it-iso-conforming
 */
#ifdef __GNUC__
#define RTB_CONTAINER_OF(ptr, type, member)	\
	((type *)				\
	 ((char *)(__typeof__(((type *) 0)->member) *){ptr} - offsetof(type, member)))
#else
#define RTB_CONTAINER_OF(ptr, type, member)	\
	((type *)				\
	 (char *)(void *){ptr} - offsetof(type, member))
#endif

#define RTB_UPCAST(x, to) (&(x)->RTB_INHERITED_MEMBER(to))
#define RTB_DOWNCAST(x, to, from)		\
	RTB_CONTAINER_OF(x, struct to, RTB_INHERITED_MEMBER(from))

/**
 * enumerations
 */

typedef enum {
	RTB_DRAW_NORMAL = 0,
	RTB_DRAW_FOCUS  = 1,
	RTB_DRAW_HOVER  = 2,
	RTB_DRAW_ACTIVE = 3,

	RTB_DRAW_STATE_MAX = RTB_DRAW_ACTIVE
} rtb_draw_state_t;

typedef enum {
	RTB_FULLY_OBSCURED     = 0x0,
	RTB_PARTIALLY_OBSCURED = 0x1,
	RTB_UNOBSCURED         = 0x2
} rtb_visibility_t;

typedef enum {
	RTB_ALIGN_HORIZONTAL = 0x0F,
	RTB_ALIGN_VERTICAL   = 0xF0,

	RTB_ALIGN_LEFT   = 0x00,
	RTB_ALIGN_CENTER = 0x01,
	RTB_ALIGN_RIGHT  = 0x02,

	RTB_ALIGN_TOP    = 0x00,
	RTB_ALIGN_MIDDLE = 0x10,
	RTB_ALIGN_BOTTOM = 0x20,
} rtb_alignment_t;

typedef enum {
	RTB_ADD_HEAD,
	RTB_ADD_TAIL
} rtb_child_add_loc_t;

/**
 * basic types
 */

typedef char rtb_utf8_t;
typedef int32_t rtb_utf32_t;
