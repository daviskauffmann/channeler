#ifndef QUESTS_HPP
#define QUESTS_HPP

#include <string>
#include <vector>

namespace hp
{
    struct quest_stage
    {
        std::string description;
    };

    struct quest
    {
        std::string name;
        std::vector<hp::quest_stage> stages;
    };

    class quests
    {
    public:
        std::vector<hp::quest> _quests;

        quests(const std::string &filename);
    };

}

#endif
