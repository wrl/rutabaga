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

#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <wchar.h>

#include <windows.h>

#include <rutabaga/rutabaga.h>
#include "rtb_private/window_impl.h"

#include <GL/wglext.h>

#include "win_rtb.h"

#ifdef _WIN64
# define PRIuSIZE PRIu64
# define PRIxSIZE PRIx64
#else
# define PRIuSIZE PRIu32
# define PRIxSIZE PRIx32
#endif

/**
 * utils
 */

static wchar_t *
utf8_to_utf16_alloc(const rtb_utf8_t *utf8)
{
	WCHAR *buf;
	size_t need;

	need = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
	if (!need)
		return NULL;

	buf = calloc(sizeof(*buf), need);
	if (!buf)
		return NULL;

	MultiByteToWideChar(CP_UTF8, 0, utf8, -1, buf, need);
	return buf;
}

static void
messageboxf(const wchar_t *title, const wchar_t *fmt, ...)
{
	va_list args;
	wchar_t *buf;
	size_t len;

	va_start(args, fmt);

	len = _vscwprintf(fmt, args) + 1;
	if( !(buf = calloc(sizeof(*buf), len)) )
		return;

	vswprintf(buf, len, fmt, args);
	va_end(args);

	MessageBoxW(NULL, buf, title,
			MB_ICONERROR | MB_OK | MB_TOPMOST);

	free(buf);
}

/**
 * wndproc
 */

static LRESULT CALLBACK
win_rtb_wndproc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	struct win_rtb_window *self = (void *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
	LRESULT ret;

	switch (message) {
	case WM_CREATE:
		PostMessage(hwnd, WM_SHOWWINDOW, 0, 0);
		return 0;

	default:
		if (self) {
			ret = win_rtb_handle_message(self, message, wparam, lparam);

			if (self->need_reconfigure)
				rtb_window_reinit(RTB_WINDOW(self));

			return ret;
		}

		return DefWindowProcW(hwnd, message, wparam, lparam);
	}
}

/**
 * window class bullshit
 */

static ATOM
make_window_class(struct win_rtb_window *self)
{
	wchar_t buf[64];
	WNDCLASSW wc;

	swprintf_s(buf, sizeof(buf) - 1, L"rtb_%" PRIxSIZE, (size_t) self);

	wc = (WNDCLASSW) {
		.style         = CS_OWNDC,
		.lpfnWndProc   = win_rtb_wndproc,

		.cbClsExtra    = 0,
		.cbWndExtra    = 0,
		.hInstance     = 0,

		.hIcon         = LoadIcon(NULL, IDI_APPLICATION),
		.hCursor       = LoadCursor(NULL, IDC_ARROW),
		.hbrBackground = NULL,

		.lpszMenuName  = NULL,
		.lpszClassName = buf
	};

	return RegisterClassW(&wc);
}

static void
free_window_class(ATOM window_class)
{
	UnregisterClassW((void *) MAKEINTATOM(window_class), NULL);
}

/**
 * gl context bullshit
 */

struct win_rtb_gl_extensions {
	int create_context_attribs;
	int swap_control;
	int swap_control_tear;
	int appears_to_be_nvidia;
};

static int
check_extensions(struct win_rtb_window *self,
		struct win_rtb_gl_extensions *ext)
{
	PFNWGLGETEXTENSIONSSTRINGEXTPROC get_extensions_string;
	const char *extensions;

	*ext = (struct win_rtb_gl_extensions) {
		.appears_to_be_nvidia   = 0,
		.create_context_attribs = 0,
		.swap_control           = 0,
		.swap_control_tear      = 0
	};

	get_extensions_string = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)
		wglGetProcAddress("wglGetExtensionsStringEXT");

	if (!get_extensions_string)
		return -1;

	extensions = get_extensions_string();

	if (strstr(extensions, "WGL_ARB_create_context"))
		ext->create_context_attribs = 1;

	if (strstr(extensions, "WGL_EXT_swap_control")) {
		ext->swap_control = 1;

		if (strstr(extensions, "WGL_EXT_swap_control_tear"))
			ext->swap_control_tear = 1;
	}

	if (strstr(extensions, "NV"))
		ext->appears_to_be_nvidia = 1;

	return 0;
}

