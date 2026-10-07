; ****************************************************************************
; t_cons - the console driver (phase 2.7), run as init with cons (task F) and t_child: its fds 0-2 on #c/cons, and
; the keys sim/test.js types after each prompt ("N> "): a line; editing (Backspace, Left, Home and End, Ctrl-U,
; Delete); the history; Ctrl-D; a line read in parts; raw mode's keys; Ctrl-C to the shown window's note group (a
; child reading ends, 130); windows: one made (wctl's new), written to while it isn't shown, its reader waiting till
; Ctrl-] 1 shows it (the harness sees it repainted), Ctrl-C to its group, gone with its last cons, one made by Ctrl-]
; c (wnew); and 115200 at the end (the harness checks the pacing: the idle bits between the characters sent).

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
fd:         .res        1

.bss
buf:        .res        96
info:       .res        TI_SIZE
w1:         .res        1                                   ; Window 1's cons
saved:      .res        1                                   ; Fd 0, kept
wctl:       .res        1
exp:        .res        2                                   ; (answer_is's: the answer expected ...
got_n:      .res        1                                   ;   the bytes read ...
exp_n:      .res        1                                   ;   and those asked for)

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

; SPAWN "t_child j" (it claims window 1's notes, then reads a byte from its fd 0: window 1's cons), a group of its
; own
.macro CHILD_W1
            lda         #0
            jsr         DUP
            sta         saved
            lda         w1
            ldx         #0
            jsr         DUP2
            LDR         r0, s_child
            LDR         r1, s_j
            lda         #SPAWN_NEWGROUP
            jsr         SPAWN
            sta         child
            lda         saved
            ldx         #0
            jsr         DUP2
            lda         saved
            jsr         CLOSE
.endmacro

; WAIT for the child.  OUT: .A = its exit code
.macro WAITCHILD
            stz         r0
            stz         r0 + 1
            lda         child
            jsr         WAIT
            txa
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
            READ_       ctl, 96
            EXPECT_A    76, "consctl reads as its state: rawoff, keys hydra, scroll smooth, group 1, window 0, size 80 24, terminal serial (76 bytes)"
            lda         buf + 38
            EXPECT_A    '1', "group 1: init's"
            lda         buf + 57
            EXPECT_A    '2', "size 80 24: the serial port's terminal's"

; ---- A BEL printed: the sound driver's bell too (#a/bell: tests.js looks for channel 7's key-on)
            lda         #$07
            jsr         PUTC
            EXPECT_OK   "a BEL printed (the bell)"

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

; ---- Raw: the console's answers to what its window was sent, as a terminal's would come (vt.s: W2)
            WRITE_      #1, s_q_da, S_Q_DA_N
            ldx         #<s_a_da
            ldy         #>s_a_da
            jsr         answer_is
            EXPECT_A    0, "raw: ESC [ c answered ESC [ ? 6 c (DA: a VT102)"
            WRITE_      #1, s_q_da2, S_Q_DA2_N
            ldx         #<s_a_da2
            ldy         #>s_a_da2
            jsr         answer_is
            EXPECT_A    0, "raw: ESC [ > c answered ESC [ > 1 ; 10 ; 0 c (the secondary DA)"
            WRITE_      #1, s_q_rqm, S_Q_RQM_N
            ldx         #<s_a_rqm
            ldy         #>s_a_rqm
            jsr         answer_is
            EXPECT_A    0, "raw: ESC [ ? 7 $ p answered ESC [ ? 7 ; 1 $ y (DECRQM: autowrap set)"
            WRITE_      #1, s_q_rqm4, S_Q_RQM4_N
            ldx         #<s_a_rqm4
            ldy         #>s_a_rqm4
            jsr         answer_is
            EXPECT_A    0, "raw: ESC [ 4 $ p answered ESC [ 4 ; 2 $ y (DECRQM: insert mode reset)"
            WRITE_      #1, s_q_size, S_Q_SIZE_N
            ldx         #<s_a_size
            ldy         #>s_a_size
            jsr         answer_is
            EXPECT_A    0, "raw: ESC [ 1 8 t answered ESC [ 8 ; 24 ; 80 t (xterm's size: the window's)"
            WRITE_      #1, s_q_parm, S_Q_PARM_N
            ldx         #<s_a_parm
            ldy         #>s_a_parm
            jsr         answer_is
            EXPECT_A    0, "raw: ESC [ x answered ESC [ 2 ; 1 ; 1 ; 120 ; 120 ; 1 ; 0 x (DECREPTPARM)"
            WRITE_      #1, s_q_cpr, S_Q_CPR_N
            READ_       #0, 32
            tax
            lda         buf - 1,X
            EXPECT_A    'R', "raw: ESC [ 6 n answered ESC [ row ; column R (CPR)"

