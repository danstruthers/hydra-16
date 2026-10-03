; ****************************************************************************
; tick - a sample of notes (the assembly SDK's: sdk/asm/README.md).  It says a dot each second (SLEEP) till a note
; comes (Ctrl-C at its window: the interrupt note), which its handler (NOTIFY) notes instead of the default (the
; task ended, with the note's name as its status); then it says how many seconds there were, and ends with code 0.
;   % tick
;   ...^C
;   3 seconds

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"

            HYX2_PROGRAM "tick", main

.bss
noted:      .res        1                                   ; The note that came (0: none yet)
count:      .res        1                                   ; The seconds

.code
main:
            stz         noted
            stz         count
            LDR         r0, handler                         ; Its note handler
            jsr         NOTIFY
@second:
            lda         #<TICK_HZ                           ; A second (TICK_HZ ticks): a note ends it sooner
            ldx         #>TICK_HZ
            jsr         SLEEP
            lda         noted
            bne         @end
            inc         count
            lda         #'.'
            jsr         PUTC
            bra         @second

@end:
            lda         #LF                                 ; "N seconds"
            jsr         PUTC
            lda         count
            ldx         #'0' - 1                            ; (Its tens)
:
            inx
            sec
            sbc         #10
            bcs         :-
            adc         #10
            pha
            cpx         #'0'
            beq         :+
            txa
            jsr         PUTC
:
            pla
            ora         #'0'
            jsr         PUTC
            PRINT       s_seconds
            lda         #0
            rts

; The note handler: .A = the note.  C = 0: the task goes on where it was (here, SLEEP ends early); C = 1 would be the
; default
handler:
            sta         noted
            clc
            rts

.rodata
s_seconds:  .byte       " seconds", LF, 0
