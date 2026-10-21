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
    code_t* bytecode = bytecode_read(argv[1], &size);
    if(bytecode == NULL)
    {
        return 1;
    }

    CPU cpu;
    cpu_err_t err = cpu_ctor(&cpu, size);
    if (err != CPU_OK)
    {
        free(bytecode);
        fprintf(stderr, "cpu error: %s\n", cpu_strerror(err));
        return 1;
    }

    cpu_err_t run_err  = cpu_run (&cpu, bytecode, size);
    cpu_err_t dtor_err = cpu_dtor(&cpu);

    if (dtor_err != CPU_OK)
        fprintf(stderr, "cpu error: %s\n", cpu_strerror(dtor_err));

    free(bytecode);
    return (run_err == CPU_OK && dtor_err == CPU_OK) ? 0 : 1;
}