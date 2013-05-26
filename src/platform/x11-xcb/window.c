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

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>

#include <xcb/xcb.h>
#include <xcb/xkb.h>
#include <xcb/xcb_icccm.h>

#define GLX_GLXEXT_PROTOTYPES "gimme dem"
#include <glloadgen/gl_core.3.0.h>
#include <GL/glx.h>
#include <GL/glxext.h>

#include "rutabaga/rutabaga.h"
#include "rutabaga/window.h"

#include "xrtb.h"

#define MIN_COLOR_CHANNEL_BITS 8

static xcb_atom_t intern_atom(xcb_connection_t *c, const char *atom_name)
{
	xcb_intern_atom_cookie_t req;
	xcb_intern_atom_reply_t *rep;
	xcb_atom_t atom;

	if (!atom_name)
		return XCB_NONE;

	req = xcb_intern_atom(c, 0, strlen(atom_name), atom_name);
	rep = xcb_intern_atom_reply(c, req, NULL);

	if (!rep)
		return XCB_NONE;

	atom = rep->atom;
	free(rep);

	return atom;
}

static int receive_xkb_events(xcb_connection_t *c)
{
	xcb_xkb_use_extension_cookie_t xkb_cookie;
	xcb_xkb_use_extension_reply_t *xkb_reply;
	xcb_generic_error_t *error;
	xcb_void_cookie_t cookie;
	unsigned int events, map;

	xkb_cookie = xcb_xkb_use_extension(c,
			XCB_XKB_MAJOR_VERSION, XCB_XKB_MINOR_VERSION);
	xkb_reply = xcb_xkb_use_extension_reply(c, xkb_cookie, NULL);

	if (!xkb_reply) {
		ERR("couldn't initialize XKB\n");
		return -1;
	} else if (!xkb_reply->supported) {
		ERR("couldn't initialize XKB: unsupported version"
				"(server is %d.%d, we're %d.%d)\n",
				xkb_reply->serverMajor, xkb_reply->serverMinor,
				XCB_XKB_MAJOR_VERSION, XCB_XKB_MINOR_VERSION);
		free(xkb_reply);
		return -1;
	}

	free(xkb_reply);

	map = XCB_XKB_MAP_PART_KEY_SYMS | XCB_XKB_MAP_PART_MODIFIER_MAP;
	events = XCB_XKB_EVENT_TYPE_NEW_KEYBOARD_NOTIFY
		| XCB_XKB_EVENT_TYPE_STATE_NOTIFY;

	cookie = xcb_xkb_select_events_checked(c,
			XCB_XKB_ID_USE_CORE_KBD, events, 0, events, map, map, 0);

	error = xcb_request_check(c, cookie);
	if (error) {
		ERR("error requesting XKB events\n");
		free(error);
		return -1;
	}

	return 0;
}

static int get_xkb_event_id(xcb_connection_t *c)
{
	const xcb_query_extension_reply_t *extdata;

	if (!(extdata = xcb_get_extension_data(c, &xcb_xkb_id)))
		return -1;

	return extdata->first_event;
}

static int get_core_kbd_id(xcb_connection_t *c)
{
	xcb_xkb_get_device_info_cookie_t cookie;
	xcb_xkb_get_device_info_reply_t *reply;
	int device_id;

	cookie = xcb_xkb_get_device_info(c, XCB_XKB_ID_USE_CORE_KBD,
			0, 0, 0, 0, 0, 0);
	reply = xcb_xkb_get_device_info_reply(c, cookie, NULL);
	if (!reply) {
		ERR("couldn't get core XKB ID\n");
		return -1;
	}

	device_id = reply->deviceID;
	free(reply);

	return device_id;
}

struct rutabaga *window_impl_rtb_alloc()
{
	struct xcb_rutabaga *self;
	Display *dpy;

	if (!(self = calloc(1, sizeof(*self))))
		goto err_malloc;

