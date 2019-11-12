#include "response.h"

response_t *response_parse(socket_t *s)
{

    response_t *res = (response_t *)malloc(sizeof(response_t));

    res->num_headers = sizeof(res->headers) / sizeof(res->headers[0]);

    phr_parse_response(s->buffer, s->buffer_size, &res->minor_version, &res->status, &res->msg, &res->msg_len, res->headers, &res->num_headers, 0);

    // for (int i = 0; i != res->num_headers; ++i)
    // {
    //     printf("%.*s: %.*s\n", (int)res->headers[i].name_len, res->headers[i].name,
    //            (int)res->headers[i].value_len, res->headers[i].value);
    // }

    char *body = strstr(s->buffer, "\r\n\r\n");
    size_t len = strlen(body);
    res->body = (char *)malloc(len + 1);
    strncpy(res->body, body, len);
    res->body[len] = '\0';

    return res;
}

void response_cleanup(response_t *res)
{
    free(res->body);
    free(res);
}
