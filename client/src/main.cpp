#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <enet/enet.h>
#include <iostream>
#include <shared/conversation.hpp>
#include <shared/conversation_list.hpp>
#include <shared/input.hpp>
#include <shared/map.hpp>
#include <shared/message.hpp>
#include <shared/player.hpp>
#include <shared/quest_list.hpp>
#include <shared/tileset.hpp>
#include <shared/world.hpp>
#include <stdarg.h>

constexpr const char *window_title = "Project Hypernova";
constexpr int window_width = 640;
constexpr int window_height = 480;

constexpr const char *server_host = "127.0.0.1";
constexpr std::uint16_t server_port = 8492;

constexpr std::size_t fps_cap = 144;
constexpr std::size_t frame_delay = 1000 / fps_cap;

constexpr std::size_t sprite_scale = 2;

namespace hp_client
{
    class active_map
    {
    public:
        SDL_Renderer *renderer;
        hp::map *map = nullptr;
        std::vector<SDL_Texture *> sprites;

        active_map(SDL_Renderer *renderer)
            : renderer(renderer)
        {
        }

        void change(hp::map *new_map)
        {
            if (map)
            {
                deactivate();
            }

            map = new_map;

            for (const auto &map_tileset : map->map_tilesets)
            {
                sprites.push_back(IMG_LoadTexture(renderer, map_tileset.tileset->sprites_filename.c_str()));
            }
        }

        void deactivate()
        {
            for (auto &sprite : sprites)
            {
                SDL_DestroyTexture(sprite);
            }
            sprites.clear();
        }
    };
};

void draw_text(
    SDL_Renderer *renderer,
    TTF_Font *font,
    const std::size_t x,
    const std::size_t y,
    const std::size_t w,
    SDL_Color fg,
    const char *const fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    auto size = vsnprintf(NULL, 0, fmt, args) + 1;
    auto text = (char *)malloc(size);
    vsprintf_s(text, size, fmt, args);
    va_end(args);

    auto surface = TTF_RenderText_Blended_Wrapped(font, text, fg, (uint32_t)w);
    auto texture = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_Rect dstrect = {
        (int)x,
        (int)y,
        surface->w,
        surface->h};

    SDL_RenderCopy(renderer, texture, nullptr, &dstrect);

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
    free(text);
}

