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

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/queue.h>
#include <string.h>
#include <pthread.h>

#include <jack/jack.h>

#include <rutabaga/rutabaga.h>
#include <rutabaga/container.h>
#include <rutabaga/window.h>
#include <rutabaga/event.h>
#include <rutabaga/layout.h>

#include <rutabaga/widgets/patchbay.h>

#define NO_ALLOC 0
#define ALLOCATE 1

/**
 * our data structures
 */

struct cabbage_patch_state {
	struct rutabaga *rtb;
	struct rtb_window *win;
	pthread_mutex_t connection_from_gui;

	struct rtb_patchbay cp;

	jack_client_t *jc;

	struct {
		RTB_INHERIT(rtb_patchbay_node);
		struct jack_client *client;
	} system_in, system_out;
} state;

struct jack_client {
	RTB_INHERIT(rtb_patchbay_node);

	int nports;
	int physical;

	TAILQ_ENTRY(jack_client) client;
	TAILQ_HEAD(ports, jack_client_port) ports;

	char name[];
};

struct jack_client_port {
	RTB_INHERIT(rtb_patchbay_port);

	TAILQ_ENTRY(jack_client_port) port;
	char name[];
};

TAILQ_HEAD(clients, jack_client) clients;


static struct jack_client *
client_alloc(const char *name, int len, int physical)
{
	struct jack_client *c;

	c = malloc(sizeof(*c) + len + 1);
	TAILQ_INIT(&c->ports);

	c->nports = 0;
	strncpy(c->name, name, len);
	c->name[len] = '\0';

	c->physical = physical;

	if (physical) {
		state.system_in.client = c;
		state.system_out.client = c;
	} else {
		rtb_patchbay_node_init(RTB_PATCHBAY_NODE(c));
		rtb_patchbay_node_set_name(RTB_PATCHBAY_NODE(c), name);

		c->x = 100.f;
		c->y = 100.f;

		rtb_elem_add_child(RTB_ELEMENT(&state.cp),
				RTB_ELEMENT(c), RTB_ADD_TAIL);
	}

	return c;
}

static struct jack_client *
get_client(const char *name, int physical)
{
	struct jack_client *iter;

	TAILQ_FOREACH(iter, &clients, client) {
		if (!strcmp(iter->name, name))
			return iter;
	}

	return NULL;
}

static struct jack_client *
get_client_or_alloc(const char *name, int len, int physical)
{
	struct jack_client *c;

	if ((c = get_client(name, physical)))
		return c;

	c = client_alloc(name, len, physical);
	TAILQ_INSERT_TAIL(&clients, c, client);
	return c;
}

static struct jack_client_port *
get_client_port(struct jack_client *client, const char *port_name)
{
	struct jack_client_port *iter;

	TAILQ_FOREACH(iter, &client->ports, port) {
		if (!strcmp(iter->name, port_name))
			return iter;
	}

	return NULL;
}

/**
 * utility functions
 */

static void
jackport_to_rtbport(jack_port_t *jack_port, struct jack_client **client,
		struct jack_client_port **port, int alloc)
{
	const char *port_name = jack_port_name(jack_port);
	int port_flags = jack_port_flags(jack_port);
	char *client_name;
	size_t client_len;

	*client = NULL;
	*port   = NULL;

	client_len = strchr(port_name, ':') - port_name;
	client_name = alloca(client_len + 1);
	client_name[client_len] = '\0';

	strncpy(client_name, port_name, client_len);
	port_name += client_len + 1;

	if (port_flags & JackPortIsPhysical)
		*client = state.system_in.client;
	else if (alloc)
		*client = get_client_or_alloc(client_name, client_len, 0);
	else
		*client = get_client(client_name, 0);

	if (*client)
		*port = get_client_port(*client, port_name);
}

static void
rtbport_to_jackport(char *dst, size_t nbytes,
		struct jack_client *c, struct jack_client_port *p)
{
	if (c == (void *) &state.system_in ||
	    c == (void *) &state.system_out)
		c = state.system_in.client;

	snprintf(dst, nbytes, "%s:%s", c->name, p->name);
}

/**
 * jack client list shit
 */

