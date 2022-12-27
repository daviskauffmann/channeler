#include <ch/tileset.hpp>

#include <fstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

ch::tileset::tileset(const std::string &filename)
{
    spdlog::info("Loading tileset {}", filename);

    const auto tileset_json = nlohmann::json::parse(std::ifstream(filename));

    columns = tileset_json.at("columns");

    if (tileset_json.contains("image"))
    {
        const std::string image_string = tileset_json.at("image");
        image = "assets/" + image_string;
    }

    for (const auto &tile_json : tileset_json.at("tiles"))
    {
        ch::tileset_tile tile;

        tile.index = tile_json.at("id");

        if (tile_json.contains("image"))
        {
            const std::string image_string = tile_json.at("image");
            tile.image = "assets/" + image_string;
        }

        if (tile_json.contains("imagewidth"))
        {
            tile.width = tile_json.at("imagewidth");
        }

        if (tile_json.contains("imageheight"))
        {
            tile.height = tile_json.at("imageheight");
        }

        if (tile_json.contains("properties"))
        {
            for (const auto &property_json : tile_json.at("properties"))
            {
                if (property_json.at("name") == "solid")
                {
                    tile.solid = property_json.at("value");
                }
            }
        }

        tiles.push_back(tile);
    }
}
