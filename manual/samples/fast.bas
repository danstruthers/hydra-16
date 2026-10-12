' fast.bas - the same job in HyBASIC and in machine code: count to 10,000
T = TIMER
C = 0
FOR I = 1 TO 10000 : C = C + 1 : NEXT
PRINT "HyBASIC counted to"; C; "in"; TIMER - T; "seconds"

ASM
count:  ldx #0          ; X and Y together: a 16-bit count
        ldy #0
again:  inx             ; Add 1 to the low byte ...
        bne skip
        iny             ; ... and when it goes round to 0, to the high byte
skip:   cpx #$10        ; 10,000 is $2710: stop when X is $10 and Y is $27
        bne again
        cpy #$27
        bne again
        rts
END ASM

T = TIMER
CALL ASM count
RREG A, X, Y, P
PRINT "Machine code counted to"; Y * 256 + X; "in"; TIMER - T; "seconds"
