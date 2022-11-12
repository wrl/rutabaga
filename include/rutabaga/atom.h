/**
 * rutabaga: an OpenGL widget toolkit
 * Copyright (c) 2013-2018 William Light.
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

#include <rutabaga/rutabaga.h>
#include <rutabaga/types.h>
#include <rutabaga/dict.h>

#define RTB_ATOM(x) RTB_UPCAST(x, rtb_atom)
#define RTB_TYPE_ATOM(x) RTB_UPCAST(x, rtb_type_atom)

#define RTB_ATOM_DESCRIPTOR(x) RTB_UPCAST(x, rtb_atom_descriptor)

typedef enum {
	RTB_TYPE_ATOM
} rtb_atom_metatype_t;

struct rtb_atom {
	rtb_atom_metatype_t metatype;
};

struct rtb_type_atom {
	RTB_INHERIT(rtb_atom);
	struct rtb_type_atom_descriptor *type;
};

struct rtb_atom_descriptor {
	const char *name;
	int ref_count;

	void *dict;
	RTB_DICT_ENTRY(rtb_atom_descriptor) dict_entry;
};

struct rtb_type_atom_descriptor {
	RTB_INHERIT(rtb_atom_descriptor);
	struct rtb_type_atom_descriptor *super[0];
};

/**
 * public API
 */

struct rtb_window;

struct rtb_type_atom_descriptor *rtb_type_lookup(
		struct rtb_window *win, const char *type_name);
int rtb_is_type(struct rtb_type_atom_descriptor *desc,
		struct rtb_type_atom *atom);

struct rtb_type_atom_descriptor *rtb_type_ref(struct rtb_window *win,
		struct rtb_type_atom_descriptor *super, const char *type_name);
int rtb_type_unref(struct rtb_type_atom_descriptor *type);

struct rutabaga;
void rtb_free_all_types(struct rutabaga *);
