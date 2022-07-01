#include <shared/net.h>

#include <assert.h>
#include <shared/message.h>

bool tcp_send(TCPsocket sock, const void *data, size_t len)
{
    assert(len <= PACKET_SIZE);

    if (SDLNet_TCP_Send(sock, data, PACKET_SIZE) < PACKET_SIZE)
    {
        printf("Error: Failed to send TCP packet: %s\n", SDLNet_GetError());

        return false;
    }

    return true;
}

bool udp_send(UDPsocket sock, IPaddress address, const void *data, size_t len)
{
    assert(len <= PACKET_SIZE);

    bool result = true;

    UDPpacket *packet = SDLNet_AllocPacket(PACKET_SIZE);
    packet->address = address;
    packet->data = (uint8_t *)data;
    packet->len = (int)len;

    if (!SDLNet_UDP_Send(sock, -1, packet))
    {
        printf("Error: Failed to send UDP packet\n");
        result = false;
    }

    SDLNet_FreePacket(packet);

    return result;
}
