#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include "client_list.hpp"
#include <cstddef>

namespace hp
{
    enum class message_type
    {
        SERVER_JOINED,
        SERVER_FULL,

        CLIENT_JOINED,
        CLIENT_DISCONNECTED
    };

    struct message
    {
        message_type type;
    };

    struct message_client_id : message
    {
        std::size_t client_id;
    };
}

#endif
