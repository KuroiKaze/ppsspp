// background.c
#include "background.h"
#include <SDL_image.h>

extern SDL_Texture *load_texture(SDL_Renderer *renderer, const char *path);

BackgroundLayer background_layer_init(SDL_Renderer *renderer, const char *path, float speed) {
    BackgroundLayer layer = {0};
    layer.texture = load_texture(renderer, path);
    layer.scroll_speed = speed;
    return layer;
}

void background_layer_render(SDL_Renderer *renderer, const BackgroundLayer *layer, int camera_x, int camera_y, int screen_width, int screen_height) {
    if (!layer->texture) return;

    int w, h;
    SDL_QueryTexture(layer->texture, NULL, NULL, &w, &h);

    // Safety check to prevent infinite loops if texture is 0 width
    if (w <= 0) return;

    // --- 1. Calculate Horizontal Offset ---
    float parallax_x = (float)camera_x * layer->scroll_speed;

    // Modulo % keeps the number wrapped within the texture width (0 to w)
    int offset_x = (int)parallax_x % w;

    // Handle negative offsets (if camera moves left past 0)
    if (offset_x < 0) offset_x += w;

    // --- 2. Calculate Vertical Offset (Parallax) ---
    // Multiply by 0.1f or 0.2f to make vertical movement subtle compared to horizontal
    float vertical_speed_factor = 0.1f;
    int offset_y = (int)(camera_y * layer->scroll_speed * vertical_speed_factor);

    // --- 3. Render Loop (The Fix) ---
    // Start drawing from the negative offset (slightly off the left side of screen)
    int current_draw_x = -offset_x;

    // Keep drawing copies to the right until we cover the entire screen width
    while (current_draw_x < screen_width) {

        SDL_Rect dest_rect = {
                current_draw_x,
                -offset_y, // Apply vertical parallax
                w,         // Use actual texture width
                h          // Use actual texture height
        };

        SDL_RenderCopy(renderer, layer->texture, NULL, &dest_rect);

        // Move to the next position
        current_draw_x += w;
    }
}

void background_layer_cleanup(BackgroundLayer *layer) {
    if (layer->texture) {
        SDL_DestroyTexture(layer->texture);
        layer->texture = NULL;
    }
}