static void
client_add_port(struct jack_client *c, const char *name, int len, int flags,
		rtb_child_add_loc_t location)
{
	struct jack_client_port *p;
	rtb_patchbay_port_type_t type;
	struct rtb_patchbay_node *node;

	p = malloc(sizeof(*p) + len + 1);
	strncpy(p->name, name, len);
	p->name[len] = '\0';

	if (flags & JackPortIsInput)
		type = PORT_TYPE_INPUT;
	else
		type = PORT_TYPE_OUTPUT;

	if (c->physical) {
		if (type == PORT_TYPE_INPUT)
			node = RTB_PATCHBAY_NODE(&state.system_in);
		else
			node = RTB_PATCHBAY_NODE(&state.system_out);
	} else
		node = RTB_PATCHBAY_NODE(c);

	rtb_patchbay_port_init(RTB_PATCHBAY_PORT(p), node, name, type, location);

	TAILQ_INSERT_TAIL(&c->ports, p, port);
}

static struct jack_client_port *
client_get_port(struct jack_client *c, const char *name, int len)
{
	struct jack_client_port *iter;

	TAILQ_FOREACH(iter, &c->ports, port) {
		if (!strncmp(iter->name, name, len))
			return iter;
	}

	return NULL;
}

static void
free_port(struct jack_client *client, struct jack_client_port *port)
{
	TAILQ_REMOVE(&client->ports, port,port);
	rtb_patchbay_port_fini(RTB_PATCHBAY_PORT(port));
	free(port);
}

static void
free_client(struct jack_client *client)
{
	struct jack_client_port *port;

	TAILQ_REMOVE(&clients, client, client);

	while ((port = TAILQ_FIRST(&client->ports)))
		free_port(client, port);

	if (!client->physical)
		rtb_patchbay_node_fini(RTB_PATCHBAY_NODE(client));

	free(client);
}

static void
free_client_tailq(void)
{
	struct jack_client *node;

	while ((node = TAILQ_FIRST(&clients)))
		free_client(node);
}

/**
 * jack shit
 */

static void
connect_jack_port(jack_client_t *jc, struct jack_client_port *clp,
		const char *other_port)
{
	struct jack_client_port *other_clp;
	struct jack_client *client;
	const char *port_name;
	jack_port_t *port;
	char *client_name;
	int len, flags;

	client_name = alloca(jack_port_name_size());

	port = jack_port_by_name(jc, other_port);
	flags = jack_port_flags(port);

	len = strchr(other_port, ':') - other_port;
	strncpy(client_name, other_port, len);
	client_name[len] = '\0';

	client = get_client(client_name, !!(flags & JackPortIsPhysical));

	port_name = other_port + len + 1;
	len = strlen(port_name);
	other_clp = client_get_port(client, port_name, len);

	rtb_patchbay_connect_ports(&state.cp,
			RTB_PATCHBAY_PORT(clp),
			RTB_PATCHBAY_PORT(other_clp));
}

static void
list_ports(jack_client_t *jc)
{
	const char **ports, **cxns, *port_name;
	struct jack_client *client;
	struct jack_client_port *clp;
	char *client_name;
	jack_port_t *port;
	int i, j, len, flags;

	ports = jack_get_ports(jc, NULL, NULL, 0);

	if (!ports)
		return;

	client_name = alloca(jack_port_name_size());

	for (i = 0; ports[i]; i++) {
		port = jack_port_by_name(jc, ports[i]);
		flags = jack_port_flags(port);

		len = strchr(ports[i], ':') - ports[i];
		strncpy(client_name, ports[i], len);
		client_name[len] = '\0';

		client = get_client_or_alloc(client_name, len,
				!!(flags & JackPortIsPhysical));

		port_name = ports[i] + len + 1;
		len = strlen(port_name);

		client_add_port(client, port_name, len, flags, RTB_ADD_TAIL);
	}

	for (i = 0; ports[i]; i++) {
		port = jack_port_by_name(jc, ports[i]);
		flags = jack_port_flags(port);
		cxns = jack_port_get_all_connections(jc, port);

		if (!cxns)
			continue;

		len = strchr(ports[i], ':') - ports[i];
		strncpy(client_name, ports[i], len);
		client_name[len] = '\0';

		client = get_client(client_name, !!(flags & JackPortIsPhysical));

		port_name = ports[i] + len + 1;
		len = strlen(port_name);
		clp = client_get_port(client, port_name, len);

		for (j = 0; cxns[j]; j++)
			if (*cxns[j])
				connect_jack_port(jc, clp, cxns[j]);

		jack_free(cxns);
	}

	jack_free(ports);
}

