#include "display.h"
#include <raylib.h>
#include "palette.h"

#define DISPLAY_CELL_SIZE 2 // размер одного "пикселя" окна в реальных пикселях

int display_init(int width, int height)
{
    int screen_width  = width  * DISPLAY_CELL_SIZE;
    int screen_height = height * DISPLAY_CELL_SIZE;

    InitWindow(screen_width, screen_height, "VM Raylib Display");
    SetTargetFPS(60);

    return IsWindowReady() ? 0 : -1;    
}

void display_show(const Elem_t* vram, int width, int height)
{
    BeginDrawing();
    ClearBackground(BLACK);

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            Elem_t val    = vram[y * width + x];
            int color_idx = (int)val;

            if (color_idx < 0 || color_idx >= PALETTE_SIZE) color_idx = 0;

            Color c = { my_palette[color_idx].r, my_palette[color_idx].g,
            my_palette[color_idx].b, my_palette[color_idx].a };

            DrawRectangle(x * DISPLAY_CELL_SIZE, y * DISPLAY_CELL_SIZE, DISPLAY_CELL_SIZE, DISPLAY_CELL_SIZE, c);
        }
    }

    EndDrawing();
}

int display_should_close(void)
{
    return WindowShouldClose();
}

void display_wait_close(const Elem_t* vram, int width, int height)
{
    while (!WindowShouldClose())              // Esc или крестик
    {
        display_show(vram, width, height);    // перерисовываем последний кадр, чтобы окно не зависло
    }
}

void display_close(void)
{
    CloseWindow();
}