#include "display.h"
#include <raylib.h>

#define DISPLAY_CELL_SIZE 20 // размер одного "пикселя" окна в реальных пикселях

static const Color palette[PALETTE_SIZE] = 
{
    {0,   0,   0,   255},  // 0: черный
    {0,   0,   128, 255}, // 1: Темно-синий
    {0,   128, 0,   255}, // 2: Зеленый
    {0,   128, 128, 255}, // 3: Сине-зеленый
    {128, 0,   0,   255}, // 4: Темно-красный
    {128, 0,   128, 255}, // 5: Пурпурный
    {128, 128, 0,   255}, // 6: Коричневый
    {192, 192, 192, 255}, // 7: Светло-серый
    {128, 128, 128, 255}, // 8: Темно-серый
    {0,   0,   255, 255}, // 9: Ярко-синий
    {0,   255, 0,   255}, // 10: Ярко-зеленый
    {0,   255, 255, 255}, // 11: Ярко-сине-зеленый
    {255, 0,   0,   255}, // 12: Ярко-красный
    {255, 0,   255, 255}, // 13: Ярко-пурпурный
    {255, 255, 0,   255}, // 14: Желтый
    {255, 255, 255, 255}  // 15: Белый
};

int display_init(int width, int height)
{
    int screen_width  = width  * DISPLAY_CELL_SIZE;
    int screen_height = height * DISPLAY_CELL_SIZE;

    InitWindow(screen_width, screen_height, "VM Raylib Display");
    SetTargetFPS(60);

    return WindowShouldClose() ? -1 : 0;
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

            DrawRectangle(x * DISPLAY_CELL_SIZE, y * DISPLAY_CELL_SIZE, DISPLAY_CELL_SIZE, DISPLAY_CELL_SIZE, palette[color_idx]);
        }
    }

    EndDrawing();
}

int display_should_close(void)
{
    return WindowShouldClose();
}

void display_close(void)
{
    CloseWindow();
}