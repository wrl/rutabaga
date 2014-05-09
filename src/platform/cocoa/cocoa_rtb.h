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

#import <Cocoa/Cocoa.h>
#include <CoreFoundation/CFRunLoop.h>

#include "rutabaga/rutabaga.h"
#include "rutabaga/window.h"

struct cocoa_rtb_window;

@interface RutabagaOpenGLContext : NSOpenGLContext
{
@public
	const NSOpenGLPixelFormat *pixelFormat;
}
@end

@interface RutabagaWindow : NSWindow
{
@public
	struct cocoa_rtb_window *rtb_win;
}
@end

@interface RutabagaOpenGLView : NSView
{
@public
	struct cocoa_rtb_window *rtb_win;
@private
	NSTrackingArea *tracking_area;
	BOOL was_mouse_coalescing_enabled;
	BOOL window_did_accept_mouse_moved_events;
}
@end

struct cocoa_rtb_window {
	RTB_INHERIT(rtb_window);
	RutabagaWindow *cocoa_win;
	RutabagaOpenGLView *view;
	RutabagaOpenGLContext *gl_ctx;

	CVDisplayLinkRef display_link;
	uv_sem_t fake_event_loop;

	volatile int event_loop_running;
	int we_are_running_nsapp;
};

void rtb_cocoa_draw_frame(struct cocoa_rtb_window *);

/* vim: set ft=objc :*/
