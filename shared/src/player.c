#include <shared/player.h>

#include <math.h>
#include <shared/world.h>
#include <stdbool.h>

#include <stdio.h>

void player_init(struct player *player, struct world *world)
{
    player->world = world;
    player->pos_x = 100.0f;
    player->pos_y = 100.0f;
    player->vel_x = 0.0f;
    player->vel_y = 0.0f;
    player->acc_x = 0.0f;
    player->acc_y = 0.0f;
}

void player_accelerate(struct player *player, float delta_time)
{
    float speed = 4000.0f;
    float drag = 20.0f;

    float acc_len = sqrtf(player->acc_x * player->acc_x + player->acc_y * player->acc_y);
    if (acc_len > 1.0f)
    {
        player->acc_x *= 1 / acc_len;
        player->acc_y *= 1 / acc_len;
    }

    player->acc_x *= speed;
    player->acc_y *= speed;

    player->acc_x -= player->vel_x * drag;
    player->acc_y -= player->vel_y * drag;

    float new_pos_x = 0.5f * player->acc_x * powf(delta_time, 2) + player->vel_x * delta_time + player->pos_x;
    float new_pos_y = 0.5f * player->acc_y * powf(delta_time, 2) + player->vel_y * delta_time + player->pos_y;

    int tile_nx_x = (int)roundf(new_pos_x / player->world->tile_width);
    int tile_nx_y = (int)roundf(player->pos_y / player->world->tile_height);
    if (tile_nx_x >= 0 && tile_nx_x < player->world->width && tile_nx_y >= 0 && tile_nx_y < player->world->height)
    {
        int tile_nx = player->world->tiles[tile_nx_x + tile_nx_y * player->world->width];
        struct tile_data *tile_data_nx = &player->world->tile_data[tile_nx];
        if (tile_data_nx->solid)
        {
            player->vel_x = 0;
        }
        else
        {
            player->pos_x = new_pos_x;
            player->vel_x = player->acc_x * delta_time + player->vel_x;
        }
    }
    else
    {
        player->vel_x = 0;
    }

    int tile_ny_x = (int)roundf(player->pos_x / player->world->tile_width);
    int tile_ny_y = (int)roundf(new_pos_y / player->world->tile_height);
    if (tile_ny_x >= 0 && tile_ny_x < player->world->width && tile_ny_y >= 0 && tile_ny_y < player->world->height)
    {
        int tile_ny = player->world->tiles[tile_ny_x + tile_ny_y * player->world->width];
        struct tile_data *tile_data_ny = &player->world->tile_data[tile_ny];
        if (tile_data_ny->solid)
        {
            player->vel_y = 0;
        }
        else
        {
            player->pos_y = new_pos_y;
            player->vel_y = player->acc_y * delta_time + player->vel_y;
        }
    }
    else
    {
        player->vel_y = 0;
    }
}

void player_attack(struct player *player)
{
    struct world *world = player->world;
    for (int i = 0; i < NUM_MOBS; i++)
    {
        struct mob *mob = &world->mobs[i];
        if (mob->alive)
        {
            float distance = sqrtf(powf(player->pos_x - mob->x, 2) + powf(player->pos_y - mob->y, 2));
            if (distance < 20.0f)
            {
                mob->alive = false;
            }
        }
    }
}
