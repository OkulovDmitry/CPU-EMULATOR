#include "bytecode.h"

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
            fprintf(stderr, "%s: bad opcode " ELEM_PRINT "\n", path, value);
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
        if (isa[op].arg != ARG_NONE)
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
        fprintf(out, ELEM_FILE "%c", code[i], i + 1 < size ? ' ' : '\n');

    fclose(out);
    return 0;
}
//=============================================================================