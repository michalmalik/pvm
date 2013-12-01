; +1 Manufacter ID
; +2 Hardware ID
:find_device
	STO Z,SP

	STO C,0
	HWN J

	:find_device_loop
		HWQ C
		IFE A,[Z+1]
			IFE B,[Z+2]
				JMP find_device_ok
		ADD C,1
		IFE C,J
			JMP find_device_fail

		JMP find_device_loop

	:find_device_fail
		STO A,0	
		STO B,0
		JMP find_device_end

	:find_device_ok
		STO A,C

	:find_device_end
		STO SP,Z
		RET

:find_floppy
	PUSH 0x236e
	PUSH 0x32ba
	JTR find_device
	ADD SP,2
	RET

:find_monitor
	PUSH 0xbeba
	PUSH 0xff21
	JTR find_device
	ADD SP,2
	RET