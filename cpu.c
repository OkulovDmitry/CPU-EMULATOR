/*=====================================================================================================================================================================================*/
/*=====================================================================================================================================================================================*/
/*=====================================================================================================================================================================================*/
//                                                                           Central Processing Unit
/*=====================================================================================================================================================================================*/
/*=====================================================================================================================================================================================*/
/*=====================================================================================================================================================================================*/

#include "cpu.h"

#define COLOR_RED     "\033[1;31m"
#define COLOR_RESET   "\033[0m"
#define CPU_STR(s) ((s) ? (s) : "?")

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
        case CPU_CALL_OVERFLOW:   return "CALL STACK IS OVERFLOW";
        case CPU_RET_EMPTY:       return "RET WITHOUT CALL";
        case CPU_BAD_MEMORY:       return "MEMORY ADDRESS OUT OF RANGE";
        case CPU_NO_MEMORY:       return "OUT OF MEMORY IN CPU";
        default:                  return "UNKNOWN CPU ERROR";
    }
}
//===================================================================

typedef struct
{
    int    flags;  // какие операнды есть
    int    reg;    // регистр (если BIT_REG)
    code_t imm;    // сырая ячейка
    Elem_t num;    // число как double (для PUSH)
} Operand;

//========================= cpu_ctor - создаёт cpu =============================
cpu_err_t cpu_ctor(CPU* cpu, int code_size CPU_PLACE_IN)
{
#ifdef CPU_PLACE_IN
    cpu->name = name;
    cpu->file = file;
    cpu->line = line;
    cpu->function = function;
#endif

    cpu->code_size  = code_size;
    cpu->pc         = 0;

    for (int i = 0; i < REG_COUNT; i++)
    {
        cpu->regs[i] = ELEM_POISON;
    }

    cpu->running        = 1;

    cpu->ram = (RAM*)calloc(1, sizeof(RAM));    if (cpu->ram == NULL) return CPU_NO_MEMORY;
    for (int i = 0; i < VRAM_SIZE; i++)
    {
        cpu->ram->vram[i] = 0;
    }
    (cpu->ram)->call_sp = 0;

    char* temp = malloc(sizeof(my_stack_t));
    if (temp == NULL)
    {
        free(cpu->ram);
        return CPU_NO_MEMORY;
    }
    
    cpu->stack = (my_stack_t*)temp;
    
    (cpu->ram)->stack_err = stack_ctor(cpu->stack,
                                16
                                STACK_PLACE_OUT("cpu_stack"));

    return (cpu->ram)->stack_err == STACK_OK ? CPU_OK : CPU_STACK_FAIL;
}
//===============================================================================

//=========================== cpu_dtor - разрушает cpu ===============================
cpu_err_t cpu_dtor(CPU* cpu)
{
    (cpu->ram)->stack_err = stack_dtor(cpu->stack);
    free(cpu->stack);

    err_t last_err = (cpu->ram)->stack_err;

    free(cpu->ram);

    return last_err == STACK_OK ? CPU_OK : CPU_STACK_FAIL;
}
//====================================================================================

static cpu_err_t pop_two_elements(CPU* cpu, Elem_t* first_pop, Elem_t* second_pop)
{
    (cpu->ram)->stack_err = stack_pop(cpu->stack, first_pop);
    if ((cpu->ram)->stack_err != STACK_OK) return CPU_STACK_FAIL;
    
    (cpu->ram)->stack_err = stack_pop(cpu->stack, second_pop);
    if ((cpu->ram)->stack_err != STACK_OK) return CPU_STACK_FAIL;

    return CPU_OK;
}

static cpu_err_t mem_address(const CPU* cpu, const Operand* opd, size_t* addr)
{
    if (opd->flags & BIT_REG)
    {
        Elem_t v = cpu->regs[opd->reg];
        if (!(v >= 0 && v < VRAM_SIZE)) return CPU_BAD_MEMORY;
        *addr = (size_t)v;
    }
    else
    {
        if (opd->imm >= VRAM_SIZE) return CPU_BAD_MEMORY;
        *addr = (size_t)opd->imm;
    }
    return CPU_OK;
}

static cpu_err_t do_jump(CPU* cpu, const Operand* opd)
{
    if (opd->imm >= (code_t)cpu->code_size) return CPU_BAD_ADDRESS;

    cpu->pc = opd->imm;
    return CPU_OK;
}

