/*=====================================================================================================================================================================================*/
/*=====================================================================================================================================================================================*/
/*=====================================================================================================================================================================================*/
//                                                                           Central Processing Unit
/*=====================================================================================================================================================================================*/
/*=====================================================================================================================================================================================*/
/*=====================================================================================================================================================================================*/

#include "cpu.h"
#include "isa.h"

//============ cpu_strerror - обрабатывает ошибки cpu ==============
const char* cpu_strerror(cpu_err_t err)
{
    switch(err)
    {
        case CPU_OK:              return "CPU OK";
        case CPU_STACK_FAIL:      return "STACK ERROR IN CPU";
        case CPU_DIV_BY_ZERO:     return "DIVISION BY ZERO IN CPU";
        case CPU_UNKNOWN_OPCODE:  return "UNKNOWN OPERATION CODE";
        case CPU_MISSING_ARG:     return "MISSING ARGUMENT IN CPU";
        case CPU_PC_OUT_OF_RANGE: return "PROGRAM COUNTER CORRUPTED";
        default:                  return "UNKNOWN CPU ERROR";
    }
}
//===================================================================

//========================= cpu_ctor - создаёт cpu =============================
cpu_err_t cpu_ctor(CPU* cpu, Elem_t* code, size_t code_size)
{
    cpu->code       = code;
    cpu->code_size  = code_size;
    cpu->pc         = 0;
    cpu->running    = 1;
    cpu->stack_err  = stack_ctor(&cpu->stack,
                                 16
                                 STACK_PLACE_OUT("cpu.stack"));

    return cpu->stack_err == STACK_OK ? CPU_OK : CPU_STACK_FAIL;
}
//===============================================================================

//=========================== cpu_dtor - разрушает cpu ===============================
cpu_err_t cpu_dtor(CPU* cpu)
{
    cpu->stack_err = stack_dtor(&cpu->stack);

    return cpu->stack_err == STACK_OK ? CPU_OK : CPU_STACK_FAIL;
}

static cpu_err_t pop_two_elements(CPU* cpu, Elem_t* first_pop, Elem_t* second_pop)
{
    cpu->stack_err = stack_pop(&cpu->stack, first_pop);
    if (cpu->stack_err != STACK_OK) return CPU_STACK_FAIL;
    
    cpu->stack_err = stack_pop(&cpu->stack, second_pop);
    if (cpu->stack_err != STACK_OK) return CPU_STACK_FAIL;

    return CPU_OK;
}
//====================================================================================


//=== Тип указателя на функцию-обработчик операции CPU ===
typedef cpu_err_t (*op_func)(CPU* cpu, Elem_t arg);
//========================================================

// ============ op_push - кладёт элемент в стек процессора =============
static cpu_err_t op_push(CPU* cpu, Elem_t arg)
{
    cpu->stack_err = stack_push(&cpu->stack, arg);

    return cpu->stack_err == STACK_OK ? CPU_OK : CPU_STACK_FAIL;
}
//======================================================================

// ============ op_add - складывает два последних элемента в стеке процессора, удаляет их из стека и кладёт результат сложения =============
static cpu_err_t op_add(CPU* cpu, Elem_t arg)
{
    (void)arg;
    Elem_t first_pop, second_pop;

    cpu_err_t err = pop_two_elements(cpu, &first_pop, &second_pop);
    if (err != CPU_OK) return err;

    cpu->stack_err = stack_push(&cpu->stack, first_pop + second_pop);
    return cpu->stack_err == STACK_OK ? CPU_OK : CPU_STACK_FAIL;
}
//===========================================================================================================================================

// ============ op_sub - из предпоследнего элемента стека вычитает последний, удаляет их из стека и кладёт результат вычитания =============
static cpu_err_t op_sub(CPU* cpu, Elem_t arg)
{
    (void)arg;
    Elem_t first_pop, second_pop;

    cpu_err_t err = pop_two_elements(cpu, &first_pop, &second_pop);
    if (err != CPU_OK) return err;

    cpu->stack_err = stack_push(&cpu->stack, second_pop - first_pop);
    return cpu->stack_err == STACK_OK ? CPU_OK : CPU_STACK_FAIL;
}
//==========================================================================================================================================

