#ifndef CHEST_H
#define CHEST_H

#include <SDL.h>
#include <stdbool.h>
#include "../entity/spriteFramesArray.h"

typedef struct {
    SDL_Rect rect;
    bool collected;           // Bereits eingesammelt
    bool opening;             // Animation läuft
    Uint32 last_frame_time;   // Timer für Animation
    int current_frame;        // Aktuelles Animationsframe
    int frame_delay;          // Zeit pro Frame in ms
    SpriteFrameArray frames;    // Öffnungsanimation
} Chest;

Chest chest_init(SDL_Renderer *renderer, const char *base_path, int x, int y);
void chest_update(Chest *chest);
void chest_render(SDL_Renderer *renderer, Chest *chest, int camera_x, int camera_y);
bool chest_check_collision(Chest *chest, SDL_Rect player_rect);
void chest_destroy(Chest *chest);

#endif