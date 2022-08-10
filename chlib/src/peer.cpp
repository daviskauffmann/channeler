#include <ch/peer.hpp>

#include <enet/enet.h>
#include <exception>

ch::peer::peer(
    ENetHost *const host,
    const ENetAddress *const address,
    const std::size_t channel_count,
    const std::uint32_t data)
{
    enet_peer = enet_host_connect(host, address, channel_count, data);
    if (!enet_peer)
    {
        throw std::exception("Failed to create ENet peer");
    }
}

ch::peer::~peer()
{
    if (!disconnected)
    {
        enet_peer_reset(enet_peer);
    }
}
