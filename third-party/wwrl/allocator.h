/**
 * allocator.h -- not even going to explain this one
 * version 1.0, May 15th, 2013
 *
 * Copyright (C) 2013 William Light <wrl@illest.net>
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source distribution.
 */

#ifndef _WWRL_ALLOCATOR_H_
#define _WWRL_ALLOCATOR_H_

struct wwrl_allocator {
	void *(*malloc)(size_t size);
	void (*free)(void *ptr);
	void *(*calloc)(size_t nmemb, size_t size);
	void *(*realloc)(void *ptr, size_t size);
};

#endif /* ndef _WWRL_ALLOCATOR_H_ */
