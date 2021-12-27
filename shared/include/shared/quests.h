#ifndef QUESTS_H
#define QUESTS_H

struct quest_stage
{
    const char *description;
};

struct quest
{
    const char *name;

    size_t num_stages;
    struct quest_stage *stages;
};

struct quests
{
    size_t num_quests;
    struct quest *quests;
};

void quests_load(struct quests *quests, const char *filename);
void quests_unload(struct quests *quests);

#endif
