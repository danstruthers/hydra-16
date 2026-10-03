; ****************************************************************************
; init - the first program (task 1), for phase 2: its fds 0-2 on the console (#c/cons, the console driver's window
; 0; or, without it, none, and the bring-up console's); its namespace, built in till there are disks (the plan's
; appendix E, as far as the devices there are go: #/ at /, #c, #n and #t at /dev, #m at /dev/mod, #p at /proc); the
; tasks listed; hello run and waited for; then the shells (tsh, till rc comes in phase 3): window 0's, and the
; windows' starter (tsh w: a shell in each window the user asks for, Ctrl-] c), each started again when it ends.
; It waits for every task left to it (the windows' shells are).  Its note handler keeps it going.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"

            HYX2_PROGRAM "init", main

.zeropage
child:      .res        1
sh0:        .res        1                                   ; Window 0's shell ...
sw:         .res        1                                   ;   and the windows' starter

.bss
msg:        .res        32                                  ; An exit message, an error's text

.code
main:
            LDR         r0, notes                           ; (Notes don't end it)
            jsr         NOTIFY
            LDR         r0, s_cons                          ; Fds 0-2: the console
            lda         #O_RDWR
            jsr         OPEN
            bcs         :+                                  ; (No console driver: the bring-up console's)
            lda         #0
            jsr         DUP
            lda         #0
            jsr         DUP
:
            jsr         namespace
            PRINT       s_up
            jsr         GETPID
            jsr         PUTHEX
            PRINT       s_crlf
            jsr         DBG_PS
            LDR         r0, s_hello                         ; hello, with arguments, and its end
            LDR         r1, s_args
            lda         #0
            jsr         SPAWN
            bcs         @failed
            sta         child
            LDR         r0, msg
            lda         child
            jsr         WAIT
            bcs         @failed
            phx
            PRINT       s_ended                             ; hello ended: code $07 ("bye")
            pla
            jsr         PUTHEX
            PRINT       s_open
            PRINT       msg
            PRINT       s_close
            bra         shells

@failed:
            jsr         error

; ****************************************************************************
; The shells: window 0's and the windows' starter, started again when they end; the rest just waited for
shells:
            jsr         shell0
            jsr         starter
@wait:
            stz         r0
            stz         r0 + 1
            lda         #$FF
            jsr         WAIT
            bcc         @ended
            cmp         #E_CHILD                            ; (None: a moment, then try again)
            bne         @wait
            lda         #TICK_HZ
            ldx         #0
            jsr         SLEEP
            jsr         shell0
            jsr         starter
            bra         @wait

@ended:
            cmp         sh0
            bne         :+
            jsr         shell0
            bra         @wait
:
            cmp         sw
            bne         @wait
            jsr         starter
            bra         @wait

; Window 0's shell (a note group of its own: its window's notes are its), or the windows' starter, started
shell0:
            LDR         r0, s_tsh
            LDR         r1, s_w0
            lda         #SPAWN_NEWGROUP
            jsr         SPAWN
            sta         sh0
            bcc         :+
            lda         #$FF
            sta         sh0
:
            rts

starter:
            LDR         r0, s_tsh
            LDR         r1, s_ww
            lda         #0
            jsr         SPAWN
            sta         sw
            bcc         :+
            lda         #$FF
            sta         sw
:
            rts

; The namespace, built in (a bind each: its flags, new, old): what can't be bound is said, and the rest goes on
namespace:
            ldx         #0
@entry:
            lda         ns_table,X
            cmp         #$FF
            beq         @done
            pha
            lda         ns_table + 1,X
            sta         r0
            lda         ns_table + 2,X
            sta         r0 + 1
            lda         ns_table + 3,X
            sta         r1
            lda         ns_table + 4,X
            sta         r1 + 1
            pla
            phx
            jsr         BIND
            bcc         :+
            jsr         error
:
            pla
            clc
            adc         #5
            tax
            bra         @entry

@done:
            rts

; The error .A, said: "init: its text"
error:
            pha
            PRINT       s_error
            LDR         r0, msg
            pla
            jsr         ERRSTR
            PRINT       msg
            PRINT       s_crlf
            rts

; The note handler: init goes on, whatever the note (but a kill, which isn't caught)
notes:
            clc
            rts

.rodata
s_cons:     .byte       "#c/cons", 0
s_up:       .byte       "init: up in task ", 0
s_hello:    .byte       "#m/hello", 0
s_args:     .byte       "from init", 0
s_ended:    .byte       "init: hello ended: code $", 0
s_open:     .byte       " (", 0
s_close:    .byte       ")"
s_crlf:     .byte       CR, LF, 0
s_error:    .byte       "init: ", 0
s_tsh:      .byte       "#m/tsh", 0
s_w0:       .byte       "0", 0
s_ww:       .byte       "w", 0
ns_table:   .byte       MREPL                               ; bind '#/' /
            .word       s_hroot, s_root
            .byte       MAFTER                              ; bind -a '#c' /dev
            .word       s_hcons, s_dev
            .byte       MAFTER                              ; bind -a '#n' /dev
            .word       s_hnull, s_dev
            .byte       MAFTER                              ; bind -a '#t' /dev
            .word       s_htime, s_dev
            .byte       MREPL                               ; bind '#m' /dev/mod
            .word       s_hmod, s_devmod
            .byte       MREPL                               ; bind '#p' /proc
            .word       s_hproc, s_proc
            .byte       $FF
s_hroot:    .byte       "#/", 0
s_hcons:    .byte       "#c", 0
s_hnull:    .byte       "#n", 0
s_htime:    .byte       "#t", 0
s_hmod:     .byte       "#m", 0
s_hproc:    .byte       "#p", 0
s_root:     .byte       "/", 0
s_dev:      .byte       "/dev", 0
s_devmod:   .byte       "/dev/mod", 0
s_proc:     .byte       "/proc", 0
