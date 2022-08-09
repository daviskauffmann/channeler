#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <ch/client.hpp>
#include <ch/conversation.hpp>
#include <ch/input.hpp>
#include <ch/map.hpp>
#include <ch/message.hpp>
#include <ch/player.hpp>
#include <ch/server.hpp>
#include <ch/tileset.hpp>
#include <ch/world.hpp>
#include <enet/enet.h>
#include <iostream>
#include <spdlog/spdlog.h>
#include <stdarg.h>

constexpr const char *window_title = "Channeler";
constexpr std::size_t window_width = 640;
constexpr std::size_t window_height = 480;

constexpr const char *server_host = "127.0.0.1";
constexpr std::uint16_t server_port = 8492;

constexpr std::size_t fps_cap = 144;
constexpr std::size_t frame_delay = 1000 / fps_cap;

constexpr std::size_t sprite_scale = 2;

namespace ch
{
    class active_map
    {
    public:
        SDL_Renderer *renderer;
        std::vector<SDL_Texture *> sprites;

        explicit active_map(SDL_Renderer *renderer)
            : renderer(renderer)
        {
        }

        ~active_map()
        {
            deactivate();
        }

        void change(ch::map *map)
        {
            deactivate();

            std::transform(
                map->map_tilesets.begin(),
                map->map_tilesets.end(),
                std::back_inserter(sprites),
                [this](const ch::map_tileset &map_tileset)
                {
                    return IMG_LoadTexture(renderer, map_tileset.tileset->sprites_filename.c_str());
                });
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
    auto size = vsnprintf(nullptr, 0, fmt, args) + 1;
    auto text = static_cast<char *>(malloc(size));
    vsprintf_s(text, size, fmt, args);
    va_end(args);

    auto surface = TTF_RenderText_Blended_Wrapped(font, text, fg, static_cast<uint32_t>(w));
    auto texture = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_Rect dstrect = {
        static_cast<int>(x),
        static_cast<int>(y),
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
        spdlog::error("Failed to initialize SDL: {}", SDL_GetError());

        return 1;
    }
    atexit(SDL_Quit);

    constexpr int img_flags = IMG_INIT_PNG;
    if (IMG_Init(img_flags) != img_flags)
    {
        spdlog::error("Failed to initialize SDL_image: {}", IMG_GetError());

        return 1;
    }
    atexit(IMG_Quit);

    constexpr int mix_flags = 0;
    if (Mix_Init(mix_flags) != mix_flags)
    {
        spdlog::error("Failed to initialize SDL_mixer: {}", Mix_GetError());

        return 1;
    }
    atexit(Mix_Quit);

    if (TTF_Init() != 0)
    {
        spdlog::error("Failed to initialize SDL_ttf: {}", TTF_GetError());

        return 1;
    }
    atexit(TTF_Quit);

    if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 4096) != 0)
    {
        spdlog::error("Failed to open audio: {}", Mix_GetError());

        return 1;
    }
    atexit(Mix_CloseAudio);

    if (enet_initialize() != 0)
    {
        spdlog::error("Failed to initialize ENet");

        return 1;
    }
    atexit(enet_deinitialize);

    ch::world world("assets/world.world", "assets/quests.json", "assets/conversations.json");

    ch::server *server = nullptr;

    std::cout << "1: Host" << std::endl;
    std::cout << "2: Join" << std::endl;
    std::cout << "> ";
    int response;
    std::cin >> response;

    if (response == 1)
    {
        std::cout << "Hosting server" << std::endl;

        server = new ch::server(server_port, world);
        if (!server->start())
        {
            spdlog::error("Failed to start server");

            return 1;
        }
    }
    else if (response == 2)
    {
        std::cout << "Joining server" << std::endl;
    }
    else
    {
        std::cout << "Invalid choice" << std::endl;
        return 1;
    }

    SDL_Window *window;
    SDL_Renderer *renderer;
    if (SDL_CreateWindowAndRenderer(window_width, window_height, 0, &window, &renderer) != 0)
    {
        spdlog::error("Failed to create window and renderer: {}", SDL_GetError());

        return 1;
    }
    SDL_SetWindowTitle(window, window_title);

    TTF_Font *font = TTF_OpenFont("assets/NinjaAdventure/HUD/Font/NormalFont.ttf", 18);