; ---- The window's size (W3): the serial port's terminal's here (the screen's off).  Its report (ESC [ 8 ; R ; C t,
; typed: the PC tool's) sets it, as terminal size C R does; a raw reader is told (KEY_RESIZE)
            PRINT       s_pz
            READ_       #0, 1
            lda         buf
            EXPECT_A    KEY_RESIZE, "raw: the terminal's size typed (ESC [ 8 ; 40 ; 100 t): KEY_RESIZE"
            WRITE_      #1, s_q_size, S_Q_SIZE_N
            ldx         #<s_a_size100
            ldy         #>s_a_size100
            jsr         answer_is
            EXPECT_A    0, "raw: ESC [ 1 8 t answered ESC [ 8 ; 40 ; 100 t (the window's size: the terminal's)"
            jsr         ctl_read
            lda         buf + 53
            EXPECT_A    '1', "consctl: size 100 40"
            WRITE_      ctl, s_tsize, S_TSIZE_N
            EXPECT_OK   "consctl: terminal size 80 24"
            READ_       #0, 1
            lda         buf
            EXPECT_A    KEY_RESIZE, "raw: KEY_RESIZE again"
            WRITE_      #1, s_q_size, S_Q_SIZE_N
            ldx         #<s_a_size
            ldy         #>s_a_size
            jsr         answer_is
            EXPECT_A    0, "raw: ESC [ 1 8 t answered ESC [ 8 ; 24 ; 80 t again"
            WRITE_      ctl, s_tsmall, S_TSMALL_N
            EXPECT_ERR  E_INVAL, "consctl: terminal size 5 5 (too small): E_INVAL"
            WRITE_      ctl, s_task, S_TASK_N
            EXPECT_OK   "consctl: terminal size (the terminal asked: ESC [ 1 8 t)"

; ---- Raw, keys vt: the keys as a VT100 sends them, following the window's DECCKM and VT52 mode (vt.s: W2)
            WRITE_      ctl, s_keysvt, 7
            EXPECT_OK   "consctl: keys vt"
            PRINT       s_pv
            ldx         #<s_k_up
            ldy         #>s_k_up
            jsr         answer_is
            EXPECT_A    0, "keys vt: Up is ESC [ A"
            WRITE_      #1, s_ckm, 5
            ldx         #<s_k_upo
            ldy         #>s_k_upo
            jsr         answer_is
            EXPECT_A    0, "keys vt, DECCKM set: Up is ESC O A"
            ldx         #<s_k_f5
            ldy         #>s_k_f5
            jsr         answer_is
            EXPECT_A    0, "keys vt: F5 is ESC [ 1 5 ~"
            WRITE_      #1, s_vt52, 10
            ldx         #<s_k_up52
            ldy         #>s_k_up52
            jsr         answer_is
            EXPECT_A    0, "keys vt, VT52 mode: Up is ESC A"
            WRITE_      #1, s_ansi, 2
            WRITE_      ctl, s_keyshy, 10
            EXPECT_OK   "consctl: keys hydra"
            WRITE_      ctl, s_rawoff, 6
            EXPECT_OK   "consctl: rawoff"

; ---- Raw lasts while consctl is open (Plan 9's): rawon, then consctl closed
            WRITE_      ctl, s_rawon, 5
            lda         ctl
            jsr         CLOSE
            OPEN_       s_consctl, O_RDWR
            sta         ctl
            READ_       ctl, 64
            lda         buf + 4
            EXPECT_A    'f', "rawon, consctl closed: raw ends with it (rawoff)"

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

; ---- Windows: one made; written to unseen; its reader waits till it's shown
            OPEN_       s_wctl, O_RDWR
            sta         wctl
            EXPECT_OK   "OPEN #c/wctl"
            WRITE_      wctl, s_new, 3
            EXPECT_OK   "wctl: new"
            jsr         wctl_read
            EXPECT_A    2, "wctl, read after its new: the window it made ..."
            lda         buf
            EXPECT_A    '1', "... window 1"
            jsr         wctl_read
            EXPECT_A    22, "then wctl reads as the windows: 0 0 80 24 * and 1 0 80 24 (number, group, size; * shown)"
            lda         buf + 14
            EXPECT_A    '0', "window 1 in window 0's group (0)"
            OPEN_       s_c1, O_RDWR
            sta         w1
            EXPECT_OK   "OPEN #c1/cons, window 1's"
            WRITE_      w1, s_hidden, 15
            EXPECT_A    15, "a write to window 1, not shown: all of it, into its text"
            OPEN_       s_c2, O_RDWR
            EXPECT_ERR  E_NOENT, "OPEN #c2/cons, no such window: E_NOENT"
            CHILD_W1
            jsr         nap
            LDR         r0, info
            lda         child
            jsr         TASKINFO
            lda         info + TI_STATE
            EXPECT_A    8, "a child reading window 1 waits (its state: event)"
            PRINT       s_pw                                ; (The harness: Ctrl-] 1, z, Enter, Ctrl-] 0)
            WAITCHILD
            EXPECT_A    'z', "Ctrl-] 1: window 1 shown, its reader gets the keys, z"
            READ_       w1, 16
            EXPECT_A    1, "the rest of its line, the LF, is window 1's next reader's"

; ---- Ctrl-C: the shown window's group's (window 1's child's, not this task's)
            CHILD_W1
            jsr         nap
            PRINT       s_pk                                ; (The harness: Ctrl-] 1, Ctrl-C, Ctrl-] 0)
            WAITCHILD
            EXPECT_A    130, "Ctrl-C with window 1 shown: its group's note (130), not window 0's"

