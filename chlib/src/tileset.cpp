#include <ch/tileset.hpp>

#include <fstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

ch::tileset::tileset(const std::string &filename)
{
    spdlog::info("Loading tileset {}", filename);

    const auto tileset_json = nlohmann::json::parse(std::ifstream(filename));

    columns = tileset_json.at("columns");

    const std::string image_string = tileset_json.at("image");
    image = "assets/" + image_string;

    for (const auto &tile_json : tileset_json.at("tiles"))
    {
        ch::tile tile;

        for (const auto &property_json : tile_json.at("properties"))
        {
            if (property_json.at("name") == "solid")
            {
                tile.solid = property_json.at("value");
            }
        }

        tiles.push_back(tile);
    }
}
