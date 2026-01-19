#ifndef LEVEL_HANDLER_H
#define LEVEL_HANDLER_H

#include <SDL.h>
#include <pspctrl.h>
#include "level.h"
#include "../player/player.h"

#define MAX_LEVELS 5

typedef struct {
    Level current_level;
    int current_level_index;
    int total_levels;

    // Config: Array of map paths
    const char* map_paths[MAX_LEVELS];

    // Dependencies
    SDL_Renderer* renderer;
    Player* player;

    bool transition_pending;
} LevelHandler;

LevelHandler level_handler_init(SDL_Renderer* renderer, Player* player);
void level_handler_update(LevelHandler* handler, SceCtrlData* pad);
void level_handler_render(LevelHandler* handler, int camera_x, int camera_y);
void level_handler_cleanup(LevelHandler* handler);

#endif