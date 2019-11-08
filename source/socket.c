#include "socket.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "slog.h"

void socket_cleanup(socket_t *s)
{
    url_parser_free(s->url);
    free(s);
}

bool socket_connect(socket_t *s)
{

    int ret = 0;
    int conn_fd;
    struct sockaddr_in server_addr = {0};

    conn_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (conn_fd == -1)
    {
        slog_error(0, "Failed to create socket: %s", s->url->host);
        return false;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(s->url->port));

    if (s->url->ip == NULL)
    {
        slog_error(0, "Failed to resolve IP address: %s", s->url->host);
        return false;
    }

    server_addr.sin_addr.s_addr = inet_addr(s->url->ip);

    if (INADDR_NONE == server_addr.sin_addr.s_addr)
    {
        close(conn_fd);
        return false;
    }

    if (fcntl(conn_fd, F_SETFL, O_NONBLOCK) == -1)
    {
        slog_error(0, "Failed to set non blocking mode: %s", s->url->host);
        return false;
    }

    ret = connect(conn_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret == -1)
    {

        if (errno != EINPROGRESS)
        {
            slog_error(0, "Connecting to server failed: %s", s->url->host);
            return false;
        }
    }

    s->fd = conn_fd;

    return true;
}

socket_t *socket_init(void)
{
    socket_t *s = (socket_t *)malloc(sizeof(socket_t));
    s->fd = -1;
    s->url = url_parser_init();
    return s;
}

void socket_set_option(socket_t *s, socket_option_t option, void *value)
{
    switch (option)
    {
    case SOCKETOPT_URL:
        url_parser_parse(s->url, (char *)value, true);
        break;

    default:
        break;
    }
}