	if (!(self->dpy = dpy = XOpenDisplay(NULL))) {
		ERR("can't open X display\n");
		goto err_dpy;
	}

	if (!(self->xcb_conn = XGetXCBConnection(dpy))) {
		ERR("can't get XCB connection for display\n");
		goto err_get_conn;
	}

	if (xrtb_keyboard_init(self))
		goto err_keyboard_init;

	XSetEventQueueOwner(dpy, XCBOwnsEventQueue);

	self->xkb_event = get_xkb_event_id(self->xcb_conn);
	self->xkb_core_kbd_id = get_core_kbd_id(self->xcb_conn);

	if (self->xkb_event >= 0 && self->xkb_core_kbd_id >= 0
			&& !receive_xkb_events(self->xcb_conn))
		self->xkb_supported = 1;

#define INTERN_ATOM(atom, name) self->atoms.atom = intern_atom(self->xcb_conn, name);
	INTERN_ATOM(wm_protocols, "WM_PROTOCOLS");
	INTERN_ATOM(wm_delete_window, "WM_DELETE_WINDOW");
#undef INTERN_ATOM

	return (struct rutabaga *) self;

err_keyboard_init:
err_get_conn:
	XCloseDisplay(self->dpy);
err_dpy:
	free(self);
err_malloc:
	return NULL;
}

void window_impl_rtb_free(struct rutabaga *rtb)
{
	struct xcb_rutabaga *self = (void *) rtb;

	xrtb_keyboard_fini(self);
	XCloseDisplay(self->dpy);
	free(self);
}

static int init_gl(struct xcb_rutabaga *xrtb)
{
	int missing;

	missing = ogl_LoadFunctions();

	if (missing == ogl_LOAD_FAILED) {
		ERR("couldn't initialize openGL.\n");
		return -1;
	} else if (missing > ogl_LOAD_SUCCEEDED)
		ERR("openGL initialized, but missing %d functions.\n",
				missing - ogl_LOAD_SUCCEEDED);

	return 0;
}

static xcb_screen_t *find_xcb_screen(xcb_connection_t *c, int default_screen)
{
	xcb_screen_iterator_t screen_iter;
	int screen_num;

	screen_iter = xcb_setup_roots_iterator(xcb_get_setup(c));
	for (screen_num = default_screen; screen_iter.rem && screen_num > 0;
			--screen_num, xcb_screen_next(&screen_iter));

	return screen_iter.data;
}

#ifndef GLX_BACK_BUFFER_AGE_EXT
#define GLX_BACK_BUFFER_AGE_EXT 0x20F4
#endif

void window_impl_swap_buffers(struct rtb_window *rwin)
{
	struct xcb_window *self = (void *) rwin;
	struct xcb_rutabaga *xrtb = self->xrtb;

	glXSwapBuffers(xrtb->dpy, self->gl_win);
	glFinish();
}

static GLXFBConfig find_reasonable_fb_config(
		Display *dpy, GLXFBConfig *cfgs, int ncfgs)
{
	struct {
		struct {
			int red;
			int green;
			int blue;
		} sizes;

		int double_buffer;
		int drawable_type;
		int caveat;
	} cfg_info;

	int found_db, found_no_win, found_slow, found_no_samples;
	int i;

	found_db =
		found_slow =
		found_no_win =
		found_no_samples = 0;

	for (i = 0; i < ncfgs; i++) {
#define GET_ATTRIB(att, dst) \
		glXGetFBConfigAttrib(dpy, cfgs[i], att, &cfg_info.dst)

		GET_ATTRIB(GLX_RED_SIZE,      sizes.red);
		GET_ATTRIB(GLX_GREEN_SIZE,    sizes.green);
		GET_ATTRIB(GLX_BLUE_SIZE,     sizes.blue);
		GET_ATTRIB(GLX_DOUBLEBUFFER,  double_buffer);
		GET_ATTRIB(GLX_CONFIG_CAVEAT, caveat);
		GET_ATTRIB(GLX_DRAWABLE_TYPE, drawable_type);

#undef GET_ATTRIB

		if (cfg_info.sizes.red   < MIN_COLOR_CHANNEL_BITS ||
			cfg_info.sizes.green < MIN_COLOR_CHANNEL_BITS ||
			cfg_info.sizes.blue  < MIN_COLOR_CHANNEL_BITS)
			continue;

		if (cfg_info.double_buffer != False) {
			found_db = 1;
			continue;
		}

		if (!(cfg_info.drawable_type & (GLX_WINDOW_BIT | GLX_PIXMAP_BIT))) {
			found_no_win = 1;
			continue;
		}

		if (cfg_info.caveat == GLX_SLOW_CONFIG) {
			found_slow = 1;
			continue;
		}

		return cfgs[i];
	}

	if (found_db)
		ERR("found good config, but it's double buffered.\n");

	if (found_no_win)
		ERR("found good config, but can't draw to a window.\n");

	if (found_slow)
		ERR("found good config, but it's slow.\n");

	return NULL; /* lol */
}

