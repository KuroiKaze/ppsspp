#ifndef PLAYER_H
#define PLAYER_H

#include <SDL.h>
#include <pspctrl.h> // Für SceCtrlData

// --- Konstanten ---
#define IDLE_FRAME_COUNT 4
#define RUN_FRAME_COUNT 6
#define ATTACK_FRAME_COUNT 6

#define ANIMATION_SPEED 100 // Millisekunden pro Frame
#define ATTACK_DURATION 300 // Dauer des gesamten Angriffs in ms (z.B. 300ms)
#define ATTACK_COOLDOWN 500 // Cooldown, bevor erneut angegriffen werden kann
#define MOVEMENT_SPEED 3    // Pixel pro Frame
#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 272
#define HURT_DURATION 300 // 300ms I-Frames / Hurt-Animation


struct Map;

// --- Struktur ---
typedef struct {
    SDL_Texture *idle_frames[IDLE_FRAME_COUNT];
    SDL_Texture *running_frames[RUN_FRAME_COUNT];
    SDL_Texture *attack_frames[ATTACK_FRAME_COUNT]; // NEU: Attack-Animation
    SDL_Texture *hurt_frame; 
    
    SDL_Rect rect;        // Die Kollisionsbox (die KLEINERE Hitbox)
    SDL_Rect attack_rect; // NEU: Rechteck für den Angriff (wird nur während der Attacke gesetzt)

    float vel_x; // NEU
    float vel_y; // NEU: Vertikale Geschwindigkeit
    int on_ground; // NEU: Stehen wir auf dem Boden?

    int sprite_w;         // Volle Breite der Textur (zum Rendern)
    int sprite_h;         // Volle Höhe der Textur (zum Rendern)
    int offset_x;         // Horizontaler Offset für die zentrierte Hitbox
    int offset_y;         // Vertikaler Offset für die bodengleiche Hitbox

    int current_idle_frame; 
    int current_run_frame;
    int current_attack_frame; // NEU
    Uint32 last_time; 
    
    int health;
    SDL_RendererFlip flip_direction; 
    Uint32 hurt_timer_end;      
    Uint32 attack_timer_end;    
    Uint32 attack_cooldown_end;
    int prev_x; // Vorherige X-Koordinate der Hitbox
    int current_x; // Aktuelle X-Koordinate der Hitbox
    uint32_t prev_buttons;

} Player;

// --- Funktionsprototypen ---
Player player_init(SDL_Renderer *renderer);
int player_handle_input(Player *player, const SceCtrlData *pad, int map_width, int map_height);
void player_handle_input_physics(Player *player, const SceCtrlData *pad, int map_width, int map_height);
void player_update_animation(Player *player, int is_moving);
void player_update_attack(Player *player); // NEU
void player_decrease_health(Player *player, int amount);
void player_update_physics(Player *player, struct Map *map);
void player_render(SDL_Renderer *renderer, Player *player, int is_moving, int camera_x, int camera_y);
void player_cleanup(Player *player);

#endif // PLAYER_H