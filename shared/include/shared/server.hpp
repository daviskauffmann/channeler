#ifndef SERVER_HPP
#define SERVER_HPP

#include "client_list.hpp"
#include <enet/enet.h>
#include <thread>

namespace hp
{
    class world;

    class server
    {
    public:
        hp::client_list client_list;
        bool listening = true;

        server(std::uint16_t port);
        ~server();

        void update(float delta_time, hp::world &world);

    private:
        ENetHost *host;
        std::thread listen_thread;

        void listen();
    };
}

#endif
