 JMP start

#define CONST_C		0x30
.const_A	DAT	0x10

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

.const_B	DAT	0x20
.const_D	DAT	0x40

:end
