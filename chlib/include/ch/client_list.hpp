#ifndef CH_CLIENT_LIST_HPP
#define CH_CLIENT_LIST_HPP

#include "player.hpp"
#include "input.hpp"
#include <array>

namespace ch
{
    struct client
    {
        std::size_t id;
        ch::player player;
        ch::input input;
    };

    class client_list
    {
    public:
        static constexpr std::size_t max_clients = 32;

        std::array<ch::client, max_clients> clients;

        client_list();

        std::size_t get_available_client();
    };
};

#endif
