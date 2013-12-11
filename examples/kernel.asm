JMP start

#include "devices.asm"
#include "memory.asm"
#include "ivt.asm"

:init_kernel
	JTR build_ivt
	RET

:start
	JTR init_kernel

	STO D,0x8
	STO C,0xF00D
	STO X,0x1000
	STO B,SYSCALL_WRITE_MEM
	INT IVT_SYSCALL