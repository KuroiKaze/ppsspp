#include "player.h"
#include <SDL_image.h>
#include <stdio.h>
#include <stdlib.h>

#include "../map/map.h"

#define PLAYER_IDLE_BASE_PATH "resources/Gothicvania Collection Files/Assets/Characters/PLAYABLE CHARACTERS/Cemetery Hero/Base/Sprites/hero-idle/hero-idle-"
#define PLAYER_RUN_BASE_PATH "resources/Gothicvania Collection Files/Assets/Characters/PLAYABLE CHARACTERS/Cemetery Hero/Base/Sprites/hero-run/hero-run-"
#define PLAYER_ATTACK_BASE_PATH "resources/Gothicvania Collection Files/Assets/Characters/PLAYABLE CHARACTERS/Cemetery Hero/Base/Sprites/hero-attack/frame"
#define PLAYER_HURT_PATH "resources/Gothicvania Collection Files/Assets/Characters/PLAYABLE CHARACTERS/Cemetery Hero/Base/Sprites/hero-hurt/hero-hurt.png"

extern SDL_Texture *load_texture(SDL_Renderer *renderer, const char *path); 

#define HIT_BOX_SCALE_W 0.3f 
#define HIT_BOX_SCALE_H 0.8f

#define ATTACK_HITBOX_WIDTH 40 
#define ATTACK_HITBOX_HEIGHT 20
#define ATTACK_HITBOX_OFFSET_Y 5
#define ATTACK_OVERLAP 15

#define GRAVITY 0.4f
#define JUMP_FORCE -10.0f
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
    Player failed_player = {0}; // Struktur für den Fehlerfall
    char path_buffer[256];

    const char* asset_root = "host0:/";
    //const char* asset_root = "ur0:pspemu:/";
    
    // idle sprites
    for (int i = 0; i < IDLE_FRAME_COUNT; i++) {
        sprintf(path_buffer, "%s%s%d.png", asset_root, PLAYER_IDLE_BASE_PATH, i + 1);
        player.idle_frames[i] = load_texture(renderer, path_buffer);
        if (!player.idle_frames[i]) {
            fprintf(stderr, "Fehler: Idle Frame %d konnte nicht geladen werden (%s).\n", i + 1, path_buffer);
            for (int j = 0; j <= i; j++) {
                if (player.idle_frames[j]) SDL_DestroyTexture(player.idle_frames[j]);
            }
            return failed_player; 
        }
    }

    // run sprites
    for (int i = 0; i < RUN_FRAME_COUNT; i++) {
        sprintf(path_buffer, "%s%s%d.png", asset_root, PLAYER_RUN_BASE_PATH, i + 1);
        player.running_frames[i] = load_texture(renderer, path_buffer);
        if (!player.running_frames[i]) {
            fprintf(stderr, "Fehler: Running Frame %d konnte nicht geladen werden (%s).\n", i + 1, path_buffer);
            player_cleanup_all_loaded(&player); 
            return failed_player; 
        }
    }
    
    // attack sprites
    for (int i = 0; i < ATTACK_FRAME_COUNT; i++) {
        sprintf(path_buffer, "%s%s%d.png", asset_root, PLAYER_ATTACK_BASE_PATH, i + 1);
        player.attack_frames[i] = load_texture(renderer, path_buffer);
        if (!player.attack_frames[i]) {
            fprintf(stderr, "Warnung: Attack Frame %d konnte nicht geladen werden (%s).\n", i + 1, path_buffer);
            player_cleanup_all_loaded(&player); 
            return failed_player;
        }
    }
    
    // hurt sprite
    player.hurt_frame = load_texture(renderer, PLAYER_HURT_PATH);
    if (!player.hurt_frame) {
        fprintf(stderr, "Warnung: Hurt Frame konnte nicht geladen werden (%s).\n", PLAYER_HURT_PATH);
    }
    
    SDL_QueryTexture(player.idle_frames[0], NULL, NULL, &player.sprite_w, &player.sprite_h);
    
    player.rect.w = (int)(player.sprite_w * HIT_BOX_SCALE_W);
    player.rect.h = (int)(player.sprite_h * HIT_BOX_SCALE_H);

    player.offset_x = (player.sprite_w - player.rect.w) / 2;
    player.offset_y = player.sprite_h - player.rect.h; 
    
    int initial_sprite_x = (SCREEN_WIDTH / 2) - (player.sprite_w / 2);
    int initial_sprite_y = (SCREEN_HEIGHT / 2) - (player.sprite_h / 2);

    player.rect.x = initial_sprite_x + player.offset_x;
    player.rect.y = initial_sprite_y + player.offset_y - 10;

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

