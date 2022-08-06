#ifndef CH_WORLD_HPP
#define CH_WORLD_HPP

#include "map.hpp"
#include "quest.hpp"
#include <unordered_map>
#include <vector>

namespace ch
{
    class conversation;
    class tileset;

    class world
    {
    public:
        std::vector<ch::map> maps;
        std::vector<ch::quest> quests;
        std::vector<ch::conversation *> conversations;

        world(
            const std::string &world_filename,
            const std::string &quests_filename,
            const std::string &conversations_filename);
        ~world();

        tileset *load_tileset(const std::string &filename);

        std::size_t get_map_index(const std::string &filename) const;

    private:
        std::unordered_map<std::string, ch::tileset *> tilesets;
    };
}

#endif
