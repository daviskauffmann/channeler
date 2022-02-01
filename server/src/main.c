#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>
#include <shared/conversations.h>
#include <shared/map.h>
#include <shared/message.h>
#include <shared/player.h>
#include <shared/quests.h>
#include <shared/world.h>
#include <stdbool.h>
#include <stdio.h>

#define SERVER_PORT 8492

#define TICK_RATE 60
#define FRAME_DELAY (1000 / TICK_RATE)

#define UPDATE_CLIENTS_TCP_RATE 2.f
#define UPDATE_POSITIONS_RATE 20.f

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
        clients[i].id = CLIENT_ID_UNUSED;
    }

    struct world world;
    world_load(&world, "assets/world.json", true);

    struct quests quests;
    quests_load(&quests, "assets/quests.json");

    struct conversations conversations;
    conversations_load(&conversations, "assets/conversations.json");

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
                    size_t new_client_id = CLIENT_ID_UNUSED;
                    for (size_t i = 0; i < MAX_CLIENTS; i++)
                    {
                        if (clients[i].id == CLIENT_ID_UNUSED)
                        {
                            new_client_id = i;
                            break;
                        }
                    }
                    if (new_client_id != CLIENT_ID_UNUSED)
                    {
                        printf("Connected to client, assigning ID %zd\n", new_client_id);

                        clients[new_client_id].id = new_client_id;
                        clients[new_client_id].socket = socket;
                        player_init(&clients[new_client_id].player, 0);

                        SDLNet_TCP_AddSocket(socket_set, clients[new_client_id].socket);

                        {
                            struct message_connect message;
                            message.type = MESSAGE_SERVER_CONNECT_OK;
                            message.id = new_client_id;
                            message.map_index = 0;

                            if (SDLNet_TCP_Send(socket, &message, sizeof(message)) < (int)sizeof(message))
                            {
                                printf("Error: Failed to send TCP packet: %s\n", SDLNet_GetError());
                            }
                        }

                        {
                            struct message_id message;
                            message.type = MESSAGE_CLIENT_CONNECT;
                            message.id = new_client_id;

                            for (size_t i = 0; i < MAX_CLIENTS; i++)
                            {
                                if (clients[i].id != CLIENT_ID_UNUSED && clients[i].id != clients[new_client_id].id)
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
                        message.type = MESSAGE_SERVER_FULL;

                        if (SDLNet_TCP_Send(socket, &message, sizeof(message)) < (int)sizeof(message))
                        {
                            printf("Error: Failed to send TCP packet: %s\n", SDLNet_GetError());
                        }
                    }
                }
            }

            for (size_t i = 0; i < MAX_CLIENTS; i++)
            {
                if (clients[i].id != CLIENT_ID_UNUSED)
                {
                    if (SDLNet_SocketReady(clients[i].socket))
                    {
                        char data[PACKET_SIZE];
                        if (SDLNet_TCP_Recv(clients[i].socket, data, sizeof(data)) > 1)
                        {
                            enum message_type type = ((struct message *)data)->type;
                            switch (type)
                            {
                            case MESSAGE_DISCONNECT:
                            {
                                printf("Client %zd disconnected\n", clients[i].id);

                                struct message_id message;
                                message.type = MESSAGE_CLIENT_DISCONNECT;
                                message.id = clients[i].id;

                                for (size_t j = 0; j < MAX_CLIENTS; j++)
                                {
                                    if (clients[j].id != CLIENT_ID_UNUSED && clients[j].id != clients[i].id)
                                    {
                                        if (SDLNet_TCP_Send(clients[j].socket, &message, sizeof(message)) < (int)sizeof(message))
                                        {
                                            printf("Error: Failed to send TCP packet: %s\n", SDLNet_GetError());
                                        }
                                    }
                                }

                                SDLNet_TCP_DelSocket(socket_set, clients[i].socket);
                                SDLNet_TCP_Close(clients[i].socket);

                                clients[i].id = CLIENT_ID_UNUSED;
                                clients[i].socket = NULL;
                            }
                            break;
                            case MESSAGE_ATTACK:
                            {
                                printf("Client %zd attacking\n", clients[i].id);

                                player_attack(&clients[i].player, &world.maps[clients[i].player.map_index]);
                            }
                            break;
                            case MESSAGE_CHANGE_MAP:
                            {
                                struct message_change_map *message = (struct message_change_map *)data;

                                printf("Client %zd changing map to %zd\n", clients[i].id, message->map_index);

                                clients[i].player.map_index = message->map_index;
                            }
                            break;
                            case MESSAGE_START_CONVERSATION:
                            {
                                struct message_start_conversation *message = (struct message_start_conversation *)data;

                                printf("Client %zd starting conversation %zd\n", clients[i].id, message->conversation_index);

                                player_start_conversation(&clients[i].player, &conversations, message->conversation_index);
                            }
                            break;
                            case MESSAGE_ADVANCE_CONVERSATION:
                            {
                                struct message *message = (struct message *)data;

                                printf("Client %zd advancing conversation\n", clients[i].id);

                                player_advance_conversation(&clients[i].player);
                            }
                            break;
                            case MESSAGE_CHOOSE_CONVERSATION_RESPONSE:
                            {
                                struct message_choose_conversation_response *message = (struct message_choose_conversation_response *)data;

                                printf("Client %zd choosing conversation response %zd\n", clients[i].id, message->choice_index);

                                player_choose_conversation_response(&clients[i].player, message->choice_index);
                            }
                            break;
                            case MESSAGE_END_CONVERSATION:
                            {
                                struct message *message = (struct message *)data;

                                printf("Client %zd ending conversation\n", clients[i].id);

                                player_end_conversation(&clients[i].player);
                            }
                            break;
                            case MESSAGE_QUEST_STATUS:
                            {
                                struct message_quest_status *message = (struct message_quest_status *)data;

                                printf("Client %zd setting quest %zd to stage %zd\n", clients[i].id, message->quest_status.quest_index, message->quest_status.stage_index);

                                player_set_quest_status(&clients[i].player, message->quest_status);
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
                    case MESSAGE_UDP_CONNECT:
                    {
                        struct message_id *message = (struct message_id *)packet->data;

                        clients[message->id].udp_address = packet->address;

                        printf("Saving UDP info of client %zd\n", message->id);
                    }
                    break;
                    case MESSAGE_INPUT:
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
            if (clients[i].id != CLIENT_ID_UNUSED)
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

        static float update_clients_tcp_timer = 0;
        update_clients_tcp_timer += delta_time;
        if (update_clients_tcp_timer >= 1 / UPDATE_CLIENTS_TCP_RATE)
        {
            update_clients_tcp_timer = 0;

            struct message_update_stuff *message = malloc(sizeof(*message));
            message->type = MESSAGE_UPDATE_STUFF;

            for (size_t i = 0; i < MAX_CLIENTS; i++)
            {
                message->clients[i].id = clients[i].id;

                if (clients[i].id != CLIENT_ID_UNUSED)
                {
                    message->clients[i].player.map_index = clients[i].player.map_index;

                    message->clients[i].player.in_conversation = clients[i].player.conversation_root != NULL;
                    if (message->clients[i].player.in_conversation)
                    {
                        message->clients[i].player.conversation_index = clients[i].player.conversation_root->index;
                        message->clients[i].player.conversation_local_index = clients[i].player.conversation_node->local_index;
                    }
                }
            }

            for (size_t i = 0; i < MAX_CLIENTS; i++)
            {
                if (clients[i].id != CLIENT_ID_UNUSED)
                {
                    if (SDLNet_TCP_Send(clients[i].socket, message, (int)sizeof(*message)) < (int)sizeof(*message))
                    {
                        printf("Error: Failed to send TCP packet: %s\n", SDLNet_GetError());
                    }
                }
            }

            free(message);
        }

        static float update_positions_timer = 0;
        update_positions_timer += delta_time;
        if (update_positions_timer >= 1 / UPDATE_POSITIONS_RATE)
        {
            update_positions_timer = 0;

            for (size_t i = 0; i < MAX_CLIENTS; i++)
            {
                if (clients[i].id != CLIENT_ID_UNUSED)
                {
                    struct message_update_positions *message = malloc(sizeof(*message));
                    message->type = MESSAGE_UPDATE_POSITIONS;
                    for (size_t j = 0; j < MAX_CLIENTS; j++)
                    {
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

                    UDPpacket *packet = SDLNet_AllocPacket(PACKET_SIZE);
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
        if (clients[i].id != CLIENT_ID_UNUSED)
        {
            player_uninit(&clients[i].player);

            SDLNet_TCP_DelSocket(socket_set, clients[i].socket);
            SDLNet_TCP_Close(clients[i].socket);
        }
    }

    world_unload(&world, true);
    quests_unload(&quests);

    SDLNet_UDP_DelSocket(socket_set, udp_socket);
    SDLNet_TCP_DelSocket(socket_set, tcp_socket);

    SDLNet_FreeSocketSet(socket_set);

    SDLNet_UDP_Close(udp_socket);
    SDLNet_TCP_Close(tcp_socket);

    SDLNet_Quit();

    SDL_Quit();

    return 0;
}