int player_handle_input(Player *player, const SceCtrlData *pad, int map_width, int map_height) {
    int is_moving = 0;
    player->prev_x = player->rect.x;

    // ... (Attack Timer Checks bleiben gleich) ...

    // --- BEWEGUNG ---
    if (pad->Buttons & PSP_CTRL_LEFT) {
        if (player->rect.x > 0) {
            player->rect.x -= MOVEMENT_SPEED;
            is_moving = 1;
            player->flip_direction = SDL_FLIP_HORIZONTAL;
        }
    }
    if (pad->Buttons & PSP_CTRL_RIGHT) {
        // FEHLER BEHOBEN: Statt SCREEN_WIDTH nutzen wir jetzt map_width
        if (player->rect.x + player->rect.w < map_width) {
            player->rect.x += MOVEMENT_SPEED;
            is_moving = 1;
            player->flip_direction = SDL_FLIP_NONE;
        }
    }
    if (pad->Buttons & PSP_CTRL_UP) {
        if (player->rect.y > 0) {
            player->rect.y -= MOVEMENT_SPEED;
            is_moving = 1;
        }
    }
    if (pad->Buttons & PSP_CTRL_DOWN) {
        // FEHLER BEHOBEN: Statt SCREEN_HEIGHT nutzen wir map_height
        if (player->rect.y + player->rect.h < map_height) {
            player->rect.y += MOVEMENT_SPEED;
            is_moving = 1;
        }
    }

    player->current_x = player->rect.x;
    return is_moving;
}

void player_handle_input_physics(Player *player, const SceCtrlData *pad) {

    // 1. Check: Sind wir gerade mitten in einem Angriff?
    // Wenn ja -> Keine Bewegung erlauben (vel_x = 0) und Funktion verlassen.
    if (SDL_GetTicks() < player->attack_timer_end) {
        player->vel_x = 0;
        return;
    }

    // 2. Check: Angriff starten (Kreis-Taste)
    // Nur möglich, wenn Cooldown abgelaufen ist.
    if ((pad->Buttons & PSP_CTRL_CIRCLE) && SDL_GetTicks() >= player->attack_cooldown_end) {
        player->attack_timer_end = SDL_GetTicks() + ATTACK_DURATION;
        player->attack_cooldown_end = SDL_GetTicks() + ATTACK_COOLDOWN;
        player->current_attack_frame = 0;

        // Sofort stehenbleiben beim Angriffsstart
        player->vel_x = 0;
        return;
    }

    // --- Ab hier: Normale Bewegung (nur wenn wir NICHT angreifen) ---

    // X-Bewegung zurücksetzen (damit wir nicht rutschen)
    player->vel_x = 0;

    if (pad->Buttons & PSP_CTRL_LEFT) {
        player->vel_x = -MOVEMENT_SPEED;
        player->flip_direction = SDL_FLIP_HORIZONTAL;
    }
    if (pad->Buttons & PSP_CTRL_RIGHT) {
        player->vel_x = MOVEMENT_SPEED;
        player->flip_direction = SDL_FLIP_NONE;
    }

    // Springen (Nur wenn am Boden)
    if ((pad->Buttons & PSP_CTRL_CROSS) && player->on_ground) {
        player->vel_y = JUMP_FORCE;
        player->on_ground = 0;
    }

    // Optional: Debug Schaden (L-Trigger) wieder einbauen
    if (pad->Buttons & PSP_CTRL_LTRIGGER) {
        player_decrease_health(player, 1);
    }
}

