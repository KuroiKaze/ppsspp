#ifndef UI_H
#define UI_H

#include <SDL.h>
#include "../player/player.h"

/**
 * Zeichnet die Health Bar des Spielers.
 * * @param renderer Der SDL_Renderer.
 * @param current_health Die aktuelle Gesundheit (0-100).
 */

void ui_render_health_bar(SDL_Renderer *renderer, int current_health);
void ui_render_inventory(SDL_Renderer *renderer, Item inventory[], int count);
#endif // UI_H