#ifndef CH_MAP_TILESET_HPP
#define CH_MAP_TILESET_HPP

#include <memory>
#include <tinyxml2.h>

namespace ch
{
    struct tileset;
    struct tileset_tile;
    class world;

    struct map_tileset
    {
        std::size_t index;

        std::size_t first_gid;
        std::shared_ptr<ch::tileset> tileset;

        map_tileset(const tinyxml2::XMLElement *tileset_xml, std::size_t index, ch::world &world);

        const ch::tileset_tile &get_tile(std::size_t gid) const;
    };
}

#endif
