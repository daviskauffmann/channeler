#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>
#include <shared/map.h>
#include <shared/message.h>
#include <shared/player.h>
#include <shared/world.h>
#include <stdbool.h>
#include <stdio.h>

#define SERVER_PORT 8492

#define TICK_RATE 60
#define FRAME_DELAY (1000 / TICK_RATE)

#define UPDATE_CLIENTS_RATE 20

struct client
{
    size_t id;
    TCPsocket socket;
    IPaddress udp_address;
    struct player player;
    struct input input;
};

int main(int argc, char *argv[])
{
    if (SDL_Init(0) != 0)
    {
        printf("Error: Failed to initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    if (SDLNet_Init() != 0)
    {
        printf("Error: Failed to initialize SDL_net: %s\n", SDLNet_GetError());
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

    printf("Listening on port %d\n", SERVER_PORT);

    struct client clients[MAX_CLIENTS];
    for (size_t i = 0; i < MAX_CLIENTS; i++)
    {
        clients[i].id = -1;
    }

    struct world world;
    world_load(&world, "assets/world.json", true);

    bool quit = false;
    while (!quit)
    {
        static uint32_t current_time = 0;
        uint32_t frame_start = SDL_GetTicks();
        uint32_t previous_time = current_time;
        current_time = frame_start;
        float delta_time = (current_time - previous_time) / 1000.0f;

        while (SDLNet_CheckSockets(socket_set, 0) > 0)
        {
            if (SDLNet_SocketReady(tcp_socket))
            {
                TCPsocket socket = SDLNet_TCP_Accept(tcp_socket);
                if (socket)
                {
                    size_t new_client_id = -1;
                    for (size_t i = 0; i < MAX_CLIENTS; i++)
                    {
                        if (clients[i].id == -1)
                        {
                            new_client_id = i;
                            break;
                        }
                    }
                    if (new_client_id != -1)
                    {
                        printf("Connected to client, assigning ID %zd\n", new_client_id);

                        clients[new_client_id].id = new_client_id;
                        clients[new_client_id].socket = socket;
                        player_init(&clients[new_client_id].player, 0);

                        SDLNet_TCP_AddSocket(socket_set, clients[new_client_id].socket);

                        {
                            struct message_connect message;
                            message.type = MESSAGE_CONNECT_OK;
                            message.id = new_client_id;
                            message.map_index = 0;
                            if (SDLNet_TCP_Send(socket, &message, sizeof(message)) < (int)sizeof(message))
                            {
                                printf("Error: Failed to send TCP packet: %s\n", SDLNet_GetError());
                            }
                        }

                        {
                            struct message_id message;
                            message.type = MESSAGE_CONNECT_BROADCAST;
                            message.id = new_client_id;
                            for (size_t i = 0; i < MAX_CLIENTS; i++)
                            {
                                if (clients[i].id != -1 && clients[i].id != clients[new_client_id].id)
                                {
                                    if (SDLNet_TCP_Send(clients[i].socket, &message, sizeof(message)) < (int)sizeof(message))
                                    {
                                        printf("Error: Failed to send TCP packet: %s\n", SDLNet_GetError());
                                    }
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

            for (size_t i = 0; i < MAX_CLIENTS; i++)
            {
                if (clients[i].id != -1)
                {
                    if (SDLNet_SocketReady(clients[i].socket))
                    {
                        char data[PACKET_SIZE];
                        if (SDLNet_TCP_Recv(clients[i].socket, data, sizeof(data)) > 1)
                        {
                            enum message_type type = ((struct message *)data)->type;
                            switch (type)
                            {
                            case MESSAGE_DISCONNECT_REQUEST:
                            {
                                printf("Client %zd disconnected\n", clients[i].id);

                                struct message_id message;
                                message.type = MESSAGE_DISCONNECT_BROADCAST;
                                message.id = clients[i].id;
                                for (size_t j = 0; j < MAX_CLIENTS; j++)
                                {
                                    if (clients[j].id != -1 && clients[j].id != clients[i].id)
                                    {
                                        if (SDLNet_TCP_Send(clients[j].socket, &message, sizeof(message)) < (int)sizeof(message))
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
                            case MESSAGE_ATTACK_REQUEST:
                            {
                                printf("Client %zd attacking\n", clients[i].id);

                                player_attack(&clients[i].player, &world.maps[clients[i].player.map_index]);
                            }
                            break;
                            case MESSAGE_CHANGE_MAP_REQUEST:
                            {
                                struct message_change_map *message = (struct message_change_map *)data;

                                printf("Client %zd changing map to %zd\n", clients[i].id, message->map_index);

                                clients[i].player.map_index = message->map_index;
                            }
                            break;
                            default:
                            {
                                printf("Error: Unknown TCP packet type: %d\n", type);
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
                    enum message_type type = ((struct message *)packet->data)->type;
                    switch (type)
                    {
                    case MESSAGE_UDP_CONNECT_REQUEST:
                    {
                        struct message_id *message = (struct message_id *)packet->data;

                        clients[message->id].udp_address = packet->address;

                        printf("Saving UDP info of client %zd\n", message->id);
                    }
                    break;
                    case MESSAGE_INPUT_REQUEST:
                    {
                        struct message_input *message = (struct message_input *)packet->data;

                        clients[message->id].input.dx = message->input.dx;
                        clients[message->id].input.dy = message->input.dy;
                    }
                    break;
                    default:
                    {
                        printf("Error: Unknown UDP packet type: %d\n", type);
                    }
                    break;
                    }
                }
                SDLNet_FreePacket(packet);
            }
        }

        for (size_t i = 0; i < MAX_CLIENTS; i++)
        {
            if (clients[i].id != -1)
            {
                clients[i].player.acc_x = (float)clients[i].input.dx;
                clients[i].player.acc_y = (float)clients[i].input.dy;

                struct player *player = &clients[i].player;
                player_accelerate(player, &world.maps[clients[i].player.map_index], delta_time);
            }
        }

        for (size_t i = 0; i < world.num_maps; i++)
        {
            map_update(&world.maps[i], delta_time);
        }

        static float update_clients_timer = 0;
        update_clients_timer += delta_time;
        if (update_clients_timer >= 1.0f / UPDATE_CLIENTS_RATE)
        {
            update_clients_timer = 0;

            for (size_t i = 0; i < MAX_CLIENTS; i++)
            {
                if (clients[i].id != -1)
                {
                    UDPpacket *packet = SDLNet_AllocPacket(PACKET_SIZE);

                    struct message_game_state *message = malloc(sizeof(*message));
                    message->type = MESSAGE_GAME_STATE_BROADCAST;
                    for (size_t j = 0; j < MAX_CLIENTS; j++)
                    {
                        message->clients[j].id = clients[j].id;
                        message->clients[j].player.map_index = clients[j].player.map_index;
                        message->clients[j].player.pos_x = clients[j].player.pos_x;
                        message->clients[j].player.pos_y = clients[j].player.pos_y;
                        message->clients[j].player.vel_x = clients[j].player.vel_x;
                        message->clients[j].player.vel_y = clients[j].player.vel_y;
                        message->clients[j].player.acc_x = clients[j].player.acc_x;
                        message->clients[j].player.acc_y = clients[j].player.acc_y;
                    }
                    for (size_t j = 0; j < MAX_MOBS; j++)
                    {
                        message->mobs[j].alive = world.maps[clients[i].player.map_index].mobs[j].alive;
                        message->mobs[j].x = world.maps[clients[i].player.map_index].mobs[j].x;
                        message->mobs[j].y = world.maps[clients[i].player.map_index].mobs[j].y;
                    }

                    packet->address = clients[i].udp_address;
                    packet->data = (uint8_t *)message;
                    packet->len = sizeof(*message);

                    if (!SDLNet_UDP_Send(udp_socket, -1, packet))
                    {
                        printf("Error: Failed to send UDP packet\n");
                    }

                    SDLNet_FreePacket(packet);
                }
            }
        }

        uint32_t frame_end = SDL_GetTicks();
        uint32_t frame_time = frame_end - frame_start;
        if (FRAME_DELAY > frame_time)
        {
            SDL_Delay(FRAME_DELAY - frame_time);
        }
    }

    // TODO: inform clients that server shut down
    for (size_t i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].id != -1)
        {
            SDLNet_TCP_DelSocket(socket_set, clients[i].socket);
            SDLNet_TCP_Close(clients[i].socket);
        }
    }

    world_unload(&world, true);

    SDLNet_UDP_DelSocket(socket_set, udp_socket);
    SDLNet_TCP_DelSocket(socket_set, tcp_socket);

    SDLNet_FreeSocketSet(socket_set);

    SDLNet_UDP_Close(udp_socket);
    SDLNet_TCP_Close(tcp_socket);

    SDLNet_Quit();

    SDL_Quit();

    return 0;
}
