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

#include "rutabaga/types.h"

static inline float halign(float havail, float w, rtb_alignment_t align)
{
	switch (align & RTB_ALIGN_HORIZONTAL) {
	case RTB_ALIGN_CENTER:
		return (havail - w) / 2.f;

	case RTB_ALIGN_RIGHT:
		return havail - w;

	default:
		return 0.f;
	}
}

static inline float valign(float vavail, float h, rtb_alignment_t align)
{
	switch (align & RTB_ALIGN_VERTICAL) {
	case RTB_ALIGN_MIDDLE:
		return (vavail - h) / 2.f;

	case RTB_ALIGN_BOTTOM:
		return vavail - h;

	default:
		return 0.f;
	}
}
