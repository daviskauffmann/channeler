#include <shared/conversation_node.hpp>

#include <shared/map.hpp>
#include <shared/player.hpp>
#include <shared/quest_status.hpp>
#include <stdio.h>
#include <string.h>

hp::conversation_node::conversation_node(nlohmann::json conversation_json, size_t root_index, size_t *node_index)
    : root_index(root_index),
      node_index(*node_index)
{
    std::string type_json = conversation_json.at("type");
    if (type_json == "root")
    {
        type = hp::conversation_node_type::ROOT;
    }
    else if (type_json == "dialog")
    {
        type = hp::conversation_node_type::DIALOG;
    }
    else if (type_json == "response")
    {
        type = hp::conversation_node_type::RESPONSE;
    }

    if (conversation_json.contains("id"))
    {
        id = conversation_json.at("id");
    }

    if (conversation_json.contains("text"))
    {
        text = conversation_json.at("text");
    }

    if (conversation_json.contains("condition"))
    {
        auto condition_json = conversation_json.at("condition");

        if (condition_json.contains("quest_status"))
        {
            auto quest_status_json = condition_json.at("quest_status");

            condition.quest_status = new hp::quest_status;
            condition.quest_status->quest_index = quest_status_json.at("quest_index");
            condition.quest_status->stage_index = quest_status_json.at("stage_index");
        }
    }

    if (conversation_json.contains("effect"))
    {
        auto effect_json = conversation_json.at("effect");

        if (effect_json.contains("quest_status"))
        {
            auto quest_status_json = effect_json.at("quest_status");

            effect.quest_status = new hp::quest_status;
            effect.quest_status->quest_index = quest_status_json.at("quest_index");
            effect.quest_status->stage_index = quest_status_json.at("stage_index");
        }
    }

    if (conversation_json.contains("jump_id"))
    {
        jump_id = conversation_json.at("jump_id");
    }

    if (conversation_json.contains("children"))
    {
        for (const auto &child_json : conversation_json.at("children"))
        {
            (*node_index)++;
            children.push_back(new hp::conversation_node(child_json, root_index, node_index));
        }
    }
}

hp::conversation_node::~conversation_node()
{
    if (condition.quest_status)
    {
        delete condition.quest_status;
    }

    if (effect.quest_status)
    {
        delete effect.quest_status;
    }

    for (auto child : children)
    {
        delete child;
    }
}

bool hp::conversation_node::has_response_nodes() const
{
    for (const auto child : children)
    {
        if (child->type == hp::conversation_node_type::RESPONSE)
        {
            return true;
        }
    }

    return false;
}

bool hp::conversation_node::check_conditions(hp::player *player) const
{
    if (condition.quest_status)
    {
        if (!player->check_quest_status(*condition.quest_status))
        {
            return false;
        }
    }

    return true;
}

hp::conversation_node *hp::conversation_node::find_by_node_index(std::size_t index)
{
    if (node_index == index)
    {
        return this;
    }

    for (auto child : children)
    {
        auto result = child->find_by_node_index(index);
        if (result)
        {
            return result;
        }
    }

    return nullptr;
}

hp::conversation_node *hp::conversation_node::find_by_id(const std::string &_id)
{
    if (!id.empty() && id == _id)
    {
        return this;
    }

    for (auto child : children)
    {
        auto result = child->find_by_id(_id);
        if (result)
        {
            return result;
        }
    }

    return nullptr;
}
