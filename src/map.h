#ifndef MAP_H
#define MAP_H

#include <stdbool.h>

#define MAX_MOBS 20

struct mob
{
    size_t gid;

    float origin_x;
    float origin_y;

    float x;
    float y;

    bool alive;
    float respawn_timer;
};

struct map
{
    char *filename;

    size_t width;
    size_t height;

    size_t num_layers;
    struct layer *layers;

    // TODO: object layers
    struct mob mobs[MAX_MOBS];

    size_t tile_width;
    size_t tile_height;

    size_t num_tilesets;
    struct tileset *tilesets;
};

void map_load(struct map *map, char *filename);
void map_unload(struct map *map);

void map_update(struct map *map, float delta_time);

struct tileset *map_get_tileset(struct map *map, size_t gid);

bool map_is_solid(struct map *map, size_t x, size_t y);

#endif
