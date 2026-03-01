#define CUTE_TILED_IMPLEMENTATION
#include "map.h"
#include <SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void debug_log(const char *format, ...);
extern SDL_Texture *load_texture(SDL_Renderer *renderer, const char *path);

// Helper
char* read_file_to_string(const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        debug_log("FILE_ERROR: Konnte %s nicht oeffnen", path);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = malloc(size + 1);
    if (!buffer) {
        debug_log("MALLOC_ERROR: Kein Speicher fuer JSON-Buffer (%ld Bytes)", size);
        fclose(file);
        return NULL;
    }

    fread(buffer, 1, size, file);
    buffer[size] = '\0';
    fclose(file);
    return buffer;
}

int get_tile_shape(Map* map, int tile_id) {
    tile_id = (tile_id & 0x1FFFFFFF);
    if (tile_id == 0) return SHAPE_EMPTY;

    int rel = tile_id - map->collision_gid_start;

    switch (rel) {
        case 0: return SHAPE_SOLID;
        case 1: return SHAPE_HALF_UP_1;
        case 2: return SHAPE_HALF_UP_2;
        case 3: return SHAPE_SLOPE_45_UP;
        case 4: return SHAPE_PLATFORM;
        case 5: return SHAPE_HALF_DOWN_1;
        case 6: return SHAPE_HALF_DOWN_2;
        case 7: return SHAPE_SLOPE_45_DOWN;
        case 8: return SHAPE_DOOR;

            // [NEW] Spawn Points
        case 9: return SHAPE_ZOMBIE_SPAWN;
        case 10: return SHAPE_SLIME_SPAWN;
        case 11: return SHAPE_PLAYER_SPAWN;
        case 12: return SHAPE_SHURIKENDUDE_SPAWN;
        case 13: return SHAPE_CHEST;

        default: return (rel >= 0 && rel < 50) ? SHAPE_SOLID : SHAPE_EMPTY;
    }
}

int calculate_height(int shape, int ty, int offset_x) {
    int base = ty * 16;
    switch (shape) {
        case SHAPE_SOLID:       return base;
        case SHAPE_PLATFORM:    return base;
        case SHAPE_SLOPE_45_UP: return base + (16 - offset_x);
        case SHAPE_SLOPE_45_DOWN: return base + offset_x;
        case SHAPE_HALF_UP_1:   return base + (16 - (offset_x / 2));
        case SHAPE_HALF_UP_2:   return base + (8 - (offset_x / 2));
        case SHAPE_HALF_DOWN_1: return base + (offset_x / 2);
        case SHAPE_HALF_DOWN_2: return base + 8 + (offset_x / 2);
        default: return -1;
    }
}

int map_init(Map* map, SDL_Renderer* renderer, const char* path, const char** texture_paths, int texture_count) {
    debug_log("--- MAP_INIT START ---");
    debug_log("Pfad: %s", path);

    // 1. JSON laden
    char* json_data = read_file_to_string(path);
    if (!json_data) {
        debug_log("MAP_ABORT: Datei-Lesefehler.");
        return -1;
    }

    // 2. Parsen mit cute_tiled
    map->tiled_map = cute_tiled_load_map_from_memory(json_data, strlen(json_data), NULL);
    free(json_data);

    if (!map->tiled_map) {
        debug_log("MAP_ABORT: cute_tiled Parser-Fehler! (Check JSON Syntax)");
        return -1;
    }
    debug_log("MAP_SUCCESS: JSON geparst. Groesse: %dx%d", map->tiled_map->width, map->tiled_map->height);

    // 3. Tilesets und Texturen verarbeiten
    map->texture_count = 0;
    map->collision_gid_start = 0;
    cute_tiled_tileset_t* ts = map->tiled_map->tilesets;
    int tex_idx = 0;

    while (ts) {
        debug_log("TILESET_CHECK: Name='%s', FirstGID=%d, Count=%d", ts->name.ptr, ts->firstgid, ts->tilecount);

        if (strstr(ts->name.ptr, "collision") || strstr(ts->name.ptr, "Collision")) {
            map->collision_gid_start = ts->firstgid;
            debug_log("COLLISION_GID: Startet bei %d", map->collision_gid_start);
        } else {
            if (tex_idx < texture_count && tex_idx < MAX_TILESETS) {
                debug_log("TEXTURE_LOAD: Index %d, Pfad: %s", tex_idx, texture_paths[tex_idx]);
                
                // SDL_image Load
                SDL_Surface* surf = IMG_Load(texture_paths[tex_idx]);
                if (!surf) {
                    debug_log("IMG_ERROR: %s (Check Pfad/Leerzeichen/ISO!)", IMG_GetError());
                    map->textures[tex_idx] = NULL;
                } else {
                    map->textures[tex_idx] = SDL_CreateTextureFromSurface(renderer, surf);
                    SDL_FreeSurface(surf);
                    if (!map->textures[tex_idx]) {
                        debug_log("SDL_ERROR: Texture Creation failed: %s", SDL_GetError());
                    } else {
                        debug_log("TEXTURE_SUCCESS: Geladen an Index %d", tex_idx);
                    }
                }
                
                map->tileset_firstgids[tex_idx] = ts->firstgid;
                map->texture_count++;
                tex_idx++;
            } else {
                debug_log("TILESET_WARNING: Zu viele Tilesets oder Array-Limit erreicht.");
            }
        }
        ts = ts->next;
    }

    // 4. Collision Layer finden
    map->collision_layer = NULL;
    cute_tiled_layer_t* layer = map->tiled_map->layers;
    while (layer) {
        debug_log("LAYER_SCAN: '%s' (Type: %d)", layer->name.ptr, layer->type);
        if (strcmp(layer->name.ptr, "Collision") == 0) {
            map->collision_layer = layer;
            debug_log("COLLISION_LAYER_FOUND: Data-Pointer: %p", (void*)layer->data);
            break;
        }
        layer = layer->next;
    }

    if (!map->collision_layer) {
        debug_log("MAP_WARNING: Kein 'Collision' Layer gefunden!");
    }

    debug_log("--- MAP_INIT END (Success) ---");
    return 1;
}

