; ****************************************************************************
; hi [name ...] - a sample RAM program (the assembly SDK's: sdk/asm/README.md).  It greets each name it's given (or
; you), then says where it runs: its task (GETPID), its current directory (GETCWD) and its window ($window, from its
; environment: ENV_GET).
;   % hi Ann Bob
;   Hello, Ann!
;   Hello, Bob!
;   I'm task 3, in /ram, in window 0.

.include "hydra.inc"                                        ; The calls and constants (made from spec/api.def)
.include "hyx2.inc"                                         ; The header: HYX2_PROGRAM
.include "macros.inc"                                       ; LDR, MOVR, PRINT, CALL, CHECK

            HYX2_PROGRAM "hi", main                         ; Its name, and where it starts

.zeropage
arg:        .res        2                                   ; An argument (its zero page: $22-$7F)

.bss
cwd:        .res        PATH_MAX + 1                        ; Its current directory
window:     .res        8                                   ;   and window

.code

; Its arguments at r0: strings one after another, each zero-terminated, an empty one after the last.  Returning
; ends it, with code 0 (EXITS)
main:
            MOVR        arg, r0
            lda         (arg)
            bne         @name
            LDR         arg, s_you                          ; None: you
@name:
            PRINT       "Hello, "                           ; "Hello, NAME!"
            MOVR        r0, arg
            CALL        PUTS
            PRINT       s_bang
:
            lda         (arg)                               ; The next: past this one's 0
            inc         arg
            bne         :+
            inc         arg + 1
:
            cmp         #0
            bne         :--
            lda         (arg)
            bne         @name
            PRINT       "I'm task "                         ; Its task: 0-15, in decimal
            CALL        GETPID
            cmp         #10
            bcc         :+
            pha
            lda         #'1'
            CALL        PUTC
            pla
            sec
            sbc         #10
:
            ora         #'0'
            CALL        PUTC
            PRINT       ", in "                             ; Its current directory
            LDR         r0, cwd
            CALL        GETCWD
            PRINT       cwd
            LDR         r0, s_window                        ; Its window, if its environment has one
            LDR         r1, window
            LDR         r2, 7
            stz         r3
            stz         r3 + 1
            lda         #$FF                                ; (This task's)
            CALL        ENV_GET
            CHECK       @end                                ; (Not there: C = 1, .A = E_NOENT)
            tax
            stz         window,X                            ; (Its value: zero-terminated)
            PRINT       ", in window "
            PRINT       window
@end:
            PRINT       s_end
            rts

.rodata
s_you:      .byte       "you", 0, 0
s_bang:     .byte       "!", LF, 0
s_window:   .byte       "window", 0
s_end:      .byte       ".", LF, 0
