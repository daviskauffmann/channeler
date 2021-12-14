#ifndef PLAYER_H
#define PLAYER_H

struct player
{
    struct world *world;
    float pos_x;
    float pos_y;
    float vel_x;
    float vel_y;
    float acc_x;
    float acc_y;
};

void player_init(struct player *player, struct world *world);
void player_accelerate(float *pos_x, float *pos_y, float *vel_x, float *vel_y, float *acc_x, float *acc_y, float delta_time);
void player_attack(struct player *player);

#endif
