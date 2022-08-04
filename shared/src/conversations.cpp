#include <shared/conversations.hpp>

#include <fstream>
#include <nlohmann/json.hpp>
#include <shared/conversation_node.hpp>

hp::conversations::conversations(const std::string &filename)
{
    auto conversations_json = nlohmann::json::parse(std::ifstream(filename));

    std::size_t root_index = 0;
    for (const auto &conversation_json : conversations_json.at("conversations"))
    {
        std::size_t node_index = 0;
        conversation_roots.push_back(new conversation_node(conversation_json, root_index++, &node_index));
    }
}

hp::conversations::~conversations()
{
    for (auto root : conversation_roots)
    {
        delete root;
    }
}
