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

int     disassembler(  const char* path, const code_t* code, const int size);  // функция дизассемблер

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "usage: %s <bytecode_in> <text_out>\n", argv[0]);
        return 1;
    }

    int size = 0;
    code_t* bytecode = bytecode_read(argv[1], &size);
    if (bytecode == NULL)
    {
        return 1;
    }

    int res = disassembler(argv[2], bytecode, size);

    free(bytecode);
    return res == 0 ? 0 : 1;
}

//=============================== Функция дизассемблер ====================================
int disassembler(const char* path, const code_t* code, const int size)
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
        code_t cell = code[ind];

        if(!(isa_cell_ok(cell)))
        {
            fprintf(stderr, "disasm: bad opcode %d at %i\n", code[ind], ind);
            fclose(out);
            return -1;
        }

        int op    = CELL_OP(cell);
        int flags = CELL_FLAGS(cell);
        ind++;

        fprintf(out, "%s", isa[op].name);

        int n = operand_cells(flags);
        if (ind + n > size)
        {
            fprintf(stderr, "disasm: missing argument for %s\n", isa[op].name);
            fclose(out);
            return -1;
        }

        if (flags & BIT_REG)
        {
            int reg = code[ind++];

            if (reg < 0 || reg > REG_COUNT)
            {
                fprintf(stderr, "disasm: bad register %d\n", reg);
                fclose(out);
                return -1;
            }

            fprintf(out, " %s", reg_names[reg]);
        }

        if (flags & BIT_IMM) 
            fprintf(out, " %d", code[ind++]);

        fprintf(out, "\n");
    }

    fclose(out);
    return 0;
}
//========================================================================================