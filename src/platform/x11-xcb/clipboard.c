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

#include <unistd.h>

#include <rutabaga/window.h>
#include <rutabaga/platform.h>

#include "xrtb.h"

void
rtb_copy_to_clipboard(struct rtb_window *rwin, const rtb_utf8_t *buf,
		size_t nbytes)
{
	struct xrtb_window *self = RTB_WINDOW_AS(rwin, xrtb_window);
	struct xcb_rutabaga *xrtb = self->xrtb;

	free(xrtb->clipboard.buffer);
	xrtb->clipboard.buffer = strndup(buf, nbytes);
	xrtb->clipboard.nbytes = nbytes;

	if (!xrtb->clipboard.buffer)
		return;

	xcb_set_selection_owner(xrtb->xcb_conn, self->xcb_win,
			xrtb->atoms.clipboard, XCB_CURRENT_TIME);
	xcb_flush(xrtb->xcb_conn);
}

ssize_t
rtb_paste_from_clipboard(struct rtb_window *rwin, rtb_utf8_t **buf)
{
	struct xrtb_window *self = RTB_WINDOW_AS(rwin, xrtb_window);
	struct xcb_rutabaga *xrtb = self->xrtb;
	xcb_get_property_reply_t *prop;
	xcb_get_property_cookie_t pck;
	xcb_void_cookie_t cookie;
	xcb_generic_error_t *err;
	xcb_connection_t *conn;
	ssize_t ret;
	unsigned i;
	int type;

	if (xrtb->clipboard.buffer) {
		*buf = strdup(xrtb->clipboard.buffer);
		return xrtb->clipboard.nbytes;
	}

	conn = xrtb->clipboard.conn;

	cookie = xcb_convert_selection_checked(conn, xrtb->clipboard.window,
			xrtb->atoms.clipboard, xrtb->atoms.utf8_string,
			xrtb->atoms.clipboard, XCB_CURRENT_TIME);
	xcb_flush(conn);

	if ((err = xcb_request_check(conn, cookie))) {
		ERR("error in xcb_convert_selection_checked() %d\n", err->error_code);
		free(err);
		goto err_convert_selection;
	}

	for (i = 1 << 7; i < (1 << 15); i <<= 1) {
		xcb_generic_event_t *ev = xcb_poll_for_event(conn);
		if (!ev) {
			usleep(i);
			continue;
		}

		type = ev->response_type & ~0x80;
		free(ev);

		if (type == XCB_SELECTION_NOTIFY)
			break;
	}

	pck = xcb_get_property(conn, 1, xrtb->clipboard.window,
			xrtb->atoms.clipboard, xrtb->atoms.utf8_string,
			0, 0);
	prop = xcb_get_property_reply(conn, pck, NULL);

	ret = prop->bytes_after;
	*buf = calloc(ret + 1, sizeof(**buf));
	if (!*buf)
		goto err_malloc;

	free(prop);

	pck = xcb_get_property(conn, 1, xrtb->clipboard.window,
			xrtb->atoms.clipboard, xrtb->atoms.utf8_string,
			0, (ret + 4) >> 2);
	prop = xcb_get_property_reply(conn, pck, NULL);

	if (xcb_get_property_value_length(prop) > ret) {
		ERR("allocated %zd bytes, now property is %d bytes\n", ret,
				xcb_get_property_value_length(prop));
		goto err_bounds_check;
	}

	memcpy(*buf, xcb_get_property_value(prop), ret);
	free(prop);
	(*buf)[ret] = 0;
	return ret;

err_bounds_check:
	free(*buf);
err_malloc:
	free(prop);
err_convert_selection:
	*buf = NULL;
	return -1;
}
