.debuginfo

; ****************************************************************************
; Making a HydraFS on a card, and its label (BIOS ROM page 3, in the storage task, inside `.scope PAGE3`):
; "format" and "label" on /dev/sd/N/ctl (from SD_CTL_FS).  The rest of HydraFS is on page 6: its routines
; used here are reached through page 3's gates (drivers/page3.s).

.segment "STORAGE_P3"

HFS_S_MAGIC:    .byte   "HYDRAFS1"

; ****************************************************************************
; "format [-f] [-s size] [label]" and "label <text>" on /dev/sd/N/ctl (from SD_CTL_FS in the SD server: the
; card is SD_DEV, and the text after the word is in HFS_STAT, zero-padded).  OUT: C = 0; or C = 1, .A = error

; Make an empty HydraFS on the card: block 0 cleared, then the superblock, with an empty root directory.
; Any HydraFS file open on the card is let go of.
;   A quick format (the default) writes no free map: the volume is version 2, and its map's blocks are
; written as the space is first used (HFS_MAP_WRITTEN, page 6).  "-f" (a full format) writes the whole map
; now (all free), for a version 1 volume, showing its progress: a 32 GB card takes 2050 block writes, about
; a minute and a half at 3.58 MHz; 256 GB, a quarter of an hour.  "-s size" makes the volume that size
; (megabytes; or gigabytes, with a G after it: -s 4G), if the card is bigger; the rest isn't used.
HFS_FORMAT:
            jsr         HFS_FMT_OPTIONS                     ; HFS_FMT_OPT, HFS_FMT_BLKS, and the label
            bcc         :+
            jmp         @done
:
            lda         SD_DEV
            sta         HFS_CARD
            tax
            lda         SD_CARD_STATE,X
            bne         :+
            jsr         SD_START                            ; (Not started yet)
            bcc         @far1
            jmp         @done
@far1:
:
            jsr         HFS_FORGET
            jsr         SD_CARD_SIZE                        ; SD_LBA = its blocks
            lda         HFS_FMT_BLKS                        ; A size asked for, smaller than the card: that
            ora         HFS_FMT_BLKS + 1
            ora         HFS_FMT_BLKS + 2
            ora         HFS_FMT_BLKS + 3
            beq         @sized                              ; (None: the whole card)
            lda         HFS_FMT_BLKS
            cmp         SD_LBA
            lda         HFS_FMT_BLKS + 1
            sbc         SD_LBA + 1
            lda         HFS_FMT_BLKS + 2
            sbc         SD_LBA + 2
            lda         HFS_FMT_BLKS + 3
            sbc         SD_LBA + 3
            bcs         @sized
            ldx         #3
:
            lda         HFS_FMT_BLKS,X
            sta         SD_LBA,X
            dex
            bpl         :-

@sized:
            sec                                             ; HFS_T4 = the clusters the map covers:
            lda         SD_LBA                              ;   (blocks - 1) / 8
            sbc         #1
            sta         HFS_T4
            lda         SD_LBA + 1
            sbc         #0
            sta         HFS_T4 + 1
            lda         SD_LBA + 2
            sbc         #0
            sta         HFS_T4 + 2
            lda         SD_LBA + 3
            sbc         #0
            sta         HFS_T4 + 3
            ldx         #HFS_T4 - HFS_C
            ldy         #HFS_CSHIFT
            jsr         HFS_SHR
            clc                                             ; HFS_N4 = the map's blocks: (that + 4095) / 4096
            lda         HFS_T4
            adc         #<4095
            sta         HFS_N4
            lda         HFS_T4 + 1
            adc         #>4095
            sta         HFS_N4 + 1
            lda         HFS_T4 + 2
            adc         #0
            sta         HFS_N4 + 2
            lda         HFS_T4 + 3
            adc         #0
            sta         HFS_N4 + 3
            ldx         #HFS_N4 - HFS_C
            ldy         #12
            jsr         HFS_SHR
            clc                                             ; HFS_D = the data area's first block: after them
            lda         HFS_N4
            adc         #1
            sta         HFS_D
            lda         HFS_N4 + 1
            adc         #0
            sta         HFS_D + 1
            lda         HFS_N4 + 2
            adc         #0
            sta         HFS_D + 2
            lda         HFS_N4 + 3
            adc         #0
            sta         HFS_D + 3
            sec                                             ; HFS_C = the data area's clusters:
            lda         SD_LBA                              ;   (blocks - HFS_D) / 8
            sbc         HFS_D
            sta         HFS_C
            lda         SD_LBA + 1
            sbc         HFS_D + 1
            sta         HFS_C + 1
            lda         SD_LBA + 2
            sbc         HFS_D + 2
            sta         HFS_C + 2
            lda         SD_LBA + 3
            sbc         HFS_D + 3
            sta         HFS_C + 3
            bcs         @far4
            jmp         @too_small
