#ifndef URL_PARSER_H
#define URL_PARSER_H

#include <stdbool.h>

typedef struct
{
    char *protocol;
    char *host;
    char *port;
    char *path;
    char *query_string;
    int host_exists;
    char *ip;
} url_t;

void url_parser_free(url_t *);
void url_parser_cleanup(url_t *);
url_t *url_parser_init(void);
int url_parser_parse(url_t *, char *, bool);

#endif
