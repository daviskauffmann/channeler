#include "conversation_node.h"
#include "conversations.h"
#include "input.h"
#include "layer.h"
#include "map.h"
#include "player.h"
#include "quest_status.h"
#include "quests.h"
#include "tileset.h"
#include "world.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define WINDOW_TITLE "Project Hypernova"
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

#define FPS_CAP 144
#define FRAME_DELAY (1000 / FPS_CAP)

#define SPRITE_SCALE 2

void draw_text(SDL_Renderer *renderer, TTF_Font *font, size_t px, size_t x, size_t y, size_t w, SDL_Color fg, const char *const fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    size_t size = vsnprintf(NULL, 0, fmt, args) + 1;
    char *text = malloc(size);
    vsprintf_s(text, size, fmt, args);
    va_end(args);

    SDL_Surface *surface = TTF_RenderText_Blended_Wrapped(font, text, fg, (uint32_t)w);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_Rect dstrect = {
        (int)x,
        (int)y,
        surface->w,
        surface->h};

    SDL_RenderCopy(renderer, texture, NULL, &dstrect);

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
    free(text);
}

int main(int argc, char *argv[])
{
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) != 0)
    {
        printf("Error: Failed to initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    int img_flags = IMG_INIT_PNG;
    if (IMG_Init(img_flags) != img_flags)
    {
        printf("Error: Failed to initialize SDL_image: %s\n", IMG_GetError());
        return 1;
    }

    int mix_flags = 0;
    if (Mix_Init(mix_flags) != mix_flags)
    {
        printf("Error: Failed to initialize SDL_mixer: %s\n", Mix_GetError());
        return 1;
    }

    if (TTF_Init() != 0)
    {
        printf("Error: Failed to initialize SDL_ttf: %s\n", TTF_GetError());
        return 1;
    }

    SDL_Window *window;
    SDL_Renderer *renderer;
    if (SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer) != 0)
    {
        printf("Error: Failed to create window and renderer: %s\n", SDL_GetError());
        return 1;
    }
    SDL_SetWindowTitle(window, WINDOW_TITLE);

    if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 1024) != 0)
    {
        printf("Error: Failed to initialize the mixer API: %s\n", Mix_GetError());
        return 1;
    }

    struct player player;
    player_init(&player, 0);

    struct world world;
    world_load(&world, "assets/world.world");

    struct quests quests;
    quests_load(&quests, "assets/quests.json");

    struct conversations conversations;
    conversations_load(&conversations, "assets/conversations.json");

    int pt = 18;
    TTF_Font *font = TTF_OpenFont("assets/NinjaAdventure/HUD/Font/NormalFont.ttf", pt);

    SDL_Texture *player_sprites = IMG_LoadTexture(renderer, "assets/NinjaAdventure/Actor/Characters/BlueNinja/SpriteSheet.png");

    struct map *map = &world.maps[player.map_index];
    map_activate(map, renderer);

    bool quest_log_open = false;

    bool quit = false;
    while (!quit)
    {
        static uint32_t current_time = 0;
        uint32_t frame_start = SDL_GetTicks();
        uint32_t previous_time = current_time;
        current_time = frame_start;
        float delta_time = (current_time - previous_time) / 1000.0f;

        int num_keys;
        const uint8_t *keys = SDL_GetKeyboardState(&num_keys);

        int mouse_x, mouse_y;
        /*uint32_t mouse = */ SDL_GetMouseState(&mouse_x, &mouse_y);

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_KEYDOWN:
            {
                switch (event.key.keysym.sym)
                {
                case SDLK_RETURN:
                {
                    if (keys[SDL_SCANCODE_LALT])
                    {
                        uint32_t flags = SDL_GetWindowFlags(window);
                        if (flags & SDL_WINDOW_FULLSCREEN_DESKTOP)
                        {
                            SDL_SetWindowFullscreen(window, 0);
                        }
                        else
                        {
                            SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
                        }
                    }
                }
                break;
                case SDLK_ESCAPE:
                {
                    quest_log_open = false;
                    player_end_conversation(&player);
                }
                break;
                case SDLK_SPACE:
                {
                    if (player.conversation_node)
                    {
                        player_advance_conversation(&player);
                    }
                    else
                    {
                        player_attack(&player, map);
                    }
                }
                break;
                case SDLK_1:
                case SDLK_2:
                case SDLK_3:
                case SDLK_4:
                case SDLK_5:
                case SDLK_6:
                case SDLK_7:
                case SDLK_8:
                case SDLK_9:
                {
                    if (player.conversation_node)
                    {
                        size_t choice_index = event.key.keysym.sym - 48;
                        player_choose_conversation_response(&player, choice_index);
                    }
                }
                break;
                case SDLK_j:
                {
                    quest_log_open = !quest_log_open;
                }
                break;
                case SDLK_F1:
                {
                    player.map_index = 0;

                    map_deactivate(map);
                    map = &world.maps[player.map_index];
                    map_activate(map, renderer);
                }
                break;
                case SDLK_F2:
                {
                    player.map_index = 1;

                    map_deactivate(map);
                    map = &world.maps[player.map_index];
                    map_activate(map, renderer);
                }
                break;
                case SDLK_F3:
                {
                    player_start_conversation(&player, &conversations, 0);
                }
                break;
                case SDLK_F4:
                {
                    player_start_conversation(&player, &conversations, 1);
                }
                break;
                case SDLK_F5:
                {
                    struct quest_status quest_status;
                    quest_status.quest_index = 0;
                    quest_status.stage_index = 1;
                    player_set_quest_status(&player, quest_status);
                }
                break;
                case SDLK_F6:
                {
                    struct quest_status quest_status;
                    quest_status.quest_index = 0;
                    quest_status.stage_index = 3;
                    player_set_quest_status(&player, quest_status);
                }
                break;
                }
            }
            break;
            case SDL_QUIT:
            {
                quit = true;
            }
            break;
            }
        }

        struct input input;
        input.dx = 0;
        input.dy = 0;

        if (keys[SDL_SCANCODE_W])
        {
            input.dy = -1;
        }
        if (keys[SDL_SCANCODE_A])
        {
            input.dx = -1;
        }
        if (keys[SDL_SCANCODE_S])
        {
            input.dy = 1;
        }
        if (keys[SDL_SCANCODE_D])
        {
            input.dx = 1;
        }

        player_update(&player, &input, map, delta_time);

        for (size_t i = 0; i < world.num_maps; i++)
        {
            map_update(&world.maps[i], delta_time);
        }

        size_t view_width = WINDOW_WIDTH / SPRITE_SCALE;
        size_t view_height = WINDOW_HEIGHT / SPRITE_SCALE;
        int64_t view_x = (int64_t)player.pos_x - view_width / 2;
        int64_t view_y = (int64_t)player.pos_y - view_height / 2;
        if (view_x + view_width > map->width * map->tile_width)
        {
            view_x = (map->width * map->tile_width) - view_width;
        }
        if (view_x < 0)
        {
            view_x = 0;
        }
        if (view_y + view_height > map->height * map->tile_height)
        {
            view_y = (map->height * map->tile_height) - view_height;
        }
        if (view_y < 0)
        {
            view_y = 0;
        }

        SDL_RenderClear(renderer);

        for (int64_t y = view_y / map->tile_height; y <= (int64_t)((view_y + view_height) / map->tile_height); y++)
        {
            for (int64_t x = view_x / map->tile_width; x <= (int64_t)((view_x + view_width) / map->tile_width); x++)
            {
                for (size_t i = 0; i < map->num_layers; i++)
                {
                    struct layer *layer = &map->layers[i];

                    struct tile *tile = layer_get_tile(layer, map, x, y);
                    if (tile)
                    {
                        struct tileset *tileset = map_get_tileset(map, tile->gid);

                        SDL_Rect srcrect = {
                            (int)(((tile->gid - tileset->first_gid) % tileset->columns) * map->tile_width),
                            (int)(((tile->gid - tileset->first_gid) / tileset->columns) * map->tile_height),
                            (int)map->tile_width,
                            (int)map->tile_height};

                        SDL_Rect dstrect = {
                            (int)(((x * map->tile_width) - view_x) * SPRITE_SCALE),
                            (int)(((y * map->tile_height) - view_y) * SPRITE_SCALE),
                            (int)(map->tile_width * SPRITE_SCALE),
                            (int)(map->tile_height * SPRITE_SCALE)};

                        double angle = 0;
                        if (tile->d_flip)
                        {
                            if (tile->h_flip)
                            {
                                angle = 90;
                            }
                            if (tile->v_flip)
                            {
                                angle = 270;
                            }
                        }
                        else
                        {
                            if (tile->h_flip && tile->v_flip)
                            {
                                angle = 180;
                            }
                        }

                        SDL_RenderCopyEx(
                            renderer,
                            map->sprites[tileset->index],
                            &srcrect,
                            &dstrect,
                            angle,
                            NULL,
                            SDL_FLIP_NONE);
                    }
                }
            }
        }

        for (size_t i = 0; i < MAX_MOBS; i++)
        {
            struct mob *mob = &map->mobs[i];

            if (mob->alive)
            {
                struct tileset *tileset = map_get_tileset(map, mob->gid);

                SDL_Rect srcrect = {
                    (int)(((mob->gid - tileset->first_gid) % tileset->columns) * map->tile_width),
                    (int)(((mob->gid - tileset->first_gid) / tileset->columns) * map->tile_height),
                    (int)map->tile_width,
                    (int)map->tile_height};

                SDL_Rect dstrect = {
                    (int)((mob->x - view_x) * SPRITE_SCALE),
                    (int)((mob->y - view_y) * SPRITE_SCALE),
                    (int)(map->tile_width * SPRITE_SCALE),
                    (int)(map->tile_height * SPRITE_SCALE)};

                SDL_RenderCopy(renderer, map->sprites[tileset->index], &srcrect, &dstrect);
            }
        }

        {
            SDL_Rect srcrect;
            switch (player.animation)
            {
            case ANIMATION_IDLE:
                srcrect.x = (int)player.direction * 16;
                srcrect.y = 0;
                break;
            case ANIMATION_WALKING:
                srcrect.x = (int)player.direction * 16;
                srcrect.y = (1 + (player.frame_index % 3)) * 16;
                break;
            }
            srcrect.w = 16;
            srcrect.h = 16;

            SDL_Rect dstrect = {
                (int)((player.pos_x - view_x) * SPRITE_SCALE),
                (int)((player.pos_y - view_y) * SPRITE_SCALE),
                (int)(16 * SPRITE_SCALE),
                (int)(16 * SPRITE_SCALE)};

            SDL_RenderCopy(renderer, player_sprites, &srcrect, &dstrect);
        }

        if (quest_log_open)
        {
            SDL_Rect rect = {12, 12, WINDOW_WIDTH - 24, WINDOW_HEIGHT - 24};
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_RenderFillRect(renderer, &rect);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

            for (size_t i = 0; i < player.num_quest_statuses; i++)
            {
                struct quest_status *status = &player.quest_statuses[i];
                struct quest *quest = &quests.quests[status->quest_index];
                struct quest_stage *stage = &quest->stages[status->stage_index];
                draw_text(renderer, font, pt, 24, 24 * (i + 1), WINDOW_WIDTH - 24, (SDL_Color){255, 255, 255}, "%s: %s", quest->name, stage->description);
            }
        }

        if (player.conversation_node)
        {
            SDL_Rect rect = {12, WINDOW_HEIGHT - 100 - 12, WINDOW_WIDTH - 24, 100};
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_RenderFillRect(renderer, &rect);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

            draw_text(renderer, font, pt, 24, WINDOW_HEIGHT - 100, WINDOW_WIDTH, (SDL_Color){255, 255, 255}, "%s", player.conversation_node->text);

            for (size_t i = 0; i < player.conversation_node->num_children; i++)
            {
                struct conversation_node *child = &player.conversation_node->children[i];
                if (child->type == CONVERSATION_NODE_RESPONSE && conversation_node_check_conditions(child, &player))
                {
                    draw_text(renderer, font, pt, 24, (WINDOW_HEIGHT - 100) + 24 * (i + 1), WINDOW_WIDTH, (SDL_Color){255, 255, 255}, "%zu) %s", i + 1, child->text);
                }
            }
        }

        SDL_RenderPresent(renderer);

        uint32_t frame_end = SDL_GetTicks();
        uint32_t frame_time = frame_end - frame_start;
        if (FRAME_DELAY > frame_time)
        {
            SDL_Delay(FRAME_DELAY - frame_time);
        }
    }

    TTF_CloseFont(font);

    world_unload(&world);
    quests_unload(&quests);
    conversations_unload(&conversations);

    player_uninit(&player);

    Mix_CloseAudio();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    IMG_Quit();
    Mix_Quit();
    TTF_Quit();

    SDL_Quit();

    return 0;
}
