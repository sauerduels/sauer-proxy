#include <iostream>
#include <string.h>
#include "extinfo.h"

using std::cout;
using std::endl;


ExtInfoServer::ExtInfoServer(unsigned short port, unsigned int remote_host, unsigned short remote_port):
    remote_address({remote_host, remote_port}), last_update(0)
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

    ping_socket = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);
    ENetAddress info_address = {ENET_HOST_ANY, ENET_PORT_ANY};
    if(ping_socket != ENET_SOCKET_NULL && enet_socket_bind(ping_socket, &info_address) < 0)
    {
        enet_socket_destroy(ping_socket);
        ping_socket = ENET_SOCKET_NULL;
    }
    if(ping_socket == ENET_SOCKET_NULL)
        throw "Could not create extinfo socket";
    enet_socket_set_option(ping_socket, ENET_SOCKOPT_NONBLOCK, 1);

    for (int i = 0; i < 3; i++)
    {
        response_bufs[i].data = new unsigned char[4096];
        response_bufs[i].dataLength = 4096;
    }

    cout<<"Extinfo server socket listening on port "<<port<<endl;
}

ExtInfoServer::~ExtInfoServer()
{
    for (int i = 0; i < 3; i++)
    {
        delete (unsigned char *)response_bufs[i].data;
        response_bufs[i].data = NULL;
        response_bufs[i].dataLength = 0;
    }

    enet_socket_destroy(pong_socket);
    pong_socket = ENET_SOCKET_NULL;
}



void ExtInfoServer::slice(unsigned long long millis)
{
    update_info(millis);
    process_requests(millis);
}

void ExtInfoServer::update_info(unsigned long long millis)
{
    static unsigned char req[4096];
    ENetBuffer buf;
    buf.data = req;
    buf.dataLength = 4096;

    if (millis-last_update >= 5000)
    {
        req[0] = 0x80;
        req[1] = 0x01;
        buf.dataLength = 2;
        enet_socket_send(ping_socket, &remote_address, &buf, 1);

        req[0] = 0x00;
        req[1] = 0x01;
        req[2] = 0xff;
        buf.dataLength = 3;
        enet_socket_send(ping_socket, &remote_address, &buf, 1);

        req[0] = 0x00;
        req[1] = 0x02;
        buf.dataLength = 2;
        enet_socket_send(ping_socket, &remote_address, &buf, 1);

        last_update = millis;
    }

    buf.dataLength = 4096;
    int len = enet_socket_receive(ping_socket, NULL, &buf, 1);
    if (len < 0)
        cout<<"Error: Could not receive extinfo response"<<endl;
    else if (len >= 2)
    {
        if (req[0] == 0x80)
        {
            memcpy(response_bufs[0].data, &req[2], len-2);
            response_bufs[0].dataLength = len-2;
        }
        else if (req[1] == 0x01)
        {
            memcpy(response_bufs[1].data, &req[3], len-3);
            response_bufs[1].dataLength = len-3;
        }
        else if (req[1] == 0x02)
        {
            memcpy(response_bufs[2].data, &req[2], len-2);
            response_bufs[2].dataLength = len-2;
        }
    }
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

    ENetBuffer *response_buf;
    if (pong[0] != 0x00) response_buf = &response_bufs[0];
    else if (pong[1] == 0x01) response_buf = &response_bufs[1];
    else if (pong[1] == 0x02) response_buf = &response_bufs[2];

    int replyLen = std::min(4096-len, (int)response_buf->dataLength);
    memcpy(&pong[len], response_buf->data, replyLen);
    buf.dataLength = len + replyLen;
    enet_socket_send(pong_socket, &pong_address, &buf, 1);
}
