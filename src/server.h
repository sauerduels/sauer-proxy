#include <vector>
#include "enet/enet.h"
#include "client.h"

class Server
{
public:
    Server(short unsigned port, unsigned int remote_host, short unsigned remote_port, int delay, int ping_offset, bool forward_ips, bool register_master);
    ~Server();
    void slice(unsigned long long millis);
    void connect_master(unsigned long long millis);
    void update_master(unsigned long long millis);
    void process_master_input();

private:
    ENetHost *server_host;
    unsigned short port;
    unsigned int remote_host;
    unsigned short remote_port;
    int delay;
    int ping_offset;
    bool forward_ips;
    std::vector<Client *> clients;

    unsigned long int last_master_connect_attempt;
    unsigned long int last_successful_master_update;
    ENetSocket master_socket;
    ENetAddress master_address;
};
