#ifndef SOCKET_H
#define SOCKET_H

#include <openssl/ssl.h>
#include <stdbool.h>
#include "url_parser.h"

typedef enum
{
    SOCKETOPT_URL
} socket_option_t;

typedef struct
{
    int fd;
    url_t *url;
    SSL *ssl;
} socket_t;

void socket_cleanup(socket_t *);
socket_t *socket_init(void);
void socket_set_option(socket_t *, socket_option_t, void *);
bool socket_connect(socket_t *);

#endif