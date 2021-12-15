#ifndef WORLD_H
#define WORLD_H

#include <stdbool.h>

#define NUM_MOBS 20

struct mob
{
    float x;
    float y;
    int alive;
};

struct tile_data
{
    bool solid;
};

struct world
{
    int width;
    int height;
    int tile_width;
    int tile_height;
    int *tiles;

    struct tile_data *tile_data;
    const char *spritesheet_filename;

    struct mob mobs[NUM_MOBS];
};

void world_load(struct world *world, const char *tiles_filename, const char *sprites_filename);
void world_update(struct world *world, float delta_time);

#endif
