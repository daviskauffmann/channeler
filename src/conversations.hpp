#ifndef CONVERSATIONS_HPP
#define CONVERSATIONS_HPP

#include <string>
#include <vector>

namespace hp
{
    class conversation_node;

    class conversations
    {
    public:
        std::vector<hp::conversation_node *> conversation_roots;

        conversations(const std::string &filename);
        ~conversations();
    };
}

#endif
