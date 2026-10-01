.debuginfo

; ****************************************************************************
; hello.s - a sample Hydra executable (.hyx): it says hello, with its task number and its arguments (hello a b:
; "Hello from task B: a b"), and returns, which ends its task (and the shell's prompt comes back).
;
;   Build it with make.bat, or build.js asm (ca65, then ld65 with hyx.cfg, which puts the header on it: bin\hello.hyx).
; Put it on a card (sim/tools/hydrafs.js put, or cp on the Hydra), in the current directory or the card's
; /bin, and type hello (or run hello.hyx).

.include "hyx.inc"

WRITE_CHAR      = $F803                             ; .A to stdout
WRITE_HEX_MASK  = $F80C                             ; .A's low 4 bits to stdout, as a hex digit
T_REGISTER      = $FFF0                             ; This task's number

            HYX_HEADER  start

.code

ARGS            = $F0                               ; (Zero page from $A9 up is the program's)

start:
            sta         ARGS                            ; .A.Y: the arguments (zero-terminated)
            sty         ARGS + 1
            ldx         #0
@char:
            lda         msg,X
            beq         @task
            jsr         WRITE_CHAR
            inx
            bne         @char
@task:
            lda         T_REGISTER
            jsr         WRITE_HEX_MASK
            ldy         #0                              ; ": " and the arguments, if there are any
            lda         (ARGS),Y
            beq         @end
            lda         #':'
            jsr         WRITE_CHAR
            lda         #' '
@arg:
            jsr         WRITE_CHAR
            lda         (ARGS),Y
            iny
            cmp         #0
            bne         @arg
@end:
            lda         #13
            jsr         WRITE_CHAR
            lda         #10
            jmp         WRITE_CHAR                  ; (Its rts ends the program)

.rodata

msg:        .byte       "Hello from task ", 0
