#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>

#include "input.h"
#include "player.h"
#include "map.h"

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
    MESSAGE_ATTACK_REQUEST,
    MESSAGE_CHANGE_MAP_REQUEST,
    MESSAGE_GAME_STATE_BROADCAST
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

struct message_connect
{
    enum message_type type;
    int id;
    int map_index;
};

struct message_input
{
    enum message_type type;
    int id;
    struct input input;
};

struct message_change_map
{
    enum message_type type;
    int map_index;
};

struct message_game_state
{
    enum message_type type;
    struct
    {
        int id;
        struct
        {
            int map_index;
            float pos_x;
            float pos_y;
            float vel_x;
            float vel_y;
            float acc_x;
            float acc_y;
        } player;
    } clients[MAX_CLIENTS];
    struct
    {
        float x;
        float y;
        int alive;
    } mobs[MAX_MOBS];
};

#endif