static int
init_gl_ctx(struct win_rtb_window *self, const wchar_t *title)
{
	PFNWGLCREATECONTEXTATTRIBSARBPROC create_context_attribs;
	PFNWGLSWAPINTERVALEXTPROC swap_interval;
	struct win_rtb_gl_extensions ext;
	int maj, min;

	PIXELFORMATDESCRIPTOR pd = {0};
	HGLRC trampoline_ctx, gl_ctx;

	const int ctx_attribs[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
		WGL_CONTEXT_MINOR_VERSION_ARB, 2,
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		0
	};

	pd = (PIXELFORMATDESCRIPTOR) {
		sizeof(pd),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		32,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		24,
		8,
		0,
		PFD_MAIN_PLANE,
		0, 0, 0, 0
	};

	SetPixelFormat(self->dc, ChoosePixelFormat(self->dc, &pd), &pd);

	trampoline_ctx = wglCreateContext(self->dc);
	if (!trampoline_ctx) {
		messageboxf(title, L"couldn't create openGL context");
		goto err_create_ctx;
	}

	wglMakeCurrent(self->dc, trampoline_ctx);
	check_extensions(self, &ext);

	/* we want at least an openGL 3.2 context. if we've already got one,
	 * bail out here. otherwise, we'll need to use
	 * wglCreateContextAttribsARB. */

	if (ogl_LoadFunctions() == ogl_LOAD_FAILED) {
		messageboxf(title, L"openGL context lacks support for 3.0 functions");
		goto err_ctx_version;
	}

	glGetError();

	glGetIntegerv(GL_MAJOR_VERSION, &maj);
	glGetIntegerv(GL_MINOR_VERSION, &min);

	if (glGetError() == GL_INVALID_ENUM) {
		messageboxf(title,
				L"need an openGL version of 3.2 or higher, "
				L"context only supports %s", glGetString(GL_VERSION));

		goto err_ctx_version;
	}

	if (!ext.create_context_attribs) {
		messageboxf(title,
				L"wglCreateContextAttribsARB is unsupported – "
				L"can't create the necessary openGL context.",
				min, maj);

		goto err_ctx_version;
	}

	create_context_attribs = (PFNWGLCREATECONTEXTATTRIBSARBPROC)
		wglGetProcAddress("wglCreateContextAttribsARB");

	if (!create_context_attribs)
		goto err_create_ctx;

	gl_ctx = create_context_attribs(self->dc, NULL, ctx_attribs);

	if (!gl_ctx)
		goto err_ctx_version;

	wglMakeCurrent(self->dc, NULL);
	wglDeleteContext(trampoline_ctx);

	self->gl_ctx = gl_ctx;
	wglMakeCurrent(self->dc, gl_ctx);

	if (ext.swap_control) {
		swap_interval = (PFNWGLSWAPINTERVALEXTPROC)
			wglGetProcAddress("wglSwapIntervalEXT");

		/* vsync + wgl + nvidia == stupid high CPU usage */
		if (ext.appears_to_be_nvidia)
			swap_interval(0);
		else
			swap_interval(1);
	}

	return 0;

err_ctx_version:
	wglMakeCurrent(self->dc, NULL);
	wglDeleteContext(trampoline_ctx);
err_create_ctx:
	return -1;
}

/**
 * window lifecycle
 */

