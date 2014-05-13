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

#include "rutabaga/rutabaga.h"
#include "rutabaga/window.h"
#include "rutabaga/platform.h"

#import <Cocoa/Cocoa.h>
#include <CoreFoundation/CFRunLoop.h>

#include "rtb_private/window_impl.h"
#include "cocoa_rtb.h"

/**
 * display link
 */

static CVReturn
display_link_cb(CVDisplayLinkRef display_link, const CVTimeStamp *now,
		const CVTimeStamp *frame_time, CVOptionFlags flags_in,
		CVOptionFlags *flags_out, void *ctx)
{
	struct cocoa_rtb_window *cwin = ctx;

	uv_run(cwin->rtb->event_loop, UV_RUN_NOWAIT);
	rtb_cocoa_draw_frame(cwin);

	return kCVReturnSuccess;
}

/**
 * public API
 */

void
rtb_event_loop_init(struct rutabaga *r)
{
	struct rtb_window *win;
	struct cocoa_rtb_window *cwin;
	RutabagaOpenGLContext *gl_ctx;
	CFRunLoopObserverContext obs_ctx = {};

	win    = r->win;
	cwin   = RTB_WINDOW_AS(win, cocoa_rtb_window);
	gl_ctx = cwin->gl_ctx;

	CVDisplayLinkCreateWithActiveCGDisplays(&cwin->display_link);
	CVDisplayLinkSetOutputCallback(cwin->display_link, display_link_cb, cwin);
	CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(cwin->display_link,
			[gl_ctx CGLContextObj], [gl_ctx->pixelFormat CGLPixelFormatObj]);

	r->event_loop = uv_loop_new();
	obs_ctx.info = r->event_loop;

	cwin->we_are_running_nsapp = 0;
}

void
rtb_event_loop_run(struct rutabaga *r)
{
	struct rtb_window *win;
	struct cocoa_rtb_window *cwin;

	win    = r->win;
	cwin   = RTB_WINDOW_AS(win, cocoa_rtb_window);

	rtb_window_reinit(win);
	cwin->event_loop_running = 1;

	CVDisplayLinkStart(cwin->display_link);

	if (![NSApp isRunning]) {
		cwin->we_are_running_nsapp = 1;
		[NSApp run];
	}
}

void
rtb_event_loop_stop(struct rutabaga *r)
{
	struct rtb_window *win;
	struct cocoa_rtb_window *cwin;

	win    = r->win;
	cwin   = RTB_WINDOW_AS(win, cocoa_rtb_window);

	cwin->event_loop_running = 0;

	if (cwin->we_are_running_nsapp)
		[NSApp stop:nil];

	CVDisplayLinkStop(cwin->display_link);
}

void
rtb_event_loop_fini(struct rutabaga *r)
{
	uv_loop_t *rtb_loop;
	struct rtb_window *win;
	struct cocoa_rtb_window *cwin;

	win    = r->win;
	cwin   = RTB_WINDOW_AS(win, cocoa_rtb_window);

	CVDisplayLinkRelease(cwin->display_link);

	rtb_loop = r->event_loop;
	r->event_loop = NULL;
	uv_loop_delete(rtb_loop);
}