static cpu_err_t cond_jump(CPU* cpu, const Operand* opd, bool (*cmp)(Elem_t, Elem_t))
{
    Elem_t top, below;

    cpu_err_t err = pop_two_elements(cpu, &top, &below);
    if (err != CPU_OK) return err;

    if (cmp(below, top)) return do_jump(cpu, opd);
    return CPU_OK;
}

static bool cmp_a  (Elem_t a, Elem_t b) { return a >  b; }
static bool cmp_ae (Elem_t a, Elem_t b) { return a >= b; }
static bool cmp_b  (Elem_t a, Elem_t b) { return a <  b; }
static bool cmp_be (Elem_t a, Elem_t b) { return a <= b; }
static bool cmp_e  (Elem_t a, Elem_t b) { return a == b; }
static bool cmp_ne (Elem_t a, Elem_t b) { return a != b; }


//=== Тип указателя на функцию-обработчик операции CPU ===
typedef cpu_err_t (*op_func)(CPU* cpu, const Operand* opd);
//========================================================

// ============ op_push - кладёт элемент в стек процессора =============
static cpu_err_t op_push(CPU* cpu, const Operand* opd)
{
    Elem_t value = 0;
    if (opd->flags & BIT_MEM)
    {
        size_t addr = 0;
        cpu_err_t err = mem_address(cpu, opd, &addr);
        if (err != CPU_OK) return err;

        value = (cpu->ram)->vram[addr];
    }
    else if (opd->flags & BIT_REG) value = cpu->regs[opd->reg];
    else                           value = opd->num; 

    (cpu->ram)->stack_err = stack_push(cpu->stack, value);

    return (cpu->ram)->stack_err == STACK_OK ? CPU_OK : CPU_STACK_FAIL;
}
//======================================================================

// ============ op_add - складывает два последних элемента в стеке процессора, удаляет их из стека и кладёт результат сложения =============
static cpu_err_t op_add(CPU* cpu, const Operand* opd)
{
    (void)opd;
    Elem_t first_pop, second_pop;

    cpu_err_t err = pop_two_elements(cpu, &first_pop, &second_pop);
    if (err != CPU_OK) return err;

    (cpu->ram)->stack_err = stack_push(cpu->stack, first_pop + second_pop);
    return (cpu->ram)->stack_err == STACK_OK ? CPU_OK : CPU_STACK_FAIL;
}
//===========================================================================================================================================

// ============ op_sub - из предпоследнего элемента стека вычитает последний, удаляет их из стека и кладёт результат вычитания =============
static cpu_err_t op_sub(CPU* cpu, const Operand* opd)
{
    (void)opd;
    Elem_t first_pop, second_pop;

    cpu_err_t err = pop_two_elements(cpu, &first_pop, &second_pop);
    if (err != CPU_OK) return err;

    (cpu->ram)->stack_err = stack_push(cpu->stack, second_pop - first_pop);
    return (cpu->ram)->stack_err == STACK_OK ? CPU_OK : CPU_STACK_FAIL;
}
//==========================================================================================================================================

// ============ op_mul - последний элемент стека умножает на предпоследний, удаляет их из стека и кладёт результат умножения =============
static cpu_err_t op_mul(CPU* cpu, const Operand* opd)
{
    (void)opd;                                                  // шлём компилятор куда подальше
    Elem_t first_pop, second_pop;

    cpu_err_t err = pop_two_elements(cpu, &first_pop, &second_pop);
    if (err != CPU_OK) return err;

    (cpu->ram)->stack_err = stack_push(cpu->stack, first_pop * second_pop);
    return (cpu->ram)->stack_err == STACK_OK ? CPU_OK : CPU_STACK_FAIL;
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
 
    (cpu->ram)->stack_err = stack_push(cpu->stack, second_pop / first_pop);
    return (cpu->ram)->stack_err == STACK_OK ? CPU_OK : CPU_STACK_FAIL;
}
//===================================================================================================================================

