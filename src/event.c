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

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "rutabaga/rutabaga.h"
#include "rutabaga/event.h"
#include "rutabaga/object.h"

static struct rtb_event_handler *find_handler_for(
		struct rtb_event_handler *handlers, rtb_ev_type_t type)
{
	int i;

	for (i = 0; i < RTB_EVENT_HANDLERS_PER_OBJECT &&
			handlers[i].callback.cb; i++)
		if (handlers[i].type == type)
			return &handlers[i];

	return NULL;
}

int rtb_handle(rtb_obj_t *victim, const rtb_ev_t *ev)
{
	struct rtb_event_handler *h;

	if (!(h = find_handler_for(victim->handlers, ev->type)))
		return 0;

	h->callback.cb(victim, ev, h->callback.ctx);
	return 1;
}

rtb_obj_t *rtb_dispatch_raw(rtb_obj_t *victim, rtb_ev_t *event)
{
	while (!victim->event_cb(victim, event) && victim->parent)
		victim = victim->parent;

	return victim;
}

rtb_obj_t *rtb_dispatch_simple(rtb_obj_t *victim, rtb_ev_type_t type)
{
	rtb_ev_t event = {
		.type = type
	};

	return rtb_dispatch_raw(victim, &event);
}

int rtb_attach(rtb_obj_t *victim, rtb_ev_type_t type, rtb_event_cb_t cb,
		void *user_arg)
{
	rtb_ev_handler_t *h;
	int i;

	assert(victim);
	assert(cb);

	for (i = 0; i < RTB_EVENT_HANDLERS_PER_OBJECT &&
			victim->handlers[i].callback.cb; i++) {
		/* if there's already a handler defined for this event type,
		 * replace it. */
		if (victim->handlers[i].type == type)
			break;
	}

	if (i == RTB_EVENT_HANDLERS_PER_OBJECT)
		return -1;

	h = &victim->handlers[i];
	h->type = type;
	h->callback.cb = cb;
	h->callback.ctx = user_arg;

	return 0;
}

void rtb_detach(rtb_obj_t *victim, rtb_ev_type_t type)
{
	int i, found;

	assert(victim);

	found = -1;

	for (i = 0; i < RTB_EVENT_HANDLERS_PER_OBJECT &&
			victim->handlers[i].callback.cb; i++) {
		if (victim->handlers[i].type == type)
			found = i;
	}

	if (found < 0)
		return;

	/* if the handler was in the middle of the handlers list, move the
	 * handlers after it up. */
	if (i > found)
		memmove(&victim->handlers[i], &victim->handlers[i + 1],
				(i - found) * sizeof(victim->handlers[i]));

	/* clear the last element of the list */
	memset(&victim->handlers[i], 0, sizeof(victim->handlers[i]));

	return;
}
