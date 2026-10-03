; ****************************************************************************
; wstart - a shell in the next window the user asks for, as rio's: it waits for the user's Ctrl-] c (a read of
; #c/wnew: the window made, "N"), then starts rc -l there (its fds 0-2 the window's cons; $window N, in the
; environment it copies; a note group and an empty namespace of its own: its profile sets them up), and ends.
; init starts it again (and the shell, an orphan now, is init's to wait for).  One that fails waits a second
; first, so init's starting it again isn't a loop.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"

            HYX2_PROGRAM "wstart", main

.bss
fd:         .res        1
buf:        .res        4
name:       .res        12                                  ; "#cN/cons"
map:        .res        4

.code
main:
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
            LDR         r0, s_window                        ; $window: N
            LDR         r1, buf
            LDR         r2, 1
            stz         r3
            stz         r3 + 1
            lda         #$FF
            jsr         ENV_PUT
            ldx         #0                                  ; "#cN/cons"
:
            lda         s_cons,X
            sta         name,X
            inx
            cpx         #8
            bne         :-
            lda         buf
            sta         name + 2
            LDR         r0, name
            lda         #O_RDWR
            jsr         OPEN
            bcs         @failed
            sta         map + 1                             ; Its fds 0-2: the window's
            sta         map + 2
            sta         map + 3
            lda         #3
            sta         map
            LDR         r0, s_rc
            LDR         r1, s_l
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
