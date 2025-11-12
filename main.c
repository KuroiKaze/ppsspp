#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include <stdlib.h> // Für sprintf

// --- WICHTIG: KONSTANTEN MÜSSEN HIER DEKLARIERT WERDEN ---
#define FRAME_COUNT 4        // ANPASSEN: Tatsächliche Anzahl der Frame-Dateien (z.B. 6)
#define ANIMATION_SPEED 100  // Zeit in Millisekunden pro Frame (10 FPS)
#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 272
// -----------------------------------------------------------

// Prototyp der Ladefunktion (Muss vor main() stehen)
SDL_Texture *load_texture(SDL_Renderer *renderer, const char *path);

int main(int argc, char *argv[]) {
    SDL_Window * window = NULL;
    SDL_Renderer * renderer = NULL;
    // SDL_Texture * sprite = NULL; // Nicht benötigt, da wir die Liste verwenden

    // --- Initialisierung ---
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
        fprintf(stderr, "SDL Init Error: %s\n", SDL_GetError());
        return 1;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        fprintf(stderr, "IMG Init Error: %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    window = SDL_CreateWindow("Texture List Animation", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if (!window) goto cleanup;

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) goto cleanup;

    // Weißen Hintergrund für RenderClear setzen
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); 

    // --- 1. Texture-Liste laden ---
    SDL_Texture *idle_frames[FRAME_COUNT] = { NULL }; 
    const char *base_path = "resources/Gothicvania Collection Files/Assets/Characters/PLAYABLE CHARACTERS/Cemetery Hero/Base/Sprites/hero-idle/hero-idle-";
    char path_buffer[256];

    for (int i = 0; i < FRAME_COUNT; i++) {
        // Erzeugt den Dateinamen (z.B. .../hero-idle-1.png bis .../hero-idle-6.png)
        sprintf(path_buffer, "%s%d.png", base_path, i + 1); 
        
        idle_frames[i] = load_texture(renderer, path_buffer);
        if (!idle_frames[i]) {
            fprintf(stderr, "Fehler: Frame %d konnte nicht geladen werden.\n", i + 1);
            goto cleanup;
        }
    }

    // --- 2. Positions-Setup ---
    SDL_Rect sprite_rect = { 0 }; 
    
    // Dimensionen vom ersten Frame abfragen
    SDL_QueryTexture(idle_frames[0], NULL, NULL, &sprite_rect.w, &sprite_rect.h);
    
    // Skalierung: Macht den Sprite doppelt so groß (Beispiel)
    sprite_rect.w *= 2; 
    sprite_rect.h *= 2;

    // Position mittig setzen
    sprite_rect.x = SCREEN_WIDTH/2 - sprite_rect.w/2;
    sprite_rect.y = SCREEN_HEIGHT/2 - sprite_rect.h/2;
    
    // --- 3. Animations-Steuerung ---
    Uint32 last_time = SDL_GetTicks();
    int current_frame = 0;
    int running = 1;
    SDL_Event event;
    

    // --- Hauptschleife ---
    while (running) { 
        // Process input
        while (SDL_PollEvent(&event)) { // <-- Die Klammer war hier falsch platziert
            switch (event.type) {
                case SDL_QUIT:
                    running = 0;
                    break;
                case SDL_CONTROLLERDEVICEADDED:
                    SDL_GameControllerOpen(event.cdevice.which);
                    break;
                case SDL_CONTROLLERBUTTONDOWN:
                    if(event.cbutton.button == SDL_CONTROLLER_BUTTON_START) {
                        running = 0;
                    }
                    break;
            }
        } // Ende der while(SDL_PollEvent) Schleife
        

        // --- Animations-Update (Frame-Wechsel) ---
        Uint32 current_time = SDL_GetTicks();
        if (current_time > last_time + ANIMATION_SPEED) {
            // Nächsten Frame in der Liste auswählen
            current_frame = (current_frame + 1) % FRAME_COUNT;
            last_time = current_time;
        }

        // --- Rendering ---
        
        // Löscht den Bildschirm mit der DrawColor (Weiß)
        SDL_RenderClear(renderer); 

        // Zeichnen des aktuellen Frames
        SDL_RenderCopy(renderer, idle_frames[current_frame], NULL, &sprite_rect);

        // Aktualisiert den Bildschirm
        SDL_RenderPresent(renderer);
    }
    
// --- Aufräumen ---
cleanup:
    // ALLE geladenen Texturen freigeben
    for (int i = 0; i < FRAME_COUNT; i++) {
        if (idle_frames[i]) {
            SDL_DestroyTexture(idle_frames[i]);
        }
    }
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return 0;
}

// Hilfsfunktion zum Laden einer einzelnen Textur
SDL_Texture *load_texture(SDL_Renderer *renderer, const char *path) {
    SDL_Surface *pixels = IMG_Load(path);
    if (!pixels) {
        fprintf(stderr, "IMG_Load Error (%s): %s\n", path, IMG_GetError());
        return NULL;
    }
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, pixels);
    SDL_FreeSurface(pixels);
    if (!texture) {
        fprintf(stderr, "SDL_CreateTextureFromSurface Error: %s\n", SDL_GetError());
    }
    return texture;
}