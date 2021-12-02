#ifndef WORLD_H
#define WORLD_H

#define NUM_MOBS 20

struct mob
{
    float x;
    float y;
    int alive;
};

struct world
{
    struct mob mobs[NUM_MOBS];
};

void world_init(struct world *world);
void world_update(struct world *world, float delta_time);

#endif
