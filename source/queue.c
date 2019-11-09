#include "queue.h"

#define EVENT_INIT_SIZE 1000

void queue_add_event(queue_t *qt, socket_t *s)
{
    EV_SET(&qt->events[qt->size], s->fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, s);
    ++qt->size;
    EV_SET(&qt->events[qt->size], s->fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, s);
    ++qt->size;
}

void queue_cleanup(queue_t *qt)
{
    free(qt->events);
    free(qt->changes);
    free(qt);
}

queue_t *queue_init()
{
    queue_t *qt = (queue_t *)malloc(sizeof(queue_t));
    qt->fd = kqueue();
    qt->size = 0;
    qt->position = 0;
    qt->events = (struct kevent *)calloc(EVENT_INIT_SIZE, sizeof(struct kevent));
    qt->changes = (struct kevent *)calloc(EVENT_INIT_SIZE, sizeof(struct kevent));
    return qt;
}
