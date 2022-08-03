#ifndef CONVERSATION_NODE_H
#define CONVERSATION_NODE_H

#include <stdbool.h>
#include <stdlib.h>

enum conversation_node_type
{
    CONVERSATION_NODE_ROOT,
    CONVERSATION_NODE_DIALOG,
    CONVERSATION_NODE_RESPONSE,
};

struct conversation_node_condition
{
    struct quest_status *quest_status;
};

struct conversation_node_effect
{
    struct quest_status *quest_status;
};

struct conversation_node
{
    size_t index;
    size_t local_index;

    enum conversation_node_type type;
    const char *id;
    struct conversation_node_condition condition;
    struct conversation_node_effect effect;

    const char *text;

    const char *jump_id;

    size_t num_children;
    struct conversation_node *children;
};

void conversation_node_load(struct conversation_node *node, struct json_object *conversation_obj, size_t index, size_t *local_index);
void conversation_node_unload(struct conversation_node *node);

struct conversation_node *conversation_node_find_by_local_index(struct conversation_node *node, size_t local_index);
struct conversation_node *conversation_node_find_by_id(struct conversation_node *node, const char *id);

bool conversation_node_has_response_nodes(struct conversation_node *node);
bool conversation_node_check_conditions(struct conversation_node *node, struct player *player);

#endif
