#include <ch/quest.hpp>

ch::quest::quest(const nlohmann::json &quest_json)
{
    name = quest_json.at("name");

    for (const auto &stage_json : quest_json.at("stages"))
    {
        ch::quest_stage stage;
        stage.description = stage_json.at("description");

        stages.push_back(stage);
    }
}
