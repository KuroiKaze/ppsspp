#include "player.h"
#include <SDL_image.h>
#include <stdio.h>
#include <stdlib.h>

#include "../map/map.h"

// --- PFADE (Bitte prüfen, ob host0:/ bei dir nötig ist) ---
#define PLAYER_IDLE_BASE_PATH "host0:/resources/Gothicvania Collection Files/Assets/Characters/PLAYABLE CHARACTERS/Cemetery Hero/Base/Sprites/hero-idle/hero-idle-"
#define PLAYER_RUN_BASE_PATH "host0:/resources/Gothicvania Collection Files/Assets/Characters/PLAYABLE CHARACTERS/Cemetery Hero/Base/Sprites/hero-run/hero-run-"
#define PLAYER_ATTACK_BASE_PATH "host0:/resources/Gothicvania Collection Files/Assets/Characters/PLAYABLE CHARACTERS/Cemetery Hero/Base/Sprites/hero-attack/frame"
#define PLAYER_HURT_PATH "host0:/resources/Gothicvania Collection Files/Assets/Characters/PLAYABLE CHARACTERS/Cemetery Hero/Base/Sprites/hero-hurt/hero-hurt.png"

extern SDL_Texture *load_texture(SDL_Renderer *renderer, const char *path);
extern void debug_log(const char *format, ...);

#define HIT_BOX_SCALE_W 0.3f
#define HIT_BOX_SCALE_H 0.8f

#define ATTACK_HITBOX_WIDTH 40
#define ATTACK_HITBOX_HEIGHT 20
#define ATTACK_HITBOX_OFFSET_Y 5
#define ATTACK_OVERLAP 15

#define GRAVITY 0.4f
#define JUMP_FORCE 10.0f
#define MAX_FALL_SPEED 10.0f

void player_cleanup_all_loaded(Player *player) {
    for (int i = 0; i < IDLE_FRAME_COUNT; i++) {
        if (player->idle_frames[i]) SDL_DestroyTexture(player->idle_frames[i]);
    }
    for (int i = 0; i < RUN_FRAME_COUNT; i++) {
        if (player->running_frames[i]) SDL_DestroyTexture(player->running_frames[i]);
    }
    for (int i = 0; i < ATTACK_FRAME_COUNT; i++) {
        if (player->attack_frames[i]) SDL_DestroyTexture(player->attack_frames[i]);
    }
    if (player->hurt_frame) SDL_DestroyTexture(player->hurt_frame);
}

Player player_init(SDL_Renderer *renderer) {
    Player player = {0};
    Player failed_player = {0};
    char path_buffer[256];

    // --- IDLE ---
    for (int i = 0; i < IDLE_FRAME_COUNT; i++) {
        sprintf(path_buffer, "%s%d.png", PLAYER_IDLE_BASE_PATH, i + 1);
        player.idle_frames[i] = load_texture(renderer, path_buffer);
        if (!player.idle_frames[i]) return failed_player;
    }

    // --- RUN ---
    for (int i = 0; i < RUN_FRAME_COUNT; i++) {
        sprintf(path_buffer, "%s%d.png", PLAYER_RUN_BASE_PATH, i + 1);
        player.running_frames[i] = load_texture(renderer, path_buffer);
        if (!player.running_frames[i]) {
            player_cleanup_all_loaded(&player);
            return failed_player;
        }
    }

    // --- ATTACK ---
    for (int i = 0; i < ATTACK_FRAME_COUNT; i++) {
        sprintf(path_buffer, "%s%d.png", PLAYER_ATTACK_BASE_PATH, i + 1);
        player.attack_frames[i] = load_texture(renderer, path_buffer);
        if (!player.attack_frames[i]) {
            player_cleanup_all_loaded(&player);
            return failed_player;
        }
    }

    // --- HURT ---
    player.hurt_frame = load_texture(renderer, PLAYER_HURT_PATH);

    SDL_QueryTexture(player.idle_frames[0], NULL, NULL, &player.sprite_w, &player.sprite_h);

    player.rect.w = (int)(player.sprite_w * HIT_BOX_SCALE_W);
    player.rect.h = (int)(player.sprite_h * HIT_BOX_SCALE_H);

    player.offset_x = (player.sprite_w - player.rect.w) / 2;
    player.offset_y = player.sprite_h - player.rect.h;

    player.rect.x = 50;
    player.rect.y = 50;

    player.vel_x = 0;
    player.vel_y = 0;
    player.on_ground = 0;

    player.last_time = SDL_GetTicks();
    player.flip_direction = SDL_FLIP_NONE;
    player.health = 100;
    player.hurt_timer_end = 0;

    player.attack_timer_end = 0;
    player.attack_cooldown_end = 0;
    player.current_attack_frame = 0;
    player.attack_rect = (SDL_Rect){0, 0, 0, 0};

    player.prev_x = player.rect.x;
    player.current_x = player.rect.x;

    return player;
}

