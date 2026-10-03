; ****************************************************************************
; t_cons - the console driver (phase 2.7), run as init with cons (task F) and t_child: its fds 0-2 on #c/cons, and
; the keys sim/test.js types after each prompt ("N> "): a line; editing (Backspace, Left, Home and End, Ctrl-U,
; Delete); the history; Ctrl-D; a line read in parts; raw mode's keys; Ctrl-C to the foreground group (a child
; reading ends, 130); a background group's read, waiting till consctl's fg; and 115200 at the end (the harness
; checks the pacing: the idle bits between the characters sent).

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "testlib.inc"

            HYX2_PROGRAM "t_cons", main

.zeropage
ctl:        .res        1
ser:        .res        1
child:      .res        1
got:        .res        1                                   ; The note this task's handler got

.bss
buf:        .res        64
info:       .res        TI_SIZE
fgcmd:      .res        5                                   ; "fg $n"

.code

; Open path (a label) with mode.  OUT: .A = the fd, C
.macro OPEN_  path, mode
            LDR         r0, path
            lda         #mode
            jsr         OPEN
.endmacro

; Read count bytes from fd into buf.  OUT: .A/.X, C
.macro READ_  fdv, count
            LDR         r0, buf
            LDR         r1, count
            lda         fdv
            jsr         READ
.endmacro

; Write len bytes at label to fd.  OUT: .A/.X, C
.macro WRITE_ fdv, label, len
            LDR         r0, label
            LDR         r1, len
            lda         fdv
            jsr         WRITE
.endmacro

; SPAWN "t_child i" (it reads a byte from its fd 0) with flags
.macro CHILD_ flags
            LDR         r0, s_child
            LDR         r1, s_i
            lda         #flags
            jsr         SPAWN
            sta         child
.endmacro

main:
            stz         T_FAILS

; ---- Fds 0-2: #c/cons
            OPEN_       s_cons, O_RDWR
            EXPECT_A    0, "OPEN #c/cons: fd 0"
            lda         #0
            jsr         DUP
            EXPECT_A    1, "fd 1 (what's printed from here on goes through the console)"
            lda         #0
            jsr         DUP
            EXPECT_A    2, "fd 2"
            OPEN_       s_consctl, O_RDWR
            sta         ctl
            EXPECT_OK   "OPEN #c/consctl"
            READ_       ctl, 64
            EXPECT_A    12, "consctl reads as its state: rawoff, fg 1 (12 bytes)"
            lda         buf + 10
            EXPECT_A    '1', "fg 1: init's group"

; ---- A line
            PRINT       s_p1
            READ_       #0, 64
            EXPECT_A    6, "a line: hello and its LF (6 bytes)"
            lda         buf + 5
            EXPECT_A    LF, "it ends with an LF (Enter: a CR)"

; ---- Editing
            PRINT       s_p2
            READ_       #0, 64
            jsr         is_abc
            EXPECT_A    0, "Backspace: abX, BS, c is abc"
            PRINT       s_p3
            READ_       #0, 64
            jsr         is_abc
            EXPECT_A    0, "Left: ac, Left, b is abc"
            PRINT       s_p4
            READ_       #0, 64
            EXPECT_A    5, "Home and End: bc, Home, a, End, d is abcd (5 bytes)"
            lda         buf
            EXPECT_A    'a', "abcd: a first"
            lda         buf + 3
            EXPECT_A    'd', "abcd: d last"
            PRINT       s_p5
            READ_       #0, 64
            EXPECT_A    3, "Ctrl-U: xyz, Ctrl-U, ok is ok (3 bytes)"
            PRINT       s_p6
            READ_       #0, 64
            jsr         is_abc
            EXPECT_A    0, "Delete: abXc, Left, Left, Delete is abc"

; ---- The history: Up twice, the line before the last
            PRINT       s_p7
            READ_       #0, 64
            EXPECT_A    3, "Up, Up: the line before the last (ok)"
            lda         buf
            EXPECT_A    'o', "ok"

; ---- Ctrl-D on an empty line: the end of the input
            PRINT       s_p8
            READ_       #0, 64
            EXPECT_A    0, "Ctrl-D: a read of 0"

; ---- A line read in parts
            PRINT       s_p9
            READ_       #0, 2
            EXPECT_A    2, "parts, read 2 at a time: pa"
            READ_       #0, 2
            lda         buf
            EXPECT_A    'r', "then rt"
            READ_       #0, 64
            EXPECT_A    2, "then the rest: s and its LF"

