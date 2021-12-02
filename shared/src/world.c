#include <shared/world.h>

#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

void world_init(struct world *world)
{
    srand((unsigned int)time(NULL));

    for (int i = 0; i < NUM_MOBS; i++)
    {
        world->mobs[i].alive = true;
        world->mobs[i].x = rand() % 1000;
        world->mobs[i].y = rand() % 500;
    }
}

void world_update(struct world *world, float delta_time)
{
    static float timer = 0;
    timer += delta_time;
    if (timer >= 2.0f)
    {
        timer = 0;

        for (int i = 0; i < NUM_MOBS; i++)
        {
            world->mobs[i].x = rand() % 1000;
            world->mobs[i].y = rand() % 500;
        }
    }
}
