#ifndef DIALOGS_H
#define DIALOGS_H

struct dialog_outcomes
{
    size_t set_dialog_index;
    struct
    {
        size_t quest_index;
        size_t stage_index;
    } set_quest_stage;
};

struct dialog_choice
{
    const char *text;

    struct dialog_outcomes outcomes;
};

struct dialog_message
{
    const char *text;

    size_t num_choices;
    struct dialog_choice *choices;
};

struct dialog
{
    size_t num_messages;
    struct dialog_message *messages;
};

struct dialogs
{
    size_t num_dialogs;
    struct dialog *dialogs;
};

void dialogs_load(struct dialogs *dialogs, const char *filename);
void dialogs_unload(struct dialogs *dialogs);

#endif