void player_update_physics(Player *player, struct Map *map) {
    // 1. Schwerkraft anwenden
    player->vel_y += GRAVITY;
    if (player->vel_y > MAX_FALL_SPEED) player->vel_y = MAX_FALL_SPEED;

    // --- Y-ACHSE (Vertikale Kollision) ---
    player->rect.y += (int)player->vel_y;

    // Wir prüfen zwei Punkte am Boden: Linker Fuß und Rechter Fuß
    // +2 und -2 sorgen dafür, dass wir nicht an Wänden hängenbleiben
    int foot_y = player->rect.y + player->rect.h;
    int left_foot_x = player->rect.x + 2;
    int right_foot_x = player->rect.x + player->rect.w - 2;

    // Prüfen ob wir in den Boden fallen (nach unten bewegen)
    if (player->vel_y > 0) {
        if (map_is_solid(map, left_foot_x, foot_y) || map_is_solid(map, right_foot_x, foot_y)) {
            // Kollision!
            // Snap to grid: Setze Spieler exakt auf die Kante des Tiles
            // (foot_y / 16) * 16 ist die Oberkante des Tiles, in dem wir stecken
            int tile_top = (foot_y / 16) * 16;
            player->rect.y = tile_top - player->rect.h;

            player->vel_y = 0;
            player->on_ground = 1;
        } else {
            player->on_ground = 0;
        }
    }
    // Optional: Kopf-Kollision (Decke) hier hinzufügen (vel_y < 0 prüfen)


    // --- X-ACHSE (Horizontale Kollision) ---
    player->rect.x += (int)player->vel_x;

    // Prüfen ob wir gegen eine Wand laufen
    // Wir testen die Mitte der Körperhöhe, links und rechts
    int center_y = player->rect.y + (player->rect.h / 2);

    if (player->vel_x > 0) { // Nach Rechts
        int right_side = player->rect.x + player->rect.w;
        if (map_is_solid(map, right_side, center_y)) {
            // Zurücksetzen an Kante
            int tile_left = (right_side / 16) * 16;
            player->rect.x = tile_left - player->rect.w;
        }
    }
    else if (player->vel_x < 0) { // Nach Links
        int left_side = player->rect.x;
        if (map_is_solid(map, left_side, center_y)) {
            // Zurücksetzen an Kante (+16 um an rechte Seite des linken Tiles zu kommen)
            int tile_right = ((left_side / 16) * 16) + 16;
            player->rect.x = tile_right;
        }
    }
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
        
        // Attack-Hitbox setzen (nur während des aktiven Frames, um Hit-Spam zu verhindern)
        if (player->current_attack_frame == 1) { 
            // Position der Hitbox relativ zur Player-Hitbox
            if (player->flip_direction == SDL_FLIP_NONE) { // Schaut nach rechts
                // NEUE LOGIK: Beginnt 15px VOR der rechten Kante der Spieler-Hitbox, um Überlappung zu erzeugen
                player->attack_rect.x = player->rect.x + player->rect.w - ATTACK_OVERLAP; 
            } else { // Schaut nach links
                player->attack_rect.x = (player->rect.x + ATTACK_OVERLAP) - ATTACK_HITBOX_WIDTH; 
            }
            
            player->attack_rect.y = player->rect.y + player->rect.h / 2 - ATTACK_HITBOX_HEIGHT / 2 + ATTACK_HITBOX_OFFSET_Y;
            player->attack_rect.w = ATTACK_HITBOX_WIDTH;
            player->attack_rect.h = ATTACK_HITBOX_HEIGHT;
        } else {
             // Hitbox zurücksetzen, wenn nicht im aktiven Frame
            player->attack_rect = (SDL_Rect){0, 0, 0, 0};
        }
        
    } else {
        // Angriff ist vorbei
        player->attack_rect = (SDL_Rect){0, 0, 0, 0};
    }
}

