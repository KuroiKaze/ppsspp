#ifndef BACKGROUND_H
#define BACKGROUND_H

#include <SDL.h>

typedef struct {
    SDL_Texture *texture;
    float scroll_speed;
    float scale;
} BackgroundLayer;

BackgroundLayer background_layer_init(SDL_Renderer *renderer, const char *path, float speed, float scale);

void background_layer_render(SDL_Renderer *renderer, const BackgroundLayer *layer, int camera_x, int camera_y, int screen_width, int screen_height);

void background_layer_cleanup(BackgroundLayer *layer);

#endif