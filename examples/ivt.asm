#define IVT_ADDRESS	0x500
#define IVT_SYSCALL	0x80
.ivt_size 	DAT	0

; IN
; 	+1 Interrupt handler
; 	+2 Offset
; OUT
;	None
:register_irq
	STO Z,SP

	STO Y,[Z+1]		; interrupt handler
	STO C,[Z+2]		; interrupt offset
	STO [IVT_ADDRESS+C],Y	; save the interrupt handler

	ADD [ivt_size],1

	STO SP,Z
	RET

; IN
;	None
; OUT
;	None
:build_ivt
	IAR handle_software_interrupt

	PUSH IVT_SYSCALL
	PUSH handle_ivt_syscall
	JTR register_irq
	ADD SP,2

	RET

; IN
;	A 	IA
; OUT
;	None
:handle_software_interrupt
	STO Y,[IVT_ADDRESS+A]
	JTR Y
	RETI

#define SYSCALL_WRITE_MEM 	0x1

; System call is fully defined by
; register B, which defines what
; type of system call will be called

; IN
; 	B 	syscall type
; OUT
; 	None
:handle_ivt_syscall
	IFE B,SYSCALL_WRITE_MEM
		JTR syscall_write_mem

	RET

; IN
; 	X 	destination address
; 	C 	value
; 	D 	count
; OUT
; 	None
:syscall_write_mem
	PUSH D
	PUSH C
	PUSH X
	JTR write_mem
	ADD SP,3
	RET