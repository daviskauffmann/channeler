#include <ch/map.hpp>

#include <ch/tileset.hpp>
#include <ch/world.hpp>
#include <fstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

ch::layer::layer(const nlohmann::json &layer_json)
{
    width = layer_json.at("width");
    height = layer_json.at("height");

    for (const auto &tile_json : layer_json.at("data"))
    {
        const std::int64_t gid = tile_json;
        constexpr std::uint32_t h_flip_flag = 0x80000000;
        constexpr std::uint32_t v_flip_flag = 0x40000000;
        constexpr std::uint32_t d_flip_flag = 0x20000000;

        ch::tile tile;
        tile.gid = gid & ~(h_flip_flag | v_flip_flag | d_flip_flag);
        tile.h_flip = gid & h_flip_flag;
        tile.v_flip = gid & v_flip_flag;
        tile.d_flip = gid & d_flip_flag;

        tiles.push_back(tile);
    }
}

const ch::tile *ch::layer::get_tile(const std::size_t x, const std::size_t y) const
{
    if (x < width && y < height)
    {
        const auto tile = &tiles.at(x + y * width);
        if (tile->gid)
        {
            return tile;
        }
    }

    return nullptr;
}

ch::map_tileset::map_tileset(const nlohmann::json &map_tileset_json, std::size_t index, ch::world &world)
    : index(index)
{
    first_gid = map_tileset_json.at("firstgid");

    const std::string source_string = map_tileset_json.at("source");
    tileset = world.load_tileset("assets/" + source_string.substr(0, source_string.find_last_of(".")) + ".json");
}

const ch::tile_data &ch::map_tileset::get_tile_data(const std::size_t gid) const
{
    return tileset->tile_data.at(gid - first_gid);
}

ch::map::map(const std::string &filename, ch::world &world)
    : filename(filename)
{
    spdlog::info("Loading map {}", filename);

    const auto map_json = nlohmann::json::parse(std::ifstream(filename));

    width = map_json.at("width");
    height = map_json.at("height");
    tile_width = map_json.at("tilewidth");
    tile_height = map_json.at("tileheight");

    for (const auto &layer_json : map_json.at("layers"))
    {
        if (layer_json.at("type") == "tilelayer")
        {
            layers.push_back({layer_json});
        }
        else if (layer_json.at("type") == "objectgroup")
        {
        }
    }

    std::size_t index = 0;
    for (const auto &map_tileset_json : map_json.at("tilesets"))
    {
        map_tilesets.push_back({map_tileset_json, index++, world});
    }
}

const ch::map_tileset &ch::map::get_map_tileset(const size_t gid) const
{
    std::size_t index = 0;
    for (std::size_t i = 0; i < map_tilesets.size(); i++)
    {
        if (map_tilesets.at(i).first_gid <= gid)
        {
            index = i;
        }
    }
    return map_tilesets.at(index);
}

bool ch::map::is_solid(const std::size_t x, const std::size_t y) const
{
    for (const auto &layer : layers)
    {
        const auto tile = layer.get_tile(x, y);
        if (tile)
        {
            const auto &map_tileset = get_map_tileset(tile->gid);
            const auto &tile_data = map_tileset.get_tile_data(tile->gid);
            if (tile_data.solid)
            {
                return true;
            }
        }
    }

    return false;
}
