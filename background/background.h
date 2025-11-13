// background.h

#ifndef BACKGROUND_H
#define BACKGROUND_H

#include <SDL.h>

// Struktur für eine einzelne Hintergrundebene
typedef struct {
    SDL_Texture *texture; // Die Textur des Hintergrundbildes
    float scroll_speed;   // Geschwindigkeit, mit der sich der Layer bewegt (Parallaxe-Effekt)
    float offset_x;       // Aktueller Scroll-Offset
} BackgroundLayer;

BackgroundLayer background_layer_init(SDL_Renderer *renderer, const char *path, float speed);

void background_layer_update(BackgroundLayer *layer, int player_movement_x);
void background_layer_render(SDL_Renderer *renderer, const BackgroundLayer *layer, int screen_width, int screen_height);
void background_layer_cleanup(BackgroundLayer *layer);

#endif // BACKGROUND_H