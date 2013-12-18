#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"

static const u32 DEVICE_ID = 0xbeed0011;
static struct cpu *p;

u32 keyboard_init(struct cpu *proc) {
	p = proc;

	printf("Initiating keyboard, id 0x%08x\n", DEVICE_ID);

	return DEVICE_ID;
}

void keyboard_handle_interrupt() {
	printf("Keyboard received an interrupt: 0x%04x and reg B: 0x%04X\n", p->ia, p->r[rB]);
}