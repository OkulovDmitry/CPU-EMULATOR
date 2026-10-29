
#include <stdio.h>
#include <stdlib.h>
#include "palette.h"
#include "cpu.h"

#define STB_IMAGE_IMPLEMENTATION     // ровно в одном .c файле
#define STBI_ONLY_PNG

#include "stb_image.h"

static int nearest_color(int a, int b, int g, int r)
{
    int best = 0;
    int best_dist = -1;

    for (int i = 0; i < PALETTE_SIZE; i++)
    {
        int da = a - palette[i].a;
        int db = b - palette[i].b;
        int dg = g - palette[i].g;
        int dr = r - palette[i].r;

        int dist = da * da + db * db + dg * dg + dr * dr;

        if (best_dist < 0 || dist < best_dist)
        {
            best = i;
            best_dist = dist;
        }
    }

    return best;
}

static int pixel_color(const unsigned char* img, int w, int h, int x, int y)
{
    //вычисляем блок пикселей, который проецируется в одну клетку
    int x0 = x * w / VRAM_W, x1 = (x + 1) * w / VRAM_W;
    int y0 = y * h / VRAM_H, y1 = (y + 1) * h / VRAM_H;
    // x0, y0 - левый верхний угол этого бллока, x1, y1 - правый нижний

    if (x1 <= x0) x1 = x0 + 1;
    if (y1 <= y0) y1 = y0 + 1;  // хотя бы 1 пиксель

    int r = 0, g = 0, b = 0;
    int n = 0;                  // счетчик учтенных пикселей

    for (int sy = y0; sy < y1 && sy < h; sy++)
    {
        for (int sx = x0; sx < x1 && sx < w; sx++)
        {
            const unsigned char* p = img + 4 * ((size_t)sy * w + sx);  // * 4, тк пиксель состоит из 4 байт
            int a = p[3];  //прозрачность пикселя
            r += p[0] * a / 255; // прозрачное смешивается с чёрным фоном
            g += p[1] * a / 255; // Рез = цвет * альфа + фон * (1 - альфа), но при черном фоне вторая часть зануляется и остается только первая
            b += p[2] * a / 255;
            n++;
        }
    }

    if (n == 0) return 0;
    return nearest_color(255, (int)(b / n), (int)(g / n), (int)(r / n));
}

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "usage: %s <picture.png> <program_out.txt>\n", argv[0]);
        return 1;
    }

    int w = 0, h = 0, channels = 0;
    unsigned char* img = stbi_load(argv[1], &w, &h, &channels, 4);
    if (img == NULL)
    {
        fprintf(stderr, "%s: %s\n", argv[1], stbi_failure_reason());
        return 1;
    }

    FILE* out = fopen(argv[2], "w");
    if (out == NULL)
    {
        perror(argv[2]);
        stbi_image_free(img);
        return 1;
    }

    fprintf(out, "; %s: %d*%d -> %d*%d\n", argv[1], w, h, VRAM_W, VRAM_H);

    for (int y = 0; y < VRAM_H; y++)
    {
        for (int x = 0; x < VRAM_W; x++)
        {
            int idx = pixel_color(img, w, h, x, y);
            if (idx == 0) continue;                  // фон уже нулевой

            fprintf(out, "   PUSH %d\n   POP [%d]\n", idx, y * VRAM_W+ x);
        }
    }

    fprintf(out, "   DRAW\n HLT\n");

    stbi_image_free(img);
    if (fclose(out) != 0) { perror(argv[2]); return 1; }
    return 0;
}