#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char u8;
typedef unsigned short u16;

void floppy_init() {
	printf("Initiating floppy");
}

void floppy_handle_interrupt(u16 hwi) {
	printf("Floppy received an interrupt: 0x%04X\n", hwi);
}