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

#include <unistd.h>
#include <uv.h>

#include <glloadgen/gl_core.3.0.h>

#import <Cocoa/Cocoa.h>
#include <CoreFoundation/CFRunLoop.h>

#include "rutabaga/rutabaga.h"
#include "rutabaga/window.h"

#include "private/window_impl.h"
#include "cocoa_rtb.h"

static void
draw_frame(struct rtb_window *win)
{
	struct cocoa_rtb_window *cwin;

	cwin = RTB_WINDOW_AS(win, cocoa_rtb_window);

	rtb_window_lock(win);
	rtb_window_draw(win);
	[cwin->gl_ctx flushBuffer];
	rtb_window_unlock(win);
}

static CVReturn
display_link_cb(CVDisplayLinkRef display_link, const CVTimeStamp *now,
		const CVTimeStamp *frame_time, CVOptionFlags flags_in,
		CVOptionFlags *flags_out, void *ctx)
{
	struct rtb_window *win = ctx;
	draw_frame(win);

	return kCVReturnSuccess;
}

void
rtb_event_loop(struct rutabaga *r)
{
	struct rtb_window *win;
	RutabagaOpenGLContext *gl_ctx;
	CVDisplayLinkRef display_link;

	win = r->win;
	gl_ctx = RTB_WINDOW_AS(win, cocoa_rtb_window)->gl_ctx;

	CVDisplayLinkCreateWithActiveCGDisplays(&display_link);
	CVDisplayLinkSetOutputCallback(display_link, display_link_cb, win);
	CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(display_link,
			[gl_ctx CGLContextObj], [gl_ctx->pixelFormat CGLPixelFormatObj]);

	rtb_window_reinit(win);

	CVDisplayLinkStart(display_link);

	[NSApp run];
}
