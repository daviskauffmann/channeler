#include <shared/conversation.hpp>

#include <shared/map.hpp>
#include <shared/player.hpp>
#include <stdio.h>
#include <string.h>

hp::conversation::conversation(const nlohmann::json &conversation_json, const std::size_t root_index, std::size_t *const node_index)
    : root_index(root_index),
      node_index(*node_index)
{
    const std::string type_string = conversation_json.at("type");
    if (type_string == "root")
    {
        type = hp::conversation_type::ROOT;
    }
    else if (type_string == "dialog")
    {
        type = hp::conversation_type::DIALOG;
    }
    else if (type_string == "response")
    {
        type = hp::conversation_type::RESPONSE;
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
        const auto condition_json = conversation_json.at("condition");

        if (condition_json.contains("quest_status"))
        {
            const auto quest_status_json = condition_json.at("quest_status");

            condition.quest_status = new hp::quest_status;
            condition.quest_status->quest_index = quest_status_json.at("quest_index");
            condition.quest_status->stage_index = quest_status_json.at("stage_index");
        }
    }

    if (conversation_json.contains("effect"))
    {
        const auto effect_json = conversation_json.at("effect");

        if (effect_json.contains("quest_status"))
        {
            const auto quest_status_json = effect_json.at("quest_status");

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
            children.push_back(new hp::conversation(child_json, root_index, node_index));
        }
    }
}

hp::conversation::~conversation()
{
    if (condition.quest_status)
    {
        delete condition.quest_status;
    }

    if (effect.quest_status)
    {
        delete effect.quest_status;
    }

    for (const auto &child : children)
    {
        delete child;
    }
}

bool hp::conversation::has_response_nodes() const
{
    for (const auto &child : children)
    {
        if (child->type == hp::conversation_type::RESPONSE)
        {
            return true;
        }
    }

    return false;
}

bool hp::conversation::check_conditions(const hp::player &player) const
{
    if (condition.quest_status)
    {
        if (!player.check_quest_status(*condition.quest_status))
        {
            return false;
        }
    }

    return true;
}

hp::conversation *hp::conversation::find_by_node_index(const std::size_t index)
{
    if (node_index == index)
    {
        return this;
    }

    for (const auto &child : children)
    {
        const auto result = child->find_by_node_index(index);
        if (result)
        {
            return result;
        }
    }

    return nullptr;
}

hp::conversation *hp::conversation::find_by_id(const std::string &id_to_find)
{
    if (!id.empty() && id == id_to_find)
    {
        return this;
    }

    for (const auto &child : children)
    {
        const auto result = child->find_by_id(id_to_find);
        if (result)
        {
            return result;
        }
    }

    return nullptr;
}
