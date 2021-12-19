#include <shared/player.h>

#include <math.h>
#include <shared/map.h>
#include <shared/tileset.h>
#include <stdbool.h>
#include <string.h>

void player_init(struct player *player, int map_index)
{
    player->map_index = map_index;
    player->pos_x = 100;
    player->pos_y = 100;
    player->vel_x = 0;
    player->vel_y = 0;
    player->acc_x = 0;
    player->acc_y = 0;
}

void player_accelerate(struct player *player, struct map *map, float delta_time)
{
    float speed = 4000;
    float drag = 20;

    float acc_len = sqrtf(player->acc_x * player->acc_x + player->acc_y * player->acc_y);
    if (acc_len > 1)
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

    int tile_nx_x = (int)roundf(new_pos_x / map->tile_width);
    int tile_nx_y = (int)roundf(player->pos_y / map->tile_height);
    struct tile *tile_nx = map_get_tile(map, tile_nx_x, tile_nx_y);
    if (tile_nx)
    {
        struct tileset *tileset = map_get_tileset(map, tile_nx->gid);
        struct tile_data *tile_data = tileset_get_tile_data(tileset, tile_nx->gid);
        if (tile_data->solid)
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

    int tile_ny_x = (int)roundf(player->pos_x / map->tile_width);
    int tile_ny_y = (int)roundf(new_pos_y / map->tile_height);
    struct tile *tile_ny = map_get_tile(map, tile_ny_x, tile_ny_y);
    if (tile_ny)
    {
        struct tileset *tileset = map_get_tileset(map, tile_ny->gid);
        struct tile_data *tile_data = tileset_get_tile_data(tileset, tile_ny->gid);
        if (tile_data->solid)
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

void player_attack(struct player *player, struct map *map)
{
    for (int i = 0; i < MAX_MOBS; i++)
    {
        struct mob *mob = &map->mobs[i];
        if (mob->alive)
        {
            float distance = sqrtf(powf(player->pos_x - mob->x, 2) + powf(player->pos_y - mob->y, 2));
            if (distance < 20)
            {
                mob->alive = false;
                mob->respawn_timer = 5;
            }
        }
    }
}
