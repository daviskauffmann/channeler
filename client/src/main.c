#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_net.h>
#include <SDL2/SDL_ttf.h>
#include <shared/clients.h>
#include <shared/conversations.h>
#include <shared/input.h>
#include <shared/map.h>
#include <shared/net.h>
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

struct active_map
{
    struct map *map;
    SDL_Texture **sprites;
};

void deactivate_map(struct active_map *active_map)
{
    for (size_t i = 0; i < active_map->map->num_tilesets; i++)
    {
        printf("Destroying texture: %s\n", active_map->map->tilesets[i].image);

        SDL_DestroyTexture(active_map->sprites[i]);
    }
    free(active_map->sprites);
}

void switch_map(struct active_map *active_map, struct world *world, size_t map_index, SDL_Renderer *renderer)
{
    if (active_map->map)
    {
        deactivate_map(active_map);
    }

    active_map->map = &world->maps[map_index];

    active_map->sprites = malloc(active_map->map->num_tilesets * sizeof(*active_map->sprites));
    for (size_t i = 0; i < active_map->map->num_tilesets; i++)
    {
        printf("Loading texture: %s\n", active_map->map->tilesets[i].image);

        active_map->sprites[i] = IMG_LoadTexture(renderer, active_map->map->tilesets[i].image);
    }
}

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
    SDL_SetWindowTitle(window, WINDOW_TITLE);

    if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 1024) != 0)
    {
        printf("Error: Failed to initialize the mixer API: %s\n", Mix_GetError());
        return 1;
    }

    TTF_Font *font = TTF_OpenFont("assets/VeraMono.ttf", 24);

    SDL_RenderClear(renderer);
    draw_text(renderer, font, 12, 0, 0, WINDOW_WIDTH, (SDL_Color){255, 255, 255}, "Connecting to server...");
    SDL_RenderPresent(renderer);

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

    printf("Connected to %s:%d\n", SERVER_HOST, SERVER_PORT);

    clients_init();

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
            case MESSAGE_SERVER_CONNECT_OK:
            {
                struct message_connect *message_connect = (struct message_connect *)data;

                client_id = message_connect->id;
                map_index = message_connect->map_index;
                printf("ID %zd assigned by server\n", client_id);
            }
            break;
            case MESSAGE_SERVER_FULL:
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
    }

    if (online)
    {
        struct message_id *message = (struct message_id *)malloc(sizeof(*message));
        message->type = MESSAGE_UDP_CONNECT;
        message->id = client_id;
        if (!udp_send(udp_socket, server_address, message, sizeof(*message)))
        {
            printf("Error: Failed to make UDP connection\n");
            online = false;
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

    clients[client_id].id = client_id;
    struct player *player = &clients[client_id].player;
    player_init(player, client_id, NULL, map_index);

    // TODO: files to load should be sent from the server
    // first pass will be just giving a filename that the client is expected to have locally and erroring if not
    // second pass might be implementing file transfer from the server if the client does not have the world data
    struct world world;
    world_load(&world, "assets/world.json");

    struct quests quests;
    quests_load(&quests, "assets/quests.json");

    struct conversations conversations;
    conversations_load(&conversations, "assets/conversations.json");

    struct active_map active_map;
    active_map.map = NULL;
    active_map.sprites = NULL;
    switch_map(&active_map, &world, map_index, renderer);

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
                    case MESSAGE_CLIENT_CONNECT:
                    {
                        struct message_id *message = (struct message_id *)data;

                        clients[message->id].id = message->id;

                        printf("Client with ID %zd has joined\n", message->id);
                    }
                    break;
                    case MESSAGE_CLIENT_DISCONNECT:
                    {
                        struct message_id *message = (struct message_id *)data;

                        clients[message->id].id = CLIENT_ID_UNUSED;

                        printf("Client with ID %zd has disconnected\n", message->id);
                    }
                    break;
                    case MESSAGE_QUEST_STATUS:
                    {
                        struct message_quest_status_broadcast *message = (struct message_quest_status_broadcast *)data;

                        player_set_quest_status(&clients[message->id].player, message->quest_status);

                        printf("Setting quest %zd to state %zd for client %zd\n", message->quest_status.quest_index, message->quest_status.stage_index, message->id);
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
                    case MESSAGE_SERVER_SHUTDOWN:
                    {
                        online = false;
                        quit = true;
                    }
                    break;
                    case MESSAGE_GAME_STATE:
                    {
                        struct message_game_state *message = (struct message_game_state *)packet->data;

                        for (size_t i = 0; i < MAX_CLIENTS; i++)
                        {
                            clients[i].id = message->clients[i].id;

                            if (clients[i].id != CLIENT_ID_UNUSED)
                            {
                                clients[i].player.map_index = message->clients[i].player.map_index;

                                clients[i].player.pos_x = message->clients[i].player.pos_x;
                                clients[i].player.pos_y = message->clients[i].player.pos_y;
                                clients[i].player.vel_x = message->clients[i].player.vel_x;
                                clients[i].player.vel_y = message->clients[i].player.vel_y;
                                clients[i].player.acc_x = message->clients[i].player.acc_x;
                                clients[i].player.acc_y = message->clients[i].player.acc_y;

                                if (message->clients[i].player.in_conversation)
                                {
                                    clients[i].player.conversation_root = &conversations.conversations[message->clients[i].player.conversation_index];
                                    clients[i].player.conversation_node = conversation_find_by_local_index(clients[i].player.conversation_root, message->clients[i].player.conversation_local_index);
                                }
                                else
                                {
                                    clients[i].player.conversation_root = NULL;
                                    clients[i].player.conversation_node = NULL;
                                }
                            }
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
                    quest_log_open = false;

                    if (online)
                    {
                        struct message message;
                        message.type = MESSAGE_END_CONVERSATION;
                        tcp_send(tcp_socket, &message, sizeof(message));
                    }
                    else
                    {
                        player_end_conversation(player);
                    }
                }
                break;
                case SDLK_SPACE:
                {
                    if (player->conversation_node)
                    {
                        if (online)
                        {
                            struct message message;
                            message.type = MESSAGE_ADVANCE_CONVERSATION;
                            tcp_send(tcp_socket, &message, sizeof(message));
                        }
                        else
                        {
                            player_advance_conversation(player);
                        }
                    }
                    else
                    {
                        if (online)
                        {
                            struct message message;
                            message.type = MESSAGE_ATTACK;
                            tcp_send(tcp_socket, &message, sizeof(message));
                        }
                        else
                        {
                            player_attack(player, active_map.map);
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
                        size_t choice_index = event.key.keysym.sym - 48;

                        if (online)
                        {
                            struct message_choose_conversation_response message;
                            message.type = MESSAGE_CHOOSE_CONVERSATION_RESPONSE;
                            message.choice_index = choice_index;
                            tcp_send(tcp_socket, &message, sizeof(message));
                        }
                        else
                        {
                            player_choose_conversation_response(player, choice_index);
                        }
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
                    size_t map_index = 0;

                    if (online)
                    {
                        struct message_change_map message;
                        message.type = MESSAGE_CHANGE_MAP;
                        message.map_index = map_index;
                        tcp_send(tcp_socket, &message, sizeof(message));
                    }
                    else
                    {
                        player->map_index = map_index;
                    }
                }
                break;
                case SDLK_F2:
                {
                    size_t map_index = 1;

                    if (online)
                    {
                        struct message_change_map message;
                        message.type = MESSAGE_CHANGE_MAP;
                        message.map_index = map_index;
                        tcp_send(tcp_socket, &message, sizeof(message));
                    }
                    else
                    {
                        player->map_index = map_index;
                    }
                }
                break;
                case SDLK_F3:
                {
                    size_t conversation_index = 0;

                    if (online)
                    {
                        struct message_start_conversation message;
                        message.type = MESSAGE_START_CONVERSATION;
                        message.conversation_index = conversation_index;
                        tcp_send(tcp_socket, &message, sizeof(message));
                    }
                    else
                    {
                        player_start_conversation(player, &conversations, conversation_index);
                    }
                }
                break;
                case SDLK_F4:
                {
                    size_t conversation_index = 1;

                    if (online)
                    {
                        struct message_start_conversation message;
                        message.type = MESSAGE_START_CONVERSATION;
                        message.conversation_index = conversation_index;
                        tcp_send(tcp_socket, &message, sizeof(message));
                    }
                    else
                    {
                        player_start_conversation(player, &conversations, conversation_index);
                    }
                }
                break;
                case SDLK_F5:
                {
                    struct quest_status quest_status;
                    quest_status.quest_index = 0;
                    quest_status.stage_index = 1;

                    if (online)
                    {
                        struct message_quest_status message;
                        message.type = MESSAGE_QUEST_STATUS;
                        message.quest_status = quest_status;
                        tcp_send(tcp_socket, &message, sizeof(message));
                    }
                    else
                    {
                        player_set_quest_status(player, quest_status);
                    }
                }
                break;
                case SDLK_F6:
                {
                    struct quest_status quest_status;
                    quest_status.quest_index = 0;
                    quest_status.stage_index = 3;

                    if (online)
                    {
                        struct message_quest_status message;
                        message.type = MESSAGE_QUEST_STATUS;
                        message.quest_status = quest_status;
                        tcp_send(tcp_socket, &message, sizeof(message));
                    }
                    else
                    {
                        player_set_quest_status(player, quest_status);
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
            switch_map(&active_map, &world, map_index, renderer);
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
            message->type = MESSAGE_INPUT;
            message->id = client_id;
            message->input.dx = input.dx;
            message->input.dy = input.dy;
            udp_send(udp_socket, server_address, message, sizeof(*message));
        }

        player->acc_x = (float)input.dx;
        player->acc_y = (float)input.dy;

        for (size_t i = 0; i < MAX_CLIENTS; i++)
        {
            if (clients[i].id != CLIENT_ID_UNUSED && clients[i].player.map_index == player->map_index)
            {
                struct player *player = &clients[i].player;
                player_accelerate(player, active_map.map, delta_time);
            }
        }

        if (online)
        {
            // update current map only for prediction purposes?
            // map_update(active_map.map, delta_time);
        }
        else
        {
            for (size_t i = 0; i < world.num_maps; i++)
            {
                map_update(&world.maps[i], delta_time);
            }
        }

        size_t view_width = WINDOW_WIDTH / SPRITE_SCALE;
        size_t view_height = WINDOW_HEIGHT / SPRITE_SCALE;
        int64_t view_x = (int64_t)player->pos_x - view_width / 2;
        int64_t view_y = (int64_t)player->pos_y - view_height / 2;
        if (view_x + view_width > active_map.map->width * active_map.map->tile_width)
        {
            view_x = (active_map.map->width * active_map.map->tile_width) - view_width;
        }
        if (view_x < 0)
        {
            view_x = 0;
        }
        if (view_y + view_height > active_map.map->height * active_map.map->tile_height)
        {
            view_y = (active_map.map->height * active_map.map->tile_height) - view_height;
        }
        if (view_y < 0)
        {
            view_y = 0;
        }

        SDL_RenderClear(renderer);

        for (int64_t y = view_y / active_map.map->tile_height; y <= (int64_t)((view_y + view_height) / active_map.map->tile_height); y++)
        {
            for (int64_t x = view_x / active_map.map->tile_width; x <= (int64_t)((view_x + view_width) / active_map.map->tile_width); x++)
            {
                struct tile *tile = map_get_tile(active_map.map, x, y);
                if (tile)
                {
                    struct tileset *tileset = map_get_tileset(active_map.map, tile->gid);

                    SDL_Rect srcrect = {
                        (int)(((tile->gid - tileset->first_gid) % tileset->columns) * active_map.map->tile_width),
                        (int)(((tile->gid - tileset->first_gid) / tileset->columns) * active_map.map->tile_height),
                        (int)active_map.map->tile_width,
                        (int)active_map.map->tile_height};

                    SDL_Rect dstrect = {
                        (int)(((x * active_map.map->tile_width) - view_x) * SPRITE_SCALE),
                        (int)(((y * active_map.map->tile_height) - view_y) * SPRITE_SCALE),
                        (int)(active_map.map->tile_width * SPRITE_SCALE),
                        (int)(active_map.map->tile_height * SPRITE_SCALE)};

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
                        active_map.sprites[tileset->index],
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
            struct mob *mob = &active_map.map->mobs[i];

            if (mob->alive)
            {
                struct tileset *tileset = map_get_tileset(active_map.map, mob->gid);

                SDL_Rect srcrect = {
                    (int)(((mob->gid - tileset->first_gid) % tileset->columns) * active_map.map->tile_width),
                    (int)(((mob->gid - tileset->first_gid) / tileset->columns) * active_map.map->tile_height),
                    (int)active_map.map->tile_width,
                    (int)active_map.map->tile_height};

                SDL_Rect dstrect = {
                    (int)((mob->x - view_x) * SPRITE_SCALE),
                    (int)((mob->y - view_y) * SPRITE_SCALE),
                    (int)(active_map.map->tile_width * SPRITE_SCALE),
                    (int)(active_map.map->tile_height * SPRITE_SCALE)};

                SDL_RenderCopy(renderer, active_map.sprites[tileset->index], &srcrect, &dstrect);
            }
        }

        for (size_t i = 0; i < MAX_CLIENTS; i++)
        {
            if (clients[i].id != CLIENT_ID_UNUSED && clients[i].player.map_index == player->map_index)
            {
                struct player *player = &clients[i].player;

                // TODO: should be on player struct and come from server
                // should also come from a separate sprite file, makes no sense to use the active map's tilesets
                int64_t player_gid = 32;
                struct tileset *tileset = map_get_tileset(active_map.map, player_gid);

                SDL_Rect srcrect = {
                    (int)(((player_gid - tileset->first_gid) % tileset->columns) * active_map.map->tile_width),
                    (int)(((player_gid - tileset->first_gid) / tileset->columns) * active_map.map->tile_height),
                    (int)active_map.map->tile_width,
                    (int)active_map.map->tile_height};

                SDL_Rect dstrect = {
                    (int)((player->pos_x - view_x) * SPRITE_SCALE),
                    (int)((player->pos_y - view_y) * SPRITE_SCALE),
                    (int)(active_map.map->tile_width * SPRITE_SCALE),
                    (int)(active_map.map->tile_height * SPRITE_SCALE)};

                SDL_RenderCopy(renderer, active_map.sprites[tileset->index], &srcrect, &dstrect);

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
                if (child->type == CONVERSATION_NODE_RESPONSE && conversation_check_conditions(child, player))
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
        message.type = MESSAGE_DISCONNECT;
        tcp_send(tcp_socket, &message, sizeof(message));
    }

    TTF_CloseFont(font);

    deactivate_map(&active_map);

    world_unload(&world);
    quests_unload(&quests);
    conversations_unload(&conversations);

    player_uninit(player);

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
