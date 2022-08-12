#ifndef CH_MAP_HPP
#define CH_MAP_HPP

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace ch
{
    class world;
    struct tileset;
    struct tile;

    struct datum
    {
        std::size_t gid = 0;
        bool h_flip = false;
        bool v_flip = false;
        bool d_flip = false;
    };

    struct layer
    {
        std::size_t width;
        std::size_t height;
        std::vector<ch::datum> data;

        layer(const nlohmann::json &layer_json);

        const ch::datum *get_datum(std::size_t x, std::size_t y) const;
    };

    struct map_tileset
    {
        std::size_t index;

        std::size_t first_gid;
        const ch::tileset *tileset;

        map_tileset(const nlohmann::json &tileset_json, std::size_t index, ch::world &world);

        const ch::tile &get_tile(std::size_t gid) const;
    };

    struct map
    {
        const std::string filename;

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