int main(int, char *[])
{
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) != 0)
    {
        return 1;
    }
    atexit(SDL_Quit);

    constexpr int img_flags = IMG_INIT_PNG;
    if (IMG_Init(img_flags) != img_flags)
    {
        return 1;
    }
    atexit(IMG_Quit);

    constexpr int mix_flags = 0;
    if (Mix_Init(mix_flags) != mix_flags)
    {
        return 1;
    }
    atexit(Mix_Quit);

    if (TTF_Init() != 0)
    {
        return 1;
    }
    atexit(TTF_Quit);

    if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 4096) != 0)
    {
        return 1;
    }
    atexit(Mix_CloseAudio);

    if (enet_initialize() != 0)
    {
        return 1;
    }
    atexit(enet_deinitialize);

    SDL_Window *window;
    SDL_Renderer *renderer;
    if (SDL_CreateWindowAndRenderer(window_width, window_height, 0, &window, &renderer) != 0)
    {
        return 1;
    }
    SDL_SetWindowTitle(window, window_title);

    TTF_Font *font = TTF_OpenFont("assets/NinjaAdventure/HUD/Font/NormalFont.ttf", 18);

    SDL_RenderClear(renderer);
    draw_text(renderer, font, 0, 0, window_width, {255, 255, 255}, "Connecting to server...");
    SDL_RenderPresent(renderer);

    hp::client_list client_list;
    auto online = false;
    ENetHost *host = nullptr;
    ENetPeer *peer = nullptr;
    std::size_t client_id = 0;

    host = enet_host_create(NULL, 1, 2, 0, 0);
    if (host)
    {
        ENetAddress address;
        enet_address_set_host(&address, server_host);
        address.port = server_port;

        peer = enet_host_connect(host, &address, 2, 0);
        if (peer)
        {
            ENetEvent event;
            while (enet_host_service(host, &event, 5000) > 0)
            {
                if (event.type == ENET_EVENT_TYPE_CONNECT)
                {
                    std::cout << "Connected to " << server_host << ":" << server_port << std::endl;
                }
                else if (event.type == ENET_EVENT_TYPE_RECEIVE)
                {
                    const auto type = reinterpret_cast<hp::message *>(event.packet->data)->type;

                    std::cout << "Receive " << (int)type << std::endl;

                    if (type == hp::message_type::SERVER_JOINED)
                    {
                        const auto message = reinterpret_cast<hp::message_client_id *>(event.packet->data);

                        std::cout << "Assigned ID " << message->client_id << std::endl;

                        online = true;
                        client_id = message->client_id;

                        break;
                    }
                    else if (type == hp::message_type::SERVER_FULL)
                    {
                        std::cout << "Server full" << std::endl;
                    }
                    else
                    {
                        std::cout << "Unknown server response" << std::endl;
                    }

                    enet_packet_destroy(event.packet);
                }
            }

            if (!online)
            {
                std::cout << "No server response, starting in offline mode" << std::endl;

                enet_peer_reset(peer);
            }
        }
    }

    hp::world world("assets/world.world");
    hp::quest_list quest_list("assets/quest_list.json");
    hp::conversation_list conversation_list("assets/conversation_list.json");

    client_list.clients.at(client_id).id = client_id;
    hp::player &player = client_list.clients.at(client_id).player;

    hp_client::active_map map(renderer);
    map.change(&world.maps.at(player.map_index));

    SDL_Texture *player_sprites = IMG_LoadTexture(renderer, "assets/NinjaAdventure/Actor/Characters/BlueNinja/SpriteSheet.png");

    auto quest_log_open = false;

    auto quit = false;
    while (!quit)
    {
        static std::uint32_t current_time = 0;
        const auto previous_time = current_time;
        current_time = SDL_GetTicks();
        const auto delta_time = (current_time - previous_time) / 1000.0f;

        const auto keys = SDL_GetKeyboardState(nullptr);

        int mouse_x, mouse_y;
        /*const auto mouse = */ SDL_GetMouseState(&mouse_x, &mouse_y);

        {
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
                            const auto flags = SDL_GetWindowFlags(window);
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
                        player.end_conversation();
                    }
                    break;
                    case SDLK_SPACE:
                    {
                        if (player.conversation_node)
                        {
                            player.advance_conversation();
                        }
                        else
                        {
                            player.attack();
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
                        if (player.conversation_node)
                        {
                            std::size_t choice_index = event.key.keysym.sym - 48;
                            player.choose_conversation_response(choice_index);
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
                        player.map_index = 0;
                        map.change(&world.maps.at(player.map_index));
                    }
                    break;
                    case SDLK_F2:
                    {
                        player.map_index = 1;
                        map.change(&world.maps.at(player.map_index));
                    }
                    break;
                    case SDLK_F3:
                    {
                        player.start_conversation(conversation_list, 0);
                    }
                    break;
                    case SDLK_F4:
                    {
                        player.start_conversation(conversation_list, 1);
                    }
                    break;
                    case SDLK_F5:
                    {
                        player.set_quest_status({0, 1});
                    }
                    break;
                    case SDLK_F6:
                    {
                        player.set_quest_status({0, 3});
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
        }

        if (online)
        {
            ENetEvent event;
            /* Wait up to 1000 milliseconds for an event. */
            while (enet_host_service(host, &event, 0) > 0)
            {
                switch (event.type)
                {
                case ENET_EVENT_TYPE_RECEIVE:
                {
                    const auto type = reinterpret_cast<hp::message *>(event.packet->data)->type;

                    std::cout << "Receive " << (int)type << std::endl;

                    switch (type)
                    {
                    case hp::message_type::CLIENT_JOINED:
                    {
                        const auto message = reinterpret_cast<hp::message_client_id *>(event.packet->data);
                        const auto new_client_id = message->client_id;

                        std::cout << "New client ID " << new_client_id << std::endl;

                        client_list.clients.at(new_client_id).id = new_client_id;
                    }
                    break;
                    case hp::message_type::CLIENT_DISCONNECTED:
                    {
                        const auto message = reinterpret_cast<hp::message_client_id *>(event.packet->data);
                        const auto disconnected_client_id = message->client_id;

                        std::cout << "Client disconnected " << disconnected_client_id << std::endl;

                        client_list.clients.at(disconnected_client_id).id = client_list.max_clients;
                    }
                    break;
                    }

                    enet_packet_destroy(event.packet);
                }
                break;
                }
            }
        }

        hp::input input;
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

        player.update(input, *map.map, delta_time);

        for (std::size_t i = 0; i < world.maps.size(); i++)
        {
            world.maps.at(i).update(delta_time);
        }

        const std::size_t view_width = window_width / sprite_scale;
        const std::size_t view_height = window_height / sprite_scale;
        std::int64_t view_x = (std::int64_t)player.pos_x - view_width / 2;
        std::int64_t view_y = (std::int64_t)player.pos_y - view_height / 2;
        if (view_x + view_width > map.map->width * map.map->tile_width)
        {
            view_x = (map.map->width * map.map->tile_width) - view_width;
        }
        if (view_x < 0)
        {
            view_x = 0;
        }
        if (view_y + view_height > map.map->height * map.map->tile_height)
        {
            view_y = (map.map->height * map.map->tile_height) - view_height;
        }
        if (view_y < 0)
        {
            view_y = 0;
        }

        SDL_RenderClear(renderer);

        for (std::int64_t y = view_y / map.map->tile_height; y <= (std::int64_t)((view_y + view_height) / map.map->tile_height); y++)
        {
            for (std::int64_t x = view_x / map.map->tile_width; x <= (std::int64_t)((view_x + view_width) / map.map->tile_width); x++)
            {
                for (const auto &layer : map.map->layers)
                {
                    const auto tile = layer.get_tile(x, y);
                    if (tile)
                    {
                        const auto &map_tileset = map.map->get_map_tileset(tile->gid);

                        const SDL_Rect srcrect = {
                            (int)(((tile->gid - map_tileset.first_gid) % map_tileset.tileset->columns) * map.map->tile_width),
                            (int)(((tile->gid - map_tileset.first_gid) / map_tileset.tileset->columns) * map.map->tile_height),
                            (int)map.map->tile_width,
                            (int)map.map->tile_height};

                        const SDL_Rect dstrect = {
                            (int)(((x * map.map->tile_width) - view_x) * sprite_scale),
                            (int)(((y * map.map->tile_height) - view_y) * sprite_scale),
                            (int)(map.map->tile_width * sprite_scale),
                            (int)(map.map->tile_height * sprite_scale)};

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
                            map.sprites.at(map_tileset.index),
                            &srcrect,
                            &dstrect,
                            angle,
                            nullptr,
                            SDL_FLIP_NONE);
                    }
                }
            }
        }

        for (const auto &client : client_list.clients)
        {
            if (client.id != client_list.max_clients)
            {
                SDL_Rect srcrect;
                switch (client.player.animation)
                {
                case hp::animation::IDLE:
                    srcrect.x = (int)client.player.direction * 16;
                    srcrect.y = 0;
                    break;
                case hp::animation::WALKING:
                    srcrect.x = (int)client.player.direction * 16;
                    srcrect.y = (1 + (client.player.frame_index % 3)) * 16;
                    break;
                }
                srcrect.w = 16;
                srcrect.h = 16;

                const SDL_Rect dstrect = {
                    (int)((client.player.pos_x - view_x) * sprite_scale),
                    (int)((client.player.pos_y - view_y) * sprite_scale),
                    (int)(16 * sprite_scale),
                    (int)(16 * sprite_scale)};

                SDL_RenderCopy(renderer, player_sprites, &srcrect, &dstrect);

                draw_text(renderer, font, dstrect.x + 24, dstrect.y - (24 * 2), window_width, {255, 255, 255}, "%zu", client.id);
            }
        }

        if (quest_log_open)
        {
            const SDL_Rect rect = {12, 12, window_width - 24, window_height - 24};
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_RenderFillRect(renderer, &rect);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

            for (std::size_t i = 0; i < player.quest_statuses.size(); i++)
            {
                auto status = player.quest_statuses.at(i);
                auto quest = &quest_list.quests.at(status.quest_index);
                auto stage = &quest->stages.at(status.stage_index);
                draw_text(renderer, font, 24, 24 * (i + 1), window_width - 24, {255, 255, 255}, "%s: %s", quest->name.c_str(), stage->description.c_str());
            }
        }

        if (player.conversation_node)
        {
            const SDL_Rect rect = {12, window_height - 100 - 12, window_width - 24, 100};
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_RenderFillRect(renderer, &rect);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

            draw_text(renderer, font, 24, window_height - 100, window_width, {255, 255, 255}, "%s", player.conversation_node->text.c_str());

            for (std::size_t i = 0; i < player.conversation_node->children.size(); i++)
            {
                const auto child = player.conversation_node->children.at(i);
                if (child->type == hp::conversation_type::RESPONSE && child->check_conditions(player))
                {
                    draw_text(renderer, font, 24, (window_height - 100) + 24 * (i + 1), window_width, {255, 255, 255}, "%zu) %s", i + 1, child->text.c_str());
                }
            }
        }

        SDL_RenderPresent(renderer);

        const auto frame_time = SDL_GetTicks() - current_time;
        if (frame_delay > frame_time)
        {
            SDL_Delay(frame_delay - frame_time);
        }
    }

    map.deactivate();

    if (online)
    {
        enet_peer_disconnect(peer, 0);

        auto disconnected = false;

        ENetEvent event;
        while (enet_host_service(host, &event, 3000) > 0)
        {
            if (event.type == ENET_EVENT_TYPE_RECEIVE)
            {
                enet_packet_destroy(event.packet);
            }
            else if (event.type == ENET_EVENT_TYPE_DISCONNECT)
            {
                std::cout << "Disconnected" << std::endl;

                disconnected = true;

                break;
            }
        }

        if (!disconnected)
        {
            enet_peer_reset(peer);
        }
    }

    enet_host_destroy(host);

    TTF_CloseFont(font);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    return 0;
}