struct rtb_window *
window_impl_open(struct rutabaga *rtb,
		const struct rtb_window_open_options *opt)
{
	struct win_rtb_window *self = calloc(1, sizeof(*self));
	wchar_t *wtitle;
	RECT wrect;
	int flags;

	wtitle = utf8_to_utf16_alloc(opt->title);
	if (!wtitle) {
		messageboxf(wtitle, L"couldn't allocate memory");
		goto err_wtitle;
	}

	self->window_class = make_window_class(self);
	if (!self->window_class) {
		messageboxf(wtitle, L"couldn't register window class");
		goto err_window_class;
	}

	flags =
		WS_POPUPWINDOW | WS_CAPTION | WS_VISIBLE
		| WS_SIZEBOX | WS_MINIMIZEBOX | WS_MAXIMIZEBOX
		| WS_CLIPSIBLINGS;

	wrect = (RECT) {0, 0, opt->width, opt->height};

	if (opt->parent || opt->flags & RTB_WINDOW_OPEN_AS_IF_PARENTED)
		flags = WS_CHILD | WS_VISIBLE;
	else
		AdjustWindowRectEx(&wrect, flags, FALSE, 0);

	self->hwnd = CreateWindowExW((DWORD) 0,
			(void *) MAKEINTATOM(self->window_class), wtitle, flags,
			0, 0,
			wrect.right - wrect.left, wrect.bottom - wrect.top,
			(HWND) opt->parent,
			NULL, NULL, NULL);

	if (!self->hwnd) {
		messageboxf(wtitle, L"couldn't create window");
		goto err_createwindow;
	}

	self->dc = GetDC(self->hwnd);

	if (init_gl_ctx(self, wtitle))
		goto err_gl_ctx;

	SetWindowLongPtr(self->hwnd, GWLP_USERDATA, (LONG_PTR) self);

	/* XXX: hardcode this for now */
	self->dpi.x = 96;
	self->dpi.y = 96;

	free(wtitle);
	uv_mutex_init(&self->lock);
	return RTB_WINDOW(self);

err_gl_ctx:
	DestroyWindow(self->hwnd);
err_createwindow:
	free_window_class(self->window_class);
err_window_class:
	free(wtitle);
err_wtitle:
	free(self);
	return NULL;
}

void
window_impl_close(struct rtb_window *rwin)
{
	struct win_rtb_window *self = RTB_WINDOW_AS(rwin, win_rtb_window);

	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(self->gl_ctx);
	ReleaseDC(self->hwnd, self->dc);

	DestroyWindow(self->hwnd);
	free_window_class(self->window_class);

	uv_mutex_unlock(&self->lock);
	uv_mutex_destroy(&self->lock);

	free(self);
}

/**
 * locking
 */

void
rtb_window_lock(struct rtb_window *rwin)
{
	struct win_rtb_window *self = RTB_WINDOW_AS(rwin, win_rtb_window);

	uv_mutex_lock(&self->lock);
	wglMakeCurrent(self->dc, self->gl_ctx);
}

void
rtb_window_unlock(struct rtb_window *rwin)
{
	struct win_rtb_window *self = RTB_WINDOW_AS(rwin, win_rtb_window);

	wglMakeCurrent(self->dc, NULL);
	uv_mutex_unlock(&self->lock);
}

/**
 * rutabaga lifecycle
 */

struct rutabaga *
window_impl_rtb_alloc(void)
{
	struct win_rtb *wrtb = calloc(1, sizeof(*wrtb));
	HMODULE ole;

	ole = LoadLibrary("ole32.dll");
	wrtb->ole32 = ole;
	wrtb->copy_cursor = LoadCursor(ole, MAKEINTRESOURCE(3));

	return (void *) wrtb;
}

void
window_impl_rtb_free(struct rutabaga *rtb)
{
	struct win_rtb *wrtb = (void *) rtb;

	FreeLibrary(wrtb->ole32);

	free(rtb);
}

/**
 * misc
 */

struct rtb_point
rtb_get_scaling(intptr_t parent_window)
{
	// FIXME: DPI or something else? ¯\_(ツ)_/¯
	return RTB_MAKE_POINT(1.f, 1.f);
}


intptr_t
rtb_window_get_native_handle(struct rtb_window *rwin)
{
	struct win_rtb_window *self = RTB_WINDOW_AS(rwin, win_rtb_window);
	return (intptr_t) self->hwnd;
}
