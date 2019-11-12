#ifndef RESPONSE_H
#define RESPONSE_H

#include "picohttpparser.h"
#include "socket.h"

#define MAX_ELEMENT_SIZE 500

typedef struct
{
    int minor_version, status;
    const char *msg;
    char *body;
    size_t msg_len;
    struct phr_header headers[4];
    size_t num_headers;

} response_t;

void response_cleanup(response_t *);
response_t *response_parse(socket_t *);

#endif
