#ifndef WORLD_HPP
#define WORLD_HPP

#include "map.hpp"
#include "quest.hpp"
#include <unordered_map>
#include <vector>

namespace hp
{
    class conversation;
    class tileset;

    class world
    {
    public:
        std::vector<hp::map> maps;
        std::vector<hp::quest> quests;
        std::vector<hp::conversation *> conversations;

        world(
            const std::string &world_filename,
            const std::string &quests_filename,
            const std::string &conversations_filename);
        ~world();

        tileset *load_tileset(const std::string &filename);

        std::size_t get_map_index(const std::string &filename) const;

    private:
        std::unordered_map<std::string, hp::tileset *> tilesets;
    };
}

#endif
