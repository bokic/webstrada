#include <webstrada/appserver.h>
#include <webstrada/config.h>
#include <webstrada/cf8.h>

#include <print>

#include <unistd.h>
#include <getopt.h>


using namespace std;

static constexpr auto DEFAULT_SOCKET_NAME = ":6000";
static constexpr int DEFAULT_BACKLOG = 100;

int main(int argc, char *argv[])
{
    cfml::seed_rand();
    webstrada::appserver server;    const char *socket_name = DEFAULT_SOCKET_NAME;
    int backlog = DEFAULT_BACKLOG;
    int worker_num = 1;
    int opt = 0;

    while((opt = getopt(argc, argv, "hn:b:w:")) != -1)
    {
        switch(opt)
        {
        case 'n':
            socket_name = optarg;
            break;
        case 'b':
            backlog = atoi(optarg);
            break;
        case 'w':
            worker_num = atoi(optarg);
            break;
        case 'h':
            println(stderr, "Usage: {} [-n socket name] [-b backlog] [-w number of workers]", argv[0]);
            return EXIT_SUCCESS;
        }
    }

    server.init(socket_name, backlog, worker_num);

    webstrada::config::loadDatasourcesFromEnv();

    return server.run();
}
