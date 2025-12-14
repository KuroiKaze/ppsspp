#ifndef ENTITY_H
#define ENTITY_H

#include <SDL.h>
#include "spriteFramesArray.h"

typedef struct {
    SDL_Rect rect;
    float vel_x, vel_y;
    int on_ground;

    SDL_Texture *current_texture;

    int current_idle_frame;
    int current_run_frame;
    int current_jump_frame;
    int current_attack_frame;

    Uint32 last_time;
    SDL_RendererFlip flip_direction;

    int offset_x, offset_y;
    int sprite_w, sprite_h;
    int health;
    float movement_speed;

    // Animationen
    SpriteFrameArray idle;
    SpriteFrameArray run;
    SpriteFrameArray jump;
    SpriteFrameArray attack;
    SpriteFrameArray hurt;

} Entity;

struct Map;

bool entity_frame_exists(const char *filepathname);
int entity_load_frames(SDL_Renderer *renderer, SpriteFrameArray *out, const char *base_path);
int entity_load_frame(SDL_Renderer *renderer, SpriteFrameArray *out, const char *base_path);
void entity_free_frames(SpriteFrameArray *a);
void entity_cleanup(Entity *e);
void entity_update_physics(Entity *e, struct Map *map, float gravity, float max_fall_speed);
void entity_update_animation(Entity *e, int is_moving, Uint32 animation_speed);
void entity_render(SDL_Renderer *renderer, Entity *e, SDL_Texture *current_texture, int camera_x, int camera_y);
#endif