@far4:
            ldx         #0
            ldy         #HFS_CSHIFT
            jsr         HFS_SHR
            lda         HFS_C + 3                           ; (2 at least)
            ora         HFS_C + 2
            ora         HFS_C + 1
            bne         :+
            lda         HFS_C
            cmp         #2
            bcc         @too_small
:
            stz         SD_LBA                              ; Block 0 cleared first: no half-made HydraFS
            stz         SD_LBA + 1
            stz         SD_LBA + 2
            stz         SD_LBA + 3
            jsr         HFS_META_NEW
            bcs         @done
            lda         HFS_FMT_OPT                         ; A quick format: no free map written
            and         #HFS_FMT_FULL
            beq         @super
            ldx         #3                                  ; (Its progress: a step per map block)
:
            lda         HFS_N4,X
            sta         HFS_T4,X
            dex
            bpl         :-
            jsr         HFS_PG_START

@map:                                                       ; The free map, from block 1: all free
            lda         HFS_N4
            ora         HFS_N4 + 1
            ora         HFS_N4 + 2
            ora         HFS_N4 + 3
            beq         @super
            inc         SD_LBA
            bne         :+
            inc         SD_LBA + 1
            bne         :+
            inc         SD_LBA + 2
            bne         :+
            inc         SD_LBA + 3
:
            jsr         HFS_META_NEW                        ; (It writes the one before)
            bcs         @done
            jsr         HFS_PG_TICK
            sec
            lda         HFS_N4
            sbc         #1
            sta         HFS_N4
            lda         HFS_N4 + 1
            sbc         #0
            sta         HFS_N4 + 1
            lda         HFS_N4 + 2
            sbc         #0
            sta         HFS_N4 + 2
            lda         HFS_N4 + 3
            sbc         #0
            sta         HFS_N4 + 3
            bra         @map

@too_small:
            lda         #ERR_IO_MEDIA
            sec
            bra         @done

