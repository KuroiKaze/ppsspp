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

void ui_render_inventory(SDL_Renderer *renderer, Item inventory[], int count) {
    int slot_size = 32; 
    int padding = 8;
    int start_x = UI_BAR_X;
    int start_y = UI_BAR_Y + UI_BAR_H + 15;

    for (int i = 0; i < count; i++) {
        SDL_Rect slot_rect = { start_x + (slot_size + padding) * i, start_y, slot_size, slot_size };

        // Hintergrund & Icon
        SDL_SetRenderDrawColor(renderer, 40, 40, 40, 200);
        SDL_RenderFillRect(renderer, &slot_rect);
        
        if (inventory[i].texture) {
            SDL_RenderCopy(renderer, inventory[i].texture, NULL, &slot_rect);
        }

        // STACK-COUNTER: Kleine gelbe Balken für die Anzahl
        // Wir zeichnen für jedes Item im Stapel einen 4x2 Pixel Block
        for (int n = 0; n < inventory[i].amount; n++) {
            SDL_Rect stack_dot = {
                slot_rect.x + 2 + (n * 5), // Versatz nach rechts
                slot_rect.y + slot_size - 6, // Am unteren Rand des Slots
                4, 2
            };
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // Gelb
            SDL_RenderFillRect(renderer, &stack_dot);
        }

        // Rahmen
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &slot_rect);
    }
}