static int
init_jack(void)
{
	state.jc = jack_client_open("cabbage-patch", JackNoStartServer, 0);
	if (!state.jc) {
		printf(" [-] couldn't initialize JACK\n");
		return -1;
	}

	printf(" [+] jack client opened\n");
	return 0;
}

static void
fini_jack(void)
{
	jack_deactivate(state.jc);
	jack_client_close(state.jc);
	printf(" [+] jack client closed\n");
}

/**
 * jack callbacks
 */

static void
client_registration(const char *client_name, int registered, void *ctx)
{
	struct jack_client *client;

	if (!registered) {
		client = get_client(client_name, 0);

		if (client) {
			rtb_window_lock(state.win);
			free_client(client);
			rtb_window_unlock(state.win);
		}
	}
}

static void
port_registration(jack_port_id_t port_id, int registered, void *ctx)
{
	struct jack_client *client;
	struct jack_client_port *port;
	jack_port_t *jack_port = jack_port_by_id(state.jc, port_id);
	const char *port_name = jack_port_name(jack_port);

	port_name = strchr(port_name, ':') + 1;

	rtb_window_lock(state.win);
	jackport_to_rtbport(jack_port, &client, &port, ALLOCATE);

	if (registered) {
		client_add_port(client, port_name, strlen(port_name),
				jack_port_flags(jack_port), RTB_ADD_TAIL);
	} else
		if (port)
			free_port(client, port);

	rtb_window_unlock(state.win);
}

static void
port_connection(jack_port_id_t a_id, jack_port_id_t b_id, int cxn, void *ctx)
{
	jack_port_t *a = jack_port_by_id(state.jc, a_id);
	jack_port_t *b = jack_port_by_id(state.jc, b_id);
	struct jack_client *client_a, *client_b;
	struct jack_client_port *port_a, *port_b;

	if (pthread_mutex_trylock(&state.connection_from_gui))
		return;
	pthread_mutex_unlock(&state.connection_from_gui);

	jackport_to_rtbport(a, &client_a, &port_a, NO_ALLOC);
	jackport_to_rtbport(b, &client_b, &port_b, NO_ALLOC);

	if (!client_a || !port_a || !client_b || !port_b)
		return;

	rtb_window_lock(state.win);

	if (cxn)
		rtb_patchbay_connect_ports(&state.cp,
				RTB_PATCHBAY_PORT(port_a),
				RTB_PATCHBAY_PORT(port_b));
	else
		rtb_patchbay_disconnect_ports(&state.cp,
				RTB_PATCHBAY_PORT(port_a),
				RTB_PATCHBAY_PORT(port_b));

	rtb_window_unlock(state.win);
}

/**
 * rutabaga shit
 */

static int
connection(struct rtb_element *obj, const struct rtb_event *_ev, void *ctx)
{
	const struct rtb_patchbay_event_connect *ev = (void *) _ev;
	char *from, *to;
	int len;

	len = jack_port_name_size();

	from = alloca(len);
	to   = alloca(len);

	/* since we've put the rtb_patchbay structures at the head of
	 * struct jack_client and struct jack_client_port, we can just
	 * type-pun them to get our local data structures. */

	rtbport_to_jackport(from, len,
			(struct jack_client *) ev->from.node,
			(struct jack_client_port *) ev->from.port);
	rtbport_to_jackport(to, len,
			(struct jack_client *) ev->to.node,
			(struct jack_client_port *) ev->to.port);

	/* this is super frustrating. jack_connect() immediately calls our
	 * JackPortConnectCallback in the client thread, which tries to lock
	 * the window, but can't because we've already got it in the main
	 * thread here. so we use state.connection_from_gui to tell the port
	 * connect callback that we've got it handled. */
	pthread_mutex_lock(&state.connection_from_gui);

	if (!jack_connect(state.jc, from, to))
		rtb_patchbay_connect_ports((struct rtb_patchbay *) obj,
				ev->from.port, ev->to.port);
	else
		printf(" !! couldn't connect %s to %s\n", from, to);

	pthread_mutex_unlock(&state.connection_from_gui);

	return 1;
}

