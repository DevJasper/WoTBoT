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
    int fd, buffer_size;
    url_t *url;
    SSL *ssl;
    char *buffer;
} socket_t;

void socket_cleanup(socket_t *);
socket_t *socket_init(void);
bool socket_connect(socket_t *);
bool socket_connect_ssl(socket_t *, SSL_CTX *);
bool socket_read(socket_t *);
bool socket_read_ssl(socket_t *);
void socket_set_option(socket_t *, socket_option_t, void *);
bool socket_write(socket_t *, void *, size_t);
bool socket_write_ssl(socket_t *, void *, size_t);

#endif
