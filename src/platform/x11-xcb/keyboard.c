/**
 * rutabaga: an OpenGL widget toolkit
 * Copyright (c) 2013 William Light.
 * All rights reserved.
 *
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <http://unlicense.org/>
 */

#include <assert.h>
#include <stdio.h>

#include <X11/XKBlib.h>
#include <X11/extensions/XKBfile.h>
#include <xkbcommon/xkbcommon.h>

#include "xrtb.h"

rtb_keysym_t
xrtb_keyboard_translate_keysym(xcb_keysym_t xsym, rtb_utf32_t *chr)
{
	switch (xsym) {
#define CODE(xkey, rtbkey) case xkey: return rtbkey
#define CHAR(xkey, rtbkey, wc) case xkey: *chr = wc; return rtbkey

		CODE(XKB_KEY_Shift_L,     RTB_KEY_LEFT_SHIFT);
		CODE(XKB_KEY_Control_L,   RTB_KEY_LEFT_CTRL);
		CODE(XKB_KEY_Super_L,     RTB_KEY_LEFT_SUPER);
		CODE(XKB_KEY_Alt_L,       RTB_KEY_LEFT_ALT);

		CODE(XKB_KEY_Shift_R,     RTB_KEY_RIGHT_SHIFT);
		CODE(XKB_KEY_Control_R,   RTB_KEY_RIGHT_CTRL);
		CODE(XKB_KEY_Super_R,     RTB_KEY_RIGHT_SUPER);
		CODE(XKB_KEY_Alt_R,       RTB_KEY_RIGHT_ALT);

		CODE(XKB_KEY_Menu,        RTB_KEY_MENU);
		CODE(XKB_KEY_Escape,      RTB_KEY_ESCAPE);

		CODE(XKB_KEY_BackSpace,   RTB_KEY_BACKSPACE);
		CODE(XKB_KEY_Return,      RTB_KEY_ENTER);
		CODE(XKB_KEY_Tab,         RTB_KEY_TAB);
		CODE(XKB_KEY_Caps_Lock,   RTB_KEY_CAPS_LOCK);

		CODE(XKB_KEY_F1,          RTB_KEY_F1);
		CODE(XKB_KEY_F2,          RTB_KEY_F2);
		CODE(XKB_KEY_F3,          RTB_KEY_F3);
		CODE(XKB_KEY_F4,          RTB_KEY_F4);
		CODE(XKB_KEY_F5,          RTB_KEY_F5);
		CODE(XKB_KEY_F6,          RTB_KEY_F6);
		CODE(XKB_KEY_F7,          RTB_KEY_F7);
		CODE(XKB_KEY_F8,          RTB_KEY_F8);
		CODE(XKB_KEY_F9,          RTB_KEY_F9);
		CODE(XKB_KEY_F10,         RTB_KEY_F10);
		CODE(XKB_KEY_F11,         RTB_KEY_F11);
		CODE(XKB_KEY_F12,         RTB_KEY_F12);

		CODE(XKB_KEY_Print,       RTB_KEY_PRINT_SCREEN);
		CODE(XKB_KEY_Pause,       RTB_KEY_SCROLL_LOCK);
		CODE(XKB_KEY_Scroll_Lock, RTB_KEY_SCROLL_LOCK);

		CODE(XKB_KEY_Insert,      RTB_KEY_INSERT);
		CODE(XKB_KEY_Delete,      RTB_KEY_DELETE);
		CODE(XKB_KEY_Home,        RTB_KEY_HOME);
		CODE(XKB_KEY_End,         RTB_KEY_END);
		CODE(XKB_KEY_Prior,       RTB_KEY_PAGE_UP);
		CODE(XKB_KEY_Next,        RTB_KEY_PAGE_DOWN);

		CODE(XKB_KEY_Up,          RTB_KEY_UP);
		CODE(XKB_KEY_Left,        RTB_KEY_LEFT);
		CODE(XKB_KEY_Down,        RTB_KEY_DOWN);
		CODE(XKB_KEY_Right,       RTB_KEY_RIGHT);

		CODE(XKB_KEY_Num_Lock,    RTB_KEY_NUM_LOCK);
		CHAR(XKB_KEY_KP_Divide,   RTB_KEY_NUMPAD, '/');
		CHAR(XKB_KEY_KP_Multiply, RTB_KEY_NUMPAD, '*');
		CHAR(XKB_KEY_KP_Subtract, RTB_KEY_NUMPAD, '-');
		CHAR(XKB_KEY_KP_Add,      RTB_KEY_NUMPAD, '+');

		CODE(XKB_KEY_KP_Home,     RTB_KEY_NUMPAD_HOME);
		CODE(XKB_KEY_KP_Up,       RTB_KEY_NUMPAD_UP);
		CODE(XKB_KEY_KP_Prior,    RTB_KEY_NUMPAD_PAGE_UP);
		CODE(XKB_KEY_KP_Left,     RTB_KEY_NUMPAD_LEFT);
		CODE(XKB_KEY_KP_Begin,    RTB_KEY_NUMPAD_MIDDLE);
		CODE(XKB_KEY_KP_Right,    RTB_KEY_NUMPAD_RIGHT);
		CODE(XKB_KEY_KP_End,      RTB_KEY_NUMPAD_END);
		CODE(XKB_KEY_KP_Down,     RTB_KEY_NUMPAD_DOWN);
		CODE(XKB_KEY_KP_Next,     RTB_KEY_NUMPAD_PAGE_DOWN);

		CODE(XKB_KEY_KP_Insert,   RTB_KEY_NUMPAD_INSERT);
		CODE(XKB_KEY_KP_Delete,   RTB_KEY_NUMPAD_INSERT);
		CODE(XKB_KEY_KP_Enter,    RTB_KEY_NUMPAD_ENTER);

#undef CHAR
#undef CODE
	}

	return RTB_KEY_UNKNOWN;
}

