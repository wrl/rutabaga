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

#include <glloadgen/gl_core.3.0.h>

#import <Cocoa/Cocoa.h>

#include "rutabaga/rutabaga.h"
#include "rutabaga/window.h"
#include "rutabaga/platform.h"

#include "private/window_impl.h"
#include "cocoa_rtb.h"

@implementation RutabagaWindow
- (id) initWithContentRect: (NSRect) contentRect
				 styleMask: (NSUInteger) windowStyle
				   backing: (NSBackingStoreType) bufferingType
					 defer: (BOOL) deferCreation
{
	self = [super initWithContentRect:contentRect
							styleMask:windowStyle
							  backing:bufferingType
								defer:deferCreation];

	return self;
}

- (BOOL) windowShouldClose: (id) sender
{
	return NO;
}

- (BOOL) canBecomeKeyWindow: (id) sender
{
	return YES;
}

- (void) sendEvent: (NSEvent *) event
{
	[super sendEvent:event];
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

@implementation RutabagaOpenGLView
- (id) initWithFrame: (NSRect) frame
{
	self = [super initWithFrame:frame];

	if (!self)
		return nil;

	tracking_area = nil;
	[self setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
	return self;
}

- (void) viewWillMoveToWindow: (NSWindow *) newWindow
{
	NSWindow *old_window;
	NSTrackingAreaOptions tracking_opts =
		NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved
		| NSTrackingEnabledDuringMouseDrag | NSTrackingInVisibleRect
		| NSTrackingActiveInActiveApp;

	if (!tracking_area) {
		tracking_area = [NSTrackingArea alloc];
		[self addTrackingArea:tracking_area];
	}

	old_window = [self window];
	if (old_window != nil) {
		[old_window
			setAcceptsMouseMovedEvents:window_did_accept_mouse_moved_events];
		was_mouse_coalescing_enabled = [NSEvent isMouseCoalescingEnabled];
		[NSEvent setMouseCoalescingEnabled:NO];
	}

	if (newWindow == nil) {
		[self removeTrackingArea:tracking_area];
		[tracking_area release];
		[NSEvent setMouseCoalescingEnabled:was_mouse_coalescing_enabled];
	} else {
		[tracking_area initWithRect:[self bounds]
							options:tracking_opts
							  owner:self
						   userInfo:nil];

		[newWindow setAcceptsMouseMovedEvents:YES];
	}

	[super viewWillMoveToWindow:newWindow];
}

- (void) setFrame: (NSRect) frame
{
	[super setFrame:frame];

	if (!rtb_win)
		return;

	rtb_win->w = frame.size.width;
	rtb_win->h = frame.size.height;

	rtb_window_lock(RTB_WINDOW(rtb_win));
	[rtb_win->gl_ctx update];
	rtb_window_reinit(RTB_WINDOW(rtb_win));
	rtb_window_draw(RTB_WINDOW(rtb_win));
	[rtb_win->gl_ctx flushBuffer];
	rtb_window_unlock(RTB_WINDOW(rtb_win));

	[super setNeedsDisplay:YES];
}

- (void) mouseEntered: (NSEvent *) e
{
	NSPoint pt = [self convertPoint:[e locationInWindow] fromView:nil];
	rtb_platform_mouse_enter_window(RTB_WINDOW(rtb_win), pt.x, pt.y);
}

- (void) mouseExited: (NSEvent *) e
{
	NSPoint pt = [self convertPoint:[e locationInWindow] fromView:nil];
	rtb_platform_mouse_leave_window(RTB_WINDOW(rtb_win), pt.x, pt.y);
}

- (void) mouseMoved: (NSEvent *) e
{
	NSPoint pt = [self convertPoint:[e locationInWindow] fromView:nil];
	rtb_platform_mouse_motion(RTB_WINDOW(rtb_win), pt.x, rtb_win->h - pt.y);
}

- (void) mouseDragged: (NSEvent *) e
{
	[self mouseMoved:e];
}

- (void) mouseDown: (NSEvent *) e
{
	NSPoint pt = [self convertPoint:[e locationInWindow] fromView:nil];
	rtb_platform_mouse_press(RTB_WINDOW(rtb_win),
			RTB_MOUSE_BUTTON1, pt.x, pt.y);
}

- (void) mouseUp: (NSEvent *) e
{
	NSPoint pt = [self convertPoint:[e locationInWindow] fromView:nil];
	rtb_platform_mouse_release(RTB_WINDOW(rtb_win),
			RTB_MOUSE_BUTTON1, pt.x, pt.y);
}
@end

@interface RutabagaWindowDelegate : NSResponder <NSWindowDelegate>
{
@public
	struct cocoa_rtb_window *rtb_win;
}

- (void) windowDidResize: (NSNotification *) notification;
@end

@implementation RutabagaWindowDelegate
- (void) windowDidResize: (NSNotification *) notification
{
	return;
}
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

struct rtb_window *
window_impl_open(struct rutabaga *rtb,
		int w, int h, const char *title, intptr_t parent)
{
	struct cocoa_rtb_window *self;
	RutabagaWindowDelegate *win_delegate;
	RutabagaWindow *cwin;
	RutabagaOpenGLContext *gl_ctx;
	RutabagaOpenGLView *view;
	NSString *nstitle;
	unsigned style_mask;

	self = calloc(1, sizeof(*self));
	if (!self)
		return NULL;

	gl_ctx = NULL;

	@autoreleasepool {
		nstitle =
			[[NSString alloc]
			initWithBytes:title
				   length:strlen(title)
				 encoding:NSUTF8StringEncoding];

		style_mask =
			NSTitledWindowMask
			| NSClosableWindowMask
			| NSMiniaturizableWindowMask;

		if (!parent)
			style_mask |= NSResizableWindowMask;

		@try {
			cwin = [[RutabagaWindow alloc]
				initWithContentRect:NSMakeRect(0, 0, w, h)
						  styleMask:style_mask
							backing:NSBackingStoreBuffered
							  defer:NO];
		} @catch (NSException *e) {
			return NULL;
		}

		[cwin setContentSize:NSMakeSize(w, h)];
		[cwin setTitle:nstitle];
		cwin->rtb_win = self;

		win_delegate = [[RutabagaWindowDelegate alloc] init];
		[cwin setDelegate:win_delegate];

		view = [RutabagaOpenGLView new];
		gl_ctx = alloc_gl_ctx();

		self->cocoa_win = cwin;
		self->gl_ctx    = gl_ctx;

		[cwin setContentView:view];
		[cwin makeFirstResponder:view];

		[gl_ctx setView:view];
		[gl_ctx makeCurrentContext];

		[NSApp activateIgnoringOtherApps:YES];
		[cwin makeKeyAndOrderFront:cwin];
		[cwin center];

		get_dpi(&self->dpi.x, &self->dpi.y);
		view->rtb_win = self;
	}

	uv_mutex_init(&self->lock);
	return RTB_WINDOW(self);
}

void
window_impl_close(struct rtb_window *rwin)
{
	return;
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
