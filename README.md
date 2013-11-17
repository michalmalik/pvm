### PCPU Specifications
* **16-bit** CPU, 16-bit words
* experimental **1 Mhz** clock 
* **0x8000 words**
* **8 16-bit general purpose registers** (A,B,C,D,X,Y,Z,J), **stack pointer** (SP), **instruction pointer** (IP), **interrupt address** (IA)
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
| 0x00   | STO A,B          | A = B                               		     | 1-3     |
| 0x01   | ADD A,B          | A = A+B ; sets OF flag              		     | 1-3     |               
| 0x02   | SUB A,B          | A = A-B ; sets OF flag              		     | 1-3     |
| 0x03   | MUL A,B          | A = A*B ; sets OF flag              		     | 1-3     |
| 0x04   | DIV A,B          | A = A/B ; D = A%B                   		     | 2-5     |
| 0x05   | MOD A,B          | A = A%B                   	  		     | 1-3     |
| 0x06   | NOT A            | A = ~(A)                            		     | 1-2     |
| 0x07   | AND A,B          | A = A&B                             		     | 1-3     |
| 0x08   | OR A,B           | A = A OR B                          		     | 1-3     |
| 0x09   | XOR A,B          | A = A^B                             		     | 1-3     |
| 0x0A   | SHL A,B          | A = A << B                          		     | 1-3     |
| 0x0B   | SHR A,B          | A = A >> B                          		     | 1-3     |
| 0x0C   | IFE A,B          | execute next instruction if A==B    		     | 1-3     |
| 0x0D   | IFN A,B          | if A!=B            		  		     | 1-3     |
| 0x0E   | IFG A,B          | if A>B            		  		     | 1-3     |
| 0x0F   | IFL A,B          | if A<B           		   	  		     | 1-3     |
| 0x10   | IFGE A,B         | if A>=B                             		     | 1-3     |
| 0x11   | IFLE A,B         | if A<=B                             		     | 1-3     |
| 0x12   | JMP label        | jump to label            		  		     | 2       |
| 0x13   | JTR label        | push IP of next instruction on stack, jump to label    | 3       |
| 0x14   | PUSH A           | push A on stack, SP--	  		     	     | 1-2     |
| 0x15   | POP A            | pops value from stack to A, SP++         		     | 1-2     |
| 0x16   | RET              | pops value from stack to IP                 	     | 1       |
| 0x17   | RETI             | sets IA to 0, pops value from stack to IP              | 1       |

#### Interrupt instructions
------------------------------------------------------------------------------------------------
| OP     | INS              | description                                            | cycles  |
| :----: | ---------------- | ------------------------------------------------------ | :-----: |
| 0x18   | IAR A            | set IA to A                                            | 2       |
| 0x19   | INT A            | jump to IA, with the message A set to register A       | 1-2     |


### Assembly language

#### Example no. 1
```
STO A,0x1000
STO B,[A]
```

#### Example no. 2
```
STO A,0
STO J,10
:loop
	ADD A,1
	IFN A,J
		JMP loop
```

#### Example. no 3
```
JMP start

:table
    DAT 0xF00D
    DAT 0xBEEF
    DAT 0xFEED
    DAT 0xBEEE

:start
    STO A,0
    STO J,4

    ; Loop through table
    :loop
        STO [0x1000+A],[table+A]
        ADD A,1
        IFN A,J
            JMP loop
```

#### Example no. 4
```
JMP start

.mem_address 	DAT 	0

:write_mem
	STO Z,SP

	STO [mem_address],[Z+1]		; addr
	STO Y,[Z+2]			; value
	STO J,[Z+3]			; count

	STO A,0
	:write_mem_loop
		STO X,[mem_address]
		ADD X,A
		STO [X],Y
		ADD A,1
		IFN A,J
			JMP write_mem_loop
	
	STO SP,Z
	RET

:start

PUSH 0x8
PUSH 0xF00D
PUSH 0x1000
JTR write_mem
ADD SP,3
```

#### Example no. 5

##### test.asm
```
JMP start

#include "memory.asm"

:start

PUSH 0x8 		; count
PUSH 0xF00D		; value
PUSH 0x1000 		; mem_address
JTR write_mem
ADD SP,3

PUSH 0x8 		; count
PUSH 0xBEAF 		; value
PUSH 0x2000 		; mem_address
JTR write_mem
ADD SP,3
```

##### memory.asm
```
.dst_addr	DAT	0
.src_addr	DAT	0

:write_mem
	STO Z,SP

	STO [dst_addr],[Z+1]	; dst_addr
	STO Y,[Z+2]		; value
	STO J,[Z+3]		; count

	STO A,0
	:loop
		STO X,[dst_addr]
		ADD X,A
		STO [X],Y
		ADD A,1
		IFL A,J
			JMP loop 
	STO SP,Z
	RET

:copy_mem
	STO Z,SP

	STO [dst_addr],[Z+1]	; dst_addr
	STO [src_addr],[Z+2]	; src_addr
	STO J,[Z+3]		; count

	STO A,0
	:loop_2
		STO X,[dst_addr]
		STO Y,[src_addr]
		ADD X,A
		ADD Y,A
		STO [X],[Y]
		ADD A,1
		IFL A,J
			JMP loop_2
	STO SP,Z
	RET

```

#### Example no. 6

##### test.asm
```
JMP start

#include "memory.asm"

.mem_1 	DAT 	0x1000
.mem_2	DAT 	0x2000

:start

PUSH 0x10
PUSH 0xF00D
PUSH [mem_1]
JTR write_mem
ADD SP,3

PUSH 0x8
PUSH [mem_1]
PUSH [mem_2]
JTR copy_mem
ADD SP,3
```

