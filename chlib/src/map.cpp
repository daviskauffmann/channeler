#include <ch/map.hpp>

#include <ch/tileset.hpp>
#include <ch/world.hpp>
#include <fstream>
#include <nlohmann/json.hpp>

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

const ch::tile_data &ch::map_tileset::get_tile_data(const std::size_t gid) const
{
    return tileset->tile_data.at(gid - first_gid);
}

ch::map::map(const std::string &filename, ch::world &world)
    : filename(filename)
{
    const auto map_json = nlohmann::json::parse(std::ifstream(filename));

    width = map_json.at("width");
    height = map_json.at("height");
    tile_width = map_json.at("tilewidth");
    tile_height = map_json.at("tileheight");

    for (const auto &layer_json : map_json.at("layers"))
    {
        if (layer_json.at("type") == "tilelayer")
        {
            ch::layer layer;

            layer.width = layer_json.at("width");
            layer.height = layer_json.at("height");

            for (const auto &tile_json : layer_json.at("data"))
            {
                const std::int64_t gid = tile_json;
                const std::uint32_t H_FLIP_FLAG = 0x80000000;
                const std::uint32_t V_FLIP_FLAG = 0x40000000;
                const std::uint32_t D_FLIP_FLAG = 0x20000000;

                ch::tile tile;
                tile.gid = gid & ~(H_FLIP_FLAG | V_FLIP_FLAG | D_FLIP_FLAG);
                tile.h_flip = gid & H_FLIP_FLAG;
                tile.v_flip = gid & V_FLIP_FLAG;
                tile.d_flip = gid & D_FLIP_FLAG;
                layer.tiles.push_back(tile);
            }

            layers.push_back(layer);
        }
    }

    std::size_t index = 0;
    for (const auto &tileset_json : map_json.at("tilesets"))
    {
        ch::map_tileset map_tileset;

        map_tileset.index = index++;

        map_tileset.first_gid = tileset_json.at("firstgid");

        const std::string source_string = tileset_json.at("source");
        map_tileset.tileset = world.load_tileset("assets/" + source_string.substr(0, source_string.find_last_of(".")) + ".json");

        map_tilesets.push_back(map_tileset);
    }
}

void ch::map::update(float)
{
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
