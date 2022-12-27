#include "game.hpp"

#include "../../client.hpp"
#include "../../display.hpp"
#include "../../font.hpp"
#include "../../sound.hpp"
#include "../../texture.hpp"
#include "../menu/menu.hpp"
#include "active_map.hpp"
#include "loaded_item.hpp"
#include "loaded_tileset.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <ch/conversation.hpp>
#include <ch/map.hpp>
#include <ch/message.hpp>
#include <ch/server.hpp>
#include <ch/tileset.hpp>
#include <ch/world.hpp>
#include <exception>
#include <spdlog/spdlog.h>

ch::game::game(
    std::shared_ptr<ch::display> display,
    const char *const hostname,
    const std::uint16_t port,
    const bool is_host)
    : ch::scene(display)
{
    const auto renderer = display->get_renderer();

    font = std::make_unique<ch::font>("assets/NinjaAdventure/HUD/Font/NormalFont.ttf", 18);
    player_spritesheet = std::make_unique<ch::texture>(renderer, "assets/NinjaAdventure/Actor/Characters/Knight/SpriteSheet.png");
    shadow_sprite = std::make_unique<ch::texture>(renderer, "assets/NinjaAdventure/Actor/Characters/Shadow.png");
    dialog_box = std::make_unique<ch::texture>(renderer, "assets/NinjaAdventure/HUD/Dialog/DialogBox.png");

    world = std::make_shared<ch::world>(
        "assets/world.world",
        "assets/quests.json",
        "assets/conversations.json",
        "assets/items.json");

    if (is_host)
    {
        server = std::make_unique<ch::server>(port, world);
    }

    display->clear();
    font->render(renderer, 0, 0, 0, {255, 255, 255}, "Connecting to server...");
    display->present();

    client = std::make_unique<ch::client>(hostname, port, world);

    std::transform(
        world->items.begin(),
        world->items.end(),
        std::back_inserter(loaded_items),
        [renderer](const auto &item)
        {
            return std::make_unique<ch::loaded_item>(item, renderer);
        });

    weapon_item_index = 0;
}

