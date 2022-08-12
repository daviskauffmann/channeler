#include <ch/quest.hpp>

ch::quest_stage::quest_stage(const nlohmann::json &stage_json)
{
    description = stage_json.at("description");
}

ch::quest::quest(const nlohmann::json &quest_json)
{
    name = quest_json.at("name");

    for (const auto &stage_json : quest_json.at("stages"))
    {
        stages.push_back({stage_json});
    }
}

