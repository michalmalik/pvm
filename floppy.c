#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"

#define DEVICE_ID	0x32ba236e

static struct cpu *proc = NULL;

unsigned int floppy_init(struct cpu *p) {
	proc = p;

	printf("Initiating floppy, id 0x%08X\n", DEVICE_ID);

	return DEVICE_ID;
}

void floppy_handle_interrupt(u16 hwi) {
	printf("Floppy received an interrupt: 0x%04X\n", hwi);
}