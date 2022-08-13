#include <ch/map.hpp>

#include <algorithm>
#include <ch/tileset.hpp>
#include <ch/world.hpp>
#include <fstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

ch::layer::layer(const nlohmann::json &layer_json)
{
    if (layer_json.contains("width"))
    {
        width = layer_json.at("width");
    }

    if (layer_json.contains("height"))
    {
        height = layer_json.at("height");
    }

    if (layer_json.contains("data"))
    {
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

    if (layer_json.contains("objects"))
    {
        for (const auto &object_json : layer_json.at("objects"))
        {
            ch::object object;
            object.gid = object_json.at("gid");
            object.x = object_json.at("x");
            object.y = object_json.at("y");
            object.rotation = object_json.at("rotation");

            objects.push_back(object);
        }
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
        layers.push_back({layer_json});
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
    const auto layer = std::find_if(
        layers.begin(),
        layers.end(),
        [this, x, y](const ch::layer &layer)
        {
            const auto datum = layer.get_datum(x, y);
            return datum && get_tileset(datum->gid).get_tile(datum->gid).solid;
        });
    return layer != layers.end();
}
