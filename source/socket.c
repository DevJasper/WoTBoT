#include "socket.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "slog.h"

#define MAX_READ_SIZE 4098

void socket_cleanup(socket_t *s)
{
    url_parser_free(s->url);
    close(s->fd);
    free(s->buffer);
    free(s);
}

bool socket_connect(socket_t *s)
{

    int check_sfd;
    struct addrinfo hints, *p, *servinfo;
    char port_number_s[sizeof("65535")];

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV;
    hints.ai_protocol = 0;

    snprintf(port_number_s, sizeof port_number_s, "%u", atoi(s->url->port));

    int res = getaddrinfo(s->url->host, s->url->port, &hints, &servinfo);
    if (res == EAI_SYSTEM)
    {
        slog_error(0, "Error looking up %s: %s\n",
                   s->url->host, strerror(errno));
        exit(1);
    }
    else if (res != 0)
    {
        slog_error(0, "Error looking up %s: %s\n",
                   s->url->host, gai_strerror(res));
        return false;
    }
    else if (servinfo == NULL)
    {
        slog_error(0, "Error looking up %s: No addresses found\n",
                   s->url->host);
        return false;
    }

    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        check_sfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (check_sfd == -1)
            continue;

        if (fcntl(check_sfd, F_SETFL, O_NONBLOCK) == -1)
        {
            slog_error(0, "Failed to set non blocking mode: %s", s->url->host);
            continue;
        }

        if (connect(check_sfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            if (errno != EINPROGRESS)
            {
                close(check_sfd);
                continue;
            }
        }

        break;
    }

    if (p == NULL)
    {
        slog_error(0, "Couldn't connect to %s: %s\n", s->url->host, strerror(errno));
        return false;
    }

    freeaddrinfo(servinfo);

    s->fd = check_sfd;
    return true;
}

socket_t *socket_init(void)
{
    socket_t *s = (socket_t *)malloc(sizeof(socket_t));
    s->fd = -1;
    s->url = url_parser_init();
    s->buffer = (char *)malloc(1 * sizeof(char));
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

bool socket_read(socket_t *s)
{
    int total_read_bytes = 0, read_bytes = 0, current_size = MAX_READ_SIZE;
    bool status = false;
    char buf[MAX_READ_SIZE];
    s->buffer = (char *)malloc(MAX_READ_SIZE * sizeof(char));
    // memset(s->buffer, 0, sizeof(s->buffer));

    while (true)
    {
        memset(buf, 0, MAX_READ_SIZE);
        read_bytes = recv(s->fd, buf, MAX_READ_SIZE, 0);

        if (read_bytes == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                status = true;
            else
                status = false;

            break;
        }

        if (read_bytes == 0)
        {
            s->buffer[total_read_bytes] = '\0';
            status = true;
            break;
        }

        if (read_bytes > 0)
        {
            if (total_read_bytes + read_bytes > current_size - 1)
            {
                current_size *= 2;
                s->buffer = realloc(s->buffer, current_size);
            }

            strncpy(s->buffer + total_read_bytes, buf, read_bytes);
            total_read_bytes += read_bytes;
            continue;
        }
    }

    s->buffer_size = total_read_bytes;
    return status;
}

bool socket_write(socket_t *s, void *buffer, size_t length)
{
    int rc = send(s->fd, buffer, length, 0);
    if (rc < 0)
    {
        slog_error(0, "Failed to send %s", s->url->host);
        return false;
    }
    return true;
}
