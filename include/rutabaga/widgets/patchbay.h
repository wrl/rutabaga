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
#include "rutabaga/object.h"
#include "rutabaga/surface.h"
#include "rutabaga/event.h"

#include "rutabaga/widgets/label.h"

#define RTB_PATCHBAY(x) RTB_UPCAST(x, rtb_patchbay)
#define RTB_PATCHBAY_NODE(x) RTB_UPCAST(x, rtb_patchbay_node)
#define RTB_PATCHBAY_PORT(x) RTB_UPCAST(x, rtb_patchbay_port)

/**
 * structures
 */

typedef struct rtb_patchbay rtb_patchbay_t;
typedef struct rtb_patchbay_node rtb_patchbay_node_t;
typedef struct rtb_patchbay_port rtb_patchbay_port_t;
typedef struct rtb_patchbay_patch rtb_patchbay_patch_t;

typedef enum {
	PORT_TYPE_INPUT,
	PORT_TYPE_OUTPUT
} rtb_patchbay_port_type_t;

struct rtb_patchbay_node {
	RTB_INHERIT(rtb_object);

	rtb_obj_t node_ui;

	/* private ********************************/
	rtb_obj_t container;
	rtb_obj_t input_ports;
	rtb_obj_t output_ports;

	GLuint vbo;
	rtb_label_t name_label;
	rtb_patchbay_t *patchbay;
};

struct rtb_patchbay_port {
	RTB_INHERIT(rtb_object);

	/* private ********************************/
	GLuint vbo;
	rtb_label_t label;
	rtb_patchbay_port_type_t port_type;

	rtb_patchbay_node_t *node;
	TAILQ_HEAD(port_patches, rtb_patchbay_patch) patches;
};

struct rtb_patchbay_patch {
	rtb_patchbay_port_t *from;
	rtb_patchbay_port_t *to;

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
	rtb_pt_t texture_offset;

	TAILQ_HEAD(patchbay_patches, rtb_patchbay_patch) patches;
	struct {
		rtb_patchbay_port_t *from;
		rtb_patchbay_port_t *to;

		rtb_pt_t cursor;
	} patch_in_progress;
};

/**
 * events
 */

typedef struct rtb_event_patchbay_connect rtb_ev_patchbay_connect_t;
typedef struct rtb_event_patchbay_disconnect rtb_ev_patchbay_disconnect_t;

typedef enum {
	RTB_PATCHBAY_CONNECT,
	RTB_PATCHBAY_DISCONNECT
} rtb_ev_patchbay_type_t;

struct rtb_event_patchbay_connect {
	RTB_INHERIT(rtb_event);

	struct {
		rtb_patchbay_node_t *node;
		rtb_patchbay_port_t *port;
	} from, to;
};

struct rtb_event_patchbay_disconnect {
	RTB_INHERIT(rtb_event_patchbay_connect);

	rtb_patchbay_patch_t *patch;
};

/**
 * public API
 */

int rtb_patchbay_are_ports_connected(rtb_patchbay_port_t *a,
		rtb_patchbay_port_t *b);
void rtb_patchbay_free_patch(rtb_patchbay_t *self,
		rtb_patchbay_patch_t *patch);
void rtb_patchbay_disconnect_ports(rtb_patchbay_t *self,
		rtb_patchbay_port_t *a, rtb_patchbay_port_t *b);
rtb_patchbay_patch_t *rtb_patchbay_connect_ports(rtb_patchbay_t *,
		rtb_patchbay_port_t *a, rtb_patchbay_port_t *b);

int rtb_patchbay_port_init(rtb_patchbay_port_t *port,
		rtb_patchbay_node_t *node, const rtb_utf8_t *name,
		rtb_patchbay_port_type_t type, rtb_child_add_loc_t location);
void rtb_patchbay_port_fini(rtb_patchbay_port_t *self);

void rtb_patchbay_node_set_name(rtb_patchbay_node_t *,
		const rtb_utf8_t *name);
void rtb_patchbay_node_init(rtb_patchbay_node_t *);
void rtb_patchbay_node_fini(rtb_patchbay_node_t *);
rtb_patchbay_node_t *rtb_patchbay_node_new(rtb_patchbay_t *parent,
		const rtb_utf8_t *name);
void rtb_patchbay_node_free(rtb_patchbay_node_t *);

rtb_patchbay_t *rtb_patchbay_new(void);
void rtb_patchbay_free(rtb_patchbay_t *);
