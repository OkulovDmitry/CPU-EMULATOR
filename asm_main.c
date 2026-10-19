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

#define MAX_LABELS 128   // максимальное число меток в массиве меток
#define LABEL_LEN  32    // максимальная длина имени метки

//========= Label - структура одной метки ==========
typedef struct
{
    char   name[LABEL_LEN];   // имя   метки
    int    addr;              // адрес метки
} Label;
//==================================================

//=== LabelTable - структура с таблицей меток ===
typedef struct
{
    Label  items[MAX_LABELS]; // массив     меток
    int    labels_count;      // количество меток
} LabelTable;
//===============================================

//=========== label_find - функция поиска метки =============
static int label_find(const LabelTable* t, const char* name)
{
    for (int i = 0; i < t->labels_count; i++)
    {
        if (strcmp(t->items[i].name, name) == 0)
            return t->items[i].addr;
    }

    return -1;
}
//============================================================

//============= lable_add - добавляет метку в массив меток ================
static bool label_add(LabelTable* t, const char* name, int addr)
{
    if (t->labels_count >= MAX_LABELS) return false;
    if (strlen(name)    >= LABEL_LEN)  return false;
    if (label_find(t, name) != -1)     return false;

    strcpy(t->items[t->labels_count].name, name);
    t->items[t->labels_count].addr = addr;
    t->labels_count++;

    return true;
}
//==========================================================================

//============================= label_name_ok - проверяет валидность имени метки =================================
// имя начинается на _ или букву, далее буквы и цифры и _, имя метки не совпадает с именами функций, регистров
static bool label_name_ok(const char* name)
{
    if (!(isalpha((unsigned char)name[0])) && name[0] != '_') return false;
    for (const char *p = name; *p; p++)
        if (!(isalnum((unsigned char)*p)) && *p != '_') return false;
    
    return find_command(name)  == -1 &&
           find_register(name) == -1;
}
//=================================================================================================================

code_t* assembler(const char* path, int* size); // функция ассемблер

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "usage: %s <text_in> <bytecode_out>\n", argv[0]);
        return 1;
    }

    int size = 0;
    code_t* bytecode = assembler(argv[1], &size);
    if (bytecode == NULL)
    {
        return 1;
    }

    int res = bytecode_write(argv[2], bytecode, size);

    free(bytecode);
    return res == 0 ? 0 : 1;
}

//=============================== Функция ассемблер ====================================
code_t* assembler(const char* path, int* size)
{
    FILE* in = fopen(path, "r");
    if (in == NULL)
    {
        perror(path);
        return NULL;
    }

    int capacity = 64;                                                    // первоначальный размер байт-кода
    code_t* bytecode = malloc(capacity * sizeof(code_t));
    if (bytecode == NULL)
    {
        fclose(in);
        fprintf(stderr, "bytecode out of memory in file %s\n", path);
        return NULL;
    }

    LabelTable labels = { .labels_count = 0 };
    int count         = 0;                      // количество команд и аргументов в bytecode

    for (int pass = 1; pass <= 2; pass++)       // pass - номер прохода ассемблера
    {
        rewind(in);

        count = 0;
        int lineno = 0; //line number
        char line[256];

        while (fgets(line, sizeof(line), in) != NULL)
        {
            lineno++;
            
            char* comment = strchr(line, ';');
            if (comment != NULL) *comment = '\0';                    //обрезка комментариев

            int used = 0;
            char word[64] = "POISON";

            if (sscanf(line, "%63s%n", word, &used) != 1) continue;  // пропускаем пустые строки

            if (word[strlen(word) - 1] == ':')
            {
                word[strlen(word) - 1] = '\0';

                if (!(label_name_ok(word)))
                {
                    fprintf(stderr, "%s:%d: bad label name '%s'\n", path, lineno, word);
                    free(bytecode);
                    fclose(in);
                    return NULL;
                }

                if (pass == 1 && !(label_add(&labels, word, count)))
                {
                    fprintf(stderr, "%s:%d: duplicate or too long label '%s'\n", path, lineno, word);
                    free(bytecode);
                    fclose(in);
                    return NULL;
                }

                char* end_of_label = line + used;
                while (isspace((unsigned char)*end_of_label)) end_of_label++;
                if (*end_of_label  != '\0' && *end_of_label != '\n')
                {
                    fprintf(stderr, "%s:%d: unexpected '%s' after label\n", path, lineno, end_of_label);
                    free(bytecode);
                    fclose(in);
                    return NULL;
                }

                continue;
            }

            int op = find_command(word);
            if (op == -1)
            {
                fprintf(stderr, "%s:%d: unknown command '%s'\n", path, lineno, word);
                free(bytecode);
                fclose(in);
                return NULL;
            }

            if (count + 3 > capacity)
            {
                code_t* bytecode_tmp = realloc(bytecode, 2 * capacity * sizeof(code_t));
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

            int flags   = 0;
            int operand = 0;

            if (isa[op].allowed != 0)
            {
                char arg_text[64];
                int   arg_used     = 0;
                
                if (sscanf(line + used, "%63s %n", arg_text, &arg_used) != 1)
                {
                    fprintf(stderr, "%s:%d: missing argument for %s\n", path, lineno, word);
                    free(bytecode);
                    fclose(in);
                    return NULL;
                }

                used += arg_used;

                int reg = find_register(arg_text);
                if (reg != -1 && (isa[op].allowed & BIT_REG))
                {
                    flags = BIT_REG;
                    operand = reg;
                }
                else
                {
                    char* endp = NULL;
                    operand    = strtol(arg_text, &endp, 10);

                    if (!(endp == arg_text || *endp != '\0'))
                    {
                        if (!(isa[op].allowed & BIT_IMM))
                        {
                            fprintf(stderr, "%s:%d: %s does not accept a number\n", path, lineno, word);
                            free(bytecode);
                            fclose(in);
                            return NULL;
                        }
                        flags = BIT_IMM;
                    }
                    else if(isa[op].is_jump)
                    {
                        if (pass == 1)
                        {
                            operand = 0;   // адрес неизвестен
                        }
                        else
                        {
                            int addr = label_find(&labels, arg_text);

                            if (addr == -1)
                            {
                                fprintf(stderr, "%s:%d: unknown label '%s'\n", path, lineno, arg_text);
                                free(bytecode);
                                fclose(in);
                                return NULL;
                            }

                            operand = addr;
                        }
                        flags = BIT_IMM;
                    }
                    else
                    {
                        fprintf(stderr, "%s:%d: bad argument '%s' for %s\n", path, lineno, arg_text, word);
                        free(bytecode);
                        fclose(in);
                        return NULL;
                    }
                }
            }

            bytecode[count++] = (code_t)(op | flags);
            if (flags != 0) bytecode[count++] = (code_t)operand;


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
    }

    fclose(in);
    *size = count;
    return bytecode;
}
//========================================================================================