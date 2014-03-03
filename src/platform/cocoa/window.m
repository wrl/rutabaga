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
- (id) initWithContentRect: (NSRect) contentRect
				 styleMask: (unsigned int) aStyle
				   backing: (NSBackingStoreType) bufferingType
					 defer: (BOOL) deferCreation
{
	self =
		[super initWithContentRect:contentRect
						 styleMask:aStyle
						   backing:bufferingType
							 defer:deferCreation];

	[self setAcceptsMouseMovedEvents:YES];
	[self setLevel:CGShieldingWindowLevel() + 1];

	return self;
}

- (BOOL) windowShouldClose: (id) sender
{
	return NO;
}

- (BOOL) canBecomeKeyWindow: (id) sender
{
	return NO;
}
@end

@implementation RutabagaOpenGLView
- (id) initWithFrame: (NSRect) frame
		   colorBits: (int) colorBits
		   depthBits: (int) depthBits
{
	NSOpenGLPixelFormat *format;
	NSOpenGLPixelFormatAttribute attribs[] = {
		NSOpenGLPFAOpenGLProfile,
		NSOpenGLProfileVersion3_2Core,
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAAccelerated,
		NSOpenGLPFAColorSize,
		colorBits,
		NSOpenGLPFADepthSize,
		depthBits,
		0
	};

	format = [[NSOpenGLPixelFormat alloc] initWithAttributes:attribs];
	if (!format)
		return nil;

	self = [super initWithFrame:frame pixelFormat:format];
	[format release];

	if (!self)
		return nil;

	[[self openGLContext] makeCurrentContext];
	[self reshape];

	return self;
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

struct rtb_window *
window_impl_open(struct rutabaga *rtb,
		int w, int h, const char *title, intptr_t parent)
{
	struct cocoa_rtb_window *self;

	RutabagaOpenGLView *glview;
	RutabagaWindow *cwin;
	NSString *nstitle;

	self = calloc(1, sizeof(*self));
	if (!self)
		return NULL;

	@autoreleasepool {
		nstitle =
			[[NSString alloc]
			initWithBytes:title
				   length:strlen(title)
				 encoding:NSUTF8StringEncoding];

		cwin = [[RutabagaWindow new] retain];
		[cwin setContentSize:NSMakeSize(w, h)];
		[cwin setTitle:nstitle];
		cwin->rtb_win = self;

		glview = [[RutabagaOpenGLView new]
					initWithFrame:NSMakeRect(0, 0, w, h)
						colorBits:24
						depthBits:24];

		[cwin setContentView:glview];
		[cwin makeFirstResponder:glview];

		[NSApp activateIgnoringOtherApps:YES];
		[cwin makeKeyAndOrderFront:cwin];
	}

	get_dpi(&self->dpi.x, &self->dpi.y);

	self->cocoa_win = cwin;
	self->gl_view = glview;

	return RTB_WINDOW(self);
}

void
window_impl_close(struct rtb_window *rwin)
{
	return;
}
