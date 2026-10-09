' asm.bas - inline assembly: ASM ... END ASM (as's language, the asm library's assembler), CALL ASM's labels and
' their registers (RREG), the program's variables and CONSTs by name (in capitals and in lower case), data in a
' block, a block in a SUB, a label in a later block, macros, .if, cheap and unnamed labels, .include (hydra.inc: a
' system call), a label named as a variable, one named as a keyword (DOUBLE), and the code's own bank
DIM SHARED checks, failed
SUB ck (got, want, what$)
    checks = checks + 1
    IF got <> want THEN failed = failed + 1: PRINT "FAIL "; what$; ":"; got; "not"; want
END SUB

CONST K = 5, BIG = &H1234
count = 7
total = 0
n = 0

' Labels called, their registers in and out
CALL ASM add2, 40: RREG a
ck a, 42, "CALL ASM: ADC #2"
CALL ASM regs, 1, 2, 3: RREG a, x, y, p
ck a * 100 + x * 10 + y, 432, "its registers in and out"
ck p AND 1, 1, "the carry after (RREG's flags)"
CALL ASM add2
RREG a
ck a, 2, "the registers not given: 0"

' The program's names: a variable (its value's address), a CONST (its value)
CALL ASM bump: RREG a
ck count, 8, "INC COUNT + 1: the variable's integer"
ck a, 5, "LDA #K: a CONST"
CALL ASM big: RREG a, x
ck x * 256 + a, &H1234, "<BIG, >BIG"
CALL ASM settotal, 200
ck total, 200, "STA total + 1: a variable in lower case"
CALL ASM settotal, 255
ck total, 255, "again"

' Data in a block: a table, by its index
FOR i = 0 TO 3
    CALL ASM square, 0, i: RREG a
    ck a, i * i, "LDA squares,X"
NEXT

' A block in a SUB; a label in a later block; a macro; .if; cheap and unnamed labels
CALL ASM double, 21: RREG a
ck a, 42, "a SUB's block; a label named as a keyword: ASL A"
CALL ASM far, 9: RREG a
ck a, 10, "JMP to a later block's label"
CALL ASM addten, 1: RREG a
ck a, 11, "a macro"
CALL ASM which: RREG a
ck a, 2, ".if"
CALL ASM sumto, 10: RREG a
ck a, 55, "a loop: @loop, :-"

' A system call from a block (hydra.inc, from /lib/as); a label named as a variable is the block's
SYS "GETPID": RREG p1
CALL ASM pid: RREG p2
ck p2, p1, "JSR GETPID"
CALL ASM n: RREG a
ck a, 77, "a label N, though N's a variable"
ck n, 0, "N itself"

' The blocks' bank: their own, at $8000
CALL ASM where: RREG a, x
ck x * 256 + a, &H8000, "the code's first address"
CALL ASM keep, 99
CALL ASM fetch: RREG a
ck a, 99, "a byte kept in the block's .bss"

PRINT "asm:"; checks; "checks,"; failed; "failed"
END

ASM
first:                          ; (The bank's first byte: where)
add2:   clc
        adc #2
        rts
regs:   phx                     ; .A = .Y + 1, .X = .A + 2, .Y = .X; C set
        tax
        inx
        inx
        iny
        tya
        ply
        sec
        rts
bump:   inc COUNT + 1           ; (count: 7, an integer: its kind 0, then its 4 bytes)
        lda #K
        rts
big:    lda #<BIG
        ldx #>BIG
        rts
settotal:
        stz total               ; (Its kind: an integer's)
        sta total + 1
        stz total + 2
        stz total + 3
        stz total + 4
        rts
square: lda squares,x
        rts
squares: .byte 0, 1, 4, 9
END ASM

SUB helpers
    ASM
double: asl a
        rts
    END ASM
END SUB

ASM
far:    jmp later
.macro  ADDN n
        clc
        adc #n
.endmacro
addten: ADDN 10
        rts
.if K > 3
which:  lda #2
.else
which:  lda #1
.endif
        rts
sumto:  tax                     ; 1 + 2 + ... + .A
        lda #0
@loop:  stx sum
        clc
        adc sum
        dex
        bne @loop
        rts
.include "hydra.inc"
pid:    jsr GETPID
        rts
n:      bra :+                  ; (An unnamed label)
        lda #1
:       lda #77
        rts
where:  lda #<first
        ldx #>first
        rts
keep:   sta kept
        rts
fetch:  lda kept
        rts
END ASM

ASM
later:  inc a
        rts
.bss
sum:    .res 1
kept:   .res 1
END ASM
