#include <stdbool.h>
#include <stdio.h>
#include "wotbot.h"
#include "slog.h"
// #include "socket.h"
// #include "crawler.h"

int main()
{

    wotbot_global_init();
    wotbot_t *wb = wotbot_init();
    wotbot_add_download(wb, "bcswfkdmvtnxlrzh.neverssl.com");
    wotbot_add_download(wb, "http://www.stealmylogin.com");
    wotbot_perform(wb);
    wotbot_cleanup(wb);

    return 0;
}
