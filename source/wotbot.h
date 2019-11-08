#ifndef WOTBOT_H
#define WOTBOT_H
#include "socket.h"

#define CONN_QUEUE_SIZE 200

typedef struct
{
    short position;
    socket_t *queue[CONN_QUEUE_SIZE];
} wotbot_t;

void wotbot_add_download(wotbot_t *, char *);
void wotbot_cleanup(wotbot_t *);
void wotbot_global_init(void);
wotbot_t *wotbot_init(void);
void wotbot_perform(wotbot_t *);
void wotbot_queue_add(wotbot_t *wb, socket_t *);

#endif
