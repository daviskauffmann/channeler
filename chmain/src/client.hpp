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

    class client
    {
    public:
        std::array<ch::player, ch::server::max_players> players;

        client(
            const char *hostname,
            std::uint16_t port,
            std::shared_ptr<ch::world> world);
        ~client();
        client(const client &other) = delete;
        client &operator=(const client &other) = delete;
        client(client &&other) = delete;
        client &operator=(client &&other) = delete;

        void handle_event(const SDL_Event &event);
        void update(float delta_time);

        void send(const void *data, std::size_t length, std::uint32_t flags) const;

        const ch::player &get_player() const;

    private:
        std::shared_ptr<ch::world> world;
        std::unique_ptr<ch::host> host;
        std::unique_ptr<ch::peer> peer;
        std::size_t player_id;
    };
}

#endif
