#include <ch/world.hpp>

#include <algorithm>
#include <ch/conversation.hpp>
#include <ch/map.hpp>
#include <ch/tileset.hpp>
#include <fstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

ch::world::world(
    const std::string &world_filename,
    const std::string &quests_filename,
    const std::string &conversations_filename,
    const std::string &items_filename)
{
    {
        spdlog::info("Loading world {}", world_filename);

        const auto world_json = nlohmann::json::parse(std::ifstream(world_filename));

        for (const auto &map_json : world_json.at("maps"))
        {
            const auto filename_json = std::string(map_json.at("fileName"));
            const auto map_filename = "assets/" + filename_json.substr(0, filename_json.find_last_of(".")) + ".json";
            maps.push_back({map_filename, *this});
        }
    }

    {
        spdlog::info("Loading quests {}", quests_filename);

        const auto quests_json = nlohmann::json::parse(std::ifstream(quests_filename));

        for (const auto &quest_json : quests_json)
        {
            quests.push_back({quest_json});
        }
    }

    {
        spdlog::info("Loading conversations {}", conversations_filename);

        const auto conversations_json = nlohmann::json::parse(std::ifstream(conversations_filename));

        std::size_t root_index = 0;
        for (const auto &conversation_json : conversations_json)
        {
            std::size_t node_index = 0;
            conversations.push_back({conversation_json, root_index++, &node_index});
        }
    }

    {
        spdlog::info("Loading items {}", items_filename);

        const auto items_json = nlohmann::json::parse(std::ifstream(items_filename));

        for (const auto &item_json : items_json)
        {
            items.push_back({item_json});
        }
    }
}

std::shared_ptr<ch::tileset> ch::world::load_tileset(const std::string &filename)
{
    if (!tilesets.contains(filename))
    {
        tilesets.insert({filename, std::make_unique<ch::tileset>(filename)});
    }

    return tilesets.at(filename);
}

std::size_t ch::world::get_map_index(const std::string &filename) const
{
    return std::distance(
        maps.begin(),
        std::find_if(
            maps.begin(),
            maps.end(),
            [filename](const auto &map)
            {
                return map.filename == filename;
            }));
}

void ch::world::update(const float delta_time)
{
    for (auto &map : maps)
    {
        map.update(delta_time);
    }
}
