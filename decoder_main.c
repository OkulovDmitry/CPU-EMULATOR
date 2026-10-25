#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <stdbool.h>
#include "bytecode.h"
#include "isa.h"

#define BITS_WIDTH       71

int decoder(const char* path, const char* decode_txt);  // функция декодер
int hex_dump(const char* path, const char* dump_txt);
void print_bits(FILE* out, uint64_t value);

typedef struct
{
    int op;
    int flag;
    int need_reg;
    int need_imm;
} DecodeState;


int main(int argc, char* argv[])
{
    if (argc == 4 && strcmp(argv[1], "-hex") == 0)
    {
        return hex_dump(argv[2], argv[3]);
    }
    if (argc != 3)
    {
        fprintf(stderr, "usage: %s [-x] <bytecode_in> <text_out>\n"
                        "  without -x: table of cells in binary with explanation\n"
                        "  with    -x: hex dump of the whole file, two characters per byte\n", argv[0]);
        return 1;
    }

    return decoder(argv[1], argv[2]);
}

static void double_text(char* buf, size_t size, double d)
{
    for (int prec = 5; prec <= 17; prec++)
    {
        snprintf(buf, size, "%.*g", prec, d);
        if (strtod(buf, NULL) == d) return;
    }
}

static void explain(FILE* out, code_t cell, DecodeState* st)
{
    if (st->need_reg)
    {
        st->need_reg = 0;
        if (cell >= REG_COUNT)       fprintf(out, " | BAD register number");
        else if (st->flag & BIT_MEM) fprintf(out, " | memory cell, its number is in register %s", reg_names[cell]);
        else                         fprintf(out, " | register %s", reg_names[cell]);
        return;
    }
    if (st->need_imm)
    {
        st->need_imm = 0;
        if (st->flag & BIT_FLT)
        { 
            char buf[32];
            double_text(buf, sizeof(buf), cell_to_double(cell));
            fprintf(out, " | number %s (double, bits are not an integer)", buf);
        }
        else if (st->flag & BIT_MEM) fprintf(out, " | memory cell number %llu", (unsigned long long)cell);
        else if(isa[st->op].is_jump) fprintf(out, " | jump to celll %llu",      (unsigned long long)cell);
        else                         fprintf(out, " | integer %llu",            (unsigned long long)cell);
        return;
    }

    if(!(isa_cell_ok(cell)))
    {
        fprintf(out, " | BAD cell, not a valid command");
        return;
    }

    st->op       = CELL_OP(cell);
    st->flag     = CELL_FLAGS(cell);
    st->need_reg = (st->flag & BIT_REG) != 0;
    st->need_imm = (st->flag & BIT_IMM) != 0;

    fprintf(out, " | command %s", isa[st->op].name);
    if (st->flag & BIT_IMM) fprintf(out, " IMM");
    if (st->flag & BIT_REG) fprintf(out, " REG");
    if (st->flag & BIT_MEM) fprintf(out, " MEM");
    if (st->flag & BIT_FLT) fprintf(out, " FLT");
}

void print_bits(FILE* out, uint64_t value)
{
    int printed = 0;

    for (int i = 63; i >= 0; i--)
    {
        fputc(((value >> i) & 1u) ? '1' : '0', out);
        printed++;
        if (i % 8 == 0 && i != 0) { fputc(' ', out); printed++; }
    }
    for (; printed < BITS_WIDTH; printed++) { fputc(' ', out); }
}

int decoder(const char* path, const char* decode_txt)
{
    FILE* in = fopen(path, "rb");
    if (!in) { perror(path); return -1; }

    FILE* decode = fopen(decode_txt, "w");
    if (!decode) { perror(decode_txt); fclose(in); return -1; }

    BytecodeHeader header;
    if (fread(&header, sizeof(header), 1, in) != 1)
    {
        fprintf(stderr, "%s: file is too short for a header\n", path);
        fclose(in);
        return -1;
    }

fprintf(decode, "magic = 0x%08X, major = %u, minor = %u, count = %u\n",
        header.magic, header.major, header.minor, header.count);

    DecodeState st = {0};
    code_t cell;
    size_t index = 0;
    while (fread(&cell, sizeof(code_t), 1, in) == 1) {
        fprintf(decode, "%04zu ", index++);
        print_bits(decode, cell);
        // Простая шестнадцатеричная или числовая расшифровка справа от |
        fprintf(decode, "| 0x%016llX (%lld)", (unsigned long long)cell, (long long)cell);

        int count = snprintf(NULL, 0, "%lld", (long long)cell);
        for (int i = 0; i < 22 - count; i++) fprintf(decode, " ");

        explain(decode, cell, &st);
        fputc('\n', decode);
    }

    fclose(in);
    fclose(decode);
    return 0;
}

int hex_dump(const char* path, const char* dump_txt)
{
    FILE* in = fopen(path, "rb");
    if (!in) { perror(path); return -1; }

    FILE* dump = fopen(dump_txt, "w");
    if (!dump) { perror(dump_txt); fclose(in); return -1; }

    unsigned char buf[16];
    size_t offset = 0;
    size_t n      = 0;

    while ((n = fread(buf, 1, sizeof(buf), in)) > 0)
    {
        fprintf(dump, "%09zx  ", offset);

        for (size_t i = 0; i < sizeof(buf); i++)
        {
            if (i < n)  fprintf(dump, "%02x ", buf[i]);
            else        fprintf(dump, "   ");
            if (i == 7) fputc(' ', dump);   
        }

        fprintf(dump, " |");
        for (size_t i = 0; i < n; i++)
        {
            fputc((buf[i] >= 32 && buf[i]  < 127) ? (char)buf[i] : '.', dump);
        }
        fprintf(dump, "|\n");

        offset += n;
    }
    
    fprintf(dump, "%08zx\n", offset);

    fclose(in);
    fclose(dump);
    return 0;
}