void player_handle_input_physics(Player *player, const SceCtrlData *pad, int map_width, int map_height) {
    // 1. Angriff blockiert Bewegung
    if (SDL_GetTicks() < player->attack_timer_end) {
        player->vel_x = 0;
        return;
    }

    // 2. Angriff starten
    if ((pad->Buttons & PSP_CTRL_CIRCLE) && SDL_GetTicks() >= player->attack_cooldown_end) {
        player->attack_timer_end = SDL_GetTicks() + ATTACK_DURATION;
        player->attack_cooldown_end = SDL_GetTicks() + ATTACK_COOLDOWN;
        player->current_attack_frame = 0;
        player->vel_x = 0;
        return;
    }

    // 3. Bewegung
    player->vel_x = 0;

    if (pad->Buttons & PSP_CTRL_LEFT) {
        player->vel_x = -MOVEMENT_SPEED;
        player->flip_direction = SDL_FLIP_HORIZONTAL;
    }
    if (pad->Buttons & PSP_CTRL_RIGHT) {
        player->vel_x = MOVEMENT_SPEED;
        player->flip_direction = SDL_FLIP_NONE;
    }

    // Map Grenzen (Links/Rechts)
    if (player->rect.x <= 0 && player->vel_x < 0) player->vel_x = 0;
    if (player->rect.x + player->rect.w >= map_width && player->vel_x > 0) player->vel_x = 0;

    // Springen
    int cross_just_pressed = (pad->Buttons & PSP_CTRL_CROSS) && !(player->prev_buttons & PSP_CTRL_CROSS);

    if (cross_just_pressed && player->on_ground) {
        player->vel_y = -JUMP_FORCE;
        player->on_ground = 0;
    }

    // --- WICHTIG: Input für den nächsten Frame speichern ---
    player->prev_buttons = pad->Buttons;

    // Debug Schaden
    if (pad->Buttons & PSP_CTRL_LTRIGGER) {
        player_decrease_health(player, 1);
    }
}

