#include "game_scene.hpp"

#include "../client.hpp"
#include "../display.hpp"
#include "../font.hpp"
#include "../sound.hpp"
#include "../texture.hpp"
#include "menu_scene.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <ch/conversation.hpp>
#include <ch/map.hpp>
#include <ch/message.hpp>
#include <ch/server.hpp>
#include <ch/tileset.hpp>
#include <ch/world.hpp>
#include <enet/enet.h>
#include <exception>
#include <iostream>
#include <spdlog/spdlog.h>

namespace ch
{
    struct loaded_tileset
    {
        std::unique_ptr<ch::texture> image;
        std::vector<std::unique_ptr<ch::texture>> tile_images;

        loaded_tileset(const ch::map_tileset &map_tileset, SDL_Renderer *const renderer)
        {
            if (!map_tileset.tileset->image.empty())
            {
                spdlog::info("Loading tileset image {}", map_tileset.tileset->image);
                image = std::make_unique<ch::texture>(renderer, map_tileset.tileset->image.c_str());
            }

            for (const auto &tile : map_tileset.tileset->tiles)
            {
                if (!tile.image.empty())
                {
                    spdlog::info("Loading tile image {}", tile.image);
                    tile_images.push_back(std::make_unique<ch::texture>(renderer, tile.image.c_str()));
                }
            }
        }
    };

    class active_map
    {
    public:
        std::vector<std::unique_ptr<ch::loaded_tileset>> loaded_tilesets;

        active_map(const ch::map &map, SDL_Renderer *const renderer)
        {
            std::transform(
                map.tilesets.begin(),
                map.tilesets.end(),
                std::back_inserter(loaded_tilesets),
                [renderer](const ch::map_tileset &map_tileset)
                {
                    return std::make_unique<ch::loaded_tileset>(map_tileset, renderer);
                });
        }
    };
};

ch::game_scene::game_scene(std::shared_ptr<ch::display> display, const char *const hostname, const std::uint16_t port, const bool is_host)
    : scene(display)
{
    font = std::make_unique<ch::font>("assets/NinjaAdventure/HUD/Font/NormalFont.ttf", 18);
    attack_sound = std::make_unique<ch::sound>("assets/NinjaAdventure/Sounds/Game/Sword.wav");
    player_idle_sprites = std::make_unique<ch::texture>(display->get_renderer(), "assets/NinjaAdventure/Actor/Characters/BlueNinja/SeparateAnim/Idle.png");
    player_walk_sprites = std::make_unique<ch::texture>(display->get_renderer(), "assets/NinjaAdventure/Actor/Characters/BlueNinja/SeparateAnim/Walk.png");
    player_attack_sprites = std::make_unique<ch::texture>(display->get_renderer(), "assets/NinjaAdventure/Actor/Characters/BlueNinja/SeparateAnim/Attack.png");
    dialog_box = std::make_unique<ch::texture>(display->get_renderer(), "assets/NinjaAdventure/HUD/Dialog/DialogBox.png");

    world = std::make_shared<ch::world>("assets/world.world", "assets/quests.json", "assets/conversations.json");

    if (is_host)
    {
        server = std::make_unique<ch::server>(port, world);
    }

    display->clear();
    font->render(display->get_renderer(), 0, 0, 0, {255, 255, 255}, "Connecting to server...");
    display->present();

    client = std::make_unique<ch::client>(hostname, port, *world);

    map_index = 0; // TODO: get initial map from server when connecting
    active_map = std::make_unique<ch::active_map>(world->maps.at(map_index), display->get_renderer());
}

