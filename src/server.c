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

#define UPDATE_CLIENTS_RATE 20

struct client
{
    int id;
    TCPsocket socket;
    IPaddress udp_address;
    float pos_x;
    float pos_y;
    float vel_x;
    float vel_y;
    float acc_x;
    float acc_y;
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
                        clients[new_client_id].pos_x = 100.0f;
                        clients[new_client_id].pos_y = 100.0f;
                        clients[new_client_id].vel_x = 0.0f;
                        clients[new_client_id].vel_y = 0.0f;
                        clients[new_client_id].acc_x = 0.0f;
                        clients[new_client_id].acc_y = 0.0f;

                        SDLNet_TCP_AddSocket(socket_set, clients[new_client_id].socket);

                        struct message_id message_id;
                        message_id.type = MESSAGE_CONNECT_OK;
                        message_id.id = new_client_id;

                        if (SDLNet_TCP_Send(socket, &message_id, sizeof(message_id)) < (int)sizeof(message_id))
                        {
                            printf("Error: Failed to send TCP packet: %s\n", SDLNet_GetError());
                        }

                        message_id.type = MESSAGE_CONNECT_BROADCAST;

                        for (int i = 0; i < MAX_CLIENTS; i++)
                        {
                            if (clients[i].id != -1 && clients[i].id != clients[new_client_id].id)
                            {
                                if (SDLNet_TCP_Send(clients[i].socket, &message_id, sizeof(message_id)) < (int)sizeof(message_id))
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

                                struct message_id message_id;
                                message_id.type = MESSAGE_DISCONNECT_BROADCAST;
                                message_id.id = clients[i].id;

                                for (int j = 0; j < MAX_CLIENTS; j++)
                                {
                                    if (clients[j].id != -1 && clients[j].id != clients[i].id)
                                    {
                                        if (SDLNet_TCP_Send(clients[j].socket, &message_id, sizeof(message_id)) < (int)sizeof(message_id))
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
                        struct message_id *message_id = (struct message_id *)message;

                        clients[message_id->id].udp_address = packet->address;

                        printf("Saving UDP info of client %d\n", message_id->id);
                    }
                    break;
                    case MESSAGE_INPUT_REQUEST:
                    {
                        struct message_input *message_input = (struct message_input *)message;

                        clients[message_input->id].acc_x = message_input->acc_x;
                        clients[message_input->id].acc_y = message_input->acc_y;
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

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            client_accelerate(&clients[i].pos_x, &clients[i].pos_y, &clients[i].vel_x, &clients[i].vel_y, &clients[i].acc_x, &clients[i].acc_y, delta_time);
        }

        world_update(&world, delta_time);

        static float update_clients_timer = 0;
        update_clients_timer += delta_time;
        if (update_clients_timer >= 1.0f / UPDATE_CLIENTS_RATE)
        {
            update_clients_timer = 0;

            UDPpacket *packet = SDLNet_AllocPacket(PACKET_SIZE);

            struct message_world_state *message_world_state = (struct message_world_state *)malloc(sizeof(*message_world_state));
            message_world_state->type = MESSAGE_WORLD_STATE_BROADCAST;
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                message_world_state->clients[i].id = clients[i].id;
                message_world_state->clients[i].pos_x = clients[i].pos_x;
                message_world_state->clients[i].pos_y = clients[i].pos_y;
                message_world_state->clients[i].vel_x = clients[i].vel_x;
                message_world_state->clients[i].vel_y = clients[i].vel_y;
                message_world_state->clients[i].acc_x = clients[i].acc_x;
                message_world_state->clients[i].acc_y = clients[i].acc_y;
            }
            for (int i = 0; i < NUM_MOBS; i++)
            {
                message_world_state->world.mobs[i].alive = world.mobs[i].alive;
                message_world_state->world.mobs[i].x = world.mobs[i].x;
                message_world_state->world.mobs[i].y = world.mobs[i].y;
            }

            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (clients[i].id != -1)
                {
                    packet->address = clients[i].udp_address;
                    packet->data = (unsigned char *)message_world_state;
                    packet->len = sizeof(*message_world_state);

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
