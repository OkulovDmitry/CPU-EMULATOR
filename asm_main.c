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

//===== asm_err_t коды ассемблерных ошибок =====
typedef enum
{
    ASM_OK                          = 0,
    ASM_NO_MEMORY                   = 1,
    ASM_WRONG_LABEL_NAME_FORMAT     = 2,
    ASM_IMPOSSIBLE_LABEL_NAME       = 3,
    ASM_GARBAGE_AFTER_LABEL         = 4,
    ASM_GARBAGE_IN_END_OF_FILE      = 5,
    ASM_UNKNOWN_COMMAND             = 6,
    ASM_MISSING_ARG_AFTER_COMMAND   = 7,
    ASM_NOT_ACCEPT_MEMORY           = 8,    // команда не рабтает с видеопамятью, а попытка заставить её это сделать есть
    ASM_NOT_ACCEPT_REGISTER         = 9,
    ASM_NOT_ACCEPT_NUMBER           = 10,
    ASM_BAD_ARG                     = 11,
    ASM_BAD_MEMORY_ADDR             = 12,
    ASM_UNKNOWN_LABEL               = 13,
} asm_err_t;

//==============================================

const char* asm_strerror(asm_err_t err)
{
    switch(err)
    {
        case ASM_OK:                          return "ASM OK";
        case ASM_NO_MEMORY:                   return "bytecode out of memory in file";
        case ASM_WRONG_LABEL_NAME_FORMAT:     return " bad label name";
        case ASM_IMPOSSIBLE_LABEL_NAME:       return " duplicate or too long label";
        case ASM_GARBAGE_AFTER_LABEL:         return " unexpected after label";
        case ASM_GARBAGE_IN_END_OF_FILE:      return " unexpected";
        case ASM_UNKNOWN_COMMAND:             return " unknown command";
        case ASM_MISSING_ARG_AFTER_COMMAND:   return " missing argument for ";
        case ASM_NOT_ACCEPT_MEMORY:           return " this command does not accept a memory operand";   // команда не рабтает с видеопамятью, а попытка заставить её это сделать есть
        case ASM_NOT_ACCEPT_REGISTER:         return " this command does not accept a register";
        case ASM_NOT_ACCEPT_NUMBER:           return " does not accept a number";
        case ASM_BAD_ARG:                     return " bad argument";
        case ASM_BAD_MEMORY_ADDR:             return " bad memory address";
        case ASM_UNKNOWN_LABEL:               return " unknown label";
        default:                              return "unknown error code";
    }
}
//===================================================================

