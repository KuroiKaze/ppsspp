#include "entity.h"
#include "../map/map.h"
#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <unistd.h>

extern int map_get_floor_height(struct Map *map, int x, int y);
extern int map_is_solid(struct Map *map, int x, int y);
extern SDL_Texture *load_texture(SDL_Renderer *renderer, const char *path);
extern void debug_log(const char *format, ...);

// ------------------------
// Frames (Unchanged)
// ------------------------
bool entity_frame_exists(const char *filepathname) {
    return access(filepathname, F_OK) != -1;
}

int entity_load_frames(SDL_Renderer *renderer, SpriteFrameArray *out, const char *base_path) {
    char path[256];
    int count = 0;

    out->frames = NULL;
    out->count  = 0;
    out->sprite_w = 0;
    out->sprite_h = 0;

    for (int i = 1;; i++) {
        snprintf(path, sizeof(path), "%s%d.png", base_path, i);
        if (!entity_frame_exists(path)) break;
        SDL_Texture *tex = load_texture(renderer, path);
        if (!tex) break;
        if (count == 0) SDL_QueryTexture(tex, NULL, NULL, &out->sprite_w, &out->sprite_h);
        SDL_Texture **tmp = realloc(out->frames, sizeof(SDL_Texture*) * (count + 1));
        if (!tmp) { SDL_DestroyTexture(tex); break; }
        out->frames = tmp;
        out->frames[count++] = tex;
    }
    out->count = count;
    return count;
}

int entity_load_frame(SDL_Renderer *renderer, SpriteFrameArray *out, const char *filepathname) {
    SDL_Texture *tex = load_texture(renderer, filepathname);
    if (!tex) return 0;
    out->frames = malloc(sizeof(SDL_Texture*));
    if (!out->frames) { SDL_DestroyTexture(tex); return 0; }
    out->frames[0] = tex;
    out->count = 1;
    SDL_QueryTexture(tex, NULL, NULL, &out->sprite_w, &out->sprite_h);
    return 1;
}

void entity_free_frames(SpriteFrameArray *a){
    for(int i=0;i<a->count;i++) SDL_DestroyTexture(a->frames[i]);
    free(a->frames);
    a->frames = NULL;
    a->count = 0;
}

void entity_cleanup(Entity *e){
    entity_free_frames(&e->idle);
    entity_free_frames(&e->run);
    entity_free_frames(&e->attack);
    entity_free_frames(&e->hurt);
    entity_free_frames(&e->jump);
    sfx_cleanup(e->attack_sfx);
    sfx_cleanup(e->grunt_sfx);
}


