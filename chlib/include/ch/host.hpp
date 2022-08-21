#ifndef CH_HOST_HPP
#define CH_HOST_HPP

#include <cstddef>
#include <cstdint>

struct _ENetAddress;
typedef _ENetAddress ENetAddress;

struct _ENetPacket;
typedef _ENetPacket ENetPacket;

struct _ENetHost;
typedef _ENetHost ENetHost;

struct _ENetEvent;
typedef _ENetEvent ENetEvent;

namespace ch
{
    class host
    {
    public:
        host(
            const ENetAddress *address,
            std::size_t peer_count,
            std::size_t channel_limit,
            std::uint32_t incoming_bandwidth,
            std::uint32_t outgoing_bandwidth);
        ~host();
        host(const host &other) = delete;
        host &operator=(const host &other) = delete;
        host(host &&other) = delete;
        host &operator=(host &&other) = delete;

        ENetHost *get_enet_host() const;

        void broadcast(ENetPacket *packet) const;

        int service(ENetEvent *event, std::uint32_t timeout) const;

    private:
        ENetHost *enet_host;
    };
}

#endif
