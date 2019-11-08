#include "wotbot.h"
#include <stdbool.h>
#include <stdio.h>
#include "slog.h"

void wotbot_add_download(wotbot_t *wb, char *url)
{

    socket_t *s = socket_init();
    socket_set_option(s, SOCKETOPT_URL, url);

    // printf("%s\n", s->url->host);

    if (!socket_connect(s))
    {
        slog_error(0, "Failed to connect to host %s", s->url->host);
        socket_cleanup(s);
        return;
    }

    slog_info(0, "Connected to host %d", s->fd);

    wotbot_queue_add(wb, s);
}

void wotbot_cleanup(wotbot_t *wb)
{
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
    wb->position = 0;
    return wb;
}

void wotbot_perform(wotbot_t *wb)
{
    for (int i = 0; i < wb->position; i++)
    {
        socket_t *s = wb->queue[i];
        printf("%s\n", s->url->host);
    }
}

void wotbot_queue_add(wotbot_t *wb, socket_t *s)
{
    wb->queue[wb->position] = s;
    ++wb->position;
}
