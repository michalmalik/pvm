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

| opcode 	| detail	   | detail				    |
| ------------- | ---------------- | -------------------------------------- |
| 0x00 - 0x06	| register 	   | literal value of the register          |
| 0x07 - 0x0D	| [register]	   | value at register 			    |
| 0x0E - 0x14	| [register+A]     | value at register + A                  |
| 0x15 - 0x1B	| [register+nextw] | value at register + next word          |
| 0x1C		| nextw		   | literal value of next word             |
| 0x1D		| [nextw]          | value at next word                     |
| 0x1E		| SP               | literal value of stack pointer         |
| 0x1F		| IP               | literal value of instruction pointer   |
