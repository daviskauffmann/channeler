#ifndef CONVERSATION_NODE_HPP
#define CONVERSATION_NODE_HPP

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace hp
{
    class player;
    struct quest_status;

    enum class conversation_node_type
    {
        ROOT,
        DIALOG,
        RESPONSE,
    };

    struct conversation_node_condition
    {
        hp::quest_status *quest_status = nullptr;
    };

    struct conversation_node_effect
    {
        hp::quest_status *quest_status = nullptr;
    };

    class conversation_node
    {
    public:
        std::size_t root_index;
        std::size_t node_index;

        hp::conversation_node_type type;
        std::string id = "";
        hp::conversation_node_condition condition;
        hp::conversation_node_effect effect;

        std::string text = "";

        std::string jump_id = "";

        std::vector<hp::conversation_node *> children;

        conversation_node(nlohmann::json conversation_json, std::size_t root_index, std::size_t *node_index);
        ~conversation_node();

        bool has_response_nodes() const;
        bool check_conditions(hp::player *player) const;

        hp::conversation_node *find_by_node_index(std::size_t index);
        hp::conversation_node *find_by_id(const std::string &id);
    };
}

#endif
