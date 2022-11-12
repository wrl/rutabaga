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

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <uv.h>

#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>
#include <X11/Xlib-xcb.h>

#include <xcb/xcb.h>
#include <xcb/xkb.h>
#include <xcb/xcb_icccm.h>

#include <rutabaga/opengl.h>

#include <rutabaga/rutabaga.h>
#include <rutabaga/window.h>
#include <rutabaga/mouse.h>

#include "rtb_private/window_impl.h"
#include "rtb_private/util.h"

#include "xrtb.h"

#define MIN_COLOR_CHANNEL_BITS 8

static xcb_atom_t
intern_atom(xcb_connection_t *c, const char *atom_name, int only_if_exists)
{
	xcb_intern_atom_cookie_t req;
	xcb_intern_atom_reply_t *rep;
	xcb_atom_t atom;

	if (!atom_name)
		return XCB_NONE;

	req = xcb_intern_atom(c, only_if_exists, strlen(atom_name), atom_name);
	rep = xcb_intern_atom_reply(c, req, NULL);

	if (!rep)
		return XCB_NONE;

	atom = rep->atom;
	free(rep);

	return atom;
}

static int
receive_xkb_events(xcb_connection_t *c)
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

static int
get_xkb_event_id(xcb_connection_t *c)
{
	const xcb_query_extension_reply_t *extdata;

	if (!(extdata = xcb_get_extension_data(c, &xcb_xkb_id)))
		return -1;

	return extdata->first_event;
}

static int
get_core_kbd_id(xcb_connection_t *c)
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

static xcb_cursor_t
create_empty_cursor(struct xcb_rutabaga *self)
{
	xcb_cursor_t cursor;
	Pixmap pixmap;
	XColor color;
	char data;

	data = 0;
	color.red = color.green = color.blue = 0;

	pixmap = XCreateBitmapFromData(self->dpy,
			DefaultRootWindow(self->dpy), &data, 1, 1);

	if (!pixmap)
		return 0;

	cursor = XCreatePixmapCursor(self->dpy, pixmap, pixmap, &color, &color, 0, 0);
	XFreePixmap(self->dpy, pixmap);

	return cursor;
}

static xcb_screen_t *
find_xcb_screen(xcb_connection_t *c, int default_screen)
{
	xcb_screen_iterator_t screen_iter;
	int screen_num;

	screen_iter = xcb_setup_roots_iterator(xcb_get_setup(c));
	for (screen_num = default_screen; screen_iter.rem && screen_num > 0;
			--screen_num, xcb_screen_next(&screen_iter));

	return screen_iter.data;
}

static int
setup_clipboard(struct xcb_rutabaga *self)
{
	xcb_void_cookie_t ck_window;
	xcb_generic_error_t *err;
	int screen_idx, conn_err;
	xcb_screen_t *screen;

	self->clipboard.conn = xcb_connect(NULL, &screen_idx);
	if ((conn_err = xcb_connection_has_error(self->clipboard.conn))) {
		ERR("error establishing clipboard XCB connection: %d\n",
				conn_err);
		goto err_connection;
	}

	screen = find_xcb_screen(self->clipboard.conn, screen_idx);

	self->clipboard.window = xcb_generate_id(self->clipboard.conn);
	ck_window = xcb_create_window_checked(
			self->clipboard.conn, 0, self->clipboard.window,
			screen->root,
			-10, -10, 1, 1, 0, XCB_WINDOW_CLASS_INPUT_ONLY,
			XCB_COPY_FROM_PARENT, 0, NULL);

	if ((err = xcb_request_check(self->xcb_conn, ck_window))) {
		ERR("can't create XCB window: %d\n", err->error_code);
		free(err);
		goto err_create_window;
	}

	self->clipboard.buffer = NULL;
	self->clipboard.nbytes = 0;

	return 0;

err_create_window:
	xcb_disconnect(self->clipboard.conn);
err_connection:
	return -1;
}

struct rutabaga *
window_impl_rtb_alloc(void)
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

	if (xrtb_keyboard_init(self)) {
		ERR("can't initialize keyboard\n");
		goto err_keyboard_init;
	}

	if (setup_clipboard(self))
		goto err_clipboard_window;

	XSetEventQueueOwner(dpy, XCBOwnsEventQueue);

	self->xkb_event = get_xkb_event_id(self->xcb_conn);
	self->xkb_core_kbd_id = get_core_kbd_id(self->xcb_conn);

	if (self->xkb_event >= 0 && self->xkb_core_kbd_id >= 0
			&& !receive_xkb_events(self->xcb_conn))
		self->xkb_supported = 1;

	self->running_in_xwayland =
		intern_atom(self->xcb_conn, "WL_SURFACE_ID", 1) != XCB_NONE;

