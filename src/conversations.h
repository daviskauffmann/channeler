#ifndef CONVERSATIONS_H
#define CONVERSATIONS_H

#include <stdbool.h>
#include <stdlib.h>

struct conversations
{
    const char *filename;

    size_t num_conversations;
    struct conversation_node *conversations;
};

void conversations_load(struct conversations *conversations, const char *filename);
void conversations_unload(struct conversations *conversations);

#endif
