/*=====================================================================================================================================================================================*/
/*=====================================================================================================================================================================================*/
/*=====================================================================================================================================================================================*/
//                                                                                 Disassembler
/*=====================================================================================================================================================================================*/
/*=====================================================================================================================================================================================*/
/*=====================================================================================================================================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "isa.h"
#include "bytecode.h"

int     disassembler(  const char* path, const Elem_t* code, const int size);  // функция дизассемблер

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "usage: %s <bytecode_in> <text_out>\n", argv[0]);
        return 1;
    }

    int size = 0;
    Elem_t* bytecode = bytecode_read(argv[1], &size);
    if (bytecode == NULL)
    {
        return 1;
    }

    int res = disassembler(argv[2], bytecode, size);

    free(bytecode);
    return res == 0 ? 0 : 1;
}

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
            fprintf(stderr, "disasm: bad opcode " ELEM_PRINT " at %i\n", code[ind], ind);
            fclose(out);
            return -1;
        }
        int op = (int)code[ind++];
        fprintf(out, "%s", isa[op].name);
        if (isa[op].arg != ARG_NONE)
        {
            if (ind >= size)
            {
                fprintf(stderr, "disasm: missing argument for %s\n", isa[op].name);
                fclose(out);
                return -1;
            }
            fprintf(out, " "ELEM_PRINT, code[ind++]);
        }
        fprintf(out, "\n");
    }

    fclose(out);
    return 0;
}
//========================================================================================