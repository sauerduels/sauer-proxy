#include <iostream>
#include <memory.h>
#include "client.h"

using std::cout;
using std::endl;

#define N_PING (30)
#define N_CLIENTPING (32)
#define N_SERVCMD (110)

Client::Client(ENetPeer *client_peer, unsigned int remote_host, unsigned short remote_port, int delay, int ping_offset, bool forward_ip):
    client_peer(client_peer), forward_ip(forward_ip), connected(false), disconnecting(false), delay(delay), ping_offset(ping_offset)
{
    client_host = enet_host_create(NULL, 2, 3, 0, 0);
    if (!client_host)
        throw "Coult not create client host";

    ENetAddress address{remote_host, remote_port};
    server_peer = enet_host_connect(client_host, &address, 3, 0);
    enet_host_flush(client_host);
}

Client::~Client()
{
    if (client_peer)
        enet_peer_disconnect(client_peer, 0);
    if (server_peer)
        enet_peer_disconnect(server_peer, 0);
    enet_host_flush(client_host);
    enet_host_destroy(client_host);
}

void Client::c2s(unsigned char chan, ENetPacket *packet)
{
    if (packet->data && packet->dataLength > 0)
    {
        switch (packet->data[0])
        {
            case N_PING:
                packet->data[0] = 31;
                enet_peer_send(client_peer, chan, packet);
                break;
            case N_CLIENTPING:
                // TODO: add ping_offset
                enet_peer_send(server_peer, chan, packet);
                break;
            case N_SERVCMD:
                break;
            default:
                enet_peer_send(server_peer, chan, packet);
                break;
        }
    } else {
        enet_peer_send(server_peer, chan, packet);
    }
}

void Client::s2c(unsigned long long millis, unsigned char chan, ENetPacket *packet)
{
    bool pass_through = false;

    if (packet->data && packet->dataLength > 0)
    {
        switch (packet->data[0])
        {
            case 1:
            case 2:
            case 35:
            case 36:
            case 37:
            case 25:
                pass_through = true;
                break;
            default:
                break;
        }
    }

    Message message{millis, chan, packet, pass_through};
    queue.push(message);
}


void Client::process_queue(unsigned long long millis)
{
    const time_t until = millis - delay;

    while (1)
    {
        if (queue.size() == 0)
            break;

        Message &message = queue.front();
        if (message.millis <= until || message.pass_through)
        {
            enet_peer_send(client_peer, message.chan, message.packet);
            queue.pop();
        }
        else
            break;
    }
}

void Client::forward_address()
{
    const unsigned char servmsg[] = {110, 115, 101, 116, 105, 112, 0, 0x81};

    const int ip = client_peer->address.host;
    unsigned char *buf = new(unsigned char[12]);
    memcpy(buf, servmsg, 8);
    buf[8] = ip;
    buf[9] = ip>>8;
    buf[10] = ip>>16;
    buf[11] = ip>>24;

    ENetPacket *packet = enet_packet_create(buf, 12, 1);
    packet->flags &= ENET_PACKET_FLAG_RELIABLE;
    enet_peer_send(server_peer, 1, packet);

    if (packet->referenceCount == 0)
        enet_packet_destroy(packet);
}

int Client::slice(unsigned long long millis)
{
    if (!client_host || disconnecting)
        return -1;

    ENetEvent event;
    while (enet_host_service(client_host, &event, 0) > 0)
    {
        switch (event.type)
        {
            case ENET_EVENT_TYPE_CONNECT:
            {
                connected = true;
                enet_peer_throttle_configure(server_peer, 5000, 2, 2);
                enet_host_bandwidth_limit(client_host, 0*1024, 0*1024);

                if (forward_ip)
                    forward_address();
                break;
            }
            case ENET_EVENT_TYPE_RECEIVE:
            {
                s2c(millis, event.channelID, event.packet);
                break;
            }
            case ENET_EVENT_TYPE_DISCONNECT:
            {
                enet_peer_disconnect(client_peer, event.data);
                client_peer = NULL;
                connected = false;
                disconnecting = true;
                return -1;
                break;
            }
            default:
                break;
        }
    }
    process_queue(millis);
    return 0;
}

void Client::disconnect(int reason)
{
    enet_peer_disconnect(server_peer, reason);
    server_peer = 0;
    disconnecting = true;
}
