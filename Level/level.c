#include "level.h"
#include <stdlib.h>
#include "../enemies/mummy/mummy.h"
#include "../enemies/slime/slime.h"
#include "../enemies/shurikenDude/shurikenDude.h"
#include "../ui/ui.h"
#include "../bgm/bgmHandler.h"
#include "../items/item.h"

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

void level_scan_entities(Level* level, SDL_Renderer* renderer) {
    if (!level || !level->map.collision_layer) {
        debug_log("SCAN_ERROR: Kein Collision Layer vorhanden!");
        return;
    }

    cute_tiled_layer_t* col = level->map.collision_layer;
    debug_log("SCAN_START: Layer %dx%d", col->width, col->height);

    // 1. Count Enemies
    int enemy_count = 0;
    for (int i = 0; i < col->width * col->height; i++) {
        // HIER LOGGEN WIR DIE GIDS, DIE DU HÄNDISCH EINGETRAGEN HAST
        int gid = col->data[i];
        if (gid > 0) {
            int shape = get_tile_shape(&level->map, gid);
            if (shape == SHAPE_ZOMBIE_SPAWN || shape == SHAPE_SLIME_SPAWN || shape == SHAPE_SHURIKENDUDE_SPAWN) {
                enemy_count++;
            }
            // Logge nur Spezial-GIDs, um das Log nicht zu fluten
            if (gid >= 687) {
                debug_log("FOUND_GID: %d at index %d -> Shape: %d", gid, i, shape);
            }
        }
    }

    debug_log("SCAN_INFO: Enemies gezaehlt: %d", enemy_count);
    level->enemy_count = enemy_count;

    if (enemy_count > 0) {
        level->enemies = malloc(sizeof(Enemy*) * enemy_count);
        // SICHERHEIT: Array nullen, damit wir keine wilden Pointer haben
        for(int i=0; i<enemy_count; i++) level->enemies[i] = NULL;
    } else {
        level->enemies = NULL;
    }

    // 2. Iterate and Spawn
    int enemy_idx = 0;
    for (int y = 0; y < col->height; y++) {
        for (int x = 0; x < col->width; x++) {
            int index = y * col->width + x;
            int gid = col->data[index];
            if (gid <= 0) continue;

            int shape = get_tile_shape(&level->map, gid);
            int world_x = x * 16;
            int world_y = y * 16;

            if (shape == SHAPE_PLAYER_SPAWN) {
                level->player->entity.rect.x = world_x;
                level->player->entity.rect.y = (world_y + 16) - level->player->entity.rect.h - 2;
                debug_log("SPAWN_PLAYER: %d, %d", world_x, world_y);
            }
            else if (shape == SHAPE_CHEST) {
                level->chest_spawn_x = world_x;
                level->chest_spawn_y = world_y - 18;
                debug_log("SPAWN_CHEST: %d, %d", world_x, world_y);
            }
            else if (shape == SHAPE_ZOMBIE_SPAWN || shape == SHAPE_SLIME_SPAWN || shape == SHAPE_SHURIKENDUDE_SPAWN) {
                if (enemy_idx < enemy_count) {
                    if (shape == SHAPE_ZOMBIE_SPAWN) {
                        Mummy* m = malloc(sizeof(Mummy));
                        *m = mummy_init(renderer, world_x, world_y);
                        m->base.base.entity.rect.y = (world_y + 16) - m->base.base.entity.rect.h - 2;
                        m->base.base.spawn_x = m->base.base.entity.rect.x;
                        m->base.base.spawn_y = m->base.base.entity.rect.y;
                        level->enemies[enemy_idx++] = &m->base.base;
                        debug_log("SPAWN_MUMMY: Index %d", enemy_idx-1);
                    } 
                    else if (shape == SHAPE_SLIME_SPAWN) {
                        Slime* s = malloc(sizeof(Slime));
                        *s = slime_init(renderer, world_x, world_y);
                        s->base.base.entity.rect.y = (world_y + 16) - s->base.base.entity.rect.h - 2;
                        s->base.base.spawn_x = s->base.base.entity.rect.x;
                        s->base.base.spawn_y = s->base.base.entity.rect.y;
                        level->enemies[enemy_idx++] = &s->base.base;
                        debug_log("SPAWN_SLIME: Index %d", enemy_idx-1);
                    }
                    else if (shape == SHAPE_SHURIKENDUDE_SPAWN) {
                        ShurikenDude* s = malloc(sizeof(ShurikenDude));
                        *s = shurikenDude_init(renderer, world_x, world_y);
                        s->base.base.entity.rect.y = (world_y + 16) - s->base.base.entity.rect.h - 2;
                        s->base.base.spawn_x = s->base.base.entity.rect.x;
                        s->base.base.spawn_y = s->base.base.entity.rect.y;
                        level->enemies[enemy_idx++] = &s->base.base;
                        debug_log("SPAWN_SHURIKEN: Index %d", enemy_idx-1);
                    }
                    // ... analog für Slime
                }
            }
        }
    }
    debug_log("SCAN_COMPLETE: %d Enemies erfolgreich initialisiert.", enemy_idx);
}

