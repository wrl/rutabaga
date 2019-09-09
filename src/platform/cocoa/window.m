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

#include <rutabaga/opengl.h>

#import <Cocoa/Cocoa.h>

#include <rutabaga/rutabaga.h>
#include <rutabaga/window.h>
#include <rutabaga/platform.h>

#include "rtb_private/window_impl.h"
#include "cocoa_rtb.h"

#if __MAC_OS_X_VERSION_MAX_ALLOWED < 101200
/* apple renamed all these constants in the 10.12 SDK, so, when compiling on
 * earlier SDKs, we'll define them to their previous constant value. */

#define NSWindowStyleMaskTitled         NSTitledWindowMask
#define NSWindowStyleMaskClosable       NSClosableWindowMask
#define NSWindowStyleMaskMiniaturizable NSMiniaturizableWindowMask

#define NSWindowStyleMaskResizable      NSResizableWindowMask

#define NSEventModifierFlagOption       NSAlternateKeyMask
#define NSEventModifierFlagShift        NSShiftKeyMask
#define NSEventModifierFlagControl      NSControlKeyMask
#define NSEventModifierFlagCommand      NSCommandKeyMask
#endif

static NSPoint
event_location_to_view(NSView *view, NSEvent *e)
{
	return [view convertPoint:[e locationInWindow] fromView:nil];
}

#define LOCK (rtb_window_lock(RTB_WINDOW(rtb_win)))
#define UNLOCK (rtb_window_unlock(RTB_WINDOW(rtb_win)))

@implementation RutabagaWindow
- (BOOL) windowShouldClose: (id) sender
{
	struct rtb_window_event ev = {
		.type = RTB_WINDOW_SHOULD_CLOSE,
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

	[self setWantsBestResolutionOpenGLSurface:YES];

	control_chorded_left_button = false;
	return self;
}

- (BOOL) acceptsFirstMouse: (NSEvent *) theEvent
{
	return YES;
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
		| NSTrackingAssumeInside | NSTrackingActiveInActiveApp
		| NSTrackingCursorUpdate;

	[tracking_area initWithRect:[self bounds]
						options:tracking_opts
						  owner:self
					   userInfo:nil];
}

- (NSPoint) mouseLocationOutsideOfEventStream
{
	NSPoint win_cursor;

	win_cursor = [[self window] mouseLocationOutsideOfEventStream];
	return [self convertPoint:win_cursor fromView:nil];
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
			tracking_area = NULL;
		}

		[NSEvent setMouseCoalescingEnabled:was_mouse_coalescing_enabled];
	} else {
		if (!tracking_area) {
			tracking_area = [NSTrackingArea alloc];
			[self addTrackingArea:tracking_area];
		}

		[newWindow setAcceptsMouseMovedEvents:YES];
		[newWindow makeFirstResponder:self];
	}

	[super viewWillMoveToWindow:newWindow];
}

- (void) updateTrackingAreas
{
	reinit_tracking_area(self, tracking_area);
}

- (void) setFrameSize: (NSSize) size
{
	[super setFrameSize:size];

	if (!rtb_win)
		return;

	size = [self convertSizeToBacking:size];

	rtb_win->phy_size.w = size.width;
	rtb_win->phy_size.h = size.height;

	LOCK;
	[rtb_win->gl_ctx update];
	rtb_window_reinit(RTB_WINDOW(rtb_win));
	UNLOCK;
}

- (void) drawRect: (NSRect) dirtyRect
{
	if (!rtb_win)
		return;

	if ([self inLiveResize])
		rtb__cocoa_draw_frame(rtb_win, 1);
	else
		rtb_win->dirty = 1;
}

- (void) cursorUpdate: (NSEvent *) e
{
	rtb__cocoa_sync_cursor(self->rtb_win);
}

- (void) scrollWheel: (NSEvent *) e
{
	NSPoint pt = event_location_to_view(self, e);
	float delta = [e scrollingDeltaY];

	if ([e isDirectionInvertedFromDevice])
		delta *= -1.f;

	delta /= ([e hasPreciseScrollingDeltas]) ? 9.f : 3.f;

	LOCK;
	rtb__platform_mouse_wheel(RTB_WINDOW(rtb_win),
			RTB_MAKE_POINT(pt.x, pt.y),
			delta);
	UNLOCK;
}

- (void) mouseEntered: (NSEvent *) e
{
	NSPoint pt = event_location_to_view(self, e);

	LOCK;
	rtb__platform_mouse_enter_window(RTB_WINDOW(rtb_win),
			RTB_MAKE_POINT(pt.x, pt.y));
	UNLOCK;
}

