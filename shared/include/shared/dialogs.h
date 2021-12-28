#ifndef DIALOGS_H
#define DIALOGS_H

#include "player.h"

struct dialog_outcomes
{
    size_t set_dialog_index;
    size_t set_dialog_message_index;
    struct quest_status set_quest_status;
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
    const char *filename;

    size_t num_dialogs;
    struct dialog *dialogs;
};

void dialogs_load(struct dialogs *dialogs, const char *filename);
void dialogs_unload(struct dialogs *dialogs);

#endif
