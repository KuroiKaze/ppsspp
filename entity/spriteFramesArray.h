
#ifndef SPRITEFRAMESARRAY_H
#define SPRITEFRAMESARRAY_H
#include <SDL.h>

typedef struct {
    SDL_Texture **frames;  // dynamisches Array
    int count;             // wie viele Frames existieren
    int sprite_w;         // Breite eines Frames
    int sprite_h;         // Höhe eines Frames
} SpriteFrameArray;

#endif // SPRITEFRAMESARRAY_H