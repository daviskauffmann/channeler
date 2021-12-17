#ifndef MAP_H
#define MAP_H

#include <stdbool.h>

#define MAX_MOBS 20

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

    float origin_x;
    float origin_y;

    float x;
    float y;

    int alive;
    float respawn_timer;
};

struct map
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

void map_load(struct map *map, const char *filename);
void map_update(struct map *map, float delta_time);
struct tile *map_get_tile(struct map *map, int x, int y);
struct tileset *map_get_tileset(struct map *map, int gid);

#endif
