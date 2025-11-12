#include "player.h"
#include <SDL_image.h>
#include <stdio.h>
#include <stdlib.h>

// --- HILFSFUNKTION (Muss hier oder in einer separaten utils.c definiert werden) ---
extern SDL_Texture *load_texture(SDL_Renderer *renderer, const char *path); 
// WICHTIG: Wenn load_texture in main.c ist, muss es hier als extern deklariert werden. 
// Besser wäre es, load_texture auch in die Player-Logik zu integrieren. 
// In diesem Beispiel gehen wir davon aus, dass Sie es extern lassen.

Player player_init(SDL_Renderer *renderer) {
    Player player = {0};

    // idle sprites
    const char *idle_base_path = "resources/Gothicvania Collection Files/Assets/Characters/PLAYABLE CHARACTERS/Cemetery Hero/Base/Sprites/hero-idle/hero-idle-";
    char path_buffer[256];
    
    for (int i = 0; i < IDLE_FRAME_COUNT; i++) {
        sprintf(path_buffer, "%s%d.png", idle_base_path, i + 1); 
        player.idle_frames[i] = load_texture(renderer, path_buffer);
        if (!player.idle_frames[i]) {
            fprintf(stderr, "Fehler: Idle Frame %d konnte nicht geladen werden.\n", i + 1);
            player_cleanup(&player); // Aufräumen, falls Fehler
            exit(1);
        }
    }

    // run sprites
    const char *running_base_path = "resources/Gothicvania Collection Files/Assets/Characters/PLAYABLE CHARACTERS/Cemetery Hero/Base/Sprites/hero-run/hero-run-";

    for (int i = 0; i < RUN_FRAME_COUNT; i++) {
        sprintf(path_buffer, "%s%d.png", running_base_path, i + 1);
        player.running_frames[i] = load_texture(renderer, path_buffer);
        if (!player.running_frames[i]) {
            fprintf(stderr, "Fehler: Running Frame %d konnte nicht geladen werden.\n", i + 1);
            player_cleanup(&player); // Aufräumen, falls Fehler
            exit(1);
        }
    }

    // --- 3. Initialposition und Dimensionen setzen ---
    SDL_QueryTexture(player.idle_frames[0], NULL, NULL, &player.rect.w, &player.rect.h);
    player.rect.x = (SCREEN_WIDTH / 2) - (player.rect.w / 2);
    player.rect.y = (SCREEN_HEIGHT / 2) - (player.rect.h / 2);
    
    // 4. Standardwerte
    player.last_time = SDL_GetTicks();
    player.flip_direction = SDL_FLIP_NONE; 

    return player;
}

int player_handle_input(Player *player, const SceCtrlData *pad) {
    int is_moving = 0;

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
    
    return is_moving;
}

void player_update_animation(Player *player, int is_moving) {
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

void player_render(SDL_Renderer *renderer, Player *player, int is_moving) {
    SDL_Texture *current_texture = NULL;
    
    if (is_moving) {
        current_texture = player->running_frames[player->current_run_frame];
    } else {
        current_texture = player->idle_frames[player->current_idle_frame];
    }

    if (current_texture) {
        SDL_RenderCopyEx(
            renderer, 
            current_texture, 
            NULL, 
            &player->rect, 
            0.0,         
            NULL,        
            player->flip_direction 
        );
    }
}

void player_cleanup(Player *player) {
    for (int i = 0; i < IDLE_FRAME_COUNT; i++) {
        if (player->idle_frames[i]) SDL_DestroyTexture(player->idle_frames[i]);
    }
    for (int i = 0; i < RUN_FRAME_COUNT; i++) {
        if (player->running_frames[i]) SDL_DestroyTexture(player->running_frames[i]);
    }
}