#define INTERN_ATOM(atom, name)												\
	self->atoms.atom = intern_atom(self->xcb_conn, name, 0);

	INTERN_ATOM(wm_protocols, "WM_PROTOCOLS");
	INTERN_ATOM(wm_delete_window, "WM_DELETE_WINDOW");

	INTERN_ATOM(targets,     "TARGETS");
	INTERN_ATOM(timestamp,   "TIMESTAMP");
	INTERN_ATOM(utf8_string, "UTF8_STRING");
	INTERN_ATOM(clipboard,   "CLIPBOARD");

	INTERN_ATOM(compositor, "_NET_WM_CM_S0");

#undef INTERN_ATOM

	self->empty_cursor = create_empty_cursor(self);
	self->xfixes = xcb_get_extension_data(self->xcb_conn, &xcb_xfixes_id);

	return (struct rutabaga *) self;

err_clipboard_window:
	xrtb_keyboard_fini(self);
err_keyboard_init:
err_get_conn:
	XCloseDisplay(self->dpy);
err_dpy:
	free(self);
err_malloc:
	return NULL;
}

void
window_impl_rtb_free(struct rutabaga *rtb)
{
	struct xcb_rutabaga *self = (void *) rtb;

	xcb_destroy_window(self->clipboard.conn, self->clipboard.window);
	xcb_disconnect(self->clipboard.conn);

	xrtb_keyboard_fini(self);
	XFreeCursor(self->dpy, self->empty_cursor);

	XCloseDisplay(self->dpy);
	free(self->clipboard.buffer);
	free(self);
}

/**
 * egl initialisation
 */

static XVisualInfo *
visual_info_from_egl_config(Display *dpy, EGLDisplay egl_dpy, EGLConfig cfg)
{
	XVisualInfo vinfo = {0};
	EGLint vid;
	int count;

	eglGetConfigAttrib(egl_dpy, cfg, EGL_NATIVE_VISUAL_ID, &vid);
	vinfo.visualid = vid;

	return XGetVisualInfo(dpy, VisualIDMask, &vinfo, &count);
}

static int
egl_config_supports_alpha(Display *dpy, EGLDisplay egl_dpy, EGLConfig cfg)
{
	XRenderPictFormat *pict_format;
	XVisualInfo *vinfo;

	int well_does_it = 1;

	vinfo = visual_info_from_egl_config(dpy, egl_dpy, cfg);
	pict_format = XRenderFindVisualFormat(dpy, vinfo->visual);

	if (!pict_format || !pict_format->direct.alphaMask)
		well_does_it = 0;

	free(vinfo);
	return well_does_it;
}

static EGLConfig
iter_egl_configs(Display *dpy, EGLDisplay egl_dpy,
		EGLConfig *configs, int nconfigs, int want_transparent)
{
	EGLConfig not_transparent_fallback;
	int found_no_win, found_not_transparent;
	int i;

	found_not_transparent =
		found_no_win = 0;

	for (i = 0; i < nconfigs; i++) {
		EGLConfig cfg = configs[i];
		EGLint vid, tr;

		eglGetConfigAttrib(egl_dpy, cfg, EGL_NATIVE_VISUAL_ID, &vid);
		if (!vid)  {
			found_no_win = 1;
			continue;
		}

		eglGetConfigAttrib(egl_dpy, cfg, EGL_TRANSPARENT_TYPE, &tr);

		if (want_transparent && !egl_config_supports_alpha(dpy, egl_dpy, cfg)) {
			found_not_transparent = 1;
			not_transparent_fallback = cfg;
			continue;
		}

		return cfg;
	}

	if (found_no_win)
		ERR("found good EGL config, but can't draw to a window.\n");

	if (found_not_transparent) {
		ERR("found good EGL config, but it isn't transparent as was requested\n");
		return not_transparent_fallback;
	}

	return NULL;
}

