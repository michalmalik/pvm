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