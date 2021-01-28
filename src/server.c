#include "server.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>
#include <stdbool.h>
#include <stdio.h>

#include "client.h"
#include "message.h"
#include "world.h"

#define SERVER_PORT 3000

#define TICK_RATE 60
#define FRAME_DELAY (1000 / TICK_RATE)

struct client
{
    int id;
    TCPsocket socket;
    IPaddress udp_address;
    float x;
    float y;
};

int server_main(int argc, char *argv[])
{
    if (SDL_Init(0) != 0)
    {
        printf("Failed to initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    if (SDLNet_Init() != 0)
    {
        printf("Failed to initialize SDL_net: %s\n", SDLNet_GetError());
        return 1;
    }

    IPaddress server_address;
    if (SDLNet_ResolveHost(&server_address, INADDR_ANY, SERVER_PORT))
    {
        printf("Error: Failed to resolve host: %s\n", SDLNet_GetError());
        return 1;
    }

    TCPsocket tcp_socket = SDLNet_TCP_Open(&server_address);
    if (!tcp_socket)
    {
        printf("Error: Failed to open TCP socket: %s\n", SDLNet_GetError());
        return 1;
    }

    UDPsocket udp_socket = SDLNet_UDP_Open(SERVER_PORT);
    if (!udp_socket)
    {
        printf("Error: Failed to open UDP socket: %s\n", SDLNet_GetError());
        return 1;
    }

    SDLNet_SocketSet socket_set = SDLNet_AllocSocketSet(MAX_CLIENTS + 2);
    if (!socket_set)
    {
        printf("Error: Failed to allocate socket set: %s\n", SDLNet_GetError());
        return 1;
    }
    SDLNet_TCP_AddSocket(socket_set, tcp_socket);
    SDLNet_UDP_AddSocket(socket_set, udp_socket);

    struct client clients[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        clients[i].id = -1;
    }

    struct world world;
    world_init(&world);

    unsigned int current_time = 0;

    bool quit = false;
    while (!quit)
    {
        unsigned int frame_start = SDL_GetTicks();
        unsigned int previous_time = current_time;
        current_time = frame_start;
        float delta_time = (current_time - previous_time) / 1000.0f;

        while (SDLNet_CheckSockets(socket_set, 0) > 0)
        {
            if (SDLNet_SocketReady(tcp_socket))
            {
                TCPsocket socket = SDLNet_TCP_Accept(tcp_socket);
                if (socket)
                {
                    int new_client_id = -1;
                    for (int i = 0; i < MAX_CLIENTS; i++)
                    {
                        if (clients[i].id == -1)
                        {
                            new_client_id = i;
                            break;
                        }
                    }
                    if (new_client_id != -1)
                    {
                        printf("Connected to client, assigning ID %d\n", new_client_id);

                        clients[new_client_id].id = new_client_id;
                        clients[new_client_id].socket = socket;
                        clients[new_client_id].x = 100.0f;
                        clients[new_client_id].y = 100.0f;

                        SDLNet_TCP_AddSocket(socket_set, clients[new_client_id].socket);

                        struct connect_ok_message connect_ok_message;
                        connect_ok_message.type = MESSAGE_CONNECT_OK;
                        connect_ok_message.assigned_id = new_client_id;
                        for (int i = 0; i < MAX_CLIENTS; i++)
                        {
                            connect_ok_message.clients[i].id = clients[i].id;
                            connect_ok_message.clients[i].x = clients[i].x;
                            connect_ok_message.clients[i].y = clients[i].y;
                        }
                        for (int i = 0; i < NUM_MOBS; i++)
                        {
                            connect_ok_message.world.mobs[i].alive = world.mobs[i].alive;
                            connect_ok_message.world.mobs[i].x = world.mobs[i].x;
                            connect_ok_message.world.mobs[i].y = world.mobs[i].y;
                        }

                        if (SDLNet_TCP_Send(socket, &connect_ok_message, sizeof(connect_ok_message)) < (int)sizeof(connect_ok_message))
                        {
                            printf("Error: Failed to send TCP packet: %s\n", SDLNet_GetError());
                        }

                        struct connect_broadcast_message connect_broadcast_message;
                        connect_broadcast_message.type = MESSAGE_CONNECT_BROADCAST;
                        connect_broadcast_message.new_client_id = new_client_id;
                        connect_broadcast_message.new_client_x = clients[new_client_id].x;
                        connect_broadcast_message.new_client_y = clients[new_client_id].y;

                        for (int i = 0; i < MAX_CLIENTS; i++)
                        {
                            if (clients[i].id != -1 && clients[i].id != clients[new_client_id].id)
                            {
                                if (SDLNet_TCP_Send(clients[i].socket, &connect_broadcast_message, sizeof(connect_broadcast_message)) < (int)sizeof(connect_broadcast_message))
                                {
                                    printf("Error: Failed to send TCP packet: %s\n", SDLNet_GetError());
                                }
                            }
                        }
                    }
                    else
                    {
                        printf("A client tried to connect, but the server is full\n");

                        struct message message;
                        message.type = MESSAGE_CONNECT_FULL;
                        if (SDLNet_TCP_Send(socket, &message, sizeof(message)) < (int)sizeof(message))
                        {
                            printf("Error: Failed to send TCP packet: %s\n", SDLNet_GetError());
                        }
                    }
                }
            }

            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (clients[i].id != -1)
                {
                    if (SDLNet_SocketReady(clients[i].socket))
                    {
                        char buffer[PACKET_SIZE];
                        if (SDLNet_TCP_Recv(clients[i].socket, buffer, sizeof(buffer)) > 1)
                        {
                            struct message *message = (struct message *)buffer;
                            switch (message->type)
                            {
                            case MESSAGE_DISCONNECT_REQUEST:
                            {
                                printf("Client %d disconnected\n", clients[i].id);

                                struct id_message id_message;
                                id_message.type = MESSAGE_DISCONNECT_BROADCAST;
                                id_message.id = clients[i].id;

                                for (int j = 0; j < MAX_CLIENTS; j++)
                                {
                                    if (clients[j].id != -1 && clients[j].id != clients[i].id)
                                    {
                                        if (SDLNet_TCP_Send(clients[j].socket, &id_message, sizeof(id_message)) < (int)sizeof(id_message))
                                        {
                                            printf("Error: Failed to send TCP packet: %s\n", SDLNet_GetError());
                                        }
                                    }
                                }

                                SDLNet_TCP_DelSocket(socket_set, clients[i].socket);
                                SDLNet_TCP_Close(clients[i].socket);

                                clients[i].id = -1;
                                clients[i].socket = NULL;
                            }
                            break;
                            default:
                            {
                                printf("Error: Unknown TCP packet type: %d\n", message->type);
                            }
                            break;
                            }
                        }
                    }
                }
            }

            if (SDLNet_SocketReady(udp_socket))
            {
                UDPpacket *packet = SDLNet_AllocPacket(PACKET_SIZE);
                if (SDLNet_UDP_Recv(udp_socket, packet) == 1)
                {
                    struct message *message = (struct message *)packet->data;
                    switch (message->type)
                    {
                    case MESSAGE_UDP_CONNECT_REQUEST:
                    {
                        struct id_message *id_message = (struct id_message *)message;

                        clients[id_message->id].udp_address = packet->address;

                        printf("Saving UDP info of client %d\n", id_message->id);
                    }
                    break;
                    case MESSAGE_INPUT_REQUEST:
                    {
                        struct input_request_message *input_request_message = (struct input_request_message *)message;

                        client_move(input_request_message->dx, input_request_message->dy, delta_time, &clients[input_request_message->id].x, &clients[input_request_message->id].y);
                    }
                    break;
                    default:
                    {
                        printf("Error: Unknown UDP packet type: %d\n", message->type);
                    }
                    break;
                    }
                }
                SDLNet_FreePacket(packet);
            }
        }

        world_update(&world, delta_time);

        {
            UDPpacket *packet = SDLNet_AllocPacket(PACKET_SIZE);

            struct world_state_broadcast_message *world_state_broadcast_message = (struct world_state_broadcast_message *)malloc(sizeof(*world_state_broadcast_message));
            world_state_broadcast_message->type = MESSAGE_WORLD_STATE_BROADCAST;
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                world_state_broadcast_message->clients[i].id = clients[i].id;
                world_state_broadcast_message->clients[i].x = clients[i].x;
                world_state_broadcast_message->clients[i].y = clients[i].y;
            }
            for (int i = 0; i < NUM_MOBS; i++)
            {
                world_state_broadcast_message->world.mobs[i].alive = world.mobs[i].alive;
                world_state_broadcast_message->world.mobs[i].x = world.mobs[i].x;
                world_state_broadcast_message->world.mobs[i].y = world.mobs[i].y;
            }

            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (clients[i].id != -1)
                {
                    packet->address = clients[i].udp_address;
                    packet->data = (unsigned char *)world_state_broadcast_message;
                    packet->len = sizeof(*world_state_broadcast_message);

                    if (!SDLNet_UDP_Send(udp_socket, -1, packet))
                    {
                        printf("Error: Failed to send UDP packet\n");
                    }
                }
            }

            SDLNet_FreePacket(packet);
        }

        unsigned int frame_end = SDL_GetTicks();
        unsigned int frame_time = frame_end - frame_start;
        if (FRAME_DELAY > frame_time)
        {
            SDL_Delay(FRAME_DELAY - frame_time);
        }
    }

    // TODO: inform clients that server shut down
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].id != -1)
        {
            SDLNet_TCP_DelSocket(socket_set, clients[i].socket);
            SDLNet_TCP_Close(clients[i].socket);
        }
    }

    SDLNet_UDP_DelSocket(socket_set, udp_socket);
    SDLNet_TCP_DelSocket(socket_set, tcp_socket);

    SDLNet_FreeSocketSet(socket_set);

    SDLNet_UDP_Close(udp_socket);
    SDLNet_TCP_Close(tcp_socket);

    SDLNet_Quit();

    SDL_Quit();

    return 0;
}
