; ****************************************************************************
; t_spi - SPI and #S (phase 3.1: the storage driver), run as init.  Its fds 0-2 stay closed, so its lines go out on
; the bring-up console and its marks are timed as they're made (the files it opens are moved to fds 5 and up).  The emulator's echo devices are on 3 and 9 (sim/lib/sd.js: each byte
; answered with the one before it; the first after a select is $A0 in mode 0, $A3 in mode 3), and nothing on 5: #S's
; directories; a write's bytes kept and read back, in parts; a read with none kept (clocked in); mode 3; a write over
; 256 bytes (a transaction a 256); one open at a time (dups sharing it); a slot's device; nothing there (MISO high:
; $FF); the ctl file's errors; and the time 256 bytes take.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "testlib.inc"

            HYX2_PROGRAM "t_spi", main

.zeropage
fd:         .res        1
fd2:        .res        1
ctl:        .res        1
total:      .res        2

BUF_SIZE        = 1024

.bss
buf:        .res        BUF_SIZE
pat:        .res        300                                 ; 0, 1, 2 ... (the low bytes)

.code

; Open path (a label) with mode, as fd fdn (fds 0-2 stay closed).  OUT: .A = the fd, C
.macro OPEN_  path, mode, fdn
            LDR         r0, path
            lda         #mode
            ldx         #fdn
            jsr         open_as
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

main:
            stz         T_FAILS
            ldx         #0                                  ; The pattern
:
            txa
            sta         pat,X
            sta         pat + 256,X
            inx
            bne         :-

; ---- #S: a directory a device, each with data and ctl
            OPEN_       s_hspi, O_READ, 5
            sta         fd
            EXPECT_OK   "OPEN #S"
            jsr         readall
            lda         total + 1
            EXPECT_A    >(16 * SR_SIZE), "#S: 16 devices (high byte)"
            lda         total
            EXPECT_A    <(16 * SR_SIZE), "#S: 16 devices (1024 bytes)"
            lda         buf + SR_NAME
            EXPECT_A    '0', "#S: 0 first"
            lda         buf + 15 * SR_SIZE + SR_NAME
            EXPECT_A    'f', "#S: f last"
            lda         buf + 15 * SR_SIZE + SR_QTYPE
            and         #QT_DIR
            EXPECT_A    QT_DIR, "#S/f: a directory"
            OPEN_       s_dev3, O_READ, 5
            sta         fd
            jsr         readall
            lda         total
            EXPECT_A    2 * SR_SIZE, "#S/3: data and ctl"

; ---- A write's bytes kept, and read back
            OPEN_       s_data3, O_RDWR, 5
            sta         fd
            EXPECT_OK   "OPEN #S/3/data"
            OPEN_       s_data3, O_RDWR, 5
            EXPECT_ERR  E_BUSY, "OPEN #S/3/data again: busy (one open at a time)"
            WRITE_      fd, s_hello, 5
            EXPECT_A    5, "a write of 5: a transaction"
            READ_       fd, 64
            EXPECT_A    5, "a read: the 5 bytes kept"
            lda         buf
            EXPECT_A    $A0, "the device's first byte: $A0 (mode 0)"
            lda         buf + 1
            EXPECT_A    'h', "then the bytes it got, one behind: h"
            lda         buf + 4
            EXPECT_A    'l', "... l"
            READ_       fd, 4
            EXPECT_A    4, "none kept: a read of 4 clocks in 4"
            lda         buf
            EXPECT_A    $A0, "a transaction of its own: $A0 first"
            lda         buf + 3
            EXPECT_A    $FF, "then the $FF it sent"
            WRITE_      fd, s_abc, 3
            READ_       fd, 1
            EXPECT_A    1, "the kept bytes in parts: 1"
            lda         buf
            EXPECT_A    $A0, "$A0"
            READ_       fd, 64
            EXPECT_A    2, "then the rest: 2"
            lda         buf + 1
            EXPECT_A    'b', "a, b"

; ---- Mode 3; the ctl file
            OPEN_       s_ctl3, O_RDWR, 6
            sta         ctl
            EXPECT_OK   "OPEN #S/3/ctl"
            jsr         ctl_read
            EXPECT_A    7, "ctl reads as the mode: mode 0 (7 bytes)"
            lda         buf + 5
            EXPECT_A    '0', "mode 0"
            WRITE_      ctl, s_mode3, 6
            EXPECT_OK   "ctl: mode 3"
            WRITE_      fd, s_xy, 2
            READ_       fd, 64
            lda         buf
            EXPECT_A    $A3, "mode 3: SCLK high at the select ($A3)"
            lda         buf + 1
            EXPECT_A    'x', "and the bytes as before"
            jsr         ctl_read
            lda         buf + 5
            EXPECT_A    '3', "ctl reads as mode 3"
            WRITE_      ctl, s_mode2, 6
            EXPECT_ERR  E_INVAL, "ctl: mode 2: E_INVAL"
            WRITE_      ctl, s_mode3, 4
            EXPECT_ERR  E_INVAL, "ctl: mode alone: E_INVAL"
            WRITE_      ctl, s_mode0, 6
            EXPECT_OK   "ctl: mode 0"
            lda         ctl
            jsr         CLOSE