static EGLConfig
find_egl_config(Display *dpy, EGLDisplay egl_dpy, int want_transparent)
{
	static const EGLint attribs[] = {
		EGL_RED_SIZE,   MIN_COLOR_CHANNEL_BITS,
		EGL_GREEN_SIZE, MIN_COLOR_CHANNEL_BITS,
		EGL_BLUE_SIZE,  MIN_COLOR_CHANNEL_BITS,

		EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,

		EGL_CONFIG_CAVEAT, EGL_NONE,

		EGL_NONE
	};

	EGLConfig *configs, found;
	int nconfigs;

	eglChooseConfig(egl_dpy, attribs, NULL, 0, &nconfigs);
	if (!nconfigs) {
		return NULL;
	}

	configs = calloc(nconfigs, sizeof(*configs));

	eglChooseConfig(egl_dpy, attribs, configs, nconfigs, &nconfigs);
	found = iter_egl_configs(dpy, egl_dpy, configs,nconfigs, want_transparent);
	free(configs);

	return found;
}

static EGLContext
new_egl_ctx(EGLDisplay egl_dpy, EGLContext cfg)
{
	static const EGLint attribs[] = {
		EGL_CONTEXT_MAJOR_VERSION_KHR, 3,
		EGL_CONTEXT_MINOR_VERSION_KHR, 2,

		EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR,
			EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR,

		EGL_NONE
	};

	return eglCreateContext(egl_dpy, cfg, EGL_NO_CONTEXT, attribs);
}

