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

#include "rutabaga/types.h"
#include "rutabaga/element.h"
#include "rutabaga/surface.h"
#include "rutabaga/event.h"
#include "rutabaga/quad.h"

#include "rutabaga/widgets/label.h"

#define RTB_PATCHBAY(x) RTB_UPCAST(x, rtb_patchbay)
#define RTB_PATCHBAY_NODE(x) RTB_UPCAST(x, rtb_patchbay_node)
#define RTB_PATCHBAY_PORT(x) RTB_UPCAST(x, rtb_patchbay_port)

/**
 * structures
 */

typedef enum {
	PORT_TYPE_INPUT,
	PORT_TYPE_OUTPUT
} rtb_patchbay_port_type_t;

struct rtb_patchbay_node {
	RTB_INHERIT(rtb_element);

	struct rtb_element node_ui;

	/* private ********************************/
	struct rtb_element container;
	struct rtb_element input_ports;
	struct rtb_element output_ports;

	struct rtb_label name_label;
	struct rtb_patchbay *patchbay;
};

struct rtb_patchbay_port {
	RTB_INHERIT(rtb_element);

	/* private ********************************/
	struct rtb_label label;
	rtb_patchbay_port_type_t port_type;

	struct rtb_patchbay_node *node;
	TAILQ_HEAD(port_patches, rtb_patchbay_patch) patches;
};

struct rtb_patchbay_patch {
	struct rtb_patchbay_port *from;
	struct rtb_patchbay_port *to;

	/* private ********************************/
	TAILQ_ENTRY(rtb_patchbay_patch) patchbay_patch;
	TAILQ_ENTRY(rtb_patchbay_patch) from_patch;
	TAILQ_ENTRY(rtb_patchbay_patch) to_patch;
};

struct rtb_patchbay {
	RTB_INHERIT(rtb_surface);

	/* private ********************************/
	GLuint bg_vbo[2];
	GLuint bg_texture;
	struct rtb_point texture_offset;

	TAILQ_HEAD(patchbay_patches, rtb_patchbay_patch) patches;
	struct {
		struct rtb_patchbay_port *from;
		struct rtb_patchbay_port *to;

		struct rtb_point cursor;
	} patch_in_progress;
};

/**
 * events
 */

typedef enum {
	RTB_PATCHBAY_CONNECT,
	RTB_PATCHBAY_DISCONNECT
} rtb_patchbay_event_type_t;

struct rtb_patchbay_event_connect {
	RTB_INHERIT(rtb_event);

	struct {
		struct rtb_patchbay_node *node;
		struct rtb_patchbay_port *port;
	} from, to;
};

struct rtb_patchbay_event_disconnect {
	RTB_INHERIT(rtb_patchbay_event_connect);
	struct rtb_patchbay_patch *patch;
};

/**
 * public API
 */

int rtb_patchbay_are_ports_connected(struct rtb_patchbay_port *a,
		struct rtb_patchbay_port *b);
void rtb_patchbay_free_patch(struct rtb_patchbay *self,
		struct rtb_patchbay_patch *patch);
void rtb_patchbay_disconnect_ports(struct rtb_patchbay *self,
		struct rtb_patchbay_port *a, struct rtb_patchbay_port *b);
struct rtb_patchbay_patch *rtb_patchbay_connect_ports(struct rtb_patchbay *,
		struct rtb_patchbay_port *a, struct rtb_patchbay_port *b);

int rtb_patchbay_port_init(struct rtb_patchbay_port *port,
		struct rtb_patchbay_node *node, const rtb_utf8_t *name,
		rtb_patchbay_port_type_t type, rtb_child_add_loc_t location);
void rtb_patchbay_port_fini(struct rtb_patchbay_port *self);

void rtb_patchbay_node_set_name(struct rtb_patchbay_node *,
		const rtb_utf8_t *name);
void rtb_patchbay_node_init(struct rtb_patchbay_node *);
void rtb_patchbay_node_fini(struct rtb_patchbay_node *);
struct rtb_patchbay_node *rtb_patchbay_node_new(struct rtb_patchbay *parent,
		const rtb_utf8_t *name);
void rtb_patchbay_node_free(struct rtb_patchbay_node *);

int rtb_patchbay_init(struct rtb_patchbay *);
void rtb_patchbay_fini(struct rtb_patchbay *);
struct rtb_patchbay *rtb_patchbay_new(void);
void rtb_patchbay_free(struct rtb_patchbay *);
