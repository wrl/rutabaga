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

#include <rutabaga/opengl.h>

#import <Cocoa/Cocoa.h>

#include "rutabaga/rutabaga.h"
#include "rutabaga/window.h"
#include "rutabaga/platform.h"

#include "rtb_private/window_impl.h"
#include "cocoa_rtb.h"

#define LOCK (rtb_window_lock(RTB_WINDOW(rtb_win)))
#define UNLOCK (rtb_window_unlock(RTB_WINDOW(rtb_win)))

@implementation RutabagaWindow
- (BOOL) windowShouldClose: (id) sender
{
	struct rtb_window_event ev = {
		.type = RTB_WINDOW_CLOSE,
		.window = RTB_WINDOW(rtb_win)
	};

	rtb_dispatch_raw(RTB_ELEMENT(rtb_win), RTB_EVENT(&ev));
	return NO;
}

- (BOOL) canBecomeKeyWindow: (id) sender
{
	return YES;
}
@end

@implementation RutabagaOpenGLView
- (id) initWithFrame: (NSRect) frame
{
	self = [super initWithFrame:frame];

	if (!self)
		return nil;

	tracking_area = nil;
	[self setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];

	/*
	[self setWantsBestResolutionOpenGLSurface:YES];
	*/

	return self;
}

- (BOOL) isFlipped
{
	/* rutabaga's coordinate system puts (0,0) at the top left, cocoa's is
	 * at the bottom left. returning YES here has cocoa flip the y coord. */
	return YES;
}

- (BOOL) preservesContentInLiveResize
{
	return NO;
}

static void
reinit_tracking_area(RutabagaOpenGLView *self, NSTrackingArea *tracking_area)
{
	NSTrackingAreaOptions tracking_opts =
		NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved
		| NSTrackingEnabledDuringMouseDrag | NSTrackingInVisibleRect
		| NSTrackingAssumeInside | NSTrackingActiveAlways;

	[tracking_area initWithRect:[self bounds]
						options:tracking_opts
						  owner:self
					   userInfo:nil];
}

- (void) viewWillMoveToWindow: (NSWindow *) newWindow
{
	NSWindow *old_window;

	old_window = [self window];
	if (old_window != nil) {
		[old_window
			setAcceptsMouseMovedEvents:window_did_accept_mouse_moved_events];
		was_mouse_coalescing_enabled = [NSEvent isMouseCoalescingEnabled];
		[NSEvent setMouseCoalescingEnabled:NO];
	}

	if (newWindow == nil) {
		if (tracking_area) {
			[self removeTrackingArea:tracking_area];
			[tracking_area release];
		}

		[NSEvent setMouseCoalescingEnabled:was_mouse_coalescing_enabled];
	} else {
		[newWindow setAcceptsMouseMovedEvents:YES];

		if (!tracking_area) {
			tracking_area = [NSTrackingArea alloc];
			reinit_tracking_area(self, tracking_area);
			[self addTrackingArea:tracking_area];
		} else
			reinit_tracking_area(self, tracking_area);
	}

	[super viewWillMoveToWindow:newWindow];
}

- (void) setFrame: (NSRect) frame
{
	NSRect backing = [self convertRectToBacking:frame];

	[super setFrame:frame];

	if (!rtb_win)
		return;

	rtb_win->w = backing.size.width;
	rtb_win->h = backing.size.height;

	LOCK;
	[rtb_win->gl_ctx update];
	rtb_window_reinit(RTB_WINDOW(rtb_win));
	UNLOCK;
}

- (void) drawRect: (NSRect) dirtyRect
{
	if (!rtb_win)
		return;

	rtb_cocoa_draw_frame(rtb_win);
}

- (void) mouseEntered: (NSEvent *) e
{
	NSPoint pt = [self convertPoint:[e locationInWindow] fromView:nil];

	LOCK;
	rtb_platform_mouse_enter_window(RTB_WINDOW(rtb_win), pt.x, pt.y);
	UNLOCK;
}

- (void) mouseExited: (NSEvent *) e
{
	NSPoint pt = [self convertPoint:[e locationInWindow] fromView:nil];

	LOCK;
	rtb_platform_mouse_leave_window(RTB_WINDOW(rtb_win), pt.x, pt.y);
	UNLOCK;
}

