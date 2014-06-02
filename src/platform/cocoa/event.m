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

/* XXX: hard-coding this because i don't know how to get vblank interval from
 *      the os yet. using a CVDisplayLink is unreliable. */
#define FPS 60
#define FRAME_SECONDS (1.0 / (double) FPS)

/**
 * frame rendering
 */

static void
frame_timer_callback(CFRunLoopTimerRef timer, void *info)
{
	struct cocoa_rtb_window *cwin = info;
	rtb_cocoa_draw_frame(cwin, 0);
}

/**
 * uv shim
 */

static void
run_uv_callback(CFRunLoopObserverRef observer, CFRunLoopActivity activity,
		void *info)
{
	uv_loop_t *loop = info;
	uv_run(loop, UV_RUN_NOWAIT);
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

	r->event_loop = uv_loop_new();
	obs_ctx.info = r->event_loop;

	cwin->run_uv_observer = CFRunLoopObserverCreate(
			kCFAllocatorDefault, kCFRunLoopBeforeSources, 1, 0,
			run_uv_callback, &obs_ctx);

	cwin->we_are_running_nsapp = 0;
}

void
rtb_event_loop_run(struct rutabaga *r)
{
	struct rtb_window *win;
	struct cocoa_rtb_window *cwin;

	CFRunLoopTimerContext timer_ctx = {};

	win  = r->win;
	cwin = RTB_WINDOW_AS(win, cocoa_rtb_window);

	if (cwin->event_loop_running)
		return;

	rtb_window_reinit(win);
	cwin->event_loop_running = 1;

	timer_ctx.info = cwin;


	cwin->frame_timer = CFRunLoopTimerCreate(
			kCFAllocatorDefault, CFAbsoluteTimeGetCurrent(),
			FRAME_SECONDS, 0, 0, frame_timer_callback, &timer_ctx);

	CFRunLoopAddObserver([[NSRunLoop mainRunLoop] getCFRunLoop],
			cwin->run_uv_observer, kCFRunLoopCommonModes);

	CFRunLoopAddTimer([[NSRunLoop mainRunLoop] getCFRunLoop],
			cwin->frame_timer, kCFRunLoopCommonModes);

	if (cwin->cocoa_win)
		[cwin->cocoa_win makeKeyAndOrderFront:cwin->cocoa_win];

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

	win  = r->win;
	cwin = RTB_WINDOW_AS(win, cocoa_rtb_window);

	cwin->event_loop_running = 0;

	if (cwin->we_are_running_nsapp)
		[NSApp stop:nil];

	CFRunLoopTimerInvalidate(cwin->frame_timer);
	CFRelease(cwin->frame_timer);

	CFRunLoopRemoveObserver([[NSRunLoop mainRunLoop] getCFRunLoop],
			cwin->run_uv_observer, kCFRunLoopCommonModes);
}

void
rtb_event_loop_fini(struct rutabaga *r)
{
	uv_loop_t *rtb_loop;
	struct rtb_window *win;
	struct cocoa_rtb_window *cwin;

	win  = r->win;
	cwin = RTB_WINDOW_AS(win, cocoa_rtb_window);

	if (cwin->event_loop_running)
		rtb_event_loop_stop(r);

	CFRunLoopObserverInvalidate(cwin->run_uv_observer);
	CFRelease(cwin->run_uv_observer);

	rtb_loop = r->event_loop;
	r->event_loop = NULL;

	/* once to run the event handlers for the last time, and once more
	 * to run the endgames for all of them. */
	uv_run(rtb_loop, UV_RUN_NOWAIT);
	uv_run(rtb_loop, UV_RUN_NOWAIT);

	uv_loop_delete(rtb_loop);
}
