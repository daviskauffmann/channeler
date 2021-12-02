#ifndef PLAYER_H
#define PLAYER_H

struct player
{
    float pos_x;
    float pos_y;
    float vel_x;
    float vel_y;
    float acc_x;
    float acc_y;
};

void player_init(struct player *player);
void player_accelerate(float *pos_x, float *pos_y, float *vel_x, float *vel_y, float *acc_x, float *acc_y, float delta_time);

#endif
