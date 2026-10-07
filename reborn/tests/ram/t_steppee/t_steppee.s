; ****************************************************************************
; t_steppee - a RAM program for t_step (the debugger's steps, phase 9), from a card: started stopped at its entry
; point (SPAWN_STOPPED), then stepped an instruction at a time through each kind TASKSTEP knows, t_step reading its
; PC and registers after each.  Each instruction's address is main + its offset, as the comments say (t_step's
; OFS_* are them).  Run on, it ends with code 0.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"

            HYX2_PROGRAM "t_steppee", main

.zeropage
val:        .res        1

.data
vec:        .word       jmpi_to                             ; (JMP (abs)'s)
vectab:     .word       0, jmpix_to                         ; (JMP (abs,X)'s, X = 2)

.code
main:
            lda         #$FE                                ; +0    run out of line
            sta         val                                 ; +2    (val's bit 0: 0)
            ldx         #0                                  ; +4
            clc                                             ; +6
            bcc         @t1                                 ; +7    taken: +10
            nop                                             ; +9
@t1:
            sec                                             ; +10
            bcc         @t1                                 ; +11   not taken: +13
            bbr0        val, @t2                            ; +13   taken: +17
            nop                                             ; +16
@t2:
            bbs0        val, @t1                            ; +17   not taken: +20
            jmp         @t3                                 ; +20   +23
@t3:
            jmp         (vec)                               ; +23   +26
jmpi_to:
            ldx         #2                                  ; +26
            jmp         (vectab,X)                          ; +28   +31
jmpix_to:
            jsr         sub                                 ; +31   into: sub (+59), its return address pushed
            jsr         sub                                 ; +34   over (TS_NEXT): +37, X 4
            lda         #>@t4                               ; +37
            pha                                             ; +39
            lda         #<@t4                               ; +40
            pha                                             ; +42
            php                                             ; +43
            rti                                             ; +44   +45
@t4:
            jsr         GETPID                              ; +45   the jump table's: run out of line, +48
            nop                                             ; +48
            nop                                             ; +49   (t_step's breakpoint)
            lda         #0                                  ; +50
            stz         r0                                  ; +52
            stz         r0 + 1                              ; +54
            jmp         EXITS                               ; +56

sub:
            inx                                             ; +59
            rts                                             ; +60
