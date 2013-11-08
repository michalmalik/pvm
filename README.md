### PCPU Specifications
* **16-bit** CPU
* **0x8000 words** of memory available (0x10000 bytes)
* **7 registers** (A, B, C, D, X, Y, Z), **stack pointer** (SP), **instruction pointer** (IP or program counter)
* **OF** (overflow) flag
* **0x2000 words** global stack size, start `0x7FFF`, goes down
* **0x500 words** screen size (80x32 bytes), start `0x0`
* instruction takes maximum **6 bytes** 

### Instruction specifications
* instruction opcode is 16 bits `0xDDDDDSSSSSOOOOOO`

| instruction part   | bits   | max value  |
| ------------------ | :----: | :--------: |
| operation	     | 6      | 0x3F       |
| destination	     | 5      | 0x1F       |
| source	     | 5      | 0x1F       |


### Opcodes for destination,source

| opcode 	| detail	   | description	                    |
| ------------- | ---------------- | -------------------------------------- |
| 0x00 - 0x06	| register 	   | literal value of the register          |
| 0x07 - 0x0D	| [register]	   | value at register 			    |
| 0x0E - 0x14	| [register+A]     | value at register + A                  |
| 0x15 - 0x1B	| [register+nextw] | value at register + next word          |
| 0x1C		| nextw		   | literal value of next word             |
| 0x1D		| [nextw]          | value at next word                     |
| 0x1E		| SP               | literal value of stack pointer         |
| 0x1F		| IP               | literal value of instruction pointer   |

### Instruction set (for operation)

| OP     | INS              | detail                              | description                         |
| :----: | ---------------- | ----------------------------------- | ----------------------------------- |
| 0x00   | SET A,B          | A = B                               |                                     |
| 0x01   | ADD A,B          | A = A+B ; sets OF flag              |                                     |               
| 0x02   | SUB A,B          | A = A-B ; sets OF flag              |                                     |
| 0x03   | MUL A,B          | A = A*B ; sets OF flag              |                                     |
| 0x04   | DIV A,B          | A = A/B ; D = A%B                   |                                     |
| 0x05   | MOD A,B          | A = A%B                   	  |                                     |
| 0x06   | NOT A            | A = ~(A)                            |                                     |
| 0x08   | AND A,B          | A = A&B                             |                                     |
| 0x09   | OR A,B           | A = A OR B                          |                                     |
| 0x0A   | XOR A,B          | A = A^B                             |                                     |
| 0x0B   | SHL A,B          | A = A << B                          |                                     |
| 0x0C   | SHR A,B          | A = A >> B                          |                                     |
| 0x0D   | IFE A,B          | execute next instruction if A==B    |                                     |
| 0x0E   | IFN A,B          | if A!=B            		  |                                     |
| 0x0F   | IFG A,B          | if A>B            		  |                                     |
| 0x10   | IFL A,B          | if A<B           		   	  |                                     |
| 0x11   | JMP label        | jump to label            		  |                                     |
| 0x12   | JTR label        | jump to label, PUSH IP+1 		  |                                     |
| 0x13   | PUSH A           | pushes A on stack; --SP 		  |                                     |
| 0x14   | POP A            | pops value from stack to A          |                                     |
| 0x15   | RET              | POP IP                       	  |                                     |
| 0x16   | END              | ends program execution              |                                     |
|   -    | DAT w            | writes literal value to memory      |                                     |

### Assembly language

#### Example no. 1
```
SET A,0x1000
SET B,[A]
END
```

#### Example no. 2
```
SET A,0
SET C,10
:loop
	ADD A,1
	IFN A,C
		JMP loop
END
```

#### Example no. 3
```
PUSH 0xF00D
PUSH 0x8
PUSH 0x1000
JTR write_mem

END

:write_mem
	POP Z

	POP X 		; mem_address
	POP C 		; count
	POP Y 		; value

	SET A,0
	:loop
		SET [X+A],Y
		ADD A,1
		IFN A,C
			JMP loop

	PUSH Z
	RET
```

#### Example no. 4

##### test.asm
```
PUSH 0xF00D		; value
PUSH 0x8 		; count
PUSH 0x1000 		; mem_address
JTR write_mem

PUSH 0xBEAF 		; value
PUSH 0x8 		; count
PUSH 0x2000 		; mem_address
JTR write_mem

END

#include "memory.asm"
```

##### memory.asm
```
:write_mem
	POP Z

	POP X 		; mem_address
	POP C 		; count
	POP Y 		; value

	SET A,0
	:loop
		SET [X+A],Y
		ADD A,1
		IFN A,C
			JMP loop

	PUSH Z
	RET
```

#### Example no. 5

##### test.asm
```
PUSH 0xF00D
PUSH 0x8
PUSH mem_1
JTR write_mem

PUSH 0x8
PUSH mem_1
PUSH mem_2
JTR copy_mem

END

.mem_1 	DAT 	0x1000
.mem_2	DAT 	0x2000
```

##### memory.asm
```
:write_mem
	POP Z

	POP X 		; mem_address
	POP C 		; count
	POP Y 		; value

	SET A,0
	:loop
		SET [X+A],Y
		ADD A,1
		IFN A,C
			JMP loop

	PUSH Z
	RET

:copy_mem
	POP Z

	POP X 		; dst_mem_addr
	POP Y 		; src_mem_addr
	POP C 		; count

	:loop_2
		SET [X+A],[Y+A]
		ADD A,1
		IFN A,C
			JMP loop_2
	PUSH Z
	RET
```

#### Macros

##### data

###### data.asm
```
.const_A        DAT     0x1000
.const_B        DAT     "Eggs"
.const_C        DAT     "On a Plane",0xF00D
```

##### include

###### data.asm
```
.const_A        DAT     0x1000
```

###### test.asm
```
SET A,const_A
SET B,[A]
SET [B],0xF00D
END

#include "data.asm"
```
