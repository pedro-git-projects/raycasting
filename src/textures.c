#include "textures.h"
#include <stdio.h>

static const char *textureFileNames[NUM_TEXTURES] = {
    "./images/index4.png",
    "./images/index3.png",
    "./images/index2.png",
    "./images/index10.png",
    "./images/index.png",
    "./images/index6.png",
    "./images/index8.png",
    "./images/index10.png",
};

texture_t wallTextures[NUM_TEXTURES];

void loadWallTextures()
{
    for (int i = 0; i < NUM_TEXTURES; i++)
    {
        upng_t *upng;

        upng = upng_new_from_file(textureFileNames[i]);
        if (upng != NULL)
        {
            upng_decode(upng);
            if (upng_get_error(upng) == UPNG_EOK)
            {
                wallTextures[i].upngTexture = upng;
                wallTextures[i].width = upng_get_width(upng);
                wallTextures[i].height = upng_get_height(upng);
                wallTextures[i].texture_buffer = (uint32_t *)upng_get_buffer(upng);
            }
        }
    }
}

void freeWallTextures()
{
    for (int i = 0; i < NUM_TEXTURES; i++)
    {
        upng_free(wallTextures[i].upngTexture);
    }
}
