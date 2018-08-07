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

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include <rutabaga/asset.h>
#include "rtb_private/util.h"

/**
 * loaders
 */

typedef int (*loader_func)(struct rtb_asset *asset);

/**
 * uncompressed
 */

static int
load_ext(struct rtb_asset *asset)
{
	FILE *f;
	int size;
	void *data;

	if (!(f = fopen(asset->external.path, "rb"))) {
		perror("rtb_asset: load_ext(): ");
		goto err_fopen;
	}

	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);

	asset->buffer.size = size;
	asset->buffer.data = data = malloc(size);
	asset->buffer.allocated = 1;

	fread(data, 1, size, f);

	fclose(f);
	return 0;

err_fopen:
	return -1;
}

static int
load_emb(struct rtb_asset *asset)
{
	return 0;
}

/**
 * xz
 */

static int
load_emb_xz(struct rtb_asset *asset)
{
	return -1;
}

static int
load_ext_xz(struct rtb_asset *asset)
{
	return -1;
}

static loader_func loaders[] = {
	[RTB_ASSET_EXTERNAL | RTB_ASSET_UNCOMPRESSED] = load_ext,
	[RTB_ASSET_EMBEDDED | RTB_ASSET_UNCOMPRESSED] = load_emb,
	[RTB_ASSET_EXTERNAL | (RTB_ASSET_COMPRESSED_XZ << 1)] = load_ext_xz,
	[RTB_ASSET_EMBEDDED | (RTB_ASSET_COMPRESSED_XZ << 1)] = load_emb_xz
};

/**
 * public API
 */

int
rtb_asset_load(struct rtb_asset *asset)
{
	unsigned int loader;

	loader = asset->location | (asset->compression << 1);

	if (loader > ARRAY_LENGTH(loaders))
		return -1;

	asset->buffer.size      = 0;
	asset->buffer.data      = NULL;
	asset->buffer.allocated = 0;
	asset->loaded = 0;

	if (loaders[loader](asset))
		return -1;

	asset->loaded = 1;
	return 0;
}

void
rtb_asset_free(struct rtb_asset *asset)
{
	assert(asset->loaded && asset->buffer.data);

	if (asset->buffer.allocated)
		free((void *) asset->buffer.data);

	asset->buffer.size      = 0;
	asset->buffer.data      = NULL;
	asset->buffer.allocated = 0;
	asset->loaded = 0;
}