void level_load(Level* level, SDL_Renderer* renderer, Player* player, const char* map_path, const char** texture_paths, int tex_count, BgConfig* bg_configs) {
    if (!level || !player) return;
    level->player = player;

    debug_log("DEBUG: Starte map_init...");
    int status = map_init(&level->map, renderer, map_path, texture_paths, tex_count);
    
    if (status != 1) {
        debug_log("DEBUG: map_init fehlgeschlagen mit Status %d", status);
        return;
    }

    if (level->map.tiled_map == NULL) {
        debug_log("CRITICAL: map_init sagte 0, aber tiled_map ist NULL!");
        return;
    }

    debug_log("DEBUG: Starte background_layer_init...");
    // Hier crasht es oft, wenn bg_configs[0].path Müll enthält
    level->layer_far_back = background_layer_init(renderer, bg_configs[0].path, bg_configs[0].speed, bg_configs[0].scale);
    level->layer_mid      = background_layer_init(renderer, bg_configs[1].path, bg_configs[1].speed, bg_configs[1].scale);

    if (bg_configs[2].path != NULL) {
        level->layer_fore = background_layer_init(renderer, bg_configs[2].path, bg_configs[2].speed, bg_configs[2].scale);
    } else {
        level->layer_fore.texture = NULL;
    }

    debug_log("DEBUG: Starte level_scan_entities...");
    level_scan_entities(level, renderer);
    
    debug_log("DEBUG: level_load fast fertig...");
    // Chest
    level->chest_spawned = false;

    // item / loot
    Item health_potion; 
    health_potion.type = HEALTH_POTION;
    health_potion.amount = 3; // 3x healing potion in chest
    
    level->loot_chest = chest_init(renderer, "resources/sprites/chest-", level->chest_spawn_x, level->chest_spawn_y, health_potion);

    level->txt_door_texture = IMG_LoadTexture(renderer, "resources/ui/text_door.png");
    if (level->txt_door_texture) {
        SDL_QueryTexture(level->txt_door_texture, NULL, NULL, &level->txt_door_w, &level->txt_door_h);
    } else {
        debug_log("Failed to load door text!");
    }

    // 2. Load Chest Text
    level->txt_chest_texture = IMG_LoadTexture(renderer, "resources/ui/text_chest.png");
    if (level->txt_chest_texture) {
        SDL_QueryTexture(level->txt_chest_texture, NULL, NULL, &level->txt_chest_w, &level->txt_chest_h);
    } else {
        debug_log("Failed to load chest text!");
    }

    if (bgm_init()) {
        bgm_play(&bgm, "resources/music/medieval-ambient-236809.wav", -1); 
    }
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

        if (e->attack_type == RANGED) { // move projectiles of ranged enemies towards player
            RangedEnemy* re = (RangedEnemy*)e;
            projectiles_update(re->projectiles, player);
        }
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
                add_loot_to_player(&level->loot_chest, player);
            }
        }
        chest_update(&level->loot_chest);

    }
}

