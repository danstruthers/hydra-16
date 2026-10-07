; ****************************************************************************
; t_disk - the disks (phase 3.2 and 3.3: the storage driver's block layer and #d), run as init.  Its fds 0-2 stay
; closed, so its lines go out on the bring-up console and its marks are timed as they're made (the files it opens
; are moved to fds 5 and up).  The emulator has an SDHC card on SPI device 0 (2048 blocks: byte i of block n is
; n * 7 + i), an SDSC card on 1 (4096 blocks: n * 13 + i + 1), an echo device on 3 and nothing on 5; tests.js
; checks the cards' writes afterwards.  #d's listing; the ROM disk (the paged ROM's block 0, its size, read only,
; its end); the cards: started by an open, read, written across a block's end and whole, their ends, their ctl
; files, a card's SPI device busy; no card; the RAM disks started, written, read, stopped; and the time 4096
; bytes take from a card, the same again (from the storage driver's cache of the cards' blocks), and from a RAM disk.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "testlib.inc"

            HYX2_PROGRAM "t_disk", main

BUF_SIZE        = 1024

.zeropage
fd:         .res        1
ctl:        .res        1
total:      .res        2
cnt:        .res        1

.bss
buf:        .res        BUF_SIZE
zblk:       .res        512                                 ; A block of Z's

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

; fd's offset to offset (32 bits)
.macro SEEK_  fdv, offset
            LDR         r0, .loword(offset)
            LDR         r1, .hiword(offset)
            lda         fdv
            ldx         #0
            jsr         SEEK
.endmacro

main:
            stz         T_FAILS
            ldx         #0                                  ; A block of Z's
            lda         #'Z'
:
            sta         zblk,X
            sta         zblk + 256,X
            inx
            bne         :-

; ---- #d: the ROM disk, alone at first
            OPEN_       s_hd, O_READ, 5
            sta         fd
            EXPECT_OK   "OPEN #d"
            jsr         readall
            lda         total
            EXPECT_A    SR_SIZE, "#d: one disk started (the ROM disk)"
            lda         buf + SR_NAME
            EXPECT_A    'x', "x"

; ---- The ROM disk: the paged ROM
            OPEN_       s_xdata, O_RDWR, 5
            sta         fd
            EXPECT_OK   "OPEN #d/x/data"
            READ_       fd, 16
            EXPECT_A    16, "a read of 16"
            lda         buf
            EXPECT_A    'H', "block 0: the paged ROM's signature (Hydra-16 reborn ...)"
            lda         buf + 9
            EXPECT_A    'r', "... r"
            WRITE_      fd, s_abc, 3
            EXPECT_ERR  E_ROFS, "a write: read only (E_ROFS)"
            SEEK_       fd, 8192 * 512 - 16
            READ_       fd, 64
            EXPECT_A    16, "a read at its end: the 16 bytes left"
            READ_       fd, 64
            EXPECT_A    0, "then nothing: the end of the file"
            lda         fd
            jsr         CLOSE
            OPEN_       s_xctl, O_RDWR, 6
            sta         ctl
            jsr         ctl_read
            lda         buf + 20
            EXPECT_A    LF, "x/ctl: rom 4 MB 8192 blocks, its first line"
            lda         buf + 4
            EXPECT_A    '4', "4 MB"
            lda         buf + 21 + 14
            EXPECT_A    'R', "then its HydraFS: hydrafs label=ROM"
            lda         ctl
            jsr         CLOSE