// ============ op_mul - последний элемент стека умножает на предпоследний, удаляет их из стека и кладёт результат умножения =============
static cpu_err_t op_mul(CPU* cpu, Elem_t arg)
{
    (void)arg;                                                  // шлём компилятор куда подальше
    Elem_t first_pop, second_pop;

    cpu_err_t err = pop_two_elements(cpu, &first_pop, &second_pop);
    if (err != CPU_OK) return err;

    cpu->stack_err = stack_push(&cpu->stack, first_pop * second_pop);
    return cpu->stack_err == STACK_OK ? CPU_OK : CPU_STACK_FAIL;
}
//========================================================================================================================================

// ============ op_div - предпоследний элемент стека делит на последний, удаляет их из стека и кладёт результат деления =============
static cpu_err_t op_div(CPU* cpu, Elem_t arg)
{
    (void)arg;
    Elem_t first_pop, second_pop;

    cpu_err_t err = pop_two_elements(cpu, &first_pop, &second_pop);
    if (err != CPU_OK) return err;
 
    if (fabs(first_pop) < 1e-17) return CPU_DIV_BY_ZERO;
 
    cpu->stack_err = stack_push(&cpu->stack, second_pop / first_pop);
    return cpu->stack_err == STACK_OK ? CPU_OK : CPU_STACK_FAIL;
}
//===================================================================================================================================

// ============ op_out - выводит последний элемент в стеке =============
static cpu_err_t op_out(CPU* cpu, Elem_t arg)
{
    (void)arg;
    Elem_t element;

    cpu->stack_err = stack_pop(&cpu->stack, &element);
    if (cpu->stack_err != STACK_OK) return CPU_STACK_FAIL;
 
    printf(ELEM_PRINT "\n", element);
    return CPU_OK;
}
//======================================================================

// =========== op_hlt - произодит полную остановку процессора ============
static cpu_err_t op_hlt(CPU* cpu, Elem_t arg)
{
    (void)arg;

    cpu->running = 0;

    return CPU_OK;
}
//========================================================================

// ========== Таблица переходов для обработчиков команд процессора ===========
static const op_func handlers[CMD_COUNT] =
{
    [CMD_HLT]  = op_hlt,
    [CMD_PUSH] = op_push,
    [CMD_ADD]  = op_add,
    [CMD_SUB]  = op_sub,
    [CMD_MUL]  = op_mul,
    [CMD_DIV]  = op_div,
    [CMD_OUT]  = op_out,
};
//============================================================================

// ========================================== cpu_run - запускает процессор ==============================================
cpu_err_t cpu_run(CPU* cpu)
{
    while(cpu->running)
    {
        if (cpu->pc < 0 || cpu->pc >= (int)(cpu->code_size))
        {
            fprintf(stderr, "cpu: %s (pc = %d)\n", cpu_strerror(CPU_PC_OUT_OF_RANGE), cpu->pc);
            return CPU_PC_OUT_OF_RANGE;
        }

        Elem_t raw = cpu->code[cpu->pc++];
        if (!(is_valid_opcode(raw))||
            handlers[(int)raw] == NULL)
        {
            fprintf(stderr, "cpu: %s " ELEM_PRINT " (pc = %d)\n", cpu_strerror(CPU_UNKNOWN_OPCODE), raw, cpu->pc - 1);
            return CPU_UNKNOWN_OPCODE;
        }

        int    op  = (int)raw;
        Elem_t arg = 0;

        if (isa[op].arg != ARG_NONE)
        {
            if (cpu->pc >= (int)cpu->code_size)
            {
                fprintf(stderr, "cpu: %s for %s (pc = %d)\n", cpu_strerror(CPU_MISSING_ARG), isa[op].name, cpu->pc);
                return CPU_MISSING_ARG;
            }
            arg = cpu->code[cpu->pc++];
        }

        cpu_err_t err = handlers[op](cpu, arg);
        if (err == CPU_STACK_FAIL)
        {
            my_perror(isa[op].name, cpu->stack_err);
            return err;
        }
        if (err != CPU_OK)
        {
            fprintf(stderr, "cpu: %s in %s\n", cpu_strerror(err), isa[op].name);
            return err;
        }
    }
    return CPU_OK;
}
//==========================================================================================================================