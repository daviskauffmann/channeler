#ifndef WORLD_HPP
#define WORLD_HPP

#include "map.hpp"
#include <SDL2/SDL_image.h>
#include <unordered_map>
#include <vector>

namespace hp
{
    class tileset;

    class world
    {
    public:
        std::vector<hp::map> maps;

        world(const std::string &filename, SDL_Renderer *renderer);
        ~world();

        tileset *load_tileset(const std::string &filename);

        std::size_t get_map_index(const std::string &filename) const;

    private:
        SDL_Renderer *renderer;
        std::unordered_map<std::string, hp::tileset *> tilesets;
    };
}

#endif
