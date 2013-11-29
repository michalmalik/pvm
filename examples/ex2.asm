STO A,0
STO J,10
:loop
	ADD A,1
	IFN A,J
		JMP loop