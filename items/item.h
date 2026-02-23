#ifndef ITEM_H
#define ITEM_H

#include <SDL.h>

typedef struct Player Player; // forward declaration to avoid circular dependency

typedef enum
{
    HEALTH_POTION,
    MANA_POTION,
    ROCK
} ItemType;

typedef struct item
{
    int x, y;
    int width, height;
    SDL_Texture *texture;
    ItemType type;
    int value; // Kann heal_amount ODER mana_amount sein
    int amount; // Für Stapelbarkeit, z.B. 3 Heiltränke
} Item;

Item item_init(SDL_Renderer *renderer, Item item, int x, int y);
void item_cleanup(Item *item);
void item_use(Item *item, Player *player);

#endif