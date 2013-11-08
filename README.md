pvm
===

a 16-bit virtual machine, 0x8000 words of memory available (0x10000 bytes) with its own assembly language

+ PCPU16 Specifications
	* 0x8000 words of memory available (0x10000 bytes)
	* 7 registers (A, B, C, D, X, Y, Z) + stack (SP) and instruction pointer (IP or program counter)
	* 3 flags (E, S, O) in one FLAGS register
		+ E (equal), S (?), O (overflow)

+ Stack
	* 0x2000 words size
	* Start [0x7fff] (goes backwards)

+ Screen
	* 0x500 words size (80x32 bytes)
	* Start [0x0]
	* 8-bit color (rrrgggbb), transparency bits ommited

---------------------------------------------------------------------

* opcode - 6 bits 			(max. 0x3F)
* destination - 5 bits 			(max. 0x1F)
* source - 5 bits 			(max. 0x1F)
* Bit pattern:				dddddsssssoooooo

* Opcodes for d,s:

0x00 - 0x06		register						literal value of register A, B, C, D, X, Y, Z
0x07 - 0x0D		[register]						value at register
0x0E - 0x14		[register+A]						value at register + register
0x15 - 0x1B		[register+nextw]					value at memory address register + next word
0x1C 			nextw							literal value of next word
0x1D			[nextw]							value at memory address next word
0x1E			SP 							literal value of stack pointer register
0x1F			IP							literal value of instruction pointer register

* Instruction set (for o):

SET D,S 				sets source value to destination					0x01
CMP A,B 				compares A with B 							0x02
										E = 1 if A == B
										E = 0 if A != B
										S = 1 if A > B
										S = 0 if A < B
ADD A,B 				A = A+B 	 		 					0x03
SUB A,B 				A = A-B 		 						0x04
MUL A,B 				A = (A*B)&0xffff							0x05
DIV A,B 				A = A/B ; D=A%B								0x06
MOD A,B 				A = A%B 								0x07
NOT A					A = ~(A)                                                                0x08
AND A,B 				A = (A&B)&0xffff 							0x09
OR A,B 					A = (A|)B&0xffff 							0x0A
XOR A,B 				A = (A^B)&0xffff 							0x0B
SHL A,B 				A = (A<<B)&0xffff 							0x0C
SHR A,B 				A = (A>>B)&0xffff 							0x0D
JMP offset				jumps to offset unconditionally 					0x0E
JE offset				jumps if E = 1 								0x0F
JNE offset				jumps if E = 0 								0x10
JG offset				jumps if S = 1								0x11
JL offset				jumps if S = 0 								0x12
JGE offset				jumps if (E = 1 || S = 1)						0x13
JLE offset				jumps if (E = 1 || S = 0)						0x14
JTR offset				jump to routine ; pushes address					0x15
					of next instruction on stack
RET 					pops next address and jumps to it 					0x16
PUSH A 					pushes A on stack ; --SP 						0x17
POP A 					pops [SP] to A ; ++SP 							0x18
END					ends program execution							0x19
DAT val 				writes literal value to memory 	