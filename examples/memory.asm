.dst_addr	DAT	0
.src_addr	DAT	0

; IN
; 	+1 Destination address
; 	+2 Value
; 	+3 Count
; OUT
;	None
:write_mem
	STO Z,SP

	STO [dst_addr],[Z+1]
	STO Y,[Z+2]
	STO J,[Z+3]

	STO A,0
	:write_mem_loop
		STO X,[dst_addr]
		ADD X,A
		STO [X],Y
		ADD A,1
		IFL A,J
			JMP write_mem_loop 
	STO SP,Z
	RET

; IN
; 	+1 Destination address
; 	+2 Source address
; 	+3 Count
; OUT
;	None
:copy_mem
	STO Z,SP

	STO [dst_addr],[Z+1]
	STO [src_addr],[Z+2]
	STO J,[Z+3]

	STO A,0
	:copy_mem_loop
		STO X,[dst_addr]
		STO Y,[src_addr]
		ADD X,A
		ADD Y,A
		STO [X],[Y]
		ADD A,1
		IFL A,J
			JMP copy_mem_loop
	STO SP,Z
	RET
