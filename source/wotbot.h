#ifndef WOTBOT_H
#define WOTBOT_H
#include <sys/event.h>
#include "socket.h"

#define CONN_QUEUE_SIZE 200
#define EVENT_INIT_SIZE 1000

typedef struct
{
    int fd, size, position;
    struct kevent *queue, *changes;
} wotbot_event_t;

typedef struct
{
    wotbot_event_t *event;
    // short position;
    // socket_t *queue[CONN_QUEUE_SIZE];
} wotbot_t;

void wotbot_add_download(wotbot_t *, char *);
void wotbot_cleanup(wotbot_t *);
void wotbot_global_init(void);
wotbot_t *wotbot_init(void);
void wotbot_perform(wotbot_t *);

#endif
