#include <shared/conversations.h>

#include <json-c/json.h>
#include <shared/map.h>
#include <shared/player.h>
#include <stdio.h>
#include <string.h>

void load_conversation_node(struct conversation_node *node, struct json_object *node_obj)
{
    {
        struct json_object *type_obj = json_object_object_get(node_obj, "type");
        const char *type = json_object_get_string(type_obj);
        if (strcmp(type, "root") == 0)
        {
            node->type = CONVERSATION_NODE_ROOT;
        }
        else if (strcmp(type, "dialog") == 0)
        {
            node->type = CONVERSATION_NODE_DIALOG;
        }
        else if (strcmp(type, "response") == 0)
        {
            node->type = CONVERSATION_NODE_RESPONSE;
        }
    }

    {
        struct json_object *id_obj = json_object_object_get(node_obj, "id");
        node->id = json_object_get_string(id_obj);
    }

    {
        struct json_object *text_obj = json_object_object_get(node_obj, "text");
        node->text = json_object_get_string(text_obj);
    }

    {
        node->condition.quest_status = NULL;

        struct json_object *condition_obj = json_object_object_get(node_obj, "condition");
        if (condition_obj)
        {
            struct json_object *quest_status_obj = json_object_object_get(condition_obj, "quest_status");
            if (quest_status_obj)
            {
                node->condition.quest_status = malloc(sizeof(*node->condition.quest_status));

                struct json_object *quest_index_obj = json_object_object_get(quest_status_obj, "quest_index");
                node->condition.quest_status->quest_index = json_object_get_int(quest_index_obj);

                struct json_object *stage_index_obj = json_object_object_get(quest_status_obj, "stage_index");
                node->condition.quest_status->stage_index = json_object_get_int(stage_index_obj);
            }
        }
    }

    {
        node->effect.quest_status = NULL;

        struct json_object *effect_obj = json_object_object_get(node_obj, "effect");
        if (effect_obj)
        {
            struct json_object *quest_status_obj = json_object_object_get(effect_obj, "quest_status");
            if (quest_status_obj)
            {
                node->effect.quest_status = malloc(sizeof(*node->effect.quest_status));

                struct json_object *quest_index_obj = json_object_object_get(quest_status_obj, "quest_index");
                node->effect.quest_status->quest_index = json_object_get_int(quest_index_obj);

                struct json_object *stage_index_obj = json_object_object_get(quest_status_obj, "stage_index");
                node->effect.quest_status->stage_index = json_object_get_int(stage_index_obj);
            }
        }
    }

    {
        struct json_object *jump_id_obj = json_object_object_get(node_obj, "jump_id");
        node->jump_id = json_object_get_string(jump_id_obj);
    }

    {
        node->num_children = 0;
        node->children = NULL;

        struct json_object *children_obj = json_object_object_get(node_obj, "children");
        if (children_obj)
        {
            node->num_children = json_object_array_length(children_obj);
            node->children = malloc(node->num_children * sizeof(*node->children));
            for (size_t i = 0; i < node->num_children; i++)
            {
                struct conversation_node *child = &node->children[i];
                struct json_object *child_obj = json_object_array_get_idx(children_obj, i);
                load_conversation_node(child, child_obj);
            }
        }
    }
}

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
        struct conversation_node *conversation = &conversations->conversations[i];
        struct json_object *conversation_obj = json_object_array_get_idx(conversations_obj, i);
        load_conversation_node(conversation, conversation_obj);
    }
}

void conversations_unload(struct conversations *conversations)
{
    printf("Unloading conversations: %s\n", conversations->filename);
}

struct conversation_node *conversation_find_by_id(struct conversation_node *node, const char *id)
{
    if (node->id && strcmp(node->id, id) == 0)
    {
        return node;
    }

    for (size_t i = 0; i < node->num_children; i++)
    {
        struct conversation_node *child = &node->children[i];

        struct conversation_node *result = conversation_find_by_id(child, id);
        if (result)
        {
            return result;
        }
    }

    return NULL;
}

bool conversation_check_conditions(struct conversation_node *node)
{
    if (node->condition.quest_status)
    {
    }

    return true;
}
