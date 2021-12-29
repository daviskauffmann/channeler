#ifndef CONVERSATIONS_H
#define CONVERSATIONS_H

#include <stdlib.h>

enum conversation_node_type
{
    CONVERSATION_NODE_ROOT,
    CONVERSATION_NODE_DIALOG,
    CONVERSATION_NODE_RESPONSE,
};

struct conversation_node_condition
{
    char _;
};

struct conversation_node_effect
{
    struct quest_status *set_quest_status;
};

struct conversation_node
{
    enum conversation_node_type type;
    struct conversation_node_condition condition;
    struct conversation_node_effect effect;

    const char *text;

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

#endif
