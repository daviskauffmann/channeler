#ifndef CH_MAP_HPP
#define CH_MAP_HPP

#include "layer.hpp"
#include "map_tileset.hpp"
#include "tileset.hpp"
#include <memory>
#include <string>
#include <vector>

namespace ch
{
    class world;

    struct map
    {
        std::string filename;

        std::size_t width;
        std::size_t height;
        std::size_t tile_width;
        std::size_t tile_height;
        std::vector<ch::layer> layers;
        std::vector<ch::map_tileset> tilesets;

        map(const std::string &filename, ch::world &world);

        const ch::map_tileset &get_tileset(std::size_t gid) const;

        bool is_solid(std::size_t x, std::size_t y) const;
    };
};

#endif
