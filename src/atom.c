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

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "rutabaga/rutabaga.h"
#include "rutabaga/window.h"
#include "rutabaga/dict.h"
#include "rutabaga/atom.h"

typedef unsigned int uint_t;

/**
 * meiyan hash from http://www.sanmayce.com/Fastest_Hash/
 */

#ifndef _rotl
# define _rotl(x, n) (((x) << (n)) | ((x) >> (32-(n))))
# define _rotr(x, n) (((x) << (n)) | ((x) >> (32-(n))))
#endif

#ifndef __forceinline
# define __forceinline inline
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
static unsigned int
hash_meiyan(const char *str, size_t wrdlen)
{
	const unsigned int PRIME = 709607;
	unsigned int hash32 = 2166136261;
	const char *p = str;

	for(; wrdlen >= 2 * sizeof(uint32_t);
			wrdlen -= 2 * sizeof(uint32_t),
			p      += 2 * sizeof(uint32_t)) {
		hash32 = (hash32 ^ (_rotl(*(uint32_t *)p,5) ^ *(uint32_t *)(p+4))) * PRIME;
	}

	// Cases: 0,1,2,3,4,5,6,7
	if (wrdlen & sizeof(uint32_t)) {
		hash32 = (hash32 ^ *(uint16_t*)p) * PRIME;
		p += sizeof(uint16_t);
		hash32 = (hash32 ^ *(uint16_t*)p) * PRIME;
		p += sizeof(uint16_t);
	}

	if (wrdlen & sizeof(uint16_t)) {
		hash32 = (hash32 ^ *(uint16_t*)p) * PRIME;
		p += sizeof(uint16_t);
	}

	if (wrdlen & 1)
		hash32 = (hash32 ^ *p) * PRIME;

	return hash32 ^ (hash32 >> 16);
}
#pragma GCC diagnostic pop

#define HASH(str, len) hash_meiyan(str, len)

/**
 * nedtries stuff
 */

static size_t
dict_key_func(const struct rtb_atom_descriptor *node)
{
	return node->dict_entry.hash;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
NEDTRIE_GENERATE(static, rtb_atom_dict, rtb_atom_descriptor,
		dict_entry.trie_entry, dict_key_func,
		NEDTRIE_NOBBLEZEROS(rtb_atom_dict));
#pragma GCC diagnostic pop

static struct rtb_type_atom_descriptor *
find_type_descriptor(struct rtb_atom_dict *dict,
		uint_t hash, const char *type_name, size_t len)
{
	struct rtb_atom_descriptor needle = {.dict_entry.hash = hash}, *ret;

	ret = NEDTRIE_FIND(rtb_atom_dict, dict, &needle);

	while (ret && strcmp(type_name, ret->name))
		ret = NEDTRIE_NEXTLEAF(rtb_atom_dict, ret);

	return (struct rtb_type_atom_descriptor *) ret;
}

static struct rtb_type_atom_descriptor *
alloc_type_descriptor(uint_t hash, const char *type_name, size_t len,
		struct rtb_type_atom_descriptor *supertype)
{
	struct rtb_type_atom_descriptor *ret, **cursor;
	size_t need = sizeof(*ret), name_start;
	int supertypes = 0;
	char *name;

	if (supertype)
		for (cursor = supertype->super, supertypes = 1;
				*cursor; cursor++, supertypes++);

	need += (supertypes + 1) * sizeof(struct rtb_atom_descriptor *);

	name_start = need;
	need += len + 1;
	ret = calloc(1, need);

	ret->dict_entry.hash = hash;
	ret->ref_count = 0;
	ret->name = name = ((char *) ret) + name_start;

	strncpy(name, type_name, len);
	name[len] = '\0';

	if (supertype) {
		ret->super[0] = supertype;

		/* copy parent's supertype array into ours */
		memcpy(&ret->super[1], supertype->super,
				(supertypes - 1) * sizeof(*ret->super));
	}

	ret->super[supertypes] = NULL;

	return ret;
}


/**
 * RTB_ATOM_TYPE public API
 */

struct rtb_type_atom_descriptor *
rtb_type_lookup(struct rtb_window *win, const char *type_name)
{
	uint_t hash;
	int len;

	len = strlen(type_name);
	hash = HASH(type_name, len);

	return find_type_descriptor(&win->rtb->atoms.type, hash, type_name, len);
}

int
rtb_is_type(struct rtb_type_atom_descriptor *desc,
		struct rtb_type_atom *atom)
{
	struct rtb_type_atom_descriptor *type_a, *type_b;

	type_a = desc;
	type_b = atom->type;

	do {
		if (type_a == type_b)
			return 1;
	} while ((type_b = type_b->super[0]));

	return 0;
}

struct rtb_type_atom_descriptor *
rtb_type_ref(struct rtb_window *win, struct rtb_type_atom_descriptor *super,
		const char *type_name)
{
	struct rtb_atom_dict *dict = &win->rtb->atoms.type;
	struct rtb_type_atom_descriptor *type, *supertype;
	uint_t hash;
	int len;

	supertype = (struct rtb_type_atom_descriptor *) super;

	len = strlen(type_name);
	hash = HASH(type_name, len);

	type = find_type_descriptor(dict, hash, type_name, len);

	if (!type) {
		if (!(type = alloc_type_descriptor(hash, type_name, len, supertype)))
			return NULL;

		type->dict = dict;
		NEDTRIE_INSERT(rtb_atom_dict, dict, RTB_ATOM_DESCRIPTOR(type));
	}

	type->ref_count++;
	return type;
}

int
rtb_type_unref(struct rtb_type_atom_descriptor *type)
{
	if (!type)
		return -1;

	rtb_type_unref(type->super[0]);

	if (!--type->ref_count) {
		NEDTRIE_REMOVE(rtb_atom_dict, type->dict, RTB_ATOM_DESCRIPTOR(type));
		free(type);
		return 0;
	}

	return type->ref_count;
}
