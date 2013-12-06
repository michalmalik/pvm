#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"

static const u32 DEVICE_ID = 0xff21beba;
static struct cpu *proc;

u32 monitor_init(struct cpu *p) {
	proc = p;

	printf("Initiating monitor, id 0x%08x\n", DEVICE_ID);

	return DEVICE_ID;
}

void monitor_handle_interrupt(u16 hwi) {
	printf("Monitor received an interrupt: 0x%04x\n", hwi);
}