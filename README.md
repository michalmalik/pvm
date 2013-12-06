### PCPU Specifications
* **16-bit** CPU, 16-bit words
* experimental **1 Mhz** clock 
* **0x8000 words**
* **8 16-bit general purpose registers** (A,B,C,D,X,Y,Z,J), **stack pointer** (SP), **instruction pointer** (IP), **interrupt address** register (IA), **overflow** (OV) register
* **0x2000 words** global stack size, start `0x7FFF`, goes down
* instruction takes maximum **6 bytes** 

### Instruction specifications
* instruction is 16 bits `0xDDDDDSSSSSOOOOOO`, might be followed by two 16-bit words

| operand   	     | bits   | max value  |
| ------------------ | :----: | :--------: |
| operation	     | 6      | 0x3F       |
| destination	     | 5      | 0x1F       |
| source	     | 5      | 0x1F       |


### Opcodes for destination,source operands

| opcode 	| detail	   | description	                    |
| ------------- | ---------------- | -------------------------------------- |
| 0x00 - 0x07	| register 	   | literal value of the register          |
| 0x08 - 0x0F	| [register]	   | value at register 			    |
| 0x10 - 0x17	| [register+nextw] | value at register + next word          |
| 0x18		| nextw		   | literal value of next word             |
| 0x19		| [nextw]          | value at next word                     |
| 0x1A		| SP               | literal value of stack pointer         |
| 0x1B		| IP               | literal value of instruction pointer   |
| 0x1C		| OV 		   | literal value of overflow register     |

### Opcodes for operation operand (instruction set)

| OP     | INS              | description                              		     | cycles  |
| :----: | ---------------- | ------------------------------------------------------ | :-----: | 
| 0x00   | STO A,B          | A = B                               		     | x       |
| 0x01   | ADD A,B          | A = A+B ; sets OF flag              		     | x       | 
| 0x02   | SUB A,B          | A = A-B ; sets OF flag              		     | x       |
| 0x03   | MUL A,B          | A = A*B ; sets OF flag              		     | x       |
| 0x04   | DIV A,B          | A = A/B ; OV = A%B                   		     | x       |
| 0x05   | MOD A,B          | A = A%B                   	  		     | x       |
| 0x06   | NOT A            | A = ~(A)                            		     | x       |
| 0x07   | AND A,B          | A = A&B                             		     | x       |
| 0x08   | OR A,B           | A = A OR B                          		     | x       |
| 0x09   | XOR A,B          | A = A^B                             		     | x       |
| 0x0A   | SHL A,B          | A = A << B                          		     | x       |
| 0x0B   | SHR A,B          | A = A >> B                          		     | x       |
| 0x0C   | MLS A,B          | signed A = A*B 					     | x       |
| 0x0D   | DVS A,B          | signed A = A/B; signed OV = A%B 			     | x       |
| 0x0E   | MDS A,B          | signed A = A%B                                         | x       |
| 0x0F   | IFE A,B          | execute next instruction if A == B    		     | x       |
| 0x10   | IFN A,B          | if A != B            		  		     | x       |
| 0x11   | IFG A,B          | if A > B            		  		     | x       |
| 0x12   | IFL A,B          | if A < B           		   	  	     | x       |
| 0x13   | IFA A,B          | signed if A > B                                        | x       |
| 0x14   | IFB A,B          | signed if A < B                                        | x       |
| 0x15   | JMP label        | jump to label            		  		     | x       |
| 0x16   | JTR label        | push IP of next instruction on stack, jump to label    | x       |
| 0x17   | PUSH A           | push A on stack, SP--	  		     	     | x       |
| 0x18   | POP A            | pops value from stack to A, SP++         		     | x       |
| 0x19   | RET              | pops value from stack to IP                 	     | x       |
| 0x1A   | RETI             | pops value from stack to IP              		     | x       |

#### Interrupt instructions
------------------------------------------------------------------------------------------------
| OP     | INS              | description                                            | cycles  |
| :----: | ---------------- | ------------------------------------------------------ | :-----: |
| 0x1B   | IAR A            | set IA to A                                            | x       |
| 0x1C   | INT A            | jump to IA, with the message A; software interrupt     | x       | 
| 0x1D   | HWI A            | sends IA to hardware A                                 | x       |
| 0x1E   | HWQ A            | set rA to manufacter id of A and rB to hw id of A      | x       |
| 0x1F   | HWN A            | set A to number of registered devices                  | x       |

#### Devices

Devices are identified and manipulated by their hardware index. 
The 32-bit version of the ID contains manufacter id (high 16 bits) and hardware
id (low 16 bits).

``0x32ba236e``

**0x32ba** being the *manufacter* id and **0x236e** the *hardware id*.

##### Known devices for PCPU:

* Floppy disk, 1.44MB, **0x32ba236e**
* Monitor, **0xff21beba**

### Assembly language

See examples directory.

#### Preprocessor

##### define

Currently, there's no limit for defines for a single file. 

##### test.asm

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