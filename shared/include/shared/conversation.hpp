#ifndef CONVERSATION_NODE_HPP
#define CONVERSATION_NODE_HPP

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace hp
{
    class player;
    struct quest_status;

    enum class conversation_type
    {
        ROOT,
        DIALOG,
        RESPONSE,
    };

    struct conversation_condition
    {
        hp::quest_status *quest_status = nullptr;
    };

    struct conversation_effect
    {
        hp::quest_status *quest_status = nullptr;
    };

    class conversation
    {
    public:
        const std::size_t root_index;
        const std::size_t node_index;

        hp::conversation_type type;
        std::string id = "";
        hp::conversation_condition condition;
        hp::conversation_effect effect;
        std::string text = "";
        std::string jump_id = "";

        std::vector<hp::conversation *> children;

        conversation(const nlohmann::json &conversation_json, std::size_t root_index, std::size_t *node_index);
        ~conversation();

        bool has_response_nodes() const;
        bool check_conditions(const hp::player &player) const;

        hp::conversation *find_by_node_index(std::size_t index);
        hp::conversation *find_by_id(const std::string &id_to_find);
    };
}

#endif