; ---- Card 0, SDHC: started by an open
            OPEN_       s_data0, O_RDWR, 5
            sta         fd
            EXPECT_OK   "OPEN #d/0/data: card 0 started"
            READ_       fd, 16
            EXPECT_A    16, "a read of 16"
            lda         buf + 5
            EXPECT_A    5, "block 0's bytes"
            SEEK_       fd, 1000
            READ_       fd, 4
            lda         buf
            EXPECT_A    <(7 + 488), "at 1000: block 1's byte 488"
            OPEN_       s_hd, O_READ, 6
            sta         ctl
            lda         ctl
            sta         fd
            jsr         readall
            lda         #5
            sta         fd
            lda         total
            EXPECT_A    2 * SR_SIZE, "#d: 0 and x"
            lda         buf + SR_NAME
            EXPECT_A    '0', "0 first"
            OPEN_       s_ctl0, O_RDWR, 6
            sta         ctl
            jsr         ctl_read
            EXPECT_A    22, "0/ctl: sdhc 1 MB 2048 blocks (22 bytes)"
            lda         buf + 3
            EXPECT_A    'c', "sdhc"
            lda         buf + 5
            EXPECT_A    '1', "1 MB"
            lda         ctl
            jsr         CLOSE
            OPEN_       s_spi0, O_RDWR, 7
            EXPECT_ERR  E_BUSY, "OPEN #S/0/data: busy (a card's)"

; ---- Card 0: writes
            SEEK_       fd, 508
            WRITE_      fd, s_digits, 10
            EXPECT_A    10, "a write of 10 at 508: across block 0's end"
            SEEK_       fd, 4096
            WRITE_      fd, zblk, 512
            jsr         is512
            EXPECT_A    0, "a whole block at 4096 (block 8)"
            SEEK_       fd, 508
            READ_       fd, 10
            lda         buf
            EXPECT_A    '0', "read back: 0 ..."
            lda         buf + 9
            EXPECT_A    '9', "... 9"
            SEEK_       fd, 2048 * 512 - 2
            WRITE_      fd, s_end, 4
            EXPECT_A    2, "a write of 4 at 2 before its end: 2"
            WRITE_      fd, s_end, 4
            EXPECT_ERR  E_NOSPC, "then E_NOSPC"
            SEEK_       fd, 2048 * 512 - 2
            READ_       fd, 64
            EXPECT_A    2, "a read there: 2"
            lda         buf + 1
            EXPECT_A    'n', "the bytes written (en)"

; ---- Card 1, SDSC
            OPEN_       s_data1, O_RDWR, 6
            sta         fd
            EXPECT_OK   "OPEN #d/1/data: card 1 started"
            SEEK_       fd, 3 * 512 + 5
            READ_       fd, 1
            lda         buf
            EXPECT_A    <(3 * 13 + 5 + 1), "block 3's byte 5 (byte addresses: SDSC)"
            SEEK_       fd, 5 * 512 + 10
            WRITE_      fd, s_sdsc, 4
            EXPECT_A    4, "a write of 4 at block 5"
            lda         fd
            jsr         CLOSE
            lda         #5                                  ; (Card 0's fd again)
            sta         fd
            OPEN_       s_ctl1, O_READ, 6
            sta         ctl
            jsr         ctl_read
            EXPECT_A    22, "1/ctl: sdsc 2 MB 4096 blocks (22 bytes)"
            lda         buf + 3
            EXPECT_A    'c', "sdsc"
            lda         ctl
            jsr         CLOSE

; ---- No card
            OPEN_       s_data5, O_RDWR, 6
            EXPECT_ERR  E_NODEV, "OPEN #d/5/data: no card (E_NODEV)"
            OPEN_       s_data3, O_RDWR, 6
            EXPECT_ERR  E_NODEV, "OPEN #d/3/data: a device that isn't a card"
            OPEN_       s_spi3, O_RDWR, 6
            EXPECT_OK   "OPEN #S/3/data"
            OPEN_       s_data3, O_RDWR, 7
            EXPECT_ERR  E_BUSY, "then #d/3/data: busy (open in #S)"
            lda         #6
            jsr         CLOSE
            OPEN_       s_ctl5, O_READ, 6
            sta         ctl
            jsr         ctl_read
            EXPECT_A    5, "5/ctl: none"
            lda         ctl
            jsr         CLOSE

