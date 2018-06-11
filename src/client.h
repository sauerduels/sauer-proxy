#include "enet/enet.h"
#include <queue>

struct Message
{
    unsigned long int millis;
    unsigned char chan;
    ENetPacket *packet;
    bool pass_through;
};

class Client
{
public:
    Client(ENetPeer *client_peer, unsigned int remote_host, unsigned short remote_port, int delay, int ping_offset, bool forward_ip);
    ~Client();
    void c2s(unsigned char chan, ENetPacket *packet);
    void s2c(unsigned long long millis, unsigned char chan, ENetPacket *packet);
    void process_queue(unsigned long long millis);
    void forward_address();
    int slice(unsigned long long millis);
    void disconnect(int reason);

private:
    ENetPeer *client_peer;
    ENetHost *client_host;
    ENetPeer *server_peer;
    bool connected;
    bool disconnecting;
    bool forward_ip;
    int delay;
    int ping_offset;
    std::queue<Message> queue;
};
