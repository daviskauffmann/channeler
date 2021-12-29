#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_net.h>
#include <SDL2/SDL_ttf.h>
#include <shared/conversations.h>
#include <shared/input.h>
#include <shared/map.h>
#include <shared/message.h>
#include <shared/player.h>
#include <shared/quests.h>
#include <shared/tileset.h>
#include <shared/world.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#define WINDOW_TITLE "Project Hypernova"
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

#define SERVER_HOST "127.0.0.1"
#define SERVER_PORT 8492

#define FPS_CAP 144
#define FRAME_DELAY (1000 / FPS_CAP)

#define SPRITE_SIZE 16
#define SPRITE_SCALE 2

struct client
{
    size_t id;
    struct player player;
};

void draw_text(SDL_Renderer *renderer, TTF_Font *font, size_t px, size_t x, size_t y, size_t w, SDL_Color fg, const char *const fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    size_t size = vsnprintf(NULL, 0, fmt, args);
    char *text = malloc(size + 1);
    vsprintf(text, fmt, args);
    va_end(args);

    SDL_Surface *surface = TTF_RenderText_Blended_Wrapped(font, text, fg, (uint32_t)w);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_Rect dstrect = {
        (int)x,
        (int)y,
        surface->w,
        surface->h};

    SDL_RenderCopy(renderer, texture, NULL, &dstrect);

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
    free(text);
}