void ch::game_scene::handle_event(const SDL_Event &event)
{
    const auto &player = client->get_player();

    switch (event.type)
    {
    case SDL_KEYDOWN:
    {
        switch (event.key.keysym.sym)
        {
        case SDLK_ESCAPE:
        {
            quest_log_open = false;

            ch::message message;
            message.type = ch::message_type::END_CONVERSATION;
            const auto packet = enet_packet_create(&message, sizeof(message), 0);
            client->send(packet);
        }
        break;
        case SDLK_SPACE:
        {
            if (player.conversation_node)
            {
                ch::message message;
                message.type = ch::message_type::ADVANCE_CONVERSATION;
                const auto packet = enet_packet_create(&message, sizeof(message), 0);
                client->send(packet);
            }
            else
            {
                ch::message message;
                message.type = ch::message_type::ATTACK;
                const auto packet = enet_packet_create(&message, sizeof(message), 0);
                client->send(packet);

                attack_sound->play();
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
                const auto packet = enet_packet_create(&message, sizeof(message), 0);
                client->send(packet);
            }
        }
        break;
        case SDLK_c:
        {
            left_panel_open = !left_panel_open;
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
            const auto packet = enet_packet_create(&message, sizeof(message), 0);
            client->send(packet);
        }
        break;
        case SDLK_F2:
        {
            ch::message_id message;
            message.type = ch::message_type::CHANGE_MAP;
            message.id = 1;
            const auto packet = enet_packet_create(&message, sizeof(message), 0);
            client->send(packet);
        }
        break;
        case SDLK_F3:
        {
            ch::message_id message;
            message.type = ch::message_type::START_CONVERSATION;
            message.id = 0;
            const auto packet = enet_packet_create(&message, sizeof(message), 0);
            client->send(packet);
        }
        break;
        case SDLK_F4:
        {
            ch::message_id message;
            message.type = ch::message_type::START_CONVERSATION;
            message.id = 1;
            const auto packet = enet_packet_create(&message, sizeof(message), 0);
            client->send(packet);
        }
        break;
        case SDLK_F5:
        {
            ch::message_quest_status message;
            message.type = ch::message_type::QUEST_STATUS;
            message.status = {0, 1};
            const auto packet = enet_packet_create(&message, sizeof(message), 0);
            client->send(packet);
        }
        break;
        case SDLK_F6:
        {
            ch::message_quest_status message;
            message.type = ch::message_type::QUEST_STATUS;
            message.status = {0, 3};
            const auto packet = enet_packet_create(&message, sizeof(message), 0);
            client->send(packet);
        }
        break;
        case SDLK_F10:
        {
            return change_scene<ch::menu_scene>(display);
        }
        break;
        }
    }
    break;
    }

    if (server)
    {
        server->handle_event(event);
    }

    client->handle_event(event);
}

