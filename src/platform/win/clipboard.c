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

#include <windows.h>
#include <rutabaga/platform.h>

#include "win_rtb.h"

void
rtb_copy_to_clipboard(struct rtb_window *rwin, const rtb_utf8_t *buf,
		size_t nbytes)
{
	struct win_rtb_window *self = RTB_WINDOW_AS(rwin, win_rtb_window);

	WCHAR *clip_obj_ptr;
	HANDLE clip_obj;
	size_t need;

	need = MultiByteToWideChar(CP_UTF8, 0, buf, -1, NULL, 0);
	if (!need)
		return;

	clip_obj = GlobalAlloc(GMEM_MOVEABLE, need * sizeof(*clip_obj_ptr));
	if (!buf)
		return;

	clip_obj_ptr = GlobalLock(clip_obj);
	if (!clip_obj_ptr)
		goto err_globallock;

	MultiByteToWideChar(CP_UTF8, 0, buf, -1, clip_obj_ptr, need);
	GlobalUnlock(clip_obj);

	if (!OpenClipboard(self->hwnd)) /* FIXME: retry? */
		goto err_openclipboard;

	EmptyClipboard();
	SetClipboardData(CF_UNICODETEXT, clip_obj);
	CloseClipboard();

	return;

err_openclipboard:
err_globallock:
	GlobalFree(clip_obj);
}

ssize_t
rtb_paste_from_clipboard(struct rtb_window *rwin, rtb_utf8_t **buf)
{
	struct win_rtb_window *self = RTB_WINDOW_AS(rwin, win_rtb_window);

	WCHAR *clip_obj_ptr;
	HANDLE clip_obj;
	size_t nbytes;

	if (!OpenClipboard(self->hwnd)) /* FIXME: retry? */
		goto err_openclipboard;

	clip_obj = GetClipboardData(CF_UNICODETEXT);
	if (!clip_obj)
		goto err_no_clipboard_data;

	clip_obj_ptr = GlobalLock(clip_obj);
	if (!clip_obj_ptr)
		goto err_globallock;

	nbytes = WideCharToMultiByte(CP_UTF8, 0, clip_obj_ptr, -1,
			NULL, 0, NULL, NULL);
	if (nbytes <= 0)
		goto err_wc2mb_probe;

	*buf = calloc(nbytes, sizeof(**buf));

	if (WideCharToMultiByte(CP_UTF8, 0, clip_obj_ptr, -1,
				*buf, nbytes, NULL, NULL) <= 0)
		goto err_wc2mb;

	GlobalUnlock(clip_obj);
	CloseClipboard();

	return nbytes;

err_wc2mb:
	free(*buf);
err_wc2mb_probe:
	GlobalUnlock(clip_obj);
err_globallock:
err_no_clipboard_data:
	CloseClipboard();
err_openclipboard:
	return -1;
}