void player_update_animation(Player *player, int is_moving) {
    if (SDL_GetTicks() < player->attack_timer_end) {
        return; 
    }

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
    if (SDL_GetTicks() < player->hurt_timer_end) {
        return;
    }
    
    player->health -= amount;
    player->hurt_timer_end = SDL_GetTicks() + HURT_DURATION;
    
    if (player->health < 0) {
        player->health = 0;
        // Handle Death
    }
}

void player_render(SDL_Renderer *renderer, Player *player, int is_moving, int camera_x, int camera_y) {
    SDL_Texture *current_texture = NULL;
    Uint32 current_ticks = SDL_GetTicks();

    SDL_Texture *base_texture = NULL;

    // 1. Determine which texture to use (Attack, Hurt, Run, or Idle)
    if (current_ticks < player->attack_timer_end) {
        base_texture = player->attack_frames[player->current_attack_frame];
    }
    else if (current_ticks < player->hurt_timer_end) {
        // If hurting, prioritize hurt frame, otherwise fall back to move/idle
        base_texture = player->hurt_frame ? player->hurt_frame :
                       (is_moving ? player->running_frames[player->current_run_frame] :
                        player->idle_frames[player->current_idle_frame]);
    } else {
        base_texture = is_moving ?
                       player->running_frames[player->current_run_frame] :
                       player->idle_frames[player->current_idle_frame];
    }

    if (!base_texture) return;

    // 2. Calculate Screen Position (World Pos - Camera Pos)
    SDL_Rect render_rect = {
            (player->rect.x - player->offset_x) - camera_x, // Subtract Camera X
            (player->rect.y - player->offset_y) - camera_y, // Subtract Camera Y
            player->sprite_w,
            player->sprite_h
    };

    // 3. Reset Texture Color Mod
    SDL_SetTextureColorMod(base_texture, 255, 255, 255);

    // 4. Handle Hurt Flash (Red/White)
    if (current_ticks < player->hurt_timer_end) {
        if (current_ticks % 100 < 50) {
            SDL_SetTextureColorMod(base_texture, 255, 100, 100); // Red tint
        } else {
            SDL_SetTextureColorMod(base_texture, 255, 255, 255); // Normal
        }
    }

    current_texture = base_texture;

    // 5. Draw the Sprite
    if (current_texture) {
        SDL_RenderCopyEx(
                renderer,
                current_texture,
                NULL,
                &render_rect,
                0.0,
                NULL,
                player->flip_direction
        );
    }

    // --- 6. DEBUG: Draw Hitboxes (Adjusted for Camera) ---
    SDL_Color original_color;
    SDL_GetRenderDrawColor(renderer, &original_color.r, &original_color.g, &original_color.b, &original_color.a);

    // Create a temporary rect for the Player Hitbox relative to the screen
    SDL_Rect screen_hitbox = player->rect;
    screen_hitbox.x -= camera_x;
    screen_hitbox.y -= camera_y;

    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // Blue
    SDL_RenderDrawRect(renderer, &screen_hitbox);

    // Create a temporary rect for the Attack Hitbox relative to the screen
    if (player->attack_rect.w > 0) {
        SDL_Rect screen_attack_rect = player->attack_rect;
        screen_attack_rect.x -= camera_x;
        screen_attack_rect.y -= camera_y;

        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Green
        SDL_RenderDrawRect(renderer, &screen_attack_rect);
    }

    // Restore original draw color
    SDL_SetRenderDrawColor(renderer, original_color.r, original_color.g, original_color.b, original_color.a);
}

void player_cleanup(Player *player) {
    player_cleanup_all_loaded(player);
}