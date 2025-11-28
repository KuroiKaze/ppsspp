// map.h
#ifndef MAP_H
#define MAP_H

#include <SDL.h>
#include "cute_tiled.h"

typedef struct Map {
    cute_tiled_map_t* tiled_map;
    SDL_Texture* tileset_texture;
    cute_tiled_layer_t* collision_layer;
} Map;

// Init lädt jetzt das JSON
int map_init(Map *map, SDL_Renderer *renderer, const char *json_path, const char *tileset_path);
int map_is_solid(Map* map, int x, int y);
void map_render(SDL_Renderer *renderer, Map *map, int camera_x, int camera_y);
void map_cleanup(Map *map);

#endif