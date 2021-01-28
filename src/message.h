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

struct message_id
{
    enum message_type type;
    int id;
};

struct message_input
{
    enum message_type type;
    int id;
    float acc_x;
    float acc_y;
};

struct message_world_state
{
    enum message_type type;
    struct
    {
        int id;
        float pos_x;
        float pos_y;
        float vel_x;
        float vel_y;
        float acc_x;
        float acc_y;
    } clients[MAX_CLIENTS];
    struct world world;
};

#endif
