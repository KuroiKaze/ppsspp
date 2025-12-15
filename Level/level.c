#include "level.h"
#include <stdlib.h>
#include "../enemies/mummy/mummy.h"
#include "../enemies/slime/slime.h"
#include "../ui/ui.h"
#include "../bgm/bgmHandler.h"

#define MAX_ENEMIES 5

BGMHandler bgm;
SFX sfx;

extern void debug_log(const char *format, ...);

bool all_enemies_dead(Level* level) {
    if (!level) return true;
    for (int i = 0; i < level->enemy_count; i++) {
        if (!level->enemies[i]->entity.is_dead) {
            return false;
        }
    }
    return true;
}

void level1_init(Level* level, SDL_Renderer* renderer, Player* player) {
    if (!level || !player) return;
    sfx_init();

    // Player Pointer setzen
    level->player = player;

    // Map & Background
    map_init(&level->map, renderer,
        "host0:/resources/maps/map_level1.json",
        "host0:/resources/Gothicvania Collection Files/Assets/Environments/Cemetery/base/Layers/tileset.png",
        "host0:/resources/Gothicvania Collection Files/Assets/Environments/Cemetery/Add-on 1/Layers/tower_size_reduced.png",
        "host0:/resources/sprites/collision_tileset.png");

    level->layer_far_back = background_layer_init(renderer, "host0:/resources/Gothicvania Collection Files/Assets/Environments/Cemetery/base/Layers/background.png", 0.005f);
    level->layer_mid      = background_layer_init(renderer, "host0:/resources/Gothicvania Collection Files/Assets/Environments/Cemetery/base/Layers/mountains.png", 0.3f);
    level->layer_fore     = background_layer_init(renderer, "host0:/resources/Gothicvania Collection Files/Assets/Environments/Cemetery/base/Layers/graveyard.png", 0.5f);

    // Gegner initialisieren
    level->enemy_count = 2;
    level->enemies = malloc(sizeof(Enemy*) * MAX_ENEMIES);

    Mummy* mummy = malloc(sizeof(Mummy));
    *mummy = mummy_init(renderer, 350, 100);
    level->enemies[0] = &mummy->base;

    Slime* slime = malloc(sizeof(Slime));
    *slime = slime_init(renderer, 500, 100);
    level->enemies[1] = &slime->base;

    if (!bgm_init()) {
        debug_log("BGM konnte nicht initialisiert werden!\n");
    }
    bgm_play(&bgm, "host0:/resources/music/medieval-ambient-236809.wav", -1); // -1 = loop
    // Initialisierung
    Mix_VolumeChunk(mummy->base.entity.grunt_sfx, 10); // Lautstärke 0-128
    mummy->base.entity.grunt_sfx_channel = Mix_PlayChannel(-1, mummy->base.entity.grunt_sfx, -1); // -1 = unendlich loopen

    if (TTF_Init() == -1) {
        debug_log("TTF_Init Error: %s\n", TTF_GetError());
    }
        level->font = TTF_OpenFont("host0:/resources/fonts/ARIAL.ttf", 16);
    if (!level->font) {
        debug_log("Font load error: %s\n", TTF_GetError());
    }

    level->loot_chest = chest_init(renderer, "host0:/resources/sprites/chest-", 400, 375);
    level->chest_spawned = false; // erst nach allen Gegnern sichtbar
}

