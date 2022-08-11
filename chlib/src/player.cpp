#include <ch/player.hpp>

#include <ch/conversation.hpp>
#include <ch/input.hpp>
#include <ch/map.hpp>
#include <ch/tileset.hpp>
#include <ch/world.hpp>
#include <iostream>

void ch::player::update(const ch::input &input, const ch::map &map, const float delta_time)
{
    if (attacking)
    {
        animation = ch::animation::ATTACKING;

        attack_timer -= delta_time;
        if (attack_timer <= 0)
        {
            attacking = false;
        }
    }

    if (!attacking)
    {
        animation = ch::animation::IDLE;
        if (input.dy == -1)
        {
            direction = ch::direction::UP;
            animation = ch::animation::WALKING;
        }
        if (input.dx == -1)
        {
            direction = ch::direction::LEFT;
            animation = ch::animation::WALKING;
        }
        if (input.dy == 1)
        {
            direction = ch::direction::DOWN;
            animation = ch::animation::WALKING;
        }
        if (input.dx == 1)
        {
            direction = ch::direction::RIGHT;
            animation = ch::animation::WALKING;
        }
    }

    animation_timer += delta_time;
    if (animation_timer > 0.15f)
    {
        animation_timer = 0;
        frame_index++;
    }

    acc_x = static_cast<float>(input.dx);
    acc_y = static_cast<float>(input.dy);

    const auto acc_len = sqrtf(powf(acc_x, 2) + powf(acc_y, 2));
    if (acc_len > 1)
    {
        acc_x /= acc_len;
        acc_y /= acc_len;
    }

    constexpr auto speed = 1000.0f;
    acc_x *= speed;
    acc_y *= speed;

    constexpr auto drag = 20.0f;
    acc_x -= vel_x * drag;
    acc_y -= vel_y * drag;

    const auto new_pos_x = 0.5f * acc_x * powf(delta_time, 2) + vel_x * delta_time + pos_x;
    const auto new_pos_y = 0.5f * acc_y * powf(delta_time, 2) + vel_y * delta_time + pos_y;

    const auto tile_nx_x = static_cast<std::size_t>(roundf(new_pos_x / map.tile_width));
    const auto tile_nx_y = static_cast<std::size_t>(roundf(pos_y / map.tile_height));
    if (map.is_solid(tile_nx_x, tile_nx_y))
    {
        vel_x = 0;
    }
    else
    {
        pos_x = new_pos_x;
        vel_x = acc_x * delta_time + vel_x;
    }

    const auto tile_ny_x = static_cast<std::size_t>(roundf(pos_x / map.tile_width));
    const auto tile_ny_y = static_cast<std::size_t>(roundf(new_pos_y / map.tile_height));
    if (map.is_solid(tile_ny_x, tile_ny_y))
    {
        vel_y = 0;
    }
    else
    {
        pos_y = new_pos_y;
        vel_y = acc_y * delta_time + vel_y;
    }
}

void ch::player::attack()
{
    attacking = true;
    attack_timer = 0.1f;
}

void ch::player::start_conversation(const ch::world &world, const std::size_t root_index)
{
    conversation_root = conversation_node = world.conversations.at(root_index).get();
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
                if (child->check_conditions(*this))
                {
                    conversation_node = child.get();

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
        if (child->type == ch::conversation_type::RESPONSE && child->check_conditions(*this))
        {
            valid_choice_index++;
            if (valid_choice_index == choice_index)
            {
                conversation_node = child.get();
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
        [status](const ch::quest_status &quest_status)
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
        [status](const ch::quest_status &quest_status)
        {
            return quest_status.quest_index == status.quest_index && quest_status.stage_index == status.stage_index;
        });
}
