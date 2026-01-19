#ifndef MAP_H
#define MAP_H

#include <SDL.h>
#include "cute_tiled.h"

// Shapes
#define SHAPE_EMPTY 0
#define SHAPE_SOLID 1
#define SHAPE_SLOPE_45_UP   2
#define SHAPE_SLOPE_45_DOWN 3
#define SHAPE_HALF_UP_1     4
#define SHAPE_HALF_UP_2     5
#define SHAPE_HALF_DOWN_1   6
#define SHAPE_HALF_DOWN_2   7
#define SHAPE_PLATFORM      8
#define SHAPE_DOOR          9
#define SHAPE_ZOMBIE_SPAWN  10
#define SHAPE_SLIME_SPAWN   11
#define SHAPE_PLAYER_SPAWN  12

#define MAX_TILESETS 8

typedef struct Map {
    cute_tiled_map_t* tiled_map;
    cute_tiled_layer_t* collision_layer;
    SDL_Texture* textures[MAX_TILESETS];
    int tileset_firstgids[MAX_TILESETS];
    int texture_count;
    int collision_gid_start;
} Map;

// Functions
int map_init(Map *map, SDL_Renderer *renderer, const char *json_path, const char** texture_paths, int texture_count);
int map_get_shape_at(Map *map, int x, int y);
int map_get_floor_height(Map* map, int x, int y);
int map_is_solid(Map* map, int x, int y);
void map_render(SDL_Renderer *renderer, Map *map, int camera_x, int camera_y);
void map_cleanup(Map *map);
int get_tile_shape(Map* map, int tile_id);

#endif