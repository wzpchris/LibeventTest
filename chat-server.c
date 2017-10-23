#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "include/event2/event.h"
#include "include/event2/event_struct.h"
#include "include/event2/bufferevent.h"
#include "include/event2/buffer.h"
#include "include/event2/util.h"
#include "include/sys/queue.h"

struct client {
	int fd;  //the client socket.
	struct bufferevent *buf_ev; //the bufferevent for this client
	TAILQ_ENTRY(client) entries; //the tail queue.
};

TAILQ_HEAD(client_tailq, client);
struct client_tailq client_tailq_head;
struct event_base *evbase;

void on_accept(int fd, short ev, void *arg);
void buffered_on_read(struct bufferevent *bev, void *arg);
void buffered_on_error(struct bufferevent *bev, short event, void *arg);

int main(int argc, char **argv)
{
	int listen_fd;
	struct sockaddr_in listen_addr;
	struct event ev_accept;
	/* Initialize libevent */
	evbase = event_base_new();

	/* Initialize the tailq. */
	TAILQ_INIT(&client_tailq_head);
	
	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(listen_fd < 0)
	{
		printf("listen failed\n");
		return 1;
	}

	evutil_make_listen_socket_reuseable(listen_fd);
	
	memset(&listen_addr, 0, sizeof(listen_addr));
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_addr.s_addr = INADDR_ANY;
	listen_addr.sin_port = htons(9999);
	
	if(bind(listen_fd, (struct sockaddr*)&listen_addr, sizeof(listen_addr)) < 0)
	{
		printf("bind failed\n");
		return 1;
	}

	if(listen(listen_fd, 10) < 0)
	{
		printf("listen failed\n");
		return 1;
	}
	
	if(evutil_make_socket_nonblocking(listen_fd) < 0)
	{
		printf("failed to set server socket to non-blocking\n");
		return 1;
	}

	event_assign(&ev_accept, evbase, listen_fd, EV_READ | EV_PERSIST, on_accept, NULL);
	event_add(&ev_accept, NULL);
	
	event_base_dispatch(evbase); 
	return 0;
}

void on_accept(int fd, short ev, void *arg)
{
	int client_fd; 
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	struct client *pclient;
	client_fd = accept(fd, (struct sockaddr*)&client_addr, &client_len);
	if(client_fd < 0)
	{
		printf("accept failed\n");
		return;
	}
	
	if(evutil_make_socket_nonblocking(client_fd) < 0)
	{	
		printf("failed to set client socket non-blocking\n");
		return;
	}

	pclient = calloc(1, sizeof(*pclient));
	if(pclient == NULL)
	{
		printf("malloc failed\n");
		return;
	}
	
	pclient->fd = client_fd;
	pclient->buf_ev = bufferevent_socket_new(evbase, client_fd, 0);
	bufferevent_setcb(pclient->buf_ev, buffered_on_read, NULL, buffered_on_error, pclient);

	bufferevent_enable(pclient->buf_ev, EV_READ);
	
	/* add the new client to the tailq */
	TAILQ_INSERT_TAIL(&client_tailq_head, pclient, entries);

	printf("accepted connection from %s, fd=%d\n", inet_ntoa(client_addr.sin_addr),client_fd);
}

void buffered_on_read(struct bufferevent *bev, void *arg)
{
	struct client *this_client = arg;
	struct client *client;
	uint8_t data[8192];
	size_t n;
	for(;;)
	{
		n = bufferevent_read(bev, data, sizeof(data));
		if(n <= 0)
		{
			break;
		}

		data[n-1] = '\0';

		sprintf(data + strlen(data), " :%d", this_client->fd); 
		TAILQ_FOREACH(client, &client_tailq_head, entries) 
		{
			if(client != this_client)
			{
				bufferevent_write(client->buf_ev, data, strlen(data));
			}
		}
	}
}
void buffered_on_error(struct bufferevent *bev, short event, void *arg)
{
	if(event & BEV_EVENT_EOF)
	{
		printf("connection closed\n");
	}else if(event & BEV_EVENT_ERROR)
	{
		printf("some other error\n");
	}

	struct client *pclient = (struct client*)arg;
	bufferevent_free(pclient->buf_ev);
	free(pclient);	
	
	bufferevent_free(bev);
}
