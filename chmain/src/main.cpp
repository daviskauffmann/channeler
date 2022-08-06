#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
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
#include <stdarg.h>

constexpr const char *window_title = "Channeler";
constexpr std::size_t window_width = 640;
constexpr std::size_t window_height = 480;

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
        ch::map *map = nullptr;
        std::vector<SDL_Texture *> sprites;

        explicit active_map(SDL_Renderer *renderer)
            : renderer(renderer)
        {
        }

        void change(ch::map *new_map)
        {
            if (map)
            {
                deactivate();
            }

            map = new_map;

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
    auto size = vsnprintf(NULL, 0, fmt, args) + 1;
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

    ch::server *server = nullptr;

    std::cout << "1: Host" << std::endl;
    std::cout << "2: Join" << std::endl;
    std::cout << "> ";
    int response;
    std::cin >> response;

    if (response == 1)
    {
        std::cout << "[Client] Hosting server" << std::endl;

        server = new ch::server(server_port);
    }
    else if (response == 2)
    {
        std::cout << "[Client] Joining server" << std::endl;
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
        return 1;
    }
    SDL_SetWindowTitle(window, window_title);

    TTF_Font *font = TTF_OpenFont("assets/NinjaAdventure/HUD/Font/NormalFont.ttf", 18);

    SDL_RenderClear(renderer);
    draw_text(renderer, font, 0, 0, window_width, {255, 255, 255}, "Connecting to server...");
    SDL_RenderPresent(renderer);

    auto host = enet_host_create(NULL, 1, 2, 0, 0);
    if (!host)
    {
        return 1;
    }

    ENetAddress address;
    enet_address_set_host(&address, server_host);
    address.port = server_port;
    auto peer = enet_host_connect(host, &address, 2, 0);
    if (!peer)
    {
        return 1;
    }

    std::size_t client_id = 0;
    std::size_t map_index = 0;

    {
        bool connected = false;

        ENetEvent event;
        while (enet_host_service(host, &event, 5000) > 0)
        {
            if (event.type == ENET_EVENT_TYPE_CONNECT)
            {
                std::cout << "[Client] Connected to " << server_host << ":" << server_port << std::endl;
            }
            else if (event.type == ENET_EVENT_TYPE_RECEIVE)
            {
                const auto type = reinterpret_cast<ch::message *>(event.packet->data)->type;

                if (type == ch::message_type::SERVER_JOINED)
                {
                    const auto message = reinterpret_cast<ch::message_id *>(event.packet->data);

                    std::cout << "[Client] Assigned ID " << message->id << std::endl;

                    connected = true;
                    client_id = message->id;
                    map_index = 0; // TODO: get from message

                    break;
                }
                else if (type == ch::message_type::SERVER_FULL)
                {
                    std::cout << "[Client] Server full" << std::endl;
                }
                else
                {
                    std::cout << "[Client] Unknown server response" << std::endl;
                }

                enet_packet_destroy(event.packet);
            }
        }

        if (!connected)
        {
            std::cout << "[Client] Could not connect to server" << std::endl;

            enet_peer_reset(peer);

            return 1;
        }
    }

    ch::world world("assets/world.world", "assets/quests.json", "assets/conversations.json");
    SDL_Texture *player_sprites = IMG_LoadTexture(renderer, "assets/NinjaAdventure/Actor/Characters/BlueNinja/SpriteSheet.png");

    std::array<ch::client, ch::server::max_clients> clients;
    auto &client = clients.at(client_id);
    client.id = client_id;
    auto &player = client.player;
    auto &input = client.input;

    hp_client::active_map map(renderer);
    map.change(&world.maps.at(map_index));

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
                        ch::message_id message;
                        message.type = ch::message_type::CHANGE_MAP;
                        message.id = 0;
                        auto packet = enet_packet_create(&message, sizeof(message), 0);
                        enet_peer_send(peer, 0, packet);
                    }
                    break;
                    case SDLK_F2:
                    {
                        ch::message_id message;
                        message.type = ch::message_type::CHANGE_MAP;
                        message.id = 1;
                        auto packet = enet_packet_create(&message, sizeof(message), 0);
                        enet_peer_send(peer, 0, packet);
                    }
                    break;
                    case SDLK_F3:
                    {
                        player.start_conversation(world, 0);
                    }
                    break;
                    case SDLK_F4:
                    {
                        player.start_conversation(world, 1);
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

        {
            ENetEvent event;
            while (enet_host_service(host, &event, 0) > 0)
            {
                switch (event.type)
                {
                case ENET_EVENT_TYPE_RECEIVE:
                {
                    const auto type = reinterpret_cast<ch::message *>(event.packet->data)->type;

                    switch (type)
                    {
                    case ch::message_type::CLIENT_JOINED:
                    {
                        const auto message = reinterpret_cast<ch::message_id *>(event.packet->data);
                        const auto new_client_id = message->id;

                        std::cout << "[Client] Client " << new_client_id << " connected" << std::endl;

                        clients.at(new_client_id).id = new_client_id;
                    }
                    break;
                    case ch::message_type::CLIENT_DISCONNECTED:
                    {
                        const auto message = reinterpret_cast<ch::message_id *>(event.packet->data);
                        const auto disconnected_client_id = message->id;

                        std::cout << "[Client] Client " << disconnected_client_id << " disconnected" << std::endl;

                        clients.at(disconnected_client_id).id = ch::server::max_clients;
                    }
                    break;
                    case ch::message_type::GAME_STATE:
                    {
                        const auto message = reinterpret_cast<ch::message_game_state *>(event.packet->data);

                        for (std::size_t i = 0; i < message->clients.size(); i++)
                        {
                            clients.at(i).id = message->clients.at(i).id;

                            clients.at(i).player.map_index = message->clients.at(i).player.map_index;

                            clients.at(i).player.pos_x = message->clients.at(i).player.pos_x;
                            clients.at(i).player.pos_y = message->clients.at(i).player.pos_y;

                            clients.at(i).player.direction = message->clients.at(i).player.direction;
                            clients.at(i).player.animation = message->clients.at(i).player.animation;
                            clients.at(i).player.frame_index = message->clients.at(i).player.frame_index;
                        }
                    }
                    break;
                    default:
                    {
                        std::cout << "[Client] Unknown message type " << static_cast<int>(type) << std::endl;
                    }
                    break;
                    }

                    enet_packet_destroy(event.packet);
                }
                break;
                }
            }
        }

        if (map_index != player.map_index)
        {
            map_index = player.map_index;
            map.change(&world.maps.at(map_index));
        }

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
        enet_peer_send(peer, 0, packet);

        if (server)
        {
            server->update(delta_time, world);
        }

        const auto view_width = window_width / sprite_scale;
        const auto view_height = window_height / sprite_scale;
        auto view_x = static_cast<std::int64_t>(player.pos_x - view_width / 2);
        auto view_y = static_cast<std::int64_t>(player.pos_y - view_height / 2);
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

        for (std::int64_t y = view_y / map.map->tile_height; y <= static_cast<std::int64_t>((view_y + view_height) / map.map->tile_height); y++)
        {
            for (std::int64_t x = view_x / map.map->tile_width; x <= static_cast<std::int64_t>((view_x + view_width) / map.map->tile_width); x++)
            {
                for (const auto &layer : map.map->layers)
                {
                    const auto tile = layer.get_tile(x, y);
                    if (tile)
                    {
                        const auto &map_tileset = map.map->get_map_tileset(tile->gid);

                        const SDL_Rect srcrect = {
                            static_cast<int>(((tile->gid - map_tileset.first_gid) % map_tileset.tileset->columns) * map.map->tile_width),
                            static_cast<int>(((tile->gid - map_tileset.first_gid) / map_tileset.tileset->columns) * map.map->tile_height),
                            static_cast<int>(map.map->tile_width),
                            static_cast<int>(map.map->tile_height)};

                        const SDL_Rect dstrect = {
                            static_cast<int>(((x * map.map->tile_width) - view_x) * sprite_scale),
                            static_cast<int>(((y * map.map->tile_height) - view_y) * sprite_scale),
                            static_cast<int>(map.map->tile_width * sprite_scale),
                            static_cast<int>(map.map->tile_height * sprite_scale)};

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

        for (const auto &_client : clients)
        {
            if (_client.id != ch::server::max_clients && _client.player.map_index == map_index)
            {
                SDL_Rect srcrect;
                switch (_client.player.animation)
                {
                case ch::animation::IDLE:
                    srcrect.x = static_cast<int>(_client.player.direction) * 16;
                    srcrect.y = 0;
                    break;
                case ch::animation::WALKING:
                    srcrect.x = static_cast<int>(_client.player.direction) * 16;
                    srcrect.y = (1 + (_client.player.frame_index % 3)) * 16;
                    break;
                }
                srcrect.w = 16;
                srcrect.h = 16;

                const SDL_Rect dstrect = {
                    static_cast<int>((_client.player.pos_x - view_x) * sprite_scale),
                    static_cast<int>((_client.player.pos_y - view_y) * sprite_scale),
                    static_cast<int>(16 * sprite_scale),
                    static_cast<int>(16 * sprite_scale)};

                SDL_RenderCopy(renderer, player_sprites, &srcrect, &dstrect);

                draw_text(renderer, font, dstrect.x + 24, dstrect.y - (24 * 2), window_width, {255, 255, 255}, "%zu", _client.id);
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

    map.deactivate();

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
                std::cout << "[Client] Disconnected" << std::endl;

                disconnected = true;

                break;
            }
        }

        if (!disconnected)
        {
            std::cout << "[Client] Server did not confirm disconnect" << std::endl;

            enet_peer_reset(peer);
        }
    }

    enet_host_destroy(host);

    if (server)
    {
        delete server;
    }

    TTF_CloseFont(font);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    return 0;
}
