#ifndef QUEST_LIST_HPP
#define QUEST_LIST_HPP

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

    class quest_list
    {
    public:
        std::vector<hp::quest> quests;

        quest_list(const std::string &filename);
    };

}

#endif