@super:
            jsr         HFS_FORMAT_SB
            bcs         @done
            stz         SD_CVALID                           ; (The cache may hold the card's old blocks)
            lda         #0
            clc

@done:
            jsr         HFS_PG_END                          ; (Keeps .A and C)
            jmp         HFS_FINISH

; The new superblock: the numbers HFS_FORMAT worked out (HFS_C clusters, HFS_N4 = the map's blocks, which
; it has counted down to 0 again, so it's worked out again; HFS_D = the data area), an empty root, the label.
HFS_FORMAT_SB:
            sec                                             ; (The map's blocks: the data area's first - 1)
            lda         HFS_D
            sbc         #1
            sta         HFS_N4
            lda         HFS_D + 1
            sbc         #0
            sta         HFS_N4 + 1
            lda         HFS_D + 2
            sbc         #0
            sta         HFS_N4 + 2
            lda         HFS_D + 3
            sbc         #0
            sta         HFS_N4 + 3
            stz         SD_LBA
            stz         SD_LBA + 1
            stz         SD_LBA + 2
            stz         SD_LBA + 3
            jsr         HFS_META_NEW                        ; (It writes the map's last block)
            bcs         @done
            stz         HFS_OFS
            stz         HFS_OFS + 1
            jsr         HFS_META_AT
            ldy         #7
:
            lda         HFS_S_MAGIC,Y
            sta         (HFS_PTR),Y
            dey
            bpl         :-
            ldy         #HFS_SB_VERSION                     ; Version 1: the whole map written (-f); 2: none
            lda         #HFS_VERSION_FULL                   ;   of it yet (HFS_SB_MAPINIT = 0)
            ldx         HFS_FMT_OPT
            bne         :+
            lda         #HFS_VERSION
:
            sta         (HFS_PTR),Y
            iny
            lda         #HFS_CSHIFT
            sta         (HFS_PTR),Y
            ldy         #HFS_SB_CLUSTERS
            ldx         #HFS_C - HFS_C
            jsr         HFS_PUT4
            ldy         #HFS_SB_MAP                         ; (The map starts at block 1)
            lda         #1
            sta         (HFS_PTR),Y
            ldy         #HFS_SB_MAPSZ
            ldx         #HFS_N4 - HFS_C
            jsr         HFS_PUT4
            ldx         #HFS_D - HFS_C                      ; (.Y = HFS_SB_DATA)
            jsr         HFS_PUT4
            ldx         #HFS_C - HFS_C                      ; (.Y = HFS_SB_FREE: all of them)
            jsr         HFS_PUT4
            ldy         #HFS_SB_NEXT_QID                    ; (The root's is 1)
            lda         #2
            sta         (HFS_PTR),Y
            ldy         #HFS_SB_STAMP
            lda         #1
            sta         (HFS_PTR),Y
            ldy         #HFS_SB_ROOT + HFS_E_NAME           ; The root: "/", a directory, qid 1, empty
            lda         #'/'
            sta         (HFS_PTR),Y
            ldy         #HFS_SB_ROOT + HFS_E_MODE
            lda         #HFS_M_DIR
            sta         (HFS_PTR),Y
            ldy         #HFS_SB_ROOT + HFS_E_QID
            lda         #1
            sta         (HFS_PTR),Y
            jsr         HFS_LABEL_PUT
            clc

@done:
            rts

.assert     HFS_SB_MAPSZ + 4 = HFS_SB_DATA && HFS_SB_DATA + 4 = HFS_SB_FREE, error, "HFS_FORMAT_SB puts them one after another"
.assert     HFS_FMT_FULL = 1, error, "HFS_FORMAT_SB: HFS_FMT_OPT is 0 or HFS_FMT_FULL"
.assert     HFS_VERSION_FULL = 1 .and HFS_VERSION = 2, error, "HFS_FORMAT_SB: a quick format is version 2"

; The options before the label in HFS_STAT: "-f" (HFS_FMT_OPT = HFS_FMT_FULL) and "-s size" (HFS_FMT_BLKS:
; megabytes, or gigabytes with a G after it; nothing, or 0: the whole card).  The label that follows is
; moved to HFS_STAT's start, and cut to 31 characters.  OUT: C = 0; or C = 1, .A = ERR_IO_BAD_REQ (an option
; this doesn't know).  Modifies: .A, .X, .Y
HFS_FMT_OPTIONS:
            stz         HFS_FMT_OPT
            ldx         #3
:
            stz         HFS_FMT_BLKS,X
            dex
            bpl         :-
            ldy         #0

@next:
            lda         HFS_STAT,Y                          ; (The text ends with a 0: SD_CTL_FS)
            cmp         #' '
            bne         :+
            iny
            bra         @next
:
            cmp         #'-'
            beq         @far3
            jmp         @label
@far3:
            lda         HFS_STAT + 1,Y
            iny
            iny
            cmp         #'f'
            bne         :+
            lda         #HFS_FMT_FULL
            sta         HFS_FMT_OPT
            bra         @next
:
            cmp         #'s'
            beq         @far2
            jmp         @bad
@far2:
:
            lda         HFS_STAT,Y                          ; (Spaces before the size)
            cmp         #' '
            bne         @digit
            iny
            bra         :-

@digit:                                                     ; HFS_FMT_BLKS = the number, * 10 for each digit
            lda         HFS_STAT,Y
            sec
            sbc         #'0'
            cmp         #10
            bcs         @unit
            pha
            ldx         #3                                  ; (* 10: * 2, kept in HFS_T4, then * 4 + that)
            asl         HFS_FMT_BLKS
            rol         HFS_FMT_BLKS + 1
            rol         HFS_FMT_BLKS + 2
            rol         HFS_FMT_BLKS + 3
:
            lda         HFS_FMT_BLKS,X
            sta         HFS_T4,X
            dex
            bpl         :-
            jsr         @double
            jsr         @double
            clc
            ldx         #0
:
            lda         HFS_FMT_BLKS,X
            adc         HFS_T4,X
            sta         HFS_FMT_BLKS,X
            inx
            txa                                             ; (Keeps C)
            eor         #4
            bne         :-
            pla                                             ; + the digit
            clc
            adc         HFS_FMT_BLKS
            sta         HFS_FMT_BLKS
            bcc         :+
            inc         HFS_FMT_BLKS + 1
            bne         :+
            inc         HFS_FMT_BLKS + 2
            bne         :+
            inc         HFS_FMT_BLKS + 3
:
            iny
            bra         @digit

@unit:                                                      ; Megabytes: * 2048 blocks; gigabytes: * 2M
            ldx         #11
            lda         HFS_STAT,Y
            and         #$DF                                ; (Upper case)
            cmp         #'G'
            bne         @shift
            iny
            ldx         #21

@shift:
            jsr         @double
            bcs         @huge
            dex
            bne         @shift
            jmp         @next

@huge:                                                      ; (Bigger than any card: the whole card)
            lda         #$FF
            sta         HFS_FMT_BLKS
            sta         HFS_FMT_BLKS + 1
            sta         HFS_FMT_BLKS + 2
            sta         HFS_FMT_BLKS + 3
            jmp         @next

@bad:
            lda         #ERR_IO_BAD_REQ
            sec
            rts

@double:                                                    ; HFS_FMT_BLKS * 2 (C = the bit shifted out)
            asl         HFS_FMT_BLKS
            rol         HFS_FMT_BLKS + 1
            rol         HFS_FMT_BLKS + 2
            rol         HFS_FMT_BLKS + 3
            rts

@label:                                                     ; The rest: the label, to HFS_STAT's start
            ldx         #0
:
            lda         HFS_STAT,Y
            sta         HFS_STAT,X
            inx
            iny
            cpy         #IO_STAT_SIZE
            bne         :-
:
            cpx         #IO_STAT_SIZE                       ; (Zeros after it)
            beq         HFS_LABEL_CUT
            stz         HFS_STAT,X
            inx
            bra         :-

; A label in HFS_STAT: cut to 31 characters (HFS_LABEL_PUT puts 32 bytes).  OUT: C = 0
HFS_LABEL_CUT:
            stz         HFS_STAT + HFS_NAME_MAX
            clc
            rts

; 4 bytes, from HFS_C + .X on, to (HFS_PTR),Y on (.X a multiple of 4).  Modifies: .A, .X, .Y (4 on each)
HFS_PUT4:
            lda         HFS_C,X
            sta         (HFS_PTR),Y
            inx
            iny
            txa
            and         #3
            bne         HFS_PUT4
            rts

.assert     (HFS_N4 - HFS_C) & 3 = 0 && (HFS_D - HFS_C) & 3 = 0 && (HFS_T4 - HFS_C) & 3 = 0, error, "HFS_PUT4 and HFS_SHR: 4-byte numbers 4 apart"

; The label (HFS_STAT, 32 bytes) into the superblock in the metadata buffer.  Modifies: .A, .Y
HFS_LABEL_PUT:
            lda         #HFS_SB_LABEL
            sta         HFS_OFS
            stz         HFS_OFS + 1
            jsr         HFS_META_AT
            ldy         #HFS_NAME_MAX
:
            lda         HFS_STAT,Y
            sta         (HFS_PTR),Y
            dey
            bpl         :-
            rts

; Set a card's HydraFS label
HFS_LABEL:
            jsr         HFS_LABEL_CUT
            lda         SD_DEV
            sta         HFS_CARD
            jsr         HFS_VOLUME
            bcs         @done
            jsr         HFS_SB_GET
            bcs         @done
            jsr         HFS_LABEL_PUT
            jsr         HFS_META_CHANGED
            lda         #0
            clc

@done:
            jmp         HFS_FINISH