; ---- A window goes with its last cons; Ctrl-] c makes one (for wnew's reader)
            lda         w1
            jsr         CLOSE
            jsr         wctl_read
            EXPECT_A    12, "window 1's last cons closed: it's gone (wctl: 0 alone)"
            OPEN_       s_wnew, O_READ
            sta         fd
            PRINT       s_pn                                ; (The harness: Ctrl-] c)
            READ_       fd, 16
            EXPECT_A    2, "Ctrl-] c: wnew's read gives a new window ..."
            lda         buf
            pha
            lda         fd
            jsr         CLOSE
            WRITE_      wctl, s_cur0, 9                     ; (Shown: window 0 again, by wctl)
            sta         fd
            pla
            EXPECT_A    '1', "... window 1, shown"
            lda         fd
            EXPECT_A    9, "wctl: current 0 (this task's output shown again)"

; ---- Groups (W5): Ctrl-] c's window (1) has a group of its own; wctl's new joins the writer's window's group (0's),
; new group makes one.  The keys (the harness's): Ctrl-] Tab and Ctrl-Tab the group's next window, Ctrl-Shift-Tab
; its previous, Ctrl-] n and p the next and previous group, each the window shown checked in wctl (then 0 again); a
; raw reader of the group told as its focus moves (KEY_FOCUS, then the window)
            WRITE_      wctl, s_new, 3
            jsr         wctl_read
            lda         buf
            EXPECT_A    '2', "wctl: new, in window 0's group: window 2"
            WRITE_      wctl, s_newg, 9
            EXPECT_OK   "wctl: new group"
            jsr         wctl_read
            lda         buf
            EXPECT_A    '3', "wctl: new group: window 3"
            jsr         wctl_read
            ldx         #<s_wins
            ldy         #>s_wins
            jsr         buf_is
            EXPECT_A    0, "wctl: 0 0 80 24 *, 1 1 80 24, 2 0 80 24, 3 2 80 24 (Ctrl-] c's and new group's groups their own)"
            WRITE_      ctl, s_rawon, 5
            PRINT       s_pg1                               ; (The harness: Ctrl-] Tab)
            jsr         wait_shown
            EXPECT_A    2, "Ctrl-] Tab: the group's next window, 2"
            jsr         cur0
            READ_       #0, 2
            lda         buf
            EXPECT_A    KEY_FOCUS, "raw: the group's focus moved: KEY_FOCUS ..."
            lda         buf + 1
            EXPECT_A    0, "... and the window, 0 (current 0's, the last)"
            WRITE_      ctl, s_rawoff, 6
            PRINT       s_pg2                               ; (Ctrl-Tab: CSI u's ESC [ 9 ; 5 u)
            jsr         wait_shown
            EXPECT_A    2, "Ctrl-Tab (ESC [ 9 ; 5 u): the group's next window, 2"
            jsr         cur0
            PRINT       s_pg3                               ; (Ctrl-] n)
            jsr         wait_shown
            EXPECT_A    1, "Ctrl-] n: the next group's window, 1"
            jsr         cur0
            PRINT       s_pg4                               ; (Ctrl-Shift-Tab: xterm's ESC [ 27 ; 6 ; 9 ~)
            jsr         wait_shown
            EXPECT_A    2, "Ctrl-Shift-Tab (ESC [ 27 ; 6 ; 9 ~): the group's previous window, 2"
            jsr         cur0
            PRINT       s_pg5                               ; (Ctrl-] p)
            jsr         wait_shown
            EXPECT_A    3, "Ctrl-] p: the previous group's window, 3"
            jsr         cur0
            lda         wctl
            jsr         CLOSE

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
; The answer to what was just written to the console (or the keys typed), read raw, as many bytes as the zero-ended
; string at .X/.Y has: .A = 0 if they're it
answer_is:
            stx         exp
            sty         exp + 1
            stx         r4
            sty         r4 + 1
            ldy         #0
