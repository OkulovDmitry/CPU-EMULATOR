/*=====================================================================================================================================================================================*/
/*=====================================================================================================================================================================================*/
/*=====================================================================================================================================================================================*/
//                                                                           Central Processing Unit
/*=====================================================================================================================================================================================*/
/*=====================================================================================================================================================================================*/
/*=====================================================================================================================================================================================*/

#ifndef CPU_H
#define CPU_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "stack.h"
#include "isa.h"
#include "display.h"

#define CPUDEBUG

#ifdef CPUDEBUG
    #define CPU_PLACE_OUT(...) , __VA_ARGS__, __func__, __FILE__, __LINE__
    #define CPU_PLACE_IN , const char* name, const char* function, const char* file, int line
#else 
    #define CPU_PLACE_OUT(...)
    #define CPU_PLACE_IN
#endif

#define CALL_STACK_SIZE 256

#define VRAM_W       16                  // ширина экрана в ячейках
#define VRAM_H       16                  // высота экрана в ячейках
#define VRAM_SIZE    (VRAM_W * VRAM_H)

#define CPU_SEP "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n"

//===== cpu_err_t коды процессорных ошибок =====
typedef enum
{
    CPU_OK              = 0,
    CPU_STACK_FAIL      = 1,
    CPU_DIV_BY_ZERO     = 2,
    CPU_UNKNOWN_OPCODE  = 3,
    CPU_MISSING_ARG     = 4,
    CPU_PC_OUT_OF_RANGE = 5,
    CPU_BAD_REGISTER    = 6,
    CPU_BAD_ADDRESS     = 7,
    CPU_CALL_OVERFLOW   = 8,
    CPU_RET_EMPTY       = 9,
    CPU_BAD_MEMORY      = 10,
    CPU_NO_MEMORY       = 11,
} cpu_err_t;
//==============================================



typedef struct
{
    code_t*     code;
    int         call_stack[CALL_STACK_SIZE];  // адреса возврата
    int         call_sp;                      // количество адресов в call стеке
    err_t       stack_err;                    // последняя ошибка стека
    Elem_t      vram[VRAM_SIZE];              // массив видеопамяти, PUSHM [4] - запись 67 в 4 ячейку POPM [8] - перезапись 67 из 4 в 8 ячейку
} RAM;


//============ CPU - структура нашего процессора =============
typedef struct
{
    my_stack_t* stack;
    Elem_t      regs[REG_COUNT];              // массив регистров процессора
    int         code_size;
    int         pc;                           // текущая исполняемая байт-кодом команда
    int         running;                      // флаг на запуск программы
    RAM*        ram;                          // оперативная память моего процессора

#ifdef CPUDEBUG
    const char* name;
    const char* function;
    const char* file;
    int line;
#endif
} CPU;
//=============================================================

cpu_err_t   cpu_ctor(CPU* cpu, int code_size CPU_PLACE_IN);      // функция создающая        cpu
cpu_err_t   cpu_dtor(CPU* cpu);                                  // функция разрушающая      cpu
cpu_err_t   cpu_run(CPU* cpu, code_t* code, int size);           // функция запускающая      cpu
const char* cpu_strerror(cpu_err_t err);                         // функция обработки ошибок cpu
void cpu_dump_to(FILE* file, const CPU* cpu, const char* name, int line, const char* function, const char* file_name);
void cpu_dump(const char* out, const CPU* cpu, const char* name, int line, const char* function, const char* file_name);

#endif //CPU_H