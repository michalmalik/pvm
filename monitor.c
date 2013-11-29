#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char u8;
typedef unsigned short u16;

void monitor_init() {
	printf("Initiating monitor");
}

void monitor_handle_interrupt(u16 hwi) {
	printf("Monitor received an interrupt: 0x%04X\n", hwi);
}