#include "chest.h"
#include <SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../entity/entity.h"
#include <unistd.h>

extern SDL_Texture *load_texture(SDL_Renderer *renderer, const char *path);
extern void debug_log(const char *format, ...);


// Prüft, ob Datei existiert
static bool chest_frame_exists(const char *path) {
    return access(path, F_OK) != -1;
}

Chest chest_init(SDL_Renderer *renderer, const char *base_path, int x, int y, Item loot) {
    Chest chest = {0};
    chest.rect.x = x;
    chest.rect.y = y;
    chest.collected = false;
    chest.opening = false;
    chest.current_frame = 0;
    chest.last_frame_time = 0;
    chest.frame_delay = 150; // 300 ms pro Frame

    // Dynamisch Frames zählen
    int count = 0;
    char path[256];
    while (1) {
        snprintf(path, sizeof(path), "%s%d.png", base_path, count + 1);
        if (!chest_frame_exists(path)) break;
        count++;
    }

    chest.frames.count = count;
    chest.frames.frames = malloc(sizeof(SDL_Texture*) * count);

    for (int i = 0; i < count; i++) {
        snprintf(path, sizeof(path), "%s%d.png", base_path, i + 1);
        chest.frames.frames[i] = load_texture(renderer, path);
        if (!chest.frames.frames[i]) {
            debug_log("Chest frame load failed: %s", path);
        }
    }

    if (chest.frames.frames[0]) SDL_QueryTexture(chest.frames.frames[0], NULL, NULL, &chest.rect.w, &chest.rect.h);

    chest.loot = item_init(renderer, loot, x, y);

    return chest;
}

void chest_update(Chest *chest) {
    if (!chest || chest->collected) return;
    if (!chest->opening) return;

    Uint32 now = SDL_GetTicks();

    if (now - chest->last_frame_time >= chest->frame_delay) {
        chest->last_frame_time = now;   
        chest->current_frame++;          

        if (chest->current_frame >= chest->frames.count) {
            chest->current_frame = chest->frames.count - 1; 
            chest->collected = true;                     
        }
    }
}

void chest_render(SDL_Renderer *renderer, Chest *chest, int camera_x, int camera_y) {
    if (!chest || chest->collected) return;
    SDL_Texture *tex = chest->frames.frames[chest->current_frame];
    SDL_Rect dst = {chest->rect.x - camera_x, chest->rect.y - camera_y, chest->rect.w * 1.5, chest->rect.h * 1.5};
    SDL_RenderCopy(renderer, tex, NULL, &dst);
}

bool chest_check_collision(Chest *chest, SDL_Rect player_rect) {
    if (!chest || chest->collected) return false;
    return SDL_HasIntersection(&chest->rect, &player_rect);
}

void chest_cleanup(Chest *chest) {
    if (!chest) return;
    for (int i = 0; i < chest->frames.count; i++) {
        SDL_DestroyTexture(chest->frames.frames[i]);
    }
    free(chest->frames.frames);
    chest->frames.frames = NULL;
    chest->frames.count = 0;
}

void add_loot_to_player(Chest *chest, Player *player) {
    if (!chest || !player) return;

    // found in inventory, just stack
    for (int i = 0; i < player->inventory_count; i++) {
        if (player->inventory[i].type == chest->loot.type) {
            player->inventory[i].amount += chest->loot.amount;
            printf("Item gestapelt! Anzahl: %d\n", player->inventory[i].amount);
            return;
        }
    }

    // not found, add new item if there's space
    if (player->inventory_count < MAX_INVENTORY) {
        player->inventory[player->inventory_count] = chest->loot;
        player->inventory[player->inventory_count].amount = chest->loot.amount;
        player->inventory_count++;
        chest->collected = true;
    }
}