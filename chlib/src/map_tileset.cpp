#include <ch/map_tileset.hpp>

#include <ch/tileset.hpp>
#include <ch/world.hpp>

ch::map_tileset::map_tileset(const nlohmann::json &tileset_json, const std::size_t index, ch::world &world)
    : index(index)
{
    first_gid = tileset_json.at("firstgid");

    const std::string source_string = tileset_json.at("source");
    tileset = world.load_tileset("assets/" + source_string.substr(0, source_string.find_last_of(".")) + ".json");
}

const ch::tileset_tile &ch::map_tileset::get_tile(const std::size_t gid) const
{
    return tileset->tiles.at(gid - first_gid);
}
