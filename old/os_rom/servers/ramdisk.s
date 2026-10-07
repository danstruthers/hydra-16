.debuginfo

; ****************************************************************************
; The RAM disks, started and stopped (BIOS ROM page 3, in the storage task; included inside `.scope PAGE3`, see
; all.s): "start" and "stop" on /dev/sd/r/ctl and /dev/sd/s/ctl (from SD_CTL_FS), their lines in those files, and
; both started at boot.  Their blocks are read and written in sd.s (SD_RAM_READ, SD_RAM_WRITE); the plan is
; docs/plans/DISKS.md.
;   start SIZE [FROM-TO]    take SIZE (8K banks; or K or M after it: 256K, 1M), from the RAM modules FROM-TO
;                           (the RAM disk, r: the storage task's own banks on them) or the shared bank IDs
;                           FROM-TO (the shared one, s: $20-$9F), or from anywhere; then an empty HydraFS on it
;                           (a quick format, labelled RAM or SRAM).  Numbers are decimal, or hex after a '$'
;   stop                    give its banks back (its files are lost).  Not while a file on it is open

.segment "STORAGE_P3"

; At boot (STORAGE_INIT3): both RAM disks, their sizes from io.inc, from anywhere; on a machine with less memory,
; half that, and so on.  (One that can't start at all isn't there: its ctl file says "none".)
RAMD_BOOT:
            lda         #DISK_RAM
            ldx         #RAMD_BANKS
            jsr         @start
            lda         #DISK_SRAM
            ldx         #SRAMD_BANKS

@start:
            sta         SD_DEV
            stx         SD_POS

@try:
            jsr         RAMD_ANYWHERE
            jsr         RAMD_START
            bcc         @done
            cmp         #ERR_IO_FULL                        ; (No room: less)
            bne         @done
            lsr         SD_POS
            bne         @try

@done:
            rts

; "start" (SD_TMP = 4) and "stop" (5) on a ctl file, the text after the word in HFS_STAT (SD_CTL_FS).
; OUT: C = 0; or C = 1, .A = error (ERR_IO_BAD_REQ: not a RAM disk, or not a size or range)
RAMD_CTL:
            lda         SD_DEV
            cmp         #DISK_RAM
            bcc         @bad
            cmp         #DISK_SRAM + 1
            bcs         @bad
            lda         SD_TMP
            cmp         #5
            bne         :+
            jmp         RAMD_STOP
:
            ldx         #0                                  ; The size: SD_POS = it in banks
            jsr         RAMD_NUMBER
            bcs         @bad
            lda         HFS_STAT,X                          ; K (kilobytes) or M (megabytes) after it?
            ora         #$20
            cmp         #'k'
            beq         @kilo
            cmp         #'m'
            bne         @banks
            inx                                             ; Megabytes: * 128
            ldy         #7
:
            asl         SD_LBA
            rol         SD_LBA + 1
            bcs         @bad
            dey
            bne         :-
            bra         @banks

@bad:
            lda         #ERR_IO_BAD_REQ
            sec
            rts

@kilo:                                                      ; Kilobytes: (K + 7) / 8
            inx
            lda         SD_LBA
            clc
            adc         #7
            sta         SD_LBA
            bcc         :+
            inc         SD_LBA + 1
            beq         @bad
:
            ldy         #3
:
            lsr         SD_LBA + 1
            ror         SD_LBA
            dey
            bne         :-

@banks:
            lda         SD_LBA + 1                          ; 1-255 banks
            bne         @bad
            lda         SD_LBA
            beq         @bad
            sta         SD_POS
            jsr         RAMD_ANYWHERE                       ; Where from: anywhere ...
            jsr         RAMD_SPACES
            beq         @start
            jsr         RAMD_NUMBER                         ; ... or FROM-TO
            bcs         @wrong
            jsr         RAMD_BYTE
            bcs         @wrong
            sta         SD_POS + 1
            lda         HFS_STAT,X
            cmp         #'-'
            bne         @wrong
            inx
            jsr         RAMD_NUMBER
            bcs         @wrong
            jsr         RAMD_BYTE
            bcs         @wrong
            sta         SD_POS + 2
            cmp         SD_POS + 1
            bcc         @wrong                              ; (TO below FROM)
            lda         SD_DEV
            cmp         #DISK_SRAM
            beq         @end                                ; (Shared bank IDs, as they are)
            lda         SD_POS + 2                          ; Modules: their banks, FROM * 16 to TO * 16 + 15
            cmp         #NUM_RAM_MODULES
            bcs         @wrong
            asl
            asl
            asl
            asl
            ora         #$0F
            sta         SD_POS + 2
            lda         SD_POS + 1
            asl
            asl
            asl
            asl
            sta         SD_POS + 1

@end:
            jsr         RAMD_SPACES                         ; Then nothing more
            bne         @wrong

@start:
            jmp         RAMD_START

@wrong:
            lda         #ERR_IO_BAD_REQ
            sec
            rts

; SD_POS + 1 and + 2: the banks RAM disk SD_DEV takes from when it isn't told (the lowest, the highest): any of
; the storage task's, or the lower half of the shared ones.  (SH_ALLOC takes from the top down, with IRQs off,
; and looks past every bank in use above a free one: the RAM disk's down there keeps that short.)  Modifies: .A
RAMD_ANYWHERE:
            stz         SD_POS + 1                          ; (SH_BANK_ALLOC keeps above the system's IDs)
            lda         SD_DEV
            cmp         #DISK_SRAM
            lda         #RAMD_SHARED_TOP
            bcs         :+
            lda         #::MMU_BANK_TOP
:
            sta         SD_POS + 2
            rts

; Spaces in HFS_STAT from .X skipped.  OUT: .A = the next character, Z = 1 if it's the text's end (its 0)
RAMD_SPACES:
            lda         HFS_STAT,X
            cmp         #' '
            bne         :+
            inx
            bra         RAMD_SPACES
:
            cmp         #0
            rts

; SD_LBA's low byte, if its high byte is 0.  OUT: C = 0, .A = it; or C = 1
RAMD_BYTE:
            lda         SD_LBA + 1
            cmp         #1
            lda         SD_LBA
            rts

; A number in HFS_STAT from .X (spaces before it skipped): decimal, or hex after a '$'.
; OUT: C = 0, SD_LBA = it (16 bits), .X past it; or C = 1: no digits, or too big.  Modifies: .A, .Y, SD_N, SD_ARG
RAMD_NUMBER:
            stz         SD_LBA
            stz         SD_LBA + 1
            stz         SD_N + 1                            ; (Digits seen)
            ldy         #10
            jsr         RAMD_SPACES
            cmp         #'$'
            bne         :+
            ldy         #16
            inx
:
            sty         SD_N                                ; The base

@digit:
            lda         HFS_STAT,X
            jsr         RAMD_DIGIT
            bcs         @end
            sta         SD_ARG + 2                          ; SD_LBA = SD_LBA * the base + the digit
            lda         SD_LBA
            sta         SD_ARG
            lda         SD_LBA + 1
            sta         SD_ARG + 1
            lda         SD_ARG + 2
            sta         SD_LBA
            stz         SD_LBA + 1
            ldy         SD_N

@times:
            clc
            lda         SD_LBA
            adc         SD_ARG
            sta         SD_LBA
            lda         SD_LBA + 1
            adc         SD_ARG + 1
            sta         SD_LBA + 1
            bcs         @over
            dey
            bne         @times
            inc         SD_N + 1
            inx
            bra         @digit

@end:
            lda         SD_N + 1
            beq         @over                               ; (No digits)
            clc
            rts

@over:
            sec
            rts

; A character's value as a digit in base SD_N (0-9, a-f, A-F).  OUT: C = 0, .A = it; or C = 1: not one
RAMD_DIGIT:
            cmp         #'A'
            bcc         @decimal
            ora         #$20                                ; (A-F as a-f)
            sbc         #'a' - 10                           ; (C = 1 from the cmp)
            cmp         #10
            bcc         @not                                ; (Below 'a')
            bra         @base

@decimal:
            sec
            sbc         #'0'
            cmp         #10
            bcs         @not

@base:
            cmp         SD_N
            rts

@not:
            sec
            rts

; Start RAM disk SD_DEV: SD_POS banks, from SD_POS + 1 to SD_POS + 2 (banks, or shared bank IDs), then an empty
; HydraFS on it.  OUT: C = 0; or C = 1, .A = error: ERR_IO_BUSY (it's started: stop it first),
; ERR_IO_FULL (no run of free banks that long there), or the format's
RAMD_START:
            ldx         SD_DEV
            lda         SD_CARD_STATE,X
            beq         :+
            lda         #ERR_IO_BUSY
            sec
            rts
:
            lda         SD_POS
            ldx         SD_POS + 2
            ldy         SD_POS + 1
            jsr         RAMD_TAKE                           ; .A = the first bank (or ID)
            bcc         :+
            lda         #ERR_IO_FULL                        ; (No room: the memory manager's error, as IO's)
            rts
:
            ldx         SD_DEV
            sta         RAMD_FIRST,X
            txa                                             ; Its state ...
            sec
            sbc         #DISK_RAM - SD_STATE_RAM
            sta         SD_CARD_STATE,X
            txa                                             ; ... and blocks: 16 a bank
            asl
            asl
            tax
            lda         SD_POS
            asl
            asl
            asl
            asl
            sta         SD_CARD_BLOCKS,X
            lda         SD_POS
            lsr
            lsr
            lsr
            lsr
            sta         SD_CARD_BLOCKS + 1,X
            stz         SD_CARD_BLOCKS + 2,X
            stz         SD_CARD_BLOCKS + 3,X
            jsr         RAMD_BLOCK0                         ; Block 0 cleared: no partition table
            bcs         @fail
            ldx         #IO_STAT_SIZE                       ; The label: RAM or SRAM
:
            stz         HFS_STAT - 1,X
            dex
            bne         :-
            ldy         #RAMD_S_RAM - RAMD_LABELS
            lda         SD_DEV
            cmp         #DISK_SRAM
            bne         :+
            ldy         #RAMD_S_SRAM - RAMD_LABELS
:
            lda         RAMD_LABELS,Y
            sta         HFS_STAT,X
            beq         :+
            iny
            inx
            bra         :-
:
            jsr         HFS_FORMAT                          ; A quick one: no free map written yet
            bcc         @done

@fail:
            pha
            jsr         RAMD_GIVE_BACK
            pla
            sec

@done:
            rts

RAMD_LABELS:
RAMD_S_RAM:     .byte   "RAM", 0
RAMD_S_SRAM:    .byte   "SRAM", 0

; Write zeros to block 0 of RAM disk SD_DEV (through the block cache), so the RAM there from before, or from
; power-up, can't look like a partition table to the format.  OUT: C = 0; or C = 1, .A = error
RAMD_BLOCK0:
            lda         SD_CACHE
            sta         SD_BUF
            lda         SD_CACHE + 1
            sta         SD_BUF + 1
            stz         SD_CVALID                           ; (The cache's block is gone)
            lda         #0
            tay
:
            sta         (SD_BUF),Y
            iny
            bne         :-
            inc         SD_BUF + 1
:
            sta         (SD_BUF),Y
            iny
            bne         :-
            dec         SD_BUF + 1
            ldx         #3
:
            stz         SD_LBA,X
            dex
            bpl         :-
            jmp         SD_WRITE_BLOCK

; Take .A banks for RAM disk SD_DEV from .Y-.X: the storage task's own (the RAM disk) or shared ones.
; OUT: C = 0, .A = the first; or C = 1, .A = error
RAMD_TAKE:
            pha
            lda         SD_DEV
            cmp         #DISK_SRAM
            pla
            bcs         :+
            jmp         MM_BANK_ALLOC_IN
:
            jmp         SH_BANK_ALLOC

; RAM disk SD_DEV stopped: its state, and its banks given back
RAMD_GIVE_BACK:
            ldx         SD_DEV
            stz         SD_CARD_STATE,X
            lda         RAMD_FIRST,X
            cpx         #DISK_SRAM
            bcs         :+
            jmp         MM_BANK_FREE
:
            jmp         SH_BANK_FREE

; "stop": RAM disk SD_DEV's banks given back, and its files with them.  OUT: C = 0; or C = 1, .A =
; ERR_IO_NOT_READY (it isn't started) or ERR_IO_BUSY (a file on it is open)
RAMD_STOP:
            ldx         SD_DEV
            lda         SD_CARD_STATE,X
            bne         :+
            lda         #ERR_IO_NOT_READY
            sec
            rts
:
            ldx         #(HFS_MAX_OPEN - 1) * HFS_FHDR_SIZE

@open:
            lda         HFS_FHDR + HFS_H_CARD,X
            cmp         SD_DEV
            beq         @busy
            txa
            sec
            sbc         #HFS_FHDR_SIZE
            tax
            bpl         @open
            stz         SD_CVALID
            jsr         HFS_FORGET
            jsr         RAMD_GIVE_BACK
            lda         #0
            clc
            rts

@busy:
            lda         #ERR_IO_BUSY
            sec
            rts

; A RAM disk's line in its ctl file: "banks $C0-$DF" (shared bank IDs for the shared one), after its size.
; Nothing for another disk.  Modifies: .A, .X, .Y
RAMD_CTL_LINE:
            ldx         SD_DEV
            lda         SD_CARD_STATE,X
            cmp         #SD_STATE_RAM
            bcc         @done
            ldx         #0
:
            lda         RAMD_S_BANKS,X
            beq         :+
            jsr         SD_PUT
            inx
            bra         :-
:
            ldx         SD_DEV
            lda         RAMD_FIRST,X
            jsr         RAMD_PUT_HEX
            lda         #'-'
            jsr         SD_PUT
            lda         #'$'
            jsr         SD_PUT
            txa                                             ; The last: the first + blocks / 16 - 1
            asl
            asl
            tax
            lda         SD_CARD_BLOCKS + 1,X
            asl
            asl
            asl
            asl
            sta         SD_TMP
            lda         SD_CARD_BLOCKS,X
            lsr
            lsr
            lsr
            lsr
            ora         SD_TMP
            ldx         SD_DEV
            clc
            adc         RAMD_FIRST,X
            dec
            jsr         RAMD_PUT_HEX
            lda         #ASCII_CR
            jsr         SD_PUT
            lda         #ASCII_LF
            jmp         SD_PUT

@done:
            rts

RAMD_S_BANKS:   .byte   "banks $", 0

; .A as 2 hex digits in the ctl file's text.  Preserves .X
RAMD_PUT_HEX:
            pha
            lsr
            lsr
            lsr
            lsr
            jsr         @digit
            pla
            and         #$0F

@digit:
            cmp         #10
            bcc         :+
            adc         #'A' - '0' - 10 - 1                 ; (C = 1)
:
            adc         #'0'
            jmp         SD_PUT
