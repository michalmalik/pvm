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