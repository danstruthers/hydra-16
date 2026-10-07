; ****************************************************************************
; Making a HydraFS on a disk, and its label (in hfs.s: the storage driver's second bank), ported from the old OS's
; fs/hfs_format.s: "format" and "label" on #d/N/ctl (storage.s's c_format, c_label: hfs_format, hfs_label), and
; a RAM disk's quick format as it starts (hfs_format_ram).

.segment "CODE2"

HFS_S_MAGIC:    .byte   "HYDRAFS1"

; ****************************************************************************
; "format [-f] [-p] [-s size] [label]" and "label <text>" on #d/N/ctl (the disk is SD_DEV; the text after the
; word goes in HFS_STAT, zero-padded: HFS_CTL_TEXT).  OUT: C = 0; or C = 1, .A = error

hfs_format:
            lda         SD_DEV                              ; (Its names in the walk cache forgotten: srv.s)
            jsr         HFS_WC_FORGET
            jsr         HFS_CTL_TEXT
            jmp         HFS_FORMAT

hfs_label:
            lda         SD_DEV
            jsr         HFS_WC_FORGET
            jsr         HFS_CTL_TEXT
            jmp         HFS_LABEL

; RAM disk SD_DEV, just started (storage.s's c_start): a quick format, labelled RAM or SRAM
hfs_format_ram:
            lda         SD_DEV
            jsr         HFS_WC_FORGET
            ldx         #SR_SIZE - 1
:
            stz         HFS_STAT,X
            dex
            bpl         :-
            ldx         #0                                  ; (r: "RAM"; s: "SRAM")
            lda         SD_DEV
            cmp         #DISK_S
            bne         :+
            lda         #'S'
            sta         HFS_STAT
            inx
:
            ldy         #0
:
            lda         HFS_S_RAM,Y
            sta         HFS_STAT,X
            inx
            iny
            cpy         #3
            bne         :-
            stz         HFS_FMT_OPT                         ; (Quick, the whole disk)
            ldx         #3
:
            stz         HFS_FMT_BLKS,X
            dex
            bpl         :-
            bra         HFS_FORMAT_GO

HFS_S_RAM:  .byte   "RAM"

; The ctl command's text after its word (srvlib's words joined again, with spaces), into HFS_STAT, zero-padded
; (SR_SIZE bytes: 63 characters at most), its trailing spaces off.  Modifies: .A, .X, .Y
HFS_CTL_TEXT:
            ldx         #SR_SIZE - 1
:
            stz         HFS_STAT,X
            dex
            bpl         :-
            lda         srv_argn
            beq         @done
            lda         srv_argp                            ; From the first word's start (its place in
            sec                                             ;   srv_ctl) ...
            sbc         #<srv_ctl
            tay
            ldx         #0
@char:
            cpy         TASK_INBOX + RQ_COUNT               ; ... to the text's end
            bcs         @end
            lda         srv_ctl,Y
            cmp         #' ' + 1
            bcs         :+
            lda         #' '                                ; (A word's end: a space)
:
            sta         HFS_STAT,X
            iny
            inx
            cpx         #SR_SIZE - 1
            bcc         @char
@end:
            dex                                             ; Its trailing spaces off
            bmi         @done
            lda         HFS_STAT,X
            cmp         #' '
            bne         @done
            stz         HFS_STAT,X
            bra         @end

@done:
            rts

; Make an empty HydraFS on the card: block 0 cleared, then the superblock, with an empty root directory.
; Any HydraFS file open on the card is let go of.  On a card with a HydraFS partition, it goes in the
; partition (the others, and the partition table, are left alone); "-p" makes one on a card without one,
; after its other partitions (HFS_PART_SETUP); otherwise it's the whole card.
;   A quick format (the default) writes no free map: the volume is version 2, and its map's blocks are
; written as the space is first used (HFS_MAP_WRITTEN, write.s).  "-f" (a full format) writes the whole map
; now (all free), for a version 1 volume, showing its progress: a 32 GB card takes 2050 block writes, about
; a minute and a half at 3.58 MHz; 256 GB, a quarter of an hour.  "-s size" makes the volume that size
; (megabytes; or gigabytes, with a G after it: -s 4G), if the card is bigger; the rest isn't used.
HFS_FORMAT:
            jsr         HFS_FMT_OPTIONS                     ; HFS_FMT_OPT, HFS_FMT_BLKS, and the label
            bcc         HFS_FORMAT_GO
            rts

