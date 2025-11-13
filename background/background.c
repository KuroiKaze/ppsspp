// background.c (KORRIGIERT)

#include "background.h"
#include <stdio.h>
#include <SDL_image.h>

extern SDL_Texture *load_texture(SDL_Renderer *renderer, const char *path);

// Initialisiert einen einzelnen Layer
BackgroundLayer background_layer_init(SDL_Renderer *renderer, const char *path, float speed) {
    BackgroundLayer layer = {0};
    layer.texture = load_texture(renderer, path);
    layer.scroll_speed = speed;
    layer.offset_x = 0.0f;
    return layer;
}

// Aktualisiert den Layer-Offset
void background_layer_update(BackgroundLayer *layer, int player_movement_x) {
    // Wenn der Spieler sich nach rechts bewegt (player_movement_x > 0), 
    // muss der Hintergrund nach links scrollen, um den Effekt zu erzeugen, 
    // dass der Spieler in der Mitte bleibt. Daher das MINUS-Zeichen.
    layer->offset_x -= (float)player_movement_x * layer->scroll_speed;
    int w, h;
    if (layer->texture) {
        SDL_QueryTexture(layer->texture, NULL, NULL, &w, &h);
    
        // Sicherstellen, dass der Offset innerhalb des Bereichs [-w, w] bleibt (für nahtloses Scrolling)
        if (layer->offset_x < -w) {
            layer->offset_x += w;
        } else if (layer->offset_x > w) {
            layer->offset_x -= w;
        }
    }
}

// Rendert den Layer (doppelt gerendert für unendliches Scrollen)
void background_layer_render(SDL_Renderer *renderer, const BackgroundLayer *layer, int screen_width, int screen_height) {
    if (!layer->texture) return;

    int w, h;
    SDL_QueryTexture(layer->texture, NULL, NULL, &w, &h);
    
    int scaled_w = screen_width; 

    int current_offset = (int)layer->offset_x; 

    SDL_Rect dst_rect1 = {current_offset, 0, scaled_w, screen_height};
    SDL_RenderCopy(renderer, layer->texture, NULL, &dst_rect1);

    SDL_Rect dst_rect2;
    if (current_offset > 0) {
        dst_rect2 = (SDL_Rect){current_offset - scaled_w, 0, scaled_w, screen_height};
    } else {
        dst_rect2 = (SDL_Rect){current_offset + scaled_w, 0, scaled_w, screen_height};
    }
    
    SDL_RenderCopy(renderer, layer->texture, NULL, &dst_rect2);
}

void background_layer_cleanup(BackgroundLayer *layer) {
    if (layer->texture) {
        SDL_DestroyTexture(layer->texture);
        layer->texture = NULL;
    }
}