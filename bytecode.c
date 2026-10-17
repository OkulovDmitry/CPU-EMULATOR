#include "bytecode.h"

//============ is_valid_opcode - проверяет валидность команды к процессору ============
static bool is_valid_opcode(Elem_t v)
{
    return v >= 0 &&
           v < CMD_COUNT &&
           v == (Elem_t)(int)v;
}
//=====================================================================================

//============ bytecode_read - производит полное чтение файла с байт кодом и производит его запись в Elem_t* ============
Elem_t* bytecode_read(const char* path, int* size)
{
    FILE* in = fopen(path, "r");
    if (in == NULL) { perror(path); return NULL; }

    int capacity = 64;
    Elem_t* bytecode = malloc(capacity * sizeof(Elem_t));
    if (bytecode == NULL)
    {
        fclose(in);
        fprintf(stderr, "bytecode out of memory in file %s\n", path);
        return NULL;
    }

    int    count = 0;
    Elem_t value = 0;
    while (fscanf(in, ELEM_FMT, &value) == 1)
    {
        if (!(is_valid_opcode(value)))
        {
            fprintf(stderr, "%s: bad opcode " ELEM_FMT "\n", path, value);
            free(bytecode);
            fclose(in);
            return NULL;
        }
        if (count + 2 > capacity)
        {
            Elem_t* bytecode_tmp = realloc(bytecode, 2 * capacity * sizeof(Elem_t));
            if (bytecode_tmp == NULL)
            {
                free(bytecode);
                fclose(in);
                fprintf(stderr, "bytecode out of memory in file %s\n", path);
                return NULL;
            }
            capacity *= 2;
            bytecode = bytecode_tmp;
        }

        int op            = (int)value;
        bytecode[count++] =      value;
        if (isa[op].has_arg == 1)
        {
            Elem_t arg = 0;

            if (fscanf(in, ELEM_FMT, &arg) != 1)
            {
                free(bytecode);
                fclose(in);
                fprintf(stderr, "error in read of %s, bytecode_read() cant read end of file\n", path);
                return NULL;
            }
            bytecode[count++] = arg;
        }
    }

    if(!(feof(in)))
    {
        free(bytecode);
        fprintf(stderr, "error in read of %s, bytecode_read() cant read end of file\n", path);
        fclose(in);
        return NULL;
    }

    fclose(in);
    *size = count;
    return bytecode;
}
//=======================================================================================================================

//============ bytecode_write - производит запись байт кода в файл ============
int bytecode_write(const char* path, const Elem_t* code, const int size)
{
    FILE* out = fopen(path, "w");
    if (out == NULL) { perror(path); return -1; }

    for (int i = 0; i < size; i++)
        fprintf(out, ELEM_FMT "%c", code[i], i + 1 < size ? ' ' : '\n');

    fclose(out);
    return 0;
}
//=============================================================================

//=============================== Функция дизассемблер ====================================
int disassembler(const char* path, const Elem_t* code, const int size)
{
    FILE* out = fopen(path, "w");
    if (out == NULL)
    {
        perror(path);
        return -1;
    }

    int ind = 0;

    while (ind < size)
    {
        if(!(is_valid_opcode(code[ind])))
        {
            fprintf(stderr, "disasm: bad opcode " ELEM_FMT " at %i\n", code[ind], ind);
            fclose(out);
            return -1;
        }
        int op = (int)code[ind++];
        fprintf(out, "%s", isa[op].name);
        if (isa[op].has_arg)
        {
            if (ind >= size)
            {
                fprintf(stderr, "disasm: missing argument for %s\n", isa[op].name);
                fclose(out);
                return -1;
            }
            fprintf(out, " "ELEM_FMT, code[ind++]);
        }
        fprintf(out, "\n");
    }

    fclose(out);
    return 0;
}
//========================================================================================

//============= Функция распознания комманды =============
static int find_command(const char* word)
{
    for (int i = 0; i < CMD_COUNT; i++)
    {
        if (strcmp(word, isa[i].name) == 0) 
            return i;
    }
    return -1;
}
//========================================================

//=============================== Функция ассемблер ====================================
Elem_t* assembler(const char* path, int* size)
{
    FILE* in = fopen(path, "r");
    if (in == NULL)
    {
        perror(path);
        return NULL;
    }

    int capacity = 64;
    Elem_t* bytecode = malloc(capacity * sizeof(Elem_t));
    if (bytecode == NULL)
    {
        fclose(in);
        fprintf(stderr, "bytecode out of memory in file %s\n", path);
        return NULL;
    }

    int count = 0;
    char value[64] = "POISON";
    while (fscanf(in, "%63s", value) == 1)
    {
        int op = find_command(value);
        if (op == -1)
        {
            fprintf(stderr, "asm: bad command %s at %i\n", value, count + 1);
            fclose(in);
            return NULL;
        }

        if (count + 2 > capacity)
        {
            Elem_t* bytecode_tmp = realloc(bytecode, 2 * capacity * sizeof(Elem_t));
            if (bytecode_tmp == NULL)
            {
                free(bytecode);
                fclose(in);
                fprintf(stderr, "bytecode out of memory in file %s\n", path);
                return NULL;
            }
            capacity *= 2;
            bytecode = bytecode_tmp;
        }

        bytecode[count++] = (Elem_t)op;

        if (isa[op].has_arg == 1)
        {
            Elem_t arg = 0;

            if (fscanf(in, ELEM_FMT, &arg) != 1)
            {
                fprintf(stderr, "asm: missing argument for %s\n", isa[op].name);
                free(bytecode);
                fclose(in);
                return NULL;
            }

            bytecode[count++] = arg;
        }
    }

    if(!(feof(in)))
    {
        free(bytecode);
        fprintf(stderr, "error in read of %s, assembler() cant read end of file\n", path);
        fclose(in);
        return NULL;
    }

    fclose(in);
    *size = count;
    return bytecode;
}
//========================================================================================