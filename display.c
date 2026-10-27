#include "display.h"

//=== palette_char - символ значения ячейки в текстовом режиме: 0 это '.', дальше цифра цвета в hex, неправильное значение это '?' ===
static char palette_char(Elem_t v)
{
    if (!(v >= 0 && v < PALETTE_SIZE)) return '?';  // вне палитры, дробь больше 15 или Nan
    int color = (int)v;                             // отброс дробной части
    return ".123456789abcdef"[color];
}
//====================================================================================================================================

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
            putchar(palette_char(vram[y * width + x]));
            putchar(' ');                                     //для придания квадратной формы клеткам(почти)
        }
        putchar('\n');
    }
}

int display_should_close(void)
{
    return 0;  // для консоли бесполезно
}

void display_close(void)
{
    // для консоли ниче не закрываем
}