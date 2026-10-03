; ****************************************************************************
; t_dev - the kernel's devices (phase 2.6: kdev) and PIPE, run as init with t_child, its fds 0-2 on #c/cons: #/ (the
; mount points), #n (null, zero), #t (ticks), #m (the modules), #p (a task's status; ctl's kill); a pipe (its two
; ends, the end of it, a broken one), a child writing into one and a child waiting on one; and a union's first bind
; keeping what was there (bind -a #n /dev: #/'s dev first).

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "testlib.inc"

            HYX2_PROGRAM "t_dev", main

.zeropage
fd:         .res        1
rfd:        .res        1
wfd:        .res        1
saved:      .res        1
child:      .res        1
total:      .res        2

.bss
buf:        .res        512
info:       .res        TI_SIZE
ctlname:    .res        16                                  ; "#p/N/ctl"

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

; SPAWN t_child with the arguments at label
.macro CHILD_ label
            LDR         r0, s_child
            LDR         r1, label
            lda         #0
            jsr         SPAWN
            sta         child
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
            OPEN_       s_cons, O_RDWR
            lda         #0
            jsr         DUP
            lda         #0
            jsr         DUP

; ---- #/: the root's mount points
            OPEN_       s_hroot, O_READ
            sta         fd
            EXPECT_OK   "OPEN #/"
            jsr         readall
            lda         fd
            jsr         CLOSE
            lda         total + 1
            EXPECT_A    >(12 * SR_SIZE), "#/: 12 mount points (high byte)"
            lda         total
            EXPECT_A    <(12 * SR_SIZE), "#/: 12 mount points (768 bytes)"
            lda         buf + SR_NAME
            EXPECT_A    'b', "#/: bin first"

; ---- #n
            OPEN_       s_null, O_RDWR
            sta         fd
            EXPECT_OK   "OPEN #n/null"
            WRITE_      fd, s_abc, 3
            EXPECT_A    3, "null takes a write: 3"
            READ_       fd, 16
            EXPECT_A    0, "null reads as nothing"
            lda         fd
            jsr         CLOSE
            OPEN_       s_zero, O_READ
            sta         fd
            lda         #$55
            sta         buf + 9
            READ_       fd, 10
            EXPECT_A    10, "zero reads as 10 bytes"
            lda         buf + 9
            EXPECT_A    0, "of zeros"
            lda         fd
            jsr         CLOSE

; ---- #t
            OPEN_       s_ticks, O_READ
            sta         fd
            READ_       fd, 16
            pha
            lda         fd
            jsr         CLOSE
            pla
            beq         :+
            OK          "#t/ticks: some digits"
            bra         :++
:
            NOTOK       "#t/ticks: some digits"
:

; ---- #m
            OPEN_       s_hmod, O_READ
            sta         fd
            READ_       fd, SR_SIZE
            lda         fd
            jsr         CLOSE
            lda         buf + SR_NAME
            EXPECT_A    'i', "#m: init first (the module directory's order)"
            OPEN_       s_modhello, O_READ
            sta         fd
            EXPECT_OK   "OPEN #m/hello"
            READ_       fd, 64
            lda         fd
            jsr         CLOSE
            lda         buf
            EXPECT_A    'p', "#m/hello: a program"

; ---- #p: this task's status, and a child killed by its ctl
            OPEN_       s_status, O_READ
            sta         fd
            EXPECT_OK   "OPEN #p/1/status"
            READ_       fd, 64
            lda         fd
            jsr         CLOSE
            lda         buf + 2
            EXPECT_A    'd', "#p/1/status: t_dev first"
            CHILD_      s_p                                 ; (It pauses)
            jsr         nap
            ldx         #0                                  ; "#p/", its number, "/ctl"
:
            lda         s_pre,X
            sta         ctlname,X
            inx
            cpx         #3
            bne         :-
            lda         child
            cmp         #10
            bcc         :+
            pha
            lda         #'1'
            sta         ctlname,X
            inx
            pla
            sbc         #10
:
            ora         #'0'
            sta         ctlname,X
            inx
            ldy         #0
:
            lda         s_ctl,Y
            sta         ctlname,X
            inx
            iny
            cmp         #0
            bne         :-
            OPEN_       ctlname, O_WRITE
            sta         fd
            EXPECT_OK   "OPEN #p/N/ctl, the child's"
            WRITE_      fd, s_kill, 4
            EXPECT_A    4, "ctl: kill"
            lda         fd
            jsr         CLOSE
            WAITCHILD
            EXPECT_A    137, "the child ends: killed (137)"

; ---- A pipe
            jsr         PIPE
            sta         rfd
            stx         wfd
            EXPECT_OK   "PIPE"
            WRITE_      wfd, s_abc, 3
            EXPECT_A    3, "a write into the pipe: 3"
            READ_       rfd, 16
            EXPECT_A    3, "and out of it: 3"
            lda         buf + 2
            EXPECT_A    'c', "abc"
            lda         wfd
            jsr         CLOSE
            READ_       rfd, 16
            EXPECT_A    0, "no writer left: the end (0)"
            lda         rfd
            jsr         CLOSE
            jsr         PIPE
            sta         rfd
            stx         wfd
            lda         rfd
            jsr         CLOSE
            WRITE_      wfd, s_abc, 3
            EXPECT_ERR  E_PIPE, "no reader left: a write is E_PIPE"
            lda         wfd
            jsr         CLOSE

; ---- A child writing into a pipe: its fd 1 the write end
            jsr         PIPE
            sta         rfd
            stx         wfd
            lda         #1                                  ; (Stdout kept; nothing printed till it's back)
            jsr         DUP
            sta         saved
            lda         wfd
            ldx         #1
            jsr         DUP2
            lda         wfd
            jsr         CLOSE
            CHILD_      s_w                                 ; (It writes W to its fd 1)
            lda         saved
            ldx         #1
            jsr         DUP2
            lda         saved
            jsr         CLOSE
            WAITCHILD
            EXPECT_A    0, "a child writes into the pipe (its fd 1)"
            READ_       rfd, 16
            EXPECT_A    1, "the pipe has its W"
            lda         buf
            EXPECT_A    'W', "W"
            READ_       rfd, 16
            EXPECT_A    0, "and then the end: the child's end closed as it ended"
            lda         rfd
            jsr         CLOSE

; ---- A child waiting on a pipe: its fd 0 the read end
            jsr         PIPE
            sta         rfd
            stx         wfd
            lda         #0
            jsr         DUP
            sta         saved
            lda         rfd
            ldx         #0
            jsr         DUP2
            lda         rfd
            jsr         CLOSE
            CHILD_      s_i                                 ; (It reads a byte from its fd 0)
            lda         saved
            ldx         #0
            jsr         DUP2
            lda         saved
            jsr         CLOSE
            jsr         nap
            LDR         r0, info
            lda         child
            jsr         TASKINFO
            lda         info + TI_STATE
            EXPECT_A    8, "a child reading an empty pipe waits (its state: event)"
            WRITE_      wfd, s_x, 1
            WAITCHILD
            EXPECT_A    'x', "a write: its read goes on, x"
            lda         wfd
            jsr         CLOSE

; ---- A union's first bind keeps what was there
            LDR         r0, s_hroot
            LDR         r1, s_root
            lda         #MREPL
            jsr         BIND
            EXPECT_OK   "BIND #/ /"
            LDR         r0, s_hnull
            LDR         r1, s_dev
            lda         #MAFTER
            jsr         BIND
            EXPECT_OK   "BIND -a #n /dev"
            OPEN_       s_dev, O_READ
            sta         fd
            jsr         readall
            lda         fd
            jsr         CLOSE
            lda         total
            EXPECT_A    <(7 * SR_SIZE), "/dev: #/'s 5 mount points, then null and zero (7 records)"
            lda         buf + SR_NAME
            EXPECT_A    'g', "/dev: gpio first: #/'s dev"
            OPEN_       s_devzero, O_READ
            EXPECT_OK   "OPEN /dev/zero"

            DONE        "t_dev"

; Fd fd read to its end, into buf (each read from its start: buf has the first records).  OUT: total = the count
readall:
            stz         total
            stz         total + 1
@read:
            LDR         r0, buf
            LDR         r1, 512
            lda         total                               ; (Each read after the first into buf + 256, so buf
            ora         total + 1                           ;   keeps the first records)
            beq         :+
            LDR         r0, buf + 256
            LDR         r1, 256
:
            lda         fd
            jsr         READ
            bcs         @end
            sta         r2
            stx         r2 + 1
            ora         r2 + 1
            beq         @end
            clc
            lda         total
            adc         r2
            sta         total
            lda         total + 1
            adc         r2 + 1
            sta         total + 1
            bra         @read

@end:
            rts

; A moment for a child to start (and wait)
nap:
            lda         #3
            ldx         #0
            jmp         SLEEP

.rodata
s_cons:     .byte       "#c/cons", 0
s_hroot:    .byte       "#/", 0
s_null:     .byte       "#n/null", 0
s_zero:     .byte       "#n/zero", 0
s_hnull:    .byte       "#n", 0
s_ticks:    .byte       "#t/ticks", 0
s_hmod:     .byte       "#m", 0
s_modhello: .byte       "#m/hello", 0
s_status:   .byte       "#p/1/status", 0
s_pre:      .byte       "#p/"
s_ctl:      .byte       "/ctl", 0
s_kill:     .byte       "kill"
s_abc:      .byte       "abc"
s_x:        .byte       "x"
s_root:     .byte       "/", 0
s_dev:      .byte       "/dev", 0
s_devzero:  .byte       "/dev/zero", 0
s_child:    .byte       "#m/t_child", 0
s_p:        .byte       "p", 0
s_w:        .byte       "w", 0
s_i:        .byte       "i", 0
