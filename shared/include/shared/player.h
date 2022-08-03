#ifndef PLAYER_H
#define PLAYER_H

#include <SDL2/SDL_net.h>
#include <stdbool.h>

enum direction
{
    DIRECTION_DOWN,
    DIRECTION_UP,
    DIRECTION_LEFT,
    DIRECTION_RIGHT
};

enum animation
{
    ANIMATION_IDLE,
    ANIMATION_WALKING,
};

struct player
{
    size_t client_id;
    TCPsocket socket;

    size_t map_index;

    float pos_x;
    float pos_y;
    float vel_x;
    float vel_y;
    float acc_x;
    float acc_y;

    enum direction direction;
    enum animation animation;
    float animation_timer;
    size_t frame_index;

    struct conversation_node *conversation_root;
    struct conversation_node *conversation_node;

    size_t num_quest_statuses;
    struct quest_status *quest_statuses;
};

void player_init(struct player *player, size_t client_id, TCPsocket socket, size_t map_index);
void player_uninit(struct player *player);
void player_update(struct player *player, struct input *input, struct map *map, float delta_time);
void player_attack(struct player *player, struct map *map);
void player_start_conversation(struct player *player, struct conversations *conversations, size_t conversation_index);
void player_advance_conversation(struct player *player);
void player_choose_conversation_response(struct player *player, size_t choice_index);
void player_end_conversation(struct player *player);
void player_set_quest_status(struct player *player, struct quest_status quest_status);
bool player_check_quest_status(struct player *player, struct quest_status quest_status);

#endif
