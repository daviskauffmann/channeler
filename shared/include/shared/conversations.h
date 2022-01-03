#ifndef CONVERSATIONS_H
#define CONVERSATIONS_H

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
    enum conversation_node_type type;
    const char *id;
    struct conversation_node_condition condition;
    struct conversation_node_effect effect;

    const char *text;

    const char *jump_id;

    size_t num_children;
    struct conversation_node *children;
};

struct conversations
{
    const char *filename;

    size_t num_conversations;
    struct conversation_node *conversations;
};

void conversations_load(struct conversations *conversations, const char *filename);
void conversations_unload(struct conversations *conversations);

struct conversation_node *conversation_find_by_id(struct conversation_node *node, const char *id);
bool conversation_check_conditions(struct conversation_node *node, struct player *player);

#endif