// ============ op_out - выводит последний элемент в стеке =============
static cpu_err_t op_out(CPU* cpu, const Operand* opd)
{
    (void)opd;
    Elem_t element;

    (cpu->ram)->stack_err = stack_pop(cpu->stack, &element);
    if ((cpu->ram)->stack_err != STACK_OK) return CPU_STACK_FAIL;
 
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
    size_t addr = 0;
    if (opd->flags & BIT_MEM)
    {
        cpu_err_t err = mem_address(cpu, opd, &addr);
        if (err != CPU_OK) return err;
    }

    Elem_t value = 0;
    (cpu->ram)->stack_err = stack_pop(cpu->stack, &value);
    if ((cpu->ram)->stack_err != STACK_OK) return CPU_STACK_FAIL;

    if (opd->flags & BIT_MEM) (cpu->ram)->vram[addr] = value;   // POP [8] / POP [AX]
    else                      cpu->regs[opd->reg]    = value;   // POP AX
    return CPU_OK;
}

static cpu_err_t op_jmp (CPU* cpu, const Operand* opd){ return do_jump   (cpu, opd); }
static cpu_err_t op_ja  (CPU* cpu, const Operand* opd){ return cond_jump (cpu, opd, cmp_a); }
static cpu_err_t op_jae (CPU* cpu, const Operand* opd){ return cond_jump (cpu, opd, cmp_ae); }
static cpu_err_t op_jb  (CPU* cpu, const Operand* opd){ return cond_jump (cpu, opd, cmp_b); }
static cpu_err_t op_jbe (CPU* cpu, const Operand* opd){ return cond_jump (cpu, opd, cmp_be); }
static cpu_err_t op_je  (CPU* cpu, const Operand* opd){ return cond_jump (cpu, opd, cmp_e); }
static cpu_err_t op_jne (CPU* cpu, const Operand* opd){ return cond_jump (cpu, opd, cmp_ne); }

static cpu_err_t op_call(CPU* cpu, const Operand* opd)
{
    if ((cpu->ram)->call_sp >= CALL_STACK_SIZE) return CPU_CALL_OVERFLOW;

    int return_addr = cpu->pc;   // адрес команды после call

    cpu_err_t err = do_jump(cpu, opd);
    if (err != CPU_OK) return err;

    (cpu->ram)->call_stack[(cpu->ram)->call_sp++] = return_addr;

    return CPU_OK;
}

static cpu_err_t op_ret(CPU* cpu, const Operand* opd)
{
    (void)opd;
    if ((cpu->ram)->call_sp == 0) return CPU_RET_EMPTY;

    cpu->pc = (cpu->ram)->call_stack[--(cpu->ram)->call_sp];
    return CPU_OK;
}

static cpu_err_t op_draw(CPU* cpu, const Operand* opd)
{
    (void)opd;
    display_show(cpu->ram->vram, VRAM_W, VRAM_H);  // отдаём видеопамять экрану
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
    [CMD_JA]   = op_ja,
    [CMD_JAE]  = op_jae,
    [CMD_JB]   = op_jb,
    [CMD_JBE]  = op_jbe,
    [CMD_JE]   = op_je,
    [CMD_JNE]  = op_jne,
    [CMD_CALL] = op_call,
    [CMD_RET]  = op_ret,
    [CMD_DRAW] = op_draw
};
//============================================================================

// ========================================== cpu_run - запускает процессор ==============================================
cpu_err_t cpu_run(CPU* cpu, code_t* code, int size)
{
    cpu->ram->code = code;
    while(cpu->running)
    {
        if (cpu->pc < 0 || cpu->pc >= size)
        {
            fprintf(stderr, "cpu: %s (pc = %d)\n", cpu_strerror(CPU_PC_OUT_OF_RANGE), cpu->pc);
            return CPU_PC_OUT_OF_RANGE;
        }

        code_t cell = code[cpu->pc++];
        if (!(isa_cell_ok(cell)) ||
            handlers[CELL_OP(cell)] == NULL)
        {
            fprintf(stderr, "cpu: %s %lld (pc = %d)\n", cpu_strerror(CPU_UNKNOWN_OPCODE), (long long)cell, cpu->pc - 1);
            return CPU_UNKNOWN_OPCODE;
        }

        int    op    = CELL_OP(cell);
        int    flags = CELL_FLAGS(cell);
        Operand opd  = {.flags = flags, .imm = 0, .num = 0, .reg = 0};

        if (cpu->pc + operand_cells(opd.flags) > size)
        {
            fprintf(stderr, "cpu: %s for %s (pc = %d)\n", cpu_strerror(CPU_MISSING_ARG), isa[op].name, cpu->pc);
            return CPU_MISSING_ARG;
        }

        if (flags & BIT_REG)
        {
            code_t r = code[cpu->pc++];
            if (r >= REG_COUNT)
            {
                fprintf(stderr, "cpu: %s %llu in %s\n",
                        cpu_strerror(CPU_BAD_REGISTER), (unsigned long long)r, isa[op].name);
                return CPU_BAD_REGISTER;
            }
            opd.reg = (int)r;
        }

        if (flags & BIT_IMM)
        {
            opd.imm = code[cpu->pc++];
            opd.num = (flags & BIT_FLT) ? cell_to_double(opd.imm) : (Elem_t)(int64_t)opd.imm;
        }

        cpu_err_t err = handlers[op](cpu, &opd);
        if (err == CPU_STACK_FAIL)
        {
            my_perror(isa[op].name, (cpu->ram)->stack_err);
            return err;
        }
        if (err != CPU_OK)
        {
            fprintf(stderr, "cpu: %s in %s\n", cpu_strerror(err), isa[op].name);
            return err;
        }

#ifdef CPUDEBUG
        cpu_dump("errors.log", cpu, cpu->name, __LINE__, __func__, __FILE__);
#endif
    }
    return CPU_OK;
}
//==========================================================================================================================

