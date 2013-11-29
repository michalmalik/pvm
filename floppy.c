#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEVICE_ID	0x32ba236e

typedef unsigned char u8;
typedef unsigned short u16;

unsigned int floppy_init() {
	printf("Initiating floppy, id 0x%08X\n", DEVICE_ID);

	return DEVICE_ID;
}

void floppy_handle_interrupt(u16 hwi) {
	printf("Floppy received an interrupt: 0x%04X\n", hwi);
}