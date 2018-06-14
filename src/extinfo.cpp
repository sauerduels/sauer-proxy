#include <iostream>
#include <map>
#include <string.h>
#include "extinfo.h"

using std::cout;
using std::endl;

void print_buf(ENetBuffer *buf, int len)
{
    for (int i = 0; i < len; i++)
    {
        cout<<std::hex<<(int)((unsigned char *)buf->data)[i]<<" ";
    }
    cout<<endl;
}

ExtInfoServer::ExtInfoServer(unsigned short port, unsigned int remote_host, unsigned short remote_port):
    remote_address({remote_host, remote_port})
{
    pong_socket = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);
    ENetAddress pong_address = {ENET_HOST_ANY, port};
    if(pong_socket != ENET_SOCKET_NULL && enet_socket_bind(pong_socket, &pong_address) < 0)
    {
        enet_socket_destroy(pong_socket);
        pong_socket = ENET_SOCKET_NULL;
    }
    if(pong_socket == ENET_SOCKET_NULL)
        throw "Could not create pong socket";
    enet_socket_set_option(pong_socket, ENET_SOCKOPT_NONBLOCK, 1);

    cout<<"Extinfo server socket listening on port "<<port<<endl;
}

ExtInfoServer::~ExtInfoServer()
{
    enet_socket_destroy(pong_socket);
    pong_socket = ENET_SOCKET_NULL;
}



void ExtInfoServer::slice(unsigned long long millis)
{
    process_requests(millis);
    process_clients(millis);
}

void ExtInfoServer::process_clients(unsigned long long millis)
{
    static unsigned char req[4096];
    ENetBuffer buf;
    buf.data = req;
    buf.dataLength = 4096;

    for (std::map<ENetAddress, ExtInfoClient, ENetAddressCompare>::iterator it = clients.begin(); it != clients.end(); it++)
    {
        ExtInfoClient &client = it->second;
        int len = enet_socket_receive(client.server_socket, NULL, &buf, 1);
        if (len > 0)
        {
            enet_socket_send(pong_socket, &it->first, &buf, 1);
            client.update_millis = millis;
        }
        else if (millis - client.update_millis > 30*1000)
        {
            enet_socket_destroy(it->second.server_socket);
            it = clients.erase(it);
        }
    }
}

ENetSocket new_socket()
{
    ENetSocket socket = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);
    ENetAddress info_address = {ENET_HOST_ANY, ENET_PORT_ANY};
    if (socket != ENET_SOCKET_NULL && enet_socket_bind(socket, &info_address) < 0)
    {
        enet_socket_destroy(socket);
        return(ENET_SOCKET_NULL);
    }
    enet_socket_set_option(socket, ENET_SOCKOPT_NONBLOCK, 1);
    return socket;
}

void ExtInfoServer::process_requests(unsigned long long millis)
{
    ENetAddress pong_address;
    ENetBuffer buf;
    static unsigned char pong[4096];
    buf.data = pong;
    buf.dataLength = sizeof(pong);
    int len = enet_socket_receive(pong_socket, &pong_address, &buf, 1);
    if (len <= 0 || len > 4096) return;

    ENetSocket ping_socket;
    std::map<ENetAddress, ExtInfoClient, ENetAddressCompare>::iterator it = clients.find(pong_address);
    if (it == clients.end())
    {
        ping_socket = new_socket();
        clients[pong_address] = ExtInfoClient{ping_socket, millis};
    }
    else
    {
        ping_socket = it->second.server_socket;
        it->second.update_millis = millis;
    }

    buf.dataLength = len;
    len = enet_socket_send(ping_socket, &remote_address, &buf, 1);
}
