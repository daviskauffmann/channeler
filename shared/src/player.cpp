#include <shared/player.hpp>

#include <shared/conversation.hpp>
#include <shared/input.hpp>
#include <shared/map.hpp>
#include <shared/tileset.hpp>
#include <shared/world.hpp>

void hp::player::update(const hp::input &input, const hp::map &map, const float delta_time)
{
    animation = hp::animation::IDLE;
    if (input.dy == -1)
    {
        direction = hp::direction::UP;
        animation = hp::animation::WALKING;
    }
    if (input.dx == -1)
    {
        direction = hp::direction::LEFT;
        animation = hp::animation::WALKING;
    }
    if (input.dy == 1)
    {
        direction = hp::direction::DOWN;
        animation = hp::animation::WALKING;
    }
    if (input.dx == 1)
    {
        direction = hp::direction::RIGHT;
        animation = hp::animation::WALKING;
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

    const auto speed = 1000.0f;
    acc_x *= speed;
    acc_y *= speed;

    const auto drag = 20.0f;
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

void hp::player::attack()
{
}

void hp::player::start_conversation(const hp::world &world, const std::size_t root_index)
{
    conversation_root = conversation_node = world.conversations.at(root_index);
    advance_conversation();
}

void hp::player::advance_conversation()
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
            for (auto child : conversation_node->children)
            {
                if (child->check_conditions(*this))
                {
                    conversation_node = child;

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

void hp::player::choose_conversation_response(const std::size_t choice_index)
{
    std::size_t valid_choice_index = 0;
    for (auto child : conversation_node->children)
    {
        if (child->type == hp::conversation_type::RESPONSE && child->check_conditions(*this))
        {
            valid_choice_index++;
            if (valid_choice_index == choice_index)
            {
                conversation_node = child;
                advance_conversation();
                break;
            }
        }
    }
}

void hp::player::end_conversation()
{
    conversation_root = conversation_node = nullptr;
}

void hp::player::set_quest_status(const hp::quest_status &status)
{
    const auto quest_status = std::find_if(
        quest_statuses.begin(),
        quest_statuses.end(),
        [status](const hp::quest_status &quest_status)
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
}

bool hp::player::check_quest_status(const hp::quest_status &status) const
{
    return std::any_of(
        quest_statuses.begin(),
        quest_statuses.end(),
        [status](const hp::quest_status &quest_status)
        {
            return quest_status.quest_index == status.quest_index && quest_status.stage_index == status.stage_index;
        });
}
