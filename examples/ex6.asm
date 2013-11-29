JMP start

#include "memory.asm"

.mem_1 	DAT 	0x1000
.mem_2	DAT 	0x2000

:start

PUSH 0x10
PUSH 0xF00D
PUSH [mem_1]
JTR write_mem
ADD SP,3

PUSH 0x8
PUSH [mem_1]
PUSH [mem_2]
JTR copy_mem
ADD SP,3