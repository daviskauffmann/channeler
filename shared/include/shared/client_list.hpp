#ifndef CLIENT_LIST_HPP
#define CLIENT_LIST_HPP

#include "player.hpp"
#include <array>

namespace hp
{
    struct client
    {
        std::size_t id;
        hp::player player;
    };

    class client_list
    {
    public:
        static constexpr std::size_t max_clients = 32;

        std::array<hp::client, max_clients> clients;

        client_list();

        std::size_t get_available_client();
    };
};

#endif