int
xrtb_keyboard_reload(struct xcb_rutabaga *xrtb)
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

	XkbFreeKeyboard(xkbfile.xkb, XkbAllMapComponentsMask, 1);
	fclose(tmp);

	if (xrtb->xkb_keymap)
		xkb_keymap_unref(xrtb->xkb_keymap);
	xrtb->xkb_keymap = new_keymap;

	if (xrtb->xkb_state)
		xkb_state_unref(xrtb->xkb_state);

	if (!(xrtb->xkb_state = xkb_state_new(new_keymap)))
		goto err_new_state;

#define CACHE_MOD_INDEX(name, constant) \
	xrtb->mod_indices.name = xkb_keymap_mod_get_index(new_keymap, constant)

	CACHE_MOD_INDEX(shift, XKB_MOD_NAME_SHIFT);
	CACHE_MOD_INDEX(ctrl,  XKB_MOD_NAME_CTRL);
	CACHE_MOD_INDEX(alt,   XKB_MOD_NAME_ALT);
	CACHE_MOD_INDEX(super, XKB_MOD_NAME_LOGO);

#undef CACHE_MOD_INDEX
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

int
xrtb_keyboard_init(struct xcb_rutabaga *xrtb)
{
	assert(!xrtb->xkb_ctx);

	if (!(xrtb->xkb_ctx = xkb_context_new(0)))
		return -1;

	if (xrtb_keyboard_reload(xrtb))
		return -1;
	return 0;
}

void
xrtb_keyboard_fini(struct xcb_rutabaga *xrtb)
{
	assert(xrtb->xkb_ctx);

	xkb_context_unref(xrtb->xkb_ctx);

	if (xrtb->xkb_keymap)
		xkb_keymap_unref(xrtb->xkb_keymap);

	if (xrtb->xkb_state)
		xkb_state_unref(xrtb->xkb_state);
}
