// map.c
#define CUTE_TILED_IMPLEMENTATION
#include "map.h"
#include <SDL_image.h>
#include <stdio.h>

extern void debug_log(const char *format, ...);
extern SDL_Texture *load_texture(SDL_Renderer *renderer, const char *path);

char* read_file_to_string(const char* path) {
    FILE* fp = fopen(path, "rb");
    if (!fp) return NULL;

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char* content = (char*)malloc(size + 1);
    if (content) {
        fread(content, 1, size, fp);
        content[size] = 0; // Null-Terminator
    }
    fclose(fp);
    return content;
}

int map_init(Map *map, SDL_Renderer *renderer, const char *json_path, const char *tileset_path) {
    // 1. JSON Datei lesen
    char* file_content = read_file_to_string(json_path);
    if (!file_content) {
        fprintf(stderr, "Could not read map JSON: %s\n", json_path);
        return 0;
    }

    // 2. Parsen mit cute_tiled
    map->tiled_map = cute_tiled_load_map_from_memory(file_content, (int)strlen(file_content), 0);
    free(file_content); // String brauchen wir nicht mehr

    if (!map->tiled_map) {
        fprintf(stderr, "Failed to parse Tiled JSON.\n");
        return 0;
    }

    // 3. Textur laden
    map->tileset_texture = load_texture(renderer, tileset_path);
    if (!map->tileset_texture) return 0;

    // 4. Collision Layer finden
    cute_tiled_layer_t* layer = map->tiled_map->layers;
    while (layer) {
        if (strcmp(layer->name.ptr, "Collision") == 0) {
            map->collision_layer = layer;
            debug_log("Collision Layer found!");
            break;
        }
        layer = layer->next;
    }

    return 1;
}

int map_is_solid(Map* map, int x, int y) {
    if (!map->collision_layer) return 0; // Kein Kollisionslayer -> Alles ist Luft

    // 1. Pixel zu Tile-Koordinaten umrechnen
    int tile_x = x / 16; // Annahme: 16px Tiles
    int tile_y = y / 16;

    // 2. Map Grenzen prüfen (WICHTIG!)
    if (tile_x < 0 || tile_x >= map->collision_layer->width ||
        tile_y < 0 || tile_y >= map->collision_layer->height) {
        return 1; // Außerhalb der Map ist alles "Wand" (sicherer)
    }

    // 3. Tile ID aus dem Array holen
    int index = (tile_y * map->collision_layer->width) + tile_x;
    int tile_id = map->collision_layer->data[index];

    // 0 bedeutet "leer" in Tiled. Alles ungleich 0 ist ein Block.
    if (tile_id != 0) return 1;

    return 0;
}

void map_render(SDL_Renderer *renderer, Map *map, int camera_x, int camera_y) {
    if (!map->tiled_map || !map->tileset_texture) return;

    cute_tiled_layer_t* layer = map->tiled_map->layers;

    // Loop durch alle Layer (falls du Background/Collision Layer hast)
    while (layer) {
        // Wenn der Layer "Collision" heißt, überspringen wir das Zeichnen!
        if (strcmp(layer->name.ptr, "Collision") == 0) {
            layer = layer->next;
            continue;
        }
        // Wir rendern nur Tile-Layer, keine Objekt-Layer
        if (strcmp(layer->type.ptr, "tilelayer") == 0) {

            // Das Array der Tile-IDs
            int* tiles = layer->data;
            int count = layer->data_count;
            int width = layer->width;  // Map Breite in Tiles

            // Infos über das Tileset (vorausgesetzt du hast nur EINS)
            cute_tiled_tileset_t* tileset = map->tiled_map->tilesets;

            // HIER IST DEIN FIX: Die Library weiß, dass das Bild 448px breit ist!
            int image_width = tileset->imagewidth;
            int tile_width = tileset->tilewidth;
            int tile_height = tileset->tileheight;
            int tiles_per_row = image_width / tile_width; // 448 / 16 = 28 (KORREKT!)

            // Render Loop
            for (int i = 0; i < count; i++) {
                int tile_id = tiles[i];

                if (tile_id == 0) continue; // Leer

                // GID Berechnung (Tiled IDs starten oft bei 1)
                // cute_tiled bereinigt flags meistens, aber zur Sicherheit:
                int gid = (tile_id & 0x1FFFFFFF) - tileset->firstgid;

                // Position berechnen
                int x = (i % width) * tile_width;
                int y = (i / width) * tile_height;

                // Culling (Nur zeichnen was im Bild ist)
                if (x - camera_x < -tile_width || x - camera_x > 480 ||
                    y - camera_y < -tile_height || y - camera_y > 272) {
                    continue;
                }

                SDL_Rect src;
                src.x = (gid % tiles_per_row) * tile_width;
                src.y = (gid / tiles_per_row) * tile_height;
                src.w = tile_width;
                src.h = tile_height;

                SDL_Rect dest;
                dest.x = x - camera_x;
                dest.y = y - camera_y;
                dest.w = tile_width;
                dest.h = tile_height;

                SDL_RenderCopy(renderer, map->tileset_texture, &src, &dest);
            }
        }
        layer = layer->next; // Nächster Layer
    }
}

void map_cleanup(Map *map) {
    if (map->tiled_map) {
        cute_tiled_free_map(map->tiled_map);
        map->tiled_map = NULL;
    }
    if (map->tileset_texture) {
        SDL_DestroyTexture(map->tileset_texture);
        map->tileset_texture = NULL;
    }
}