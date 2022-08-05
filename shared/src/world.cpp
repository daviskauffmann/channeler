#include <shared/world.hpp>

#include <fstream>
#include <nlohmann/json.hpp>
#include <shared/map.hpp>
#include <shared/tileset.hpp>

hp::world::world(const std::string &filename)
{
    const auto world_json = nlohmann::json::parse(std::ifstream(filename));

    for (const auto &map_json : world_json.at("maps"))
    {
        const auto filename_json = std::string(map_json.at("fileName"));
        const auto map_filename = "assets/" + filename_json.substr(0, filename_json.find_last_of(".")) + ".json";
        maps.push_back({map_filename, *this});
    }
}

hp::world::~world()
{
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
