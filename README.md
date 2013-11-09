### PCPU Specifications
* **16-bit** CPU
* **0x8000 words**
* **8 16-bit general purpose registers** (A, B, C, D, X, Y, Z, J), **stack pointer** (SP), **instruction pointer** (IP)
* **OF** (overflow) flag
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

### Opcodes for operation operand (instruction set)

| OP     | INS              | description                              		     | cycles  |
| :----: | ---------------- | ------------------------------------------------------ | :-----: | 
| 0x00   | SET A,B          | A = B                               		     | 1-3     |
| 0x01   | ADD A,B          | A = A+B ; sets OF flag              		     | 2-3     |               
| 0x02   | SUB A,B          | A = A-B ; sets OF flag              		     | 2-3     |
| 0x03   | MUL A,B          | A = A*B ; sets OF flag              		     | 2-3     |
| 0x04   | DIV A,B          | A = A/B ; D = A%B                   		     | 2-3     |
| 0x05   | MOD A,B          | A = A%B                   	  		     |         |
| 0x06   | NOT A            | A = ~(A)                            		     |         |
| 0x07   | AND A,B          | A = A&B                             		     |         |
| 0x08   | OR A,B           | A = A OR B                          		     |         |
| 0x09   | XOR A,B          | A = A^B                             		     |         |
| 0x0A   | SHL A,B          | A = A << B                          		     |         |
| 0x0B   | SHR A,B          | A = A >> B                          		     |         |
| 0x0C   | IFE A,B          | execute next instruction if A==B    		     |         |
| 0x0D   | IFN A,B          | if A!=B            		  		     |         |
| 0x0E   | IFG A,B          | if A>B            		  		     |         |
| 0x0F   | IFL A,B          | if A<B           		   	  		     |         |
| 0x10   | IFGE A,B         | if A>=B                             		     |         |
| 0x11   | IFLE A,B         | if A<=B                             		     |         |
| 0x12   | JMP label        | jump to label            		  		     |         |
| 0x13   | JTR label        | push IP of next instruction on stack, jump to label    |	       |
| 0x14   | PUSH A           | push A on stack, SP--	  		     	     |         |
| 0x15   | POP A            | pops value from stack to A, SP++         		     |         |
| 0x16   | RET              | pops value from stack to IP                 	     |         |

### Assembly language

#### Example no. 1
```
SET A,0x1000
SET B,[A]
```

#### Example no. 2
```
SET A,0
SET C,10
:loop
	ADD A,1
	IFN A,C
		JMP loop
```

#### Example no. 3
```
JMP start

.mem_address 	DAT 	0

:write_mem
	POP Z

	POP [mem_address] 	; mem_address
	POP C 			; count
	POP Y 			; value

	SET A,0
	:loop
		SET X,[mem_address]
		ADD X,A
		SET [X],Y
		ADD A,1
		IFN A,C
			JMP loop
	PUSH Z
	RET

:start

PUSH 0xF00D
PUSH 0x8
PUSH 0x1000
JTR write_mem
```

#### Example no. 4

##### test.asm
```
JMP start

#include "memory.asm"

:start

PUSH 0xF00D		; value
PUSH 0x8 		; count
PUSH 0x1000 		; mem_address
JTR write_mem

PUSH 0xBEAF 		; value
PUSH 0x8 		; count
PUSH 0x2000 		; mem_address
JTR write_mem
```

##### memory.asm
```
.mem_address 	DAT 	0

:write_mem
	POP Z

	POP [mem_address] 	; mem_address
	POP C 			; count
	POP Y 			; value

	SET A,0
	:loop
		SET X,[mem_address]
		ADD X,A
		SET [X],Y
		ADD A,1
		IFN A,C
			JMP loop

	PUSH Z
	RET
```

#### Example no. 5

##### test.asm
```
JMP start

#include "memory.asm"

.mem_1 	DAT 	0x1000
.mem_2	DAT 	0x2000

:start

PUSH 0xF00D
PUSH 0x8
PUSH [mem_1]
JTR write_mem

PUSH 0x8
PUSH [mem_1]
PUSH [mem_2]
JTR copy_mem
```

##### memory.asm
```
.dst_addr	DAT 	0
.src_addr 	DAT 	0

:write_mem
	POP Z

	POP [dst_addr]		; mem_address
	POP C 			; count
	POP Y 			; value

	SET A,0
	:loop
		SET X,[dst_addr]
		ADD X,A
		SET [X],Y
		ADD A,1
		IFN A,C
			JMP loop

	PUSH Z
	RET

:copy_mem
	POP Z

	POP [dst_addr]
	POP [src_addr] 
	POP C 

	:loop_2
		SET X,[dst_addr]
		SET Y,[src_addr] 
		ADD X,A
		ADD Y,A
		SET [X],[Y]
		ADD A,1
		IFN A,C
			JMP loop_2
	PUSH Z
	RET
```

#### Example no. 6
```
JMP start

; struct Person
#define PERSON_AGE		0
#define PERSON_HEIGHT		1

#define ST_PERSON_MICHAL	0x1000

:start

SET A,ST_PERSON_MICHAL
SET [A+PERSON_AGE],0x12
SET [A+PERSON_HEIGHT],0xB4
```

##### hexdump -C
```
00000000  c0 12 00 02 06 00 10 00  86 00 00 00 00 12 86 00  |................|
00000010  00 01 00 b4 00 00 00 00  00 00 00 00 00 00 00 00  |................|
00000020  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
*
00002000  00 12 00 b4 00 00 00 00  00 00 00 00 00 00 00 00  |................|
00002010  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
*
00010000
```

#### Macros

##### define

###### data.asm
```
JMP start

#define MEM_ADDR 	0x1000

:start

SET [MEM_ADDR],0xF00D
PUSH [MEM_ADDR]
```

##### include

###### data.asm
```
.const_A        DAT     0x1000
```

###### test.asm
```
JMP start

#include "data.asm"

:start

SET A,const_A
SET B,[A]
SET [B],0xF00D
```
