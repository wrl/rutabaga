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

#include <stdlib.h>
#include <inttypes.h>
#include <wchar.h>

#include <windows.h>

#include "rtb_private/window_impl.h"

#include "win_rtb.h"

#ifdef _WIN64
# define PRIuSIZE PRIu64
# define PRIxSIZE PRIx64
#else
# define PRIuSIZE PRIu32
# define PRIxSIZE PRIx32
#endif

/**
 * wndproc
 */

static LRESULT CALLBACK
win_rtb_wndproc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	return DefWindowProc(hwnd, message, wparam, lparam);
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
	UnregisterClassW((void *) MAKELONG(window_class, 0), NULL);
}

/**
 * window lifecycle
 */

static wchar_t *
utf8_to_utf16_alloc(const rtb_utf8_t *utf8)
{
	wchar_t *buf;
	size_t need;

	need = uv_utf8_to_utf16(utf8, NULL, 0);
	if (!need)
		return NULL;

	buf = calloc(sizeof(*buf), need);
	if (!buf)
		return NULL;

	uv_utf8_to_utf16(utf8, buf, need);
	return buf;
}

struct rtb_window *
window_impl_open(struct rutabaga *r,
		int width, int height, const char *title, intptr_t parent)
{
	struct win_rtb_window *self = calloc(1, sizeof(*self));
	wchar_t *wtitle;
	RECT wrect;
	int flags;

	self->window_class = make_window_class(self);
	if (!self->window_class)
		goto err_window_class;

	flags = WS_POPUPWINDOW | WS_CAPTION | WS_SIZEBOX | WS_VISIBLE;
	wrect = (RECT) {0, 0, width, height};
	AdjustWindowRectEx(&wrect, flags, FALSE, WS_EX_TOPMOST);

	wtitle = utf8_to_utf16_alloc(title);
	if (!wtitle)
		goto err_wtitle;

	if (parent)
		flags = WS_CHILD;

	self->hwnd = CreateWindowExW((DWORD) 0,
			(void *) MAKELONG(self->window_class, 0), wtitle, flags,
			wrect.left, wrect.top,
			wrect.right - wrect.left, wrect.bottom - wrect.top,
			(HWND) parent, NULL, NULL, NULL);

	free(wtitle);

	if (!self->hwnd)
		goto err_createwindow;

	SetWindowLongPtr(self->hwnd, 0, self);

	return RTB_WINDOW(self);

err_createwindow:
err_wtitle:
	free_window_class(self->window_class);
err_window_class:
	return NULL;
}

void
window_impl_close(struct rtb_window *rwin)
{
	struct win_rtb_window *self = RTB_WINDOW_AS(rwin, win_rtb_window);

	free_window_class(self->window_class);
	free(self);
}

/**
 * rutabaga lifecycle
 */

struct rutabaga *
window_impl_rtb_alloc(void)
{
	struct rutabaga *rtb = calloc(1, sizeof(*rtb));
	return rtb;
}

void
window_impl_rtb_free(struct rutabaga *rtb)
{
	free(rtb);
}
