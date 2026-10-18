CC      = gcc
CFLAGS  = -Wall -Wextra -Wshadow -g -fsanitize=address,undefined -MMD -MP
LDFLAGS = -fsanitize=address,undefined

COMMON  = bytecode.o     isa.o       stack.o
ASM_OBJ = asm_main.o                 $(COMMON)
DIS_OBJ = disasm_main.o              $(COMMON)
CPU_OBJ = cpu_main.o     cpu.o       $(COMMON)

all: asm cpu disasm

asm:    $(ASM_OBJ)
	   $(CC) $(CFLAGS)    $^ -o $@

cpu:    $(CPU_OBJ)
	   $(CC) $(LDFLAGS)   $^ -o $@ -lm

disasm: $(DIS_OBJ)
	   $(CC) $(CFLAGS)    $^ -o $@

%.o: %.c
	   $(CC) $(CFLAGS) -c $< -o $@

-include $(wildcard *.d)

test: all
	./asm input.txt byte_code.txt
	./cpu byte_code.txt
	./disasm byte_code.txt output.txt
	./asm output.txt byte_code2.txt
	diff byte_code.txt byte_code2.txt
	@echo "ROUNDTRIP OK"

clean:
	rm -f asm cpu disasm *.o *.d

.PHONY: all test clean