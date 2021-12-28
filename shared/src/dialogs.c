#include <shared/dialogs.h>

#include <json-c/json.h>
#include <stdio.h>
#include <string.h>

void dialogs_load(struct dialogs *dialogs, const char *filename)
{
    printf("Loading dialogs: %s\n", filename);

    dialogs->filename = filename;

    struct json_object *root = json_object_from_file(filename);

    struct json_object *dialogs_obj = json_object_object_get(root, "dialogs");
    dialogs->num_dialogs = json_object_array_length(dialogs_obj);
    dialogs->dialogs = malloc(dialogs->num_dialogs * sizeof(*dialogs->dialogs));
    for (size_t i = 0; i < dialogs->num_dialogs; i++)
    {
        struct dialog *dialog = &dialogs->dialogs[i];

        struct json_object *dialog_obj = json_object_array_get_idx(dialogs_obj, i);

        struct json_object *messages_obj = json_object_object_get(dialog_obj, "messages");
        dialog->num_messages = json_object_array_length(messages_obj);
        dialog->messages = malloc(dialog->num_messages * sizeof(*dialog->messages));
        for (size_t j = 0; j < dialog->num_messages; j++)
        {
            struct dialog_message *message = &dialog->messages[j];

            struct json_object *message_obj = json_object_array_get_idx(messages_obj, j);

            struct json_object *text_obj = json_object_object_get(message_obj, "text");
            message->text = json_object_get_string(text_obj);

            struct json_object *choices_obj = json_object_object_get(message_obj, "choices");
            if (choices_obj)
            {
                message->num_choices = json_object_array_length(choices_obj);
                message->choices = malloc(message->num_choices * sizeof(*message->choices));
                for (size_t k = 0; k < message->num_choices; k++)
                {
                    struct dialog_choice *choice = &message->choices[k];

                    struct json_object *choice_obj = json_object_array_get_idx(choices_obj, k);

                    struct json_object *text_obj = json_object_object_get(choice_obj, "text");
                    choice->text = json_object_get_string(text_obj);

                    struct json_object *outcomes_obj = json_object_object_get(choice_obj, "outcomes");
                    {
                        struct json_object *set_dialog_index_obj = json_object_object_get(outcomes_obj, "set_dialog_index");
                        if (set_dialog_index_obj)
                        {
                            choice->outcomes.set_dialog_index = json_object_get_int(set_dialog_index_obj);
                        }
                        else
                        {
                            choice->outcomes.set_dialog_index = -1;
                        }

                        struct json_object *set_quest_status_obj = json_object_object_get(outcomes_obj, "set_quest_status");
                        if (set_quest_status_obj)
                        {
                            struct json_object *quest_index_obj = json_object_object_get(set_quest_status_obj, "quest_index");
                            choice->outcomes.set_quest_status.quest_index = json_object_get_int(quest_index_obj);

                            struct json_object *stage_index_obj = json_object_object_get(set_quest_status_obj, "stage_index");
                            choice->outcomes.set_quest_status.stage_index = json_object_get_int(stage_index_obj);
                        }
                        else
                        {
                            choice->outcomes.set_quest_status.quest_index = -1;
                            choice->outcomes.set_quest_status.stage_index = -1;
                        }
                    }
                }
            }
            else
            {
                message->num_choices = 0;
                message->choices = NULL;
            }
        }
    }
}

void dialogs_unload(struct dialogs *dialogs)
{
    printf("Unloading dialogs: %s\n", dialogs->filename);

    for (size_t i = 0; i < dialogs->num_dialogs; i++)
    {
        struct dialog *dialog = &dialogs->dialogs[i];

        free(dialog->messages);
    }
    free(dialogs->dialogs);
}