void level_render(Level* level, SDL_Renderer* renderer, int camera_x, int camera_y) {
    if (!level || !level->player) return;
    Player* player = level->player;

    // 1. Backgrounds
    background_layer_render(renderer, &level->layer_far_back, camera_x, camera_y, 480, 272);
    background_layer_render(renderer, &level->layer_mid, camera_x, camera_y, 480, 272);
    background_layer_render(renderer, &level->layer_fore, camera_x, camera_y, 480, 272);

    // 2. Map
    map_render(renderer, &level->map, camera_x, camera_y);

    // 3. Enemies
    for(int i = 0; i < level->enemy_count; i++) {
        enemy_render(renderer, level->enemies[i], camera_x, camera_y);

        if (level->enemies[i]->attack_type == RANGED) { // render proctiles of ranged enemies
        RangedEnemy* re = (RangedEnemy*)level->enemies[i];
        projectiles_render(renderer, re->projectiles, camera_x, camera_y);
    }
    }

    // 4. Player
    int is_moving = (player->entity.vel_x != 0);
    player_render(renderer, player, is_moving, camera_x, camera_y);

    // 5. Chest & Interaction UI
    if (level->chest_spawned && !level->loot_chest.collected) {
        chest_render(renderer, &level->loot_chest, camera_x, camera_y);

        if (chest_check_collision(&level->loot_chest, player->entity.rect)) {
            // Check if texture exists before drawing
            if (level->txt_chest_texture) {
                // Center text above chest
                int visual_chest_w = level->loot_chest.rect.w * 1.5;
                int text_x = (level->loot_chest.rect.x - camera_x) + (visual_chest_w / 2) - (level->txt_chest_w / 2);
                int text_y = (level->loot_chest.rect.y - camera_y) - level->txt_chest_h - 5;

                SDL_Rect dst = { text_x, text_y, level->txt_chest_w, level->txt_chest_h };
                SDL_RenderCopy(renderer, level->txt_chest_texture, NULL, &dst);
            }
        }
    }

    // 6. UI 
    ui_render_health_bar(renderer, player->entity.health);
    ui_render_inventory(renderer, player->inventory, player->inventory_count);

// 7. Door Indicator
    int check_x = player->entity.rect.x + (player->entity.rect.w / 2);
    int check_y = player->entity.rect.y + player->entity.rect.h - 8;

    if (map_get_shape_at(&level->map, check_x, check_y) == SHAPE_DOOR) {
        if (level->txt_door_texture) {
            SDL_Rect dst = {
                    (player->entity.rect.x - camera_x) - 20,
                    (player->entity.rect.y - camera_y) - 30,
                    level->txt_door_w, level->txt_door_h
            };
            SDL_RenderCopy(renderer, level->txt_door_texture, NULL, &dst);
        }
    }
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
        level->enemies[i]->entity.health = level->enemies[i]->max_health;
        level->enemies[i]->entity.rect.x = level->enemies[i]->spawn_x;
        level->enemies[i]->entity.rect.y = level->enemies[i]->spawn_y;
        level->enemies[i]->is_moving = 0;
        level->enemies[i]->entity.vel_x = 0;
        level->enemies[i]->entity.vel_y = 0;
    }
}

void level_cleanup(Level* level) {
    if (!level) return;

    bgm_cleanup(&bgm);

    for(int i = 0; i < level->enemy_count; i++) {
        if (level->enemies[i]->attack_type == RANGED) {
            projectile_sprite_cleanup((RangedEnemy*)level->enemies[i]);
        }
        free(level->enemies[i]);
        level->enemies[i] = NULL;
    }
    free(level->enemies);
    level->enemies = NULL;


    if (level->txt_door_texture) {
        SDL_DestroyTexture(level->txt_door_texture);
        level->txt_door_texture = NULL;
    }
    if (level->txt_chest_texture) {
        SDL_DestroyTexture(level->txt_chest_texture);
        level->txt_chest_texture = NULL;
    }


    map_cleanup(&level->map);
    background_layer_cleanup(&level->layer_far_back);
    background_layer_cleanup(&level->layer_mid);
    background_layer_cleanup(&level->layer_fore);
}