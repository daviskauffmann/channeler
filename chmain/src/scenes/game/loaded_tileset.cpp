#include "loaded_tileset.hpp"

#include "../../texture.hpp"
#include <ch/map.hpp>
#include <spdlog/spdlog.h>

ch::loaded_tileset::loaded_tileset(const ch::map_tileset &map_tileset, SDL_Renderer *const renderer)
{
    if (!map_tileset.tileset->image.empty())
    {
        spdlog::info("Loading tileset image {}", map_tileset.tileset->image);
        image = std::make_unique<ch::texture>(renderer, map_tileset.tileset->image.c_str());
    }

    for (const auto &tile : map_tileset.tileset->tiles)
    {
        if (!tile.image.empty())
        {
            spdlog::info("Loading tile image {}", tile.image);
            tile_images.push_back(std::make_unique<ch::texture>(renderer, tile.image.c_str()));
        }
    }
}
