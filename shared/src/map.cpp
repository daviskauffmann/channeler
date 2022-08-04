#include <shared/map.hpp>

#include <fstream>
#include <nlohmann/json.hpp>
#include <shared/tileset.hpp>
#include <shared/world.hpp>

const hp::tile *hp::layer::get_tile(std::size_t x, std::size_t y) const
{
    if (x < width && y < height)
    {
        const hp::tile *tile = &tiles.at(x + y * width);
        if (tile->gid)
        {
            return tile;
        }
    }

    return NULL;
}

hp::tile_data *hp::map_tileset::get_tile_data(std::size_t gid)
{
    return &tileset->tile_data.at(gid - first_gid);
}

hp::map::map(const std::string &filename, hp::world &world)
    : filename(filename)
{
    auto map_json = nlohmann::json::parse(std::ifstream(filename));

    width = map_json.at("width");
    height = map_json.at("height");
    tile_width = map_json.at("tilewidth");
    tile_height = map_json.at("tileheight");

    for (const auto &layer_json : map_json.at("layers"))
    {
        if (layer_json.at("type") == "tilelayer")
        {
            hp::layer layer;

            layer.width = layer_json.at("width");
            layer.height = layer_json.at("height");

            for (const auto &tile_json : layer_json.at("data"))
            {
                const std::int64_t gid = tile_json;
                const std::uint32_t H_FLIP_FLAG = 0x80000000;
                const std::uint32_t V_FLIP_FLAG = 0x40000000;
                const std::uint32_t D_FLIP_FLAG = 0x20000000;

                hp::tile tile;
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
        hp::map_tileset map_tileset;

        map_tileset.index = index++;

        map_tileset.first_gid = tileset_json.at("firstgid");

        const auto source_json = std::string(tileset_json.at("source"));
        map_tileset.tileset = world.load_tileset("assets/" + source_json.substr(0, source_json.find_last_of(".")) + ".json");

        map_tilesets.push_back(map_tileset);
    }
}

void hp::map::update(float)
{
}

hp::map_tileset *hp::map::get_map_tileset(size_t gid)
{
    size_t index = 0;
    for (size_t i = 0; i < map_tilesets.size(); i++)
    {
        if (map_tilesets.at(i).first_gid <= gid)
        {
            index = i;
        }
    }
    return &map_tilesets[index];
}

bool hp::map::is_solid(std::size_t x, std::size_t y)
{
    for (auto &layer : layers)
    {
        const hp::tile *tile = layer.get_tile(x, y);
        if (tile)
        {
            hp::map_tileset *map_tileset = get_map_tileset(tile->gid);
            hp::tile_data *tile_data = map_tileset->get_tile_data(tile->gid);
            if (tile_data->solid)
            {
                return true;
            }
        }
    }

    return false;
}
