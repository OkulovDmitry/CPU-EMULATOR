#include "display.h"
#include <stdio.h>

int display_init(int width, int height)
{
    (void)width;
    (void)height;
    return 0;         // пока что функция бесполезна, так как вывод идет в консоль
}

void display_show(const Elem_t* vram, int width, int height)
{
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            Elem_t v = vram[y * width + x];
            int c = (v >= 0 && v < PALETTE_SIZE) ? (int)v : 0;  // мусор и nan дают цвет 0

            printf("\x1b[48;2;%d;%d;%dm ", my_palette[c].r, my_palette[c].g, my_palette[c].b);
        }
        printf("\x1b[0m\n");                                    // в конце строки сбрасываем цвет
    }
}

int display_should_close(void)
{
    return 0;  // для консоли бесполезно
}

void  display_wait_close(const Elem_t* vram, int width, int height)
{
    (void)vram;
    (void)width;
    (void)height;
    // в консоли ниче не ждем, последний кадр уже напечатан
}

void display_close(void)
{
    // для консоли ниче не закрываем
}