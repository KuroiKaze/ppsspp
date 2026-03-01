// background.c
#include "background.h"
#include <SDL_image.h>

extern SDL_Texture *load_texture(SDL_Renderer *renderer, const char *path);
extern void debug_log(const char *format, ...);

BackgroundLayer background_layer_init(SDL_Renderer *renderer, const char *path, float speed, float scale) {
    BackgroundLayer layer = {0};
    layer.texture = load_texture(renderer, path);

    if (!layer.texture) {
        debug_log("BG_ERROR: Konnte Textur nicht laden: %s", path);
    } else {
        int w, h;
        SDL_QueryTexture(layer.texture, NULL, NULL, &w, &h);
        debug_log("BG_SUCCESS: Layer geladen [%s]. Original: %dx%d, Speed: %.2f, Scale: %.2f", 
                  path, w, h, speed, scale);
    }

    layer.scroll_speed = speed;
    layer.scale = scale > 0.0f ? scale : 1.0f;
    return layer;
}

void background_layer_render(SDL_Renderer *renderer, const BackgroundLayer *layer, int camera_x, int camera_y, int screen_width, int screen_height) {
    if (!layer->texture) return;

    int raw_w, raw_h;
    SDL_QueryTexture(layer->texture, NULL, NULL, &raw_w, &raw_h);

    if (raw_w <= 0) return;

    // --- 2. Calculate Scaled Dimensions ---
    int w = (int)(raw_w * layer->scale);
    int h = (int)(raw_h * layer->scale);

    // --- 3. Update Parallax with Scaled Width ---
    float parallax_x = (float)camera_x * layer->scroll_speed;

    // Wrap using the SCALED width
    int offset_x = (int)parallax_x % w;
    if (offset_x < 0) offset_x += w;

    float vertical_speed_factor = 0.1f;
    int offset_y = (int)(camera_y * layer->scroll_speed * vertical_speed_factor);

    static Uint32 last_bg_debug = 0;
    if (SDL_GetTicks() - last_bg_debug > 5000) {
        debug_log("BG_RENDER: Drawing at OffsetX: %d, OffsetY: %d, ScaledW: %d", offset_x, offset_y, w);
        last_bg_debug = SDL_GetTicks();
    }
    
    // --- 4. Render Loop with Scaled Rects ---
    int current_draw_x = -offset_x;

    while (current_draw_x < screen_width) {
        SDL_Rect dest_rect = {
                current_draw_x,
                -offset_y,
                w, // Render with scaled width
                h  // Render with scaled height
        };

        SDL_RenderCopy(renderer, layer->texture, NULL, &dest_rect);

        // Move forward by scaled width
        current_draw_x += w;
    }
}

void background_layer_cleanup(BackgroundLayer *layer) {
    if (layer->texture) {
        debug_log("BG_CLEANUP: Textur freigegeben.");
        SDL_DestroyTexture(layer->texture);
        layer->texture = NULL;
    }
}