#ifndef CH_TILESET_HPP
#define CH_TILESET_HPP

#include <string>
#include <vector>

namespace ch
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
        std::vector<ch::tile_data> tile_data;

        tileset(const std::string &filename);
    };
}

#endif