static int set_xprop(xcb_connection_t *c, xcb_window_t win,
		xcb_atom_t prop, const char *value)
{
	xcb_void_cookie_t cookie;
	xcb_generic_error_t *err;
	size_t len;

	len = strnlen(value, 128);

	cookie = xcb_change_property_checked(c, XCB_PROP_MODE_REPLACE, win, prop,
			XCB_ATOM_STRING, 8, len, value);

	if ((err = xcb_request_check(c, cookie))) {
		free(err);
		return 1;
	}

	return 0;
}

void rtb_window_lock(struct rtb_window *rwin)
{
	struct xcb_window *self = (struct xcb_window *) rwin;

	XLockDisplay(self->xrtb->dpy);
	pthread_mutex_lock(&self->lock);
	glXMakeContextCurrent(
				self->xrtb->dpy, self->gl_draw, self->gl_draw, self->gl_ctx);
}

void rtb_window_unlock(struct rtb_window *rwin)
{
	struct xcb_window *self = (struct xcb_window *) rwin;

	glXMakeContextCurrent(self->xrtb->dpy, None, None, NULL);
	pthread_mutex_unlock(&self->lock);
	XUnlockDisplay(self->xrtb->dpy);
}

struct rtb_window *window_impl_open(struct rutabaga *rtb,
		int w, int h, const char *title)
{
	struct xcb_rutabaga *xrtb = (void *) rtb;
	struct xcb_window *self;

	Display *dpy;
	xcb_connection_t *xcb_conn;

	int default_screen;

	GLXFBConfig *fb_configs, fb_config;
	int nfb_configs;
	int visual_id;

	uint32_t event_mask =
		XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_EXPOSURE |
		XCB_EVENT_MASK_VISIBILITY_CHANGE | XCB_EVENT_MASK_BUTTON_PRESS |
		XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION |
		XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW |
		XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
		XCB_EVENT_MASK_KEYMAP_STATE;
	uint32_t value_mask =
		XCB_CW_BORDER_PIXEL | XCB_CW_BACK_PIXEL |
		XCB_CW_BIT_GRAVITY |
		XCB_CW_EVENT_MASK | XCB_CW_COLORMAP;
	uint32_t value_list[6];

	xcb_colormap_t colormap;
	xcb_void_cookie_t ck_window, ck_map;
	xcb_generic_error_t *err;

	assert(rtb);
	assert(h > 0);
	assert(w > 0);

	if (!(self = calloc(1, sizeof(*self))))
		goto err_malloc;

	self->rtb = rtb;
	self->xrtb = xrtb;

	dpy = xrtb->dpy;
	xcb_conn = xrtb->xcb_conn;

	default_screen = DefaultScreen(dpy);

	self->screen = find_xcb_screen(xcb_conn, default_screen);
	if (!self->screen) {
		ERR("couldn't find XCB screen\n");
		goto err_screen;
	}

	/**
	 * gl configuration
	 */

	fb_configs = glXGetFBConfigs(dpy, default_screen, &nfb_configs);
	if (!fb_configs || !nfb_configs) {
		ERR("no GL configurations, bailing out\n");
		goto err_gl_config;
	}

	fb_config = find_reasonable_fb_config(dpy, fb_configs, nfb_configs);
	if (!fb_config) {
		ERR("no reasonable GL configurations, bailing out\n");
		goto err_gl_config;
	}

	glXGetFBConfigAttrib(dpy, fb_config, GLX_VISUAL_ID, &visual_id);

	self->gl_ctx = glXCreateNewContext(
			dpy, fb_config, GLX_RGBA_TYPE, 0, True);
	if (!self->gl_ctx) {
		ERR("couldn't create GLX context\n");
		goto err_gl_ctx;
	}

	/**
	 * window setup
	 */

	colormap = xcb_generate_id(xcb_conn);
	self->xcb_win = xcb_generate_id(xcb_conn);

	xcb_create_colormap(
			xcb_conn, XCB_COLORMAP_ALLOC_NONE, colormap,
			self->screen->root, visual_id);

	value_list[0] = 0;
	value_list[1] = 0;
	value_list[2] = XCB_GRAVITY_STATIC;
	value_list[3] = event_mask;
	value_list[4] = colormap;
	value_list[5] = 0;

	ck_window = xcb_create_window_checked(
			xcb_conn, XCB_COPY_FROM_PARENT, self->xcb_win, self->screen->root,
			0, 0,
			w, h,
			0,
			XCB_WINDOW_CLASS_INPUT_OUTPUT,
			visual_id, value_mask, value_list);

	if ((err = xcb_request_check(xcb_conn, ck_window))) {
		ERR("can't create XCB window: %d\n", err->error_code);
		goto err_xcb_win;
	}

	self->gl_win = glXCreateWindow(dpy, fb_config, self->xcb_win, 0);
	if (!self->gl_win) {
		ERR("couldn't create GL window\n");
		goto err_gl_win;
	}

	if (set_xprop(xcb_conn, self->xcb_win, XCB_ATOM_WM_NAME, title))
		set_xprop(xcb_conn, self->xcb_win, XCB_ATOM_WM_NAME, "oh no");

	self->gl_draw = self->gl_win;

	if (!glXMakeContextCurrent(
				dpy, self->gl_draw, self->gl_draw, self->gl_ctx)) {
		ERR("couldn't activate GLX context\n");
		goto err_gl_make_current;
	}

	ck_map = xcb_map_window_checked(xcb_conn, self->xcb_win);

	if ((err = xcb_request_check(xcb_conn, ck_map))) {
		ERR("can't map XCB window: %d\n", err->error_code);
		goto err_win_map;
	}

	init_gl(xrtb);

	xcb_icccm_set_wm_protocols(xcb_conn,
			self->xcb_win, xrtb->atoms.wm_protocols,
			1, &xrtb->atoms.wm_delete_window);

	free(fb_configs);
	return RTB_WINDOW(self);

err_win_map:
err_gl_make_current:
err_gl_win:
	xcb_destroy_window(xcb_conn, self->xcb_win);

err_xcb_win:
	glXDestroyContext(dpy, self->gl_ctx);

err_gl_ctx:
err_gl_config:
	free(fb_configs);
err_screen:
	free(self);

err_malloc:
	return NULL;
}

void window_impl_close(struct rtb_window *rwin)
{
	struct xcb_window *self = (void *) rwin;

	glXMakeContextCurrent(self->xrtb->dpy, None, None, NULL);
	glXDestroyWindow(self->xrtb->dpy, self->gl_win);
	xcb_destroy_window(self->xrtb->xcb_conn, self->xcb_win);
	glXDestroyContext(self->xrtb->dpy, self->gl_ctx);

	free(self);
}
