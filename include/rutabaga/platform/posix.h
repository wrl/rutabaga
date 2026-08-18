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

#include <pthread.h>

/**
 * mutex
 */

typedef struct {
	pthread_mutex_t inner;
} rtb_mutex_t;

static inline int
rtb_mutex_init(rtb_mutex_t *mutex)
{
#if defined(PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP)
	mutex->inner = (pthread_mutex_t) PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP;
#elif defined(PTHREAD_ERRORCHECK_MUTEX_INITIALIZER)
	mutex->inner = (pthread_mutex_t) PTHREAD_ERRORCHECK_MUTEX_INITIALIZER;
#else
# error "where is my pthread error check mutex initializer???"
#endif

	return 0;
}

static inline int
rtb_mutex_lock(rtb_mutex_t *mutex)
{
	return pthread_mutex_lock(&mutex->inner);
}

static inline int
rtb_mutex_unlock(rtb_mutex_t *mutex)
{
	return pthread_mutex_unlock(&mutex->inner);
}

static inline int
rtb_mutex_destroy(rtb_mutex_t *mutex)
{
	return pthread_mutex_destroy(&mutex->inner);
}
