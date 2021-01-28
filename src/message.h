#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>

#include "world.h"

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
    MESSAGE_DISCONNECT_BROADCAST,
    MESSAGE_INPUT_REQUEST,
    MESSAGE_WORLD_STATE_BROADCAST
};

struct message
{
    enum message_type type;
};

struct id_message
{
    enum message_type type;
    int id;
};

struct connect_ok_message
{
    enum message_type type;
    int assigned_id;
    struct
    {
        int id;
        float x;
        float y;
    } clients[MAX_CLIENTS];
    struct world world;
};

struct connect_broadcast_message
{
    enum message_type type;
    int new_client_id;
    int new_client_x;
    int new_client_y;
};

struct input_request_message
{
    enum message_type type;
    int id;
    float dx;
    float dy;
};

struct world_state_broadcast_message
{
    enum message_type type;
    struct
    {
        int id;
        float x;
        float y;
    } clients[MAX_CLIENTS];
    struct world world;
};

#endif
