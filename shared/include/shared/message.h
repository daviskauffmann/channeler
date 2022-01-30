#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>

#include "input.h"
#include "map.h"
#include "quest_status.h"

#define PACKET_SIZE 1024
#define MAX_STRLEN 256
#define MAX_CLIENTS 8
#define CLIENT_ID_UNUSED MAX_CLIENTS

enum message_type
{
    MESSAGE_SERVER_CONNECT_OK,
    MESSAGE_SERVER_FULL,
    MESSAGE_CLIENT_CONNECT,
    MESSAGE_CLIENT_DISCONNECT,
    MESSAGE_UDP_CONNECT,
    MESSAGE_DISCONNECT,
    MESSAGE_INPUT,
    MESSAGE_ATTACK,
    MESSAGE_CHANGE_MAP,
    MESSAGE_START_CONVERSATION,
    MESSAGE_ADVANCE_CONVERSATION,
    MESSAGE_CHOOSE_CONVERSATION_RESPONSE,
    MESSAGE_QUEST_STATUS,
    MESSAGE_GAME_STATE
};

struct message
{
    enum message_type type;
};

struct message_id
{
    enum message_type type;
    size_t id;
};

struct message_connect
{
    enum message_type type;
    size_t id;
    size_t map_index;
};

struct message_input
{
    enum message_type type;
    size_t id;
    struct input input;
};

struct message_change_map
{
    enum message_type type;
    size_t map_index;
};

struct message_start_conversation
{
    enum message_type type;
    size_t conversation_index;
};

struct message_choose_conversation_response
{
    enum message_type type;
    size_t choice_index;
};

struct message_quest_status
{
    enum message_type type;
    struct quest_status quest_status;
};

struct message_game_state
{
    enum message_type type;
    struct
    {
        size_t id;
        struct
        {
            size_t map_index;
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
