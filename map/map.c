#define CUTE_TILED_IMPLEMENTATION
#include "map.h"
#include <SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- FORMEN ---
#define SHAPE_EMPTY 0
#define SHAPE_SOLID 1
#define SHAPE_SLOPE_45_UP   2
#define SHAPE_SLOPE_45_DOWN 3
#define SHAPE_HALF_UP_1     4
#define SHAPE_HALF_UP_2     5
#define SHAPE_HALF_DOWN_1   6
#define SHAPE_HALF_DOWN_2   7
#define SHAPE_PLATFORM      8

#define COLLISION_GID_START 687

extern void debug_log(const char *format, ...);
extern SDL_Texture *load_texture(SDL_Renderer *renderer, const char *path);

// --- HELPER ---
char* read_file_to_string(const char* path) {
    FILE* fp = fopen(path, "rb"); if (!fp) return NULL;
    fseek(fp, 0, SEEK_END); long size = ftell(fp); fseek(fp, 0, SEEK_SET);
    char* content = (char*)malloc(size + 1); fread(content, 1, size, fp); content[size] = 0;
    fclose(fp); return content;
}

// --- ID MAPPING ---
int get_tile_shape(int tile_id) {
    tile_id = (tile_id & 0x1FFFFFFF);
    if (tile_id == 0) return SHAPE_EMPTY;

    int rel = tile_id - COLLISION_GID_START;

    switch (rel) {
        case 0: return SHAPE_SOLID;
        case 1: return SHAPE_HALF_UP_1;
        case 2: return SHAPE_HALF_UP_2;
        case 3: return SHAPE_SLOPE_45_UP;
        case 4: return SHAPE_PLATFORM;
        case 5: return SHAPE_HALF_DOWN_1;
        case 6: return SHAPE_HALF_DOWN_2;
        case 7: return SHAPE_SLOPE_45_DOWN;

            // Safety Fallback für unbekannte Collision-Tiles
        default: return (rel >= 0 && rel < 30) ? SHAPE_SOLID : SHAPE_EMPTY;
    }
}

// --- HÖHENBERECHNUNG (Kernlogik) ---
int calculate_height(int shape, int ty, int offset_x) {
    int base = ty * 16;
    int lift = 0; // Visual Lift (optional)

    switch (shape) {
        case SHAPE_SOLID:       return base;
        case SHAPE_PLATFORM:    return base;

        case SHAPE_SLOPE_45_UP: return base + (16 - offset_x);
        case SHAPE_SLOPE_45_DOWN: return base + offset_x;

        case SHAPE_HALF_UP_1:   return (base + (16 - (offset_x / 2))) - lift;
        case SHAPE_HALF_UP_2:   return (base + (8 - (offset_x / 2))) - lift;

        case SHAPE_HALF_DOWN_1: return (base + (offset_x / 2)) - lift;
        case SHAPE_HALF_DOWN_2: return (base + 8 + (offset_x / 2)) - lift;

        default: return -1;
    }
}

// --- INIT ---
int map_init(Map *map, SDL_Renderer *renderer, const char *json_path,
             const char *cemetery_path, const char *tower_path, const char *collision_path) {
    char* c = read_file_to_string(json_path); if (!c) return 0;
    map->tiled_map = cute_tiled_load_map_from_memory(c, (int)strlen(c), 0); free(c);
    if (!map->tiled_map) return 0;

    map->textures[0] = load_texture(renderer, cemetery_path); map->tileset_firstgids[0] = 1;
    map->textures[1] = load_texture(renderer, tower_path);    map->tileset_firstgids[1] = 281;
    map->textures[2] = load_texture(renderer, collision_path);map->tileset_firstgids[2] = 687;
    map->texture_count = 3;

    cute_tiled_layer_t* layer = map->tiled_map->layers;
    while (layer) {
        if (layer->name.ptr && strcmp(layer->name.ptr, "Collision") == 0) {
            map->collision_layer = layer; break;
        }
        layer = layer->next;
    }
    return (map->textures[0] && map->textures[1]);
}

// --- IS SOLID ---
int map_is_solid(Map* map, int x, int y) {
    if (!map->collision_layer) return 0;
    int tx = x / 16; int ty = y / 16;
    if (tx < 0 || tx >= map->collision_layer->width || ty < 0 || ty >= map->collision_layer->height) return 0;

    int id = map->collision_layer->data[ty * map->collision_layer->width + tx];
    return (get_tile_shape(id) == SHAPE_SOLID);
}

