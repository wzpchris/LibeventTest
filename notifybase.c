#include "./include/event2/event.h"
#include <stdio.h>
#include <unistd.h>
#include "./include/event2/thread.h"

#include <pthread.h>

struct event_base *base = NULL;
void pipe_cb(int fd, short events, void *arg)
{
	printf("in the cmd_cb\n");
}

void timeout_cb(int fd, short events, void *arg)
{
	printf("int the timeout_cb\n");
}

void* thread_fn(void *arg)
{
	char ch;
	printf("thread input a char:");
	scanf("%c", &ch); 
	struct event *ev = event_new(base, -1, EV_TIMEOUT | EV_PERSIST, timeout_cb, NULL);
	struct timeval tv = {2, 0};
	event_add(ev, &tv);
}

int main(int argc, char** argv)
{
	if(argc >=2 && argv[1][0] == 'y')
		evthread_use_pthreads();	
	base = event_base_new();
	evthread_make_base_notifiable(base);

	int pipe_fd[2];
	pipe(pipe_fd);
	struct event *ev = event_new(base, pipe_fd[0], EV_READ | EV_PERSIST, pipe_cb, NULL);
	event_add(ev, NULL);
	pthread_t thid;
	pthread_create(&thid, NULL, thread_fn, NULL);

	event_base_dispatch(base);
	return 0;
}
