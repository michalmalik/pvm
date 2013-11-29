; included file not defined
;#include

; included file empty
;#include ""

; included file does not exist
;#include "ff.asm"

; add_symbol multiple definitions test
;.const_A	DAT	0x1000
;.const_A	DAT 	0x2000

; redaclaration of variables in a different file
;#include "asm_test_data.asm"

; redeclaration of variables in an included file
; from an already included file
;#include "asm_test_data_2.asm"

; add_define multiple definitions test
;#define CONST_B		0x1000
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
STO A,[const_A+]
;STO [const_A+],A

; assemble expected value after DAT test
;.const_D	DAT

; assemble expected value after multiple DAT
;.const_D	DAT 	0x3000,

; data table
:data_table
	;DAT
	;DAT 0x1000
	;DAT
	;DAT 0x2000
	;DAT
	;DAT 0x3000
	;DAT
	;DAT 0x1000,
	;DAT