#ifndef CH_HOST_HPP
#define CH_HOST_HPP

#include <cstddef>
#include <cstdint>

struct _ENetHost;
typedef _ENetHost ENetHost;

struct _ENetAddress;
typedef _ENetAddress ENetAddress;

namespace ch
{
    class host
    {
    public:
        ENetHost *enet_host;

        host(
            const ENetAddress *address,
            std::size_t peer_count,
            std::size_t channel_limit,
            std::uint32_t incoming_bandwidth,
            std::uint32_t outgoing_bandwidth);
        ~host();
    };
}

#endif
