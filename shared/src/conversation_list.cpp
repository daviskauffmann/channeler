#include <shared/conversation_list.hpp>

#include <fstream>
#include <nlohmann/json.hpp>
#include <shared/conversation.hpp>

hp::conversation_list::conversation_list(const std::string &filename)
{
    const auto conversation_list_json = nlohmann::json::parse(std::ifstream(filename));

    std::size_t root_index = 0;
    for (const auto &conversation_json : conversation_list_json.at("conversations"))
    {
        std::size_t node_index = 0;
        conversations.push_back(new conversation(conversation_json, root_index++, &node_index));
    }
}

hp::conversation_list::~conversation_list()
{
    for (const auto &root : conversations)
    {
        delete root;
    }
}