:
            lda         (r4),Y
            beq         :+
            iny
            bra         :-
:
            sty         exp_n
            LDR         r0, buf
            lda         exp_n
            sta         r1
            stz         r1 + 1
            lda         #0
            jsr         READ
            sta         got_n
            lda         exp
            sta         r4
            lda         exp + 1
            sta         r4 + 1
            ldy         #0
:
            lda         (r4),Y
            beq         @end
            cmp         buf,Y
            bne         @no
            iny
            bra         :-
@end:
            cpy         got_n
            bne         @no
            lda         #0
            rts
@no:
            lda         #1
            rts

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

; consctl read from its start (a write moved its offset), into buf.  OUT: .A = the count read
ctl_read:
            stz         r0
            stz         r0 + 1
            stz         r1
            stz         r1 + 1
            lda         ctl
            ldx         #0
            jsr         SEEK
            LDR         r0, buf
            LDR         r1, 96
            lda         ctl
            jmp         READ

; .A = the window shown (wctl's line with the *), once it isn't 0 (40 naps at most)
wait_shown:
            lda         #40
            sta         exp_n
@try:
            jsr         nap
            jsr         wctl_read
            sta         got_n
            ldy         #0                                  ; (The *)
:
            cpy         got_n
            bcs         @not
            lda         buf,Y
            cmp         #'*'
            beq         :+
            iny
            bra         :-
:
            dey                                             ; (Its line's start: its number)
            bmi         :+
            lda         buf,Y
            cmp         #LF
            bne         :-
:
            iny
            lda         buf,Y
            and         #$0F
            bne         @done
@not:
            dec         exp_n
            bne         @try
            lda         #0
@done:
            rts

; Window 0 shown again (wctl's current 0)
cur0:
            WRITE_      wctl, s_cur0, 9
            rts

; .A = 0 if wctl's read (got_n bytes in buf) is the string at .X/.Y
buf_is:
            sta         got_n
            stx         r4
            sty         r4 + 1
            ldy         #0
:
            lda         (r4),Y
            beq         :+
            cmp         buf,Y
            bne         @no
            iny
            bra         :-
:
            cpy         got_n
            bne         @no
            lda         #0
            rts
@no:
            lda         #1
            rts

; wctl read from its start (the write moved its offset), into buf.  OUT: .A = the count read
wctl_read:
            stz         r0
            stz         r0 + 1
            stz         r1
            stz         r1 + 1
            lda         wctl
            ldx         #0
            jsr         SEEK
            LDR         r0, buf
            LDR         r1, 64
            lda         wctl
            jmp         READ

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
s_i:        .byte       "i", 0, 0
s_rawon:    .byte       "rawon"
ESC         = $1B
s_q_da:     .byte       ESC, "[c"                          ; The queries, and their answers
S_Q_DA_N    = * - s_q_da
s_a_da:     .byte       ESC, "[?6c", 0
s_q_da2:    .byte       ESC, "[>c"
S_Q_DA2_N   = * - s_q_da2
s_a_da2:    .byte       ESC, "[>1;10;0c", 0
s_q_rqm:    .byte       ESC, "[?7$p"
S_Q_RQM_N   = * - s_q_rqm
s_a_rqm:    .byte       ESC, "[?7;1$y", 0
s_q_rqm4:   .byte       ESC, "[4$p"
S_Q_RQM4_N  = * - s_q_rqm4
s_a_rqm4:   .byte       ESC, "[4;2$y", 0
s_q_size:   .byte       ESC, "[18t"
S_Q_SIZE_N  = * - s_q_size
s_a_size:   .byte       ESC, "[8;24;80t", 0
s_a_size100: .byte      ESC, "[8;40;100t", 0
s_tsize:    .byte       "terminal size 80 24"
S_TSIZE_N   = * - s_tsize
s_tsmall:   .byte       "terminal size 5 5"
S_TSMALL_N  = * - s_tsmall
s_task:     .byte       "terminal size"
S_TASK_N    = * - s_task
s_q_parm:   .byte       ESC, "[x"
S_Q_PARM_N  = * - s_q_parm
s_a_parm:   .byte       ESC, "[2;1;1;120;120;1;0x", 0
s_q_cpr:    .byte       ESC, "[6n"
S_Q_CPR_N   = * - s_q_cpr
s_keysvt:   .byte       "keys vt"                           ; keys vt: the modes, and the keys as they come
s_keyshy:   .byte       "keys hydra"
s_ckm:      .byte       ESC, "[?1h"
s_vt52:     .byte       ESC, "[?1l", ESC, "[?2l"
s_ansi:     .byte       ESC, "<"
s_k_up:     .byte       ESC, "[A", 0
s_k_upo:    .byte       ESC, "OA", 0
s_k_f5:     .byte       ESC, "[15~", 0
s_k_up52:   .byte       ESC, "A", 0
s_pv:       .byte       "v> ", 0
s_rawoff:   .byte       "rawoff"
s_wctl:     .byte       "#c/wctl", 0
s_wnew:     .byte       "#c/wnew", 0
s_c1:       .byte       "#c1/cons", 0
s_c2:       .byte       "#c2/cons", 0
s_new:      .byte       "new"
s_cur0:     .byte       "current 0"
s_hidden:   .byte       "w1 hidden text", LF
s_j:        .byte       "j", 0, 0
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
s_pg1:      .byte       "g1> ", 0
s_pg2:      .byte       "g2> ", 0
s_pg3:      .byte       "g3> ", 0
s_pg4:      .byte       "g4> ", 0
s_pg5:      .byte       "g5> ", 0
s_newg:     .byte       "new group"
s_wins:     .byte       "0 0 80 24 *", LF, "1 1 80 24", LF, "2 0 80 24", LF, "3 2 80 24", LF, 0
s_pz:       .byte       "z> ", 0
s_pc:       .byte       "c> ", 0
s_pw:       .byte       "w> ", 0
s_pk:       .byte       "k> ", 0
s_pn:       .byte       "n> ", 0
