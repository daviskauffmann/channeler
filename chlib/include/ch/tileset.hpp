#ifndef CH_TILESET_HPP
#define CH_TILESET_HPP

#include <string>
#include <vector>

namespace ch
{
    struct tileset_tile
    {
        std::size_t index;

        std::string image = "";
        std::size_t width = 0;
        std::size_t height = 0;

        bool solid = false;
    };

    struct tileset
    {
        std::size_t columns;
        std::string image = "";
        std::vector<ch::tileset_tile> tiles;

        tileset(const std::string &filename);
    };
}

#endif
