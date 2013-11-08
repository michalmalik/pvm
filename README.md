### PCPU Specifications
* **16-bit** CPU
* **0x8000 words** of memory available (0x10000 bytes)
* 7 registers (A, B, C, D, X, Y, Z), stack pointer (SP), instruction pointer (IP or program counter)
* 3 flags (E, S, O) in FLAGS register
* instruction takes maximum **6 bytes** 
* **0x2000 words** global stack size, start `0x7FFF`, goes down
* **0x500 words** screen size (80x32 bytes), start `0x0`

### Instruction specifications
* **FLAGS** bit pattern: `0x00000OSE`
* instruction opcode is 2 bytes `0xDDDDDSSSSSOOOOOO`
* **destination** - 5 bits (max. 0x1F)
* **source** - 5 bits (max. 0x1F)
* **operation** - 6 bits (max. 0x3F)

# Opcodes for destination,source

| opcode 	| detail	   | description				    |
| ------------- | ---------------- | -------------------------------------- |
| 0x00 - 0x06	| register 	   | literal value of the register          |
| 0x07 - 0x0D	| [register]	   | value at register 			    |
| 0x0E - 0x14	| [register+A]     | value at register + A                  |
| 0x15 - 0x1B	| [register+nextw] | value at register + next word          |
| 0x1C		| nextw		   | literal value of next word             |
| 0x1D		| [nextw]          | value at next word                     |
| 0x1E		| SP               | literal value of stack pointer         |
| 0x1F		| IP               | literal value of instruction pointer   |

# Instruction set

| OP     | INS              | detail                              | description                         |
| ------ | ---------------- | ----------------------------------- | ----------------------------------- |
| 0x01   | SET A,B          | A = B                               |                                     |
| 0x02   | CMP A,B          | compares A with B                   | if A == B -> E = 1;if A > B -> S = 1|                                                    
| 0x03   | ADD A,B          | A = A+B ; sets O flag               |                                     |
| 0x04   | SUB A,B          | A = A-B ; sets O flag               |                                     |
| 0x05   | MUL A,B          | A = A*B                             |                                     |
| 0x06   | DIV A,B          | A = A/B ; D = A%B                   |                                     |
| 0x07   | MOD A,B          | A = A%B                             |                                     |
| 0x08   | NOT A            | A = ~A                              |                                     |
| 0x09   | AND A,B          | A = A AND B                         |                                     |
| 0x0A   | OR A,B           | A = A|B                             |                                     |
| 0x0B   | XOR A,B          | A = A^B                             |                                     |
| 0x0C   | SHL A,B          | A = A << B                          |                                     |
| 0x0D   | SHR A,B          | A = A >> B                          |                                     |
| 0x0E   | JMP offset       | jumps to offset                     |                                     |
| 0x0F   | JE offset        | jump to offset if E == 1            |                                     |
| 0x10   | JNE offset       | jump to offset if E == 0            |                                     |
| 0x11   | JG offset        | jump to offset if S == 1            |                                     |
| 0x12   | JL offset        | jump to offset if S == 0            |                                     |
| 0x13   | JGE offset       | jump to offset if (E == 1 | S == 1) |                                     |
| 0x14   | JLE offset       | jump to offset if (E == 1 | S == 0) |                                     |
| 0x15   | JTR offset       | jump to routine ; PUSH IP+1         |                                     |
| 0x16   | RET              | POP IP ; jump                       |                                     |
| 0x17   | PUSH A           | pushes A on stack ; --SP            |                                     |
| 0x18   | POP A            | pops [SP] to A ; ++SP               |                                     |
| 0x19   | END              | ends program execution              |                                     |
|   -    | DAT w            | writes literal value to memory      |                                     |
