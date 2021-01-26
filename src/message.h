#ifndef MESSAGE_H
#define MESSAGE_H

#define PACKET_SIZE 1024
#define MAX_STRLEN 256
#define MAX_CLIENTS 8

enum message_type
{
    MESSAGE_CONNECT_OK,
    MESSAGE_CONNECT_FULL,
    MESSAGE_CONNECT_BROADCAST,
    MESSAGE_UDP_CONNECT_REQUEST,
    MESSAGE_DISCONNECT_REQUEST,
    MESSAGE_DISCONNECT_BROADCAST
};

struct message
{
    enum message_type type;
};

struct id_message
{
    struct message message;
    int id;
};

struct connect_ok_message
{
    struct message message;
    int id;
    // TODO: list of players
};

struct connect_broadcast_message
{
    struct message message;
    int id;
    // TODO: player info
};

#endif
