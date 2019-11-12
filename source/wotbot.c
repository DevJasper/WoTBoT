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

void wotbot_add_download(wotbot_t *wb, char *url)
{

    socket_t *s = socket_init();
    socket_set_option(s, SOCKETOPT_URL, url);

    if (!socket_connect(s))
    {
        slog_error(0, "Failed to connect to host: %s", s->url->host);
        socket_cleanup(s);
        return;
    }

    slog_info(0, "Connected to host %s", s->url->host);
    wotbot_add_event(wb, s);
}

static void wotbot_add_event(wotbot_t *wb, socket_t *s)
{
    EV_SET(&wb->event->queue[wb->event->position], s->fd, EVFILT_READ, EV_ADD, 0, 0, s);
    ++wb->event->position;
    EV_SET(&wb->event->queue[wb->event->position], s->fd, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0, s);
    ++wb->event->position;
}

void wotbot_cleanup(wotbot_t *wb)
{
    free(wb->event->queue);
    free(wb->event->changes);
    free(wb->event);
    free(wb);
}

void wotbot_global_init()
{
    //init SSL
    //init thread pool
}

wotbot_t *wotbot_init(void)
{
    wotbot_t *wb = (wotbot_t *)malloc(sizeof(wotbot_t));
    wb->event = malloc(sizeof(wotbot_event_t));
    wb->event->fd = kqueue();
    wb->event->size = EVENT_INIT_SIZE * sizeof(struct kevent);
    wb->event->position = 0;
    wb->event->queue = (struct kevent *)calloc(EVENT_INIT_SIZE, sizeof(struct kevent));
    wb->event->changes = (struct kevent *)calloc(EVENT_INIT_SIZE, sizeof(struct kevent));
    // wb->queue = queue_init();
    return wb;
}

void wotbot_perform(wotbot_t *wb)
{

    if (kevent(wb->event->fd, wb->event->queue, wb->event->position, NULL, 0, NULL) == -1)
    {
        slog_error(0, "Failed to register events: %s", strerror(errno));
        return;
    }

    int nev;
    // int waitms = 10000;
    // struct timespec timeout;
    // timeout.tv_sec = waitms / 1000;
    // timeout.tv_nsec = (waitms % 1000) * 1000 * 1000;

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
                slog_warn(0, "%s connection closed\n", s->url->host);
                socket_cleanup(s);
            }
            else if (filter == EVFILT_READ)
            {
                sleep(1);
                wotbot_read_handler(wb, s);
            }
            else if (filter == EVFILT_WRITE)
            {

                char msg[80];
                sprintf(msg, "GET %s HTTP/1.1\r\n"
                             "Host: %s\r\n\r\n",
                        s->url->path,
                        s->url->host);

                socket_write(s, msg, strlen(msg));
            }
        }
    }
}

// static const char *find_title(const GumboNode *root)
// {
//     assert(root->type == GUMBO_NODE_ELEMENT);
//     assert(root->v.element.children.length >= 2);

//     const GumboVector *root_children = &root->v.element.children;
//     GumboNode *head = NULL;
//     for (int i = 0; i < root_children->length; ++i)
//     {
//         GumboNode *child = root_children->data[i];
//         if (child->type == GUMBO_NODE_ELEMENT &&
//             child->v.element.tag == GUMBO_TAG_HEAD)
//         {
//             head = child;
//             break;
//         }
//     }
//     assert(head != NULL);

//     GumboVector *head_children = &head->v.element.children;
//     for (int i = 0; i < head_children->length; ++i)
//     {
//         GumboNode *child = head_children->data[i];
//         if (child->type == GUMBO_NODE_ELEMENT &&
//             child->v.element.tag == GUMBO_TAG_TITLE)
//         {
//             if (child->v.element.children.length != 1)
//             {
//                 return "<empty title>";
//             }
//             GumboNode *title_text = child->v.element.children.data[0];
//             assert(title_text->type == GUMBO_NODE_TEXT || title_text->type == GUMBO_NODE_WHITESPACE);
//             return title_text->v.text.text;
//         }
//     }
//     return "<no title found>";
// }

static const char *wotbot_get_title(const GumboNode *root)
{
    assert(root->type == GUMBO_NODE_ELEMENT);
    assert(root->v.element.children.length >= 2);

    //printf("find_title() called\n");
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
        //printf("In loop iteration %d\n", i);
        GumboNode *child = head_children->data[i];
        if (child->type == GUMBO_NODE_ELEMENT &&
            child->v.element.tag == GUMBO_TAG_TITLE)
        {
            //printf("TITLE tag found\n");
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

static void wotbot_read_handler(wotbot_t *wb, socket_t *s)
{
    if (!socket_read(s))
        return;

    EV_SET(&wb->event->queue[wb->event->position++], s->fd, EVFILT_READ, EV_ADD, 0, 0, s);

    response_t *res = response_parse(s);

    if (HttpStatus_isSuccessful(res->status))
    {
        GumboOutput *output = gumbo_parse(res->body);
        const char *t = wotbot_get_title(output->root);
        printf("%s\n", t);
        gumbo_destroy_output(&kGumboDefaultOptions, output);
    }

    response_cleanup(res);
}
