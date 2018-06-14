#include <map>
#include <enet/enet.h>

struct ExtInfoClient {
    ENetSocket server_socket;
    unsigned long long update_millis;
};

struct ENetAddressCompare
{
   bool operator() (const ENetAddress& lhs, const ENetAddress& rhs) const
   {
       if (lhs.host == rhs.host) return lhs.port < rhs.port;
       return lhs.host < rhs.host;
   }
};

class ExtInfoServer
{
public:
    ExtInfoServer(unsigned short port, unsigned int remote_host, short unsigned remote_port);
    ~ExtInfoServer();
    void slice(unsigned long long millis);
    void process_clients(unsigned long long millis);
    void process_requests(unsigned long long millis);

private:
    ENetSocket pong_socket;
    ENetAddress remote_address;
    std::map<ENetAddress, ExtInfoClient, ENetAddressCompare> clients;
};
