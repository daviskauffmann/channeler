#include <shared/world.hpp>

#include <fstream>
#include <nlohmann/json.hpp>
#include <shared/conversation.hpp>
#include <shared/map.hpp>
#include <shared/tileset.hpp>

hp::world::world(
    const std::string &world_filename,
    const std::string &quests_filename,
    const std::string &conversations_filename)
{
    {
        const auto world_json = nlohmann::json::parse(std::ifstream(world_filename));

        for (const auto &map_json : world_json.at("maps"))
        {
            const auto filename_json = std::string(map_json.at("fileName"));
            const auto map_filename = "assets/" + filename_json.substr(0, filename_json.find_last_of(".")) + ".json";
            maps.push_back({map_filename, *this});
        }
    }

    {
        const auto quests_json = nlohmann::json::parse(std::ifstream(quests_filename));

        for (const auto &quest_json : quests_json)
        {
            struct hp::quest quest;

            quest.name = quest_json.at("name");

            for (const auto &stage_json : quest_json.at("stages"))
            {
                struct hp::quest_stage stage;

                stage.description = stage_json.at("description");

                quest.stages.push_back(stage);
            }

            quests.push_back(quest);
        }
    }

    {
        const auto conversations_json = nlohmann::json::parse(std::ifstream(conversations_filename));

        std::size_t root_index = 0;
        for (const auto &conversation_json : conversations_json)
        {
            std::size_t node_index = 0;
            conversations.push_back(new hp::conversation(conversation_json, root_index++, &node_index));
        }
    }
}

hp::world::~world()
{
    for (const auto &conversation : conversations)
    {
        delete conversation;
    }

    for (const auto &[filename, tileset] : tilesets)
    {
        delete tileset;
    }
}

hp::tileset *hp::world::load_tileset(const std::string &filename)
{
    if (tilesets.find(filename) == tilesets.end())
    {
        tilesets.insert({filename, new hp::tileset(filename)});
    }

    return tilesets.at(filename);
}

std::size_t hp::world::get_map_index(const std::string &filename) const
{
    for (std::size_t i = 0; i < maps.size(); i++)
    {
        if (maps.at(i).filename == filename)
        {
            return i;
        }
    }
    return maps.size();
}
