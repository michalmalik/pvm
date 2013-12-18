### PCPU Specifications

* **NOTICE:** PCPU is supposed to be ticking on 1 Mhz frequency, but it's purely
arbitrary and **not working properly**. I am just using it to slow down the execution.
Basically every instruction takes from 1 to 3 execution cycles. Which is not how CPUs work.
Also, the devices are not synchronised with PCPU. PCPU is partially inspired by [PDP-11](http://en.wikipedia.org/wiki/PDP-11) and [Notch's](http://twitter.com/notch) [DCPU-16](http://dcpu.com/dcpu-16/)

* **16-bit** CPU, 16-bit words 
* **0x8000 words** of memory
* **8 16-bit general purpose registers** (A,B,C,D,X,Y,Z,J), **stack pointer** (SP), **instruction pointer** (IP), **interrupt address** register (IA), **overflow** (O) register
* stack start **0x8000**

### Instruction specifications
* instruction format 0xdddddsssssoooooo

| Operand   	     | Bits   | Max. value |
| ------------------ | :----: | :--------: |
| operation	     | 6      | 0x3F       |
| destination	     | 5      | 0x1F       |
| source	     | 5      | 0x1F       |

* 16-bit instruction might be followed by two 16-bit immediate constants

### Opcodes for destination,source operands
-----------------------------------------------------------------------------
| Opcode 	| Detail	   | Description	                    |
| ------------- | ---------------- | -------------------------------------- |
| 0x00 - 0x07	| register 	   | literal value of the register          |
| 0x08 - 0x0F	| [register]	   | value at register 			    |
| 0x10 - 0x17	| [register+nextw] | value at register + next word          |
| 0x18		| nextw		   | literal value of next word             | 
| 0x19		| [nextw]          | value at next word                     |
| 0x1A		| SP               | literal value of stack pointer         |
| 0x1B		| IP               | literal value of instruction pointer   |
| 0x1C		| O 		   | literal value of overflow register     |

### Opcodes for operation operand (instruction set)
--------------------------------------------------------------------------------------
| OP     | INS              | Description                              		     |
| :----: | ---------------- | ------------------------------------------------------ |
| 0x00   | STO A,B          | A = B                               		     |
| 0x01   | ADD A,B          | A = A + B            		     		     |
| 0x02   | SUB A,B          | A = A - B             		     		     |
| 0x03   | MUL A,B          | A = A * B              		     		     |
| 0x04   | DIV A,B          | A = A / B ; O = A % B                   		     |
| 0x05   | MOD A,B          | A = A % B                   	  		     |
| 0x06   | NOT A            | A = ~(A)                            		     |
| 0x07   | AND A,B          | A = A & B                             		     |
| 0x08   | OR A,B           | A = A OR B                          		     |
| 0x09   | XOR A,B          | A = A ^ B                             		     |
| 0x0A   | SHL A,B          | A = A << B                          		     |
| 0x0B   | SHR A,B          | A = A >> B                          		     |
| 0x0C   | MULS A,B         | signed A = A * B 					     |
| 0x0D   | DIVS A,B         | signed A = A / B; signed O = A % B		     |
| 0x0E   | MODS A,B         | signed A = A % B                                       |
| 0x0F   | IFE A,B          | execute next instruction if A == B    		     |
| 0x10   | IFN A,B          | if A != B            		  		     |
| 0x11   | IFG A,B          | if A > B            		  		     |
| 0x12   | IFL A,B          | if A < B           		   	  	     |
| 0x13   | IFA A,B          | signed if A > B                                        |
| 0x14   | IFB A,B          | signed if A < B                                        |
| 0x15   | JMP label        | jump to label            		  		     |
| 0x16   | JTR label        | push IP of next instruction on stack, jump to label    |
| 0x17   | PUSH A           | push A on stack, SP--	  		     	     |
| 0x18   | POP A            | pops value from stack to A, SP++         		     |
| 0x19   | RET              | pops value from stack to IP                 	     |
| 0x1A   | RETI             | pops value from stack to IP              		     |

* Branching is pretty much like [Notch's DCPU-16](http://dcpu.com/dcpu-16/). Previously I was using
x86-style branching (instructions that would set Z flag and branching would be done accordingly to that), but I find that obnoxious.

* O register is set to 1 when integer overflow is performed and to 0xffff when underflow is performed.

* PCPU can't address bytes, only words. 

#### Interrupt instructions
--------------------------------------------------------------------------------------
| OP     | INS              | Description                                            |
| :----: | ---------------- | ------------------------------------------------------ |
| 0x1B   | IAR A            | set IA to A                                            |
| 0x1C   | INT A            | jump to IA with the message A   			     |
| 0x1D   | HWI A            | sends IA to hardware A                                 |
| 0x1E   | HWQ A            | set rA to manufacter id of A and rB to hw id of A      |
| 0x1F   | HWN A            | set A to number of registered devices                  |

### Special instructions
--------------------------------------------------------------------------------------
| OP     | INS              | Description                                            |
| :----: | ---------------- | ------------------------------------------------------ |
| 0x20   | HLT              | halt the CPU execution                                 |

#### Devices

* Devices are identified and manipulated by their hardware index. 
The 32-bit version of the uid contains manufacter id (high 16 bits) and hardware
id (low 16 bits).

``0x32ba236e``

**0x32ba** being the *manufacter* id and **0x236e** the *hardware id*.

* Devices can fully manipulate the CPU and the memory.

### Assembly language

See examples directory.

#### Preprocessor

##### define

###### test.asm

```nasm
#define CONST_A		0xAAAA
#define CONST_B		0xBBBB

STO A, CONST_A
STO B, CONST_B

#define CONST_C		0xCCCC

STO C, CONST_C
```

##### include 

###### c.asm
```nasm
#define C_ASM	0xCCCC

:C_return
	STO A, A_ASM		; undefined		
	STO B, B_ASM		; undefined
	STO C, C_ASM
	RET
```

###### b.asm
```nasm
#include "c.asm"
#define B_ASM	0xBBBB

:B_return
	STO A, A_ASM		; undefined
	STO B, B_ASM
	STO C, C_ASM
	RET
```

###### a.asm
```nasm
JMP start

#include "b.asm"
#define A_ASM	0xAAAA

:start

; All defines are available

STO A, A_ASM
STO B, B_ASM
STO C, C_ASM

JTR B_return
JTR C_return
```