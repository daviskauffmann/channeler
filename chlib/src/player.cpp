#include <ch/player.hpp>

#include <ch/conversation.hpp>
#include <ch/map.hpp>
#include <ch/tileset.hpp>
#include <ch/world.hpp>

void ch::player::update(const float delta_time)
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
        if (input_y == -1)
        {
            direction = ch::direction::up;
            animation = ch::animation::walking;
        }
        if (input_x == -1)
        {
            direction = ch::direction::left;
            animation = ch::animation::walking;
        }
        if (input_y == 1)
        {
            direction = ch::direction::down;
            animation = ch::animation::walking;
        }
        if (input_x == 1)
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

    constexpr auto speed = 100.0f;
    auto velocity = b2Vec2(input_x * speed, input_y * speed);

    const auto velocity_length = velocity.Length();
    if (velocity_length > speed)
    {
        velocity.x *= (1 / velocity_length) * speed;
        velocity.y *= (1 / velocity_length) * speed;
    }

    body->SetLinearVelocity(velocity);
}

void ch::player::attack()
{
    attacking = true;
    attack_timer = 0.1f;
}

void ch::player::start_conversation(const std::shared_ptr<const ch::world> world, const std::size_t root_index)
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
