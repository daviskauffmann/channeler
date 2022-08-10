#ifndef PEER_HPP
#define PEER_HPP

#include <cstddef>
#include <cstdint>

struct _ENetPeer;
typedef _ENetPeer ENetPeer;

struct _ENetHost;
typedef _ENetHost ENetHost;

struct _ENetAddress;
typedef _ENetAddress ENetAddress;

namespace ch
{
    class peer
    {
    public:
        ENetPeer *enet_peer;
        bool disconnected = false;

        peer(
            ENetHost *host,
            const ENetAddress *address,
            std::size_t channel_count,
            std::uint32_t data);
        ~peer();
    };
}

#endif
