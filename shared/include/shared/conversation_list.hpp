#ifndef CONVERSATIONS_HPP
#define CONVERSATIONS_HPP

#include <string>
#include <vector>

namespace hp
{
    class conversation;

    class conversation_list
    {
    public:
        std::vector<hp::conversation *> conversations;

        conversation_list(const std::string &filename);
        ~conversation_list();
    };
}

#endif
