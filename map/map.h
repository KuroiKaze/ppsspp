#ifndef MAP_H
#define MAP_H

#include <SDL.h>
#include "cute_tiled.h"

// Wir haben jetzt exakt 3 Tilesets (Cemetery, Tower, Collision)
#define MAX_TILESETS 3

typedef struct Map {
    cute_tiled_map_t* tiled_map;

    // Speicher für die Texturen
    SDL_Texture* textures[MAX_TILESETS];

    // Wir speichern die "FirstGID" jedes Tilesets, damit wir wissen,
    // welche Textur zu welcher ID gehört.
    int tileset_firstgids[MAX_TILESETS];
    int texture_count;

    cute_tiled_layer_t* collision_layer;
} Map;

// Init nimmt jetzt nur noch den Pfad zur JSON (die Pfade zu Bildern stehen IN der JSON)
// Aber da wir auf der PSP sind, müssen wir die Pfade vielleicht manuell fixen.
// Wir bleiben sicherheitshalber beim manuellen Laden für maximale Kontrolle.
int map_init(Map *map, SDL_Renderer *renderer, const char *json_path,
             const char *cemetery_path, const char *tower_path, const char *collision_path);

int map_is_solid(Map* map, int x, int y);
int map_get_floor_height(Map* map, int x, int y);
int get_tile_shape(int tile_id);
void map_render(SDL_Renderer *renderer, Map *map, int camera_x, int camera_y);
void map_render_debug(SDL_Renderer *renderer, Map *map, int camera_x, int camera_y); // NEU: Um Kollisionen zu sehen
void map_cleanup(Map *map);

#endif