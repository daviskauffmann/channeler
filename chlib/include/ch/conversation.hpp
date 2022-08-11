#ifndef CH_CONVERSATION_NODE_HPP
#define CH_CONVERSATION_NODE_HPP

#include "player.hpp"
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace ch
{
    enum class conversation_type
    {
        ROOT,
        DIALOG,
        RESPONSE,
    };

    struct conversation_condition
    {
        std::unique_ptr<ch::quest_status> quest_status;
    };

    struct conversation_effect
    {
        std::unique_ptr<ch::quest_status> quest_status;
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

        std::vector<std::unique_ptr<ch::conversation>> children;

        conversation(const nlohmann::json &conversation_json, std::size_t root_index, std::size_t *node_index);

        bool has_response_nodes() const;
        bool check_conditions(const ch::player &player) const;

        const ch::conversation *find_by_node_index(std::size_t index) const;
        const ch::conversation *find_by_id(const std::string &id_to_find) const;
    };
}

#endif
