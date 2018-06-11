#include <enet/enet.h>

class ExtInfoServer
{
public:
    ExtInfoServer(unsigned short port, unsigned int remote_host, short unsigned remote_port);
    ~ExtInfoServer();
    void slice(unsigned long long millis);
    void update_info(unsigned long long millis);
    void process_requests(unsigned long long millis);

private:
    ENetSocket pong_socket;
    ENetSocket ping_socket;
    ENetAddress remote_address;
    unsigned long long last_update;
    ENetBuffer response_bufs[3];
};
