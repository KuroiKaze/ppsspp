#ifndef UI_H
#define UI_H

#include <SDL.h>

/**
 * Zeichnet die Health Bar des Spielers.
 * * @param renderer Der SDL_Renderer.
 * @param current_health Die aktuelle Gesundheit (0-100).
 */
void ui_render_health_bar(SDL_Renderer *renderer, int current_health);

#endif // UI_H