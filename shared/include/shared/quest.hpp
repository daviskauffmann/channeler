#ifndef QUEST_HPP
#define QUEST_HPP

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
}

#endif
