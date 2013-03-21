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
	RTB_DIRECTION_LEAFWARD,
	RTB_DIRECTION_ROOTWARD
} rtb_event_direction_t;

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
 * structures
 */

typedef struct rutabaga    rtb_t;
typedef struct rtb_point   rtb_pt_t;
typedef struct rtb_rect    rtb_rect_t;
typedef struct rtb_size    rtb_size_t;

typedef struct rtb_object  rtb_obj_t;
typedef struct rtb_object  rtb_container_t;
typedef struct rtb_surface rtb_surface_t;
typedef struct rtb_window  rtb_win_t;

typedef struct rtb_style       rtb_style_t;
typedef struct rtb_style_props rtb_style_props_t;

typedef struct rtb_shader_program rtb_shader_program_t;

typedef struct rtb_font_manager rtb_font_manager_t;

typedef struct rtb_event_handler rtb_ev_handler_t;

typedef struct rtb_event  rtb_ev_t;
typedef struct rtb_event_drag rtb_ev_drag_t;
typedef struct rtb_event_mouse rtb_ev_mouse_t;
typedef struct rtb_event_key rtb_ev_key_t;

typedef struct rtb_mouse rtb_mouse_t;

/**
 * callbacks
 */

typedef void (*rtb_draw_cb_t)(rtb_obj_t *obj, rtb_draw_state_t state);
typedef int  (*rtb_internal_event_cb_t)(rtb_obj_t *obj, const rtb_ev_t *event);
typedef void (*rtb_realize_cb_t)(rtb_obj_t *obj, rtb_obj_t *parent,
		rtb_win_t *window);
typedef void (*rtb_layout_cb_t)(rtb_obj_t *);
typedef void (*rtb_size_cb_t)(rtb_obj_t *obj, const rtb_size_t *avail,
		rtb_size_t *want);
typedef void (*rtb_recalc_cb_t)(rtb_obj_t *obj, rtb_obj_t *instigator,
		rtb_event_direction_t direction);
typedef void (*rtb_attach_child_cb_t)(rtb_obj_t *obj, rtb_obj_t *child);
typedef void (*rtb_mark_dirty_cb_t)(rtb_obj_t *);

typedef int  (*rtb_event_cb_t)(rtb_obj_t *obj, const rtb_ev_t *event,
		void *ctx);

/**
 * structure definitions
 */

struct rtb_point {
	GLfloat x;
	GLfloat y;
};

struct rtb_size {
	GLfloat w;
	GLfloat h;
};

struct rtb_padding {
	struct rtb_point;
};

struct rtb_rect {
	rtb_pt_t p1;
	rtb_pt_t p2;
};
