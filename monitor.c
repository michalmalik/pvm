#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEVICE_ID	0xff21beba

typedef unsigned char u8;
typedef unsigned short u16;

unsigned int monitor_init() {
	printf("Initiating monitor, id 0x%08X\n", DEVICE_ID);

	return DEVICE_ID;
}

void monitor_handle_interrupt(u16 hwi) {
	printf("Monitor received an interrupt: 0x%04X\n", hwi);
}