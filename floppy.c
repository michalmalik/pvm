#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"

static const u32 DEVICE_ID = 0x32ba236e;
static struct cpu *p;

u32 floppy_init(struct cpu *proc) {
	p = proc;

	printf("Initiating floppy, id 0x%08x\n", DEVICE_ID);

	return DEVICE_ID;
}

void floppy_handle_interrupt() {
	printf("Floppy received an interrupt: 0x%04x and reg B: 0x%04X\n", p->ia, p->r[rB]);
}