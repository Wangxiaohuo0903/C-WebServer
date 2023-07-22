#include "server.h"
int main()
{
    WebServer server;
    server.init(16, 0);
    server.log_init();
    server.thread_pool_init();
    server.event_listen();
    server.event_loop();

    return 0;
}
