#include "handmade.h"

void
renderGradient(gameOffscreenBuffer* buffer, int XOffset, int YOffset)
{
    // good way to write pixel loops. usiamo esplicitamente un
    // row pointer aggiuntivo dato che non è detto che il pixel
    // pointer sia allineato con la prossima riga alla fine del
    // loop interno
    uint8 *row = (uint8 *)buffer->memory;
    for (int Y = 0; Y < buffer->height; Y++)
    {
        uint32 *pixel = (uint32 *)row;
        for (int X = 0; X < buffer->width; X++)
        {
            uint8 red = (uint8)(X + XOffset);
            uint8 green = (uint8)(Y + YOffset);

            // BGR windows pixel layout
            *pixel = red << 16 | green << 8;
            pixel++;
        }

        row += buffer->stride;
    }
}

void gameUpdateAndRender(gameOffscreenBuffer* buffer) {
    int XOffset = 0;
    int YOffset = 0;
    renderGradient(buffer, XOffset, YOffset);
}