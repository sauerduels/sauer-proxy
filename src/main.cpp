#include <iostream>
#include <string.h>
#include "enet/enet.h"
#include "server.h"
#include "extinfo.h"

using std::cout;
using std::endl;

const char *help_message =
        "Usage:\n"
        "    ./sauer-proxy [options] remote_host\n"
        "\n"
        "Sauerbraten proxy server.\n"
        "\n"
        "Positional arguments:\n"
        "  remote_host           IP address of remote server\n"
        "\n"
        "Optional arguments:\n"
        "  -h                    Show this help message and exit\n"
        "  -p                    Port on which to listen (default: 28785)\n"
        "  -r                    Port of remote server (default: 28785)\n"
        "  -d                    Delay server->client packets by this many milliseconds\n"
        "                        (default: 0)\n"
        "  -o                    Add this many milliseconds to the pings of all\n"
        "                        connected players (default: 0)\n"
        "  -m                    Register this server with the master server (default:\n"
        "                        false)\n"
        "  -f                    Forward real player IP addresses to server (requires\n"
        "                        compatible server mod, default: false)\n";

int port = 28785;
int delay = 0;
int ping_offset = 0;
int remote_port = 28785;
char remote_host[100] = "";
bool register_master = false;
bool forward_ips = false;

int parse_args(int argc, char *argv[])
{
    bool valid = false;
    int i = 1;
    while (i < argc)
    {
        if (argv[i][0] != '-')
        {
            strncpy(remote_host, argv[i++], 100);
            valid = true;
            continue;
        }
        char *arg = argv[i];
        for (int j = 1; j < strlen(arg); j++)
        {
            switch (arg[j])
            {
                case 'h':
                    cout<<help_message;
                    return -1;
                    break;
                case 'p':
                    port = strtol(argv[++i], NULL, 10);
                    break;
                case 'r':
                    remote_port = strtol(argv[++i], NULL, 10);
                    break;
                case 'd':
                    delay = strtol(argv[++i], NULL, 10);
                    break;
                case 'o':
                    ping_offset = strtol(argv[++i], NULL, 10);
                    break;
                case 'm':
                    register_master = true;
                    break;
                case 'f':
                    forward_ips = true;
                    break;
            }
        }
        i++;
    }
    if (!valid)
    {
        cout<<"Error: remote_host argument required"<<endl;
        return -1;
    }
    return 0;
}

unsigned long int get_milliseconds()
{
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    return spec.tv_sec*1000 + spec.tv_nsec/1000000;
}

int main(int argc, char *argv[])
{
    try {
        if (parse_args(argc, argv))
            return 1;

        if (enet_initialize() < 0)
            throw "Error initializing enet";

        ENetAddress remote_host_address;
        if (enet_address_set_host(&remote_host_address, remote_host))
            throw "Error resolving remote host";

        Server *server = new Server(port, remote_host_address.host, remote_port, delay, ping_offset, forward_ips, register_master);
        ExtInfoServer *extinfo_server = new ExtInfoServer(port+1, remote_host_address.host, remote_port+1);

        while (1)
        {
            unsigned long int millis = get_milliseconds();
            server->slice(millis);
            extinfo_server->slice(millis);
            usleep(1000);
        }

        delete server;
        delete extinfo_server;

        enet_deinitialize();
    }
    catch (char const* e)
    {
        cout<<"Error: "<<e<<endl;
        return 1;
    }
    return 0;
}

