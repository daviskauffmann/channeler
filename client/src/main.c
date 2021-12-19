#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_net.h>
#include <shared/input.h>
#include <shared/map.h>
#include <shared/message.h>
#include <shared/player.h>
#include <shared/tileset.h>
#include <shared/world.h>
#include <stdbool.h>
#include <stdio.h>

#define WINDOW_TITLE "Project Hypernova"
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

#define SERVER_HOST "127.0.0.1"
#define SERVER_PORT 3000

#define FPS_CAP 144
#define FRAME_DELAY (1000 / FPS_CAP)

#define SPRITE_SIZE 16
#define SPRITE_SCALE 2

struct client
{
    int id;
    struct player player;
};

int main(int argc, char *argv[])
{
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) != 0)
    {
        printf("Error: Failed to initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    int img_flags = IMG_INIT_JPG | IMG_INIT_PNG;
    if (IMG_Init(img_flags) != img_flags)
    {
        printf("Error: Failed to initialize SDL_image: %s\n", IMG_GetError());
        return 1;
    }

    int mix_flags = 0;
    if (Mix_Init(mix_flags) != mix_flags)
    {
        printf("Error: Failed to initialize SDL_mixer: %s\n", Mix_GetError());
        return 1;
    }

    if (SDLNet_Init() != 0)
    {
        printf("Error: Failed to initialize SDL_net: %s\n", SDLNet_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        WINDOW_TITLE,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        0);
    if (!window)
    {
        printf("Error: Failed to create window: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED);
    if (!renderer)
    {
        printf("Error: Failed to create renderer: %s\n", SDL_GetError());
        return 1;
    }

    if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 1024) != 0)
    {
        printf("Error: Failed to initialize the mixer API: %s\n", Mix_GetError());
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

    int client_id = -1;
    int map_index = -1;
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
                struct message_id *message_id = (struct message_id *)message;

                client_id = message_id->id;
                map_index = 0; // TODO: add to message
                printf("ID %d assigned by server\n", client_id);
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
            struct message_id *message_id = (struct message_id *)malloc(sizeof(*message_id));
            message_id->type = MESSAGE_UDP_CONNECT_REQUEST;
            message_id->id = client_id;

            UDPpacket *packet = SDLNet_AllocPacket(PACKET_SIZE);
            packet->address = server_address;
            packet->data = (unsigned char *)message_id;
            packet->len = sizeof(*message_id);

            if (!SDLNet_UDP_Send(udp_socket, -1, packet))
            {
                printf("Error: Failed to make UDP connection\n");
                online = false;
            }

            SDLNet_FreePacket(packet);
        }
    }

    struct world world;
    struct map *map;
    struct player *player;

    if (online)
    {
        clients[client_id].id = client_id;
        player = &clients[client_id].player;
        player_init(player, map_index);

        // TODO: file to load should be sent from the server
        // first pass will be just giving a filename that the client is expected to have locally and erroring if not
        // second pass might be implementing file transfer from the server if the client does not have the world data
        world_load(&world, "assets/world.json", false);
        map = &world.maps[map_index];
        map_load(map);
    }
    else
    {
        printf("Starting in offline mode\n");

        client_id = 0;
        map_index = 0;

        clients[client_id].id = client_id;
        player = &clients[client_id].player;
        player_init(player, map_index);

        world_load(&world, "assets/world.json", false);
        map = &world.maps[map_index];
        map_load(map);
    }

    SDL_Texture **sprites = malloc(map->num_tilesets * sizeof(sprites[0]));
    for (int i = 0; i < map->num_tilesets; i++)
    {
        sprites[i] = IMG_LoadTexture(renderer, map->tilesets[i].image);
    }

    bool quit = false;
    while (!quit)
    {
        static unsigned int current_time = 0;
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
                        struct message_id *message_id = (struct message_id *)message;

                        clients[message_id->id].id = message_id->id;

                        printf("Client with ID %d has joined\n", message_id->id);
                    }
                    break;
                    case MESSAGE_DISCONNECT_BROADCAST:
                    {
                        struct message_id *message_id = (struct message_id *)message;

                        clients[message_id->id].id = -1;

                        printf("Client with ID %d has disconnected\n", message_id->id);
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
                    case MESSAGE_GAME_STATE_BROADCAST:
                    {
                        struct message_game_state *message_game_state = (struct message_game_state *)message;

                        for (int i = 0; i < MAX_CLIENTS; i++)
                        {
                            clients[i].id = message_game_state->clients[i].id;
                            clients[i].player.map_index = message_game_state->clients[i].player.map_index;
                            clients[i].player.pos_x = message_game_state->clients[i].player.pos_x;
                            clients[i].player.pos_y = message_game_state->clients[i].player.pos_y;
                            clients[i].player.vel_x = message_game_state->clients[i].player.vel_x;
                            clients[i].player.vel_y = message_game_state->clients[i].player.vel_y;
                            clients[i].player.acc_x = message_game_state->clients[i].player.acc_x;
                            clients[i].player.acc_y = message_game_state->clients[i].player.acc_y;
                        }

                        for (int i = 0; i < MAX_MOBS; i++)
                        {
                            world.maps[player->map_index].mobs[i].alive = message_game_state->mobs[i].alive;
                            world.maps[player->map_index].mobs[i].x = message_game_state->mobs[i].x;
                            world.maps[player->map_index].mobs[i].y = message_game_state->mobs[i].y;
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
                case SDLK_SPACE:
                {
                    player_attack(player, map);

                    if (online)
                    {
                        struct message message;
                        message.type = MESSAGE_ATTACK_REQUEST;

                        if (SDLNet_TCP_Send(tcp_socket, &message, sizeof(message)) < (int)sizeof(message))
                        {
                            printf("Error: Failed to send TCP packet: %s\n", SDLNet_GetError());
                        }
                    }
                }
                break;
                case SDLK_1:
                {
                    player->map_index = 0;

                    if (online)
                    {
                        struct message_change_map message;
                        message.type = MESSAGE_CHANGE_MAP_REQUEST;
                        message.map_index = 0;

                        if (SDLNet_TCP_Send(tcp_socket, &message, sizeof(message)) < (int)sizeof(message))
                        {
                            printf("Error: Failed to send TCP packet: %s\n", SDLNet_GetError());
                        }
                    }
                }
                break;
                case SDLK_2:
                {
                    player->map_index = 1;

                    if (online)
                    {
                        struct message_change_map message;
                        message.type = MESSAGE_CHANGE_MAP_REQUEST;
                        message.map_index = 1;

                        if (SDLNet_TCP_Send(tcp_socket, &message, sizeof(message)) < (int)sizeof(message))
                        {
                            printf("Error: Failed to send TCP packet: %s\n", SDLNet_GetError());
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

        if (map_index != player->map_index)
        {
            map_index = player->map_index;

            for (int i = 0; i < map->num_tilesets; i++)
            {
                SDL_DestroyTexture(sprites[i]);
            }
            free(sprites);

            map_unload(map);
            map = &world.maps[map_index];
            map_load(map);

            sprites = malloc(map->num_tilesets * sizeof(sprites[0]));
            for (int i = 0; i < map->num_tilesets; i++)
            {
                sprites[i] = IMG_LoadTexture(renderer, map->tilesets[i].image);
            }
        }

        struct input input;
        input.dx = 0;
        input.dy = 0;

        if (keys[SDL_SCANCODE_W])
        {
            input.dy = -1;
        }
        if (keys[SDL_SCANCODE_A])
        {
            input.dx = -1;
        }
        if (keys[SDL_SCANCODE_S])
        {
            input.dy = 1;
        }
        if (keys[SDL_SCANCODE_D])
        {
            input.dx = 1;
        }

        if (online)
        {
            struct message_input *message_input = (struct message_input *)malloc(sizeof(*message_input));
            message_input->type = MESSAGE_INPUT_REQUEST;
            message_input->id = client_id;
            message_input->input.dx = input.dx;
            message_input->input.dy = input.dy;

            UDPpacket *packet = SDLNet_AllocPacket(PACKET_SIZE);
            packet->address = server_address;
            packet->data = (unsigned char *)message_input;
            packet->len = sizeof(*message_input);

            if (!SDLNet_UDP_Send(udp_socket, -1, packet))
            {
                printf("Error: Failed to send UDP packet\n");
            }

            SDLNet_FreePacket(packet);
        }

        player->acc_x = (float)input.dx;
        player->acc_y = (float)input.dy;

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (clients[i].id != -1 && clients[i].player.map_index == player->map_index)
            {
                struct player *player = &clients[i].player;
                player_accelerate(player, map, delta_time);

                player->acc_x = 0;
                player->acc_y = 0;
            }
        }

        if (!online)
        {
            map_update(map, delta_time);
        }

        int view_width = WINDOW_WIDTH / SPRITE_SCALE;
        int view_height = WINDOW_HEIGHT / SPRITE_SCALE;
        int view_x = (int)player->pos_x - view_width / 2;
        int view_y = (int)player->pos_y - view_height / 2;
        if (view_x + view_width > map->width * map->tile_width)
        {
            view_x = (map->width * map->tile_width) - view_width;
        }
        if (view_x < 0)
        {
            view_x = 0;
        }
        if (view_y + view_height > map->height * map->tile_height)
        {
            view_y = (map->height * map->tile_height) - view_height;
        }
        if (view_y < 0)
        {
            view_y = 0;
        }

        SDL_RenderClear(renderer);

        for (int y = view_y / map->tile_height; y <= (view_y + view_height) / map->tile_height; y++)
        {
            for (int x = view_x / map->tile_width; x <= (view_x + view_width) / map->tile_width; x++)
            {
                struct tile *tile = map_get_tile(map, x, y);
                if (tile)
                {
                    struct tileset *tileset = map_get_tileset(map, tile->gid);

                    SDL_Rect srcrect = {
                        ((tile->gid - tileset->first_gid) % tileset->columns) * map->tile_width,
                        ((tile->gid - tileset->first_gid) / tileset->columns) * map->tile_height,
                        map->tile_width,
                        map->tile_height};

                    SDL_Rect dstrect = {
                        ((x * map->tile_width) - view_x) * SPRITE_SCALE,
                        ((y * map->tile_height) - view_y) * SPRITE_SCALE,
                        map->tile_width * SPRITE_SCALE,
                        map->tile_height * SPRITE_SCALE};

                    double angle = 0;
                    if (tile->d_flip)
                    {
                        if (tile->h_flip)
                        {
                            angle = 90;
                        }
                        if (tile->v_flip)
                        {
                            angle = 270;
                        }
                    }
                    else
                    {
                        if (tile->h_flip && tile->v_flip)
                        {
                            angle = 180;
                        }
                    }

                    SDL_RenderCopyEx(
                        renderer,
                        sprites[tileset->index],
                        &srcrect,
                        &dstrect,
                        angle,
                        NULL,
                        SDL_FLIP_NONE);
                }
            }
        }

        for (int i = 0; i < MAX_MOBS; i++)
        {
            struct mob *mob = &map->mobs[i];

            if (map->mobs[i].alive)
            {
                struct tileset *tileset = map_get_tileset(map, mob->gid);

                SDL_Rect srcrect = {
                    ((mob->gid - tileset->first_gid) % tileset->columns) * map->tile_width,
                    ((mob->gid - tileset->first_gid) / tileset->columns) * map->tile_height,
                    map->tile_width,
                    map->tile_height};

                SDL_Rect dstrect = {
                    ((int)map->mobs[i].x - view_x) * SPRITE_SCALE,
                    ((int)map->mobs[i].y - view_y) * SPRITE_SCALE,
                    map->tile_width * SPRITE_SCALE,
                    map->tile_height * SPRITE_SCALE};

                SDL_RenderCopy(renderer, sprites[tileset->index], &srcrect, &dstrect);
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (clients[i].id != -1 && clients[i].player.map_index == player->map_index)
            {
                int player_gid = 32;
                struct tileset *tileset = map_get_tileset(map, player_gid);

                SDL_Rect srcrect = {
                    ((player_gid - tileset->first_gid) % tileset->columns) * map->tile_width,
                    ((player_gid - tileset->first_gid) / tileset->columns) * map->tile_height,
                    map->tile_width,
                    map->tile_height};

                SDL_Rect dstrect = {
                    ((int)clients[i].player.pos_x - view_x) * SPRITE_SCALE,
                    ((int)clients[i].player.pos_y - view_y) * SPRITE_SCALE,
                    map->tile_width * SPRITE_SCALE,
                    map->tile_height * SPRITE_SCALE};

                SDL_RenderCopy(renderer, sprites[tileset->index], &srcrect, &dstrect);
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

    for (int i = 0; i < map->num_tilesets; i++)
    {
        SDL_DestroyTexture(sprites[i]);
    }
    free(sprites);

    map_unload(map);
    world_unload(&world, false);

    SDLNet_UDP_DelSocket(socket_set, udp_socket);
    SDLNet_TCP_DelSocket(socket_set, tcp_socket);

    SDLNet_FreeSocketSet(socket_set);

    SDLNet_UDP_Close(udp_socket);
    SDLNet_TCP_Close(tcp_socket);

    Mix_CloseAudio();

    SDL_DestroyRenderer(renderer);

    SDL_DestroyWindow(window);

    IMG_Quit();
    Mix_Quit();
    SDLNet_Quit();

    SDL_Quit();

    return 0;
}