HFS_FORMAT_GO:
            lda         #HFS_CSHIFT                         ; Its clusters: 4 KB on a card, 1 KB on a RAM disk
            ldx         SD_DEV
            cpx         #DISK_R
            beq         :+
            cpx         #DISK_S
            bne         :++
:
            lda         #HFS_CSHIFT_RAM
:
            jsr         HFS_SHIFT_SET
            lda         SD_DEV
            sta         HFS_CARD
            jsr         HFS_RO_DISK                         ; (Not the ROM disk)
            bcc         :+
            jmp         @done
:
            FAR1        disk_start                          ; (A card not started yet: now)
            bcc         :+
            jmp         @done
:
            jsr         hfs_forget
            jsr         HFS_PART_SETUP                      ; Where it goes (HFS_V_BASE), SD_LBA = its blocks
            bcc         :+
            jmp         @done
:
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
            lda         SD_LBA                              ;   (blocks - 1) / a cluster's blocks
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
            ldy         HFS_SHIFT
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
            lda         SD_LBA                              ;   (blocks - HFS_D) / a cluster's blocks
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
            ldy         HFS_SHIFT
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
            lda         #E_MEDIA
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
            lda         #HFS_FMT_FULL                       ;   of it yet (HFS_SB_MAPINIT = 0)
            and         HFS_FMT_OPT
            bne         :+
            lda         #HFS_VERSION
:
            sta         (HFS_PTR),Y
            iny
            lda         HFS_SHIFT
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
.assert     HFS_FMT_FULL = HFS_VERSION_FULL, error, "HFS_FORMAT_SB: HFS_FMT_FULL is a full format's version"
.assert     HFS_VERSION_FULL = 1 .and HFS_VERSION = 2, error, "HFS_FORMAT_SB: a quick format is version 2"

; The options before the label in HFS_STAT: "-f" and "-p" (HFS_FMT_OPT: HFS_FMT_FULL, HFS_FMT_PART), and "-s size" (HFS_FMT_BLKS:
; megabytes, or gigabytes with a G after it; nothing, or 0: the whole card).  The label that follows is
; moved to HFS_STAT's start, and cut to 31 characters.  OUT: C = 0; or C = 1, .A = E_INVAL (an option
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
            bra         @option
:
            cmp         #'p'
            bne         :+
            lda         #HFS_FMT_PART

@option:
            tsb         HFS_FMT_OPT
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
            lda         #E_INVAL
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
            cpy         #SR_SIZE
            bne         :-
:
            cpx         #SR_SIZE                            ; (Zeros after it)
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
            jsr         HFS_RO_DISK                         ; (Not the ROM disk)
            bcs         @done
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

; ****************************************************************************
; Partitions (docs/design/plans/HYDRAFS.md): a card's HydraFS partition, found in its partition table (block 0),
; and made for it by "format -p"

; Is block 0 of card SD_DEV, in the cache (read from the card itself, not a partition), a partition table
; with a HydraFS partition (HFS_PART_TYPE) in it?  Called by HFS_VOLUME (srv.s) too.
; OUT: C = 0: HFS_PTR -> its entry; or C = 1.  Modifies: .A, .X, .Y
HFS_PART_FIND:
            jsr         HFS_MBR_OK
            bcs         @done
            ldx         #4                                  ; (Its 4 entries)

@entry:
            ldy         #MBR_P_TYPE
            lda         (HFS_PTR),Y
            cmp         #HFS_PART_TYPE
            beq         @found
            jsr         HFS_MBR_NEXT
            dex
            bne         @entry
            sec

