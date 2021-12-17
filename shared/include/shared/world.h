#ifndef WORLD_H
#define WORLD_H

#include <stdbool.h>

#define MAX_MOBS 20

struct tile_data
{
    bool solid;
};

struct tileset
{
    int index;
    int columns;
    int first_gid;
    const char *image;
    struct tile_data *tile_data;
};

void tileset_load(struct tileset *tileset, const char *filename);
struct tile_data *tileset_get_tile_data(struct tileset *tileset, int gid);

struct tile
{
    int gid;
    bool h_flip;
    bool v_flip;
    bool d_flip;
};

struct mob
{
    int gid;
    float x;
    float y;
    int alive;
};

struct world
{
    int width;
    int height;
    int tile_width;
    int tile_height;

    size_t tileset_count;
    struct tileset *tilesets;

    // TODO: layer support
    struct tile *tiles;

    struct mob mobs[MAX_MOBS];
};

void world_load(struct world *world, const char *filename);
void world_update(struct world *world, float delta_time);
struct tile *world_get_tile(struct world *world, int x, int y);
struct tileset *world_get_tileset(struct world *world, int gid);

#endif
