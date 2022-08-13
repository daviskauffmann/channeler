#ifndef CH_CLIENT_HPP
#define CH_CLIENT_HPP

#include <SDL2/SDL.h>
#include <ch/server.hpp>
#include <memory>

struct _ENetPacket;
typedef _ENetPacket ENetPacket;

namespace ch
{
    class host;
    class peer;
    struct player;

    class client
    {
    public:
        std::array<ch::connection, ch::server::max_connections> connections;

        client(const char *hostname, std::uint16_t port, ch::world &world);
        ~client();

        void handle_event(const SDL_Event &event);
        void update(float delta_time);

        void send(ENetPacket *packet) const;

        const ch::player &get_player() const;

    private:
        ch::world &world;
        std::unique_ptr<ch::host> host;
        std::unique_ptr<ch::peer> peer;
        std::size_t connection_id;
    };
}

#endif
