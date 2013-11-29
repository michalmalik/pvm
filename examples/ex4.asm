JMP start

.mem_address 	DAT 	0

:write_mem
	STO Z,SP

	STO [mem_address],[Z+1]		; addr
	STO Y,[Z+2]			; value
	STO J,[Z+3]			; count

	STO A,0
	:write_mem_loop
		STO X,[mem_address]
		ADD X,A
		STO [X],Y
		ADD A,1
		IFN A,J
			JMP write_mem_loop
	
	STO SP,Z
	RET

:start

PUSH 0x8
PUSH 0xF00D
PUSH 0x1000
JTR write_mem
ADD SP,3