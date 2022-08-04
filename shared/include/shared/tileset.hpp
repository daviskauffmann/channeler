#ifndef TILESET_HPP
#define TILESET_HPP

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
        std::string sprites_filename;
        std::vector<hp::tile_data> tile_data;

        tileset(const std::string &filename);
    };
}

#endif
