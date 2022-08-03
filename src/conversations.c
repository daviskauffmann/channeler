#include "conversations.h"

#include "conversation_node.h"
#include "map.h"
#include "player.h"
#include "quest_status.h"
#include <json-c/json.h>
#include <stdio.h>
#include <string.h>

void conversations_load(struct conversations *conversations, const char *filename)
{
    printf("Loading conversations: %s\n", filename);

    conversations->filename = filename;

    struct json_object *root = json_object_from_file(filename);

    struct json_object *conversations_obj = json_object_object_get(root, "conversations");
    conversations->num_conversations = json_object_array_length(conversations_obj);
    conversations->conversations = malloc(conversations->num_conversations * sizeof(*conversations->conversations));
    for (size_t i = 0; i < conversations->num_conversations; i++)
    {
        size_t local_index = 0;
        conversation_node_load(
            &conversations->conversations[i],
            json_object_array_get_idx(conversations_obj, i),
            i,
            &local_index);
    }
}

void conversations_unload(struct conversations *conversations)
{
    printf("Unloading conversations: %s\n", conversations->filename);

    for (size_t i = 0; i < conversations->num_conversations; i++)
    {
        conversation_node_unload(&conversations->conversations[i]);
    }
    free(conversations->conversations);
}
