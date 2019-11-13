#include "url_parser.h"
#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

void url_parser_free(url_t *url)
{
    if (url->protocol)
        free(url->protocol);
    if (url->host)
        free(url->host);
    if (url->path)
        free(url->path);
    if (url->query_string)
        free(url->query_string);

    free(url);
    url = NULL;
}

url_t *url_parser_init(void)
{
    url_t *url = (url_t *)malloc(sizeof(url_t));
    url->protocol = NULL;
    url->host = NULL;
    url->port = "80";
    url->path = NULL;
    url->query_string = NULL;
    url->host_exists = -1;
    url->ip = NULL;
    return url;
}

int url_parser_parse(url_t *parsed_url, char *url, bool verify_host)
{
    size_t url_len = strlen(url);
    char *local_url = (char *)malloc(sizeof(char) * (url_len + 1));
    char *token;
    char *token_host;
    char *host_port;
    char *token_ptr;
    char *host_token_ptr;
    char *path = NULL;

    parsed_url->self = strdup(url);

    if (!strstr(url, "http") && !strstr(url, "https"))
    {
        char *tmp = "http://";
        local_url = (char *)realloc(local_url, sizeof(char) * (url_len + 1) + strlen(tmp));
        strcpy(local_url, tmp);
        strcat(local_url, url);
    }
    else
    {
        strcpy(local_url, url);
    }

    token = strtok_r(local_url, ":", &token_ptr);
    parsed_url->protocol = (char *)malloc(sizeof(char) * strlen(token) + 1);
    strcpy(parsed_url->protocol, token);

    // Host:Port
    token = strtok_r(NULL, "/", &token_ptr);
    if (token)
    {
        host_port = (char *)malloc(sizeof(char) * (strlen(token) + 1));
        strcpy(host_port, token);
    }
    else
    {
        host_port = (char *)malloc(sizeof(char) * 1);
        strcpy(host_port, "");
    }

    token_host = strtok_r(host_port, ":", &host_token_ptr);
    parsed_url->ip = NULL;
    if (token_host)
    {
        parsed_url->host = (char *)malloc(sizeof(char) * strlen(token_host) + 1);
        strcpy(parsed_url->host, token_host);

        if (verify_host)
        {
            struct hostent *host;
            host = gethostbyname(parsed_url->host);
            if (host != NULL)
            {
                parsed_url->ip = inet_ntoa(*(struct in_addr *)host->h_addr_list);
                parsed_url->host_exists = 1;
            }
            else
            {
                parsed_url->host_exists = 0;
            }
        }
        else
        {
            parsed_url->host_exists = -1;
        }
    }
    else
    {
        parsed_url->host_exists = -1;
        parsed_url->host = NULL;
    }

    // printf("%s", parsed_url->host);

    // Port
    token_host = strtok_r(NULL, ":", &host_token_ptr);
    if (token_host)
        parsed_url->port = token_host;
    else
    {
        if (strcmp(parsed_url->protocol, "https") == 0)
            parsed_url->port = "443";
    }

    token_host = strtok_r(NULL, ":", &host_token_ptr);
    assert(token_host == NULL);

    token = strtok_r(NULL, "?", &token_ptr);
    parsed_url->path = NULL;
    if (token)
    {
        path = (char *)realloc(path, sizeof(char) * (strlen(token) + 2));
        strcpy(path, "/");
        strcat(path, token);

        parsed_url->path = (char *)malloc(sizeof(char) * strlen(path) + 1);
        strncpy(parsed_url->path, path, strlen(path));
        parsed_url->path[strlen(path)] = '\0';
        free(path);
    }
    else
    {
        parsed_url->path = (char *)malloc(sizeof(char) * 2);
        strcpy(parsed_url->path, "/");
    }

    token = strtok_r(NULL, "?", &token_ptr);
    if (token)
    {
        parsed_url->query_string = (char *)malloc(sizeof(char) * (strlen(token) + 1));
        strncpy(parsed_url->query_string, token, strlen(token));
    }
    else
    {
        parsed_url->query_string = NULL;
    }

    token = strtok_r(NULL, "?", &token_ptr);
    // assert(token == NULL);

    free(local_url);
    free(host_port);
    return 0;
}
