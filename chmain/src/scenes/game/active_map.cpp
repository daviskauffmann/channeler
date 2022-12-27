#include "active_map.hpp"

#include "loaded_tileset.hpp"
#include <ch/map.hpp>

ch::active_map::active_map(const ch::map &map, SDL_Renderer *const renderer)
{
    std::transform(
        map.tilesets.begin(),
        map.tilesets.end(),
        std::back_inserter(loaded_tilesets),
        [renderer](const auto &map_tileset)
        {
            return std::make_unique<ch::loaded_tileset>(map_tileset, renderer);
        });
}
