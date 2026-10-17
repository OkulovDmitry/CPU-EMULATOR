#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "isa.h"
#include "bytecode.h"
#include "cpu.h"

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <bytecode_in>\n", argv[0]);
        return 1;
    }

    int size = 0;
    Elem_t* bytecode = bytecode_read(argv[1], &size);
    if(bytecode == NULL)
    {
        return 1;
    }

    CPU cpu;
    cpu_err_t err = cpu_ctor(&cpu, bytecode, size);
    if (err != CPU_OK)
    {
        free(bytecode);
        cpu_strerror(err);
        return 1;
    }

    cpu_err_t run_err = cpu_run(&cpu);

    err = cpu_dtor(&cpu);
    if (err != CPU_OK)
    {
        free(bytecode);
        cpu_strerror(err);
        return 1;
    }
    free(bytecode);

    if (run_err != CPU_OK)
    {
        return 1;
    }
    
    return 0;
}