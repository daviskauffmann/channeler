#include <ch/conversation.hpp>

#include <ch/map.hpp>
#include <ch/player.hpp>

ch::conversation::conversation(const nlohmann::json &conversation_json, const std::size_t root_index, std::size_t *const node_index)
    : root_index(root_index),
      node_index(*node_index)
{
    const std::string type_string = conversation_json.at("type");
    if (type_string == "root")
    {
        type = ch::conversation_type::ROOT;
    }
    else if (type_string == "dialog")
    {
        type = ch::conversation_type::DIALOG;
    }
    else if (type_string == "response")
    {
        type = ch::conversation_type::RESPONSE;
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

            condition.quest_status = new ch::quest_status;
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

            effect.quest_status = new ch::quest_status;
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
            children.push_back(new ch::conversation(child_json, root_index, node_index));
        }
    }
}

ch::conversation::~conversation()
{
    if (condition.quest_status)
    {
        delete condition.quest_status;
    }

    if (effect.quest_status)
    {
        delete effect.quest_status;
    }

    for (const auto child : children)
    {
        delete child;
    }
}

bool ch::conversation::has_response_nodes() const
{
    return std::any_of(
        children.begin(),
        children.end(),
        [](const ch::conversation *const child)
        {
            return child->type == ch::conversation_type::RESPONSE;
        });
}

bool ch::conversation::check_conditions(const ch::player &player) const
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

ch::conversation *ch::conversation::find_by_node_index(const std::size_t index)
{
    if (node_index == index)
    {
        return this;
    }

    for (const auto child : children)
    {
        const auto result = child->find_by_node_index(index);
        if (result)
        {
            return result;
        }
    }

    return nullptr;
}

ch::conversation *ch::conversation::find_by_id(const std::string &id_to_find)
{
    if (!id.empty() && id == id_to_find)
    {
        return this;
    }

    for (const auto child : children)
    {
        const auto result = child->find_by_id(id_to_find);
        if (result)
        {
            return result;
        }
    }

    return nullptr;
}
