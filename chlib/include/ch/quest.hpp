#ifndef CH_QUEST_HPP
#define CH_QUEST_HPP

#include <string>
#include <vector>

namespace ch
{
    struct quest_stage
    {
        std::string description;
    };

    struct quest
    {
        std::string name;
        std::vector<ch::quest_stage> stages;
    };
}

#endif