; ---- The time 4096 bytes take from a card (less the marks' own: b0)
            SEEK_       fd, 16 * 512
            MARK        "<b0"
            jsr         keep
            MARK        "b0>"
            MARK        "<card"
            jsr         read4k
            MARK        "card>"
            lda         total
            EXPECT_A    0, "4096 bytes from card 0"
            SEEK_       fd, 16 * 512                        ; The same again: from the cache
            MARK        "<hit"
            jsr         read4k
            MARK        "hit>"
            lda         total
            EXPECT_A    0, "the same 4096 bytes again (from the cache)"
            lda         fd
            jsr         CLOSE

; ---- The RAM disk
            OPEN_       s_datar, O_RDWR, 5
            EXPECT_ERR  E_NODEV, "OPEN #d/r/data: not started"
            OPEN_       s_ctlr, O_RDWR, 6
            sta         ctl
            WRITE_      ctl, s_start2x, 8
            EXPECT_ERR  E_INVAL, "r/ctl: start 2x (E_INVAL)"
            WRITE_      ctl, s_start0, 7
            EXPECT_ERR  E_INVAL, "r/ctl: start 0 (E_INVAL)"
            WRITE_      ctl, s_start4, 7
            EXPECT_OK   "r/ctl: start 4 (4 banks: 32K)"
            WRITE_      ctl, s_start4, 7
            EXPECT_ERR  E_BUSY, "start again: E_BUSY"
            jsr         ctl_read
            lda         buf + 19
            EXPECT_A    LF, "r/ctl: ram 32 KB 64 blocks, its first line"
            lda         buf + 4
            EXPECT_A    '3', "32 KB"
            lda         buf + 20 + 14
            EXPECT_A    'R', "then its HydraFS (start makes one): hydrafs label=RAM"
            OPEN_       s_datar, O_RDWR, 5
            sta         fd
            EXPECT_OK   "OPEN #d/r/data"
            SEEK_       fd, 17 * 512 + 100
            WRITE_      fd, s_ramdisk, 7
            EXPECT_A    7, "a write at block 17 (its second bank)"
            SEEK_       fd, 0
            READ_       fd, 512
            SEEK_       fd, 17 * 512 + 100
            READ_       fd, 7
            lda         buf + 6
            EXPECT_A    'k', "read back (another block read between)"
            SEEK_       fd, 0
            MARK        "<ram"
            jsr         read4k
            MARK        "ram>"
            lda         total
            EXPECT_A    0, "4096 bytes from the RAM disk"
            WRITE_      ctl, s_stop, 4
            EXPECT_ERR  E_BUSY, "stop while it's open: E_BUSY"
            lda         fd
            jsr         CLOSE
            WRITE_      ctl, s_stop, 4
            EXPECT_OK   "stop"
            jsr         ctl_read
            EXPECT_A    5, "r/ctl: none"
            WRITE_      ctl, s_start2_1, s_start2_1_end - s_start2_1
            EXPECT_ERR  E_INVAL, "r/ctl: start 4 2-1 (E_INVAL: FROM past TO)"
            WRITE_      ctl, s_start5_5, s_start5_5_end - s_start5_5
            EXPECT_ERR  E_NOMEM, "r/ctl: start 4 5-5 (E_NOMEM: no RAM module 5)"
            WRITE_      ctl, s_start1_1, s_start1_1_end - s_start1_1
            EXPECT_OK   "r/ctl: start 4 1-1 (RAM module 1's banks)"
            jsr         ctl_read
            jsr         banks_at
            stx         cnt                                 ; (The checks use .X)
            lda         buf + 1,X
            EXPECT_A    '1', "r/ctl: banks $10-$13"
            ldx         cnt
            lda         buf + 2,X
            EXPECT_A    '0', "(its first's low digit)"
            WRITE_      ctl, s_stop, 4
            lda         ctl
            jsr         CLOSE

; ---- The shared RAM disk
            OPEN_       s_ctls, O_RDWR, 6
            sta         ctl
            WRITE_      ctl, s_start16x, s_start16x_end - s_start16x
            EXPECT_ERR  E_NOMEM, "s/ctl: start 16k $90-$90 (E_NOMEM: 2 banks, 1 ID)"
            WRITE_      ctl, s_start16k, 9
            EXPECT_OK   "s/ctl: start 16k (2 banks)"
            jsr         ctl_read
            lda         buf + 20
            EXPECT_A    LF, "s/ctl: sram 16 KB 32 blocks, its first line"
            lda         buf + 21 + 14
            EXPECT_A    'S', "then hydrafs label=SRAM"
            OPEN_       s_datas, O_RDWR, 5
            sta         fd
            SEEK_       fd, 20 * 512
            WRITE_      fd, s_shared, 6
            EXPECT_A    6, "a write at block 20 (its second bank)"
            SEEK_       fd, 0
            READ_       fd, 512
            SEEK_       fd, 20 * 512
            READ_       fd, 6
            lda         buf + 5
            EXPECT_A    'd', "read back"
            lda         fd
            jsr         CLOSE
            WRITE_      ctl, s_stop, 4
            EXPECT_OK   "stop"
            lda         ctl
            jsr         CLOSE
            DONE        "t_disk"

; 4096 bytes from fd, 512 at a time.  OUT: .A = 0 if they all came (kept across the mark after it, in total)
read4k:
            lda         #8
            sta         cnt
@read:
            READ_       fd, 512
            bcs         @short
            jsr         is512
            bne         @short
            dec         cnt
            bne         @read
            lda         #0
            bra         keep

@short:
            lda         #$EE
; .A kept in total (across a MARK); .A back from it
keep:
            sta         total
            rts

; .A = 0 if .A/.X is 512
is512:
            cmp         #<512
            bne         :+
            cpx         #>512
            beq         :++
:
            lda         #$EE
            rts
:
            lda         #0
            rts

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

; ctl read from its start, into buf.  OUT: .A = the count read
ctl_read:
            SEEK_       ctl, 0
            READ_       ctl, 128
            rts

; .X = where "banks $" ends in buf ($FF: not there; then buf + 1 and on are no digits)
banks_at:
            ldx         #0
@at:
            ldy         #0
:
            lda         buf,X
            cmp         s_banks,Y
            bne         @next
            inx
            iny
            cpy         #s_banks_end - s_banks
            bne         :-
            dex                                             ; (The $)
            rts

@next:
            inx
            cpx         #120
            bcc         @at
            ldx         #$FF
            rts

.rodata
s_hd:       .byte       "#d", 0
s_xdata:    .byte       "#d/x/data", 0
s_xctl:     .byte       "#d/x/ctl", 0
s_data0:    .byte       "#d/0/data", 0
s_ctl0:     .byte       "#d/0/ctl", 0
s_data1:    .byte       "#d/1/data", 0
s_ctl1:     .byte       "#d/1/ctl", 0
s_data3:    .byte       "#d/3/data", 0
s_data5:    .byte       "#d/5/data", 0
s_ctl5:     .byte       "#d/5/ctl", 0
s_datar:    .byte       "#d/r/data", 0
s_ctlr:     .byte       "#d/r/ctl", 0
s_datas:    .byte       "#d/s/data", 0
s_ctls:     .byte       "#d/s/ctl", 0
s_spi0:     .byte       "#S/0/data", 0
s_spi3:     .byte       "#S/3/data", 0
s_abc:      .byte       "abc"
s_digits:   .byte       "0123456789"
s_end:      .byte       "endX"
s_sdsc:     .byte       "sdsc"
s_ramdisk:  .byte       "ramdisk"
s_shared:   .byte       "shared"
s_start2x:  .byte       "start 2x"
s_start0:   .byte       "start 0"
s_start4:   .byte       "start 4"
s_start16k: .byte       "start 16k"
s_start16x: .byte       "start 16k $90-$90"
s_start16x_end:
s_start2_1: .byte       "start 4 2-1"
s_start2_1_end:
s_start5_5: .byte       "start 4 5-5"
s_start5_5_end:
s_start1_1: .byte       "start 4 1-1"
s_start1_1_end:
s_banks:    .byte       "banks $"
s_banks_end:
s_stop:     .byte       "stop"
