#ifndef CH_TILESET_HPP
#define CH_TILESET_HPP

#include <string>
#include <vector>

namespace ch
{
    struct tile
    {
        bool solid;
    };

    struct tileset
    {
        std::size_t columns;
        std::string image;
        std::vector<ch::tile> tiles;

        tileset(const std::string &filename);
    };
}

#endif
