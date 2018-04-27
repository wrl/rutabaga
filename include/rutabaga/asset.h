/**
 * rutabaga: an OpenGL widget toolkit
 * Copyright (c) 2013-2017 William Light.
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

#pragma once

#include <unistd.h>

#define RTB_ASSET(x) RTB_UPCAST(x, rtb_asset)

#define RTB_ASSET_IS_LOADED(a) ((a)->loaded)
#define RTB_ASSET_SIZE(a) ((a)->buffer.size)
#define RTB_ASSET_DATA(a) ((a)->buffer.data)

typedef enum {
	RTB_ASSET_EXTERNAL = 0,
	RTB_ASSET_EMBEDDED = 1
} rtb_asset_location_t;

typedef enum {
	RTB_ASSET_UNCOMPRESSED  = 0,
	RTB_ASSET_COMPRESSED_XZ = 1
} rtb_asset_compression_t;

struct rtb_asset {
	/* public *********************************/
	rtb_asset_location_t location;
	rtb_asset_compression_t compression;

	/**
	 * location
	 */
	struct {
		char *path;
	} external;

	/**
	 * compression
	 */
	union {
		struct {
			size_t deflated_size;
		} xz;
	} compressor;

	/* private ********************************/
	int loaded;

	struct {
		int allocated;
		size_t size;
		const void *data;
	} buffer;
};

int rtb_asset_load(struct rtb_asset *);
void rtb_asset_free(struct rtb_asset *);
