#include <ch/layer.hpp>

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
        for (const auto &tile_json : layer_json.at("data"))
        {
            const std::int64_t gid = tile_json;
            constexpr std::uint32_t h_flip_flag = 0x80000000;
            constexpr std::uint32_t v_flip_flag = 0x40000000;
            constexpr std::uint32_t d_flip_flag = 0x20000000;

            ch::layer_tile tile;
            tile.gid = gid & ~(h_flip_flag | v_flip_flag | d_flip_flag);
            tile.h_flip = gid & h_flip_flag;
            tile.v_flip = gid & v_flip_flag;
            tile.d_flip = gid & d_flip_flag;

            tiles.push_back(tile);
        }
    }

    if (layer_json.contains("objects"))
    {
        for (const auto &object_json : layer_json.at("objects"))
        {
            ch::layer_object object;
            object.gid = object_json.at("gid");
            object.x = object_json.at("x");
            object.y = object_json.at("y");
            object.rotation = object_json.at("rotation");

            objects.push_back(object);
        }
    }
}

const ch::layer_tile *ch::layer::get_tile(const std::size_t x, const std::size_t y) const
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