; ---- Raw: each key as it comes, Up as KEY_UP
            WRITE_      ctl, s_rawon, 5
            EXPECT_OK   "consctl: rawon"
            PRINT       s_pr
            READ_       #0, 1
            lda         buf
            EXPECT_A    'x', "raw: x as it comes"
            READ_       #0, 1
            lda         buf
            EXPECT_A    KEY_UP, "raw: ESC [ A is KEY_UP"
            WRITE_      ctl, s_rawoff, 6
            EXPECT_OK   "consctl: rawoff"

; ---- Ctrl-C: the foreground group's note.  A child reading (in this task's group) ends, 130; this task's
; handler keeps it
            stz         got
            LDR         r0, keep
            jsr         NOTIFY
            CHILD_      0
            jsr         nap
            PRINT       s_pc
@wait:
            stz         r0
            stz         r0 + 1
            lda         child
            jsr         WAIT
            bcc         :+
            cmp         #E_INTR                             ; (The note came while this task waited)
            beq         @wait
:
            txa
            EXPECT_A    130, "Ctrl-C: the child reading ends (130)"
            lda         got
            EXPECT_A    NOTE_INTERRUPT, "and this task, in its group, gets the note too"
            stz         r0
            stz         r0 + 1
            jsr         NOTIFY

; ---- A background group's read waits for consctl's fg
            CHILD_      SPAWN_NEWGROUP
            jsr         nap
            LDR         r0, info
            lda         child
            jsr         TASKINFO
            lda         info + TI_STATE
            EXPECT_A    8, "a background group's read waits (its state: event)"
            ldx         #3                                  ; "fg $n", n: the child
:
            lda         s_fgcmd,X
            sta         fgcmd,X
            dex
            bpl         :-
            lda         child
            ora         #'0'
            cmp         #'9' + 1
            bcc         :+
            adc         #'a' - '9' - 2                      ; (C = 1)
:
            sta         fgcmd + 4
            WRITE_      ctl, fgcmd, 5
            EXPECT_OK   "consctl: fg, the child's group"
            PRINT       s_pg
            stz         r0
            stz         r0 + 1
            lda         child
            jsr         WAIT
            txa
            EXPECT_A    'z', "its read goes on: z"
            WRITE_      ctl, s_fg1, 4
            EXPECT_OK   "consctl: fg 1"
            READ_       #0, 64
            EXPECT_A    1, "and the rest of its line, the LF, is this task's"

; ---- 115200
            OPEN_       s_serctl, O_RDWR
            sta         ser
            EXPECT_OK   "OPEN #c/serctl"
            READ_       ser, 64
            EXPECT_A    6, "serctl reads as the rate: b9600 (6 bytes)"
            WRITE_      ser, s_b115200, 7
            EXPECT_OK   "serctl: b115200"
            SAY         "at 115200: 0123456789 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789"
            DONE        "t_cons"

; .A = 0 if the read's .A bytes in buf are abc and an LF
is_abc:
            cmp         #4
            bne         @no
            ldx         #3
:
            lda         buf,X
            cmp         s_abc,X
            bne         @no
            dex
            bpl         :-
            lda         #0
            rts

@no:
            lda         #1
            rts

; The note handler: keeps the note, and goes on
keep:
            sta         got
            clc
            rts

; A moment for a child to start (and wait)
nap:
            lda         #3
            ldx         #0
            jmp         SLEEP

.rodata
s_cons:     .byte       "#c/cons", 0
s_consctl:  .byte       "#c/consctl", 0
s_serctl:   .byte       "#c/serctl", 0
s_child:    .byte       "#m/t_child", 0
s_i:        .byte       "i", 0
s_rawon:    .byte       "rawon"
s_rawoff:   .byte       "rawoff"
s_fg1:      .byte       "fg 1"
s_fgcmd:    .byte       "fg $"
s_b115200:  .byte       "b115200"
s_abc:      .byte       "abc", LF
s_p1:       .byte       "1> ", 0
s_p2:       .byte       "2> ", 0
s_p3:       .byte       "3> ", 0
s_p4:       .byte       "4> ", 0
s_p5:       .byte       "5> ", 0
s_p6:       .byte       "6> ", 0
s_p7:       .byte       "7> ", 0
s_p8:       .byte       "8> ", 0
s_p9:       .byte       "9> ", 0
s_pr:       .byte       "r> ", 0
s_pc:       .byte       "c> ", 0
s_pg:       .byte       "g> ", 0
