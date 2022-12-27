#include <ch/player.hpp>

#include <ch/conversation.hpp>
#include <ch/input.hpp>
#include <ch/map.hpp>
#include <ch/tileset.hpp>
#include <ch/world.hpp>

void ch::player::update(const ch::input &input, const ch::map &map, const float delta_time)
{
    if (attacking)
    {
        animation = ch::animation::attacking;

        attack_timer -= delta_time;
        if (attack_timer <= 0)
        {
            attacking = false;
        }
    }

    if (!attacking)
    {
        animation = ch::animation::idle;
        if (input.dy == -1)
        {
            direction = ch::direction::up;
            animation = ch::animation::walking;
        }
        if (input.dx == -1)
        {
            direction = ch::direction::left;
            animation = ch::animation::walking;
        }
        if (input.dy == 1)
        {
            direction = ch::direction::down;
            animation = ch::animation::walking;
        }
        if (input.dx == 1)
        {
            direction = ch::direction::right;
            animation = ch::animation::walking;
        }
    }

    animation_timer += delta_time;
    if (animation_timer > 0.15f)
    {
        animation_timer = 0;
        frame_index++;
    }

    acceleration = {
        static_cast<float>(input.dx),
        static_cast<float>(input.dy)};

    const auto acceleration_length = acceleration.length();
    if (acceleration_length > 1)
    {
        acceleration /= acceleration_length;
    }

    constexpr auto speed = 1000.0f;
    acceleration *= speed;

    constexpr auto drag = 20.0f;
    acceleration -= velocity * drag;

    const auto new_position = 0.5f * acceleration * powf(delta_time, 2) + velocity * delta_time + position;

    const auto tile_nx_x = static_cast<std::size_t>(roundf(new_position.x / map.tile_width));
    const auto tile_nx_y = static_cast<std::size_t>(roundf(position.y / map.tile_height));
    if (map.is_solid(tile_nx_x, tile_nx_y))
    {
        velocity.x = 0;
    }
    else
    {
        position.x = new_position.x;
        velocity.x = acceleration.x * delta_time + velocity.x;
    }

    const auto tile_ny_x = static_cast<std::size_t>(roundf(position.x / map.tile_width));
    const auto tile_ny_y = static_cast<std::size_t>(roundf(new_position.y / map.tile_height));
    if (map.is_solid(tile_ny_x, tile_ny_y))
    {
        velocity.y = 0;
    }
    else
    {
        position.y = new_position.y;
        velocity.y = acceleration.y * delta_time + velocity.y;
    }
}

void ch::player::attack()
{
    attacking = true;
    attack_timer = 0.1f;
}

void ch::player::start_conversation(std::shared_ptr<ch::world> world, const std::size_t root_index)
{
    conversation_root = conversation_node = &world->conversations.at(root_index);
    advance_conversation();
}

void ch::player::advance_conversation()
{
    if (!conversation_node->jump_id.empty())
    {
        conversation_node = conversation_root->find_by_id(conversation_node->jump_id);
    }
    else
    {
        if (!conversation_node->children.size())
        {
            return end_conversation();
        }

        if (!conversation_node->has_response_nodes())
        {
            for (const auto &child : conversation_node->children)
            {
                if (child.check_conditions(*this))
                {
                    conversation_node = &child;

                    if (conversation_node->effect.quest_status)
                    {
                        set_quest_status(*conversation_node->effect.quest_status);
                    }

                    break;
                }
            }
        }
    }
}

void ch::player::choose_conversation_response(const std::size_t choice_index)
{
    std::size_t valid_choice_index = 0;
    for (const auto &child : conversation_node->children)
    {
        if (child.type == ch::conversation_type::response && child.check_conditions(*this))
        {
            valid_choice_index++;
            if (valid_choice_index == choice_index)
            {
                conversation_node = &child;
                advance_conversation();
                break;
            }
        }
    }
}

void ch::player::end_conversation()
{
    conversation_root = conversation_node = nullptr;
}

void ch::player::set_quest_status(const ch::quest_status &status)
{
    const auto quest_status = std::find_if(
        quest_statuses.begin(),
        quest_statuses.end(),
        [status](const auto &quest_status)
        {
            return quest_status.quest_index == status.quest_index;
        });

    if (quest_status == quest_statuses.end())
    {
        quest_statuses.push_back(status);
    }
    else
    {
        quest_status->stage_index = status.stage_index;
    }

    if (on_quest_status_set)
    {
        on_quest_status_set(status);
    }
}

bool ch::player::check_quest_status(const ch::quest_status &status) const
{
    return std::any_of(
        quest_statuses.begin(),
        quest_statuses.end(),
        [status](const auto &quest_status)
        {
            return quest_status.quest_index == status.quest_index &&
                   quest_status.stage_index == status.stage_index;
        });
}