int map_is_solid(Map* map, int x, int y) {
    if (!map->collision_layer) return 0;
    int tx = x / 16; int ty = y / 16;
    if (tx < 0 || tx >= map->collision_layer->width || ty < 0 || ty >= map->collision_layer->height) return 0;

    int id = map->collision_layer->data[ty * map->collision_layer->width + tx];
    return (get_tile_shape(map, id) == SHAPE_SOLID); // Pass map
}

int map_get_shape_at(Map *map, int x, int y) {
    if (!map->collision_layer) return SHAPE_EMPTY;
    int tx = x / 16; int ty = y / 16;
    if (tx < 0 || tx >= map->collision_layer->width || ty < 0 || ty >= map->collision_layer->height) return SHAPE_EMPTY;
    int id = map->collision_layer->data[ty * map->collision_layer->width + tx];
    return get_tile_shape(map, id); // Pass map
}

int map_get_floor_height(Map* map, int x, int y) {
    if (!map->collision_layer) return -1;
    int tx = x / 16; int ty = y / 16; int offset_x = x % 16;
    int best_height = -1;

    for (int check_y = ty - 1; check_y <= ty + 1; check_y++) {
        if (check_y < 0 || check_y >= map->collision_layer->height) continue;

        int index = (check_y * map->collision_layer->width) + tx;
        int id = map->collision_layer->data[index];
        int shape = get_tile_shape(map, id); // Pass map

        if (shape == SHAPE_EMPTY) continue;
        int h = calculate_height(shape, check_y, offset_x);

        if (shape >= SHAPE_SLOPE_45_UP && shape <= SHAPE_HALF_DOWN_2) return h;
        if (shape == SHAPE_PLATFORM && y <= h + 4) { if (best_height == -1 || h < best_height) best_height = h; }
        if (shape == SHAPE_SOLID) { if (best_height == -1 || h < best_height) best_height = h; }
    }
    return best_height;
}

void map_render(SDL_Renderer *renderer, Map *map, int camera_x, int camera_y) {
    if (!map->tiled_map) return;
    cute_tiled_layer_t* layer = map->tiled_map->layers;

    while (layer) {
        if (strcmp(layer->name.ptr, "Collision") == 0) { layer = layer->next; continue; }
        if (strcmp(layer->type.ptr, "tilelayer") == 0) {
            int* tiles = layer->data; int count = layer->data_count; int width = layer->width;

            for (int i = 0; i < count; i++) {
                int tile_id = tiles[i]; if (tile_id == 0) continue;

                // Find correct texture based on GID range
                int t_idx = -1;
                for (int t = map->texture_count - 1; t >= 0; t--) {
                    if (tile_id >= map->tileset_firstgids[t]) { t_idx = t; break; }
                }

                if (t_idx == -1 || !map->textures[t_idx]) continue;

                int firstgid = map->tileset_firstgids[t_idx];
                int gid = (tile_id & 0x1FFFFFFF) - firstgid;
                int img_w, img_h; SDL_QueryTexture(map->textures[t_idx], NULL, NULL, &img_w, &img_h);
                int tile_size = 16; int tiles_per_row = img_w / tile_size;

                int px = (i % width) * tile_size; int py = (i / width) * tile_size;
                if (px - camera_x < -tile_size || px - camera_x > 480 || py - camera_y < -tile_size || py - camera_y > 272) continue;

                SDL_Rect src = { (gid % tiles_per_row) * tile_size, (gid / tiles_per_row) * tile_size, tile_size, tile_size };
                SDL_Rect dest = { px - camera_x, py - camera_y, tile_size, tile_size };
                SDL_RenderCopy(renderer, map->textures[t_idx], &src, &dest);
            }
        }
        layer = layer->next;
    }
}

void map_cleanup(Map *map) {
    if (map->tiled_map) cute_tiled_free_map(map->tiled_map);
    for (int i = 0; i < MAX_TILESETS; i++) if (map->textures[i]) SDL_DestroyTexture(map->textures[i]);
}