void entity_update_physics(Entity *e, Map *map, float gravity, float max_fall_speed) {
    // ============================================================
    // 1. X-AXIS MOVEMENT
    // ============================================================
    int original_x = e->rect.x;
    int next_x = e->rect.x + (int)e->vel_x;

    // We tentatively apply the move
    e->rect.x = next_x;

    // Check HEAD collision (Ceiling / Low Overhangs)
    // If the head is inside a solid block, we stop immediately.
    // We check the leading edge + offset to avoid clipping.
    int direction = (e->vel_x > 0) ? 1 : -1;
    int lead_x = (direction > 0) ? e->rect.x + e->rect.w : e->rect.x;

    if (map_is_solid(map, lead_x, e->rect.y)) {
        // Head hit a wall -> Revert X
        e->rect.x = original_x;
        e->vel_x = 0;
    }
    else {
        // FEET / SLOPE COLLISION
        int feet_y = e->rect.y + e->rect.h;
        int center_x = e->rect.x + (e->rect.w / 2);

        // Check floor height at the NEW center position
        int new_floor = map_get_floor_height(map, center_x, feet_y);

        if (new_floor != -1) {
            int diff = feet_y - new_floor;
            // Positive = Floor is higher (Step Up / Slope Up)
            // Negative = Floor is lower (Step Down / Slope Down)

            // TOLERANCE:
            // 24px allows climbing 16px slopes + 8px of gravity/margin.
            // Any step higher than 24px is treated as a WALL.
            if (diff > 24) {
                // Wall is too steep -> Revert X
                e->rect.x = original_x;
                e->vel_x = 0;
            }
                // If it is a walkable slope (Up or Down within limit)
            else if (abs(diff) <= 24) {
                // If we are on the ground (or close enough to snap), adjust Y
                if (e->on_ground) {
                    e->rect.y = new_floor - e->rect.h;
                    e->vel_y = 0;
                }
            }
            // If diff < -24 (Falling off a cliff), we do nothing here.
            // Gravity in Step 2 will handle the fall.
        }
    }

    // ============================================================
    // 2. Y-AXIS MOVEMENT
    // ============================================================
    e->vel_y += gravity;
    if(e->vel_y > max_fall_speed) e->vel_y = max_fall_speed;

    if (e->vel_y < 0) {
        // --- JUMPING ---
        int next_y = e->rect.y + (int)e->vel_y;

        // Check Head corners
        if (map_is_solid(map, e->rect.x, next_y) ||
            map_is_solid(map, e->rect.x + e->rect.w, next_y)) {
            e->vel_y = 0;
            e->rect.y = ((e->rect.y / 16) + 1) * 16; // Snap down
        } else {
            e->rect.y = next_y;
        }
        e->on_ground = 0;
    }
    else {
        // --- FALLING / STICKY ---

        // Sticky Logic: Before moving down, check if we can stick to floor
        int center_x = e->rect.x + e->rect.w/2;
        int feet_y   = e->rect.y + e->rect.h;
        int current_floor = map_get_floor_height(map, center_x, feet_y);
        int snapped = 0;

        // If grounded and floor is close (<= 10px), stick to it
        // 10px handles steep descents (45 degrees) better than 6px
        if (e->on_ground && current_floor != -1) {
            if (abs(current_floor - feet_y) <= 10) {
                e->rect.y = current_floor - e->rect.h;
                e->vel_y = 0;
                snapped = 1;
            }
        }

        if (!snapped) {
            e->rect.y += (int)e->vel_y;

            // Landing Logic
            int new_feet_y = e->rect.y + e->rect.h;
            int final_floor = map_get_floor_height(map, center_x, new_feet_y);

            if (final_floor != -1 && new_feet_y >= final_floor - 2) {
                e->rect.y = final_floor - e->rect.h;
                e->vel_y = 0;
                e->on_ground = 1;
            } else {
                e->on_ground = 0;
            }
        }
    }
}void entity_update_animation(Entity *e, int is_moving, Uint32 speed) {
    Uint32 now = SDL_GetTicks();
    if (now - e->last_time < speed) return;
    e->last_time = now;

    if (!e->on_ground && e->jump.count > 0) {
        e->current_jump_frame++;
        if (e->current_jump_frame >= e->jump.count)
            e->current_jump_frame = e->jump.count - 1;
        return;
    } else {
        e->current_jump_frame = 0;
    }

    if (is_moving && e->run.count > 0) {
        e->current_run_frame = (e->current_run_frame + 1) % e->run.count;
    } else if (!is_moving && e->idle.count > 0) {
        e->current_idle_frame = (e->current_idle_frame + 1) % e->idle.count;
    }
}

void entity_render(SDL_Renderer *renderer, Entity *e, SDL_Texture *current_texture, int camera_x, int camera_y) {
    if (!current_texture) return;
    SDL_Rect render_rect = {
            e->rect.x - e->offset_x - camera_x,
            e->rect.y - e->offset_y - camera_y,
            e->sprite_w, e->sprite_h
    };
    SDL_RenderCopyEx(renderer, current_texture, NULL, &render_rect, 0.0, NULL, e->flip_direction);
}

void entity_update_death(Entity *e) {
    Mix_HaltChannel(e->grunt_sfx_channel);
    e->grunt_sfx_channel = -1;
    if (e->death.count == 0) { e->is_dead = 1; return; }
    Uint32 now = SDL_GetTicks();
    if (now - e->death_last_time < 100) return;
    e->death_last_time = now;
    e->current_death_frame++;
    if (e->alpha > 10) e->alpha -= 10; else e->alpha = 0;
    if (e->current_death_frame >= e->death.count) {
        e->is_dead = 1;
        e->is_dying = 0;
    }
}