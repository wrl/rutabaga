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

#include "private/window_impl.h"
#include "cocoa_rtb.h"

@implementation RutabagaWindow
- (BOOL) windowShouldClose: (id) sender
{
	return NO;
}

- (BOOL) canBecomeKeyWindow: (id) sender
{
	return YES;
}
@end

@implementation RutabagaOpenGLContext
@end

@interface RutabagaOpenGLView : NSView
{
@public
	struct cocoa_rtb_window *rtb_win;
}
@end

@implementation RutabagaOpenGLView
- (id) initWithFrame: (NSRect) frame
{
	self = [super initWithFrame:frame];
	[self setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
	return self;
}

- (void) setFrame: (NSRect) frame
{
	[super setFrame:frame];

	if (!rtb_win)
		return;

	rtb_win->w = frame.size.width;
	rtb_win->h = frame.size.height;

	[rtb_win->gl_ctx update];
	[rtb_win->gl_ctx makeCurrentContext];

	rtb_window_reinit(RTB_WINDOW(rtb_win));

	[super setNeedsDisplay:YES];
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
	[fmt release];

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

	return RTB_WINDOW(self);
}

void
window_impl_close(struct rtb_window *rwin)
{
	return;
}
