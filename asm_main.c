/*=====================================================================================================================================================================================*/
/*=====================================================================================================================================================================================*/
/*=====================================================================================================================================================================================*/
//                                                                                 Assembler
/*=====================================================================================================================================================================================*/
/*=====================================================================================================================================================================================*/
/*=====================================================================================================================================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include "isa.h"
#include "bytecode.h"

Elem_t* assembler(const char* path, int* size); // функция ассемблер

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "usage: %s <text_in> <bytecode_out>\n", argv[0]);
        return 1;
    }

    int size = 0;
    Elem_t* bytecode = assembler(argv[1], &size);
    if (bytecode == NULL)
    {
        return 1;
    }

    int res = bytecode_write(argv[2], bytecode, size);

    free(bytecode);
    return res == 0 ? 0 : 1;
}

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

    char line[256];
    int lineno = 0; //line number
    int count  = 0; //количество команд и аргументов в bytecode 
    while (fgets(line, sizeof(line), in) != NULL)
    {
        lineno++;
        
        char* comment = strchr(line, ';');
        if (comment != NULL) *comment = '\0';                    //обрезка комментариев

        int used = 0;
        char word[64] = "POISON";

        if (sscanf(line, "%63s%n", word, &used) != 1) continue;  // пропускаем пустые строки

        int op = find_command(word);
        if (op == -1)
        {
            fprintf(stderr, "%s:%d: unknown command '%s'\n", path, lineno, word);
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

        bytecode[count++] = (Elem_t)op;

        if (isa[op].arg != ARG_NONE)
        {
            Elem_t arg = 0;

            if (sscanf(line, "%63s " ELEM_FMT "%n", word, &arg, &used) != 2)
            {
                fprintf(stderr, "%s:%d: missing argument for %s\n", path, lineno, word);
                free(bytecode);
                fclose(in);
                return NULL;
            }

            bytecode[count++] = arg;
        }

        char* end = line + used;                                            // здесь проверка что нет мусора после основной части строки
        while(isspace((unsigned char)(*end))) end++;
        if (*end != '\0')
        {
            free(bytecode);
            fprintf(stderr, "%s:%d: unexpected '%s'\n", path, lineno, end);
            fclose(in);
            return NULL;
        }
    }

    if(ferror(in))
    {
        free(bytecode);
        fprintf(stderr, "asm: error reading %s\n", path);
        perror(path);
        fclose(in);
        return NULL;
    }

    fclose(in);
    *size = count;
    return bytecode;
}
//========================================================================================