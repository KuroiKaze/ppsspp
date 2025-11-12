#include "ui.h"
#include <SDL.h>

#define UI_BAR_X 15
#define UI_BAR_Y 15
#define UI_BAR_W 200
#define UI_BAR_H 15

void ui_render_health_bar(SDL_Renderer *renderer, int current_health) {
    if (current_health < 0) current_health = 0;
    if (current_health > 100) current_health = 100;

    SDL_Rect background_rect = {
        UI_BAR_X, UI_BAR_Y,
        UI_BAR_W, UI_BAR_H
    };
    
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255); 
    SDL_RenderFillRect(renderer, &background_rect);


    float health_ratio = (float)current_health / 100.0f;
    int health_width = (int)(UI_BAR_W * health_ratio);

    SDL_Rect health_rect = {
        UI_BAR_X, UI_BAR_Y,
        health_width, UI_BAR_H
    };

    Uint8 r, g, b;
    
    if (current_health > 70) {
        // Hoch: Grün
        r = 0; g = 200; b = 0;
    } else if (current_health > 30) {
        // Mittel: Gelb
        r = 200; g = 200; b = 0;
    } else {
        // Niedrig: Rot
        r = 200; g = 0; b = 0;
    }
    
    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
    SDL_RenderFillRect(renderer, &health_rect);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); 
    SDL_RenderDrawRect(renderer, &background_rect);
}