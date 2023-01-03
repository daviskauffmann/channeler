#ifndef CH_LAYER_HPP
#define CH_LAYER_HPP

#include <nlohmann/json.hpp>
#include <vector>

namespace ch
{
    struct layer_tile
    {
        std::size_t gid;
        bool h_flip;
        bool v_flip;
        bool d_flip;

        inline double angle() const
        {
            if (d_flip)
            {
                if (h_flip)
                {
                    return 90.0;
                }
                else if (v_flip)
                {
                    return 270.0;
                }
            }
            else if (h_flip && v_flip)
            {
                return 180.0;
            }

            return 0.0;
        }
    };

    struct layer_object
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
        std::vector<ch::layer_tile> tiles;
        std::vector<ch::layer_object> objects;

        layer(const nlohmann::json &layer_json);

        const ch::layer_tile *get_tile(std::size_t x, std::size_t y) const;
    };
}

#endif