- (void) mouseExited: (NSEvent *) e
{
	NSPoint pt = event_location_to_view(self, e);

	LOCK;
	rtb__platform_mouse_leave_window(RTB_WINDOW(rtb_win),
			RTB_MAKE_POINT(pt.x, pt.y));
	UNLOCK;
}

- (void) mouseMoved: (NSEvent *) e
{
	NSPoint pt = event_location_to_view(self, e);

	LOCK;
	rtb__platform_mouse_motion(RTB_WINDOW(rtb_win),
			RTB_MAKE_POINT(pt.x, pt.y));
	rtb__cocoa_sync_cursor(self->rtb_win);
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
app_kit_button_to_rtb_button(const NSEvent *e)
{
	NSInteger app_kit_button = [e buttonNumber];

	switch (app_kit_button) {
	case 0: return RTB_MOUSE_BUTTON1;
	case 1: return RTB_MOUSE_BUTTON3;
	case 2: return RTB_MOUSE_BUTTON2;
	default: return app_kit_button;
	}
}

- (void) mouseDown: (NSEvent *) e
{
	NSPoint pt = event_location_to_view(self, e);
	NSInteger control_key = !!(e.modifierFlags & NSControlKeyMask);

	rtb_mouse_buttons_t button = app_kit_button_to_rtb_button(e);

	if (button == RTB_MOUSE_BUTTON_LEFT && control_key) {
		control_chorded_left_button = true;
		button = RTB_MOUSE_BUTTON_RIGHT;
	}

	LOCK;
	rtb__platform_mouse_press(RTB_WINDOW(rtb_win), button,
			RTB_MAKE_POINT(pt.x, pt.y));
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
	NSPoint pt = event_location_to_view(self, e);

	rtb_mouse_buttons_t button = app_kit_button_to_rtb_button(e);

	if (button == RTB_MOUSE_BUTTON_LEFT && control_chorded_left_button) {
		control_chorded_left_button = false;
		button = RTB_MOUSE_BUTTON_RIGHT;
	}

	LOCK;
	rtb__platform_mouse_release(RTB_WINDOW(rtb_win), button,
			RTB_MAKE_POINT(pt.x, pt.y));
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

- (oneway void) release
{
	[super release];

	if (rtb_win && [self retainCount] == 1) {
		struct rtb_window_event ev = {
			.type = RTB_WINDOW_WILL_CLOSE,
			.window = RTB_WINDOW(rtb_win)
		};

		rtb_win->on_event(RTB_ELEMENT(rtb_win), RTB_EVENT(&ev));
	}
}

#undef LOCK
#undef UNLOCK
@end /* @implementation RutabagaOpenGLView */

@implementation RutabagaOpenGLContext
@end

/**
 * rutabaga interface
 */

static NSCursor *
create_invisible_cursor(void)
{
	NSImage *image = [[NSImage alloc] initWithSize:NSMakeSize(1, 1)];
	NSCursor *cursor = [[NSCursor alloc] initWithImage:image
											   hotSpot:NSZeroPoint];
	[image release];

	return cursor;
}

struct rutabaga *
window_impl_rtb_alloc(void)
{
	struct cocoa_rtb *rtb = calloc(1, sizeof(*rtb));

	if (!NSApp) {
		[NSApplication sharedApplication];
		[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
		[NSApp finishLaunching];
	}


	rtb->invisible_cursor = create_invisible_cursor();

	return (void *) rtb;
}

void
window_impl_rtb_free(struct rutabaga *_rtb)
{
	struct cocoa_rtb *rtb = (void *) _rtb;

	[rtb->invisible_cursor release];

	free(rtb);
}

/**
 * window things
 */

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
		NSWindowStyleMaskTitled
		| NSWindowStyleMaskClosable
		| NSWindowStyleMaskMiniaturizable;

	nstitle = [[NSString alloc]
		initWithBytes:title
			   length:strlen(title)
			 encoding:NSUTF8StringEncoding];

	if (resizable)
		style_mask |= NSWindowStyleMaskResizable;

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
		const struct rtb_window_open_options *opt)
{
	struct cocoa_rtb_window *self;
	RutabagaWindow *cwin;
	RutabagaOpenGLContext *gl_ctx;
	RutabagaOpenGLView *view;
	NSView *parent_view;
	NSSize phy_size;

	self = calloc(1, sizeof(*self));
	if (!self)
		return NULL;

	gl_ctx = NULL;
	uv_mutex_init(&self->lock);

	@autoreleasepool {
		self->view = view = [RutabagaOpenGLView alloc];
		gl_ctx = alloc_gl_ctx();

		view->gl_ctx = gl_ctx;
		self->gl_ctx = gl_ctx;

		[view initWithFrame:NSMakeRect(0, 0, opt->width, opt->height)];

		phy_size = [view
			convertSizeToBacking:NSMakeSize(opt->width, opt->height)];

		self->phy_size.w = phy_size.width;
		self->phy_size.h = phy_size.height;

		self->scale = RTB_MAKE_POINT(
				phy_size.width / (float) opt->width,
				phy_size.height / (float) opt->height);

		self->scale_recip.x = 1.f / self->scale.x;
		self->scale_recip.y = 1.f / self->scale.y;

		self->dpi.x = 96 * self->scale.x;
		self->dpi.y = 96 * self->scale.y;

		[gl_ctx makeCurrentContext];

		if (opt->parent || opt->flags & RTB_WINDOW_OPEN_AS_IF_PARENTED) {
			self->cocoa_win = cwin = NULL;

			// FIXME: move to client code?
			if (opt->parent) {
				parent_view = (NSView *) opt->parent;
				[parent_view addSubview:view];
			}

			[view setHidden:NO];
		} else {
			cwin = alloc_nswindow(opt->width, opt->height, opt->title, 1);
			if (!cwin)
				goto err_alloc_nswindow;

			cwin->rtb_win = self;
			self->cocoa_win = cwin;

			[view setAutoresizingMask:
				NSViewWidthSizable | NSViewHeightSizable];

			[cwin setContentView:view];
			[cwin makeFirstResponder:view];
			[cwin center];

			[NSApp activateIgnoringOtherApps:YES];
		}

		view->rtb_win = self;
	}

	return RTB_WINDOW(self);

err_alloc_nswindow:
	uv_mutex_destroy(&self->lock);
	free(self);
	return NULL;
}

void
window_impl_close(struct rtb_window *rwin)
{
	struct cocoa_rtb_window *self = RTB_WINDOW_AS(rwin, cocoa_rtb_window);

	[NSOpenGLContext clearCurrentContext];

	self->view->rtb_win = NULL;

	if (self->cocoa_win)
		[self->cocoa_win close];

	[self->view removeFromSuperview];
	[self->gl_ctx clearDrawable];
	[self->view release];
	[self->gl_ctx release];

	if (self->cocoa_win)
		[self->cocoa_win release];

	uv_mutex_unlock(&self->lock);
	uv_mutex_destroy(&self->lock);

	free(self);
	return;
}

void
rtb__cocoa_draw_frame(struct cocoa_rtb_window *self, int force)
{
	struct rtb_window *win = RTB_WINDOW(self);

	if ([self->gl_ctx view] != self->view)
		[self->gl_ctx setView:self->view];

	rtb_window_lock(win);

	if (rtb_window_draw(win, force))
		[self->gl_ctx flushBuffer];

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

/**
 * misc rutabaga interfaces
 */

rtb_modkey_t
rtb_get_modkeys(struct rtb_window *win)
{
#define MOD_ACTIVE(cocoa_mod, rtb_mod) \
	((!!(cocoa_modkeys & cocoa_mod)) * rtb_mod)

	NSUInteger cocoa_modkeys = [NSEvent modifierFlags];

	return
		MOD_ACTIVE(NSEventModifierFlagOption,    RTB_KEY_MOD_ALT)
		| MOD_ACTIVE(NSEventModifierFlagShift,   RTB_KEY_MOD_SHIFT)
		| MOD_ACTIVE(NSEventModifierFlagControl, RTB_KEY_MOD_CTRL)
		| MOD_ACTIVE(NSEventModifierFlagCommand, RTB_KEY_MOD_SUPER);

#undef MOD_ACTIVE
}

struct rtb_point
rtb_get_scaling(intptr_t parent_window)
{
	/* scaling is always 1x due to macOS handling this in the translation of
	 * backing store coordinates. */
	return RTB_MAKE_POINT(1.f, 1.f);
}

intptr_t
rtb_window_get_native_handle(struct rtb_window *rwin)
{
	struct cocoa_rtb_window *self = RTB_WINDOW_AS(rwin, cocoa_rtb_window);
	return (intptr_t) self->view;
}
