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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "private/targa.h"

int targa_load(targa_t *img, const char *path)
{
	targa_header_t *head;
	off_t offset;
	ssize_t len;
	int fd;

	assert(img);
	assert(path);

	memset(img, 0, sizeof(targa_t));
	head = &img->info;

	if ((fd = open(path, O_RDONLY)) < 0)
		return 1;

	if (read(fd, head, sizeof(targa_header_t)) != sizeof(targa_header_t))
		goto err;

	offset = head->id_length + (head->cmap_len * head->cmap_bpp);
	if (lseek(fd, offset, SEEK_CUR) != offset + sizeof(targa_header_t))
		goto err;

	len = head->width * head->height * (head->bpp / 8);
	img->data = calloc(sizeof(uint8_t), len);

	if (read(fd, img->data, len) < 0)
		goto read_err;

	close(fd);
	return 0;

read_err:
	free(img->data);
err:
	perror("targa_load");
	close(fd);
	return -1;
}

void targa_data_free(targa_t *img)
{
	assert(img);

	if (img->data)
		free(img->data);
}