void ch::game::handle_event(const SDL_Event &event)
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
            message.type = ch::message_type::end_conversation;
            client->send(&message, sizeof(message), 0);
        }
        break;
        case SDLK_SPACE:
        {
            if (player.conversation_node)
            {
                ch::message message;
                message.type = ch::message_type::advance_conversation;
                client->send(&message, sizeof(message), 0);
            }
            else
            {
                ch::message message;
                message.type = ch::message_type::attack;
                client->send(&message, sizeof(message), 0);

                const auto &loaded_weapon = loaded_items.at(weapon_item_index);
                loaded_weapon->attack_sound->play();
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
                message.type = ch::message_type::choose_conversation_response;
                message.id = event.key.keysym.sym - 48;
                client->send(&message, sizeof(message), 0);
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
            message.type = ch::message_type::change_map;
            message.id = 0;
            client->send(&message, sizeof(message), 0);
        }
        break;
        case SDLK_F2:
        {
            ch::message_id message;
            message.type = ch::message_type::change_map;
            message.id = 1;
            client->send(&message, sizeof(message), 0);
        }
        break;
        case SDLK_F3:
        {
            ch::message_id message;
            message.type = ch::message_type::start_conversation;
            message.id = 0;
            client->send(&message, sizeof(message), 0);
        }
        break;
        case SDLK_F4:
        {
            ch::message_id message;
            message.type = ch::message_type::start_conversation;
            message.id = 1;
            client->send(&message, sizeof(message), 0);
        }
        break;
        case SDLK_F5:
        {
            ch::message_quest_status message;
            message.type = ch::message_type::quest_status;
            message.status = {0, 1};
            client->send(&message, sizeof(message), 0);
        }
        break;
        case SDLK_F6:
        {
            ch::message_quest_status message;
            message.type = ch::message_type::quest_status;
            message.status = {0, 3};
            client->send(&message, sizeof(message), 0);
        }
        break;
        case SDLK_F10:
        {
            return change_scene<ch::menu>(display);
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

void ch::game::update(
    const float delta_time,
    const std::uint8_t *const keys,
    const std::uint32_t,
    const int,
    const int)
{
    const auto &player = client->get_player();
    const auto renderer = display->get_renderer();

    if (!active_map || map_index != player.map_index)
    {
        map_index = player.map_index;
        active_map = std::make_unique<ch::active_map>(world->maps.at(map_index), renderer);
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
        message.type = ch::message_type::input;
        message.input = input;
        client->send(&message, sizeof(message), 0);
    }

    if (server)
    {
        server->update(delta_time);
    }

    client->update(delta_time);

    const auto &map = world->maps.at(map_index);

    constexpr std::size_t sprite_scale = 2;
    const auto display_width = display->get_width();
    const auto display_height = display->get_height();
    const auto view_width = display_width / sprite_scale / (left_panel_open ? 2 : 1);
    const auto view_height = display_height / sprite_scale;
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
            for (auto y = static_cast<std::int64_t>(view_y / map.tile_height); y <= static_cast<std::int64_t>((view_y + view_height) / map.tile_height); y++)
            {
                for (auto x = static_cast<std::int64_t>(view_x / map.tile_width); x <= static_cast<std::int64_t>((view_x + view_width) / map.tile_width); x++)
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

                        auto angle = 0.0;
                        if (datum->d_flip)
                        {
                            if (datum->h_flip)
                            {
                                angle = 90.0;
                            }
                            if (datum->v_flip)
                            {
                                angle = 270.0;
                            }
                        }
                        else
                        {
                            if (datum->h_flip && datum->v_flip)
                            {
                                angle = 180.0;
                            }
                        }

                        texture->render_ex(
                            renderer,
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

                texture->render_ex(
                    renderer,
                    &srcrect,
                    &dstrect,
                    static_cast<double>(object.rotation),
                    nullptr,
                    SDL_FLIP_NONE);
            }
        }
    }

    for (const auto &connection : client->connections)
    {
        if (connection.id != ch::server::max_connections && connection.player.map_index == map_index)
        {
            constexpr int player_sprite_size = 16;

            SDL_Rect srcrect = {
                0,
                0,
                player_sprite_size,
                player_sprite_size};
            switch (connection.player.animation)
            {
            case ch::animation::idle:
            {
                srcrect.x = static_cast<int>(connection.player.direction) * player_sprite_size;
                srcrect.y = 0;
            }
            break;
            case ch::animation::walking:
            {
                srcrect.x = static_cast<int>(connection.player.direction) * player_sprite_size;
                srcrect.y = (connection.player.frame_index % 4) * player_sprite_size;
            }
            break;
            case ch::animation::attacking:
            {
                srcrect.x = static_cast<int>(connection.player.direction) * player_sprite_size;
                srcrect.y = 4 * player_sprite_size;
            }
            break;
            }

            const SDL_Rect dstrect = {
                static_cast<int>((connection.player.pos_x - view_x) * sprite_scale),
                static_cast<int>((connection.player.pos_y - view_y) * sprite_scale),
                static_cast<int>(player_sprite_size * sprite_scale),
                static_cast<int>(player_sprite_size * sprite_scale)};

            player_spritesheet->render(renderer, &srcrect, &dstrect);

            if (connection.player.animation == ch::animation::attacking)
            {
                const auto &weapon = world->items.at(weapon_item_index);
                const auto &loaded_weapon = loaded_items.at(weapon_item_index);
                const auto &attack_position = weapon.attack_positions.at(static_cast<std::size_t>(connection.player.direction));

                const SDL_Rect weapon_srcrect = {
                    0,
                    0,
                    static_cast<int>(weapon.width),
                    static_cast<int>(weapon.height)};
                const SDL_Rect weapon_dstrect = {
                    dstrect.x + static_cast<int>(attack_position.x_offset * sprite_scale),
                    dstrect.y + static_cast<int>(attack_position.y_offset * sprite_scale),
                    static_cast<int>(weapon.width * sprite_scale),
                    static_cast<int>(weapon.height * sprite_scale)};
                const auto weapon_angle = attack_position.angle;

                loaded_weapon->attack_sprite->render_ex(
                    renderer,
                    &weapon_srcrect,
                    &weapon_dstrect,
                    weapon_angle,
                    nullptr,
                    SDL_FLIP_NONE);
            }

            font->render(renderer, dstrect.x + 24, dstrect.y - (24 * 2), display_width, {255, 255, 255}, "{}", connection.id);
        }
    }

    if (quest_log_open)
    {
        const SDL_Rect rect = {
            12,
            12,
            static_cast<int>(display_width) - 24,
            static_cast<int>(display_height) - 24};
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_RenderFillRect(renderer, &rect);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        for (std::size_t i = 0; i < player.quest_statuses.size(); i++)
        {
            const auto &status = player.quest_statuses.at(i);
            const auto &quest = world->quests.at(status.quest_index);
            const auto &stage = quest.stages.at(status.stage_index);
            font->render(renderer, 24, 24 * (i + 1), display_width - 24, {255, 255, 255}, "{}: {}", quest.name, stage.description);
        }
    }

    if (left_panel_open)
    {
        const SDL_Rect rect = {
            static_cast<int>(display_width / 2),
            0,
            static_cast<int>(display_width / 2),
            static_cast<int>(display_height)};
        SDL_SetRenderDrawColor(renderer, 128, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderFillRect(renderer, &rect);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    }

    if (player.conversation_node)
    {
        const auto w = static_cast<int>(display_width);
        const auto h = static_cast<int>(display_height / 3);
        const auto x = 0;
        const auto y = static_cast<int>(display_height - h);
        const SDL_Rect dstrect = {x, y, w, h};

        dialog_box->render(renderer, nullptr, &dstrect);
        font->render(renderer, x + 18, y + 36, w, {0, 0, 0}, "{}", player.conversation_node->text);

        for (std::size_t i = 0; i < player.conversation_node->children.size(); i++)
        {
            const auto &child = player.conversation_node->children.at(i);
            if (child.type == ch::conversation_type::response && child.check_conditions(player))
            {
                font->render(renderer, x + 18, y + 36 + (18 * (i + 1)), w, {0, 0, 0}, "{}) {}", i + 1, child.text);
            }
        }
    }
}
