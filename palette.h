#ifndef PALETTE_H
#define PALETTE_H

#include <stdint.h>

#define PALETTE_SIZE 32          // сколько цветов знает экран, 0...31

typedef struct
{
    uint8_t r, g, b, a;
} PaletteColor;                  // не Color: это имя занято в raylib

extern const PaletteColor my_palette[PALETTE_SIZE];

#endif //PALETTE_H