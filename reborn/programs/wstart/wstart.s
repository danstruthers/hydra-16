; ****************************************************************************
; wstart - a shell in the next window the user asks for, as rio's: it waits for the user's Ctrl-] c (a read of
; #c/wnew: the window made, "N" (0-15)), then starts the shell there (its arguments: the shell's program and its own, as
; init has them from /lib/shell; none, rc -l), its fds 0-2 the window's cons, $window N in the environment it copies,
; a note group and an empty namespace of its own (its profile sets them up), and ends.  init starts it in its own
; namespace, so the shell's program is found as init finds it, and starts it again (and the shell, an orphan now, is
; init's to wait for).  One that fails waits a second first, so init's starting it again isn't a loop.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"

            HYX2_PROGRAM "wstart", main

.bss
fd:         .res        1
digits:     .res        1                                   ; The window's number's digits
buf:        .res        4
name:       .res        12                                  ; "#cN/cons"
map:        .res        4
prog:       .res        2                                   ; The shell's program ...
args:       .res        2                                   ;   and its arguments

.code
main:
            LDR         prog, s_rc                          ; The shell: its arguments', or rc -l
            LDR         args, s_l
            lda         r0
            ora         r0 + 1
            beq         @window
            lda         (r0)
            beq         @window
            lda         r0
            sta         prog
            lda         r0 + 1
            sta         prog + 1
            ldy         #0                                  ; (Its arguments: after the program's 0)
:
            lda         (r0),y
            beq         :+
            iny
            bne         :-
:
            iny
            clc
            tya
            adc         r0
            sta         args
            lda         r0 + 1
            adc         #0
            sta         args + 1
@window:
            LDR         r0, s_wnew                          ; The window: "N"
            lda         #O_READ
            jsr         OPEN
            bcc         :+
            jmp         @failed
:
            sta         fd
            LDR         r0, buf
            LDR         r1, 4
            lda         fd
            jsr         READ
            php
            lda         fd
            jsr         CLOSE
            plp
            bcc         :+
            jmp         @failed
:
            ldx         #0                                  ; (Its digits: one or two)
:
            lda         buf,X
            cmp         #'0'
            bcc         :+
            inx
            cpx         #2
            bcc         :-
:
            stx         digits
            LDR         r0, s_window                        ; $window: N
            LDR         r1, buf
            lda         digits
            sta         r2
            stz         r2 + 1
            stz         r3
            stz         r3 + 1
            lda         #$FF
            jsr         ENV_PUT
            lda         #'#'                                ; "#cN/cons"
            sta         name
            lda         #'c'
            sta         name + 1
            ldx         #0
:
            lda         buf,X
            sta         name + 2,X
            inx
            cpx         digits
            bcc         :-
            ldy         #0
:
            lda         s_cons + 3,Y                        ; ("/cons", and its zero)
            sta         name + 2,X
            inx
            iny
            cpy         #6
            bcc         :-
            LDR         r0, name
            lda         #O_RDWR
            jsr         OPEN
            bcs         @failed
            sta         map + 1                             ; Its fds 0-2: the window's
            sta         map + 2
            sta         map + 3
            lda         #3
            sta         map
            lda         prog
            sta         r0
            lda         prog + 1
            sta         r0 + 1
            lda         args
            sta         r1
            lda         args + 1
            sta         r1 + 1
            LDR         r2, map
            lda         #SPAWN_NEWGROUP | SPAWN_NEWNS | SPAWN_FDMAP
            jsr         SPAWN
            bcs         @failed
            lda         #0
            rts

@failed:
            lda         #<TICK_HZ
            ldx         #>TICK_HZ
            jsr         SLEEP
            lda         #0
            rts

.rodata
s_wnew:     .byte       "#c/wnew", 0
s_window:   .byte       "window", 0
s_cons:     .byte       "#cN/cons", 0
s_rc:       .byte       "#m/rc", 0
s_l:        .byte       "-l", 0, 0
