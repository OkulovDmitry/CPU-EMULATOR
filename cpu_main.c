#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "isa.h"
#include "bytecode.h"
#include "display.h"
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
    cpu_err_t err = cpu_ctor(&cpu, size CPU_PLACE_OUT("FIRST_CPU"));
    if (err != CPU_OK)
    {
        free(bytecode);
        fprintf(stderr, "cpu error: %s\n", cpu_strerror(err));
        return 1;
    }

    if (display_init(VRAM_W, VRAM_H) != 0)
    {
        cpu_dtor(&cpu);
        free(bytecode);
        fprintf(stderr, "display error: cannot init screen\n");
        return 1;
    }

    cpu_err_t run_err  = cpu_run (&cpu, bytecode, size);

    // Процессор уже остановился (HLT), но держим окно открытым и ждем действий пользователя
    display_wait_close(cpu.ram->vram, VRAM_W, VRAM_H);
    display_close();
    cpu_err_t dtor_err = cpu_dtor(&cpu);

    if (dtor_err != CPU_OK)
        fprintf(stderr, "cpu error: %s\n", cpu_strerror(dtor_err));

    free(bytecode);
    return (run_err == CPU_OK && dtor_err == CPU_OK) ? 0 : 1;
}