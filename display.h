#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "stack.h"
#include "isa.h"

#define PALETTE_SIZE 16                  // сколько цветов знает экран, 0...15 - это номер цвета

int   display_init(int width, int height);
void  display_show(const Elem_t* vram, int width, int height);
void  display_close(void);

#endif // DIDPLAY_H

