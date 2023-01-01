#ifndef CH_WORLD_HPP
#define CH_WORLD_HPP

#include "conversation.hpp"
#include "item.hpp"
#include "map.hpp"
#include "quest.hpp"
#include "tileset.hpp"
#include <memory>
#include <unordered_map>
#include <vector>

namespace ch
{
    class world
    {
    public:
        std::vector<ch::map> maps;
        std::vector<ch::quest> quests;
        std::vector<ch::conversation> conversations;
        std::vector<ch::item> items;

        world(
            const std::string &world_filename,
            const std::string &quests_filename,
            const std::string &conversations_filename,
            const std::string &items_filename);

        std::shared_ptr<ch::tileset> load_tileset(const std::string &filename);

        std::size_t get_map_index(const std::string &filename) const;

        void update(float delta_time);

    private:
        std::unordered_map<std::string, std::shared_ptr<ch::tileset>> tilesets;
    };
}

#endif
