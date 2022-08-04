#include <shared/player.hpp>

#include <shared/conversation_node.hpp>
#include <shared/conversations.hpp>
#include <shared/input.hpp>
#include <shared/map.hpp>
#include <shared/quest_status.hpp>
#include <shared/tileset.hpp>

hp::player::player(std::size_t map_index)
    : map_index(map_index)
{
}

void hp::player::update(struct input *input, hp::map *map, float delta_time)
{
    animation = hp::animation::IDLE;
    if (input->dy == -1)
    {
        direction = hp::direction::UP;
        animation = hp::animation::WALKING;
    }
    if (input->dx == -1)
    {
        direction = hp::direction::LEFT;
        animation = hp::animation::WALKING;
    }
    if (input->dy == 1)
    {
        direction = hp::direction::DOWN;
        animation = hp::animation::WALKING;
    }
    if (input->dx == 1)
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

    acc_x = (float)input->dx;
    acc_y = (float)input->dy;

    float acc_len = sqrtf(powf(acc_x, 2) + powf(acc_y, 2));
    if (acc_len > 1)
    {
        acc_x /= acc_len;
        acc_y /= acc_len;
    }

    float speed = 1000;
    acc_x *= speed;
    acc_y *= speed;

    float drag = 20;
    acc_x -= vel_x * drag;
    acc_y -= vel_y * drag;

    float new_pos_x = 0.5f * acc_x * powf(delta_time, 2) + vel_x * delta_time + pos_x;
    float new_pos_y = 0.5f * acc_y * powf(delta_time, 2) + vel_y * delta_time + pos_y;

    int tile_nx_x = (int)roundf(new_pos_x / map->tile_width);
    int tile_nx_y = (int)roundf(pos_y / map->tile_height);
    if (map->is_solid(tile_nx_x, tile_nx_y))
    {
        vel_x = 0;
    }
    else
    {
        pos_x = new_pos_x;
        vel_x = acc_x * delta_time + vel_x;
    }

    int tile_ny_x = (int)roundf(pos_x / map->tile_width);
    int tile_ny_y = (int)roundf(new_pos_y / map->tile_height);
    if (map->is_solid(tile_ny_x, tile_ny_y))
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

void hp::player::start_conversation(hp::conversations *conversations, std::size_t root_index)
{
    conversation_root = conversation_node = conversations->conversation_roots[root_index];
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
                if (child->check_conditions(this))
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

void hp::player::choose_conversation_response(std::size_t choice_index)
{
    size_t valid_choice_index = 0;
    for (auto child : conversation_node->children)
    {
        if (child->type == hp::conversation_node_type::RESPONSE && child->check_conditions(this))
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

void hp::player::set_quest_status(hp::quest_status status)
{
    for (auto quest_status : quest_statuses)
    {
        if (quest_status.quest_index == status.quest_index)
        {
            quest_status.stage_index = status.stage_index;
            return;
        }
    }

    quest_statuses.push_back(status);
}

bool hp::player::check_quest_status(hp::quest_status status)
{
    for (auto quest_status : quest_statuses)
    {
        if (quest_status.quest_index == status.quest_index && quest_status.stage_index == status.stage_index)
        {
            return true;
        }
    }

    return false;
}
