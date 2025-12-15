#ifndef LEVEL_H
#define LEVEL_H

#include <SDL.h>
#include "../player/player.h"
#include "../map/map.h"
#include "../enemies/enemy.h"
#include "../background/background.h"
#include "../interactables/chest.h"
#include <SDL_ttf.h>

typedef struct Level {
    Map map;
    BackgroundLayer layer_far_back;
    BackgroundLayer layer_mid;
    BackgroundLayer layer_fore;
    Player* player;
    Enemy** enemies;
    int enemy_count;
    Chest loot_chest;
    bool chest_spawned;
    TTF_Font* font;
} Level;

// Level-Funktionen
void level1_init(Level* level, SDL_Renderer* renderer, Player* player);
void level_update(Level* level, SceCtrlData* pad, SDL_Renderer* renderer);
void level_reset(Level* level);
void level_render(Level* level, SDL_Renderer* renderer, int camera_x, int camera_y);
void level_cleanup(Level* level);

#endif