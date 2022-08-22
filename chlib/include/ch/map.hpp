#ifndef CH_MAP_HPP
#define CH_MAP_HPP

#include "tileset.hpp"
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace ch
{
    class world;

    struct datum
    {
        std::size_t gid;
        bool h_flip;
        bool v_flip;
        bool d_flip;
    };

    struct object
    {
        std::size_t gid;
        float x;
        float y;
        float rotation;
    };

    struct layer
    {
        std::size_t width = 0;
        std::size_t height = 0;
        std::vector<ch::datum> data;
        std::vector<ch::object> objects;

        layer(const nlohmann::json &layer_json);

        const ch::datum *get_datum(std::size_t x, std::size_t y) const;
    };

    struct map_tileset
    {
        std::size_t index;

        std::size_t first_gid;
        std::shared_ptr<ch::tileset> tileset;

        map_tileset(const nlohmann::json &tileset_json, std::size_t index, ch::world &world);

        const ch::tile &get_tile(std::size_t gid) const;
    };

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
