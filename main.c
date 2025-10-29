#include <pspkernel.h>
#include <pspctrl.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// PSP module info
PSP_MODULE_INFO("texture_demo", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

// Screen constants
#define BUFFER_WIDTH 512
#define BUFFER_HEIGHT 272
#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 272

typedef struct
{
    float u, v;
    unsigned int colour;
    float x, y, z;
} TextureVertex;

typedef struct
{
    int width, height;
    unsigned int *data;
} Texture;

// Global variables
static unsigned int __attribute__((aligned(16))) list[262144];
int running = 1;
void *fbp0, *fbp1;

// ==== Exit callback handling ====
int exit_callback(int arg1, int arg2, void *common)
{
    running = 0;
    return 0;
}

int callback_thread(SceSize args, void *argp)
{
    int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}

int setup_callbacks(void)
{
    int thid = sceKernelCreateThread("callback_thread", callback_thread, 0x11, 0xFA0, 0, 0);
    if (thid >= 0)
        sceKernelStartThread(thid, 0, 0);
    return thid;
}

// ==== Graphics setup ====
void initGu()
{
    sceGuInit();

    fbp0 = guGetStaticVramBuffer(BUFFER_WIDTH, BUFFER_HEIGHT, GU_PSM_8888);
    fbp1 = guGetStaticVramBuffer(BUFFER_WIDTH, BUFFER_HEIGHT, GU_PSM_8888);

    sceGuStart(GU_DIRECT, list);
    sceGuDrawBuffer(GU_PSM_8888, fbp0, BUFFER_WIDTH);
    sceGuDispBuffer(SCREEN_WIDTH, SCREEN_HEIGHT, fbp1, BUFFER_WIDTH);
    sceGuDepthBuffer(fbp0, 0);

    sceGuDisable(GU_DEPTH_TEST);
    sceGuOffset(2048 - (SCREEN_WIDTH / 2), 2048 - (SCREEN_HEIGHT / 2));
    sceGuViewport(2048, 2048, SCREEN_WIDTH, SCREEN_HEIGHT);
    sceGuScissor(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    sceGuEnable(GU_SCISSOR_TEST);

    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);
}

void endGu()
{
    sceGuDisplay(GU_FALSE);
    sceGuTerm();
}

void startFrame()
{
    sceGuStart(GU_DIRECT, list);
    sceGuClearColor(0xFFFFFFFF);
    sceGuClear(GU_COLOR_BUFFER_BIT);
}

void endFrame()
{
    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
    sceGuSwapBuffers();
}

// ==== Texture loading ====
Texture *loadTexture(const char *filename)
{
    Texture *texture = (Texture *)calloc(1, sizeof(Texture));

    texture->data = (unsigned int *)stbi_load(filename, &texture->width, &texture->height, NULL, STBI_rgb_alpha);
    if (!texture->data)
    {
        printf("Fehler: konnte Textur nicht laden: %s\n", filename);
        free(texture);
        return NULL;
    }

    sceKernelDcacheWritebackInvalidateAll();
    return texture;
}

void freeTexture(Texture *texture)
{
    if (texture)
    {
        if (texture->data)
            stbi_image_free(texture->data);
        free(texture);
    }
}

// ==== Texture drawing ====
void drawTexture(Texture *texture, float x, float y, float w, float h)
{
    TextureVertex *vertices = (TextureVertex *)sceGuGetMemory(2 * sizeof(TextureVertex));

    vertices[0].u = 0.0f;
    vertices[0].v = 0.0f;
    vertices[0].colour = 0xFFFFFFFF;
    vertices[0].x = x;
    vertices[0].y = y;
    vertices[0].z = 0.0f;

    vertices[1].u = (float)texture->width;
    vertices[1].v = (float)texture->height;
    vertices[1].colour = 0xFFFFFFFF;
    vertices[1].x = x + w;
    vertices[1].y = y + h;
    vertices[1].z = 0.0f;

    sceGuTexMode(GU_PSM_8888, 0, 0, GU_FALSE);
    sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGB);
    sceGuTexImage(0, texture->width, texture->height, texture->width, texture->data);

    sceGuEnable(GU_TEXTURE_2D);
    sceGuDrawArray(GU_SPRITES,
                   GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D,
                   2, 0, vertices);
    sceGuDisable(GU_TEXTURE_2D);
}

// ==== Main ====
int main()
{
    setup_callbacks();
    initGu();

    Texture *texture = loadTexture("resources/sprites/grass.png");
    if (!texture)
    {
        printf("Textur konnte nicht geladen werden!\n");
        endGu();
        sceKernelExitGame();
        return 0;
    }

    while (running)
    {
        startFrame();

        drawTexture(texture,
                    (SCREEN_WIDTH - texture->width) / 2,
                    (SCREEN_HEIGHT - texture->height) / 2,
                    texture->width, texture->height);

        endFrame();
    }

    freeTexture(texture);
    endGu();

    sceKernelExitGame();
    return 0;
}