#include "wotbot.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include "slog.h"
#include "sys/socket.h"

void wotbot_add_download(wotbot_t *wb, char *url)
{

    socket_t *s = socket_init();
    socket_set_option(s, SOCKETOPT_URL, url);

    if (!socket_connect(s))
    {
        slog_error(0, "Failed to connect to host: %s", s->url->host);
        socket_cleanup(s);
        return;
    }

    // char msg[80];
    // sprintf(msg,
    //         "GET / HTTP/1.1\r\n"
    //         "Host: %s\r\n"
    //         "Connection: close\r\n\r\n",
    //         s->url->host);

    // socket_write(s, msg, strlen(msg));

    slog_info(0, "Connected to host %d", s->fd);
    queue_add_event(wb->queue, s);
}

void wotbot_cleanup(wotbot_t *wb)
{
    queue_cleanup(wb->queue);
    free(wb);
}

void wotbot_global_init()
{
    //init SSL
    //init thread pool
}

wotbot_t *wotbot_init(void)
{
    wotbot_t *wb = (wotbot_t *)malloc(sizeof(wotbot_t));
    wb->queue = queue_init();
    return wb;
}

void wotbot_perform(wotbot_t *wb)
{

    if (kevent(wb->queue->fd, wb->queue->events, wb->queue->size, NULL, 0, NULL) == -1)
    {
        slog_error(0, "Failed to register events: %s", strerror(errno));
        return;
    }
    while (1)
    {
        int nev = kevent(wb->queue->fd, NULL, 0, wb->queue->changes, wb->queue->size, NULL);
        printf("%d\n", nev);
        break;
    }
}
