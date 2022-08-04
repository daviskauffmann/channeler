#include <shared/quests.hpp>

#include <fstream>
#include <nlohmann/json.hpp>

hp::quests::quests(const std::string &filename)
{
    auto quests_json = nlohmann::json::parse(std::ifstream(filename));

    for (const auto &quest_json : quests_json.at("quests"))
    {
        struct hp::quest quest;

        quest.name = quest_json.at("name");

        for (const auto &stage_json : quest_json.at("stages"))
        {
            struct hp::quest_stage stage;

            stage.description = stage_json.at("description");

            quest.stages.push_back(stage);
        }

        _quests.push_back(quest);
    }
}
