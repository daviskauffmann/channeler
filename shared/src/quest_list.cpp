#include <shared/quest_list.hpp>

#include <fstream>
#include <nlohmann/json.hpp>

hp::quest_list::quest_list(const std::string &filename)
{
    const auto quest_list_json = nlohmann::json::parse(std::ifstream(filename));

    for (const auto &quest_json : quest_list_json.at("quests"))
    {
        struct hp::quest quest;

        quest.name = quest_json.at("name");

        for (const auto &stage_json : quest_json.at("stages"))
        {
            struct hp::quest_stage stage;

            stage.description = stage_json.at("description");

            quest.stages.push_back(stage);
        }

        quests.push_back(quest);
    }
}
