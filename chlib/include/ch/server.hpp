#ifndef CH_SERVER_HPP
#define CH_SERVER_HPP

#include "client_list.hpp"
#include <enet/enet.h>
#include <thread>

namespace ch
{
    class world;

    class server
    {
    public:
        ch::client_list client_list;
        bool listening = true;

        server(std::uint16_t port);
        ~server();

        void update(float delta_time, ch::world &world);

    private:
        ENetHost *host;
        std::thread listen_thread;

        void listen();
    };
}

#endif
