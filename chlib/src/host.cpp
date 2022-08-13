#include <ch/host.hpp>

#include <enet/enet.h>
#include <exception>

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
        throw std::exception("Failed to create ENet host");
    }
}

ch::host::~host()
{
    enet_host_destroy(enet_host);
}
