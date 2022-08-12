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

    for (const auto &datum_json : layer_json.at("data"))
    {
        const std::int64_t gid = datum_json;
        constexpr std::uint32_t h_flip_flag = 0x80000000;
        constexpr std::uint32_t v_flip_flag = 0x40000000;
        constexpr std::uint32_t d_flip_flag = 0x20000000;

        ch::datum datum;
        datum.gid = gid & ~(h_flip_flag | v_flip_flag | d_flip_flag);
        datum.h_flip = gid & h_flip_flag;
        datum.v_flip = gid & v_flip_flag;
        datum.d_flip = gid & d_flip_flag;

        data.push_back(datum);
    }
}

const ch::datum *ch::layer::get_datum(const std::size_t x, const std::size_t y) const
{
    if (x < width && y < height)
    {
        const auto datum = &data.at(x + y * width);
        if (datum->gid)
        {
            return datum;
        }
    }

    return nullptr;
}

ch::map_tileset::map_tileset(const nlohmann::json &tileset_json, const std::size_t index, ch::world &world)
    : index(index)
{
    first_gid = tileset_json.at("firstgid");

    const std::string source_string = tileset_json.at("source");
    tileset = world.load_tileset("assets/" + source_string.substr(0, source_string.find_last_of(".")) + ".json");
}

const ch::tile &ch::map_tileset::get_tile(const std::size_t gid) const
{
    return tileset->tiles.at(gid - first_gid);
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
    for (const auto &tileset_json : map_json.at("tilesets"))
    {
        tilesets.push_back({tileset_json, index++, world});
    }
}

const ch::map_tileset &ch::map::get_tileset(const size_t gid) const
{
    std::size_t index = 0;
    for (std::size_t i = 0; i < tilesets.size(); i++)
    {
        if (tilesets.at(i).first_gid <= gid)
        {
            index = i;
        }
    }
    return tilesets.at(index);
}

bool ch::map::is_solid(const std::size_t x, const std::size_t y) const
{
    for (const auto &layer : layers)
    {
        const auto datum = layer.get_datum(x, y);
        if (datum)
        {
            const auto &map_tileset = get_tileset(datum->gid);
            const auto &tile = map_tileset.get_tile(datum->gid);
            if (tile.solid)
            {
                return true;
            }
        }
    }

    return false;
}
