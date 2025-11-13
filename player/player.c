#include "player.h"
#include <SDL_image.h>
#include <stdio.h>
#include <stdlib.h>

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
    
    // idle sprites
    for (int i = 0; i < IDLE_FRAME_COUNT; i++) {
        sprintf(path_buffer, "%s%d.png", PLAYER_IDLE_BASE_PATH, i + 1); 
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
        sprintf(path_buffer, "%s%d.png", PLAYER_RUN_BASE_PATH, i + 1);
        player.running_frames[i] = load_texture(renderer, path_buffer);
        if (!player.running_frames[i]) {
            fprintf(stderr, "Fehler: Running Frame %d konnte nicht geladen werden (%s).\n", i + 1, path_buffer);
            player_cleanup_all_loaded(&player); 
            return failed_player; 
        }
    }
    
    // attack sprites
    for (int i = 0; i < ATTACK_FRAME_COUNT; i++) {
        sprintf(path_buffer, "%s%d.png", PLAYER_ATTACK_BASE_PATH, i + 1);
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
    player.rect.y = initial_sprite_y + player.offset_y;
    
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

int player_handle_input(Player *player, const SceCtrlData *pad) {
    int is_moving = 0;
    player->prev_x = player->rect.x;

    if (SDL_GetTicks() < player->attack_timer_end) {
        player->current_x = player->rect.x;
        return 0; 
    }

    // NEU: Attacke (Kreis-Taste)
    if ((pad->Buttons & PSP_CTRL_CIRCLE) && SDL_GetTicks() >= player->attack_cooldown_end) {
        player->attack_timer_end = SDL_GetTicks() + ATTACK_DURATION;
        player->attack_cooldown_end = SDL_GetTicks() + ATTACK_COOLDOWN;
        player->current_attack_frame = 0;
        return 0; 
    }

    if (pad->Buttons & PSP_CTRL_LEFT) {
       if (player->rect.x > 0) {
           player->rect.x -= MOVEMENT_SPEED;
           is_moving = 1;
           player->flip_direction = SDL_FLIP_HORIZONTAL;
       }
    }
    if (pad->Buttons & PSP_CTRL_RIGHT) {
       if (player->rect.x + player->rect.w < SCREEN_WIDTH) {
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
       if (player->rect.y + player->rect.h < SCREEN_HEIGHT) {
           player->rect.y += MOVEMENT_SPEED;
           is_moving = 1;
       }
    }

    if (pad->Buttons & PSP_CTRL_LTRIGGER) {
       player_decrease_health(player, 10);
    }
    
    player->current_x = player->rect.x;
    return is_moving;
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

void player_render(SDL_Renderer *renderer, Player *player, int is_moving) {
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
        player->rect.x - player->offset_x, 
        player->rect.y - player->offset_y, 
        player->sprite_w,                  
        player->sprite_h                   
    };


    // 3. **WICHTIG:** Alle Texturen auf Normal zurücksetzen 
    if (base_texture) SDL_SetTextureColorMod(base_texture, 255, 255, 255);

    // 4. **Färben:** Nur die BASE_TEXTURE färben, falls Hurt-Timer aktiv
    if (current_ticks < player->hurt_timer_end) {
        if (current_ticks % 100 < 50) { 
            // 50ms Rot
            SDL_SetTextureColorMod(base_texture, 255, 100, 100);
        } else {
            // 50ms Normal (Weiß)
            SDL_SetTextureColorMod(base_texture, 255, 255, 255);
        }
    }
    
    current_texture = base_texture;
    
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

    // --- 6. DEBUG: Blaue Box um die Kollisionsbox (Hitbox) zeichnen ---
    SDL_Color original_color;
    SDL_GetRenderDrawColor(renderer, &original_color.r, &original_color.g, &original_color.b, &original_color.a);

    // Zeichne Player Hitbox (Blau)
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); 
    SDL_RenderDrawRect(renderer, &player->rect);
    
    // NEU: Zeichne Attack Hitbox (Grün) - sollte jetzt näher am Spieler sein
    if (player->attack_rect.w > 0) {
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); 
        SDL_RenderDrawRect(renderer, &player->attack_rect);
    }
    
    // Setzt die Farbe zurück
    SDL_SetRenderDrawColor(renderer, original_color.r, original_color.g, original_color.b, original_color.a);
}

void player_cleanup(Player *player) {
    player_cleanup_all_loaded(player);
}