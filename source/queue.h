#ifndef QUEUE_H
#define QUEUE_H

#include <sys/event.h>
#include "socket.h"

typedef struct
{
    int fd, size, position;
    struct kevent *events;
    struct kevent *changes;
} queue_t;

void queue_add_event(queue_t *, socket_t *);
void queue_cleanup(queue_t *);
void queue_delete_event(void *);
queue_t *queue_init(void);

#endif
