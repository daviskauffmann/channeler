#include "tileset.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

hp::tileset::tileset(const std::string &filename, SDL_Renderer *renderer)
{
    auto tileset_json = nlohmann::json::parse(std::ifstream(filename));

    columns = tileset_json.at("columns");

    const auto sprites_filename = "assets/" + std::string(tileset_json.at("image"));
    sprites = IMG_LoadTexture(renderer, sprites_filename.c_str());

    for (const auto &tile_json : tileset_json.at("tiles"))
    {
        hp::tile_data tile_datum;

        for (const auto &property_json : tile_json.at("properties"))
        {
            if (property_json.at("name") == "solid")
            {
                tile_datum.solid = property_json.at("value");
            }
        }

        tile_data.push_back(tile_datum);
    }
}

hp::tileset::~tileset()
{
    SDL_DestroyTexture(sprites);
}