    SDL_RenderClear(renderer);
    draw_text(renderer, font, 0, 0, window_width, {255, 255, 255}, "Connecting to server...");
    SDL_RenderPresent(renderer);

    auto *client = new ch::client(server_host, server_port, world);
    if (!client->connect())
    {
        spdlog::error("Failed to start client");

        return 1;
    }

    auto &player = client->connections.at(client->connection_id).player;

    ch::active_map active_map(renderer);
    std::size_t map_index = 0; // TODO: get initial map from server when connecting
    active_map.change(&world.maps.at(map_index));

    SDL_Texture *player_idle_sprites = IMG_LoadTexture(renderer, "assets/NinjaAdventure/Actor/Characters/BlueNinja/SeparateAnim/Idle.png");
    SDL_Texture *player_walk_sprites = IMG_LoadTexture(renderer, "assets/NinjaAdventure/Actor/Characters/BlueNinja/SeparateAnim/Walk.png");
    SDL_Texture *player_attack_sprites = IMG_LoadTexture(renderer, "assets/NinjaAdventure/Actor/Characters/BlueNinja/SeparateAnim/Attack.png");

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

                    ch::message message;
                    message.type = ch::message_type::END_CONVERSATION;
                    auto packet = enet_packet_create(&message, sizeof(message), 0);
                    client->send(packet);
                }
                break;
                case SDLK_SPACE:
                {
                    if (player.conversation_node)
                    {
                        ch::message message;
                        message.type = ch::message_type::ADVANCE_CONVERSATION;
                        auto packet = enet_packet_create(&message, sizeof(message), 0);
                        client->send(packet);
                    }
                    else
                    {
                        ch::message message;
                        message.type = ch::message_type::ATTACK;
                        auto packet = enet_packet_create(&message, sizeof(message), 0);
                        client->send(packet);
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
                        ch::message_id message;
                        message.type = ch::message_type::CHOOSE_CONVERSATION_RESPONSE;
                        message.id = event.key.keysym.sym - 48;
                        auto packet = enet_packet_create(&message, sizeof(message), 0);
                        client->send(packet);
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
                    ch::message_id message;
                    message.type = ch::message_type::CHANGE_MAP;
                    message.id = 0;
                    auto packet = enet_packet_create(&message, sizeof(message), 0);
                    client->send(packet);
                }
                break;
                case SDLK_F2:
                {
                    ch::message_id message;
                    message.type = ch::message_type::CHANGE_MAP;
                    message.id = 1;
                    auto packet = enet_packet_create(&message, sizeof(message), 0);
                    client->send(packet);
                }
                break;
                case SDLK_F3:
                {
                    ch::message_id message;
                    message.type = ch::message_type::START_CONVERSATION;
                    message.id = 0;
                    auto packet = enet_packet_create(&message, sizeof(message), 0);
                    client->send(packet);
                }
                break;
                case SDLK_F4:
                {
                    ch::message_id message;
                    message.type = ch::message_type::START_CONVERSATION;
                    message.id = 1;
                    auto packet = enet_packet_create(&message, sizeof(message), 0);
                    client->send(packet);
                }
                break;
                case SDLK_F5:
                {
                    ch::message_quest_status message;
                    message.type = ch::message_type::QUEST_STATUS;
                    message.status = {0, 1};
                    auto packet = enet_packet_create(&message, sizeof(message), 0);
                    client->send(packet);
                }
                break;
                case SDLK_F6:
                {
                    ch::message_quest_status message;
                    message.type = ch::message_type::QUEST_STATUS;
                    message.status = {0, 3};
                    auto packet = enet_packet_create(&message, sizeof(message), 0);
                    client->send(packet);
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

        if (map_index != player.map_index)
        {
            map_index = player.map_index;
            active_map.change(&world.maps.at(map_index));
        }

        {
            ch::input input;
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

            ch::message_input message;
            message.type = ch::message_type::INPUT;
            message.input = input;
            auto packet = enet_packet_create(&message, sizeof(message), 0);
            client->send(packet);
        }

        if (server)
        {
            server->update(delta_time);
        }

        const auto &map = world.maps.at(map_index);
        const auto view_width = window_width / sprite_scale;
        const auto view_height = window_height / sprite_scale;
        auto view_x = static_cast<std::int64_t>(player.pos_x - view_width / 2);
        auto view_y = static_cast<std::int64_t>(player.pos_y - view_height / 2);
        if (view_x + view_width > map.width * map.tile_width)
        {
            view_x = (map.width * map.tile_width) - view_width;
        }
        if (view_x < 0)
        {
            view_x = 0;
        }
        if (view_y + view_height > map.height * map.tile_height)
        {
            view_y = (map.height * map.tile_height) - view_height;
        }
        if (view_y < 0)
        {
            view_y = 0;
        }

        SDL_RenderClear(renderer);

        for (std::int64_t y = view_y / map.tile_height; y <= static_cast<std::int64_t>((view_y + view_height) / map.tile_height); y++)
        {
            for (std::int64_t x = view_x / map.tile_width; x <= static_cast<std::int64_t>((view_x + view_width) / map.tile_width); x++)
            {
                for (const auto &layer : map.layers)
                {
                    const auto tile = layer.get_tile(x, y);
                    if (tile)
                    {
                        const auto &map_tileset = map.get_map_tileset(tile->gid);

                        const SDL_Rect srcrect = {
                            static_cast<int>(((tile->gid - map_tileset.first_gid) % map_tileset.tileset->columns) * map.tile_width),
                            static_cast<int>(((tile->gid - map_tileset.first_gid) / map_tileset.tileset->columns) * map.tile_height),
                            static_cast<int>(map.tile_width),
                            static_cast<int>(map.tile_height)};

                        const SDL_Rect dstrect = {
                            static_cast<int>(((x * map.tile_width) - view_x) * sprite_scale),
                            static_cast<int>(((y * map.tile_height) - view_y) * sprite_scale),
                            static_cast<int>(map.tile_width * sprite_scale),
                            static_cast<int>(map.tile_height * sprite_scale)};

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
                            active_map.sprites.at(map_tileset.index),
                            &srcrect,
                            &dstrect,
                            angle,
                            nullptr,
                            SDL_FLIP_NONE);
                    }
                }
            }
        }

        for (const auto &connection : client->connections)
        {
            if (connection.id != ch::server::max_connections && connection.player.map_index == map_index)
            {
                SDL_Texture *sprites = nullptr;
                SDL_Rect srcrect;
                switch (connection.player.animation)
                {
                case ch::animation::IDLE:
                    sprites = player_idle_sprites;
                    srcrect.x = static_cast<int>(connection.player.direction) * 16;
                    srcrect.y = 0;
                    break;
                case ch::animation::WALKING:
                    sprites = player_walk_sprites;
                    srcrect.x = static_cast<int>(connection.player.direction) * 16;
                    srcrect.y = (connection.player.frame_index % 4) * 16;
                    break;
                case ch::animation::ATTACKING:
                    sprites = player_attack_sprites;
                    srcrect.x = static_cast<int>(connection.player.direction) * 16;
                    srcrect.y = 0;
                    break;
                }
                srcrect.w = 16;
                srcrect.h = 16;

                const SDL_Rect dstrect = {
                    static_cast<int>((connection.player.pos_x - view_x) * sprite_scale),
                    static_cast<int>((connection.player.pos_y - view_y) * sprite_scale),
                    static_cast<int>(16 * sprite_scale),
                    static_cast<int>(16 * sprite_scale)};

                SDL_RenderCopy(renderer, sprites, &srcrect, &dstrect);

                draw_text(renderer, font, dstrect.x + 24, dstrect.y - (24 * 2), window_width, {255, 255, 255}, "%zu", connection.id);
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
                const auto &status = player.quest_statuses.at(i);
                const auto &quest = world.quests.at(status.quest_index);
                const auto &stage = quest.stages.at(status.stage_index);
                draw_text(renderer, font, 24, 24 * (i + 1), window_width - 24, {255, 255, 255}, "%s: %s", quest.name.c_str(), stage.description.c_str());
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
                if (child->type == ch::conversation_type::RESPONSE && child->check_conditions(player))
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

    client->disconnect();
    delete client;

    if (server)
    {
        server->stop();
        delete server;
    }

    TTF_CloseFont(font);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    return 0;
}
