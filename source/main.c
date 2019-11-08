#include <stdbool.h>
#include <stdio.h>
#include "wotbot.h"
#include "slog.h"
// #include "socket.h"
// #include "crawler.h"
#include "url_parser.h"

int main()
{

    wotbot_global_init();
    wotbot_t *wb = wotbot_init();
    wotbot_add_download(wb, "google.com");
    wotbot_add_download(wb, "https://web.facebook.com");
    wotbot_add_download(wb, "https://facebook.com");
    wotbot_perform(wb);
    wotbot_cleanup(wb);

    // SOCKET *client = socket_init();
    // if (!client)
    // {
    //     printf("Failed to start socket");
    //     exit(1);
    // }
    // socket_set_option(client, SOCKETOPT_HOST, "hoogle.com");
    // socket_set_option(client, SOCKETOPT_PORT, "80");
    // // socket_set_option(client, SOCKETOPT_READ_TIMEOUT, 5L);
    // // socket_set_option(client, SOCKETOPT_WRITE_TIMEOUT, 5L);
    // // socket_set_option(client, SOCKETOPT_USE_TLS, true);

    // int res = socket_open(client);

    // // printf("%d", res);

    // socket_close(client);

    return 0;
}
