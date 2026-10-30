CC      = gcc
CFLAGS  = -Wall -Wextra -Wshadow -g -fsanitize=address,undefined -MMD -MP
LDFLAGS = -fsanitize=address,undefined

LIBS_RAYLIB = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

COMMON  = bytecode.o       isa.o       stack.o
ASM_OBJ = asm_main.o                                    		              $(COMMON)
DIS_OBJ = disasm_main.o                                 		              $(COMMON)
CPU_OBJ     = cpu_main.o  cpu.o  	   display.o         		              $(COMMON)    # консольный экран
CPU_GUI_OBJ = cpu_main.o  cpu.o  	   display_raylib.o  		palette.o     $(COMMON)    # окно raylib
DEC_OBJ = decoder_main.o                               		                  $(COMMON)
PNG_OBJ = png2asm.o       palette.o                                           $(COMMON)
ANI_OBG = png2asm.o       palette.o    cpu.o                                  $(COMMON)

all: asm cpu disasm decoder png2asm frames_animation
all_gui: asm cpu_gui disasm decoder

asm:    $(ASM_OBJ)
	$(CC) $(LDFLAGS)   $^ -o $@

cpu:     $(CPU_OBJ)
	$(CC) $(LDFLAGS)   $^ -o $@ -lm

cpu_gui: $(CPU_GUI_OBJ)
	$(CC) $(LDFLAGS)   $^ -o $@ $(LIBS_RAYLIB)

disasm: $(DIS_OBJ)
	$(CC) $(LDFLAGS)   $^ -o $@

decoder: $(DEC_OBJ)
	$(CC) $(LDFLAGS)   $^ -o $@

png2asm: $(PNG_OBJ)
	$(CC) $(LDFLAGS) $^ -o $@ -lm

ani: $(ANI_OBJ)
	$(CC) $(LDFLAGS) $^ -o $@ -lm

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

-include $(wildcard *.d)

TESTS_COMMON = tests/call.txt tests/count.txt tests/new_quadratic.txt

test: all
	@for t in $(TESTS_COMMON); do \
	    echo "== $$t"; \
	    ./asm $$t byte_code.bin && ./cpu byte_code.bin && \
	    ./disasm byte_code.bin output.txt && ./asm output.txt byte_code2.bin && \
	    cmp byte_code.bin byte_code2.bin || exit 1; \
	done
	@echo "ROUNDTRIP OK"

TESTS_GUI = tests/draw_diagonal.txt tests/first_animation.txt tests/second_animation.txt

console_draw_test: all
	@for t in $(TESTS_GUI); do \
	    echo "== $$t"; \
	    ./asm $$t byte_code.bin && ./cpu byte_code.bin && \
	    ./disasm byte_code.bin output.txt && ./asm output.txt byte_code2.bin && \
	    cmp byte_code.bin byte_code2.bin || exit 1; \
	done
	@echo "CONSOLE OK"

raylib_draw_test: all_gui
	@for t in $(TESTS_GUI); do \
	    echo "== $$t"; \
	    ./asm $$t byte_code.bin && ./cpu_gui byte_code.bin && \
	    ./disasm byte_code.bin output.txt && ./asm output.txt byte_code2.bin && \
	    cmp byte_code.bin byte_code2.bin || exit 1; \
	done
	@echo "GUI DRAW OK"

clean:
	rm -f asm cpu disasm decoder cpu_gui *.o *.d *.bin output.txt

.PHONY: all all_gui test console_draw_test raylib_draw_test clean
