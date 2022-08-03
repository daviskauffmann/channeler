#include "quests.h"

#include <json-c/json.h>
#include <stdio.h>
#include <string.h>

void quests_load(struct quests *quests, const char *filename)
{
    printf("Loading quests: %s\n", filename);

    quests->filename = filename;

    struct json_object *root = json_object_from_file(filename);

    struct json_object *quests_obj = json_object_object_get(root, "quests");
    quests->num_quests = json_object_array_length(quests_obj);
    quests->quests = malloc(quests->num_quests * sizeof(*quests->quests));
    for (size_t i = 0; i < quests->num_quests; i++)
    {
        struct quest *quest = &quests->quests[i];

        struct json_object *quest_obj = json_object_array_get_idx(quests_obj, i);

        struct json_object *name_obj = json_object_object_get(quest_obj, "name");
        quest->name = json_object_get_string(name_obj);

        struct json_object *stages_obj = json_object_object_get(quest_obj, "stages");
        quest->num_stages = json_object_array_length(stages_obj);
        quest->stages = malloc(quest->num_stages * sizeof(*quest->stages));
        for (size_t j = 0; j < quest->num_stages; j++)
        {
            struct quest_stage *stage = &quest->stages[j];

            struct json_object *stage_obj = json_object_array_get_idx(stages_obj, j);

            struct json_object *description_obj = json_object_object_get(stage_obj, "description");
            stage->description = json_object_get_string(description_obj);
        }
    }
}

void quests_unload(struct quests *quests)
{
    printf("Unloading quests: %s\n", quests->filename);

    for (size_t i = 0; i < quests->num_quests; i++)
    {
        struct quest *quest = &quests->quests[i];

        free(quest->stages);
    }
    free(quests->quests);
}
