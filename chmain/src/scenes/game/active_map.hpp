#ifndef CH_ACTIVE_MAP_HPP
#define CH_ACTIVE_MAP_HPP

#include <SDL2/SDL.h>
#include <memory>
#include <vector>

namespace ch
{
    struct loaded_tileset;
    struct map;

    struct active_map
    {
        std::vector<std::unique_ptr<ch::loaded_tileset>> loaded_tilesets;

        active_map(const ch::map &map, SDL_Renderer *renderer);
    };
}

#endif
