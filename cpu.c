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
        case CPU_BAD_REGISTER:    return "BAD REGISTER NUMBER";
        case CPU_BAD_ADDRESS:     return "JMP TO BAD ADDRESS";
        default:                  return "UNKNOWN CPU ERROR";
    }
}
//===================================================================

//========================= cpu_ctor - создаёт cpu =============================
cpu_err_t cpu_ctor(CPU* cpu, code_t* code, size_t code_size)
{
    cpu->code       = code;
    cpu->code_size  = code_size;
    cpu->pc         = 0;

    for (int i = 0; i < REG_COUNT; i++)
    {
        cpu->regs[i] = ELEM_POISON;
    }

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

typedef struct
{
    int    flags;  // какие операнды есть
    int    reg;    // регистр (если BIT_REG)
    code_t imm;    // число   (если BIT_IMM)
} Operand;


//=== Тип указателя на функцию-обработчик операции CPU ===
typedef cpu_err_t (*op_func)(CPU* cpu, const Operand* opd);
//========================================================

// ============ op_push - кладёт элемент в стек процессора =============
static cpu_err_t op_push(CPU* cpu, const Operand* opd)
{
    Elem_t value = 0;
    if (opd->flags & BIT_REG)  value += cpu->regs[opd->reg];
    if (opd->flags & BIT_IMM)  value += opd->imm;

    cpu->stack_err = stack_push(&cpu->stack, value);

    return cpu->stack_err == STACK_OK ? CPU_OK : CPU_STACK_FAIL;
}
//======================================================================

// ============ op_add - складывает два последних элемента в стеке процессора, удаляет их из стека и кладёт результат сложения =============
static cpu_err_t op_add(CPU* cpu, const Operand* opd)
{
    (void)opd;
    Elem_t first_pop, second_pop;

    cpu_err_t err = pop_two_elements(cpu, &first_pop, &second_pop);
    if (err != CPU_OK) return err;

    cpu->stack_err = stack_push(&cpu->stack, first_pop + second_pop);
    return cpu->stack_err == STACK_OK ? CPU_OK : CPU_STACK_FAIL;
}
//===========================================================================================================================================

// ============ op_sub - из предпоследнего элемента стека вычитает последний, удаляет их из стека и кладёт результат вычитания =============
static cpu_err_t op_sub(CPU* cpu, const Operand* opd)
{
    (void)opd;
    Elem_t first_pop, second_pop;

    cpu_err_t err = pop_two_elements(cpu, &first_pop, &second_pop);
    if (err != CPU_OK) return err;

    cpu->stack_err = stack_push(&cpu->stack, second_pop - first_pop);
    return cpu->stack_err == STACK_OK ? CPU_OK : CPU_STACK_FAIL;
}
//==========================================================================================================================================

// ============ op_mul - последний элемент стека умножает на предпоследний, удаляет их из стека и кладёт результат умножения =============
static cpu_err_t op_mul(CPU* cpu, const Operand* opd)
{
    (void)opd;                                                  // шлём компилятор куда подальше
    Elem_t first_pop, second_pop;

    cpu_err_t err = pop_two_elements(cpu, &first_pop, &second_pop);
    if (err != CPU_OK) return err;

    cpu->stack_err = stack_push(&cpu->stack, first_pop * second_pop);
    return cpu->stack_err == STACK_OK ? CPU_OK : CPU_STACK_FAIL;
}
//========================================================================================================================================

// ============ op_div - предпоследний элемент стека делит на последний, удаляет их из стека и кладёт результат деления =============
static cpu_err_t op_div(CPU* cpu, const Operand* opd)
{
    (void)opd;
    Elem_t first_pop, second_pop;

    cpu_err_t err = pop_two_elements(cpu, &first_pop, &second_pop);
    if (err != CPU_OK) return err;
 
    if (fabs(first_pop) < 1e-17) return CPU_DIV_BY_ZERO;
 
    cpu->stack_err = stack_push(&cpu->stack, second_pop / first_pop);
    return cpu->stack_err == STACK_OK ? CPU_OK : CPU_STACK_FAIL;
}
//===================================================================================================================================

// ============ op_out - выводит последний элемент в стеке =============
static cpu_err_t op_out(CPU* cpu, const Operand* opd)
{
    (void)opd;
    Elem_t element;

    cpu->stack_err = stack_pop(&cpu->stack, &element);
    if (cpu->stack_err != STACK_OK) return CPU_STACK_FAIL;
 
    printf(ELEM_PRINT "\n", element);
    return CPU_OK;
}
//======================================================================

// =========== op_hlt - произодит полную остановку процессора ============
static cpu_err_t op_hlt(CPU* cpu, const Operand* opd)
{
    (void)opd;

    cpu->running = 0;

    return CPU_OK;
}
//========================================================================

static cpu_err_t op_pop(CPU* cpu, const Operand* opd)
{
    Elem_t value = 0;

    cpu->stack_err = stack_pop(&cpu->stack, &value);
    if (cpu->stack_err != STACK_OK) return CPU_STACK_FAIL;

    cpu->regs[opd->reg] = value;
    return CPU_OK;
}

static cpu_err_t op_jmp(CPU* cpu, const Operand* opd)
{
    if (opd->imm < 0 ||
        opd->imm >= (code_t)cpu->code_size) return CPU_BAD_ADDRESS;

    cpu->pc = opd->imm;
    return CPU_OK;
}

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
    [CMD_POP]  = op_pop,
    [CMD_JMP]  = op_jmp,
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

        code_t cell = cpu->code[cpu->pc++];
        if (!(isa_cell_ok(cell)) ||
            handlers[CELL_OP(cell)] == NULL)
        {
            fprintf(stderr, "cpu: %s %d (pc = %d)\n", cpu_strerror(CPU_UNKNOWN_OPCODE), cell, cpu->pc - 1);
            return CPU_UNKNOWN_OPCODE;
        }

        int    op    = CELL_OP(cell);
        int    flags = CELL_FLAGS(cell);
        Operand opd  = {.flags = flags, .imm = 0, .reg = 0};

        if (cpu->pc + operand_cells(opd.flags) > (int)cpu->code_size)
        {
            fprintf(stderr, "cpu: %s for %s (pc = %d)\n", cpu_strerror(CPU_MISSING_ARG), isa[op].name, cpu->pc);
            return CPU_MISSING_ARG;
        }

        if (flags & BIT_REG)
        {
            opd.reg = cpu->code[cpu->pc++];
            if (opd.reg < 0 || opd.reg >= REG_COUNT)
            {
                fprintf(stderr, "cpu: %s %d in %s\n", cpu_strerror(CPU_BAD_REGISTER), opd.reg, isa[op].name);
                return CPU_BAD_REGISTER;
            }
        }

        if (flags & BIT_IMM)
        {
            opd.imm = cpu->code[cpu->pc++];
        }

        cpu_err_t err = handlers[op](cpu, &opd);
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