void player_update_physics(Player *player, struct Map *map) {
    // 0. Update History
    player->prev_x = player->current_x;

    // 1. Gravity
    player->vel_y += GRAVITY;
    if (player->vel_y > MAX_FALL_SPEED) player->vel_y = MAX_FALL_SPEED;

    // 2. Y-Movement
    // Wir bewegen den Spieler ZUERST. Wenn er springt, geht er hier nach oben.
    player->rect.y += (int)player->vel_y;

    // 3. Determine Foot Position
    int foot_x = player->rect.x + (player->rect.w / 2);
    int foot_y = player->rect.y + player->rect.h;

    // 4. SLOPES & FLOOR CHECK
    // Wir holen die Bodenhöhe. Dank des Scanners in map.c finden wir auch Rampen über/unter uns.
    int floor_height = map_get_floor_height(map, foot_x, foot_y);
    int hit_ground_or_slope = 0;

    // WICHTIG: Wir snappen NUR, wenn wir NICHT aktiv nach oben springen!
    // vel_y < 0 bedeutet: Wir bewegen uns nach oben (Sprung).
    // In diesem Fall ignorieren wir den Boden komplett, damit wir abheben können.
    if (floor_height != -1 && player->vel_y >= 0) {

        int dist = foot_y - floor_height;

        // Wir unterscheiden zwei Fälle:

        // A) LANDEN: Wir sind in den Boden gefallen (dist <= 0) oder berühren ihn gerade.
        // Toleranzbereich: Bis zu 4 Pixel "drin" oder genau drauf.
        int condition_land = (dist <= 4 && dist >= -16);

        // B) KLEBEN (Slope Down): Wir laufen bergab und schweben kurz über dem Boden.
        // Das erlauben wir NUR, wenn wir vorher schon am Boden waren ("on_ground").
        // Toleranz: Bis zu 24 Pixel darüber (für steile Rampen).
        int condition_stick = (player->on_ground && dist > 4 && dist <= 24);

        if (condition_land || condition_stick) {

            // Snap ausführen
            if (dist <= 4) {
                // Hartes Snappen (bei Landung oder Bergauf)
                player->rect.y = floor_height - player->rect.h;
            } else {
                // Weiches Snappen (bei Bergab, damit Kamera nicht ruckelt)
                // Wir bewegen uns max 10px pro Frame nach unten.
                int snap_speed = 10;
                player->rect.y += snap_speed;
                if (player->rect.y > floor_height - player->rect.h) {
                    player->rect.y = floor_height - player->rect.h;
                }
            }

            // Physik resetten
            player->vel_y = 0;
            player->on_ground = 1;
            hit_ground_or_slope = 1;
        }
    }

    // 5. Solid Block Check (Nur wenn kein Slope gefunden wurde)
    // Das ist ein Fallback für Kanten, damit man nicht in Blöcke reinspringt.
    if (!hit_ground_or_slope) {
        // ... (Dein alter Code für Edge Priority, unverändert) ...
        if (player->vel_y >= 0) {
            int left_foot = player->rect.x + 4;
            int right_foot = player->rect.x + player->rect.w - 4;
            foot_y = player->rect.y + player->rect.h; // Update nach potentiellem Y-Move

            // Nur prüfen, wenn wir NICHT schon gesnappt sind
            if (map_is_solid(map, left_foot, foot_y) || map_is_solid(map, right_foot, foot_y)) {
                int tile_top = (foot_y / 16) * 16;
                if (foot_y <= tile_top + 10 && foot_y >= tile_top - 10) {
                    player->rect.y = tile_top - player->rect.h;
                    player->vel_y = 0;
                    player->on_ground = 1;
                    hit_ground_or_slope = 1;
                }
            }
        }
    }

    // Wenn wir nirgendwo gelandet sind, sind wir in der Luft.
    if (!hit_ground_or_slope) {
        player->on_ground = 0;
    }

    // 6. X-Movement
    player->rect.x += (int)player->vel_x;

    // 7. Wall Collisions
    int center_y = player->rect.y + (player->rect.h / 2);

    if (player->vel_x > 0) { // Rechts
        int right_side = player->rect.x + player->rect.w;
        // WICHTIG: Wir prüfen die Wand auf halber Höhe, nicht an den Füßen!
        // Sonst bleiben wir an kleinen Stufen der Rampe hängen.
        if (map_is_solid(map, right_side, center_y)) {
            int tile_left = (right_side / 16) * 16;
            player->rect.x = tile_left - player->rect.w;
            player->vel_x = 0;
        }
    }
    else if (player->vel_x < 0) { // Links
        int left_side = player->rect.x;
        if (map_is_solid(map, left_side, center_y)) {
            int tile_right = ((left_side / 16) * 16) + 16;
            player->rect.x = tile_right;
            player->vel_x = 0;
        }
    }

    // 8. Update Current Position
    player->current_x = player->rect.x;
}