static int
set_xprop(xcb_connection_t *c, xcb_window_t win,
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

static void
set_swap_interval(xcb_connection_t *xcb_conn,
		xcb_atom_t compositor, EGLDisplay egl_dpy)
{
	int swap_blocks_until_vsync = 1;

	if (compositor > 0) {
		xcb_get_selection_owner_cookie_t req;
		xcb_get_selection_owner_reply_t *rep;

		req = xcb_get_selection_owner(xcb_conn, compositor);
		rep = xcb_get_selection_owner_reply(xcb_conn, req, NULL);

		if (rep) {
			swap_blocks_until_vsync = !rep->owner;
			free(rep);
		}
	}

	eglSwapInterval(egl_dpy, swap_blocks_until_vsync);
}

/**
 * from http://stackoverflow.com/questions/2621439
 * thanx <3
 */

static void
get_dpi(xcb_screen_t *screen, int *x, int *y)
{
	double xres, yres;

	/*
	 * there are 2.54 centimeters to an inch; so there are 25.4 millimeters.
	 *
	 *     dpi = N pixels / (M millimeters / (25.4 millimeters / 1 inch))
	 *         = N pixels / (M inch / 25.4)
	 *         = N * 25.4 pixels / M inch
	 */
	xres = (screen->width_in_pixels * 25.4) /
			((double) screen->width_in_millimeters);
	yres = (screen->height_in_pixels * 25.4) /
			((double) screen->height_in_millimeters);

	*x = (int) (xres + 0.5);
	*y = (int) (yres + 0.5);
}

static struct rtb_point
get_scaling(int dpi_x, int dpi_y)
{
	const char *env_factor;
	char *end;

	const char *env_variables_to_try[] = {
		"RTB_SCALE",
		"GDK_DPI_SCALE",
		"GDK_SCALE",
		"QT_SCALE_FACTOR"
	};

	for (unsigned i = 0; i < ARRAY_LENGTH(env_variables_to_try); i++) {
		const char *var = env_variables_to_try[i];

		if ((env_factor = getenv(var))) {
			float factor;

			factor = strtod(env_factor, &end);
			if (*end != '\0' || factor <= 0.f)
				continue;

			return RTB_MAKE_POINT(factor, factor);
		}
	}

	if (dpi_x != 96 || dpi_y != 96) {
		struct rtb_point factor;

		factor.x = fmax(1.f, roundf(dpi_x / 96.f));
		factor.y = fmax(1.f, roundf(dpi_y / 96.f));

		return factor;
	}

	return RTB_MAKE_POINT(1.f, 1.f);
}

struct rtb_point
rtb_get_scaling(intptr_t parent_window)
{
	int screen_nr, dpi_x, dpi_y;
	struct rtb_point scale;
	xcb_connection_t *conn;
	xcb_screen_t *screen;

	conn = xcb_connect(NULL, &screen_nr);
	if (!conn)
		goto err_connect;

	screen = find_xcb_screen(conn, screen_nr);
	get_dpi(screen, &dpi_x, &dpi_y);

	scale = get_scaling(dpi_x, dpi_y);

	xcb_disconnect(conn);
	return scale;

err_connect:
	return RTB_MAKE_POINT(1.f, 1.f);
}

static void
raise_window(xcb_connection_t *xcb_conn, xcb_window_t window)
{
	const uint32_t values[] = {
		XCB_STACK_MODE_ABOVE
	};

	xcb_configure_window(xcb_conn, window,
			XCB_CONFIG_WINDOW_STACK_MODE, values);
}

struct rtb_window *
window_impl_open(struct rutabaga *rtb,
		const struct rtb_window_open_options *opt)
{
	struct xcb_rutabaga *xrtb = (void *) rtb;
	struct xrtb_window *self;

	Display *dpy;
	xcb_connection_t *xcb_conn;

	int default_screen;
	EGLConfig egl_config;
	XVisualInfo *visual;

	uint32_t event_mask =
		XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_EXPOSURE
		| XCB_EVENT_MASK_VISIBILITY_CHANGE | XCB_EVENT_MASK_BUTTON_PRESS
		| XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION
		| XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW
		| XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE
		| XCB_EVENT_MASK_KEYMAP_STATE;
	uint32_t value_mask =
		XCB_CW_BORDER_PIXEL | XCB_CW_BACK_PIXMAP |
		XCB_CW_BIT_GRAVITY |
		XCB_CW_EVENT_MASK | XCB_CW_COLORMAP;
	uint32_t value_list[6];

	xcb_colormap_t colormap;
	xcb_void_cookie_t ck_window, ck_map;
	xcb_generic_error_t *err;

	assert(rtb);

	if (!(self = calloc(1, sizeof(*self))))
		goto err_malloc;

	self->xrtb = xrtb;

	dpy = xrtb->dpy;
	xcb_conn = xrtb->xcb_conn;

	default_screen = DefaultScreen(dpy);

	self->screen = find_xcb_screen(xcb_conn, default_screen);
	if (!self->screen) {
		ERR("couldn't find XCB screen\n");
		goto err_screen;
	}

	if (xcb_cursor_context_new(xrtb->xcb_conn, self->screen,
				&self->cursor_ctx)) {
		ERR("couldn't initialise xcb_cursor context\n");
		goto err_cursor_ctx;
	}

	self->cursor.dfault = xcb_cursor_load_cursor(self->cursor_ctx, "arrow");
	self->cursor.copy = 0;

	/**
	 * egl configuration
	 */

	self->egl_dpy = eglGetDisplay(dpy);
	if (!self->egl_dpy) {
		ERR("couldn't open EGL display connection");
		goto err_egl_dpy;
	}

	if (eglInitialize(self->egl_dpy, NULL, NULL) != EGL_TRUE) {
		ERR("eglInitialize failed: %d\n", eglGetError());
		goto err_egl_init;
	}

	egl_config = find_egl_config(dpy, self->egl_dpy, 0);
	if (!egl_config) {
		ERR("couldn't find a reasonable EGL config\n");
		goto err_egl_config;
	}

	eglBindAPI(EGL_OPENGL_API);

	self->egl_ctx = new_egl_ctx(self->egl_dpy, egl_config);
	if (!self->egl_ctx) {
		ERR("couldn't create EGL context: %d\n", eglGetError());
		goto err_egl_ctx;
	}

	visual = visual_info_from_egl_config(dpy, self->egl_dpy, egl_config);

	/**
	 * window setup
	 */

	colormap = xcb_generate_id(xcb_conn);
	self->xcb_win = xcb_generate_id(xcb_conn);

	xcb_create_colormap(
			xcb_conn, XCB_COLORMAP_ALLOC_NONE, colormap,
			self->screen->root, visual->visualid);

	value_list[0] = 0;
	value_list[1] = 0;
	value_list[2] = XCB_GRAVITY_STATIC;
	value_list[3] = event_mask;
	value_list[4] = colormap;
	value_list[5] = 0;

	get_dpi(self->screen, &self->dpi.x, &self->dpi.y);
	self->scale = get_scaling(self->dpi.x, self->dpi.y);
	self->scale_recip.x = 1.f / self->scale.x;
	self->scale_recip.y = 1.f / self->scale.y;

	self->phy_size.w = self->scale.x * opt->width;
	self->phy_size.h = self->scale.y * opt->height;

	self->dpi.x = 96 * self->scale.x;
	self->dpi.y = 96 * self->scale.y;

	ck_window = xcb_create_window_checked(
			xcb_conn, visual->depth, self->xcb_win,
			opt->parent ? (xcb_window_t) opt->parent : self->screen->root,
			0, 0,
			self->phy_size.w, self->phy_size.h,
			0,
			XCB_WINDOW_CLASS_INPUT_OUTPUT,
			visual->visualid, value_mask, value_list);

	free(visual);

	if ((err = xcb_request_check(xcb_conn, ck_window))) {
		ERR("can't create XCB window: %d\n", err->error_code);
		goto err_xcb_win;
	}

	self->egl_surface = eglCreateWindowSurface(self->egl_dpy, egl_config,
			self->xcb_win, NULL);
	if (!self->egl_surface) {
		ERR("couldn't create EGL window surface: %d\n", eglGetError());
		goto err_egl_surface;
	}

	if (set_xprop(xcb_conn, self->xcb_win, XCB_ATOM_WM_NAME, opt->title))
		set_xprop(xcb_conn, self->xcb_win, XCB_ATOM_WM_NAME, "");

	if (eglMakeCurrent(self->egl_dpy, self->egl_surface, self->egl_surface,
				self->egl_ctx) != EGL_TRUE) {
		ERR("couldn't activate EGL surface: %d\n", eglGetError());
		goto err_egl_make_current;
	}

	set_swap_interval(xcb_conn, xrtb->atoms.compositor,
			self->egl_dpy);

	ck_map = xcb_map_window_checked(xcb_conn, self->xcb_win);
	if ((err = xcb_request_check(xcb_conn, ck_map))) {
		ERR("can't map XCB window: %d\n", err->error_code);
		goto err_win_map;
	}

	if (opt->parent)
		raise_window(xcb_conn, self->xcb_win);
	else
		xcb_icccm_set_wm_protocols(xcb_conn,
				self->xcb_win, xrtb->atoms.wm_protocols,
				1, &xrtb->atoms.wm_delete_window);


	if (xrtb->atoms.compositor && xrtb->xfixes->present) {
		xcb_xfixes_select_selection_input(xcb_conn, self->xcb_win,
			xrtb->atoms.compositor,
			XCB_XFIXES_SELECTION_EVENT_MASK_SET_SELECTION_OWNER
			| XCB_XFIXES_SELECTION_EVENT_MASK_SELECTION_WINDOW_DESTROY
			| XCB_XFIXES_SELECTION_EVENT_MASK_SELECTION_CLIENT_CLOSE);
	}

	uv_mutex_init(&self->lock);
	return RTB_WINDOW(self);

err_win_map:
err_egl_make_current:
err_egl_surface:
	xcb_destroy_window(xcb_conn, self->xcb_win);

err_xcb_win:
	eglDestroyContext(self->egl_dpy, self->egl_surface);

err_egl_ctx:
err_egl_config:
err_egl_init:
	eglTerminate(self->egl_dpy);
err_egl_dpy:
err_cursor_ctx:
err_screen:
	free(self);

err_malloc:
	return NULL;
}

void
window_impl_close(struct rtb_window *rwin)
{
	struct xrtb_window *self = RTB_WINDOW_AS(rwin, xrtb_window);

	eglBindAPI(EGL_OPENGL_API);

	eglMakeCurrent(self->egl_dpy,
		EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroyContext(self->egl_dpy, self->egl_ctx);
	eglTerminate(self->egl_dpy);

	xcb_destroy_window(self->xrtb->xcb_conn, self->xcb_win);

	uv_mutex_unlock(&self->lock);
	uv_mutex_destroy(&self->lock);

	xcb_cursor_context_free(self->cursor_ctx);

	free(self);
}

void
rtb_window_lock(struct rtb_window *rwin)
{
	struct xrtb_window *self = RTB_WINDOW_AS(rwin, xrtb_window);

	uv_mutex_lock(&self->lock);
	XLockDisplay(self->xrtb->dpy);

	eglBindAPI(EGL_OPENGL_API);
	eglMakeCurrent(self->egl_dpy,
		self->egl_surface, self->egl_surface, self->egl_ctx);
}

void
rtb_window_unlock(struct rtb_window *rwin)
{
	struct xrtb_window *self = RTB_WINDOW_AS(rwin, xrtb_window);

	eglMakeCurrent(self->egl_dpy,
		EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

	XUnlockDisplay(self->xrtb->dpy);
	uv_mutex_unlock(&self->lock);
}

intptr_t
rtb_window_get_native_handle(struct rtb_window *rwin)
{
	struct xrtb_window *self = RTB_WINDOW_AS(rwin, xrtb_window);
	return self->xcb_win;
}