; ---- Over 256 bytes: a transaction a 256
            WRITE_      fd, pat, 300
            cpx         #>300
            beq         :+
            lda         #$EE
:
            EXPECT_A    <300, "a write of 300: all of it"
            READ_       fd, 512
            EXPECT_A    300 - 256, "kept: the last transaction's (44 bytes)"
            lda         buf
            EXPECT_A    $A0, "a transaction of its own ($A0)"
            lda         buf + 43
            EXPECT_A    <298, "its bytes: the write's 257th to 299th"

; ---- The time 256 bytes take (less the marks' own: b0)
            MARK        "<b0"
            jsr         keep
            MARK        "b0>"
            MARK        "<rx"
            READ_       fd, 256
            jsr         keep
            MARK        "rx>"
            jsr         is256
            EXPECT_A    0, "256 bytes clocked in"
            MARK        "<tx"
            WRITE_      fd, pat, 256
            jsr         keep
            MARK        "tx>"
            jsr         is256
            EXPECT_A    0, "256 bytes sent"

; ---- One open at a time: the fd's dups share it
            lda         fd
            ldx         #7
            jsr         DUP2
            lda         #7
            sta         fd2
            lda         fd
            jsr         CLOSE
            OPEN_       s_data3, O_RDWR, 5
            EXPECT_ERR  E_BUSY, "its dup still open: busy"
            lda         fd2
            jsr         CLOSE
            OPEN_       s_data3, O_RDWR, 5
            sta         fd
            EXPECT_OK   "all closed: it opens again"
            lda         fd
            jsr         CLOSE

; ---- A slot's device; nothing there
            OPEN_       s_data9, O_RDWR, 5
            sta         fd
            EXPECT_OK   "OPEN #S/9/data (a slot's)"
            WRITE_      fd, s_xy, 1
            READ_       fd, 64
            lda         buf
            EXPECT_A    $A0, "device 9 answers"
            lda         fd
            jsr         CLOSE
            OPEN_       s_data5, O_RDWR, 5
            sta         fd
            WRITE_      fd, s_xy, 2
            READ_       fd, 64
            EXPECT_A    2, "device 5, nothing there: 2 bytes kept"
            lda         buf + 1
            EXPECT_A    $FF, "MISO high: $FF"
            lda         fd
            jsr         CLOSE
            OPEN_       s_datag, O_RDWR, 5
            EXPECT_ERR  E_NOENT, "OPEN #S/g/data: no such device"
            OPEN_       s_dev3x, O_READ, 5
            EXPECT_ERR  E_NOENT, "OPEN #S/3x: no such device"
            DONE        "t_spi"

; OPEN r0 with .A, the fd then moved to .X.  OUT: .A = .X; or C = 1, .A = the error
open_as:
            phx
            jsr         OPEN
            plx
            bcs         @done
            phx
            pha
            jsr         DUP2
            pla
            jsr         CLOSE
            pla
            clc
@done:
            rts

; Read fd to its end into buf (BUF_SIZE bytes at most): total = the bytes; the fd closed
readall:
            stz         total
            stz         total + 1
@read:
            clc                                             ; Into buf + total, the room left
            lda         #<buf
            adc         total
            sta         r0
            lda         #>buf
            adc         total + 1
            sta         r0 + 1
            sec
            lda         #<BUF_SIZE
            sbc         total
            sta         r1
            lda         #>BUF_SIZE
            sbc         total + 1
            sta         r1 + 1
            ora         r1
            beq         @done
            lda         fd
            jsr         READ
            bcs         @done
            sta         r2
            stx         r2 + 1
            ora         r2 + 1
            beq         @done
            clc
            lda         total
            adc         r2
            sta         total
            lda         total + 1
            adc         r2 + 1
            sta         total + 1
            bra         @read

@done:
            lda         fd
            jmp         CLOSE

; .A/.X kept in total (across a MARK); is256: .A = 0 if they're 256
keep:
            sta         total
            stx         total + 1
            rts

is256:
            lda         total
            ldx         total + 1
            cpx         #1
            beq         :+
            lda         #$EE
:
            rts

; ctl read from its start, into buf.  OUT: .A = the count read
ctl_read:
            stz         r0
            stz         r0 + 1
            stz         r1
            stz         r1 + 1
            lda         ctl
            ldx         #0
            jsr         SEEK
            LDR         r0, buf
            LDR         r1, 64
            lda         ctl
            jmp         READ

.rodata
s_hspi:     .byte       "#S", 0
s_dev3:     .byte       "#S/3", 0
s_dev3x:    .byte       "#S/3x", 0
s_data3:    .byte       "#S/3/data", 0
s_ctl3:     .byte       "#S/3/ctl", 0
s_data5:    .byte       "#S/5/data", 0
s_data9:    .byte       "#S/9/data", 0
s_datag:    .byte       "#S/g/data", 0
s_hello:    .byte       "hello"
s_abc:      .byte       "abc"
s_xy:       .byte       "xy"
s_mode0:    .byte       "mode 0"
s_mode2:    .byte       "mode 2"
s_mode3:    .byte       "mode 3"
