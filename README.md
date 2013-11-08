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
* operation code is 2 bytes `0xDDDDDSSSSSOOOOOO`
--* destination - 5 bits (max. 0x1F)
--* source - 5 bits (max. 0x1F)
--* operation - 6 bits (max. 0x3F)