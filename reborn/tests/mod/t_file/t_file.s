; ****************************************************************************
; t_file - files and servers (phase 2.2-2.4), run as init with t_srv (#T) and t_child: stdout through a server
; (fds 0-2 on #T/out, so what it prints is a test of WRITE too); OPEN and its errors,
; READ, WRITE, SEEK, FSTAT and STAT, CLOSE; a text file at offsets, a ctl file, a data file, a directory read as
; stat records; DUP and DUP2 sharing an offset; a read that waits for the server (and a note ending it); fds
; 0-2 inherited by a child.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "testlib.inc"

            HYX2_PROGRAM "t_file", main

.zeropage
fd:         .res        1
fd2:        .res        1
ctl:        .res        1
child:      .res        1
n:          .res        2

.bss
buf:        .res        512
stat:       .res        SR_SIZE
info:       .res        TI_SIZE

.code

; Open path (a label) with mode; fail the test's step if it fails.  OUT: .A = the fd, C
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

; Write the string at label (len bytes) to fd.  OUT: .A/.X, C
.macro WRITE_ fdv, label, len
            LDR         r0, label
            LDR         r1, len
            lda         fdv
            jsr         WRITE
.endmacro

; SPAWN t_child with the arguments at label.  OUT: as SPAWN's
.macro CHILD_ label
            LDR         r0, s_child
            LDR         r1, label
            lda         #0
            jsr         SPAWN
.endmacro

; WAIT for child what, no message wanted.  OUT: as WAIT's
.macro WAITFOR what
            stz         r0
            stz         r0 + 1
            lda         what
            jsr         WAIT
.endmacro

; Seek fd to offset (from the start)
.macro SEEK_  fdv, offset
            LDR         r0, offset
            stz         r1
            stz         r1 + 1
            lda         fdv
            ldx         #0
            jsr         SEEK
.endmacro

main:
            stz         T_FAILS

; ---- Fds 0-2: #T/out, as init's are the console (what's printed from here on goes through it)
            OPEN_       s_out, O_RDWR
            EXPECT_A    0, "OPEN #T/out: fd 0"
            lda         #0
            jsr         DUP
            EXPECT_A    1, "DUP: fd 1 (stdout from here on)"
            lda         #0
            jsr         DUP
            EXPECT_A    2, "DUP: fd 2"

; ---- A text file
            OPEN_       s_hello, O_READ
            sta         fd
            EXPECT_OK   "OPEN #T/hello"
            READ_       fd, 64
            sta         n
            EXPECT_A    13, "READ: hello, world and a new line, 13 bytes"
            lda         buf + 7
            EXPECT_A    'w', "and what they are"
            READ_       fd, 64
            EXPECT_A    0, "READ again: the end of the file (0)"
            SEEK_       fd, 0
            READ_       fd, 5
            EXPECT_A    5, "SEEK to 0, READ 5: 5"
            lda         buf + 4
            EXPECT_A    'o', "hello"
            LDR         r0, 0
            stz         r1
            stz         r1 + 1
            lda         fd
            ldx         #2
            jsr         SEEK
            lda         r0
            EXPECT_A    13, "SEEK to the end: 13"
            LDR         r0, stat
            lda         fd
            jsr         FSTAT
            EXPECT_OK   "FSTAT"
            lda         stat + SR_NAME + 1
            EXPECT_A    'e', "FSTAT: its name"
            lda         stat + SR_LENGTH
            EXPECT_A    13, "FSTAT: its length"
            lda         stat + SR_DEV
            EXPECT_A    'T', "FSTAT: its device"
            lda         fd
            jsr         CLOSE
            EXPECT_OK   "CLOSE"
            lda         fd
            jsr         CLOSE
            EXPECT_ERR  E_BADF, "CLOSE again: E_BADF"
            READ_       fd, 1
            EXPECT_ERR  E_BADF, "READ a closed fd: E_BADF"
            LDR         r0, s_inner
            LDR         r1, stat
            jsr         STAT
            EXPECT_OK   "STAT #T//sub/inner (two slashes: one)"
            lda         stat + SR_LENGTH
            EXPECT_A    6, "STAT: its length"

; ---- OPEN's errors
            OPEN_       s_nope, O_READ
            EXPECT_ERR  E_NOENT, "OPEN #T/nope: E_NOENT"
            OPEN_       s_nodev, O_READ
            EXPECT_ERR  E_NODEV, "OPEN #Z/x (no server): E_NODEV"
            OPEN_       s_nohash, O_READ
            EXPECT_ERR  E_NOENT, "OPEN T/x (no namespace yet): E_NOENT"
            OPEN_       s_notdir, O_READ
            EXPECT_ERR  E_NOTDIR, "OPEN #T/hello/x: E_NOTDIR"
            OPEN_       s_ro, O_WRITE
            EXPECT_ERR  E_PERM, "OPEN #T/ro for writing: E_PERM"
            OPEN_       s_root, O_WRITE
            EXPECT_ERR  E_ISDIR, "OPEN #T for writing: E_ISDIR"

; ---- A ctl file
            OPEN_       s_ctl, O_RDWR
            sta         ctl
            EXPECT_OK   "OPEN #T/ctl"
            WRITE_      ctl, s_add5, 5
            EXPECT_A    5, "WRITE add 5: 5 bytes"
            WRITE_      ctl, s_add16, 7
            EXPECT_OK   "WRITE add $10"
            SEEK_       ctl, 0
            READ_       ctl, 16
            EXPECT_A    3, "the ctl file reads as count: 21 and a new line"
            lda         buf
            EXPECT_A    '2', "21"
            lda         buf + 1
            EXPECT_A    '1', "21 (2)"
            WRITE_      ctl, s_bogus, 5
            EXPECT_ERR  E_INVAL, "WRITE bogus: E_INVAL"
            WRITE_      ctl, s_fail, 4
            EXPECT_ERR  E_BUSY, "WRITE fail: its handler's error, E_BUSY"

; ---- A data file
            OPEN_       s_data, O_RDWR
            sta         fd
            WRITE_      fd, s_digits, 10
            EXPECT_A    10, "WRITE 10 bytes to #T/data"
            SEEK_       fd, 3
            READ_       fd, 4
            EXPECT_A    4, "SEEK 3, READ 4"
            lda         buf
            EXPECT_A    '3', "3456"
            SEEK_       fd, 62
            WRITE_      fd, s_digits, 5
            EXPECT_A    2, "WRITE 5 at 62 of 64: 2 (a short count)"
            READ_       fd, 1
            EXPECT_A    0, "READ at 64: the end"

; ---- A directory: stat records
            OPEN_       s_root, O_READ
            sta         fd2
            EXPECT_OK   "OPEN #T (its root directory)"
            READ_       fd2, 512
            sta         n
            stx         n + 1
            txa
            EXPECT_A    2, "READ: 8 records (512 bytes: high byte)"
            lda         n
            EXPECT_A    0, "READ: 8 records (512 bytes)"
            lda         buf + SR_NAME
            EXPECT_A    'h', "the first: hello"
            lda         buf + 4 * SR_SIZE + SR_QTYPE
            EXPECT_A    QT_DIR, "the fifth: sub, a directory"
            READ_       fd2, 512
            EXPECT_A    0, "READ again: no more"
            lda         fd2
            jsr         CLOSE

; ---- DUP and DUP2: one offset
            OPEN_       s_hello, O_READ
            sta         fd
            jsr         DUP
            sta         fd2
            EXPECT_OK   "DUP"
            READ_       fd, 5
            READ_       fd2, 2
            lda         buf
            EXPECT_A    ',', "the dup reads on from the same offset"
            lda         fd
            ldx         #9
            jsr         DUP2
            EXPECT_OK   "DUP2 to fd 9"
            READ_       #9, 2
            lda         buf
            EXPECT_A    'w', "fd 9 reads on from the same offset too"
            lda         fd
            jsr         CLOSE
            lda         fd2
            jsr         CLOSE
            READ_       #9, 1
            EXPECT_A    1, "and has it still after the others close"
            lda         #9
            jsr         CLOSE

; ---- A read that waits: a child reads #T/wait; the kick wakes it
            WRITE_      ctl, s_reset, 5
            CHILD_      s_r
            sta         child
            jsr         nap
            LDR         r0, info
            lda         child
            jsr         TASKINFO
            lda         info + TI_STATE
            EXPECT_A    8, "a child's read of #T/wait waits (its state: event)"
            WRITE_      ctl, s_kick, 4
            WAITFOR     child
            txa
            EXPECT_A    'o', "the kick: its read came back, ok"

            WRITE_      ctl, s_reset, 5                     ; And a note ends a wait
            CHILD_      s_r
            sta         child
            jsr         nap
            lda         child
            ldx         #NOTE_INTERRUPT
            jsr         NOTE
            WAITFOR     child
            txa
            EXPECT_A    130, "a note ends a waiting read (E_INTR, then the note's default)"

; ---- Fds 0-2 inherited: a child writes to its fd 1, this one's #T/data for a moment
            OPEN_       s_data, O_RDWR
            sta         fd
            ldx         #1
            jsr         DUP2
            CHILD_      s_w
            sta         child
            WAITFOR     child
            phx                                             ; (Its exit code)
            lda         #0                                  ; Fd 1 #T/out again, before anything's printed
            ldx         #1
            jsr         DUP2
            pla
            EXPECT_A    0, "a child writes to its fd 1"
            SEEK_       fd, 0
            READ_       fd, 1
            lda         buf
            EXPECT_A    'W', "into this task's fd 1: #T/data"

            DONE        "t_file"

; A moment for a child to start (and wait)
nap:
            lda         #3
            ldx         #0
            jmp         SLEEP

.rodata
s_hello:    .byte       "#T/hello", 0
s_inner:    .byte       "#T//sub/inner", 0
s_nope:     .byte       "#T/nope", 0
s_nodev:    .byte       "#Z/x", 0
s_nohash:   .byte       "T/x", 0
s_notdir:   .byte       "#T/hello/x", 0
s_ro:       .byte       "#T/ro", 0
s_root:     .byte       "#T", 0
s_ctl:      .byte       "#T/ctl", 0
s_data:     .byte       "#T/data", 0
s_out:      .byte       "#T/out", 0
s_add5:     .byte       "add 5"
s_add16:    .byte       "add $10"
s_bogus:    .byte       "bogus"
s_fail:     .byte       "fail"
s_kick:     .byte       "kick"
s_reset:    .byte       "reset"
s_digits:   .byte       "0123456789"
s_child:    .byte       "#m/t_child", 0
s_r:        .byte       "r", 0, 0
s_w:        .byte       "w", 0, 0
