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
#include <stdio.h>

#include <X11/XKBlib.h>
#include <X11/extensions/XKBfile.h>
#include <xkbcommon/xkbcommon.h>

#include "xrtb.h"

int xrtb_keyboard_reload(struct xcb_rutabaga *xrtb)
{
	FILE *tmp;
	XkbFileInfo xkbfile;
	struct xkb_keymap *new_keymap;

	/* large portions here lifted from i3lock.
	 * thx. */

	assert(xrtb->xkb_ctx);

	if (!(tmp = tmpfile()))
		goto err_tmpfile;

	if (!(xkbfile.xkb = XkbGetKeyboard(xrtb->dpy,
					XkbAllMapComponentsMask, XkbUseCoreKbd)))
		goto err_get_keyboard;

	if (!XkbWriteXKBKeymap(tmp, &xkbfile, 0, 0, NULL, NULL))
		goto err_write_keymap;

	rewind(tmp);

	if (!(new_keymap = xkb_keymap_new_from_file(xrtb->xkb_ctx, tmp,
					XKB_KEYMAP_FORMAT_TEXT_V1, 0)))
		goto err_new_keymap;

	if (xrtb->xkb_keymap)
		xkb_keymap_unref(xrtb->xkb_keymap);
	xrtb->xkb_keymap = new_keymap;

	if (xrtb->xkb_state)
		xkb_state_unref(xrtb->xkb_state);

	if (!(xrtb->xkb_state = xkb_state_new(new_keymap)))
		goto err_new_state;

	XkbFreeKeyboard(xkbfile.xkb, XkbAllMapComponentsMask, 1);
	fclose(tmp);
	return 0;

err_new_state:
err_new_keymap:
err_write_keymap:
	XkbFreeKeyboard(xkbfile.xkb, XkbAllMapComponentsMask, 1);
err_get_keyboard:
	fclose(tmp);
err_tmpfile:
	return -1;
}

int xrtb_keyboard_init(struct xcb_rutabaga *xrtb)
{
	assert(!xrtb->xkb_ctx);

	if (!(xrtb->xkb_ctx = xkb_context_new(0)))
		return -1;

	if (xrtb_keyboard_reload(xrtb))
		return -1;
	return 0;
}

void xrtb_keyboard_fini(struct xcb_rutabaga *xrtb)
{
	assert(xrtb->xkb_ctx);

	xkb_context_unref(xrtb->xkb_ctx);

	if (xrtb->xkb_keymap)
		xkb_keymap_unref(xrtb->xkb_keymap);

	if (xrtb->xkb_state)
		xkb_state_unref(xrtb->xkb_state);
}
