#include "item.h"
#include <SDL2/SDL_image.h>
#include "../player/player.h"


#define HEALTH_POTION_PATH  "resources/sprites/items/item-114.png"
#define MANA_POTION_PATH   "resources/sprites/items/item-113.png"
#define HEALTH_POTION_HEAL_AMOUNT 30
#define MANA_POTION_MANA_AMOUNT 30

extern SDL_Texture *load_texture(SDL_Renderer *renderer, const char *path);

Item item_init(SDL_Renderer *renderer, Item item, int x, int y) {
    const char* path = (item.type == HEALTH_POTION) ? HEALTH_POTION_PATH : MANA_POTION_PATH;
    
    item.texture = load_texture(renderer, path);
    
    if (item.texture) {
        SDL_QueryTexture(item.texture, NULL, NULL, &item.width, &item.height);
    } else {
        printf("Error: Could not load texture for item type %d\n", item.type);
    }

    item.x = x;
    item.y = y;

    if (item.amount <= 0) {
        item.amount = 1;
    }

    // Werte zuweisen
    if (item.type == HEALTH_POTION) {
        item.value = HEALTH_POTION_HEAL_AMOUNT;
    } else if (item.type == MANA_POTION) {
        item.value = MANA_POTION_MANA_AMOUNT;
    }

    return item;
}

void item_cleanup(struct item *item) {
    if (item->texture) {
        SDL_DestroyTexture(item->texture);
        item->texture = NULL;
    }
}

void item_use(Item *item, Player *player) {
    if (!item || !player) return;

    if(item->type == HEALTH_POTION) {
        if(player->entity.health < PLAYER_MAX_HEALTH) { 
            player->entity.health += item->value;
        } 
        
        if (player->entity.health > PLAYER_MAX_HEALTH) {
            player->entity.health = PLAYER_MAX_HEALTH;
        }
        
        printf("Health Potion used! Heals for %d HP.\n", item->value);
        printf("Current Health: %d\n", player->entity.health);
    } 
    else if (item->type == MANA_POTION) {
        printf("Mana Potion used! Restores %d MP.\n", item->value);
    }
}