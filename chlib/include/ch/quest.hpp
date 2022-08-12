#ifndef CH_QUEST_HPP
#define CH_QUEST_HPP

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace ch
{
    struct quest_stage
    {
        std::string description;

        quest_stage(const nlohmann::json &stage_json);
    };

    struct quest
    {
        std::string name;
        std::vector<ch::quest_stage> stages;

        quest(const nlohmann::json &quest_json);
    };
}

#endif