- (void) mouseMoved: (NSEvent *) e
{
	NSPoint pt = [self convertPoint:[e locationInWindow] fromView:nil];

	LOCK;
	rtb_platform_mouse_motion(RTB_WINDOW(rtb_win), pt.x, pt.y);
	UNLOCK;
}

- (void) mouseDragged: (NSEvent *) e
{
	[self mouseMoved:e];
}

- (void) rightMouseDragged: (NSEvent *) e
{
	[self mouseDragged:e];
}

- (void) otherMouseDragged: (NSEvent *) e
{
	[self mouseDragged:e];
}

static rtb_mouse_buttons_t
app_kit_button_to_rtb_button(NSInteger app_kit_button)
{
	switch (app_kit_button) {
	case 0: return RTB_MOUSE_BUTTON1;
	case 1: return RTB_MOUSE_BUTTON3;
	case 2: return RTB_MOUSE_BUTTON2;
	default: return app_kit_button;
	}
}

- (void) mouseDown: (NSEvent *) e
{
	NSPoint pt = [self convertPoint:[e locationInWindow] fromView:nil];

	LOCK;
	rtb_platform_mouse_press(RTB_WINDOW(rtb_win),
			app_kit_button_to_rtb_button([e buttonNumber]), pt.x, pt.y);
	UNLOCK;
}

- (void) rightMouseDown: (NSEvent *) e
{
	[self mouseDown:e];
}

- (void) otherMouseDown: (NSEvent *) e
{
	[self mouseDown:e];
}

- (void) mouseUp: (NSEvent *) e
{
	NSPoint pt = [self convertPoint:[e locationInWindow] fromView:nil];

	LOCK;
	rtb_platform_mouse_release(RTB_WINDOW(rtb_win),
			app_kit_button_to_rtb_button([e buttonNumber]), pt.x, pt.y);
	UNLOCK;
}

- (void) rightMouseUp: (NSEvent *) e
{
	[self mouseUp:e];
}

- (void) otherMouseUp: (NSEvent *) e
{
	[self mouseUp:e];
}

#undef LOCK
#undef UNLOCK
@end

@implementation RutabagaOpenGLContext
@end

/**
 * rutabaga interface
 */

