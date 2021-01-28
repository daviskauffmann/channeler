#include "client.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_net.h>
#include <stdbool.h>
#include <stdio.h>

#include "message.h"
#include "world.h"

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

#define SERVER_HOST "127.0.0.1"
#define SERVER_PORT 3000

#define FPS_CAP 60
#define FRAME_DELAY (1000 / FPS_CAP)

struct client
{
    int id;
    float x;
    float y;
};

int client_main(int argc, char *argv[])
{
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) != 0)
    {
        printf("Failed to initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    int img_flags = IMG_INIT_JPG | IMG_INIT_PNG;
    if (IMG_Init(img_flags) != img_flags)
    {
        printf("Failed to initialize SDL_image: %s\n", IMG_GetError());
        return 1;
    }

    int mix_flags = 0;
    if (Mix_Init(mix_flags) != mix_flags)
    {
        printf("Failed to initialize SDL_mixer: %s\n", Mix_GetError());
        return 1;
    }

    if (SDLNet_Init() != 0)
    {
        printf("Failed to initialize SDL_net: %s\n", SDLNet_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "Project Hypernova",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        0);
    if (!window)
    {
        printf("Failed to create window: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED);
    if (!renderer)
    {
        printf("Failed to create renderer: %s\n", SDL_GetError());
        return 1;
    }

    if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 1024) != 0)
    {
        printf("Failed to initialize the mixer API: %s\n", Mix_GetError());
        return 1;
    }

    bool online = true;

    IPaddress server_address;
    if (SDLNet_ResolveHost(&server_address, SERVER_HOST, SERVER_PORT))
    {
        printf("Error: Failed to resolve host: %s\n", SDLNet_GetError());
        online = false;
    }

    TCPsocket tcp_socket = SDLNet_TCP_Open(&server_address);
    if (!tcp_socket)
    {
        printf("Error: Failed to open TCP socket: %s\n", SDLNet_GetError());
        online = false;
    }

    UDPsocket udp_socket = SDLNet_UDP_Open(0);
    if (!udp_socket)
    {
        printf("Error: Failed to open UDP socket: %s\n", SDLNet_GetError());
        online = false;
    }

    SDLNet_SocketSet socket_set = SDLNet_AllocSocketSet(2);
    if (!socket_set)
    {
        printf("Error: Failed to allocate socket set: %s\n", SDLNet_GetError());
        online = false;
    }
    SDLNet_TCP_AddSocket(socket_set, tcp_socket);
    SDLNet_UDP_AddSocket(socket_set, udp_socket);

    struct client clients[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        clients[i].id = -1;
    }

    struct world world;

    int client_id = -1;
    if (online)
    {
        char buffer[PACKET_SIZE];
        if (SDLNet_TCP_Recv(tcp_socket, buffer, sizeof(buffer)) > 1)
        {
            struct message *message = (struct message *)buffer;
            switch (message->type)
            {
            case MESSAGE_CONNECT_OK:
            {
                struct connect_ok_message *connect_ok_message = (struct connect_ok_message *)message;

                client_id = connect_ok_message->assigned_id;
                printf("ID %d assigned by server\n", client_id);

                for (int i = 0; i < MAX_CLIENTS; i++)
                {
                    clients[i].id = connect_ok_message->clients[i].id;
                    clients[i].x = connect_ok_message->clients[i].x;
                    clients[i].y = connect_ok_message->clients[i].y;
                }
                for (int i = 0; i < NUM_MOBS; i++)
                {
                    world.mobs[i].alive = connect_ok_message->world.mobs[i].alive;
                    world.mobs[i].x = connect_ok_message->world.mobs[i].x;
                    world.mobs[i].y = connect_ok_message->world.mobs[i].y;
                }
            }
            break;
            case MESSAGE_CONNECT_FULL:
            {
                printf("Error: Server is full\n");
                online = false;
            }
            break;
            default:
            {
                printf("Error: Unknown server response\n");
                online = false;
            }
            break;
            }
        }

        if (online)
        {
            struct id_message *id_message = (struct id_message *)malloc(sizeof(*id_message));
            id_message->type = MESSAGE_UDP_CONNECT_REQUEST;
            id_message->id = client_id;

            UDPpacket *packet = SDLNet_AllocPacket(PACKET_SIZE);
            packet->address = server_address;
            packet->data = (unsigned char *)id_message;
            packet->len = sizeof(*id_message);

            if (!SDLNet_UDP_Send(udp_socket, -1, packet))
            {
                printf("Error: Failed to make UDP connection\n");
                online = false;
            }

            SDLNet_FreePacket(packet);
        }
    }
    else
    {
        client_id = 0;
        clients[client_id].id = 0;
        clients[client_id].x = 100;
        clients[client_id].y = 100;

        world_init(&world);

        printf("Starting in offline mode");
    }

    SDL_Texture *sprites = IMG_LoadTexture(renderer, "assets/sprites.png");

    SDL_Rect player_clip;
    player_clip.x = 493;
    player_clip.y = 0;
    player_clip.w = 16;
    player_clip.h = 16;

    SDL_Rect mob_clip;
    mob_clip.x = 493;
    mob_clip.y = 86;
    mob_clip.w = 16;
    mob_clip.h = 16;

    unsigned int current_time = 0;

    bool quit = false;
    while (!quit)
    {
        unsigned int frame_start = SDL_GetTicks();
        unsigned int previous_time = current_time;
        current_time = frame_start;
        float delta_time = (current_time - previous_time) / 1000.0f;

        while (online && SDLNet_CheckSockets(socket_set, 0) > 0)
        {
            if (SDLNet_SocketReady(tcp_socket))
            {
                char buffer[PACKET_SIZE];
                if (SDLNet_TCP_Recv(tcp_socket, buffer, sizeof(buffer)) > 1)
                {
                    struct message *message = (struct message *)buffer;
                    switch (message->type)
                    {
                    case MESSAGE_CONNECT_BROADCAST:
                    {
                        struct id_message *id_message = (struct id_message *)message;

                        // TODO: initialize new player
                        clients[id_message->id].id = id_message->id;

                        printf("Client with ID %d has joined\n", id_message->id);
                    }
                    break;
                    case MESSAGE_DISCONNECT_BROADCAST:
                    {
                        struct id_message *id_message = (struct id_message *)message;

                        clients[id_message->id].id = -1;

                        printf("Client with ID %d has disconnected\n", id_message->id);
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

            if (SDLNet_SocketReady(udp_socket))
            {
                UDPpacket *packet = SDLNet_AllocPacket(PACKET_SIZE);
                if (SDLNet_UDP_Recv(udp_socket, packet) == 1)
                {
                    struct message *message = (struct message *)packet->data;
                    switch (message->type)
                    {
                    case MESSAGE_WORLD_STATE_BROADCAST:
                    {
                        struct world_state_broadcast_message *world_state_broadcast_message = (struct world_state_broadcast_message *)message;

                        for (int i = 0; i < MAX_CLIENTS; i++)
                        {
                            clients[i].id = world_state_broadcast_message->clients[i].id;
                            clients[i].x = world_state_broadcast_message->clients[i].x;
                            clients[i].y = world_state_broadcast_message->clients[i].y;
                        }
                        for (int i = 0; i < NUM_MOBS; i++)
                        {
                            world.mobs[i].alive = world_state_broadcast_message->world.mobs[i].alive;
                            world.mobs[i].x = world_state_broadcast_message->world.mobs[i].x;
                            world.mobs[i].y = world_state_broadcast_message->world.mobs[i].y;
                        }
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

        int num_keys;
        const unsigned char *keys = SDL_GetKeyboardState(&num_keys);

        int mouse_x, mouse_y;
        unsigned int mouse = SDL_GetMouseState(&mouse_x, &mouse_y);

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_KEYDOWN:
            {
                switch (event.key.keysym.sym)
                {
                case SDLK_RETURN:
                {
                    if (keys[SDL_SCANCODE_LALT])
                    {
                        unsigned int flags = SDL_GetWindowFlags(window);
                        if (flags & SDL_WINDOW_FULLSCREEN_DESKTOP)
                        {
                            SDL_SetWindowFullscreen(window, 0);
                        }
                        else
                        {
                            SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
                        }
                    }
                }
                break;
                }
            }
            break;
            case SDL_QUIT:
            {
                quit = true;
            }
            break;
            }
        }

        float dx = 0;
        float dy = 0;

        if (keys[SDL_SCANCODE_W])
        {
            dy = -1;
        }
        if (keys[SDL_SCANCODE_A])
        {
            dx = -1;
        }
        if (keys[SDL_SCANCODE_S])
        {
            dy = 1;
        }
        if (keys[SDL_SCANCODE_D])
        {
            dx = 1;
        }

        client_move(dx, dy, delta_time, &clients[client_id].x, &clients[client_id].y);

        if (online)
        {
            struct input_request_message *input_request_message = (struct input_request_message *)malloc(sizeof(*input_request_message));
            input_request_message->type = MESSAGE_INPUT_REQUEST;
            input_request_message->id = client_id;
            input_request_message->dx = dx;
            input_request_message->dy = dy;

            UDPpacket *packet = SDLNet_AllocPacket(PACKET_SIZE);
            packet->address = server_address;
            packet->data = (unsigned char *)input_request_message;
            packet->len = sizeof(*input_request_message);

            if (!SDLNet_UDP_Send(udp_socket, -1, packet))
            {
                printf("Error: Failed to send UDP packet\n");
            }

            SDLNet_FreePacket(packet);
        }

        if (!online)
        {
            world_update(&world, delta_time);
        }

        SDL_RenderClear(renderer);

        for (int i = 0; i < NUM_MOBS; i++)
        {
            if (world.mobs[i].alive)
            {
                SDL_Rect render_quad = {world.mobs[i].x, world.mobs[i].y, mob_clip.w * 2, mob_clip.h * 2};
                SDL_RenderCopy(renderer, sprites, &mob_clip, &render_quad);
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (clients[i].id != -1)
            {
                SDL_Rect render_quad = {clients[i].x, clients[i].y, player_clip.w * 2, player_clip.h * 2};
                SDL_RenderCopy(renderer, sprites, &player_clip, &render_quad);
            }
        }

        SDL_RenderPresent(renderer);

        unsigned int frame_end = SDL_GetTicks();
        unsigned int frame_time = frame_end - frame_start;
        if (FRAME_DELAY > frame_time)
        {
            SDL_Delay(FRAME_DELAY - frame_time);
        }
    }

    if (online)
    {
        struct message message;
        message.type = MESSAGE_DISCONNECT_REQUEST;

        if (SDLNet_TCP_Send(tcp_socket, &message, sizeof(message)) < (int)sizeof(message))
        {
            printf("Error: Failed to send TCP packet: %s\n", SDLNet_GetError());
        }
    }

    SDLNet_UDP_DelSocket(socket_set, udp_socket);
    SDLNet_TCP_DelSocket(socket_set, tcp_socket);

    SDLNet_FreeSocketSet(socket_set);

    SDLNet_UDP_Close(udp_socket);
    SDLNet_TCP_Close(tcp_socket);

    Mix_CloseAudio();

    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);

    IMG_Quit();
    Mix_Quit();
    SDLNet_Quit();

    SDL_Quit();

    return 0;
}

void client_move(float dx, float dy, float delta_time, float *new_x, float *new_y)
{
    float speed = 100.0f;

    float dlen = sqrt(dx * dx + dy * dy);
    if (dlen > 1.0f)
    {
        dx *= 1 / dlen;
        dy *= 1 / dlen;
    }

    *new_x += dx * speed * delta_time;
    *new_y += dy * speed * delta_time;
}