void player_update_attack(Player *player) {
    Uint32 current_time = SDL_GetTicks();

    if (current_time < player->attack_timer_end) {
        Uint32 time_since_attack = current_time - (player->attack_timer_end - ATTACK_DURATION);
        int frame_duration = ATTACK_DURATION / ATTACK_FRAME_COUNT;

        player->current_attack_frame = time_since_attack / frame_duration;
        if (player->current_attack_frame >= ATTACK_FRAME_COUNT) {
            player->current_attack_frame = ATTACK_FRAME_COUNT - 1;
        }

        if (player->current_attack_frame == 1) {
            if (player->flip_direction == SDL_FLIP_NONE) {
                player->attack_rect.x = player->rect.x + player->rect.w - ATTACK_OVERLAP;
            } else {
                player->attack_rect.x = (player->rect.x + ATTACK_OVERLAP) - ATTACK_HITBOX_WIDTH;
            }

            player->attack_rect.y = player->rect.y + player->rect.h / 2 - ATTACK_HITBOX_HEIGHT / 2 + ATTACK_HITBOX_OFFSET_Y;
            player->attack_rect.w = ATTACK_HITBOX_WIDTH;
            player->attack_rect.h = ATTACK_HITBOX_HEIGHT;
        } else {
            player->attack_rect = (SDL_Rect){0, 0, 0, 0};
        }

    } else {
        player->attack_rect = (SDL_Rect){0, 0, 0, 0};
    }
}

void player_update_animation(Player *player, int is_moving) {
    if (SDL_GetTicks() < player->attack_timer_end) return;

    Uint32 current_time = SDL_GetTicks();
    if (current_time > player->last_time + ANIMATION_SPEED) {
        if (is_moving) {
            player->current_run_frame = (player->current_run_frame + 1) % RUN_FRAME_COUNT;
        } else {
            player->current_idle_frame = (player->current_idle_frame + 1) % IDLE_FRAME_COUNT;
        }
        player->last_time = current_time;
    }
}

void player_decrease_health(Player *player, int amount) {
    if (SDL_GetTicks() < player->hurt_timer_end) return;

    player->health -= amount;
    player->hurt_timer_end = SDL_GetTicks() + HURT_DURATION;

    if (player->health < 0) player->health = 0;
}

void player_render(SDL_Renderer *renderer, Player *player, int is_moving, int camera_x, int camera_y) {
    SDL_Texture *current_texture = NULL;
    Uint32 current_ticks = SDL_GetTicks();
    SDL_Texture *base_texture = NULL;

    if (current_ticks < player->attack_timer_end) {
        base_texture = player->attack_frames[player->current_attack_frame];
    }
    else if (current_ticks < player->hurt_timer_end) {
        base_texture = player->hurt_frame ? player->hurt_frame :
                       (is_moving ? player->running_frames[player->current_run_frame] :
                        player->idle_frames[player->current_idle_frame]);
    } else {
        base_texture = is_moving ?
                       player->running_frames[player->current_run_frame] :
                       player->idle_frames[player->current_idle_frame];
    }

    if (!base_texture) return;

    SDL_Rect render_rect = {
            (player->rect.x - player->offset_x) - camera_x,
            (player->rect.y - player->offset_y) - camera_y,
            player->sprite_w,
            player->sprite_h
    };


    SDL_SetTextureColorMod(base_texture, 255, 255, 255);

    if (current_ticks < player->hurt_timer_end) {
        if (current_ticks % 100 < 50) {
            SDL_SetTextureColorMod(base_texture, 255, 100, 100);
        } else {
            SDL_SetTextureColorMod(base_texture, 255, 255, 255);
        }
    }

    current_texture = base_texture;

    if (current_texture) {
        SDL_RenderCopyEx(renderer, current_texture, NULL, &render_rect, 0.0, NULL, player->flip_direction);
    }

// --- DEBUG: HITBOX ZEICHNEN ---
    // Speichere die aktuelle Farbe
    Uint8 old_r, old_g, old_b, old_a;
    SDL_GetRenderDrawColor(renderer, &old_r, &old_g, &old_b, &old_a);

    // Berechne die Position relativ zur Kamera
    SDL_Rect debug_rect = player->rect;
    debug_rect.x -= camera_x;
    debug_rect.y -= camera_y;
/*
    // Zeichne roten Rahmen
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderDrawRect(renderer, &debug_rect);
*/
    // Stelle alte Farbe wieder her
    SDL_SetRenderDrawColor(renderer, old_r, old_g, old_b, old_a);

}

void player_cleanup(Player *player) {
    player_cleanup_all_loaded(player);
}