#include "peer.hpp"

#include <enet/enet.h>
#include <stdexcept>

ch::peer::peer(
    ENetHost *const host,
    const ENetAddress *const address,
    const std::size_t channel_count,
    const std::uint32_t data)
{
    enet_peer = enet_host_connect(host, address, channel_count, data);
    if (!enet_peer)
    {
        throw std::runtime_error("Failed to create ENet peer");
    }
}

ch::peer::~peer()
{
    if (!successfully_disconnected)
    {
        enet_peer_reset(enet_peer);
    }
}

int ch::peer::send(ENetPacket *const packet) const
{
    return enet_peer_send(enet_peer, 0, packet);
}

void ch::peer::disconnect() const
{
    enet_peer_disconnect(enet_peer, 0);
}

bool ch::peer::is_successfully_disconnected() const
{
    return successfully_disconnected;
}

void ch::peer::set_successfully_disconnected()
{
    successfully_disconnected = true;
}