struct rutabaga *
window_impl_rtb_alloc(void)
{
	struct rutabaga *rtb = calloc(1, sizeof(*rtb));

	if (!NSApp) {
		[NSApplication sharedApplication];
		[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
		[NSApp finishLaunching];
	}

	return rtb;
}

void
window_impl_rtb_free(struct rutabaga *rtb)
{
	free(rtb);
}

/**
 * window things
 */

static void
get_dpi(int *x, int *y)
{
#if 0
	NSScreen *screen = [NSScreen mainScreen];
	NSDictionary *desc = [screen deviceDescription];
	NSSize pixel_size = [[desc objectForKey:NSDeviceSize] sizeValue];
	CGSize phy_size = CGDisplayScreenSize(
		[[desc objectForKey:@"NSScreenNumber"] unsignedIntValue]);

	*x = lrintf(ceilf((pixel_size.width / phy_size.width) * 25.4f));
	*y = lrintf(ceilf((pixel_size.height / phy_size.height) * 25.4f));
#else
	/* XXX: need some mechanism for scaling based on EMs. using the actual
	 *      DPI looks like shit at the moment. going to bite us in the ass
	 *      on hidpi tho. */

	*x = *y = 96;
#endif
}

static RutabagaOpenGLContext *
alloc_gl_ctx(void)
{
	RutabagaOpenGLContext *ctx;
	NSOpenGLPixelFormat *fmt;
	GLint interval = 1;

	NSOpenGLPixelFormatAttribute attribs[] = {
		NSOpenGLPFAOpenGLProfile,
		NSOpenGLProfileVersion3_2Core,
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAAccelerated,
		NSOpenGLPFAColorSize,
		8,
		0
	};

	fmt = [[NSOpenGLPixelFormat alloc] initWithAttributes:attribs];
	if (!fmt)
		return nil;

	ctx = [[RutabagaOpenGLContext alloc] initWithFormat:fmt shareContext:nil];
	[fmt retain];

	ctx->pixelFormat = fmt;
	[ctx setValues:&interval forParameter:NSOpenGLCPSwapInterval];

	if (!ctx)
		return nil;

	return ctx;
}

static RutabagaWindow *
alloc_nswindow(int w, int h, const char *title, int resizable)
{
	RutabagaWindow *win;
	NSString *nstitle;
	unsigned style_mask;

	style_mask =
		NSTitledWindowMask
		| NSClosableWindowMask
		| NSMiniaturizableWindowMask;

	nstitle = [[NSString alloc] initWithBytes:title
									   length:strlen(title)
									 encoding:NSUTF8StringEncoding];

	if (resizable)
		style_mask |= NSResizableWindowMask;

	@try {
		win = [[RutabagaWindow alloc]
			initWithContentRect:NSMakeRect(0, 0, w, h)
					  styleMask:style_mask
						backing:NSBackingStoreBuffered
						  defer:NO];
	} @catch (NSException *e) {
		return NULL;
	}

	[win setContentSize:NSMakeSize(w, h)];
	[win setTitle:nstitle];

	return win;
}

struct rtb_window *
window_impl_open(struct rutabaga *rtb,
		int w, int h, const char *title, intptr_t parent)
{
	struct cocoa_rtb_window *self;
	RutabagaWindow *cwin;
	RutabagaOpenGLContext *gl_ctx;
	RutabagaOpenGLView *view;
	NSView *parent_view;

	self = calloc(1, sizeof(*self));
	if (!self)
		return NULL;

	gl_ctx = NULL;

	@autoreleasepool {
		view = [RutabagaOpenGLView new];
		self->gl_ctx = gl_ctx = alloc_gl_ctx();

		if (parent) {
			self->cocoa_win = cwin = NULL;

			parent_view = (void *) parent;
			[parent_view addSubview:view];

			[view setFrame:NSMakeRect(0, 0, w, h)];
		} else {
			cwin = alloc_nswindow(w, h, title, 1);
			if (!cwin)
				goto err_alloc_nswindow;

			cwin->rtb_win = self;
			self->cocoa_win = cwin;

			[cwin setContentView:view];
			[cwin makeFirstResponder:view];
		}

		[gl_ctx setView:view];
		[gl_ctx makeCurrentContext];

		if (parent) {
			[view setHidden:NO];
		} else {
			[cwin makeKeyAndOrderFront:cwin];
			[NSApp activateIgnoringOtherApps:YES];
			[cwin center];
		}

		get_dpi(&self->dpi.x, &self->dpi.y);
		view->rtb_win = self;
		self->view = view;
	}

	uv_mutex_init(&self->lock);
	return RTB_WINDOW(self);

err_alloc_nswindow:
	free(self);
	return NULL;
}

void
window_impl_close(struct rtb_window *rwin)
{
	struct cocoa_rtb_window *self = RTB_WINDOW_AS(rwin, cocoa_rtb_window);

	[self->cocoa_win close];
	[self->view release];
	[self->gl_ctx release];
	[self->cocoa_win release];

	uv_mutex_destroy(&self->lock);
	free(self);

	return;
}

void
rtb_cocoa_draw_frame(struct cocoa_rtb_window *self)
{
	struct rtb_window *win = RTB_WINDOW(self);

	rtb_window_lock(win);

	if (win->visibility != RTB_FULLY_OBSCURED && self->dirty) {
		rtb_window_draw(win);
		[self->gl_ctx flushBuffer];
	}

	rtb_window_unlock(win);
}

/**
 * locking
 */

void
rtb_window_lock(struct rtb_window *rwin)
{
	struct cocoa_rtb_window *self = RTB_WINDOW_AS(rwin, cocoa_rtb_window);

	uv_mutex_lock(&self->lock);
	[self->gl_ctx makeCurrentContext];
}

void
rtb_window_unlock(struct rtb_window *rwin)
{
	struct cocoa_rtb_window *self = RTB_WINDOW_AS(rwin, cocoa_rtb_window);

	[NSOpenGLContext clearCurrentContext];
	uv_mutex_unlock(&self->lock);
}
