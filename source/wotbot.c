#include "wotbot.h"
#include <assert.h>
#include <errno.h>
#include <gumbo.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/socket.h>
#include "http_status_codes.h"
#include "response.h"
#include "slog.h"

static void wotbot_add_event(wotbot_t *wb, socket_t *s);
static void wotbot_read_handler(wotbot_t *, socket_t *);
static void wotbot_get_links(wotbot_t *, const GumboNode *);
static const char *wotbot_get_title(wotbot_t *, const GumboNode *);
static const char *wotbot_get_header_value(wotbot_t *, response_t *, const char *);

void wotbot_add_download(wotbot_t *wb, char *url)
{

    socket_t *s = socket_init();
    socket_set_option(s, SOCKETOPT_URL, url);

    if (!socket_connect(s))
    {
        slog_error(0, "Failed to connect to URL: %s", s->url->self);
        socket_cleanup(s);
        return;
    }

    if (strcmp(s->url->protocol, "https") == 0)
    {
        if (!socket_connect_ssl(s, wb->ctx))
        {
            slog_error(0, "Failed to SSL connect to URL: %s", s->url->self);
            socket_cleanup(s);
            return;
        }
    }

    slog_info(0, "Connected to URL: %s", s->url->self);
    wotbot_add_event(wb, s);
}

static void wotbot_add_event(wotbot_t *wb, socket_t *s)
{
    if (wb->event->position >= wb->event->size)
    {
        wb->event->size *= 2;
        wb->event->changes = realloc(wb->event->changes, wb->event->size * sizeof(struct kevent));
    }
    struct kevent evSet[2];
    EV_SET(&evSet[0], s->fd, EVFILT_READ, EV_ADD, 0, 0, s);
    EV_SET(&evSet[1], s->fd, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0, s);

    if (kevent(wb->event->fd, &evSet, 2, NULL, 0, NULL) == -1)
    {
        slog_error(0, "Failed to register %s on event loop: %s", s->url->host, strerror(errno));
        return;
    }

    wb->event->position += 2;
}

void wotbot_cleanup(wotbot_t *wb)
{
    SSL_CTX_free(wb->ctx);
    free(wb->event->changes);
    free(wb->event);
    free(wb);
}

static const char *wotbot_get_header_value(wotbot_t *wb, response_t *res, const char *target)
{
    for (int i = 0; i < res->num_headers; ++i)
    {
        struct phr_header header = res->headers[i];
        if (strncmp(header.name, target, header.name_len) == 0)
        {
            size_t len = header.value_len;
            char *value = (char *)malloc(len + 1);
            strncpy(value, header.value, len);
            value[len] = '\0';
            return value;
        }
    }

    return NULL;
}

static void wotbot_get_links(wotbot_t *wb, const GumboNode *root)
{
    if (root->type != GUMBO_NODE_ELEMENT)
        return;

    GumboAttribute *href;
    if (root->v.element.tag == GUMBO_TAG_A &&
        (href = gumbo_get_attribute(&root->v.element.attributes, "href")))
    {
        // printf("%s\n", href->value);
        wotbot_add_download(wb, href->value);
    }

    GumboVector *children = &root->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i)
    {
        wotbot_get_links(wb, (GumboNode *)children->data[i]);
    }
}

static const char *wotbot_get_title(wotbot_t *wb, const GumboNode *root)
{
    assert(root->type == GUMBO_NODE_ELEMENT);
    assert(root->v.element.children.length >= 2);

    const GumboVector *root_children = &root->v.element.children;
    GumboNode *head = NULL;
    for (int i = 0; i < root_children->length; ++i)
    {
        GumboNode *child = root_children->data[i];
        if (child->type == GUMBO_NODE_ELEMENT &&
            child->v.element.tag == GUMBO_TAG_HEAD)
        {
            head = child;
            //printf("HEAD tag found\n");
            break;
        }
    }
    assert(head != NULL);

    GumboVector *head_children = &head->v.element.children;
    for (int i = 0; i < head_children->length; ++i)
    {
        GumboNode *child = head_children->data[i];
        if (child->type == GUMBO_NODE_ELEMENT &&
            child->v.element.tag == GUMBO_TAG_TITLE)
        {

            if (child->v.element.children.length != 1)
            {
                return NULL;
            }
            GumboNode *title_text = child->v.element.children.data[0];
            assert(title_text->type == GUMBO_NODE_TEXT || title_text->type == GUMBO_NODE_WHITESPACE);
            return title_text->v.text.text;
        }
    }
    return NULL;
}

void wotbot_global_init()
{
    //init SSL
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    //init thread pool
}

wotbot_t *wotbot_init(void)
{
    wotbot_t *wb = (wotbot_t *)malloc(sizeof(wotbot_t));
    wb->event = malloc(sizeof(wotbot_event_t));
    wb->event->fd = kqueue();
    wb->event->size = EVENT_INIT_SIZE;
    wb->event->position = 0;
    wb->event->changes = (struct kevent *)calloc(EVENT_INIT_SIZE, sizeof(struct kevent));
    wb->ctx = SSL_CTX_new(TLSv1_2_client_method());

    if (wb->ctx == NULL)
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    SSL_CTX_set_options(wb->ctx, SSL_OP_NO_SSLv2);

    return wb;
}

void wotbot_perform(wotbot_t *wb)
{
    struct kevent evSet;
    int nev;
    for (;;)
    {
        nev = kevent(wb->event->fd, NULL, 0, wb->event->changes, wb->event->position, NULL);

        for (int i = 0; i < nev; i++)
        {
            struct kevent ev = wb->event->changes[i];
            socket_t *s = (socket_t *)ev.udata;
            int filter = ev.filter;

            if (ev.flags & EV_EOF)
            {
                slog_warn(0, "%s connection closed", s->url->self);
                socket_cleanup(s);
            }
            else if (filter == EVFILT_READ)
            {
                sleep(1);
                wotbot_read_handler(wb, s);
            }
            else if (filter == EVFILT_WRITE)
            {
                --wb->event->position;
                char msg[200];
                sprintf(msg, "GET %s HTTP/1.1\r\n"
                             "Host: %s\r\n\r\n",
                        s->url->path,
                        s->url->host);

                if (s->ssl != NULL)
                {
                    socket_write_ssl(s, msg, strlen(msg));
                }
                else
                {
                    socket_write(s, msg, strlen(msg));
                }
            }
        }
    }
}

static void wotbot_read_handler(wotbot_t *wb, socket_t *s)
{
    if (s->ssl != NULL)
    {
        if (!socket_read_ssl(s))
            return;

        slog_error(0, "%s", s->buffer);
    }
    else
    {
        if (!socket_read(s))
            return;
    }

    printf("%s\n", s->buffer);

    response_t *res = response_parse(s);

    if (HttpStatus_isSuccessful(res->status))
    {
        GumboOutput *output = gumbo_parse(res->body);
        const char *t = wotbot_get_title(wb, output->root);
        printf("%s\n", t);
        wotbot_get_links(wb, output->root);
        gumbo_destroy_output(&kGumboDefaultOptions, output);
    }
    else if (HttpStatus_isRedirection(res->status))
    {
        char *value = wotbot_get_header_value(wb, res, "Location");
        if (value == NULL)
            return;
        wotbot_add_download(wb, value);
        free(value);
    }
    else if (HttpStatus_isError(res->status))
    {
        return;
    }
    response_cleanup(res);
    socket_cleanup(s);
}