void level_update(Level* level, SceCtrlData* pad, SDL_Renderer* renderer) {
    if (!level || !level->player) return;

    Player* player = level->player;
    int is_moving = (player->entity.vel_x != 0);

    player_handle_input(player, pad);
    player_update_physics(player, &level->map);

    int map_width  = level->map.tiled_map->width  * level->map.tiled_map->tilewidth;
    int map_height = level->map.tiled_map->height * level->map.tiled_map->tileheight;

    if (player->entity.rect.x < 0) {
        player->entity.rect.x = 0;
        player->entity.vel_x = 0;
    }

    if (player->entity.rect.x + player->entity.rect.w > map_width) {
        player->entity.rect.x = map_width - player->entity.rect.w;
        player->entity.vel_x = 0;
    }

    if (player->entity.rect.y < 0) {
        player->entity.rect.y = 0;
        player->entity.vel_y = 0;
    }

    if (player->entity.rect.y + player->entity.rect.h > map_height) {
        player->entity.rect.y = map_height - player->entity.rect.h;
        player->entity.vel_y = 0;
    }

    player_update_animation(player, is_moving);
    player_update_attack(player);

    for (int i = 0; i < level->enemy_count; i++) {
        Enemy* e = level->enemies[i];
    
        enemy_take_damage_from_player(e, player->attack_rect, 10);
        enemy_update(e, player, &level->map);
        enemy_handle_attack(e, player);
    }

    if (all_enemies_dead(level) && !level->chest_spawned) {
        level->chest_spawned = true;
    }

    if (level->chest_spawned && !level->loot_chest.collected) {
        bool near_player = chest_check_collision(&level->loot_chest, player->entity.rect);

        if (near_player && (pad->Buttons & PSP_CTRL_SQUARE)) {
            if (!level->loot_chest.opening) {
                level->loot_chest.opening = true;
                level->loot_chest.current_frame = 0;
                level->loot_chest.last_frame_time = SDL_GetTicks();
            }
        }
        chest_update(&level->loot_chest);
    }
}

void level_render(Level* level, SDL_Renderer* renderer, int camera_x, int camera_y) {
    if (!level || !level->player) return;
    Player* player = level->player;

    // Hintergrund
    background_layer_render(renderer, &level->layer_far_back, camera_x, camera_y, 480, 272);
    background_layer_render(renderer, &level->layer_mid, camera_x, camera_y, 480, 272);
    background_layer_render(renderer, &level->layer_fore, camera_x, camera_y, 480, 272);

    // Map
    map_render(renderer, &level->map, camera_x, camera_y);

    // Gegner
    for(int i = 0; i < level->enemy_count; i++) {
        enemy_render(renderer, level->enemies[i], camera_x, camera_y);
    }

    // Player
    int is_moving = (player->entity.vel_x != 0);
    player_render(renderer, player, is_moving, camera_x, camera_y);

    if (level->chest_spawned && !level->loot_chest.collected) {
        chest_render(renderer, &level->loot_chest, camera_x, camera_y);

        bool near_player = chest_check_collision(&level->loot_chest, level->player->entity.rect);
        if (near_player) {
            SDL_Color color = {255, 255, 255, 255};
            SDL_Surface* text_surface = TTF_RenderText_Blended(level->font, "Press Square to open", color);
            if (text_surface) {
                SDL_Texture* text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
                SDL_FreeSurface(text_surface);

                SDL_Rect text_rect;
                text_rect.x = level->loot_chest.rect.x - camera_x;
                text_rect.y = level->loot_chest.rect.y - camera_y;
                SDL_QueryTexture(text_texture, NULL, NULL, &text_rect.w, &text_rect.h );

                SDL_RenderCopy(renderer, text_texture, NULL, &text_rect);
                SDL_DestroyTexture(text_texture);
            }
        }   
}
    // UI
    ui_render_health_bar(renderer, player->entity.health);
}

void level_reset(Level* level) {
    if (!level || !level->player) return;
    Player* player = level->player;

    player->entity.rect.x = 50;
    player->entity.rect.y = 20;
    player->entity.health = PLAYER_MAX_HEALTH;
    player->entity.vel_x = 0;
    player->entity.vel_y = 0;

    for (int i = 0; i < level->enemy_count; i++) {
        level->enemies[i]->entity.health = ENEMY_MAX_HEALTH;
        level->enemies[i]->entity.rect.x = level->enemies[i]->spawn_x;
        level->enemies[i]->entity.rect.y = level->enemies[i]->spawn_y;
        level->enemies[i]->is_moving = 0;
        level->enemies[i]->entity.vel_x = 0;
        level->enemies[i]->entity.vel_y = 0;
    }
}

void level_cleanup(Level* level) {
    if (!level) return;

    for(int i = 0; i < level->enemy_count; i++) {
        free(level->enemies[i]);
        level->enemies[i] = NULL;
    }
    free(level->enemies);
    level->enemies = NULL;

    if (level->font) {
        TTF_CloseFont(level->font);
        level->font = NULL;
    }
    TTF_Quit();

    map_cleanup(&level->map);
    background_layer_cleanup(&level->layer_far_back);
    background_layer_cleanup(&level->layer_mid);
    background_layer_cleanup(&level->layer_fore);
}