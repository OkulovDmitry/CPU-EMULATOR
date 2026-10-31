#include "png2asm.h"

#include "stb_image.h"

#include <string.h>
#include <ctype.h>
#include <limits.h>

#define PATH_MAX_LEN 1024

/*
Разберем адрес фрэймов на части:
 - prefix = sheep_frames/frame_
 - digits = xxxx (количество цифр любое, ведущие нули сохраняется)
 - suffix = .png
*/

static int split_frame_path(const char* path, 
                            char* prefix, size_t prefix_sz,
                            char* suffix, size_t suffix_sz,
                            long* first_number, int* width)
{
    size_t len = strlen(path);
    const char* dot    = strrchr(path, '.');
    const char* slash1 = strrchr(path, '/');
    const char* slash2 = strrchr(path, '\\');
    const char* slash  = slash1 > slash2 ? slash1 : slash2;
    
    size_t end = (dot != NULL && (slash == NULL || dot > slash)) ? (size_t)(dot - path) : len;

    size_t start = end;
    while (start > 0 && isdigit((unsigned char)path[start - 1]))
        start--;

    if (start == end)
        return 0;     // цифр нет

    *width = (int)(end - start);
    if (*width > 18)
        return 0;     // слишком много

    if (start >= prefix_sz || (len - end) >= suffix_sz)
        return 0;

    memcpy(prefix, path, start);
    prefix[start] = '\0';
    strcpy(suffix, path + end);

    char digits[32];
    memcpy(digits, path + start, (size_t)*width);
    digits[*width] = '\0';
    *first_number = strtol(digits, NULL, 10);
    return 1;
}


int main(int argc, char* argv[])
{
    if (argc != 4)
    {
        fprintf(stderr, "usage: %s <first_picture.png> <program_out.txt> <number_of_frames>\n", argv[0]);
        fprintf(stderr, "example: %s frames/frame_0001.png out.txt 250\n", argv[0]);
        return 1;
    }

    char* endptr;
    long number_of_frames = strtol(argv[3], &endptr, 10);
    if (*argv[3] == '\0' || *endptr != '\0' || number_of_frames <= 0)
    {
        fprintf(stderr, "bad number_of_frames: %s\n", argv[3]);
        return 1;
    }

    char prefix[PATH_MAX_LEN], suffix[PATH_MAX_LEN];
    long first_number = 0;
    int width = 0;

    if (!split_frame_path(argv[1], prefix, sizeof(prefix), suffix, sizeof(suffix), &first_number, &width))
    {
        fprintf(stderr, "%s: can't find frame number in file name (expected ...<digits>.png)\n",  argv[1]);
        return 1;
    }

    FILE* out = fopen(argv[2], "w");
    if (out == NULL)
    {
        perror(argv[2]);
        return 1;
    }

    unsigned char* prev = calloc(VRAM_W * VRAM_H, 1);
    if (prev == NULL) { fclose(out); return 1; }

    char frame_name[PATH_MAX_LEN];

    for (long i = 0; i < number_of_frames; i++)
    {
        long frame_no = first_number + i;

        int n = snprintf(frame_name, sizeof(frame_name), "%s%0*ld%s", prefix, width, frame_no, suffix);
        if(n < 0 || (size_t)n >= sizeof(frame_name))
        {
            fprintf(stderr, "frame path too long\n");
            fclose(out);
            return 1;
        }

        int w = 0, h = 0, channels = 0;
        unsigned char* img = stbi_load(frame_name, &w, &h, &channels, 4);
        if (img == NULL)
        {
            fprintf(stderr, "%s: %s\n", frame_name, stbi_failure_reason());
            fclose(out);
            return 1;
        }

        asm_generation(img, w, h, out, frame_name, prev);

        fprintf(out, "   PUSH 10\n   SLEEP\n");

        stbi_image_free(img);
    }

    fprintf(out, "   HLT\n");
    free(prev);

    if (fclose(out) != 0) { perror(argv[2]); return 1; }
    return 0;
}