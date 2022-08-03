#include <shared/conversation_node.h>

#include <json-c/json.h>
#include <shared/map.h>
#include <shared/player.h>
#include <shared/quest_status.h>
#include <stdio.h>
#include <string.h>

void conversation_node_load(struct conversation_node *node, struct json_object *conversation_obj, size_t index, size_t *local_index)
{
    node->index = index;
    node->local_index = *local_index;

    {
        struct json_object *type_obj = json_object_object_get(conversation_obj, "type");
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
        struct json_object *id_obj = json_object_object_get(conversation_obj, "id");
        node->id = json_object_get_string(id_obj);
    }

    {
        struct json_object *text_obj = json_object_object_get(conversation_obj, "text");
        node->text = json_object_get_string(text_obj);
    }

    {
        node->condition.quest_status = NULL;

        struct json_object *condition_obj = json_object_object_get(conversation_obj, "condition");
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

        struct json_object *effect_obj = json_object_object_get(conversation_obj, "effect");
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
        struct json_object *jump_id_obj = json_object_object_get(conversation_obj, "jump_id");
        node->jump_id = json_object_get_string(jump_id_obj);
    }

    {
        node->num_children = 0;
        node->children = NULL;

        struct json_object *children_obj = json_object_object_get(conversation_obj, "children");
        if (children_obj)
        {
            node->num_children = json_object_array_length(children_obj);
            node->children = malloc(node->num_children * sizeof(*node->children));
            for (size_t child_index = 0; child_index < node->num_children; child_index++)
            {
                (*local_index)++;
                conversation_node_load(
                    &node->children[child_index],
                    json_object_array_get_idx(children_obj, child_index),
                    index,
                    local_index);
            }
        }
    }
}

void conversation_node_unload(struct conversation_node *node)
{ 
    if (node->condition.quest_status)
    {
        free(node->condition.quest_status);
    }

    if (node->effect.quest_status)
    {
        free(node->effect.quest_status);
    }

    for (size_t i = 0; i < node->num_children; i++)
    {
        conversation_node_unload(&node->children[i]);
    }
    free(node->children);
}

struct conversation_node *conversation_node_find_by_local_index(struct conversation_node *node, size_t local_index)
{
    if (node->local_index == local_index)
    {
        return node;
    }

    for (size_t i = 0; i < node->num_children; i++)
    {
        struct conversation_node *result = conversation_node_find_by_local_index(&node->children[i], local_index);
        if (result)
        {
            return result;
        }
    }

    return NULL;
}

struct conversation_node *conversation_node_find_by_id(struct conversation_node *node, const char *id)
{
    if (node->id && strcmp(node->id, id) == 0)
    {
        return node;
    }

    for (size_t i = 0; i < node->num_children; i++)
    {
        struct conversation_node *child = &node->children[i];

        struct conversation_node *result = conversation_node_find_by_id(child, id);
        if (result)
        {
            return result;
        }
    }

    return NULL;
}

bool conversation_node_check_conditions(struct conversation_node *node, struct player *player)
{
    if (node->condition.quest_status)
    {
        if (!player_check_quest_status(player, *node->condition.quest_status))
        {
            return false;
        }
    }

    return true;
}