int main(int argc, char *argv[])
{
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) != 0)
    {
        printf("Error: Failed to initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    int img_flags = IMG_INIT_PNG;
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

    if (TTF_Init() != 0)
    {
        printf("Error: Failed to initialize SDL_ttf: %s\n", TTF_GetError());
        return 1;
    }

    SDL_Window *window;
    SDL_Renderer *renderer;
    if (SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer) != 0)
    {
        printf("Error: Failed to create window and renderer: %s\n", SDL_GetError());
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
    for (size_t i = 0; i < MAX_CLIENTS; i++)
    {
        clients[i].id = CLIENT_ID_UNUSED;
    }

    size_t client_id = 0;
    size_t map_index = 0;
    if (online)
    {
        char data[PACKET_SIZE];
        if (SDLNet_TCP_Recv(tcp_socket, data, sizeof(data)) > 1)
        {
            enum message_type type = ((struct message *)data)->type;
            switch (type)
            {
            case MESSAGE_CONNECT_OK:
            {
                struct message_connect *message_connect = (struct message_connect *)data;

                client_id = message_connect->id;
                map_index = message_connect->map_index;
                printf("ID %zd assigned by server\n", client_id);
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
            packet->data = (uint8_t *)message_id;
            packet->len = sizeof(*message_id);

            if (!SDLNet_UDP_Send(udp_socket, -1, packet))
            {
                printf("Error: Failed to make UDP connection\n");
                online = false;
            }

            SDLNet_FreePacket(packet);
        }
    }

    if (online)
    {
        printf("Connected to %s:%d\n", SERVER_HOST, SERVER_PORT);
    }
    else
    {
        printf("Starting in offline mode\n");
    }

    // TODO: files to load should be sent from the server
    // first pass will be just giving a filename that the client is expected to have locally and erroring if not
    // second pass might be implementing file transfer from the server if the client does not have the world data
    struct world world;
    world_load(&world, "assets/world.json", false);

    struct quests quests;
    quests_load(&quests, "assets/quests.json");

    struct conversations conversations;
    conversations_load(&conversations, "assets/conversations.json");

    struct map *map = &world.maps[map_index];
    map_load(map);

    SDL_Texture **sprites = malloc(map->num_tilesets * sizeof(*sprites));
    for (size_t i = 0; i < map->num_tilesets; i++)
    {
        sprites[i] = IMG_LoadTexture(renderer, map->tilesets[i].image);
    }

    clients[client_id].id = client_id;
    struct player *player = &clients[client_id].player;
    player_init(player, map_index);

    TTF_Font *font = TTF_OpenFont("assets/VeraMono.ttf", 24);

    bool quest_log_open = false;

    bool quit = false;
    while (!quit)
    {
        static uint32_t current_time = 0;
        uint32_t frame_start = SDL_GetTicks();
        uint32_t previous_time = current_time;
        current_time = frame_start;
        float delta_time = (current_time - previous_time) / 1000.0f;

        while (online && SDLNet_CheckSockets(socket_set, 0) > 0)
        {
            if (SDLNet_SocketReady(tcp_socket))
            {
                char data[PACKET_SIZE];
                if (SDLNet_TCP_Recv(tcp_socket, data, sizeof(data)) > 1)
                {
                    enum message_type type = ((struct message *)data)->type;
                    switch (type)
                    {
                    case MESSAGE_CONNECT_BROADCAST:
                    {
                        struct message_id *message = (struct message_id *)data;

                        clients[message->id].id = message->id;

                        printf("Client with ID %zd has joined\n", message->id);
                    }
                    break;
                    case MESSAGE_DISCONNECT_BROADCAST:
                    {
                        struct message_id *message = (struct message_id *)data;

                        clients[message->id].id = CLIENT_ID_UNUSED;

                        printf("Client with ID %zd has disconnected\n", message->id);
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

            if (SDLNet_SocketReady(udp_socket))
            {
                UDPpacket *packet = SDLNet_AllocPacket(PACKET_SIZE);
                if (SDLNet_UDP_Recv(udp_socket, packet) == 1)
                {
                    enum message_type type = ((struct message *)packet->data)->type;
                    switch (type)
                    {
                    case MESSAGE_GAME_STATE_BROADCAST:
                    {
                        struct message_game_state *message = (struct message_game_state *)packet->data;

                        for (size_t i = 0; i < MAX_CLIENTS; i++)
                        {
                            clients[i].id = message->clients[i].id;
                            clients[i].player.map_index = message->clients[i].player.map_index;
                            clients[i].player.pos_x = message->clients[i].player.pos_x;
                            clients[i].player.pos_y = message->clients[i].player.pos_y;
                            clients[i].player.vel_x = message->clients[i].player.vel_x;
                            clients[i].player.vel_y = message->clients[i].player.vel_y;
                            clients[i].player.acc_x = message->clients[i].player.acc_x;
                            clients[i].player.acc_y = message->clients[i].player.acc_y;
                        }

                        for (size_t i = 0; i < MAX_MOBS; i++)
                        {
                            world.maps[player->map_index].mobs[i].alive = message->mobs[i].alive;
                            world.maps[player->map_index].mobs[i].x = message->mobs[i].x;
                            world.maps[player->map_index].mobs[i].y = message->mobs[i].y;
                        }
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

        int num_keys;
        const uint8_t *keys = SDL_GetKeyboardState(&num_keys);

        int mouse_x, mouse_y;
        uint32_t mouse = SDL_GetMouseState(&mouse_x, &mouse_y);

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
                        uint32_t flags = SDL_GetWindowFlags(window);
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
                case SDLK_ESCAPE:
                {
                    player->conversation_node = NULL;
                    quest_log_open = false;
                }
                break;
                case SDLK_SPACE:
                {
                    if (player->conversation_node)
                    {
                        if (player->conversation_node->num_children == 0)
                        {
                            player->conversation_node = NULL;
                        }
                        else
                        {
                            bool has_response_nodes = false;
                            for (size_t i = 0; i < player->conversation_node->num_children; i++)
                            {
                                struct conversation_node *child = &player->conversation_node->children[i];
                                if (child->type == CONVERSATION_NODE_RESPONSE)
                                {
                                    has_response_nodes = true;
                                    break;
                                }
                            }
                            if (!has_response_nodes)
                            {
                                player_advance_conversation(player);
                            }
                        }

                        if (online)
                        {
                            // TODO: send msg
                        }
                    }
                    else
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
                }
                break;
                case SDLK_1:
                case SDLK_2:
                case SDLK_3:
                case SDLK_4:
                case SDLK_5:
                case SDLK_6:
                case SDLK_7:
                case SDLK_8:
                case SDLK_9:
                {
                    if (player->conversation_node)
                    {
                        bool has_response_nodes = false;
                        for (size_t i = 0; i < player->conversation_node->num_children; i++)
                        {
                            struct conversation_node *child = &player->conversation_node->children[i];
                            if (child->type == CONVERSATION_NODE_RESPONSE)
                            {
                                has_response_nodes = true;
                                break;
                            }
                        }
                        if (has_response_nodes)
                        {
                            size_t choice_index = event.key.keysym.sym - 49;
                            if (choice_index < player->conversation_node->num_children)
                            {
                                player->conversation_node = &player->conversation_node->children[choice_index];
                                player_advance_conversation(player);
                            }
                        }

                        if (online)
                        {
                            // TODO: send msg
                        }
                    }
                }
                break;
                case SDLK_e:
                {
                    size_t conversation_index = 0;
                    player->conversation_node = &conversations.conversations[conversation_index];
                    player_advance_conversation(player);

                    if (online)
                    {
                        // TODO: send msg
                    }
                }
                break;
                case SDLK_j:
                {
                    quest_log_open = !quest_log_open;
                }
                break;
                case SDLK_F1:
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
                case SDLK_F2:
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

            for (size_t i = 0; i < map->num_tilesets; i++)
            {
                SDL_DestroyTexture(sprites[i]);
            }
            free(sprites);

            map_unload(map);
            map = &world.maps[map_index];
            map_load(map);

            sprites = malloc(map->num_tilesets * sizeof(*sprites));
            for (size_t i = 0; i < map->num_tilesets; i++)
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
            struct message_input *message = malloc(sizeof(*message));
            message->type = MESSAGE_INPUT_REQUEST;
            message->id = client_id;
            message->input.dx = input.dx;
            message->input.dy = input.dy;

            UDPpacket *packet = SDLNet_AllocPacket(PACKET_SIZE);
            packet->address = server_address;
            packet->data = (uint8_t *)message;
            packet->len = sizeof(*message);

            if (!SDLNet_UDP_Send(udp_socket, -1, packet))
            {
                printf("Error: Failed to send UDP packet\n");
            }

            SDLNet_FreePacket(packet);
        }

        player->acc_x = (float)input.dx;
        player->acc_y = (float)input.dy;

        for (size_t i = 0; i < MAX_CLIENTS; i++)
        {
            if (clients[i].id != CLIENT_ID_UNUSED && clients[i].player.map_index == player->map_index)
            {
                struct player *player = &clients[i].player;
                player_accelerate(player, map, delta_time);
            }
        }

        if (!online)
        {
            map_update(map, delta_time);
        }

        size_t view_width = WINDOW_WIDTH / SPRITE_SCALE;
        size_t view_height = WINDOW_HEIGHT / SPRITE_SCALE;
        int64_t view_x = (int64_t)player->pos_x - view_width / 2;
        int64_t view_y = (int64_t)player->pos_y - view_height / 2;
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

        for (int64_t y = view_y / map->tile_height; y <= (int64_t)((view_y + view_height) / map->tile_height); y++)
        {
            for (int64_t x = view_x / map->tile_width; x <= (int64_t)((view_x + view_width) / map->tile_width); x++)
            {
                struct tile *tile = map_get_tile(map, x, y);
                if (tile)
                {
                    struct tileset *tileset = map_get_tileset(map, tile->gid);

                    SDL_Rect srcrect = {
                        (int)(((tile->gid - tileset->first_gid) % tileset->columns) * map->tile_width),
                        (int)(((tile->gid - tileset->first_gid) / tileset->columns) * map->tile_height),
                        (int)map->tile_width,
                        (int)map->tile_height};

                    SDL_Rect dstrect = {
                        (int)(((x * map->tile_width) - view_x) * SPRITE_SCALE),
                        (int)(((y * map->tile_height) - view_y) * SPRITE_SCALE),
                        (int)(map->tile_width * SPRITE_SCALE),
                        (int)(map->tile_height * SPRITE_SCALE)};

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

        for (size_t i = 0; i < MAX_MOBS; i++)
        {
            struct mob *mob = &map->mobs[i];

            if (map->mobs[i].alive)
            {
                struct tileset *tileset = map_get_tileset(map, mob->gid);

                SDL_Rect srcrect = {
                    (int)(((mob->gid - tileset->first_gid) % tileset->columns) * map->tile_width),
                    (int)(((mob->gid - tileset->first_gid) / tileset->columns) * map->tile_height),
                    (int)map->tile_width,
                    (int)map->tile_height};

                SDL_Rect dstrect = {
                    (int)((map->mobs[i].x - view_x) * SPRITE_SCALE),
                    (int)((map->mobs[i].y - view_y) * SPRITE_SCALE),
                    (int)(map->tile_width * SPRITE_SCALE),
                    (int)(map->tile_height * SPRITE_SCALE)};

                SDL_RenderCopy(renderer, sprites[tileset->index], &srcrect, &dstrect);
            }
        }

        for (size_t i = 0; i < MAX_CLIENTS; i++)
        {
            if (clients[i].id != CLIENT_ID_UNUSED && clients[i].player.map_index == player->map_index)
            {
                int64_t player_gid = 32;
                struct tileset *tileset = map_get_tileset(map, player_gid);

                SDL_Rect srcrect = {
                    (int)(((player_gid - tileset->first_gid) % tileset->columns) * map->tile_width),
                    (int)(((player_gid - tileset->first_gid) / tileset->columns) * map->tile_height),
                    (int)map->tile_width,
                    (int)map->tile_height};

                SDL_Rect dstrect = {
                    (int)((clients[i].player.pos_x - view_x) * SPRITE_SCALE),
                    (int)((clients[i].player.pos_y - view_y) * SPRITE_SCALE),
                    (int)(map->tile_width * SPRITE_SCALE),
                    (int)(map->tile_height * SPRITE_SCALE)};

                SDL_RenderCopy(renderer, sprites[tileset->index], &srcrect, &dstrect);

                draw_text(renderer, font, 12, dstrect.x + 12, dstrect.y - 24, WINDOW_WIDTH, (SDL_Color){255, 255, 255}, "%zd", clients[i].id);
            }
        }

        if (quest_log_open)
        {
            SDL_Rect rect = {12, 12, WINDOW_WIDTH - 24, WINDOW_HEIGHT - 24};
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_RenderFillRect(renderer, &rect);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

            for (size_t i = 0; i < player->num_quest_statuses; i++)
            {
                struct quest_status *status = &player->quest_statuses[i];
                struct quest *quest = &quests.quests[status->quest_index];
                struct quest_stage *stage = &quest->stages[status->stage_index];
                draw_text(renderer, font, 12, 24, 24 * (i + 1), WINDOW_WIDTH - 24, (SDL_Color){255, 255, 255}, "%s: %s", quest->name, stage->description);
            }
        }

        if (player->conversation_node)
        {
            SDL_Rect rect = {12, WINDOW_HEIGHT - 100 - 12, WINDOW_WIDTH - 24, 100};
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_RenderFillRect(renderer, &rect);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

            draw_text(renderer, font, 12, 24, WINDOW_HEIGHT - 100, WINDOW_WIDTH, (SDL_Color){255, 255, 255}, "%s", player->conversation_node->text);

            for (size_t i = 0; i < player->conversation_node->num_children; i++)
            {
                struct conversation_node *child = &player->conversation_node->children[i];
                if (child->type == CONVERSATION_NODE_RESPONSE)
                {
                    draw_text(renderer, font, 12, 24, (WINDOW_HEIGHT - 100) + 24 * (i + 1), WINDOW_WIDTH, (SDL_Color){255, 255, 255}, "%zd) %s", i + 1, child->text);
                }
            }
        }

        SDL_RenderPresent(renderer);

        uint32_t frame_end = SDL_GetTicks();
        uint32_t frame_time = frame_end - frame_start;
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

    TTF_CloseFont(font);

    for (size_t i = 0; i < map->num_tilesets; i++)
    {
        SDL_DestroyTexture(sprites[i]);
    }
    free(sprites);

    map_unload(map);
    player_uninit(player);

    world_unload(&world, false);
    quests_unload(&quests);
    conversations_unload(&conversations);

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
    TTF_Quit();

    SDL_Quit();

    return 0;
}