##### memory.asm
```
.dst_addr	DAT	0
.src_addr	DAT	0

:write_mem
	STO Z,SP

	STO [dst_addr],[Z+1]	; dst_addr
	STO Y,[Z+2]		; value
	STO J,[Z+3]		; count

	STO A,0
	:loop
		STO X,[dst_addr]
		ADD X,A
		STO [X],Y
		ADD A,1
		IFL A,J
			JMP loop 
	STO SP,Z
	RET

:copy_mem
	STO Z,SP

	STO [dst_addr],[Z+1]	; dst_addr
	STO [src_addr],[Z+2]	; src_addr
	STO J,[Z+3]		; count

	STO A,0
	:loop_2
		STO X,[dst_addr]
		STO Y,[src_addr]
		ADD X,A
		ADD Y,A
		STO [X],[Y]
		ADD A,1
		IFL A,J
			JMP loop_2
	STO SP,Z
	RET

```

#### Example no. 7
```
JMP start

; struct Person
#define PERSON_AGE		0
#define PERSON_HEIGHT		1

#define ST_PERSON_MICHAL	0x1000

; +3 PERSON_HEIGHT
; +2 PERSON_AGE
; +1 PERSON_ADDR
:create_person
	STO Z,SP

	STO Y,[Z+1]		; PERSON_ADDR
	STO B,[Z+2]		; PERSON_AGE
	STO C,[Z+3]		; PERSON_HEIGHT

	STO [Y+PERSON_AGE],B
	STO [Y+PERSON_HEIGHT],C

	STO SP,Z
	RET

:start

PUSH 0xB4		; height
PUSH 0x12		; age
PUSH ST_PERSON_MICHAL	; addr
JTR create_person
ADD SP,3
```

#### Example no. 8

##### ivt.asm
```
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
		JTR handle_ivt_syscall

	RETI

:handle_ivt_syscall
	; handle_ivt_syscall_read
	; set J to 0xAAAA
	IFE B,IVT_SYSCALL_READ
		STO J,0xAAAA

	; handle_ivt_syscall_write
	IFE B,IVT_SYSCALL_WRITE
		STO J,0xBBBB

	; handle_ivt_syscall_exit
	IFE B,IVT_SYSCALL_EXIT
		STO J,0xCCCC

	RET
```

##### test.asm
```
JMP start

#include "ivt.asm"

:start
	IAR handle_ivt_interrupt

	; SYSCALL routine is determined
	; by the register B and the return value
	; by the register J for the moment
	; 1 - read, 0xAAAA
	; 2 - write, 0xBBBB
	; 3 - exit, 0xCCCC

	; SYSCALL is called from IVT
	; by software interrupt 0xAA

	; J should be 0xAAA
	STO B,1 	; read
	INT 0xAA

	; when INT is called, IA is zeroed
	IAR handle_ivt_interrupt
	STO B,2 	; write
	INT 0xAA

	IAR handle_ivt_interrupt
	STO B,3 	; exit
	INT 0xAA
```

#### ASSEMBLER TEST: Example no. 9
```
JMP start

#define CONST_C 	0x30
.const_A 	DAT 	0x10

:start

STO A,const_A
STO A,const_B
STO A,[B+const_A]
STO A,[B+const_B]
STO A,[const_A+B]
STO A,[const_B+B]
STO A,[const_A+0x10]
STO A,[const_B+0x10]
STO A,[0x10+const_A]
STO A,[0x10+const_B]
STO A,[const_A]
STO A,[const_B]
STO A,[const_A+CONST_C]
STO A,[const_B+CONST_C]
STO A,[CONST_C+const_A]
STO A,[CONST_C+const_B]
STO A,[const_B+const_B]	
STO A,[const_B+const_D]	

JMP end

.const_B 	DAT 	0x20
.const_D	DAT 	0x40

:end
```

#### ASSEMBLER TEST: Example no. 10

Uncomment those you want to test.

##### asm_test.asm
```
; included file not defined
;#include

; included file empty
;#include ""

; included file does not exist
;#include "ff.asm"

; add_symbol multiple definitions test
.const_A	DAT	0x1000
;.const_A	DAT 	0x2000

; redaclaration of variables in a different file
;#include "asm_test_data.asm"

; add_define multiple definitions test
#define CONST_B		0x1000
;#define CONST_B		0x2000

; fix_symbols symbol not defined test
;STO A, const_C

; assemble_o expected operand test
;STO A,
;STO ,B

; assemble_o expected ] test
;STO A,[B
;STO [A,B

; assemble_o [reg +value] test
;STO A,[B+]
;STO [B+],A

; assemble_o [num +value] test
;STO A,[0x100+]
;STO [0x100+],A

; assemble_o [str +value] test
;STO A,[const_A+]
;STO [const_A+],A

; assemble expected value after DAT test
;.const_D	DAT

; assemble expected value after multiple DAT
;.const_D	DAT 	0x3000,

; data table
:data_table
	;DAT
	DAT 0x1000
	;DAT
	DAT 0x2000
	;DAT
	DAT 0x3000
	;DAT
	;DAT 0x1000,
	;DAT
```

##### asm_test_data.asm
```
.const_A	DAT	0x1000
```

#### Macros

##### define

Currently, there's no limit for defines for a single file. 

###### data.asm
```
JMP start

#define MEM_ADDR 	0x1000

:start

STO [MEM_ADDR],0xF00D
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

STO A,const_A
STO B,[A]
STO [B],0xF00D
```
