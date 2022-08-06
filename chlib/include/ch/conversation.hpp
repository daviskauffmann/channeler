#ifndef CH_CONVERSATION_NODE_HPP
#define CH_CONVERSATION_NODE_HPP

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace ch
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
        ch::quest_status *quest_status = nullptr;
    };

    struct conversation_effect
    {
        ch::quest_status *quest_status = nullptr;
    };

    class conversation
    {
    public:
        const std::size_t root_index;
        const std::size_t node_index;

        ch::conversation_type type;
        std::string id = "";
        ch::conversation_condition condition;
        ch::conversation_effect effect;
        std::string text = "";
        std::string jump_id = "";

        std::vector<ch::conversation *> children;

        conversation(const nlohmann::json &conversation_json, std::size_t root_index, std::size_t *node_index);
        ~conversation();

        bool has_response_nodes() const;
        bool check_conditions(const ch::player &player) const;

        ch::conversation *find_by_node_index(std::size_t index);
        ch::conversation *find_by_id(const std::string &id_to_find);
    };
}

#endif