@done:
            rts

@found:
            clc
            rts

; Is the block in the cache a partition table: $55 $AA at its end, and each entry's status $00 or $80?
; (A FAT volume that isn't partitioned has code where the table would be.)
; OUT: C = 0: HFS_PTR -> its first entry; or C = 1.  Modifies: .A, .Y
HFS_MBR_OK:
            lda         SD_CACHE
            clc
            adc         #<MBR_TABLE
            sta         HFS_PTR
            lda         SD_CACHE + 1
            adc         #>MBR_TABLE
            sta         HFS_PTR + 1
            ldy         #MBR_SIG - MBR_TABLE
            lda         (HFS_PTR),Y
            cmp         #$55
            bne         @no
            iny
            lda         (HFS_PTR),Y
            cmp         #$AA
            bne         @no
            ldy         #3 * MBR_ENTRY_SIZE + MBR_P_STATUS

@status:
            lda         (HFS_PTR),Y
            beq         :+
            cmp         #$80
            bne         @no
:
            tya
            sec
            sbc         #MBR_ENTRY_SIZE
            tay
            bcs         @status
            clc
            rts

@no:
            sec
            rts

; HFS_PTR on to the next entry.  Modifies: .A
HFS_MBR_NEXT:
            lda         HFS_PTR
            clc
            adc         #MBR_ENTRY_SIZE
            sta         HFS_PTR
            bcc         :+
            inc         HFS_PTR + 1
:
            rts

; Where a format's HydraFS goes (card SD_DEV = HFS_CARD, started): its HFS_V_BASE, and SD_LBA = its blocks.
; On a card with a HydraFS partition, in it (up to the card's end); with HFS_FMT_PART (-p), a card without
; one gets one (HFS_PART_MAKE); otherwise, the whole card.
; OUT: C = 0; or C = 1, .A = error.  Modifies: .A, .X, .Y, HFS_T4, HFS_N4, HFS_D, HFS_C
HFS_PART_SETUP:
            jsr         HFS_BASE_ZERO
            stz         SD_LBA                              ; Block 0 of the card
            stz         SD_LBA + 1
            stz         SD_LBA + 2
            stz         SD_LBA + 3
            jsr         SD_CACHE_LOAD
            bcs         @done
            jsr         HFS_PART_FIND
            bcc         @part
            lda         HFS_FMT_OPT
            and         #HFS_FMT_PART
            beq         @whole
            jsr         HFS_PART_MAKE
            bcs         @done

@part:
            jsr         HFS_BASE_X                          ; The base: the partition's first block ...
            ldy         #MBR_P_START
:
            lda         (HFS_PTR),Y
            sta         HFS_V_BASE,X
            inx
            iny
            cpy         #MBR_P_START + 4
            bne         :-
            ldx         #0                                  ;   and HFS_T4 = its size
:
            lda         (HFS_PTR),Y
            sta         HFS_T4,X
            inx
            iny
            cpy         #MBR_P_BLOCKS + 4
            bne         :-
            jsr         HFS_DISK_SIZE                       ; SD_LBA = the disk's blocks after the base
            jsr         HFS_BASE_X
            sec
            lda         SD_LBA
            sbc         HFS_V_BASE,X
            sta         SD_LBA
            lda         SD_LBA + 1
            sbc         HFS_V_BASE + 1,X
            sta         SD_LBA + 1
            lda         SD_LBA + 2
            sbc         HFS_V_BASE + 2,X
            sta         SD_LBA + 2
            lda         SD_LBA + 3
            sbc         HFS_V_BASE + 3,X
            sta         SD_LBA + 3
            bcc         @bad                                ; (It starts past the card's end)
            lda         HFS_T4                              ; The partition's size, if that's less
            cmp         SD_LBA
            lda         HFS_T4 + 1
            sbc         SD_LBA + 1
            lda         HFS_T4 + 2
            sbc         SD_LBA + 2
            lda         HFS_T4 + 3
            sbc         SD_LBA + 3
            bcs         @ok
            ldx         #3
:
            lda         HFS_T4,X
            sta         SD_LBA,X
            dex
            bpl         :-

@ok:
            clc
            rts

@whole:
            jsr         HFS_DISK_SIZE
            clc

@done:
            rts

@bad:
            lda         #E_MEDIA
            sec
            rts

; A HydraFS partition for card SD_DEV, after its other partitions, to the card's end, in the partition
; table in block 0 (in the cache; a card with no table gets one, with the partition from block
; HFS_PART_ALIGN).  It starts on a 1 MB boundary (HFS_PART_ALIGN).  The new table goes to the card.
; OUT: C = 0: HFS_PTR -> its entry; or C = 1, .A = E_NOSPC (no room, or no free entry) or a card error.
; Modifies: .A, .X, .Y, HFS_T4, HFS_N4, HFS_D, HFS_C
HFS_PART_MAKE:
            jsr         HFS_MBR_OK
            bcc         @table
            lda         SD_CACHE                            ; No table: a new one (the block all zeros, then
            sta         HFS_PTR                             ;   the table's signature)
            lda         SD_CACHE + 1
            sta         HFS_PTR + 1
            lda         #0
            tay
:
            sta         (HFS_PTR),Y
            iny
            bne         :-
            inc         HFS_PTR + 1
:
            sta         (HFS_PTR),Y
            iny
            bne         :-
            ldy         #<MBR_SIG
            lda         #$55
            sta         (HFS_PTR),Y
            iny
            lda         #$AA
            sta         (HFS_PTR),Y
            jsr         HFS_MBR_OK                          ; (HFS_PTR -> the table)

@table:
            ldx         #3                                  ; HFS_T4 = where the last partition ends
:
            stz         HFS_T4,X
            dex
            bpl         :-
            lda         #$FF                                ; HFS_C = a free entry's offset ($FF: none)
            sta         HFS_C
            ldx         #0                                  ; (.X = the entry's offset)

@entry:
            txa
            ora         #MBR_P_TYPE
            tay
            lda         (HFS_PTR),Y
            bne         @used
            lda         HFS_C                               ; (The first free one)
            bpl         @next
            stx         HFS_C
            bra         @next

@used:
            txa                                             ; HFS_N4 = its first block, HFS_D its size
            ora         #MBR_P_START
            tay
            phx
            ldx         #0
:
            lda         (HFS_PTR),Y
            sta         HFS_N4,X
            iny
            inx
            cpx         #8
            bne         :-
            plx
            clc                                             ; HFS_N4 = where it ends
            lda         HFS_N4
            adc         HFS_D
            sta         HFS_N4
            lda         HFS_N4 + 1
            adc         HFS_D + 1
            sta         HFS_N4 + 1
            lda         HFS_N4 + 2
            adc         HFS_D + 2
            sta         HFS_N4 + 2
            lda         HFS_N4 + 3
            adc         HFS_D + 3
            sta         HFS_N4 + 3
            lda         HFS_T4                              ; Past the last one so far?
            cmp         HFS_N4
            lda         HFS_T4 + 1
            sbc         HFS_N4 + 1
            lda         HFS_T4 + 2
            sbc         HFS_N4 + 2
            lda         HFS_T4 + 3
            sbc         HFS_N4 + 3
            bcs         @next
            phx
            ldx         #3
:
            lda         HFS_N4,X
            sta         HFS_T4,X
            dex
            bpl         :-
            plx

@next:
            txa
            clc
            adc         #MBR_ENTRY_SIZE
            tax
            cpx         #4 * MBR_ENTRY_SIZE
            bne         @entry
            lda         HFS_C
            bpl         @far6
            jmp         @full
@far6:
            clc                                             ; The new one's first block: the end rounded up
            lda         HFS_T4                              ;   to HFS_PART_ALIGN (and not block 0)
            adc         #<(HFS_PART_ALIGN - 1)
            lda         HFS_T4 + 1
            adc         #>(HFS_PART_ALIGN - 1)
            and         #<~(>(HFS_PART_ALIGN - 1))
            sta         HFS_T4 + 1
            lda         HFS_T4 + 2
            adc         #0
            sta         HFS_T4 + 2
            lda         HFS_T4 + 3
            adc         #0
            sta         HFS_T4 + 3
            bcc         @far5
            jmp         @full
@far5:
            stz         HFS_T4
            lda         HFS_T4 + 1
            ora         HFS_T4 + 2
            ora         HFS_T4 + 3
            bne         :+
            lda         #>HFS_PART_ALIGN
            sta         HFS_T4 + 1
:
            jsr         HFS_DISK_SIZE                       ; HFS_N4 = its size: the rest of the card
            sec
            lda         SD_LBA
            sbc         HFS_T4
            sta         HFS_N4
            lda         SD_LBA + 1
            sbc         HFS_T4 + 1
            sta         HFS_N4 + 1
            lda         SD_LBA + 2
            sbc         HFS_T4 + 2
            sta         HFS_N4 + 2
            lda         SD_LBA + 3
            sbc         HFS_T4 + 3
            sta         HFS_N4 + 3
            bcc         @full
            lda         HFS_N4 + 3                          ; (Room for a HydraFS: HFS_FORMAT checks it)
            ora         HFS_N4 + 2
            ora         HFS_N4 + 1
            beq         @full
            ldy         HFS_C                               ; The entry
            lda         #0
            sta         (HFS_PTR),Y
            iny
            ldx         #0
:
            lda         HFS_PART_CHS,X                      ; (Its blocks as cylinder, head, sector: none)
            sta         (HFS_PTR),Y
            iny
            inx
            cpx         #7
            bne         :-
            ldx         #0
:
            lda         HFS_T4,X                            ; Its first block and size (HFS_N4 then)
            sta         (HFS_PTR),Y
            iny
            inx
            cpx         #4
            bne         :-
            ldx         #0
:
            lda         HFS_N4,X
            sta         (HFS_PTR),Y
            iny
            inx
            cpx         #4
            bne         :-
            stz         SD_LBA                              ; The table to the card (the cache has it)
            stz         SD_LBA + 1
            stz         SD_LBA + 2
            stz         SD_LBA + 3
            lda         SD_CACHE
            sta         SD_BUF
            lda         SD_CACHE + 1
            sta         SD_BUF + 1
            jsr         SD_WRITE_BLOCK
            bcs         @done
            jmp         HFS_PART_FIND

@full:
            lda         #E_NOSPC
            sec

@done:
            stz         SD_CVALID                           ; (The cache's block 0 may differ from the card's)
            rts

HFS_PART_CHS:   .byte   $FE, $FF, $FF, HFS_PART_TYPE, $FE, $FF, $FF

.assert     HFS_D = HFS_N4 + 4, error, "HFS_PART_MAKE: an entry's first block and size go in HFS_N4, HFS_D"
.assert     MBR_P_CHS1 = 1 && MBR_P_TYPE = 4 && MBR_P_CHS2 = 5 && MBR_P_START = 8 && MBR_P_BLOCKS = 12, error, "HFS_PART_MAKE: the entry's fields"
.assert     (HFS_PART_ALIGN & $FF) = 0, error, "HFS_PART_MAKE: HFS_PART_ALIGN is whole pages of blocks"

; Card HFS_CARD's HydraFS starts at block 0 (until HFS_PART_SETUP finds otherwise).  Modifies: .A, .X, .Y
HFS_BASE_ZERO:
            jsr         HFS_BASE_X
            ldy         #4
:
            stz         HFS_V_BASE,X
            inx
            dey
            bne         :-
            rts

; SD_LBA = disk HFS_CARD's size in blocks (storage.s's d_blocks).  Modifies: .A, .X, .Y
HFS_DISK_SIZE:
            jsr         HFS_CARD_X
            ldy         #0
:
            lda         d_blocks,X
            sta         SD_LBA,Y
            inx
            iny
            cpy         #4
            bne         :-
            rts
