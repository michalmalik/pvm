CC=clang
CFLAGS=-W -O3 -g

all: asm cpu

asm: asm.c	
	$(CC) asm.c -o asm $(CFLAGS)

cpu: cpu.c disasm.c
	$(CC) cpu.c disasm.c monitor.c floppy.c -o cpu $(CFLAGS)

clean:
	rm asm cpu