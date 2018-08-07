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

#include <rutabaga/platform.h>

void
rtb_copy_to_clipboard(struct rtb_window *rwin, const rtb_utf8_t *buf,
		size_t nbytes)
{
	@autoreleasepool {
		NSPasteboard *pb = [NSPasteboard generalPasteboard];
		NSString *format = NSPasteboardTypeString;

		[pb declareTypes:[NSArray arrayWithObject:format]
				   owner:nil];

		[pb setString:[NSString stringWithUTF8String:buf]
			  forType:format];
	}
}

ssize_t
rtb_paste_from_clipboard(struct rtb_window *rwin, rtb_utf8_t **buf)
{
	ssize_t nbytes;

	@autoreleasepool {
		NSPasteboard *pb = [NSPasteboard generalPasteboard];
		NSString *res, *format;

		format = NSPasteboardTypeString;

		if (![[pb availableTypeFromArray:[NSArray arrayWithObject:format]]
						 isEqualToString:format])
			return 0;

		res = [pb stringForType:format];
		if (!res)
			return -1;

		nbytes = [res lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
		*buf = strndup([res UTF8String], nbytes);
	}

	return nbytes;
}
