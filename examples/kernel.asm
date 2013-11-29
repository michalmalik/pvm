JMP start

#include "ivt.asm"
#include "memory.asm"

:start
	IAR handle_ivt_interrupt
	; SYSCALL routine is determined
	; by the register B and the return value
	; by register J
	; 1 - read, 	0xAAAA
	; 2 - write,	0xBBBB
	; 3 - exit;	0xCCCC

	; SYSCALL is called from IVT
	; by software interrupt 0xAA

	; J should be 0xAAAA
	STO B,0x1	; read
	INT 0xAA

	; J should be 0xBBBB
	STO B,0x2	; write
	INT 0xAA

	; J should be 0xCCCC
	STO B,0x3	; exit
	INT 0xAA
