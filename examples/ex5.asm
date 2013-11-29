JMP start

#include "memory.asm"

:start

PUSH 0x8 		; count
PUSH 0xF00D		; value
PUSH 0x1000 		; mem_address
JTR write_mem
ADD SP,3

PUSH 0x8 		; count
PUSH 0xBEAF 		; value
PUSH 0x2000 		; mem_address
JTR write_mem
ADD SP,3