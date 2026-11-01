#include "display.h"
#include <stdio.h>
#include <sys/ioctl.h>   // ioctl - спросить у системы параметры терминала
#include <unistd.h>      // STDOUT_FILENO - номер стандартного вывода

int display_init(int width, int height)
{
    (void)width;
    (void)height;
    printf("\x1b[2J\x1b[?25l");        // очистить экран и спрятать курсор
    return 0;         // пока что функция бесполезна, так как вывод идет в консоль
}

static void term_size(int* cols, int* rows)
{
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0)
    {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
    }
    else
    {
        *cols = 80;
        *rows = 24;
    }
}

// (ox, oy) - номер клетки экрана, (ow, oh) - сколько клеток экрана по ширине и высоте
static void avg_color(const Elem_t* vram, int width, int height, int ox, int oy, int ow, int oh, int* r, int* g, int* b)
{
    int x0 = ox * width  / ow,  x1 = (ox + 1) * width  / ow;  // границы блока по X
    int y0 = oy * height / oh, y1  = (oy + 1) * height / oh;  // границы блока по Y

    if (x1 <= x0) x1 = x0 + 1;
    if (y1 <= y0) y1 = y0 + 1;  // хотя бы 1 клетка

    int sr = 0, sg = 0, sb = 0;
    int n = 0;                  // счетчик учтенных клеток

    for (int y = y0; y < y1 && y < height; y++)
    {
        for (int x = x0; x < x1 && x < width; x++)
        {
            Elem_t v = vram[y * width + x];
            int c = (v >= 0 && v < PALETTE_SIZE) ? (int)v : 0;

            sr += my_palette[c].r;
            sg += my_palette[c].g;
            sb += my_palette[c].b;
            n++;
        }
    }

    if (n == 0) n = 1;
    *r = (int)(sr / n);
    *g = (int)(sg / n);
    *b = (int)(sb / n);
}

void display_show(const Elem_t* vram, int width, int height)
{
    int cols, rows;
    term_size(&cols, &rows);
    rows--;
    if (rows < 1) rows = 1;

    int out_w = cols;
    int out_h = height * out_w / width;

    if (out_h > rows * 2)
    {
        out_h = rows * 2;
        out_w = width * out_h / height;
    }

    if (out_w < 1)  out_w = 1;
    if (out_h < 2)  out_h = 2;

    out_h -= out_h % 2;

    printf("\x1b[H");                            // курсор в левый верхний угол, экран обновляется на месте

    int cell_rows = out_h / 2; // кусок экрана, который займет картинка

    for (int cy = 0; cy < cell_rows; cy++)
    {
        int last = -1;         // какой цыет сейчас включен

        for (int cx = 0; cx < out_w; cx++)
        {
            int r, g, b;
            avg_color(vram, width, height, cx, cy, out_w, cell_rows, &r, &g, &b);

            int key = (r << 16) | (g << 8) | b;
            if (key != last)
            {
                printf("\x1b[48;2;%d;%d;%dm", r, g, b);
                last = key;
            }
            putchar(' ');
        }

        printf("\x1b[0m\n");         // сброс цвета в конце строки
    }
    fflush(stdout);
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
    printf("\x1b[0m\x1b[?25h"); // возвращаем курсор на место
    // для консоли ниче не закрываем
}