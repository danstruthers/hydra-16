; ****************************************************************************
; ps - each task, as /proc has it: its number, then its status file's line (its name, state, parent, CPU time and
; note group), on fd 1.  It reads #p (the kernel's devices': /proc), not the namespace's /proc.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"

            HYX2_PROGRAM "ps", main

LINE_LEN        = 96

.zeropage
rec:        .res        2                                   ; A stat record, in buf

.bss
buf:        .res        16 * SR_SIZE                        ; #p's records (a task each: 16 at most)
left:       .res        2
fd:         .res        1
name:       .res        24                                  ; "#p/N/status"
line:       .res        LINE_LEN
llen:       .res        1

.code
main:
            LDR         r0, s_hp
            lda         #O_READ
            jsr         OPEN
            bcs         @done
            sta         fd
            LDR         r0, buf
            LDR         r1, 16 * SR_SIZE
            lda         fd
            jsr         READ
            sta         left
            stx         left + 1
            lda         fd
            jsr         CLOSE
            LDR         rec, buf
@record:
            lda         left + 1
            bne         :+
            lda         left
            cmp         #SR_SIZE
            bcc         @done
:
            jsr         task
            clc
            lda         rec
            adc         #SR_SIZE
            sta         rec
            bcc         :+
            inc         rec + 1
:
            sec
            lda         left
            sbc         #SR_SIZE
            sta         left
            bcs         @record
            dec         left + 1
            bra         @record

@done:
            lda         #0
            rts

; The task record rec names: "N  " and its status's line
task:
            ldx         #0                                  ; line: its number, a space or two
            ldy         #0
:
            lda         (rec),Y
            beq         :+
            sta         line,X
            inx
            iny
            bra         :-
:
            lda         #' '
:
            sta         line,X
            inx
            cpx         #4
            bcc         :-
            stx         llen
            ldx         #0                                  ; name: "#p/N/status"
:
            lda         s_hps,X
            sta         name,X
            inx
            cpx         #3
            bne         :-
            ldy         #0
:
            lda         (rec),Y
            beq         :+
            sta         name,X
            inx
            iny
            bra         :-
:
            ldy         #0
:
            lda         s_status,Y
            sta         name,X
            beq         :+
            inx
            iny
            bra         :-
:
            LDR         r0, name
            lda         #O_READ
            jsr         OPEN
            bcs         @write
            sta         fd
            clc                                             ; Its line, after the number
            lda         #<line
            adc         llen
            sta         r0
            lda         #>line
            adc         #0
            sta         r0 + 1
            sec
            lda         #LINE_LEN
            sbc         llen
            sta         r1
            stz         r1 + 1
            lda         fd
            jsr         READ
            bcs         :+
            clc
            adc         llen
            sta         llen
:
            lda         fd
            jsr         CLOSE
@write:
            LDR         r0, line
            lda         llen
            sta         r1
            stz         r1 + 1
            lda         #1
            jmp         WRITE

.rodata
s_hp:       .byte       "#p", 0
s_hps:      .byte       "#p/"
s_status:   .byte       "/status", 0
