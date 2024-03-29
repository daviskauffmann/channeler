#include <ch/host.hpp>

#include <enet/enet.h>
#include <stdexcept>

ch::host::host(
    const ENetAddress *const address,
    const std::size_t peer_count,
    const std::size_t channel_limit,
    const std::uint32_t incoming_bandwidth,
    const std::uint32_t outgoing_bandwidth)
{
    enet_host = enet_host_create(address, peer_count, channel_limit, incoming_bandwidth, outgoing_bandwidth);
    if (!enet_host)
    {
        throw std::runtime_error("Failed to create ENet host");
    }
}

ch::host::~host()
{
    enet_host_destroy(enet_host);
}

ENetHost *ch::host::get_enet_host() const
{
    return enet_host;
}

void ch::host::broadcast(ENetPacket *const packet) const
{
    enet_host_broadcast(enet_host, 0, packet);
}

int ch::host::service(ENetEvent *const event, const std::uint32_t timeout) const
{
    return enet_host_service(enet_host, event, timeout);
}
