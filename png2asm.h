#ifndef PNG2ASM_H
#define PNG2ASM_H

#include <stdio.h>
#include <stdlib.h>
#include "palette.h"
#include "cpu.h"

void asm_generation(unsigned char* img, int w, int h, FILE* out, const char* name, unsigned char* prev);

#endif //