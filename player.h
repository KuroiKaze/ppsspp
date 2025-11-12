#ifndef PLAYER_H
#define PLAYER_H

#include <SDL.h>
#include <pspctrl.h>

// --- KONSTANTEN ---
#define IDLE_FRAME_COUNT 4
#define RUN_FRAME_COUNT 6
#define ANIMATION_SPEED 100 
#define MOVEMENT_SPEED 3
#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 272

// --- DATENSTRUKTUR FÜR DEN SPIELER ---
typedef struct {
    SDL_Rect rect;                  // Position und Größe des Spielers
    SDL_Texture *idle_frames[IDLE_FRAME_COUNT]; // Idle Animation Texturen
    SDL_Texture *running_frames[RUN_FRAME_COUNT]; // Run Animation Texturen
    Uint32 last_time;               // Letzte Animationszeit
    int current_idle_frame;         
    int current_run_frame;          
    SDL_RendererFlip flip_direction; // Blickrichtung
} Player;

// --- FUNKTIONSPROTOTYPEN ---

/**
 * Initialisiert die Player-Struktur, lädt alle Texturen und setzt die Startposition.
 * @param renderer Der SDL_Renderer.
 * @return Eine initialisierte Player-Struktur.
 */
Player player_init(SDL_Renderer *renderer);

/**
 * Verarbeitet die PSP-Controllereingabe und aktualisiert Position, Richtung und Zustand (moving).
 * @param player Pointer auf die Player-Struktur.
 * @param pad Die SceCtrlData-Struktur des Controllers.
 * @return 1, wenn sich der Spieler bewegt; ansonsten 0.
 */
int player_handle_input(Player *player, const SceCtrlData *pad);

/**
 * Aktualisiert die Animationsframes basierend auf der Zeit und dem Bewegungszustand.
 * @param player Pointer auf die Player-Struktur.
 * @param is_moving Zustand, ob sich der Spieler bewegt.
 */
void player_update_animation(Player *player, int is_moving);

/**
 * Rendert den Player auf dem Bildschirm (Idle oder Run, gespiegelt oder ungespiegelt).
 * @param renderer Der SDL_Renderer.
 * @param player Pointer auf die Player-Struktur.
 * @param is_moving Zustand, ob sich der Spieler bewegt.
 */
void player_render(SDL_Renderer *renderer, Player *player, int is_moving);

/**
 * Gibt den gesamten vom Player belegten Speicher (Texturen) frei.
 * @param player Pointer auf die Player-Struktur.
 */
void player_cleanup(Player *player);

#endif // PLAYER_H