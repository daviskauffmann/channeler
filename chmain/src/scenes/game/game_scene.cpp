#include "game_scene.hpp"

#include "../../client.hpp"
#include "../../display.hpp"
#include "../../font.hpp"
#include "../../sound.hpp"
#include "../../texture.hpp"
#include "../menu/menu_scene.hpp"
#include "active_map.hpp"
#include "loaded_item.hpp"
#include "loaded_tileset.hpp"
#include "renderable.hpp"
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

ch::game_scene::game_scene(
    std::shared_ptr<ch::display> display,
    const char *const hostname,
    const std::uint16_t port,
    const bool is_host)
    : ch::scene(display)
{
    const auto renderer = display->get_renderer();

    font = std::make_unique<ch::font>(renderer, "data/NinjaAdventure/HUD/Font/NormalFont.ttf", 18);
    player_spritesheet = std::make_unique<ch::texture>(renderer, "data/NinjaAdventure/Actor/Characters/Knight/SpriteSheet.png");
    dialog_box = std::make_unique<ch::texture>(renderer, "data/NinjaAdventure/HUD/Dialog/DialogBox.png");

    world = std::make_shared<ch::world>(
        "data/world.world",
        "data/quests.json",
        "data/conversations.json",
        "data/items.json");

    if (is_host)
    {
        server = std::make_unique<ch::server>(port, world);
    }

    display->clear();
    font->render(0, 0, 0, {255, 255, 255}, "Connecting to server...");
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

ch::scene *ch::game_scene::handle_event(const SDL_Event &event)
{
    const auto &player = client->get_self();

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

    return this;
}

ch::scene *ch::game_scene::update(
    const float delta_time,
    const std::uint8_t *const keys,
    const std::uint32_t,
    const int,
    const int)
{
    const auto &self = client->get_self();
    const auto renderer = display->get_renderer();

    if (!active_map || map_index != self.map_index)
    {
        map_index = self.map_index;
        active_map = std::make_unique<ch::active_map>(world->maps.at(map_index), renderer);
    }

    {
        ch::message_input message;
        message.type = ch::message_type::input;
        message.input_x = 0;
        message.input_y = 0;

        if (keys[SDL_SCANCODE_W])
        {
            message.input_y = -1;
        }
        if (keys[SDL_SCANCODE_A])
        {
            message.input_x = -1;
        }
        if (keys[SDL_SCANCODE_S])
        {
            message.input_y = 1;
        }
        if (keys[SDL_SCANCODE_D])
        {
            message.input_x = 1;
        }

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
        static_cast<std::int64_t>(self.position_x - view_width / 2),
        static_cast<std::int64_t>(0.0f),
        static_cast<std::int64_t>((map.width * map.tile_width) - view_width));
    const auto view_y = std::clamp(
        static_cast<std::int64_t>(self.position_y - view_height / 2),
        static_cast<std::int64_t>(0.0f),
        static_cast<std::int64_t>((map.height * map.tile_height) - view_height));

    std::vector<ch::renderable> renderables;

    for (size_t layer_index = 0; const auto &layer : map.layers)
    {
        if (layer.tiles.size())
        {
            for (auto y = static_cast<std::int64_t>(view_y / map.tile_height); y <= static_cast<std::int64_t>((view_y + view_height) / map.tile_height); y++)
            {
                for (auto x = static_cast<std::int64_t>(view_x / map.tile_width); x <= static_cast<std::int64_t>((view_x + view_width) / map.tile_width); x++)
                {
                    const auto layer_tile = layer.get_tile(x, y);
                    if (layer_tile)
                    {
                        const auto &map_tileset = map.get_tileset(layer_tile->gid);
                        const auto tileset = map_tileset.tileset;

                        ch::renderable renderable;

                        if (tileset->image.empty())
                        {
                            const auto &tileset_tile = map_tileset.get_tile(layer_tile->gid);

                            renderable.texture = active_map->loaded_tilesets.at(map_tileset.index)->tile_images.at(tileset_tile.index).get();
                            renderable.srcrect = {
                                0,
                                0,
                                static_cast<int>(tileset_tile.width),
                                static_cast<int>(tileset_tile.height)};
                            renderable.dstrect = {
                                static_cast<int>(((x * map.tile_width) - view_x) * sprite_scale),
                                static_cast<int>(((y * map.tile_height) - view_y) * sprite_scale),
                                static_cast<int>(tileset_tile.width * sprite_scale),
                                static_cast<int>(tileset_tile.height * sprite_scale)};
                        }
                        else
                        {
                            renderable.texture = active_map->loaded_tilesets.at(map_tileset.index)->image.get();
                            renderable.srcrect = {
                                static_cast<int>(((layer_tile->gid - map_tileset.first_gid) % tileset->columns) * map.tile_width),
                                static_cast<int>(((layer_tile->gid - map_tileset.first_gid) / tileset->columns) * map.tile_height),
                                static_cast<int>(map.tile_width),
                                static_cast<int>(map.tile_height)};
                            renderable.dstrect = {
                                static_cast<int>(((x * map.tile_width) - view_x) * sprite_scale),
                                static_cast<int>(((y * map.tile_height) - view_y) * sprite_scale),
                                static_cast<int>(map.tile_width * sprite_scale),
                                static_cast<int>(map.tile_height * sprite_scale)};
                        }

                        renderable.angle = layer_tile->angle();
                        renderable.y = layer.depth ? static_cast<int>(y * map.tile_height) : -1;

                        renderables.push_back(renderable);
                    }
                }
            }
        }

        layer_index++;
    }

    for (const auto &objectgroup : map.objectgroups)
    {
        for (const auto &object : objectgroup.objects)
        {
            const auto &map_tileset = map.get_tileset(object.gid);
            const auto tileset = map_tileset.tileset;

            ch::renderable renderable;

            if (tileset->image.empty())
            {
                const auto &tileset_tile = map_tileset.get_tile(object.gid);

                renderable.texture = active_map->loaded_tilesets.at(map_tileset.index)->tile_images.at(tileset_tile.index).get();
                renderable.srcrect = {
                    0,
                    0,
                    static_cast<int>(tileset_tile.width),
                    static_cast<int>(tileset_tile.height)};
                renderable.dstrect = {
                    static_cast<int>((object.x - view_x) * sprite_scale),
                    static_cast<int>((object.y - view_y) * sprite_scale),
                    static_cast<int>(tileset_tile.width * sprite_scale),
                    static_cast<int>(tileset_tile.height * sprite_scale)};
            }
            else
            {
                renderable.texture = active_map->loaded_tilesets.at(map_tileset.index)->image.get();
                renderable.srcrect = {
                    static_cast<int>(((object.gid - map_tileset.first_gid) % tileset->columns) * map.tile_width),
                    static_cast<int>(((object.gid - map_tileset.first_gid) / tileset->columns) * map.tile_height),
                    static_cast<int>(map.tile_width),
                    static_cast<int>(map.tile_height)};
                renderable.dstrect = {
                    static_cast<int>((object.x - view_x) * sprite_scale),
                    static_cast<int>((object.y - view_y) * sprite_scale),
                    static_cast<int>(map.tile_width * sprite_scale),
                    static_cast<int>(map.tile_height * sprite_scale)};
            }

            renderable.angle = object.rotation;
            renderable.y = static_cast<int>(object.y);

            renderables.push_back(renderable);
        }
    }

    for (const auto &player : client->players)
    {
        if (player.id != ch::server::max_players && player.map_index == map_index)
        {
            constexpr int player_sprite_size = 16;
            const int player_x = static_cast<int>((player.position_x - view_x) * sprite_scale);
            const int player_y = static_cast<int>((player.position_y - view_y) * sprite_scale);

            ch::renderable renderable;

            renderable.texture = player_spritesheet.get();

            renderable.srcrect = {
                0,
                0,
                player_sprite_size,
                player_sprite_size};
            switch (player.animation)
            {
            case ch::animation::idle:
            {
                renderable.srcrect.x = static_cast<int>(player.direction) * player_sprite_size;
                renderable.srcrect.y = 0;
            }
            break;
            case ch::animation::walking:
            {
                renderable.srcrect.x = static_cast<int>(player.direction) * player_sprite_size;
                renderable.srcrect.y = (player.frame_index % 4) * player_sprite_size;
            }
            break;
            case ch::animation::attacking:
            {
                renderable.srcrect.x = static_cast<int>(player.direction) * player_sprite_size;
                renderable.srcrect.y = 4 * player_sprite_size;
            }
            break;
            }

            renderable.dstrect = {
                player_x,
                player_y,
                static_cast<int>(player_sprite_size * sprite_scale),
                static_cast<int>(player_sprite_size * sprite_scale)};
            renderable.angle = 0;
            renderable.y = static_cast<int>(player.position_y);

            renderables.push_back(renderable);

            if (player.animation == ch::animation::attacking)
            {
                const auto &weapon = world->items.at(weapon_item_index);
                const auto &loaded_weapon = loaded_items.at(weapon_item_index);
                const auto &attack_position = weapon.attack_positions.at(static_cast<std::size_t>(player.direction));

                ch::renderable weapon_renderable;

                weapon_renderable.texture = loaded_weapon->attack_sprite.get();
                weapon_renderable.srcrect = {
                    0,
                    0,
                    static_cast<int>(weapon.width),
                    static_cast<int>(weapon.height)};
                weapon_renderable.dstrect = {
                    player_x + static_cast<int>(attack_position.x_offset * sprite_scale),
                    player_y + static_cast<int>(attack_position.y_offset * sprite_scale),
                    static_cast<int>(weapon.width * sprite_scale),
                    static_cast<int>(weapon.height * sprite_scale)};
                weapon_renderable.angle = attack_position.angle;
                weapon_renderable.y = static_cast<int>(player.position_y);

                renderables.push_back(weapon_renderable);
            }
        }
    }

    std::sort(
        renderables.begin(),
        renderables.end(), [](const ch::renderable &a, const ch::renderable &b)
        { return a.y < b.y; });

    for (const auto &renderable : renderables)
    {
        renderable.texture->render_ex(
            &renderable.srcrect,
            &renderable.dstrect,
            renderable.angle,
            nullptr,
            SDL_FLIP_NONE);
    }

    for (const auto &player : client->players)
    {
        if (player.id != ch::server::max_players && player.map_index == map_index)
        {
            const int player_x = static_cast<int>((player.position_x - view_x) * sprite_scale);
            const int player_y = static_cast<int>((player.position_y - view_y) * sprite_scale);

            font->render(player_x, player_y, display_width, {255, 255, 255}, "{}", player.id);
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

        for (std::size_t i = 0; i < self.quest_statuses.size(); i++)
        {
            const auto &status = self.quest_statuses.at(i);
            const auto &quest = world->quests.at(status.quest_index);
            const auto &stage = quest.stages.at(status.stage_index);

            font->render(24, 24 * (i + 1), display_width - 24, {255, 255, 255}, "{}: {}", quest.name, stage.description);
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

    if (self.conversation_node)
    {
        const auto w = static_cast<int>(display_width);
        const auto h = static_cast<int>(display_height / 3);
        const auto x = 0;
        const auto y = static_cast<int>(display_height - h);
        const SDL_Rect dstrect = {x, y, w, h};

        dialog_box->render(nullptr, &dstrect);
        font->render(x + 18, y + 36, w, {0, 0, 0}, "{}", self.conversation_node->text);

        for (std::size_t i = 0; i < self.conversation_node->children.size(); i++)
        {
            const auto &child = self.conversation_node->children.at(i);
            if (child.type == ch::conversation_type::response && child.check_conditions(self))
            {
                font->render(x + 18, y + 36 + (18 * (i + 1)), w, {0, 0, 0}, "{}) {}", i + 1, child.text);
            }
        }
    }

    return this;
}