void ch::game_scene::update(
    float delta_time,
    const std::uint8_t *keys,
    std::uint32_t,
    int,
    int)
{
    const auto &player = client->get_player();

    if (map_index != player.map_index)
    {
        map_index = player.map_index;
        active_map = std::make_unique<ch::active_map>(world->maps.at(map_index), display->get_renderer());
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
        const auto packet = enet_packet_create(&message, sizeof(message), 0);
        client->send(packet);
    }

    if (server)
    {
        server->update(delta_time);
    }

    client->update(delta_time);

    const auto &map = world->maps.at(map_index);
    constexpr std::size_t sprite_scale = 2;
    const auto view_width = display->get_width() / sprite_scale / (left_panel_open ? 2 : 1);
    const auto view_height = display->get_height() / sprite_scale;
    const auto view_x = std::clamp(
        static_cast<std::int64_t>(player.pos_x - view_width / 2),
        static_cast<std::int64_t>(0.0f),
        static_cast<std::int64_t>((map.width * map.tile_width) - view_width));
    const auto view_y = std::clamp(
        static_cast<std::int64_t>(player.pos_y - view_height / 2),
        static_cast<std::int64_t>(0.0f),
        static_cast<std::int64_t>((map.height * map.tile_height) - view_height));

    for (const auto &layer : map.layers)
    {
        if (layer.data.size())
        {
            for (std::int64_t y = static_cast<std::int64_t>(view_y / map.tile_height); y <= static_cast<std::int64_t>((view_y + view_height) / map.tile_height); y++)
            {
                for (std::int64_t x = static_cast<std::int64_t>(view_x / map.tile_width); x <= static_cast<std::int64_t>((view_x + view_width) / map.tile_width); x++)
                {
                    const auto datum = layer.get_datum(x, y);
                    if (datum)
                    {
                        const auto &map_tileset = map.get_tileset(datum->gid);
                        const auto tileset = map_tileset.tileset;

                        ch::texture *texture;
                        SDL_Rect srcrect;
                        SDL_Rect dstrect;
                        if (tileset->image.empty())
                        {
                            const auto &tile = map_tileset.get_tile(datum->gid);

                            texture = active_map->loaded_tilesets.at(map_tileset.index)->tile_images.at(tile.index).get();
                            srcrect = {
                                0,
                                0,
                                static_cast<int>(tile.width),
                                static_cast<int>(tile.height)};
                            dstrect = {
                                static_cast<int>(((x * map.tile_width) - view_x) * sprite_scale),
                                static_cast<int>(((y * map.tile_height) - view_y) * sprite_scale),
                                static_cast<int>(tile.width * sprite_scale),
                                static_cast<int>(tile.height * sprite_scale)};
                        }
                        else
                        {
                            texture = active_map->loaded_tilesets.at(map_tileset.index)->image.get();
                            srcrect = {
                                static_cast<int>(((datum->gid - map_tileset.first_gid) % tileset->columns) * map.tile_width),
                                static_cast<int>(((datum->gid - map_tileset.first_gid) / tileset->columns) * map.tile_height),
                                static_cast<int>(map.tile_width),
                                static_cast<int>(map.tile_height)};
                            dstrect = {
                                static_cast<int>(((x * map.tile_width) - view_x) * sprite_scale),
                                static_cast<int>(((y * map.tile_height) - view_y) * sprite_scale),
                                static_cast<int>(map.tile_width * sprite_scale),
                                static_cast<int>(map.tile_height * sprite_scale)};
                        }

                        double angle = 0;
                        if (datum->d_flip)
                        {
                            if (datum->h_flip)
                            {
                                angle = 90;
                            }
                            if (datum->v_flip)
                            {
                                angle = 270;
                            }
                        }
                        else
                        {
                            if (datum->h_flip && datum->v_flip)
                            {
                                angle = 180;
                            }
                        }

                        texture->render_ex(
                            display->get_renderer(),
                            &srcrect,
                            &dstrect,
                            angle,
                            nullptr,
                            SDL_FLIP_NONE);
                    }
                }
            }
        }

        if (layer.objects.size())
        {
            for (const auto &object : layer.objects)
            {
                const auto &map_tileset = map.get_tileset(object.gid);
                const auto tileset = map_tileset.tileset;

                ch::texture *texture;
                SDL_Rect srcrect;
                SDL_Rect dstrect;
                if (tileset->image.empty())
                {
                    const auto &tile = map_tileset.get_tile(object.gid);

                    texture = active_map->loaded_tilesets.at(map_tileset.index)->tile_images.at(tile.index).get();
                    srcrect = {
                        0,
                        0,
                        static_cast<int>(tile.width),
                        static_cast<int>(tile.height)};
                    dstrect = {
                        static_cast<int>((object.x - view_x) * sprite_scale),
                        static_cast<int>((object.y - view_y) * sprite_scale),
                        static_cast<int>(tile.width * sprite_scale),
                        static_cast<int>(tile.height * sprite_scale)};
                }
                else
                {
                    texture = active_map->loaded_tilesets.at(map_tileset.index)->image.get();
                    srcrect = {
                        static_cast<int>(((object.gid - map_tileset.first_gid) % tileset->columns) * map.tile_width),
                        static_cast<int>(((object.gid - map_tileset.first_gid) / tileset->columns) * map.tile_height),
                        static_cast<int>(map.tile_width),
                        static_cast<int>(map.tile_height)};
                    dstrect = {
                        static_cast<int>((object.x - view_x) * sprite_scale),
                        static_cast<int>((object.y - view_y) * sprite_scale),
                        static_cast<int>(map.tile_width * sprite_scale),
                        static_cast<int>(map.tile_height * sprite_scale)};
                }

                double angle = object.rotation;

                texture->render_ex(
                    display->get_renderer(),
                    &srcrect,
                    &dstrect,
                    angle,
                    nullptr,
                    SDL_FLIP_NONE);
            }
        }
    }

    for (const auto &connection : client->connections)
    {
        if (connection.id != ch::server::max_connections && connection.player.map_index == map_index)
        {
            ch::texture *texture;
            SDL_Rect srcrect;
            switch (connection.player.animation)
            {
            case ch::animation::IDLE:
                texture = player_idle_sprites.get();
                srcrect.x = static_cast<int>(connection.player.direction) * 16;
                srcrect.y = 0;
                break;
            case ch::animation::WALKING:
                texture = player_walk_sprites.get();
                srcrect.x = static_cast<int>(connection.player.direction) * 16;
                srcrect.y = (connection.player.frame_index % 4) * 16;
                break;
            case ch::animation::ATTACKING:
                texture = player_attack_sprites.get();
                srcrect.x = static_cast<int>(connection.player.direction) * 16;
                srcrect.y = 0;
                break;
            default:
                texture = nullptr;
                break;
            }
            srcrect.w = 16;
            srcrect.h = 16;

            const SDL_Rect dstrect = {
                static_cast<int>((connection.player.pos_x - view_x) * sprite_scale),
                static_cast<int>((connection.player.pos_y - view_y) * sprite_scale),
                static_cast<int>(16 * sprite_scale),
                static_cast<int>(16 * sprite_scale)};

            texture->render(display->get_renderer(), &srcrect, &dstrect);

            font->render(display->get_renderer(), dstrect.x + 24, dstrect.y - (24 * 2), display->get_width(), {255, 255, 255}, "{}", connection.id);
        }
    }

    if (quest_log_open)
    {
        const SDL_Rect rect = {
            12,
            12,
            static_cast<int>(display->get_width()) - 24,
            static_cast<int>(display->get_height()) - 24};
        SDL_SetRenderDrawColor(display->get_renderer(), 0, 0, 0, 200);
        SDL_SetRenderDrawBlendMode(display->get_renderer(), SDL_BLENDMODE_BLEND);
        SDL_RenderFillRect(display->get_renderer(), &rect);
        SDL_SetRenderDrawColor(display->get_renderer(), 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_SetRenderDrawBlendMode(display->get_renderer(), SDL_BLENDMODE_NONE);

        for (std::size_t i = 0; i < player.quest_statuses.size(); i++)
        {
            const auto &status = player.quest_statuses.at(i);
            const auto &quest = world->quests.at(status.quest_index);
            const auto &stage = quest.stages.at(status.stage_index);
            font->render(display->get_renderer(), 24, 24 * (i + 1), display->get_width() - 24, {255, 255, 255}, "{}: {}", quest.name, stage.description);
        }
    }

    if (left_panel_open)
    {
        const SDL_Rect rect = {
            static_cast<int>(display->get_width() / 2),
            0,
            static_cast<int>(display->get_width() / 2),
            static_cast<int>(display->get_height())};
        SDL_SetRenderDrawColor(display->get_renderer(), 128, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderFillRect(display->get_renderer(), &rect);
        SDL_SetRenderDrawColor(display->get_renderer(), 0, 0, 0, SDL_ALPHA_OPAQUE);
    }

    if (player.conversation_node)
    {
        const auto w = static_cast<int>(display->get_width());
        const auto h = static_cast<int>(display->get_height() / 3);
        const auto x = 0;
        const auto y = static_cast<int>(display->get_height() - h);
        const SDL_Rect dstrect = {x, y, w, h};

        dialog_box->render(display->get_renderer(), nullptr, &dstrect);
        font->render(display->get_renderer(), x + 18, y + 36, w, {0, 0, 0}, "{}", player.conversation_node->text);

        for (std::size_t i = 0; i < player.conversation_node->children.size(); i++)
        {
            const auto &child = player.conversation_node->children.at(i);
            if (child.type == ch::conversation_type::RESPONSE && child.check_conditions(player))
            {
                font->render(display->get_renderer(), x + 18, y + 36 + (18 * (i + 1)), w, {0, 0, 0}, "{}) {}", i + 1, child.text);
            }
        }
    }
}