static code_t* error_abort(const char* path, int lineno, int err, const char* word, const char* arg_text, code_t* bytecode, FILE* in)
{
    if (err == ASM_BAD_MEMORY_ADDR || 
        err == ASM_BAD_ARG) fprintf(stderr, "%s:%d:%s '%s' for %s\n",  path, lineno, asm_strerror(err), arg_text, word);
    else                            fprintf(stderr, "%s:%d:%s '%s'\n",         path, lineno, asm_strerror(err),           word);
    free(bytecode);
    fclose(in);
    return NULL;
}


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
        fprintf(stderr, "%s %s\n", asm_strerror(ASM_NO_MEMORY), path);
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

                if (!(label_name_ok(word)))                          return error_abort(path, lineno, ASM_WRONG_LABEL_NAME_FORMAT, word, "", bytecode, in);

                if (pass == 1 && !(label_add(&labels, word, count))) return error_abort(path, lineno, ASM_IMPOSSIBLE_LABEL_NAME, word, "", bytecode, in);

                char* end_of_label = line + used;
                while (isspace((unsigned char)*end_of_label)) end_of_label++;
                if (*end_of_label  != '\0' && *end_of_label != '\n') return error_abort(path, lineno, ASM_GARBAGE_AFTER_LABEL, end_of_label, "", bytecode, in);

                continue;
            }

            int op = find_command(word);
            if (op == -1)                                            return error_abort(path, lineno, ASM_UNKNOWN_COMMAND, word, "", bytecode, in);

            if (count + 3 > capacity)
            {
                code_t* bytecode_tmp = realloc(bytecode, 2 * capacity * sizeof(code_t));
                if (bytecode_tmp == NULL)                            return error_abort(path, lineno, ASM_NO_MEMORY, "bytecode_tmp", "", bytecode, in);
                capacity *= 2;
                bytecode = bytecode_tmp;
            }

            int flags   = 0;
            code_t operand = 0;

            if (isa[op].allowed != 0)
            {
                char arg_text[64];
                int   arg_used     = 0;
                
                if (sscanf(line + used, "%63s %n", arg_text, &arg_used) != 1) return error_abort(path, lineno, ASM_MISSING_ARG_AFTER_COMMAND, word, "", bytecode, in);

                used += arg_used;

                bool is_memory = is_mem(arg_text);
                int reg = find_register(arg_text);

                if (is_memory && !(isa[op].allowed & BIT_MEM))                return error_abort(path, lineno, ASM_NOT_ACCEPT_MEMORY, word, "",  bytecode, in);

                if (reg != -1)
                {
                    if (!(isa[op].allowed & BIT_REG))                         return error_abort(path, lineno, ASM_NOT_ACCEPT_REGISTER, word, "", bytecode, in);
                    flags   = (is_memory ? BIT_MEM : 0) | BIT_REG;
                    operand = (code_t)reg;
                }
                else if (is_memory)
                {
                    char*     endp = NULL;
                    long long addr = strtoll(arg_text, &endp, 10);

                    if (endp == arg_text || *endp != '\0' || addr < 0)       return error_abort(path, lineno, ASM_BAD_MEMORY_ADDR, word, arg_text, bytecode, in);

                    flags   = BIT_MEM | BIT_IMM;
                    operand = (code_t)addr;
                }
                else if (isa[op].is_jump)
                {
                    char* endp = NULL;
                    operand    = strtoll(arg_text, &endp, 10);

                    if (!(endp == arg_text || *endp != '\0'))
                    {
                        if (!(isa[op].allowed & BIT_IMM))                    return error_abort(path, lineno, ASM_NOT_ACCEPT_NUMBER, word, "", bytecode, in);
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

                            if (addr == -1)                                  return error_abort(path, lineno, ASM_UNKNOWN_LABEL, word, arg_text, bytecode, in);

                            operand = addr;
                        }
                        flags = BIT_IMM;
                    }
                    else                                                     return error_abort(path, lineno, ASM_BAD_ARG, word, arg_text, bytecode, in);
                }
                else
                {
                    char* endp = NULL;
                    double d   = strtod(arg_text, &endp);

                    if (endp == arg_text || *endp != '\0')                   return error_abort(path, lineno, ASM_BAD_ARG, word, arg_text, bytecode, in);
                    
                    if (!(isa[op].allowed & BIT_FLT))                        return error_abort(path, lineno, ASM_NOT_ACCEPT_NUMBER, word, "", bytecode, in);

                    operand = double_to_cell(d);
                    flags = BIT_IMM | (isa[op].allowed & BIT_FLT);
                }
            }

            if (!isa_cell_ok((code_t)(op | flags)))
            {
                fprintf(stderr, "%s:%d: invalid operand for %s (cell=%" PRIu64 ", flags=%d, allowed=%d)\n", path, lineno, word, (code_t)(op | flags), flags, isa[op].allowed);
                free(bytecode);
                fclose(in);
                return NULL;
            }

            bytecode[count++] = (code_t)(op | flags);
            if (flags != 0) bytecode[count++] = (code_t)operand;


            char* end = line + used;                                            // здесь проверка что нет мусора после основной части строки
            while(isspace((unsigned char)(*end))) end++;
            if (*end != '\0')
            {
                fprintf(stderr, "%s:%d: unexpected '%s'\n", path, lineno, end);
                return error_abort(path, lineno, ASM_GARBAGE_IN_END_OF_FILE, end, "", bytecode, in);
            }
        }

        if(ferror(in))
        {
            fprintf(stderr, "asm: error reading %s\n", path);
            perror(path);
            free(bytecode);
            fclose(in);
            return NULL;
        }
    }

    fclose(in);
    *size = count;
    return bytecode;
}
//========================================================================================