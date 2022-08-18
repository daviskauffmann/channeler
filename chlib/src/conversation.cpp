#include <ch/conversation.hpp>

#include <ch/map.hpp>

ch::conversation::conversation(const nlohmann::json &conversation_json, const std::size_t root_index, std::size_t *const node_index)
    : root_index(root_index),
      node_index(*node_index)
{
    const std::string type_string = conversation_json.at("type");
    if (type_string == "root")
    {
        type = ch::conversation_type::root;
    }
    else if (type_string == "dialog")
    {
        type = ch::conversation_type::dialog;
    }
    else if (type_string == "response")
    {
        type = ch::conversation_type::response;
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

            condition.quest_status = std::make_unique<ch::quest_status>();
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

            effect.quest_status = std::make_unique<ch::quest_status>();
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
            children.push_back({child_json, root_index, node_index});
        }
    }
}

bool ch::conversation::has_response_nodes() const
{
    return std::any_of(
        children.begin(),
        children.end(),
        [](const auto &child)
        {
            return child.type == ch::conversation_type::response;
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

const ch::conversation *ch::conversation::find_by_node_index(const std::size_t index) const
{
    if (node_index == index)
    {
        return this;
    }

    for (const auto &child : children)
    {
        const auto child_ptr = child.find_by_node_index(index);
        if (child_ptr)
        {
            return child_ptr;
        }
    }

    return nullptr;
}

const ch::conversation *ch::conversation::find_by_id(const std::string &id_to_find) const
{
    if (!id.empty() && id == id_to_find)
    {
        return this;
    }

    for (const auto &child : children)
    {
        const auto child_ptr = child.find_by_id(id_to_find);
        if (child_ptr)
        {
            return child_ptr;
        }
    }

    return nullptr;
}
