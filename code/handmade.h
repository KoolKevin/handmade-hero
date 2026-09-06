#ifndef HANDMADE_H
#define HANDMADE_H

/*
    Services that the game provides to the platform layer.
*/
struct gameOffscreenBuffer {
    void* memory;
    int width;
    int height;
    int stride;
};

void gameUpdateAndRender(gameOffscreenBuffer* buffer, int XOffset, int YOffset);

/*
    Services that the platform layer provides to the the game
*/

#endif