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

void socket_cleanup(socket_t *s)
{
    url_parser_free(s->url);
    close(s->fd);
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
    // If we get here, we couldn't connect to any of the addresses.

    // int ret = 0;
    // int conn_fd;
    // struct sockaddr_in server_addr = {0};

    // conn_fd = socket(AF_INET, SOCK_STREAM, 0);
    // if (conn_fd == -1)
    // {
    //     slog_error(0, "Failed to create socket: %s", s->url->host);
    //     return false;
    // }

    // memset(&server_addr, 0, sizeof(server_addr));
    // server_addr.sin_family = AF_INET;
    // server_addr.sin_port = htons(atoi(s->url->port));

    // if (s->url->ip == NULL)
    // {
    //     slog_error(0, "Failed to resolve IP address: %s", s->url->host);
    //     return false;
    // }

    // server_addr.sin_addr.s_addr = inet_addr(s->url->ip);

    // if (INADDR_NONE == server_addr.sin_addr.s_addr)
    // {
    //     close(conn_fd);
    //     return false;
    // }

    // if (fcntl(conn_fd, F_SETFL, O_NONBLOCK) == -1)
    // {
    //     slog_error(0, "Failed to set non blocking mode: %s", s->url->host);
    //     return false;
    // }

    // ret = connect(conn_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    // if (ret == -1)
    // {

    //     if (errno != EINPROGRESS)
    //     {
    //         slog_error(0, "Connecting to server failed: %s", s->url->host);
    //         return false;
    //     }
    // }

    // s->fd = conn_fd;

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

void socket_read(socket_t *s)
{
    char buffer[200];
    int rc, len;
    do
    {
        rc = recv(s->fd, buffer, strlen(buffer), 0);
        if (rc < 0)
        {
            if (errno != EWOULDBLOCK)
            {
                perror("  recv() failed");
            }
            break;
        }

        /**********************************************/
        /* Check to see if the connection has been    */
        /* closed by the client                       */
        /**********************************************/
        if (rc == 0)
        {
            printf("  Connection closed\n");
            break;
        }

        /**********************************************/
        /* Data was received                          */
        /**********************************************/
        len = rc;
        printf("%d bytes received\n", len);

        /**********************************************/

    } while (true);
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
