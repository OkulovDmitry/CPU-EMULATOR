#include "png2asm.h"

#define STB_IMAGE_IMPLEMENTATION     // ровно в одном .c файле
#include "stb_image.h"

#include <string.h>

static int sort_ch;

typedef struct
{
    unsigned char r;
    unsigned char g;
    unsigned char b;
} Rgb;

typedef struct
{
    int mi;
    int ma;
} Box;

static int chch(const Rgb* c, int ch)  // достать нужный канал из цвета
{
    return ch == 0 ? c->r : ch == 1 ? c->g : c->b;
}

static int cmp_pixels(const void* a, const void* b)  // компаратор для qsort
{
    return chch((const Rgb*)a, sort_ch) - chch((const Rgb*)b, sort_ch);
}

// ищем интервал с наиб разницей max и min
static int cell_range(const Rgb* px, Box bx, int* ch)
{
    int mn[3] = {255, 255, 255};
    int mx[3] = {0, 0, 0};
    
    for (int i = bx.mi; i < bx.ma; i++)
    {
        for (int c = 0; c < 3; c++)
        {
            int v = chch(&px[i], c);
            if (v < mn[c])   mn[c] = v;
            if (v > mx[c])   mx[c] = v;
        }
    }

    int best = 0;
    for (int c = 1; c < 3; c++)
    {
        if (mx[c] - mn[c] > mx[best] - mn[best])  best = c;
    }

    *ch = best;
    return mx[best] - mn[best];
}

int main(int argc, char* argv[])
{
    if (argc != 6)
    {
        fprintf(stderr, "usage: %s <pattern> <first> <count> <step> <out.c>\n", argv[0]);
        fprintf(stderr, "usage: %s frames/frame_%%03ld.png 1 250 10 palette_new.c\n", argv[0]);
        return 1;
    }

    long first = strtol(argv[2], NULL, 10);   // номер первого кадра
    long count = strtol(argv[3], NULL, 10);   // сколько кадров всего
    long step  = strtol(argv[4], NULL, 10);   // берем каждый step-й кадр
    if (count <= 0 || step <= 0)
    {
        fprintf(stderr, "bad count.step\n");
        return 1;
    }

    // сколько кадров считается и сколько пикселей будет
    long used  = (count + step - 1) / step;
    size_t cap = (size_t)used * VRAM_W * VRAM_H;

    Rgb* px = malloc(cap * sizeof(Rgb));  // все пиксели будут в этом массиве
    if (px == NULL)  return 1;
    size_t total = 0;                                  // сколлько реально записано

    char name[1024];           // путь к кадру

    for (long i = 0; i < count; i += step)
    {
        snprintf(name, sizeof(name), argv[1], first + i);

        int w = 0, h = 0, channels = 0;
        unsigned char* img = stbi_load(name, &w, &h, &channels, 4);
        if (img == NULL)
        {
            fprintf(stderr, "%s: %s\n", name, stbi_failure_reason());
            free(px);
            return 1;
        }

        // для каждой клетки vram усредним блок пикселей исходной картинки
        for (int y = 0; y < VRAM_H; y++)
        {
            for (int x = 0; x < VRAM_W; x++)
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

                if (n == 0) continue;;

                px[total].r = (unsigned char)(r / n);
                px[total].g = (unsigned char)(g / n);
                px[total].b = (unsigned char)(b / n);
                total++;
            }
        }

        stbi_image_free(img);
        fprintf(stderr, "%s done\n", name);
    }

    fprintf(stderr, "pixels collected: %zu\n", total);

    Box cells[PALETTE_SIZE];
    int nc = 1;                // количество интервалов сейчас
    cells[0].mi = 0;
    cells[0].ma = (int)total;

    while (nc < PALETTE_SIZE)
    {
        int pick = -1, pick_range = 0, pick_ch = 0;

        for (int i = 0; i < nc; i++)
        {
            if (cells[i].ma - cells[i].mi < 2) continue;      // в одной точке резать нечего

            int ch = 0;
            int rg = box_range(px, cells[i], &ch);
            if (rg > pick_range)
            {
                pick = i;
                pick_range = rg;
                pick_ch = ch;
            }
        }

        if (pick < 0) break;                      // все коробки однотонные, резать больше нечего

        // сортируем пиксели выбранной коробки по её самому широкому каналу
        sort_ch = pick_ch;
        qsort(px + cells[pick].mi, (size_t)(cells[pick].ma - cells[pick].mi), sizeof(Rgb), cmp_pixels);

        // режем пополам: правая половина становится новой коробкой
        int mid = (cells[pick].mi + cells[pick].ma) / 2;
        cells[nc].mi   = mid;
        cells[nc].ma   = cells[pick].ma;
        cells[pick].ma = mid;
        
    }

    free(px);
    return 0;
}