void cpu_dump_to(FILE* file, const CPU* cpu, const char* name, int line, const char* function, const char* file_name)
{
    fprintf(file, CPU_SEP);

    if (cpu == NULL)
    {
        fprintf(file, "CPU %s dumped from %s() at %s: %i\n", CPU_STR(name),      CPU_STR(function),      CPU_STR(file_name), line);
        fprintf(file, "CPU = NULL\n");
        fprintf(file, CPU_SEP);
        return;
    }
#ifdef CPUDEBUG
    if (name == NULL) name = cpu->name;
    fprintf(file, "CPU %s created by %s() at %s: %i\n",  CPU_STR(cpu->name), CPU_STR(cpu->function), CPU_STR(cpu->file), cpu->line);
#endif
    fprintf(file, "CPU %s dumped from %s() at %s: %i\n", CPU_STR(name),      CPU_STR(function),      CPU_STR(file_name), line);

    fprintf(file, "\n\n");

    dump_to(file, cpu->stack, name, line , function, file_name);

    fprintf(file, "\n\n");

    fprintf(file, "size = %i\n", cpu->code_size);
    if (cpu->running)
    {
        fprintf(file, "number of now running command: %i\n", cpu->pc);
    }
    else
    {
        fprintf(file, "CPU STOP\n");
    }

    for (int i = 0; i < cpu->code_size; i++)
    {
        if (i == cpu->pc) fprintf(file, "-->[%llu] ", (unsigned long long)cpu->ram->code[i]);
        else              fprintf(file, "%llu ", (unsigned long long)cpu->ram->code[i]);
    }

    fprintf(file, "\n===== CPU regisers dump =====\n");

    fprintf(file, "number of registers = %i\n", REG_COUNT);

    for (int i = 0; i < REG_COUNT; i++)
    {
        if (cpu->regs[i] == ELEM_POISON)
        {
            fprintf(file, "[%s]: 0x%08" PRIX32" (ELEM_POISON)\n", reg_names[i], (uint32_t)cpu->regs[i]);
        }
        else
        {
            fprintf(file, "[%s]: 0x%016" PRIX64 " (" ELEM_PRINT ")\n", reg_names[i], (uint64_t)cpu->regs[i], cpu->regs[i]);
        }
    }

    fprintf(file, "=============================\n");

    fprintf(file, "\n======================================== VRAM of CPU ========================================\n");

    for (int y = 0; y < VRAM_H; y++)
    {
        for (int x = 0; x < VRAM_W; x++)
        {
            Elem_t v = cpu->ram->vram[y * VRAM_W + x];

            if (v == ELEM_POISON) fprintf(file, "%7s ", "POISON");
            else                  fprintf(file, "%7.3g ", v);
        }
        fputc('\n', file);
    }

    fprintf(file, "=============================================================================================\n");

    fprintf(file, CPU_SEP);

    fprintf(file, "\n\n\n");
}

void cpu_dump(const char* out, const CPU* cpu, const char* name, int line, const char* function, const char* file_name)
{
    FILE* file = fopen(out, "a");
    if (file == NULL) {
        perror("Error of open file");
        return;
    }

    cpu_dump_to(file, cpu, name, line, function, file_name);
    fclose(file);
}