#ifndef CH_WORLD_HPP
#define CH_WORLD_HPP

#include "conversation.hpp"
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
        std::vector<std::unique_ptr<ch::conversation>> conversations;

        world(
            const std::string &world_filename,
            const std::string &quests_filename,
            const std::string &conversations_filename);

        const ch::tileset *load_tileset(const std::string &filename);

        std::size_t get_map_index(const std::string &filename) const;

    private:
        std::unordered_map<std::string, std::unique_ptr<ch::tileset>> tilesets;
    };
}

#endif
