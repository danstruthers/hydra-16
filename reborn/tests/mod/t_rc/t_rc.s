; ****************************************************************************
; t_rc - rc (phase 4.2), run as init: the RAM disks started (r and s: 256K each, as init has them; s's bin and
; lib) and its namespace built (nslib's ns_default).  Then, its fds 0-2 still closed (its lines go out on the
; bring-up console), the times: rc -c 'x=1' from SPAWN to its end, and ls /bin (the caches, /rom/bin, then #m/bin;
; its output to #n/null).  Then its fds 0-2 #c/cons, $window 0, and rc -l (newns, then /rom/lib/profile) in a note
; group of its own, waited for: "t_rc: rc ended" when it does.  tests.js types rc's commands (the rc and tools
; tests) and looks for what they say.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "testlib.inc"

            HYX2_PROGRAM "t_rc", main

.bss
fd:         .res        1
code:       .res        1                                   ; A child's exit code
map:        .res        4                                   ; ls's fds: none, #n/null, none
msg:        .res        32

.code
main:
            LDR         r0, s_ctlr                          ; The RAM disks, and its namespace
            jsr         start
            LDR         r0, s_ctls
            jsr         start
            LDR         r0, s_sbin
            jsr         mkdir
            LDR         r0, s_slib
            jsr         mkdir
            jsr         ns_default

; ---- The times
            LDR         r0, s_null
            lda         #O_WRITE
            jsr         OPEN
            sta         map + 2
            EXPECT_OK   "OPEN #n/null"
            lda         #3
            sta         map
            lda         #$FF
            sta         map + 1
            sta         map + 3
            MARK        "<b0"                               ; (A baseline: the marks' own time)
            MARK        "b0>"
            MARK        "<rc"
            LDR         r0, s_rc
            LDR         r1, s_cx
            lda         #0
            jsr         SPAWN
            jsr         wait
            MARK        "rc>"
            lda         code
            pha                                             ; (Its code, for after the marks' line)
            MARK        "<ls"
            LDR         r0, s_ls
            LDR         r1, s_bin
            LDR         r2, map
            lda         #SPAWN_FDMAP
            jsr         SPAWN
            jsr         wait
            MARK        "ls>"
            SAY         " (the times)"
            pla
            EXPECT_A    0, "rc -c 'x=1': its end, code 0"
            lda         code
            EXPECT_A    0, "ls /bin: its end, code 0"
            lda         map + 2
            jsr         CLOSE

; ---- rc, for tests.js's lines
            LDR         r0, s_cons
            lda         #O_RDWR
            jsr         OPEN
            lda         #0
            jsr         DUP
            lda         #0
            jsr         DUP
            LDR         r0, s_window                        ; $window: 0
            LDR         r1, s_zero
            LDR         r2, 1
            stz         r3
            stz         r3 + 1
            lda         #$FF
            jsr         ENV_PUT
            LDR         r0, s_rc
            LDR         r1, s_l
            lda         #SPAWN_NEWGROUP | SPAWN_NEWNS
            jsr         SPAWN
            bcs         @ended
            pha
            LDR         r0, msg
            pla
            jsr         WAIT
            phx
            PRINT       s_ended
            pla                                             ; Its code and message
            jsr         PUTHEX
            lda         #' '
            jsr         PUTC
            PRINT       msg
            lda         #LF
            jsr         PUTC
:
            jsr         PAUSE
            bra         :-

@ended:
            PRINT       s_ended
:
            jsr         PAUSE
            bra         :-

; The child SPAWN gave (C and .A, SPAWN's) waited for.  OUT: code = its exit code ($FF: SPAWN failed)
wait:
            bcs         @failed
            pha
            LDR         r0, msg
            pla
            jsr         WAIT
            stx         code
            rts

@failed:
            lda         #$FF
            sta         code
            rts

; The RAM disk whose ctl is r0 started: 32 banks
start:
            lda         #O_WRITE
            jsr         OPEN
            bcs         @done
            sta         fd
            LDR         r0, s_start
            LDR         r1, 8
            lda         fd
            jsr         WRITE
            lda         fd
            jsr         CLOSE
@done:
            rts

; The directory r0 made
mkdir:
            lda         #O_READ
            ldx         #DM_DIR
            jsr         CREATE
            bcs         :+
            jsr         CLOSE
:
            rts

.rodata
s_cons:     .byte       "#c/cons", 0
s_ctlr:     .byte       "#d/r/ctl", 0
s_ctls:     .byte       "#d/s/ctl", 0
s_start:    .byte       "start 32"
s_sbin:     .byte       "#fs/bin", 0
s_slib:     .byte       "#fs/lib", 0
s_null:     .byte       "#n/null", 0
s_ls:       .byte       "/bin/ls", 0
s_bin:      .byte       "/bin", 0, 0
s_cx:       .byte       "-c", 0, "x=1", 0, 0
s_window:   .byte       "window", 0
s_zero:     .byte       "0"
s_rc:       .byte       "#m/rc", 0
s_l:        .byte       "-l", 0, 0
s_ended:    .byte       "t_rc: rc ended ", 0

.include "nslib.s"
