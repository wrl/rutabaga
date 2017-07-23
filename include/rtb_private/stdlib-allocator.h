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

#pragma once

/* XXX: this is a monstrous hack. i don't like this.
 *      unfortunately a lot of client-facing API calls need to be modified
 *      in order to get rid of this.
 *
 *      what i'm thinking:
 *          - struct rutabaga has its own `global` allocator for everything
 *            that's created underneath it.
 *          - anything that allocates memory needs a pointer to said
 *            struct rutabaga.
 *
 *      this is going to suck because currently we don't need any reference
 *      to the struct rutabaga until an element is attached to the tree
 *      somewhere, so this would require things like
 *
 *			rtb_button_new(const rtb_utf8_t *label);
 *
 *		to instead be
 *
 *			rtb_button_new(struct rutabaga *, const rtb_utf8_t *label);
 */

extern struct wwrl_allocator stdlib_allocator;