static int
disconnection(struct rtb_element *obj, const struct rtb_event *_ev, void *ctx)
{
	const struct rtb_patchbay_event_disconnect *ev = (void *) _ev;
	char *from, *to;
	int len;

	len = jack_port_name_size();

	from = alloca(len);
	to   = alloca(len);

	rtbport_to_jackport(from, len,
			(struct jack_client *) ev->from.node,
			(struct jack_client_port *) ev->from.port);
	rtbport_to_jackport(to, len,
			(struct jack_client *) ev->to.node,
			(struct jack_client_port *) ev->to.port);

	pthread_mutex_lock(&state.connection_from_gui);
	if (!jack_disconnect(state.jc, from, to))
		rtb_patchbay_free_patch((struct rtb_patchbay *) obj, ev->patch);
	else
		printf(" !! couldn't disconnect %s from %s\n", from, to);
	pthread_mutex_unlock(&state.connection_from_gui);

	return 1;
}

static void
init_patchbay(struct rtb_element *parent)
{
	rtb_patchbay_init(&state.cp);
	rtb_container_add(parent, RTB_ELEMENT(&state.cp));

	rtb_elem_set_size_cb(RTB_ELEMENT(&state.cp), rtb_size_fill);

	rtb_register_handler(RTB_ELEMENT(&state.cp),
			RTB_PATCHBAY_CONNECT, connection, NULL);
	rtb_register_handler(RTB_ELEMENT(&state.cp),
			RTB_PATCHBAY_DISCONNECT, disconnection, NULL);
}

typedef int gucci;

gucci
main(int argc, char **argv)
{
	if (init_jack() < 0)
		return EXIT_FAILURE;

	pthread_mutex_init(&state.connection_from_gui, NULL);

	state.rtb = rtb_new();
	assert(state.rtb);
	state.win = rtb_window_open(state.rtb, 1440, 768, "cabbage patch");
	assert(state.win);

	state.win->outer_pad.x = 0.f;
	state.win->outer_pad.y = 8.f;

	TAILQ_INIT(&clients);

	init_patchbay(RTB_ELEMENT(state.win));

	rtb_patchbay_node_init(RTB_PATCHBAY_NODE(&state.system_in));
	rtb_patchbay_node_set_name(RTB_PATCHBAY_NODE(&state.system_in), "system");
	rtb_patchbay_node_init(RTB_PATCHBAY_NODE(&state.system_out));
	rtb_patchbay_node_set_name(RTB_PATCHBAY_NODE(&state.system_out), "system");

	rtb_elem_add_child(RTB_ELEMENT(&state.cp),
			RTB_ELEMENT(&state.system_in), RTB_ADD_TAIL);
	rtb_elem_add_child(RTB_ELEMENT(&state.cp),
			RTB_ELEMENT(&state.system_out), RTB_ADD_TAIL);

	list_ports(state.jc);

	jack_set_client_registration_callback(state.jc,
			client_registration, NULL);
	jack_set_port_registration_callback(state.jc,
			port_registration, NULL);
	jack_set_port_connect_callback(state.jc,
			port_connection, NULL);
	jack_activate(state.jc);

	rtb_event_loop(state.rtb);

	rtb_window_lock(state.win);
	rtb_patchbay_node_fini(RTB_PATCHBAY_NODE(&state.system_out));
	rtb_patchbay_node_fini(RTB_PATCHBAY_NODE(&state.system_in));

	free_client_tailq();
	fini_jack();

	rtb_patchbay_fini(&state.cp);
	rtb_window_close(state.rtb->win);
	rtb_free(state.rtb);

	return EXIT_SUCCESS;
}
