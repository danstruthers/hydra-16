; ****************************************************************************
; t_exec - EXEC, run as init with t_child: its errors (not there, not a program: the caller goes on); a program
; replaced by another in its task (the same task, its parent's WAIT getting the new one's status and message); the
; fds kept (the new program writes to fd 1); the old program's note handler gone (a note's default ends the new
; one).

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "testlib.inc"

            HYX2_PROGRAM "t_exec", main

.zeropage
child:      .res        1
got:        .res        1                                   ; WAIT's task ...
code:       .res        1                                   ;   and code

.bss
msg:        .res        32

.code

; SPAWN t_child with the arguments at label.  OUT: as SPAWN's
.macro CHILD    label
            LDR         r0, s_child
            LDR         r1, label
            lda         #0
            jsr         SPAWN
.endmacro

; WAIT for child what, its message into msg.  OUT: as WAIT's
.macro WAITMSG  what
            LDR         r0, msg
            lda         what
            jsr         WAIT
.endmacro

main:
            stz         T_FAILS
            ldx         #3                                  ; Fds 0-2: the console (init has none: its
:                                                           ;   children get them)
            phx
            LDR         r0, s_cons
            lda         #O_RDWR
            jsr         OPEN
            plx
            dex
            bne         :-

; ---- Its errors: the caller goes on
            LDR         r0, s_nosuch
            stz         r1
            stz         r1 + 1
            jsr         EXEC
            EXPECT_ERR  E_NOENT, "EXEC of a file that isn't there: E_NOENT, and the caller goes on"
            LDR         r0, s_null
            stz         r1
            stz         r1 + 1
            jsr         EXEC
            EXPECT_ERR  E_NOEXEC, "EXEC of a file that isn't a program: E_NOEXEC"

; ---- A program replaced: the same task; its parent gets the new one's status
            CHILD       s_x
            sta         child
            WAITMSG     child
            sta         got
            stx         code
            EXPECT_OK   "WAIT for a child that EXECs another"
            lda         got
            cmp         child
            beq         :+
            NOTOK       "the status from the child's own task"
            bra         :++
:
            OK          "the status from the child's own task"
:
            lda         code
            EXPECT_A    'Z', "the new program's code (and E_NOENT first, from inside the child)"
            lda         msg
            EXPECT_A    'e', "the new program's message, its arguments"
            lda         msg + 1
            EXPECT_A    'Z', "the message's second byte"

; ---- Its fds kept
            CHILD       s_xx
            sta         child
            WAITMSG     child
            txa
            EXPECT_A    0, "the new program wrote to fd 1, the old one's (W)"

; ---- The old program's note handler gone
            CHILD       s_q
            sta         child
            jsr         nap
            lda         child
            ldx         #NOTE_INTERRUPT
            jsr         NOTE
            EXPECT_OK   "NOTE: interrupt, to the new program"
            WAITMSG     child
            txa
            EXPECT_A    130, "the old program's handler gone: the default ends it, 130"

            DONE        "t_exec"

; A moment for a child to start (and pause)
nap:
            lda         #10
            ldx         #0
            jmp         SLEEP

.rodata
s_cons:     .byte       "#c/cons", 0
s_child:    .byte       "#m/t_child", 0
s_nosuch:   .byte       "#m/nosuch", 0
s_null:     .byte       "#n/null", 0
s_x:        .byte       "x", 0, 0
s_xx:       .byte       "X", 0, 0
s_q:        .byte       "q", 0, 0