// --- GET FLOOR HEIGHT (DER NEUE SCANNER) ---
int map_get_floor_height(Map* map, int x, int y) {
    if (!map->collision_layer) return -1;

    int tx = x / 16;
    int ty = y / 16;
    int offset_x = x % 16;

    // Wir speichern das "beste" Ergebnis.
    // -1 = nichts gefunden
    int best_height = -1;
    int found_slope = 0;

    // Scan von Oben (Kopf) nach Unten (Füße+1)
    // Wir wollen die OBERSTE Fläche finden, auf der man stehen kann.
    for (int check_y = ty - 1; check_y <= ty + 1; check_y++) {
        if (check_y < 0 || check_y >= map->collision_layer->height) continue;
        if (tx < 0 || tx >= map->collision_layer->width) continue;

        int index = (check_y * map->collision_layer->width) + tx;
        int id = map->collision_layer->data[index];
        int shape = get_tile_shape(id);

        if (shape == SHAPE_EMPTY) continue;

        // Berechnung der Höhe für dieses Tile
        int h = calculate_height(shape, check_y, offset_x);

        // --- ENTSCHEIDUNGSLOGIK ---

        // 1. Slopes (Rampen) haben immer Vorrang!
        if (shape >= SHAPE_SLOPE_45_UP && shape <= SHAPE_HALF_DOWN_2) {
            return h; // Sofort zurückgeben! Wir sind in einer Rampe.
        }

        // 2. Plattformen
        if (shape == SHAPE_PLATFORM) {
            // Nur gültig, wenn wir nicht zu tief drunter sind
            if (y <= h + 4) {
                // Wenn wir schon einen Boden haben, nehmen wir den höheren (kleinere Y)
                if (best_height == -1 || h < best_height) best_height = h;
            }
        }

        // 3. Solide Blöcke
        if (shape == SHAPE_SOLID) {
            // Nur gültig, wenn wir nicht "im" Block stecken (außer wir fallen von oben rein)
            // Hier nehmen wir einfach die Oberkante.
            if (best_height == -1 || h < best_height) best_height = h;
        }
    }

    return best_height;
}

// --- RENDER ---
void map_render(SDL_Renderer *renderer, Map *map, int camera_x, int camera_y) {
    if (!map->tiled_map) return;
    cute_tiled_layer_t* layer = map->tiled_map->layers;
    while (layer) {
        if (layer->name.ptr && strcmp(layer->name.ptr, "Collision") == 0) { layer = layer->next; continue; }
        if (layer->type.ptr && strcmp(layer->type.ptr, "tilelayer") == 0) {
            int* tiles = layer->data; int count = layer->data_count; int width = layer->width;
            for (int i = 0; i < count; i++) {
                int tile_id = tiles[i]; if (tile_id == 0) continue;
                int t_idx = -1;
                for (int t = map->texture_count - 1; t >= 0; t--) { if (tile_id >= map->tileset_firstgids[t]) { t_idx = t; break; } }
                if (t_idx == -1 || !map->textures[t_idx]) continue;
                SDL_Texture* tex = map->textures[t_idx];
                int firstgid = map->tileset_firstgids[t_idx];
                int gid = (tile_id & 0x1FFFFFFF) - firstgid;
                int img_w, img_h; SDL_QueryTexture(tex, NULL, NULL, &img_w, &img_h);
                int tile_size = 16; int tiles_per_row = img_w / tile_size;
                int px = (i % width) * tile_size; int py = (i / width) * tile_size;
                if (px - camera_x < -tile_size || px - camera_x > 480 || py - camera_y < -tile_size || py - camera_y > 272) continue;
                SDL_Rect src = { (gid % tiles_per_row) * tile_size, (gid / tiles_per_row) * tile_size, tile_size, tile_size };
                SDL_Rect dest = { px - camera_x, py - camera_y, tile_size, tile_size };
                SDL_RenderCopy(renderer, tex, &src, &dest);
            }
        }
        layer = layer->next;
    }
}

void map_cleanup(Map *map) {
    if (map->tiled_map) cute_tiled_free_map(map->tiled_map);
    for (int i = 0; i < MAX_TILESETS; i++) if (map->textures[i]) SDL_DestroyTexture(map->textures[i]);
}