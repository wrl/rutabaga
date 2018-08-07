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
#include <stdio.h>
#include <string.h>

#include <rutabaga/rutabaga.h>
#include <rutabaga/event.h>
#include <rutabaga/element.h>

static const struct rtb_event_handler *
find_handler_for(struct rtb_element *elem, rtb_ev_type_t type)
{
	struct rtb_event_handler *handlers;
	int i, size;

	handlers = elem->handlers.data;
	size = elem->handlers.size;

	for (i = 0; i < size; i++)
		if (handlers[i].type == type)
			return &handlers[i];

	return NULL;
}

static int
replace_handler(struct rtb_element *elem, struct rtb_event_handler *with)
{
	struct rtb_event_handler *handlers;
	int i, size;

	handlers = elem->handlers.data;
	size = elem->handlers.size;

	for (i = 0; i < size; i++) {
		/* if there's already a handler defined for this event type,
		 * replace it. */
		if (handlers[i].type == with->type) {
			handlers[i] = *with;
			return 1;
		}
	}

	return 0;
}

/**
 * public API
 */

int
rtb_handle(struct rtb_element *target, const struct rtb_event *event)
{
	const struct rtb_event_handler *h;

	if (!(h = find_handler_for(target, event->type)))
		return 0;

	h->callback.cb(target, event, h->callback.ctx);
	return 1;
}

struct rtb_element *
rtb_dispatch_raw(struct rtb_element *target, struct rtb_event *event)
{
	while (!rtb_elem_deliver_event(target, event) && target->parent)
		target = target->parent;

	return target;
}

struct rtb_element *
rtb_dispatch_simple(struct rtb_element *target, rtb_ev_type_t type)
{
	struct rtb_event event = {
		.type = type
	};

	return rtb_dispatch_raw(target, &event);
}

int
rtb_register_handler(struct rtb_element *target, rtb_ev_type_t type,
		rtb_event_cb_t cb, void *user_arg)
{
	struct rtb_event_handler handler = {
		.type         = type,
		.callback.cb  = cb,
		.callback.ctx = user_arg
	};

	assert(target);
	assert(cb);

	if (!replace_handler(target, &handler))
		VECTOR_PUSH_BACK(&target->handlers, &handler);

	return 0;
}

void
rtb_unregister_handler(struct rtb_element *target, rtb_ev_type_t type)
{
	const struct rtb_event_handler *handlers;
	int i, size;

	assert(target);

	handlers = target->handlers.data;
	size = target->handlers.size;

	for (i = 0; i < size; i++) {
		if (handlers[i].type == type) {
			VECTOR_ERASE(&target->handlers, i);
			return;
		}
	}
}
