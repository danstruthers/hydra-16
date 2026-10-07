; ****************************************************************************
; storage - the storage driver (docs/design/reimplementation-from-scratch.md, §14.3), a boot driver on srvlib (task E): it
; owns the SPI bus and every disk, and HydraFS on them.  A module of two banks: this file is its first (srvlib, SPI,
; the disks, #S and #d); hfs.s its second (HydraFS: #f).  Its devices:
;   #S      the SPI devices, a directory each: 0-f (0-7 the board's headers, 8-f the slots' cards)
;     N/data  write: its bytes sent with device N selected, a transaction a request (256 bytes at most: a longer
;             WRITE is one a 256), and the bytes the device sends back meanwhile kept.  Read: the bytes kept, as
;             many as asked (then they're gone); none kept: as many as asked (256 at most) clocked in, a
;             transaction of their own (sending $FF).  Open by one at a time, and not while it's a card's (E_BUSY):
;             its fid's dups share it
;     N/ctl   mode 0 (SCLK idles low) or mode 3 (it idles high): both send and sample on the clock's rising edge,
;             MSB first; kept till it's changed.  Reads as the mode
;   #d      the disks, a directory each (those started: a card's opens start it): 0-f the SD cards on the SPI
;           devices, x the ROM disk (the paged ROM, read only), r the RAM disk (this task's banks), s the shared
;           one (a shared segment)
;     N/data  the disk as a file of bytes, at the fd's offset (its first 4 GB), through the block buffer; writes
;             go to the disk at once.  A card's blocks are cached (L2_SLOTS of them, written through).  Opening a
;             card's starts it (E_NODEV: no card; E_BUSY: open in #S)
;     N/ctl   reads as the disk: "sdhc 7580 MB 15523840 blocks" (sdsc, rom; ram and sram in KB), or "none".
;             init: the card started again (after it's changed); start SIZE [FROM-TO]: a RAM disk of SIZE 8K banks (or
;             SIZE K, SIZE M: 256K, 1M), and an empty HydraFS on it; stop: its banks given back (not while it's
;             open).  And HydraFS's: format [-f] [-p] [-s SIZE] [LABEL], label TEXT, check [fix] (hfs.s), and its
;             lines in the text (the label, the space free, the last check's results)
;   #f      HydraFS (hfs.s): the cards' file systems (a directory each: 0-f), or with a spec, one disk's (x, r,
;           s, a card's), or a directory of one (r/5)
; SPI is bit-banged on the VIA's port B (hw.inc), which only this task touches.  The loops are the old OS's
; (drivers/spi.s: 18 cycles a bit in, 33 out), unchanged: their timing is proven on the board; so is the SD card
; layer (drivers/sd.s).  The ROM disk is read through the kernel's ROMREAD (this module runs in place in its own
; bank: block n is bank n / 32, at $A000 + (n % 32) * 512); a RAM disk's block n is bank n / 16 of its banks, at
; $8000 + (n % 16) * 512.

.include "hydra.inc"
.include "hw.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "srvlib.inc"
.include "storage.inc"

            HYX2_DRIVER "storage", init, srv_serve, 0, 0, HF_BOOT, 2

SPI_KEEP        = 256                                       ; A transaction's bytes, at most
L2_SLOTS        = 16                                        ; The cards' block cache: its blocks ...
L2_HOT          = 12                                        ;   and the hot ones (used again), at most
ROM_BLOCKS      = 256 * (PROM_BANK_SIZE / BLOCK)            ; The paged ROM's 256 banks

.assert     PROM_BANK_SIZE .mod BLOCK = 0 .and BANK_SIZE .mod BLOCK = 0, error, "A block is inside one bank"

.zeropage
port:       .res        1                                   ; Port B: the device selected, SCLK low, MOSI high
sp_out:     .res        1                                   ; (spi_xfer's)
sp_in:      .res        1
dev:        .res        1                                   ; The SPI device
n:          .res        2                                   ; A transfer's count (1-256: n = 0 for 256)
src:        .res        2
dst:        .res        2
dk:         .res        1                                   ; The disk
lba:        .res        4                                   ; A block of it
bp:         .res        2                                   ; A RAM disk's block, mapped
pos:        .res        4                                   ; A data request's place on the disk ...
done:       .res        2                                   ;   the bytes moved ...
part:       .res        2                                   ;   this block's part of them ...
within:     .res        2                                   ;   and where it starts in the block
sd_cnt:     .res        2                                   ; (The SD layer's tries)
num:        .res        4                                   ; A number
bufp:       .res        2                                   ; A block's 512 bytes (blk_read, blk_write)
l2_ptr:     .res        2                                   ; A cached block ...
l2_from:    .res        2                                   ;   and a copy's ends
l2_to:      .res        2

.bss
spi_open:   .res        SPI_DEVS                            ; Each SPI device: its data file's fids (one open) ...
spi_mode:   .res        SPI_DEVS                            ;   its mode (0; SPI_SCLK: mode 3, SCLK idles high)
spi_rxn:    .res        SPI_DEVS                            ;   the bytes kept (2: 0-256) ...
spi_rxh:    .res        SPI_DEVS
spi_rxat:   .res        SPI_DEVS                            ;   where the next of them is ...
spi_kept:   .res        SPI_DEVS * SPI_KEEP                 ;   and they
xbuf:       .res        SPI_KEEP                            ; A request's bytes
d_state:    .res        DISKS                               ; Each disk: its state (DS_*; 0: not started) ...
d_blocks:   .res        DISKS * 4                           ;   its size in blocks ...
d_open:     .res        DISKS                               ;   its data file's fids ...
d_aux:      .res        DISKS                               ;   and a RAM disk's first bank (r) or segment (s)
c_ok:       .res        1                                   ; <> 0: blk holds a block ...
c_disk:     .res        1                                   ;   its disk ...
c_lba:      .res        4                                   ;   and its number
was_bank:   .res        1                                   ; (ram_map's: $00 and U as they were)
was_u:      .res        1
sd_arg:     .res        4                                   ; An SD command's argument (MSB first) ...
sd_r1:      .res        1                                   ;   the card's last answer ...
sd_tmp:     .res        1
sd_csd:     .res        16                                  ;   and its CSD register
blk:        .res        BLOCK                               ; The block buffer
l2_data:    .res        L2_SLOTS * BLOCK                    ; The cards' block cache: each slot's block ...
l2_disk:    .res        L2_SLOTS                            ;   its disk ($FF: free) ...
l2_b0:      .res        L2_SLOTS                            ;   its number (4 bytes) ...
l2_b1:      .res        L2_SLOTS
l2_b2:      .res        L2_SLOTS
l2_b3:      .res        L2_SLOTS
l2_agel:    .res        L2_SLOTS                            ;   when it was last used (l2_clock) ...
l2_ageh:    .res        L2_SLOTS
l2_hot:     .res        L2_SLOTS                            ;   and 1: used again since it came
l2_clock:   .res        2
l2_card:    .res        1                                   ; (l2_find's: bit 7, a card)
l2_want:    .res        1                                   ; (l2_oldest's)
l2_pick:    .res        1
l2_minl:    .res        1
l2_minh:    .res        1

.code
; ****************************************************************************
; Init: the module's banks, port B (nothing selected), the cache (empty), the ROM disk, HydraFS, and each device's
; letter
init:
            HYX2_BANKS_INIT
            ldx         #L2_SLOTS - 1
            lda         #$FF
:
            sta         l2_disk,X
            dex
            bpl         :-
            lda         #SPI_CSB | SPI_MOSI                 ; Deselected, SCLK low, MOSI high
            sta         port
            sta         VIA_PORTB
            lda         #SPI_DDR
            sta         VIA_DDRB
            lda         #DS_ROM
            sta         d_state + DISK_X
            lda         #<ROM_BLOCKS
            sta         d_blocks + DISK_X * 4
            lda         #>ROM_BLOCKS
            sta         d_blocks + DISK_X * 4 + 1
            FAR2        hfs_init
            ldx         #0
@letter:
            lda         SRV_TREES,X
            beq         @done
            phx
            jsr         SRV_REGISTER
            plx
            bcs         @failed
            inx
            inx
            inx
            bra         @letter

@done:
            clc
@failed:
            rts

; ****************************************************************************
; SPI

; Pad the receive loop at 7.16 MHz, to keep SCLK at the 3.58 MHz build's rate (about 275 kHz): 18 cycles a bit
; would be 398 kHz, too close to the 400 kHz an SD card allows while it starts.  Nothing at 3.58 MHz
.macro SPI_PAD
.if CPU_CLOCK_MULT > 1
            nop
            nop
            nop
            nop
.endif
.endmacro

; One bit in, MSB first: SCLK high (the device presents its bit), MISO sampled, SCLK low, the bit into .A.  IN: .Y
; = the port with SCLK low; .X is the sample.  18 cycles
.macro SPI_BIT_IN
            inc         VIA_PORTB                           ; SCLK high: the device presents its bit
            ldx         VIA_PORTB                           ; MISO = bit 7
            sty         VIA_PORTB                           ; SCLK low
            cpx         #$80                                ; C = MISO
            rol                                             ; ... into the byte
            SPI_PAD
.endmacro

; Select device dev for a transaction, in its mode (mode 3: SCLK high before the select, so the first bit's store is
; a falling edge).  port keeps SCLK low, as the loops want.  Modifies: .A, .X
spi_on:
            ldx         dev
            txa
            asl
            asl
            asl
            and         #SPI_DEV
            ora         #SPI_MOSI                           ; Selected, MOSI high, SCLK low
            sta         port
            ora         spi_mode,X                          ; (Mode 3: SCLK high)
            pha
            ora         #SPI_CSB                            ; Its number on the lines first, deselected, SCLK
            sta         VIA_PORTB                           ;   idle
            pla
            sta         VIA_PORTB                           ; Then selected
            rts

; Deselect, and SCLK to the mode's idle.  Modifies: .A, .X
spi_off:
            jsr         spi_deselect
            ldx         dev
            lda         spi_mode,X
            beq         :+
            lda         port                                ; (Mode 3: SCLK back up)
            ora         #SPI_SCLK
            sta         VIA_PORTB
:
            rts

; Select device .A (0-15), in mode 0 (an SD card's).  Modifies: .A
spi_select:
            asl
            asl
            asl
            and         #SPI_DEV
            ora         #SPI_MOSI                           ; /CS enable low, SCLK low, MOSI high
            sta         port
            sta         VIA_PORTB
            rts

; Deselect (the device keeps its number; the enable goes high).  Keeps .A, .X, .Y
spi_deselect:
            pha
            lda         port
            ora         #SPI_CSB
            sta         port
            sta         VIA_PORTB
            pla
            rts

; Send .A and return the byte received meanwhile.  Keeps .X, .Y.  (The bit's store drops SCLK for the next one, so
; there's no store of its own for SCLK low)
spi_xfer:
            phx
            phy
            sta         sp_out
            lda         port
            and         #<~SPI_MOSI
            tax                                             ; .X = the port with MOSI low
            ora         #SPI_MOSI
            tay                                             ; .Y = with MOSI high
            lda         #1                                  ; (The 1 comes out after 8 bits)
            sta         sp_in
@bit:
            asl         sp_out                              ; C = the bit to send
            bcs         @one
            stx         VIA_PORTB                           ; MOSI low, SCLK low
            bra         @clock

@one:
            sty         VIA_PORTB                           ; MOSI high, SCLK low
@clock:
            inc         VIA_PORTB                           ; SCLK high: both sides sample
            lda         VIA_PORTB                           ; (MISO = bit 7)
            asl                                             ; C = MISO
            rol         sp_in
            bcc         @bit
            sty         VIA_PORTB                           ; Idle: SCLK low, MOSI high
            ply
            plx
            lda         sp_in
            rts

; Receive a byte (sending $FF: MOSI high).  OUT: .A, and N/Z from it.  Keeps .X, .Y
spi_recv:
            phx
            phy
            ldy         port                                ; (SCLK low, MOSI high)
            sty         VIA_PORTB
            lda         #0

            .repeat     8
            SPI_BIT_IN
            .endrepeat

            ply
            plx
            ora         #0                                  ; N/Z from the byte (the pulls clobbered them)
            rts

; Send n bytes from (src), and put the bytes that come back at (dst); or (spi_recv_n) receive n bytes there, sending
; $FF.  (n: 1-256, 0 for 256.)  Modifies: .A, .Y
spi_xfer_n:
            ldy         #0
:
            lda         (src),Y
            jsr         spi_xfer
            sta         (dst),Y
            iny
            cpy         n
            bne         :-
            rts

spi_recv_n:
            ldy         #0
:
            jsr         spi_recv
            sta         (dst),Y
            iny
            cpy         n
            bne         :-
            rts

; .A * 8 clocks with nothing selected and MOSI high (an SD card needs 74 before it starts).  Modifies: .A
spi_idle_clocks:
            jsr         spi_deselect
@byte:
            pha
            jsr         spi_recv
            pla
            dec
            bne         @byte
            rts

; ****************************************************************************
; #S: the SPI devices, a directory each, 0-f (its id: the device)

h_devs:
            cmp         #DYN_FIND
            beq         @find
            cmp         #DYN_IDNAME
            beq         @idname
            lda         z:srv_k                             ; DYN_NAME: the srv_k-th
            cmp         #SPI_DEVS
            bcs         @done                               ; (C = 1: no more)
            tax
            jsr         disk_name
            txa
            clc
@done:
            rts

@idname:
            jsr         disk_name
            clc
            rts

@find:                                                      ; The name at srv_p: 0-f
            jsr         find_disk
            bcs         @noent
            cmp         #SPI_DEVS
            bcs         @noent
            rts

@noent:
            lda         #E_NOENT
            sec
            rts

; N/data (a fid's aux: the device)
h_data:
            ldy         z:srv_id
            sty         dev
            cmp         #R_READ
            bne         :+
            jmp         d_read
:
            cmp         #R_WRITE
            bne         :+
            jmp         d_write
:
            ldx         dev
            cmp         #R_OPEN
            beq         @open
            cmp         #R_DUP
            beq         @dup
            cmp         #R_CLUNK
            beq         @clunk
            clc
            rts

@open:                                                      ; One open at a time, and not while it's a card's
            lda         spi_open,X
            ora         d_state,X
            bne         @busy
            stz         spi_rxn,X                           ; (Nothing kept)
            stz         spi_rxh,X
@dup:
            inc         spi_open,X
            clc
            rts

@busy:
            lda         #E_BUSY
            sec
            rts

@clunk:
            dec         spi_open,X
            bne         :+
            stz         spi_rxn,X                           ; The last: nothing kept
            stz         spi_rxh,X
:
            clc
            rts

; A write: a transaction, the bytes that came back kept
d_write:
            jsr         spi_count
            bcs         @done
            LDR         r0, xbuf                            ; Its bytes, here
            MOVR        r1, TASK_INBOX + RQ_BUF
            MOVR        r2, n
            jsr         CLIENT_READ
            LDR         src, xbuf
            ldx         dev
            stz         spi_rxat,X
            jsr         kept_at                             ; dst: the device's kept bytes
            jsr         spi_on
            jsr         spi_xfer_n
            jsr         spi_off
            lda         n                                   ; Kept: all of them
            ldy         n + 1
            sta         spi_rxn,X
            tya
            sta         spi_rxh,X
            MOVR        TASK_INBOX + RQ_DONE, n
@done:
            clc
            rts

; A read: the bytes kept, as many as asked; none kept, a transaction of its own clocks them in
d_read:
            jsr         spi_count
            bcs         @done
            ldx         dev
            lda         spi_rxn,X
            ora         spi_rxh,X
            bne         @kept
            LDR         dst, xbuf                           ; None: they're clocked in
            jsr         spi_on
            jsr         spi_recv_n
            jsr         spi_off
            LDR         r0, xbuf
            bra         @send

@kept:                                                      ; As many as asked, or as are kept: the fewer
            lda         n
            cmp         spi_rxn,X
            lda         n + 1
            sbc         spi_rxh,X
            bcc         :+
            lda         spi_rxn,X                           ; (All that are kept)
            sta         n
            lda         spi_rxh,X
            sta         n + 1
:
            jsr         kept_at
            MOVR        r0, dst
            sec                                             ; Kept: fewer, from further on
            lda         spi_rxn,X
            sbc         n
            sta         spi_rxn,X
            lda         spi_rxh,X
            sbc         n + 1
            sta         spi_rxh,X
            clc
            lda         spi_rxat,X
            adc         n
            sta         spi_rxat,X
@send:
            MOVR        r2, n
            jsr         srv_toclient
@done:
            clc
            rts

; n = the request's count, 256 at most.  OUT: C = 0; or C = 1: 0 (RQ_DONE 0: nothing to do)
spi_count:
            lda         TASK_INBOX + RQ_COUNT
            ldx         TASK_INBOX + RQ_COUNT + 1
            beq         :+
            lda         #0                                  ; (Over 255: 256)
            ldx         #1
:
            sta         n
            stx         n + 1
            ora         n + 1
            bne         :+
            stz         TASK_INBOX + RQ_DONE
            stz         TASK_INBOX + RQ_DONE + 1
            sec
            rts
:
            clc
            rts

; dst = device dev's next kept byte.  OUT: .X = dev
kept_at:
            ldx         dev
            lda         spi_rxat,X
            clc
            adc         #<spi_kept
            sta         dst
            lda         #>spi_kept
            adc         dev
            sta         dst + 1
            rts

; N/ctl: mode 0, mode 3
c_mode:
            lda         z:srv_argn
            cmp         #1
            bne         @inval
            jsr         arg_word                            ; Its word: 0 or 3, alone
            ldy         #1
            lda         (src),Y
            bne         @inval
            ldx         z:srv_id
            lda         (src)
            cmp         #'0'
            beq         @set                                ; (Mode 0: 0)
            cmp         #'3'
            bne         @inval
            lda         #SPI_SCLK
@set:
            and         #SPI_SCLK
            sta         spi_mode,X
            clc
            rts

@inval:
            lda         #E_INVAL
            sec
            rts

gen_mode:
            lda         #<s_mode
            ldx         #>s_mode
            jsr         srv_tputs
            lda         #' '
            jsr         srv_tputc
            ldx         z:srv_id
            lda         spi_mode,X
            beq         :+
            lda         #3
:
            ora         #'0'
            jsr         srv_tputc
            lda         #LF
            jsr         srv_tputc
            clc
            rts

; ****************************************************************************
; SD cards, SPI mode (the old OS's drivers/sd.s): sd_init starts the card on SPI device dk (SDHC and SDXC, or SDSC,
; and its size); sd_read and sd_write move block lba to or from blk.  SDHC and SDXC cards take block numbers, SDSC
; cards byte addresses (lba * 512).  Errors: E_NODEV (no card, or it didn't answer), E_MEDIA (it refused the
; command or the data), E_IO (it stayed busy)

SD_CMD0         = 0                                         ; GO_IDLE_STATE
SD_CMD8         = 8                                         ; SEND_IF_COND
SD_CMD9         = 9                                         ; SEND_CSD (the card's size)
SD_CMD16        = 16                                        ; SET_BLOCKLEN
SD_CMD17        = 17                                        ; READ_SINGLE_BLOCK
SD_CMD24        = 24                                        ; WRITE_BLOCK
SD_CMD55        = 55                                        ; APP_CMD (the next one is an ACMD)
SD_CMD58        = 58                                        ; READ_OCR
SD_ACMD41       = 41                                        ; SD_SEND_OP_COND
SD_R1_IDLE      = $01
SD_TOKEN_DATA   = $FE                                       ; A data block's start (both ways)
SD_INIT_TRIES   = 1000                                      ; ACMD41 tries (about a second at 3.58 MHz)
SD_TOKEN_TRIES  = 4000                                      ; Bytes to wait for a data token or the end of busy

; Start the card on SPI device dk (not while it's open in #S): its state (DS_SDHC or DS_SDSC) and size.  OUT: C = 0;
; or C = 1, .A = the error (its state 0).  Modifies: .A, .X, .Y
sd_init:
            ldx         dk
            lda         spi_open,X                          ; (Open in #S: not a card's now)
            beq         :+
            lda         #E_BUSY
            sec
            rts
:
            stz         d_state,X
            jsr         blk_forget                          ; (It may be another card now)
            lda         #10                                 ; 80 clocks, nothing selected
            jsr         spi_idle_clocks
            lda         dk
            jsr         spi_select
            ldy         #10                                 ; CMD0: to SPI mode, idle
@cmd0:
            jsr         sd_arg_zero
            lda         #SD_CMD0
            jsr         sd_cmd
            cmp         #SD_R1_IDLE
            beq         @cmd8
            dey
            bne         @cmd0
            jmp         sd_no_card

@cmd8:                                                      ; CMD8: a v2 card echoes the pattern
            jsr         sd_arg_zero
            lda         #$01                                ; (2.7-3.6 V)
            sta         sd_arg + 2
            lda         #$AA                                ; (The pattern)
            sta         sd_arg + 3
            lda         #SD_CMD8
            jsr         sd_cmd
            ldx         #0                                  ; HCS = 0 for a v1 card (an illegal command)
            cmp         #SD_R1_IDLE
            bne         @acmd41
            jsr         spi_recv                            ; The R7 answer: 3 bytes, then the pattern
            jsr         spi_recv
            jsr         spi_recv
            jsr         spi_recv
            cmp         #$AA
            beq         :+
            jmp         sd_no_card
:
            ldx         #$40                                ; HCS: we take high-capacity cards
@acmd41:
            stx         sd_tmp                              ; (HCS)
            lda         #<SD_INIT_TRIES
            sta         sd_cnt
            lda         #>SD_INIT_TRIES
            sta         sd_cnt + 1
@acmd41_loop:                                               ; ACMD41 until the card leaves idle
            jsr         sd_arg_zero
            lda         #SD_CMD55
            jsr         sd_cmd
            jsr         sd_arg_zero
            lda         sd_tmp
            sta         sd_arg
            lda         #SD_ACMD41
            jsr         sd_cmd
            beq         @ready                              ; (R1 = 0)
            bmi         sd_no_card                          ; (No answer)
            jsr         sd_count_down
            bne         @acmd41_loop
            bra         sd_no_card

@ready:
            lda         #DS_SDSC
            ldx         sd_tmp
            beq         @sdsc                               ; (A v1 card is SDSC)
            jsr         sd_arg_zero                         ; CMD58: CCS in the OCR says SDHC or SDXC
            lda         #SD_CMD58
            jsr         sd_cmd
            bne         sd_refused
            jsr         spi_recv                            ; OCR bits 31-24
            tax
            jsr         spi_recv
            jsr         spi_recv
            jsr         spi_recv
            lda         #DS_SDHC
            cpx         #$C0                                ; Powered up (bit 31) and CCS (bit 30)?
            bcs         @done
@sdsc:                                                      ; Standard capacity: 512-byte blocks
            jsr         sd_arg_zero
            lda         #>512
            sta         sd_arg + 2
            lda         #SD_CMD16
            jsr         sd_cmd
            bne         sd_refused
            lda         #DS_SDSC
@done:
            sta         sd_tmp                              ; (The state)
            jsr         sd_read_size
            bcs         sd_refused
            lda         sd_tmp
            ldx         dk
            sta         d_state,X
            jsr         sd_end
            clc
            rts

sd_no_card:
            jsr         sd_end
            lda         #E_NODEV
            sec
            rts

sd_refused:
            jsr         sd_end
            lda         #E_MEDIA
            sec
            rts

; Block lba of card dk into the 512 bytes at bufp.  OUT: C = 0; or C = 1, .A = the error.  Modifies: .A, .X, .Y
sd_read:
            jsr         sd_block_arg
            lda         dk
            jsr         spi_select
            lda         #SD_CMD17
            jsr         sd_cmd
            bne         sd_refused
            jsr         sd_token_wait                       ; The data token
            cmp         #SD_TOKEN_DATA
            bne         sd_refused
            ldy         #0                                  ; 512 bytes
@first:
            jsr         spi_recv
            sta         (bufp),Y
            iny
            bne         @first
            inc         bufp + 1
@second:
            jsr         spi_recv
            sta         (bufp),Y
            iny
            bne         @second
            dec         bufp + 1
            jsr         spi_recv                            ; (The CRC: not checked)
            jsr         spi_recv
            jsr         sd_end
            clc
            rts

; The 512 bytes at bufp to block lba of card dk.  OUT: C = 0; or C = 1, .A = the error.  Modifies: .A, .X, .Y
sd_write:
            jsr         sd_block_arg
            lda         dk
            jsr         spi_select
            lda         #SD_CMD24
            jsr         sd_cmd
            bne         sd_refused
            jsr         spi_recv                            ; (A byte's gap)
            lda         #SD_TOKEN_DATA
            jsr         spi_xfer
            ldy         #0
@first:
            lda         (bufp),Y
            jsr         spi_xfer
            iny
            bne         @first
            inc         bufp + 1
@second:
            lda         (bufp),Y
            jsr         spi_xfer
            iny
            bne         @second
            dec         bufp + 1
            lda         #$FF                                ; (The CRC: not checked in SPI mode)
            jsr         spi_xfer
            lda         #$FF
            jsr         spi_xfer
            jsr         spi_recv                            ; The data response: xxx0 0101 is accepted
            sta         sd_r1
            and         #$1F
            cmp         #$05
            beq         :+
            jmp         sd_refused
:
            jsr         sd_busy_wait                        ; While it writes
            bcc         :+
            jsr         sd_end
            lda         #E_IO
            sec
            rts
:
            jsr         sd_end
            clc
            rts

; Send command .A with argument sd_arg (MSB first), and get its R1 answer.  OUT: .A = R1 (N = 1: no answer), Z from
; it.  Modifies: .X
sd_cmd:
            pha
            jsr         spi_recv                            ; (A byte before each command)
            pla
            pha
            ora         #$40
            jsr         spi_xfer
            ldx         #0
@arg:
            lda         sd_arg,X
            jsr         spi_xfer
            inx
            cpx         #4
            bne         @arg
            pla                                             ; The CRC: only CMD0 and CMD8 need a real one
            ldx         #$95                                ;   (CRC checking is off in SPI mode)
            cmp         #SD_CMD0
            beq         @crc
            ldx         #$87
            cmp         #SD_CMD8
            beq         @crc
            ldx         #$01                                ; (The end bit)
@crc:
            txa
            jsr         spi_xfer
            ldx         #10                                 ; The answer: within 8 bytes
@answer:
            jsr         spi_recv
            bpl         @got
            dex
            bne         @answer
@got:
            sta         sd_r1
            ora         #0
            rts

; Card dk's CSD register (into sd_csd), and its size in blocks (into its d_blocks).  The card must be selected.  CSD
; v2 (SDHC, SDXC): (C_SIZE + 1) * 1024 blocks.  CSD v1 (SDSC): (C_SIZE + 1) << (C_SIZE_MULT + 2) blocks of
; READ_BL_LEN (as a shift), in 512-byte blocks.  OUT: C = 0; or C = 1 (no answer, or a CSD version we don't know).
; Modifies: .A, .X, lba, sd_arg
sd_read_size:
            jsr         sd_arg_zero
            lda         #SD_CMD9
            jsr         sd_cmd
            bne         @fail
            jsr         sd_token_wait                       ; The data token
            cmp         #SD_TOKEN_DATA
            beq         @read
@fail:
            sec
            rts

@read:
            ldx         #0
@csd:                                                       ; 16 bytes, MSB (bit 127) first
            jsr         spi_recv
            sta         sd_csd,X
            inx
            cpx         #16
            bne         @csd
            jsr         spi_recv                            ; (The CRC: not checked)
            jsr         spi_recv
            stz         lba + 3
            lda         sd_csd                              ; CSD_STRUCTURE: bits 127-126
            and         #$C0
            beq         @v1
            cmp         #$40
            bne         @fail
            lda         sd_csd + 9                          ; v2: C_SIZE = bits 69-48
            sta         lba
            lda         sd_csd + 8
            sta         lba + 1
            lda         sd_csd + 7
            and         #$3F
            sta         lba + 2
            ldx         #10                                 ; (* 1024)
            bra         @plus_one

@v1:                                                        ; v1: C_SIZE = bits 73-62
            lda         sd_csd + 8
            sta         lba
            lda         sd_csd + 7
            sta         lba + 1
            lda         sd_csd + 6
            and         #$03
            sta         lba + 2
            ldx         #6
@down:
            lsr         lba + 2
            ror         lba + 1
            ror         lba
            dex
            bne         @down
            lda         sd_csd + 10                         ; C_SIZE_MULT = bits 49-47
            asl                                             ; (C = bit 47)
            lda         sd_csd + 9
            and         #$03
            rol                                             ; (C = 0)
            sta         sd_arg                              ; (Free: the command is done)
            lda         sd_csd + 5                          ; READ_BL_LEN = bits 83-80
            and         #$0F
            adc         sd_arg
            sec
            sbc         #7                                  ; The shift: C_SIZE_MULT + 2 + READ_BL_LEN - 9
            tax
@plus_one:                                                  ; lba = (C_SIZE + 1) << .X
            inc         lba
            bne         @up
            inc         lba + 1
            bne         @up
            inc         lba + 2
@up:
            asl         lba
            rol         lba + 1
            rol         lba + 2
            rol         lba + 3
            dex
            bne         @up
            lda         dk                                  ; Its d_blocks
            asl
            asl
            tax
            lda         lba
            sta         d_blocks,X
            lda         lba + 1
            sta         d_blocks + 1,X
            lda         lba + 2
            sta         d_blocks + 2,X
            lda         lba + 3
            sta         d_blocks + 3,X
            clc
            rts

; sd_arg = 0.  Keeps .A, .X, .Y
sd_arg_zero:
            stz         sd_arg
            stz         sd_arg + 1
            stz         sd_arg + 2
            stz         sd_arg + 3
            rts

; sd_arg = block lba's address: its number (SDHC), or * 512 (SDSC).  Modifies: .A, .X
sd_block_arg:
            ldx         dk
            lda         d_state,X
            cmp         #DS_SDHC
            bne         @bytes
            lda         lba + 3                             ; (MSB first)
            sta         sd_arg
            lda         lba + 2
            sta         sd_arg + 1
            lda         lba + 1
            sta         sd_arg + 2
            lda         lba
            sta         sd_arg + 3
            rts

@bytes:                                                     ; lba << 9
            stz         sd_arg + 3
            lda         lba
            asl
            sta         sd_arg + 2
            lda         lba + 1
            rol
            sta         sd_arg + 1
            lda         lba + 2
            rol
            sta         sd_arg
            rts

; Wait for a byte other than $FF (a data token).  OUT: .A = it ($FF: gave up).  Modifies: .X
sd_token_wait:
            lda         #<SD_TOKEN_TRIES
            sta         sd_cnt
            lda         #>SD_TOKEN_TRIES
            sta         sd_cnt + 1
@wait:
            jsr         spi_recv
            cmp         #$FF
            bne         @got
            jsr         sd_count_down
            bne         @wait
            lda         #$FF
@got:
            sta         sd_r1
            rts

; Wait while the card is busy (it sends 0s).  OUT: C = 0; or C = 1: it's still busy.  Modifies: .A
sd_busy_wait:
            lda         #<SD_TOKEN_TRIES
            sta         sd_cnt
            lda         #>SD_TOKEN_TRIES
            sta         sd_cnt + 1
@wait:
            jsr         spi_recv
            cmp         #$FF
            beq         @done                               ; ($FF: not busy any more)
            jsr         sd_count_down
            bne         @wait
            sec
            rts

@done:
            clc
            rts

; sd_cnt - 1; Z = 1 when it reaches 0.  Modifies: .A
sd_count_down:
            lda         sd_cnt
            bne         :+
            dec         sd_cnt + 1
:
            dec         sd_cnt
            lda         sd_cnt
            ora         sd_cnt + 1
            rts

; Deselect, and a byte more of clocks (the card lets go of MISO).  Keeps .A
sd_end:
            pha
            jsr         spi_deselect
            jsr         spi_recv
            pla
            rts

; ****************************************************************************
; The ROM disk and the RAM disks

; Block lba of the ROM disk into the 512 bytes at bufp: bank lba / 32, at $A000 + (lba % 32) * 512, through ROMREAD.
; The disk is the paged ROM in socket order (hw.inc: PROM_SOCKET_BANKS), so a disk's blocks fill the sockets in
; turn: its bank i is the CPU's bank i with bits 6 and 7 swapped
rom_read:
            lda         lba + 1                             ; The bank: lba / 32 (13 bits: 8 of them)
            asl
            asl
            asl
            sta         num
            lda         lba
            lsr
            lsr
            lsr
            lsr
            lsr
            ora         num
            tay                                             ; Bits 6 and 7 swapped: both flipped if they differ
            and         #$C0
            beq         :+
            cmp         #$C0
            beq         :+
            tya
            eor         #$C0
            tay
:
            tya
            pha
            lda         lba                                 ; Where in it
            and         #$1F
            asl
            adc         #>PROM_WINDOW                       ; (C = 0: the asl's bit 7 was 0)
            sta         r0 + 1
            stz         r0
            MOVR        r1, bufp
            LDR         r2, BLOCK
            pla
            jmp         ROMREAD

rom_write:
            lda         #E_ROFS
            sec
            rts

; Block lba of RAM disk dk into the 512 bytes at bufp (ram_read), or them to it (ram_write)
ram_read:
            jsr         ram_map
            bcs         @done
            ldy         #0
:
            lda         (bp),Y
            sta         (bufp),Y
            iny
            bne         :-
            inc         bp + 1
            inc         bufp + 1
:
            lda         (bp),Y
            sta         (bufp),Y
            iny
            bne         :-
            dec         bufp + 1
            jmp         ram_unmap

@done:
            rts

ram_write:
            jsr         ram_map
            bcs         @done
            ldy         #0
:
            lda         (bufp),Y
            sta         (bp),Y
            iny
            bne         :-
            inc         bp + 1
            inc         bufp + 1
:
            lda         (bufp),Y
            sta         (bp),Y
            iny
            bne         :-
            dec         bufp + 1
            jmp         ram_unmap

@done:
            rts

; Block lba of RAM disk dk mapped at $8000-$9FFF, bp -> it: bank lba / 16 of its banks (this task's own, from its
; d_aux; or its shared segment's), with $00 and U as they were kept for ram_unmap.  OUT: C = 0; or C = 1, .A = the
; error (SEG_MAP's)
ram_map:
            lda         lba                                 ; Where in its bank: $8000 + (lba % 16) * 512
            and         #$0F
            asl
            ora         #>BANK_WINDOW
            sta         bp + 1
            stz         bp
            lda         lba + 1                             ; Which of its banks: lba / 16 (8 bits)
            asl
            asl
            asl
            asl
            sta         num
            lda         lba
            lsr
            lsr
            lsr
            lsr
            ora         num
            ldx         RAM_BANK                            ; (Put back by ram_unmap)
            stx         was_bank
            ldx         U_REGISTER
            stx         was_u
            ldx         dk
            cpx         #DISK_S
            beq         @shared
            clc                                             ; The RAM disk: this task's banks
            adc         d_aux,X
            sta         RAM_BANK
            clc
            rts

@shared:                                                    ; The shared one: its segment's
            tax
            lda         d_aux + DISK_S
            jsr         SEG_MAP
            bcs         @done
            sta         U_REGISTER
            stx         RAM_BANK
@done:
            rts

; $00 and U put back.  OUT: C = 0
ram_unmap:
            lda         was_bank
            sta         RAM_BANK
            lda         was_u
            sta         U_REGISTER
            clc
            rts

; ****************************************************************************
; Blocks: the block buffer, and each kind of disk's reads and writes

; Block lba of disk dk in blk (and bufp = blk): read, unless it's there already.  OUT: C = 0; or C = 1, .A = the
; error
blk_get:
            lda         #<blk
            sta         bufp
            lda         #>blk
            sta         bufp + 1
            jsr         blk_here
            bcc         @done
            stz         c_ok                                ; (Nothing there, if the read fails)
            jsr         blk_read
            bcs         @done
            jsr         blk_claim
@done:
            rts

; Is block lba of disk dk the one in blk?  OUT: C = 0: it is.  Modifies: .A, .X
blk_here:
            lda         c_ok
            beq         @no
            lda         c_disk
            cmp         dk
            bne         @no
            ldx         #3
:
            lda         c_lba,X
            cmp         lba,X
            bne         @no
            dex
            bpl         :-
            clc
            rts

@no:
            sec
            rts

; blk is block lba of disk dk.  OUT: C = 0
blk_claim:
            lda         dk
            sta         c_disk
            ldx         #3
:
            lda         lba,X
            sta         c_lba,X
            dex
            bpl         :-
            lda         #1
            sta         c_ok
            clc
            rts

; Nothing of disk dk's in blk, or the cache.  Keeps .X
blk_forget:
            lda         c_disk
            cmp         dk
            bne         :+
            stz         c_ok
:
            jmp         l2_forget

; Block lba of disk dk into the 512 bytes at bufp (blk_read), or them to it (blk_write: then blk is that block, if
; they were blk's; if not, blk forgets it, if it had it); a card's through the cache.  OUT: C = 0; or C = 1, .A =
; the error
blk_read:
            jsr         blk_check
            bcs         blk_failed
            phx
            jsr         l2_get                              ; (A card's block kept: from the cache)
            plx
            bcs         :+
            rts
:
            jsr         @go
            bcs         blk_failed
            jmp         l2_put

@go:
            jmp         (blk_readers,X)

blk_write:
            jsr         blk_check
            bcs         blk_failed
            jsr         @go
            bcc         :+
            jmp         l2_drop                             ; (What the card has is unknown now)
:
            jsr         l2_put
            lda         bufp
            cmp         #<blk
            bne         @other
            lda         bufp + 1
            cmp         #>blk
            bne         @other
            jmp         blk_claim

@other:
            jsr         blk_here
            bcs         :+
            stz         c_ok
:
            clc
            rts

@go:
            jmp         (blk_writers,X)

blk_failed:
            rts

; Disk dk started, and lba on it.  OUT: .X = its state * 2; or C = 1, .A = E_NODEV, E_RANGE
blk_check:
            ldx         dk
            lda         d_state,X
            beq         @nodev
            asl
            pha
            jsr         on_disk
            plx
            bcc         :+
            lda         #E_RANGE
:
            rts

@nodev:
            lda         #E_NODEV
            sec
            rts

; lba before disk dk's end?  OUT: C = 0; or C = 1: past it
on_disk:
            lda         dk
            asl
            asl
            tax
            lda         lba
            cmp         d_blocks,X
            lda         lba + 1
            sbc         d_blocks + 1,X
            lda         lba + 2
            sbc         d_blocks + 2,X
            lda         lba + 3
            sbc         d_blocks + 3,X
            rts

; ****************************************************************************
; The cards' block cache: L2_SLOTS blocks in this task's RAM, under blk_read and blk_write (and so under blk, which
; holds the block being worked on), and always what's on the card: a block read from a card is kept, a block
; written to one is kept as written, and a block that's kept is copied from here (about 6,000 cycles; the card's
; read is 131,000).  So a name looked up again (a program's, through /bin) costs no card reads.  A block used again
; is hot: a new block takes a free slot, else the oldest cold one's, else the oldest hot one's, so a file read
; through doesn't push the directories out; L2_HOT at most are hot.  A card started (or started again: its ctl's
; init) or stopped has none here.

; Block lba of disk dk from the cache into the 512 bytes at bufp, if it's a card's and kept.  OUT: C = 0: it was;
; C = 1: it wasn't.  Modifies: .A, .X, .Y
l2_get:
            jsr         l2_find
            bcs         @done
            jsr         l2_at                               ; From its slot
            MOVR        l2_from, l2_ptr
            MOVR        l2_to, bufp
            jsr         l2_copy
            jsr         l2_touch
            jsr         l2_heat
            clc
@done:
            rts

; The 512 bytes at bufp (just read from block lba of disk dk, or written to it) kept, if it's a card's: in its
; slot, or a new one.  OUT: C = 0.  Modifies: .A, .X, .Y
l2_put:
            jsr         l2_find
            bcc         @slot
            bit         l2_card                             ; (Not a card's: nothing kept)
            bpl         @done
            jsr         l2_victim                           ; A new slot: the block's, cold
            lda         dk
            sta         l2_disk,X
            lda         lba
            sta         l2_b0,X
            lda         lba + 1
            sta         l2_b1,X
            lda         lba + 2
            sta         l2_b2,X
            lda         lba + 3
            sta         l2_b3,X
            stz         l2_hot,X
@slot:
            jsr         l2_at
            MOVR        l2_from, bufp
            MOVR        l2_to, l2_ptr
            jsr         l2_copy
            jsr         l2_touch
@done:
            clc
            rts

; Block lba of disk dk not kept (its write failed: what the card has is unknown).  Keeps .A and C
l2_drop:
            php
            pha
            jsr         l2_find
            bcs         :+
            lda         #$FF
            sta         l2_disk,X
:
            pla
            plp
            rts

; Disk dk's blocks not kept.  Keeps .X
l2_forget:
            phx
            ldx         #L2_SLOTS - 1
@slot:
            lda         l2_disk,X
            cmp         dk
            bne         :+
            lda         #$FF
            sta         l2_disk,X
:
            dex
            bpl         @slot
            plx
            rts

; The slot keeping block lba of disk dk; l2_card's bit 7: dk is a card.  OUT: C = 0, .X = it; or C = 1: none
l2_find:
            stz         l2_card
            lda         dk
            cmp         #SPI_DEVS
            bcs         @none
            dec         l2_card
            ldx         #L2_SLOTS - 1
@slot:
            lda         l2_disk,X
            cmp         dk
            bne         @next
            lda         l2_b0,X
            cmp         lba
            bne         @next
            lda         l2_b1,X
            cmp         lba + 1
            bne         @next
            lda         l2_b2,X
            cmp         lba + 2
            bne         @next
            lda         l2_b3,X
            cmp         lba + 3
            beq         @found
@next:
            dex
            bpl         @slot
@none:
            sec
            rts

@found:
            clc
            rts

; l2_ptr = slot .X's block: l2_data + .X * 512.  Keeps .X
l2_at:
            lda         #<l2_data
            sta         l2_ptr
            txa
            asl
            clc
            adc         #>l2_data
            sta         l2_ptr + 1
            rts

; 512 bytes from l2_from to l2_to.  Modifies: .A, .Y
l2_copy:
            ldy         #0
:
            lda         (l2_from),Y
            sta         (l2_to),Y
            iny
            bne         :-
            inc         l2_from + 1
            inc         l2_to + 1
:
            lda         (l2_from),Y
            sta         (l2_to),Y
            iny
            bne         :-
            rts

; Slot .X used now: its age the clock's (the clock wrapped: every slot as old as the others first).  Keeps .X
l2_touch:
            inc         l2_clock
            bne         @age
            inc         l2_clock + 1
            bne         @age
            phx
            ldx         #L2_SLOTS - 1
:
            stz         l2_agel,X
            stz         l2_ageh,X
            dex
            bpl         :-
            plx
            inc         l2_clock
@age:
            lda         l2_clock
            sta         l2_agel,X
            lda         l2_clock + 1
            sta         l2_ageh,X
            rts

; Slot .X used again: hot (and the oldest other hot one cold, if that's more than L2_HOT).  Keeps .X
l2_heat:
            lda         l2_hot,X
            bne         @done
            inc         l2_hot,X
            phx
            ldy         #0                                  ; The hot ones
            ldx         #L2_SLOTS - 1
:
            lda         l2_hot,X
            beq         :+
            iny
:
            dex
            bpl         :--
            cpy         #L2_HOT + 1
            bcc         :+
            lda         #1                                  ; (Slot .X is the newest: not the oldest)
            jsr         l2_oldest
            stz         l2_hot,X
:
            plx
@done:
            rts

; The slot a new block takes: a free one, else the oldest cold one, else the oldest hot one.  OUT: .X
l2_victim:
            ldx         #L2_SLOTS - 1
:
            lda         l2_disk,X
            cmp         #$FF
            beq         @done
            dex
            bpl         :-
            lda         #0
            jsr         l2_oldest
            bcc         @done
            lda         #1
            jsr         l2_oldest
@done:
            rts

; The oldest slot whose hot flag is .A (0 or 1).  OUT: C = 0, .X = it; or C = 1: none.  Modifies: .A, .Y
l2_oldest:
            sta         l2_want
            lda         #$FF
            sta         l2_pick
            ldy         #L2_SLOTS - 1
@slot:
            lda         l2_hot,Y
            cmp         l2_want
            bne         @next
            lda         l2_pick                             ; The first, or older than the oldest so far?
            cmp         #$FF
            beq         @take
            lda         l2_agel,Y
            cmp         l2_minl
            lda         l2_ageh,Y
            sbc         l2_minh
            bcs         @next
@take:
            sty         l2_pick
            lda         l2_agel,Y
            sta         l2_minl
            lda         l2_ageh,Y
            sta         l2_minh
@next:
            dey
            bpl         @slot
            ldx         l2_pick
            cpx         #$FF                                ; (C = 1: none)
            rts

; ****************************************************************************
; #d: the disks, a directory each (its id: the disk)

h_disks:
            cmp         #DYN_FIND
            beq         @find
            cmp         #DYN_IDNAME
            beq         @idname
            ldx         #0                                  ; DYN_NAME: the srv_k-th disk started
            ldy         z:srv_k
@disk:
            lda         d_state,X
            beq         @next
            cpy         #0
            beq         @this
            dey
@next:
            inx
            cpx         #DISKS
            bcc         @disk
            sec                                             ; (No more)
            rts

@this:
            jsr         disk_name
            txa
            clc
            rts

@idname:
            jsr         disk_name
            clc
            rts

@find:
            jsr         find_disk
            bcc         :+
            lda         #E_NOENT
:
            rts

; Disk .X's name (0-f, x, r, s) into srv_dname.  Keeps .X
disk_name:
            lda         s_disks,X
            sta         srv_dname
            stz         srv_dname + 1
            rts

; The name at srv_p, a disk's (one of s_disks, alone).  OUT: C = 0, .A = the disk; or C = 1
find_disk:
            lda         z:srv_p
            sta         src
            lda         z:srv_p + 1
            sta         src + 1
            ldy         #1
            lda         (src),Y
            beq         :+
            cmp         #'/'
            bne         @none
:
            lda         (src)
            ldx         #DISKS - 1
:
            cmp         s_disks,X
            beq         @found
            dex
            bpl         :-
@none:
            sec
            rts

@found:
            txa
            clc
            rts

; Disk dk started, if it isn't: a card (sd_init); the others start otherwise (the ROM disk at init, a RAM disk by its
; ctl's start).  OUT: C = 0; or C = 1, .A = the error (E_NODEV: none there)
disk_start:
            ldx         dk
            lda         d_state,X
            bne         @ok
            cpx         #SPI_DEVS
            bcs         @none
            jmp         sd_init

@ok:
            clc
            rts

@none:
            lda         #E_NODEV
            sec
            rts

; N/data (a fid's aux: the disk)
h_disk:
            ldy         z:srv_id
            sty         dk
            ldx         dk
            cmp         #R_READ
            bne         :+
            jmp         dd_read
:
            cmp         #R_WRITE
            bne         :+
            jmp         dd_write
:
            cmp         #R_OPEN
            beq         @open
            cmp         #R_DUP
            beq         @dup
            cmp         #R_CLUNK
            beq         @clunk
            clc
            rts

@open:                                                      ; (A card not started yet: now)
            jsr         disk_start
            bcs         @done
            ldx         dk
@dup:
            inc         d_open,X
            clc
@done:
            rts

@clunk:
            dec         d_open,X
            clc
            rts

; A read at the request's offset, block by block through blk; at the disk's end, short (nothing: the end of the
; file).  An error after some were moved: those (the next read gets it)
dd_read:
            jsr         dd_setup
@block:
            jsr         dd_next
            bcs         dd_end
            jsr         blk_get
            bcs         dd_failed
            jsr         dd_ptrs
            jsr         CLIENT_WRITE
            jsr         dd_moved
            bra         @block

dd_failed:
            ldx         done                                ; (Some moved: those)
            bne         dd_end
            ldx         done + 1
            bne         dd_end
            sec
            rts

dd_end:
            MOVR        TASK_INBOX + RQ_DONE, done
            clc
            rts

; A write at the request's offset, block by block through blk (a part of one read first), each block written at once.
; Past the disk's end: E_NOSPC
dd_write:
            lda         d_state,X
            cmp         #DS_ROM
            bne         :+
            lda         #E_ROFS
            sec
            rts
:
            jsr         dd_setup
@block:
            jsr         dd_next
            bcs         @end
            lda         part + 1                            ; A whole block?  (512: then within is 0)
            cmp         #>BLOCK
            bne         @part
            jsr         blk_claim                           ; (Nothing to read first)
            bra         @fill

@part:
            jsr         blk_get
            bcs         dd_failed
@fill:
            jsr         dd_ptrs
            jsr         CLIENT_READ
            jsr         blk_write
            bcc         :+
            pha
            jsr         blk_forget
            pla
            bra         dd_failed
:
            jsr         dd_moved
            bra         @block

@end:
            lda         done
            ora         done + 1
            bne         dd_end
            lda         #E_NOSPC                            ; (None: past the end)
            sec
            rts

; pos = the request's offset; done = 0; bufp = blk
dd_setup:
            lda         #<blk
            sta         bufp
            lda         #>blk
            sta         bufp + 1
            ldx         #3
:
            lda         TASK_INBOX + RQ_OFFSET,X
            sta         pos,X
            dex
            bpl         :-
            stz         done
            stz         done + 1
            rts

; The next block's part: lba = pos / 512, within = pos % 512, part = the fewer of 512 - within and the bytes left
; (RQ_COUNT - done).  OUT: C = 0; or C = 1: no more (all moved, or the disk's end)
dd_next:
            sec                                             ; Left
            lda         TASK_INBOX + RQ_COUNT
            sbc         done
            sta         part
            lda         TASK_INBOX + RQ_COUNT + 1
            sbc         done + 1
            sta         part + 1
            ora         part
            beq         @none
            lda         pos + 3                             ; lba = pos >> 9
            lsr
            sta         lba + 2
            lda         pos + 2
            ror
            sta         lba + 1
            lda         pos + 1
            ror
            sta         lba
            stz         lba + 3
            jsr         on_disk
            bcs         @none
            lda         pos                                 ; within = pos & 511
            sta         within
            lda         pos + 1
            and         #>(BLOCK - 1)
            sta         within + 1
            sec                                             ; The block's room: 512 - within
            lda         #<BLOCK
            sbc         within
            sta         num
            lda         #>BLOCK
            sbc         within + 1
            sta         num + 1
            lda         num                                 ; Fewer than the bytes left?
            cmp         part
            lda         num + 1
            sbc         part + 1
            bcs         :+
            lda         num
            sta         part
            lda         num + 1
            sta         part + 1
:
            clc
            rts

@none:
            sec
            rts

; r0 = blk + within, r1 = the client's buffer + done, r2 = part
dd_ptrs:
            clc
            lda         #<blk
            adc         within
            sta         r0
            lda         #>blk
            adc         within + 1
            sta         r0 + 1
            clc
            lda         TASK_INBOX + RQ_BUF
            adc         done
            sta         r1
            lda         TASK_INBOX + RQ_BUF + 1
            adc         done + 1
            sta         r1 + 1
            MOVR        r2, part
            rts

; done and pos on by part
dd_moved:
            clc
            lda         done
            adc         part
            sta         done
            lda         done + 1
            adc         part + 1
            sta         done + 1
            clc
            lda         pos
            adc         part
            sta         pos
            lda         pos + 1
            adc         part + 1
            sta         pos + 1
            bcc         :+
            inc         pos + 2
            bne         :+
            inc         pos + 3
:
            rts

; N/ctl: init; start SIZE, stop (a RAM disk)
c_init:
            lda         z:srv_id
            sta         dk
            cmp         #SPI_DEVS
            bcs         @done                               ; (Not a card: nothing to do)
            jsr         blk_forget
            FAR2        hfs_forget                          ; (Another card may be in it now)
            jmp         sd_init

@done:
            clc
            rts

c_start:
            jsr         ram_disk                            ; A RAM disk, not started
            bcs         @done
            ldx         dk
            lda         d_state,X
            bne         @busy
            jsr         start_args                          ; Its size (n: its banks), and from where
            bcs         @done
            ldx         dk
            cpx         #DISK_S
            beq         @shared
            lda         num + 2                             ; This task's banks, on modules FROM-TO ($m0-$mF)
            asl
            asl
            asl
            asl
            tax
            lda         num + 3
            asl
            asl
            asl
            asl
            ora         #$0F
            tay
            lda         n
            jsr         BANKS_ALLOC_IN
            bcs         @done
            ldy         #DS_RAM
            bra         @started

@shared:
            lda         n                                   ; A shared segment, from shared bank IDs FROM-TO
            ldx         num + 2
            ldy         num + 3
            jsr         SEG_CREATE_IN
            bcs         @done
            ldy         #DS_SRAM
@started:
            ldx         dk
            sta         d_aux,X
            tya
            sta         d_state,X
            txa                                             ; Its blocks: its banks * 16
            asl
            asl
            tax
            lda         n
            stz         d_blocks + 1,X
            .repeat     4
            asl
            rol         d_blocks + 1,X
            .endrepeat
            sta         d_blocks,X
            stz         d_blocks + 2,X
            stz         d_blocks + 3,X
            FAR2        hfs_format_ram                      ; An empty HydraFS on it
@done:
            rts

@busy:
            lda         #E_BUSY
            sec
            rts

@inval:
            lda         #E_INVAL
            sec
            rts

c_stop:
            jsr         ram_disk
            bcs         @failed
            ldx         dk
            lda         d_state,X
            beq         @done                               ; (Not started: C = 0)
            lda         d_open,X                            ; (Not while it's open: as a disk, or a file on it)
            bne         @busy
            FAR2        hfs_in_use
            bcs         @failed
            FAR2        hfs_forget
            ldx         dk
            jsr         blk_forget
            stz         d_state,X
            lda         d_aux,X                             ; Its banks back
            cpx         #DISK_S
            beq         @shared
            pha
            txa                                             ; (How many: its blocks / 16)
            asl
            asl
            tax
            lda         d_blocks,X
            sta         num
            lda         d_blocks + 1,X
            .repeat     4
            lsr
            ror         num
            .endrepeat
            ldx         num
            pla
            jmp         BANKS_FREE

@shared:
            jmp         SEG_DETACH

@busy:
            lda         #E_BUSY
            sec
            rts

@done:
            clc
@failed:
            rts

; format [-f] [-p] [-s SIZE] [LABEL], label TEXT, check [fix]: HydraFS's (hfs.s)
c_format:
            lda         z:srv_id
            sta         dk
            FAR2        hfs_format
            rts

c_label:
            lda         z:srv_id
            sta         dk
            FAR2        hfs_label
            rts

c_check:
            lda         z:srv_id
            sta         dk
            FAR2        hfs_check
            rts

; #f: HydraFS (hfs.s), every request
h_fs:
            FAR2        hfs_serve
            rts

; dk = the ctl's disk, a RAM disk.  OUT: C = 0; or C = 1, .A = E_INVAL (not one)
ram_disk:
            lda         z:srv_id
            sta         dk
            cmp         #DISK_R
            bcs         :+
            lda         #E_INVAL
            sec
            rts
:
            clc
            rts

; The command's word, a size in 8K banks: N, N K (kilobytes, rounded up to banks) or N M (megabytes); N decimal, 4
; digits at most.  OUT: .A = the banks (1-255); or C = 1
ram_size:
            jsr         arg_word
            stz         num
            stz         num + 1
            ldy         #0
@digit:
            lda         (src),Y
            sec
            sbc         #'0'
            cmp         #10
            bcs         @unit
            cpy         #4
            bcs         @bad
            pha                                             ; num * 10 + the digit
            lda         num
            ldx         num + 1
            asl         num
            rol         num + 1
            asl         num
            rol         num + 1
            clc
            adc         num
            sta         num
            txa
            adc         num + 1
            sta         num + 1
            asl         num
            rol         num + 1
            pla
            clc
            adc         num
            sta         num
            bcc         :+
            inc         num + 1
:
            iny
            bra         @digit

@unit:
            tya                                             ; (No digits: not a size)
            beq         @bad
            lda         (src),Y
            beq         @banks
            ora         #$20                                ; (Either case)
            tax
            iny
            lda         (src),Y                             ; (The unit ends the word)
            bne         @bad
            cpx         #'k'
            beq         @kilo
            cpx         #'m'
            bne         @bad
            lda         num + 1                             ; Megabytes: 1 (128 banks)
            bne         @bad
            lda         num
            cmp         #1
            bne         @bad
            lda         #128
            clc
            rts

@kilo:                                                      ; Kilobytes: (K + 7) / 8
            clc
            lda         num
            adc         #7
            sta         num
            bcc         :+
            inc         num + 1
:
            .repeat     3
            lsr         num + 1
            ror         num
            .endrepeat
@banks:
            lda         num + 1
            bne         @bad
            lda         num
            beq         @bad
            clc
            rts

@bad:
            sec
            rts

; start's words: n = its banks (SIZE), num + 2 and num + 3 = FROM and TO (from anywhere: 0 and $FF).  OUT: C = 0;
; or C = 1, .A = E_INVAL
start_args:
            lda         z:srv_argn
            beq         @inval
            cmp         #3
            bcs         @inval
            jsr         ram_size
            bcs         @inval
            sta         n
            lda         #0
            sta         num + 2
            dec         a
            sta         num + 3
            lda         z:srv_argn
            cmp         #2
            bne         :+
            jsr         ram_range
            bcs         @inval
:
            clc
            rts

@inval:
            lda         #E_INVAL
            sec
            rts

; start's FROM-TO, its second word after it: num + 2 = FROM, num + 3 = TO, bytes (decimal, or $hex), FROM <= TO, and
; on r modules (0-15).  OUT: C = 0; or C = 1
ram_range:
            lda         srv_argp + 2
            sta         src
            lda         srv_argp + 3
            sta         src + 1
            ldy         #0
            jsr         @number
            bcs         @bad
            sta         num + 2
            lda         (src),Y
            cmp         #'-'
            bne         @bad
            iny
            jsr         @number
            bcs         @bad
            sta         num + 3
            lda         (src),Y                             ; (The word's end)
            bne         @bad
            lda         num + 3
            cmp         num + 2
            bcc         @bad
            ldx         dk                                  ; (r: modules)
            cpx         #DISK_S
            beq         :+
            cmp         #16
            bcs         @bad
:
            clc
            rts

@bad:
            sec
            rts

@number:                                                    ; .A = the byte at (src),Y on, .Y past it; or C = 1
            stz         num
            stz         num + 1                             ; (num + 1: its digits)
            lda         (src),Y
            cmp         #'$'
            beq         @hex
@dec:
            lda         (src),Y
            sec
            sbc         #'0'
            cmp         #10
            bcs         @end
            pha
            lda         num                                 ; * 10 (past 255: bad)
            cmp         #26
            bcs         @big
            asl
            asl
            adc         num
            asl
            sta         num
            pla
            clc
            adc         num
            bcs         @bad
            sta         num
            inc         num + 1
            iny
            bra         @dec

@hex:
            iny
:
            lda         (src),Y
            cmp         #'0'
            bcc         @end
            cmp         #'9' + 1
            bcc         @d09
            ora         #$20                                ; (a-f, A-F)
            cmp         #'a'
            bcc         @end
            cmp         #'f' + 1
            bcs         @end
            sbc         #'a' - 10 - 1                       ; (C = 0: - 'a' + 10)
            bra         @nib

@d09:
            sbc         #'0' - 1                            ; (C = 0: - '0')
@nib:
            ldx         num + 1                             ; (Two digits at most)
            cpx         #2
            bcs         @bad
            asl         num
            asl         num
            asl         num
            asl         num
            ora         num
            sta         num
            inc         num + 1
            iny
            bra         :-

@big:
            pla
            sec
            rts

@end:
            lda         num + 1                             ; (A digit at least)
            beq         @bad
            lda         num
            clc
            rts

; src = the ctl command's first word after it
arg_word:
            lda         srv_argp
            sta         src
            lda         srv_argp + 1
            sta         src + 1
            rts

; The ctl file's text: the disk ("sdhc 7580 MB 15523840 blocks"), or "none".  A card not started yet is started
gen_disk:
            lda         z:srv_id
            sta         dk
            jsr         disk_start
            ldx         dk
            lda         d_state,X
            bne         :+
            lda         #<s_none
            ldx         #>s_none
            jsr         srv_tputs
            clc
            rts
:
            pha
            asl
            tax
            lda         s_kinds,X                           ; "sdhc " ...
            pha
            lda         s_kinds + 1,X
            tax
            pla
            jsr         srv_tputs
            pla
            ldx         #11                                 ; Its size: in MB (blocks >> 11) ...
            ldy         #0
            cmp         #DS_RAM
            bcc         :+
            ldx         #1                                  ;   a RAM disk's in KB (blocks >> 1)
            ldy         #2
:
            phy
            jsr         get_blocks
:
            lsr         num + 3
            ror         num + 2
            ror         num + 1
            ror         num
            dex
            bne         :-
            jsr         put_dec32
            ply
            lda         s_units,Y
            ldx         s_units + 1,Y
            jsr         srv_tputs
            jsr         get_blocks                          ; ... and in blocks
            jsr         put_dec32
            lda         #<s_blocks
            ldx         #>s_blocks
            jsr         srv_tputs
            FAR2        hfs_ctl_lines                       ; (Its HydraFS, if it has one)
            ldx         dk                                  ; A RAM disk's memory: its banks ("banks $10-$13"),
            lda         d_state,X                           ;   or its segment ("segment 0")
            cmp         #DS_RAM
            beq         @banks
            cmp         #DS_SRAM
            bne         @end
            lda         #<s_segment
            ldx         #>s_segment
            jsr         srv_tputs
            ldx         dk
            lda         d_aux,X
            ldx         #0
            jsr         srv_tputdec
            bra         @nl

@banks:
            lda         #<s_banks
            ldx         #>s_banks
            jsr         srv_tputs
            ldx         dk
            lda         d_aux,X                             ; (The first)
            pha
            jsr         srv_tputhex
            lda         #<s_to
            ldx         #>s_to
            jsr         srv_tputs
            jsr         get_blocks                          ; (The last: the first + its blocks / 16 - 1)
            ldx         #4
:
            lsr         num + 1
            ror         num
            dex
            bne         :-
            pla
            clc
            adc         num
            dec         a
            jsr         srv_tputhex
@nl:
            lda         #LF
            jsr         srv_tputc
@end:
            clc
            rts

; num = disk dk's blocks.  Keeps .X, .Y
get_blocks:
            phx
            phy
            lda         dk
            asl
            asl
            tax
            ldy         #0
:
            lda         d_blocks,X
            sta         num,Y
            inx
            iny
            cpy         #4
            bne         :-
            ply
            plx
            rts

; num (32 bits) in decimal, into the text (num is used up)
put_dec32:
            lda         #0                                  ; (A 0 on the stack: the digits' end)
            pha
@digit:
            ldx         #32                                 ; num / 10: the remainder in .A
            lda         #0
:
            asl         num
            rol         num + 1
            rol         num + 2
            rol         num + 3
            rol         a
            cmp         #10
            bcc         :+
            sbc         #10
            inc         num
:
            dex
            bne         :--
            clc
            adc         #'0'
            pha
            lda         num
            ora         num + 1
            ora         num + 2
            ora         num + 3
            bne         @digit
:
            pla
            beq         :+
            jsr         srv_tputc
            bra         :-
:
            rts

.rodata
; ****************************************************************************
; The devices
SRV_TREES:
            .byte       'S'
            .word       tree_spi
            .byte       'd'
            .word       tree_sd
            .byte       'f'
            .word       tree_fs
            .byte       0

tree_spi:
            SRV_ENTRY   s_slash,   $FF, SK_DYN,  h_devs,    SM_READ,            1     ; 0
            SRV_ENTRY   s_slash,   SE_TEMPLATE, SK_DIR, 0,  SM_READ,            0     ; 1 (Each device's directory)
            SRV_ENTRY   s_data,    1,   SK_DATA, h_data,    SM_READ | SM_WRITE, 0     ; 2
            SRV_ENTRY   s_ctl,     1,   SK_CTL,  spi_cmds,  SM_READ | SM_WRITE, 4     ; 3 (reads as 4)
            SRV_ENTRY   s_ctl,     $FE, SK_TEXT, gen_mode,  SM_READ,            0     ; 4 (its state: in no directory)
            .word       0
tree_sd:
            SRV_ENTRY   s_slash,   $FF, SK_DYN,  h_disks,   SM_READ,            1     ; 0
            SRV_ENTRY   s_slash,   SE_TEMPLATE, SK_DIR, 0,  SM_READ,            0     ; 1 (Each disk's directory)
            SRV_ENTRY   s_data,    1,   SK_DATA, h_disk,    SM_READ | SM_WRITE, 0     ; 2
            SRV_ENTRY   s_ctl,     1,   SK_CTL,  disk_cmds, SM_READ | SM_WRITE, 4     ; 3 (reads as 4)
            SRV_ENTRY   s_ctl,     $FE, SK_TEXT, gen_disk,  SM_READ,            0     ; 4 (its state: in no directory)
            .word       0
tree_fs:
            SRV_ENTRY   s_slash,   $FF, SK_RAW,  h_fs,      SM_READ | SM_WRITE, 0     ; (All of it: hfs.s)
            .word       0
spi_cmds:
            .word       s_mode, c_mode
            .word       0
disk_cmds:
            .word       s_init, c_init
            .word       s_start, c_start
            .word       s_stop, c_stop
            .word       s_format, c_format
            .word       s_label, c_label
            .word       s_check, c_check
            .word       0
blk_readers: .word      0, sd_read, sd_read, rom_read, ram_read, ram_read       ; (By state: DS_*)
blk_writers: .word      0, sd_write, sd_write, rom_write, ram_write, ram_write
s_kinds:    .word       0, s_sdsc, s_sdhc, s_rom, s_ram, s_sram
s_units:    .word       s_mb, s_kb
s_slash:    .byte       "/", 0
s_data:     .byte       "data", 0
s_ctl:      .byte       "ctl", 0
s_mode:     .byte       "mode", 0
s_init:     .byte       "init", 0
s_start:    .byte       "start", 0
s_stop:     .byte       "stop", 0
s_format:   .byte       "format", 0
s_label:    .byte       "label", 0
s_check:    .byte       "check", 0
s_none:     .byte       "none", LF, 0
s_sdsc:     .byte       "sdsc ", 0
s_sdhc:     .byte       "sdhc ", 0
s_rom:      .byte       "rom ", 0
s_ram:      .byte       "ram ", 0
s_sram:     .byte       "sram ", 0
s_mb:       .byte       " MB ", 0
s_kb:       .byte       " KB ", 0
s_blocks:   .byte       " blocks", LF, 0
s_banks:    .byte       "banks $", 0
s_to:       .byte       "-$", 0
s_segment:  .byte       "segment ", 0
s_disks:    .byte       "0123456789abcdefxrs"                ; (By disk: its name)

.include "srvlib.s"
