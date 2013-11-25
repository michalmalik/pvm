#define IVT_SYSCALL		0xAA
#define IVT_SYSCALL_READ	1
#define IVT_SYSCALL_WRITE	2
#define IVT_SYSCALL_EXIT	3

:handle_ivt_interrupt
	; check what type of IVT interrupt
	; to call
	; just syscall for the moment
	
	; B must be set to call syscall
	; routines
	IFE A,IVT_SYSCALL
		JTR handle_ivt_interrupt_syscall

	RETI

:handle_ivt_interrupt_syscall	
	; handle_read
	; set J to 0xAAAA
	IFE B,IVT_SYSCALL_READ
		STO J,0xAAAA

	; handle_write
	; set J to 0xBBBB
	IFE B,IVT_SYSCALL_WRITE
		STO J,0xBBBB

	; handle_exit
	; set J to 0xCCCC
	IFE B,IVT_SYSCALL_EXIT
		STO J,0xCCCC

	RET
