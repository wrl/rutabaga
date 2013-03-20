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

#include "rutabaga/style.h"
#include "private/util.h"

/**
 * styles should go from specific to general.
 * since all objects subclass from net.illest.rutabaga.object,
 * its rule will match all widgets.
 */

static struct rtb_style dark_style[] = {
	/**
	 * patchbay
	 */

	{"net.illest.rutabaga.widgets.patchbay",
		.fg = {RGB(0x0D0D0F), 1.f},
		.bg = {RGB(0x000000), 1.f}},

	{"net.illest.rutabaga.widgets.patchbay.node",
		.fg = {RGB(0xFFFFFF), 1.f},
		.bg = {RGB(0x18181C), .9f}},

	{"net.illest.rutabaga.widgets.patchbay.port",
		.fg = {RGB(0xFFFFFF), 1.f},
		.bg = {RGB(0x404F3C), 1.f}},

	/**
	 * basic stuff
	 */

	{"net.illest.rutabaga.window",
		.fg = {RGB(0xFFFFFF), 1.f},
		.bg = {RGB(0x18181C), 1.f}},

	{"net.illest.rutabaga.object",
		.fg = {RGB(0xFFFFFF), 1.f},
		.bg = {RGB(0x404F3C), 1.f}},

	{NULL}
};

static struct rtb_style *default_style = dark_style;
