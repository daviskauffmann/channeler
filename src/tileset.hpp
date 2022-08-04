#ifndef TILESET_HPP
#define TILESET_HPP

#include <SDL2/SDL_image.h>
#include <string>
#include <vector>

namespace hp
{
    struct tile_data
    {
        bool solid;
    };

    class tileset
    {
    public:
        std::size_t columns;
        SDL_Texture *sprites;
        std::vector<hp::tile_data> tile_data;

        tileset(const std::string &filename, SDL_Renderer *renderer);
        ~tileset();
    };
}

#endif
