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
	./asm input.txt byte_code.bin
	./cpu byte_code.bin
	./disasm byte_code.bin output.txt
	./asm output.txt byte_code2.bin
	cmp byte_code.bin byte_code2.bin
	@echo "ROUNDTRIP OK"

clean:
	rm -f asm cpu disasm *.o *.d *.bin output.txt

.PHONY: all test clean