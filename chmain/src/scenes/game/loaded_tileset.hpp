#ifndef CH_SCENES_LOADED_TILESET_HPP
#define CH_SCENES_LOADED_TILESET_HPP

#include "../../texture.hpp"
#include <SDL2/SDL.h>
#include <memory>
#include <vector>

namespace ch
{
    struct map_tileset;

    struct loaded_tileset
    {
        std::unique_ptr<ch::texture> image;
        std::vector<std::unique_ptr<ch::texture>> tile_images;

        loaded_tileset(const ch::map_tileset &map_tileset, SDL_Renderer *renderer);
    };
}

#endif
