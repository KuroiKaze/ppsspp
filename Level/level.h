#ifndef LEVEL_H
#define LEVEL_H

#include <SDL.h>
#include <SDL_ttf.h>
#include "../map/map.h"
#include "../player/player.h"
#include "../enemies/enemy.h"
#include "../interactables/chest.h"
#include "../background/background.h"

typedef struct {
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

    // [NEW] Cached Text Textures
    SDL_Texture* txt_door_texture;
    int txt_door_w, txt_door_h;

    SDL_Texture* txt_chest_texture;
    int txt_chest_w, txt_chest_h;

} Level;

void level_load(Level* level, SDL_Renderer* renderer, Player* player, const char* map_path, const char** texture_paths, int tex_count);
void level_scan_entities(Level* level, SDL_Renderer* renderer);
void level_update(Level* level, SceCtrlData* pad, SDL_Renderer* renderer);
void level_render(Level* level, SDL_Renderer* renderer, int camera_x, int camera_y);
void level_reset(Level* level);
void level_cleanup(Level* level);

#endif