#include "player.h"

#include <math.h>

void player_init(struct player *player)
{
    player->pos_x = 100.0f;
    player->pos_y = 100.0f;
    player->vel_x = 0.0f;
    player->vel_y = 0.0f;
    player->acc_x = 0.0f;
    player->acc_y = 0.0f;
}

void player_accelerate(float *pos_x, float *pos_y, float *vel_x, float *vel_y, float *acc_x, float *acc_y, float delta_time)
{
    float speed = 2000.0f;
    float drag = 10.0f;

    float acc_len = sqrt(*acc_x * *acc_x + *acc_y * *acc_y);
    if (acc_len > 1.0f)
    {
        *acc_x *= 1 / acc_len;
        *acc_y *= 1 / acc_len;
    }

    *acc_x *= speed;
    *acc_y *= speed;

    *acc_x -= *vel_x * drag;
    *acc_y -= *vel_y * drag;

    *pos_x = 0.5f * *acc_x * powf(delta_time, 2) + *vel_x * delta_time + *pos_x;
    *pos_y = 0.5f * *acc_y * powf(delta_time, 2) + *vel_y * delta_time + *pos_y;
    *vel_x = *acc_x * delta_time + *vel_x;
    *vel_y = *acc_y * delta_time + *vel_y;
}
