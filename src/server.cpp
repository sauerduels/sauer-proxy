#include <iostream>
#include <string.h>
#include "server.h"

using std::cout;
using std::endl;

Server::Server(short unsigned port, unsigned int remote_host, short unsigned remote_port, int delay, int ping_offset, bool forward_ips, bool register_master):
    port(port), remote_host(remote_host), remote_port(remote_port), delay(delay), ping_offset(ping_offset), forward_ips(forward_ips), last_master_connect_attempt(0),
    last_successful_master_update(0), master_socket(ENET_SOCKET_NULL)
{
    ENetAddress address{0, port};
    server_host = enet_host_create(&address, 128, 3, 0, 0);
    if (!server_host)
    {
        throw "Could not create server host";
    }
    cout<<"Server host listening on port "<<port<<endl;
    server_host->duplicatePeers = 128;

    if (register_master && !enet_address_set_host(&master_address, "37.59.116.203"))
        master_address.port = 28787;
    else
        master_address = ENetAddress{ENET_HOST_ANY, ENET_PORT_ANY};
}

Server::~Server()
{
    enet_host_destroy(server_host);
    server_host = NULL;
}

void Server::slice(unsigned long long millis)
{
    ENetEvent event;
    bool serviced = false;

    while (!serviced)
    {
        if (enet_host_check_events(server_host, &event) <= 0)
        {
            if (enet_host_service(server_host, &event, 5) <= 0)
                break;
            serviced = true;
        }
        char hn[100];
        switch (event.type)
        {
            case ENET_EVENT_TYPE_CONNECT:
            {
                Client *client = new Client(event.peer, remote_host, remote_port, delay, ping_offset, forward_ips);
                clients.push_back(client);
                event.peer->data = client;
                int res = enet_address_get_host_ip(&event.peer->address, hn, sizeof(hn));
                cout<<"Client connected ("<<(res==0? hn: "unknown")<<")"<<endl;
                break;
            }
            case ENET_EVENT_TYPE_RECEIVE:
            {
                Client *client = (Client *)event.peer->data;
                if (client)
                    client->c2s(event.channelID, event.packet);
                if (event.packet->referenceCount == 0)
                    enet_packet_destroy(event.packet);
                break;
            }
            case ENET_EVENT_TYPE_DISCONNECT:
            {
                Client *client = (Client *)event.peer->data;
                if (client)
                    client->disconnect(event.data);
                int res = enet_address_get_host_ip(&event.peer->address, hn, sizeof(hn));
                cout<<"Client disconnected ("<<(res==0? hn: "unknown")<<")"<<endl;
                break;
            }
            default:
                break;
        }
    }
    for (std::vector<Client *>::iterator it = clients.begin(); it != clients.end();)
    {
        if ((*it)->slice(millis))
        {
            delete *it;
            it = clients.erase(it);
        }
        else
            it++;
    }
    update_master(millis);
    process_master_input();
}

void Server::connect_master(unsigned long long millis)
{
    if (master_address.host == ENET_HOST_ANY || millis-last_master_connect_attempt < 60*1000)
        return;
    last_master_connect_attempt = millis;

    if (master_socket == ENET_SOCKET_NULL)
    {
        master_socket = enet_socket_create(ENET_SOCKET_TYPE_STREAM);
        if (master_socket == ENET_SOCKET_NULL)
        {
            cout<<"Error: Could not connect to master server"<<endl;
            return;
        }

        const ENetAddress server_address{ENET_HOST_ANY, ENET_PORT_ANY};
        if (enet_socket_bind(master_socket, &server_address) < 0)
        {
            cout<<"Error: Could not bind master socket"<<endl;
            enet_socket_destroy(master_socket);
            master_socket = ENET_SOCKET_NULL;
            return;
        }

        enet_socket_set_option(master_socket, ENET_SOCKOPT_NONBLOCK, 1);
        if (enet_socket_connect(master_socket, &master_address) < 0)
        {
            cout<<"Error: Could not connect master socket"<<endl;
            enet_socket_destroy(master_socket);
            master_socket = ENET_SOCKET_NULL;
            return;
        }
    }
}

void Server::update_master(unsigned long long millis)
{
    if (master_socket == ENET_SOCKET_NULL)
    {
        connect_master(millis);
        if (master_socket == ENET_SOCKET_NULL)
            return;
    }

    if (millis-last_successful_master_update < 60*60*1000)
        return;

    char request[100];
    sprintf(request, "regserv %u\n", port);

    ENetBuffer buf{request, strlen(request)};
    int sent = enet_socket_send(master_socket, NULL, &buf, 1);
    if (sent < 0)
    {
        cout<<"Error: Could not send master server registration request"<<endl;
        enet_socket_destroy(master_socket);
        master_socket = ENET_SOCKET_NULL;
        return;
    }

    if (sent > 0)
        last_successful_master_update = millis;
}

void Server::process_master_input()
{
    if (master_socket == ENET_SOCKET_NULL)
        return;

    ENetBuffer buf;
    char response[1024];
    response[0] = '\0';
    buf.data = response;
    buf.dataLength = 1024;

    if (enet_socket_receive(master_socket, NULL, &buf, 1) < 0)
    {
        cout<<"Error: Could not receive response from master server"<<endl;
        enet_socket_destroy(master_socket);
        master_socket = ENET_SOCKET_NULL;
        return;
    }

   if (response[0])
   {
       if (!strncmp(response, "failreg", 7))
           cout<<"Error: Master server registration failed: "<<response<<endl;
       else if (!strncmp(response, "succreg", 7))
           cout<<"Master server registration succeeded"<<endl;
       else
           cout<<"Unknown master server message received: "<<response<<endl;
   }
}
