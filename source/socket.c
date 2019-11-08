#include "socket.h"
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include "slog.h"

void socket_cleanup(socket_t *s)
{
    url_parser_free(s->url);
    free(s);
}

bool socket_connect(socket_t *s)
{
    struct addrinfo hints = {0}, *addrs;
    int sfd = -1;

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    const int status = getaddrinfo(s->url->ip, s->url->port, &hints, &addrs);
    if (status != 0)
    {
        slog_error(0, "%s: %s", s->url->host, gai_strerror(status));
        return false;
    }

    for (struct addrinfo *addr = addrs; addr != NULL; addr = addr->ai_next)
    {
        sfd = socket(addrs->ai_family, addrs->ai_socktype, addrs->ai_protocol);

        if (sfd == -1)
            continue;

        if (fcntl(sfd, F_SETFL, O_NONBLOCK) == -1)
            continue;

        if (connect(sfd, addr->ai_addr, addr->ai_addrlen) == 0)
            break;

        close(sfd);
    }

    freeaddrinfo(addrs);

    if (sfd == -1)
    {
        slog_error(0, "%s: %s", s->url->host, gai_strerror(status));
        return false;
    }

    s->fd = sfd;

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
