.debuginfo

; ****************************************************************************
; HydraFS's check, and a card's details in its ctl file's text (BIOS ROM page 6, with hfs_srv.s and
; hfs_write.s, inside `.scope PAGE6`).  Both are reached from /dev/sd/N/ctl, in the SD server (page 3):
;
;   "check"      walks every directory from the root, marks in a bitmap every cluster a file, a directory or
;                an extent block uses, and compares that with the free map: clusters marked in use that
;                nothing uses (lost: harmless, but the space is wasted), clusters in use but marked free
;                (unmarked: dangerous, as a new file could be given them), and clusters used twice (two
;                files share them: one of them is damaged).  It recounts the free clusters into the
;                superblock.  The results are shown in the ctl file's text.
;   "check fix"  does the same, and makes the free map say what the files use: lost clusters are freed, and
;                unmarked ones marked in use.  (A cluster used twice needs a person, to decide which file
;                keeps it.)
;
; The bitmap covers HFS_CK_WINDOW clusters (256 MB of card), so a bigger card takes a pass for each 256 MB,
; and each walks the directories again.  The walk follows directories HFS_CK_DEPTH_MAX deep.

.segment "HFS_P6"

HFS_CK_S            = HFS_PLOC                              ; Comparing a map byte: the bitmap's byte ...
HFS_CK_M            = HFS_PLOC + 1                          ;   the map's (the bits that are clusters)
HFS_CK_MO           = HFS_PLOC + 2                          ;   and the map's, all of it
HFS_CK_REM          = HFS_DLOC                              ; Clusters from here to the card's end (4)
HFS_CK_CNT          = HFS_NLOC                              ; A map block's bytes left (2) ...
HFS_CK_BLKS         = HFS_NLOC + 2                          ;   and the window's map blocks left
; (HFS_D = the pass's first cluster, HFS_N4 = the cluster after its last; HFS_LASTB = a run's end)

HFS_LOW_BITS:   .byte   $00, $01, $03, $07, $0F, $1F, $3F, $7F
HFS_S_FIX:      .byte   "fix"

; "check" or "check fix" (the card is SD_DEV; the text after the word is in HFS_STAT).
; OUT: C = 0; or C = 1, .A = error (ERR_IO_NAME: directories too deep to walk; ERR_IO_NOT_FS ...)
HFS_CHECK:
            lda         #$FF                                ; (No results, unless it gets to the end)
            sta         HFS_CK_CARD
            lda         SD_DEV
            sta         HFS_CARD
            jsr         HFS_VOLUME
            bcc         @far7
            jmp         @done
@far7:
            stz         HFS_CK_FIXED                        ; "fix"?
            ldy         #2
:
            lda         HFS_S_FIX,Y
            cmp         HFS_STAT,Y
            bne         :+
            dey
            bpl         :-
            inc         HFS_CK_FIXED
:
            lda         HFS_CK_BUF + 1                      ; Its buffer: from the MMU the first time
            bne         :+
            lda         #<HFS_CK_BUF_SIZE
            ldy         #>HFS_CK_BUF_SIZE
            ldx         #0
            jsr         MM_ALLOC
            bcc         @far8
            jmp         @done
@far8:
            jsr         MM_LOCK                             ; (.A.Y = the address)
            sta         HFS_CK_BUF
            sty         HFS_CK_BUF + 1
:
            ldx         #HFS_CK_BUF - HFS_CK_LOST - 1       ; The counts: none yet
:
            stz         HFS_CK_LOST,X
            dex
            bpl         :-
            stz         HFS_D                               ; The first pass: from cluster 0
            stz         HFS_D + 1
            stz         HFS_D + 2
            stz         HFS_D + 3
            lda         #$FF                                ; (The buffer isn't clear yet)
            sta         HFS_CK_MARKS
            jsr         HFS_CARD_X                          ; Its progress: a step per pass, (the clusters
            clc                                             ;   + 65535) / 65536 of them
            lda         HFS_V_CLUSTERS,X
            adc         #$FF
            lda         HFS_V_CLUSTERS + 1,X
            adc         #$FF
            lda         HFS_V_CLUSTERS + 2,X
            adc         #0
            sta         HFS_T4
            lda         HFS_V_CLUSTERS + 3,X
            adc         #0
            sta         HFS_T4 + 1
            stz         HFS_T4 + 2
            stz         HFS_T4 + 3
            jsr         HFS_PG_START

@pass:
            jsr         HFS_CARD_X                          ; Past the card's last cluster: that's all
            lda         HFS_D
            cmp         HFS_V_CLUSTERS,X
            lda         HFS_D + 1
            sbc         HFS_V_CLUSTERS + 1,X
            lda         HFS_D + 2
            sbc         HFS_V_CLUSTERS + 2,X
            lda         HFS_D + 3
            sbc         HFS_V_CLUSTERS + 3,X
            bcs         @counted
            jsr         HFS_CK_CLEAR
            jsr         HFS_CK_WALK
            bcs         @done
            jsr         HFS_CK_COMPARE
            bcs         @done
            jsr         HFS_PG_TICK
            inc         HFS_D + 2                           ; The next HFS_CK_WINDOW clusters
            bne         @pass
            inc         HFS_D + 3
            bra         @pass

@counted:
            jsr         HFS_CARD_X                          ; The free clusters, counted, to the superblock
            ldy         #0
:
            lda         HFS_CK_FREE,Y
            sta         HFS_V_FREE,X
            inx
            iny
            cpy         #4
            bne         :-
            lda         #1
            sta         HFS_SBDIRTY
            lda         HFS_CARD
            sta         HFS_CK_CARD
            lda         #0
            clc

@done:
            jsr         HFS_PG_END                          ; (Keeps .A and C)
            jmp         HFS_FINISH                          ; (A fixed map block, and the superblock, go now)

.assert     HFS_CK_WINDOW = $10000, error, "HFS_CHECK: a pass is the next 65536 clusters (inc HFS_D + 2)"

; A pass's bitmap all clear (unless the last pass marked nothing in it: HFS_CK_MARKS), and HFS_N4 = the
; cluster after the pass's last.  Modifies: .A, .X, .Y
HFS_CK_CLEAR:
            lda         HFS_CK_MARKS
            beq         @clear                              ; (Nothing marked: clear already)
            stz         HFS_CK_MARKS
            lda         HFS_CK_BUF
            sta         HFS_PTR
            lda         HFS_CK_BUF + 1
            sta         HFS_PTR + 1
            ldx         #>(HFS_CK_WINDOW / 8)               ; (Pages)
            lda         #0
            tay
:
            sta         (HFS_PTR),Y
            iny
            bne         :-
            inc         HFS_PTR + 1
            dex
            bne         :-

@clear:
            lda         HFS_D
            sta         HFS_N4
            lda         HFS_D + 1
            sta         HFS_N4 + 1
            lda         HFS_D + 2
            clc
            adc         #1
            sta         HFS_N4 + 2
            lda         HFS_D + 3
            adc         #0
            sta         HFS_N4 + 3
            rts

; Mark everything the card's files use, in the pass's window: the root's clusters, then every directory's
; entries', down through the directories.  The walk keeps a copy of each directory's entry it's in, and
; where it is in it, in the buffer after the bitmap (HFS_CK_SP: the deepest).
; OUT: C = 0; or C = 1, .A = error (ERR_IO_NAME: too deep)
HFS_CK_WALK:
            LOAD_ADDR   HFS_CK_RUN, HFS_RUN_VEC
            lda         HFS_CK_BUF                          ; The walk's first directory goes after the bitmap:
            clc                                             ;   HFS_CK_SP starts a level before it
            adc         #<(HFS_CK_WINDOW / 8 - HFS_CK_LEVEL)
            sta         HFS_CK_SP
            lda         HFS_CK_BUF + 1
            adc         #>(HFS_CK_WINDOW / 8 - HFS_CK_LEVEL)
            sta         HFS_CK_SP + 1
            stz         HFS_CK_DEPTH
            stz         HFS_LOC                             ; The root's entry: in the superblock
            stz         HFS_LOC + 1
            stz         HFS_LOC + 2
            stz         HFS_LOC + 3
            lda         #HFS_SB_ROOT / HFS_ENTRY_SIZE
            sta         HFS_LOC + 4
            jsr         HFS_ENT_READ
            bcc         @far6
            jmp         @done
@far6:
            lda         #<HFS_ENT
            sta         HFS_PTR
            lda         #>HFS_ENT
            sta         HFS_PTR + 1
            jsr         HFS_CK_ENTER

@next:
            bcs         @done
            lda         HFS_CK_DEPTH
            beq         @walked                             ; (Back above the root: all done)
            lda         HFS_CK_SP                           ; HFS_FP = the deepest directory, and SD_POS =
            sta         HFS_FP                              ;   where the walk is in it
            lda         HFS_CK_SP + 1
            sta         HFS_FP + 1
            ldy         #HFS_ENTRY_SIZE
            ldx         #0
:
            lda         (HFS_FP),Y
            sta         SD_POS,X
            iny
            inx
            cpx         #4
            bne         :-
            jsr         HFS_AT_END
            bcc         @entry
            dec         HFS_CK_DEPTH                        ; Its last entry: back to the one above
            lda         HFS_CK_SP
            sec
            sbc         #HFS_CK_LEVEL
            sta         HFS_CK_SP
            bcs         :+
            dec         HFS_CK_SP + 1
:
            clc
            bra         @next

@entry:
            ldy         #HFS_ENTRY_SIZE                     ; (Next time: the entry after this.  Unrolled: a
            lda         SD_POS                              ;   loop's cpx would lose the carry)
            clc
            adc         #HFS_ENTRY_SIZE
            sta         (HFS_FP),Y
            iny
            lda         SD_POS + 1
            adc         #0
            sta         (HFS_FP),Y
            iny
            lda         SD_POS + 2
            adc         #0
            sta         (HFS_FP),Y
            iny
            lda         SD_POS + 3
            adc         #0
            sta         (HFS_FP),Y
            jsr         HFS_FILE_BLOCK                      ; The entry, in the cache
            bcs         @done
            jsr         HFS_LOAD
            bcs         @done
            lda         SD_POS
            sta         HFS_OFS
            lda         SD_POS + 1
            and         #1
            sta         HFS_OFS + 1
            jsr         HFS_AT
            lda         (HFS_PTR)
            clc
            beq         @next                               ; (A free one)
            jsr         HFS_CK_ENTER
            bra         @next

@walked:
            clc

@done:
            rts

; An entry the walk has come to (at HFS_PTR): copied to the level after the deepest, and its clusters
; marked; a directory becomes the deepest, to walk next.
; OUT: C = 0; or C = 1, .A = error (ERR_IO_NAME: too deep).  Modifies: .A, .X, .Y
HFS_CK_ENTER:
            lda         HFS_CK_SP                           ; HFS_FP = the next level
            clc
            adc         #HFS_CK_LEVEL
            sta         HFS_FP
            lda         HFS_CK_SP + 1
            adc         #0
            sta         HFS_FP + 1
            ldy         #HFS_ENTRY_SIZE - 1
:
            lda         (HFS_PTR),Y
            sta         (HFS_FP),Y
            dey
            bpl         :-
            jsr         HFS_EACH_RUN                        ; Its clusters (HFS_CK_RUN)
            bcs         @done
            ldy         #HFS_E_MODE
            lda         (HFS_FP),Y
            bpl         @file                               ; (HFS_M_DIR)
            lda         HFS_CK_DEPTH
            cmp         #HFS_CK_DEPTH_MAX
            bcs         @deep
            inc         HFS_CK_DEPTH                        ; A directory: the deepest now, from its first
            lda         HFS_FP                              ;   entry
            sta         HFS_CK_SP
            lda         HFS_FP + 1
            sta         HFS_CK_SP + 1
            ldy         #HFS_ENTRY_SIZE
            lda         #0
            sta         (HFS_FP),Y
            iny
            sta         (HFS_FP),Y
            iny
            sta         (HFS_FP),Y
            iny
            sta         (HFS_FP),Y

@file:
            clc

@done:
            rts

@deep:
            lda         #ERR_IO_NAME
            sec
            rts

.assert     HFS_CK_LEVEL >= HFS_ENTRY_SIZE + 4, error, "A check level: the entry, then 4 bytes of where the walk is"

; HFS_EACH_RUN's routine for the check: mark a run (HFS_XLEN clusters from HFS_XCL) in the bitmap, the part
; of it in the pass's window (HFS_D to HFS_N4); a cluster that's marked already is used twice.  OUT: C = 0
HFS_CK_RUN:
            clc                                             ; HFS_LASTB = the run's end
            lda         HFS_XCL
            adc         HFS_XLEN
            sta         HFS_LASTB
            lda         HFS_XCL + 1
            adc         HFS_XLEN + 1
            sta         HFS_LASTB + 1
            lda         HFS_XCL + 2
            adc         #0
            sta         HFS_LASTB + 2
            lda         HFS_XCL + 3
            adc         #0
            sta         HFS_LASTB + 3
            lda         HFS_D                               ; It ends before the window: none of it here
            cmp         HFS_LASTB
            lda         HFS_D + 1
            sbc         HFS_LASTB + 1
            lda         HFS_D + 2
            sbc         HFS_LASTB + 2
            lda         HFS_D + 3
            sbc         HFS_LASTB + 3
            bcc         @far5
            jmp         @none
@far5:
            lda         HFS_XCL                             ; It starts after the window: none of it
            cmp         HFS_N4
            lda         HFS_XCL + 1
            sbc         HFS_N4 + 1
            lda         HFS_XCL + 2
            sbc         HFS_N4 + 2
            lda         HFS_XCL + 3
            sbc         HFS_N4 + 3
            bcc         @far4
            jmp         @none
@far4:
            lda         HFS_XCL                             ; HFS_C = its first cluster in the window
            cmp         HFS_D
            lda         HFS_XCL + 1
            sbc         HFS_D + 1
            lda         HFS_XCL + 2
            sbc         HFS_D + 2
            lda         HFS_XCL + 3
            sbc         HFS_D + 3
            ldx         #3
            bcc         @from_window
:
            lda         HFS_XCL,X
            sta         HFS_C,X
            dex
            bpl         :-
            bra         @to

@from_window:
            lda         HFS_D,X
            sta         HFS_C,X
            dex
            bpl         @from_window

@to:
            lda         HFS_N4                              ; HFS_LASTB = its end in the window
            cmp         HFS_LASTB
            lda         HFS_N4 + 1
            sbc         HFS_LASTB + 1
            lda         HFS_N4 + 2
            sbc         HFS_LASTB + 2
            lda         HFS_N4 + 3
            sbc         HFS_LASTB + 3
            bcs         :++
            ldx         #3
:
            lda         HFS_N4,X
            sta         HFS_LASTB,X
            dex
            bpl         :-
:
            sec                                             ; HFS_CK_N = the clusters to mark (65536 at most)
            lda         HFS_LASTB
            sbc         HFS_C
            sta         HFS_CK_N
            lda         HFS_LASTB + 1
            sbc         HFS_C + 1
            sta         HFS_CK_N + 1
            lda         HFS_LASTB + 2
            sbc         HFS_C + 2
            sta         HFS_CK_N + 2
            sec                                             ; HFS_OFS = the first's bit in the bitmap: its
            lda         HFS_C                               ;   byte, and its bit in it
            sbc         HFS_D
            sta         HFS_OFS
            lda         HFS_C + 1
            sbc         HFS_D + 1
            sta         HFS_OFS + 1
            lda         HFS_OFS
            and         #7
            tax
            lda         HFS_BITS,X
            sta         HFS_CK_MASK
            lsr         HFS_OFS + 1
            ror         HFS_OFS
            lsr         HFS_OFS + 1
            ror         HFS_OFS
            lsr         HFS_OFS + 1
            ror         HFS_OFS
            lda         HFS_CK_BUF
            clc
            adc         HFS_OFS
            sta         HFS_PTR
            lda         HFS_CK_BUF + 1
            adc         HFS_OFS + 1
            sta         HFS_PTR + 1

            sta         HFS_CK_MARKS                        ; (The bitmap has marks: .A, the pointer's high
                                                            ;   byte, is never 0)

@mark:
            lda         (HFS_PTR)
            and         HFS_CK_MASK
            beq         :+
            ldx         #1                                  ; Marked already: a cluster used twice
            ldy         #HFS_CK_TWICE - HFS_CK_LOST
            jsr         HFS_CK_ADD
:
            lda         (HFS_PTR)
            ora         HFS_CK_MASK
            sta         (HFS_PTR)
            asl         HFS_CK_MASK                         ; The next cluster's bit
            bcc         :+
            rol         HFS_CK_MASK                         ; (C = 1: bit 0 of the next byte)
            inc         HFS_PTR
            bne         :+
            inc         HFS_PTR + 1
:
            lda         HFS_CK_N
            sec
            sbc         #1
            sta         HFS_CK_N
            lda         HFS_CK_N + 1
            sbc         #0
            sta         HFS_CK_N + 1
            lda         HFS_CK_N + 2
            sbc         #0
            sta         HFS_CK_N + 2
            ora         HFS_CK_N + 1
            ora         HFS_CK_N
            bne         @mark

@none:
            clc
            rts

; Compare the pass's bitmap with the free map, counting the lost, unmarked and free clusters (with
; HFS_CK_FIXED, making the map say what the bitmap does first).  OUT: C = 0; or C = 1, .A = a card error
HFS_CK_COMPARE:
            ldx         #3                                  ; HFS_C = the window's first cluster
:
            lda         HFS_D,X
            sta         HFS_C,X
            dex
            bpl         :-
            jsr         HFS_CARD_X                          ; HFS_CK_REM = the clusters from there to the end
            sec
            lda         HFS_V_CLUSTERS,X
            sbc         HFS_D
            sta         HFS_CK_REM
            lda         HFS_V_CLUSTERS + 1,X
            sbc         HFS_D + 1
            sta         HFS_CK_REM + 1
            lda         HFS_V_CLUSTERS + 2,X
            sbc         HFS_D + 2
            sta         HFS_CK_REM + 2
            lda         HFS_V_CLUSTERS + 3,X
            sbc         HFS_D + 3
            sta         HFS_CK_REM + 3
            lda         HFS_CK_BUF                          ; HFS_XP -> the bitmap
            sta         HFS_XP
            lda         HFS_CK_BUF + 1
            sta         HFS_XP + 1
            lda         #HFS_CK_WINDOW / (HFS_BLOCK * 8)    ; The window's map blocks
            sta         HFS_CK_BLKS

@block:
            jsr         HFS_CK_REM0
            bne         @far3
            jmp         @end
@far3:
            jsr         HFS_MAP_BIT                         ; HFS_PTR -> the map block's first byte
            bcc         @far2
            jmp         @done
@far2:
            bit         HFS_MSTATE                          ; A map block a quick format hasn't written yet
            bvc         @bytes                              ;   (HFS_MS_FRESH: all free), a whole one, with
            lda         HFS_CK_REM + 3                      ;   none of its clusters used: 4096 free ones, at
            ora         HFS_CK_REM + 2                      ;   once
            bne         :+
            lda         HFS_CK_REM + 1
            cmp         #>(HFS_BLOCK * 8)
            bcc         @bytes
:
            lda         HFS_CK_MARKS                        ; (No marks at all: none of its clusters used)
            beq         @none
            ldy         #0                                  ; (The bitmap's 512 bytes for it: all 0?)
:
            lda         (HFS_XP),Y
            bne         @bytes
            iny
            bne         :-
            inc         HFS_XP + 1
:
            lda         (HFS_XP),Y
            bne         @undo
            iny
            bne         :-
            bra         @past

@none:
            inc         HFS_XP + 1

@past:
            inc         HFS_XP + 1                          ; (Past them)
            clc
            lda         HFS_CK_FREE + 1
            adc         #>(HFS_BLOCK * 8)
            sta         HFS_CK_FREE + 1
            bcc         :+
            inc         HFS_CK_FREE + 2
            bne         :+
            inc         HFS_CK_FREE + 3
:
            sec
            lda         HFS_CK_REM + 1
            sbc         #>(HFS_BLOCK * 8)
            sta         HFS_CK_REM + 1
            bcc         @far9
            jmp         @next_block
@far9:
            lda         HFS_CK_REM + 2
            bne         :+
            dec         HFS_CK_REM + 3
:
            dec         HFS_CK_REM + 2
            jmp         @next_block

@undo:
            dec         HFS_XP + 1

@bytes:
            stz         HFS_CK_CNT
            lda         #>HFS_BLOCK
            sta         HFS_CK_CNT + 1

@byte:
            jsr         HFS_CK_REM0
            bne         @far1
            jmp         @end
@far1:
            lda         HFS_CK_REM + 3                      ; The bits in this byte that are clusters: all
            ora         HFS_CK_REM + 2                      ;   8, or the ones the card has left
            ora         HFS_CK_REM + 1
            bne         @eight
            lda         HFS_CK_REM
            cmp         #8
            bcs         @eight
            tax
            lda         HFS_LOW_BITS,X
            sta         HFS_CK_MASK
            stz         HFS_CK_REM
            bra         @bits

@eight:
            lda         #$FF
            sta         HFS_CK_MASK
            sec
            lda         HFS_CK_REM
            sbc         #8
            sta         HFS_CK_REM
            lda         HFS_CK_REM + 1
            sbc         #0
            sta         HFS_CK_REM + 1
            lda         HFS_CK_REM + 2
            sbc         #0
            sta         HFS_CK_REM + 2
            lda         HFS_CK_REM + 3
            sbc         #0
            sta         HFS_CK_REM + 3

@bits:
            lda         HFS_CK_MASK                         ; (Quickly: 8 clusters that aren't used, nor
            eor         #$FF                                ;   marked, are 8 free ones: most of an empty
            ora         (HFS_XP)                            ;   card)
            ora         (HFS_PTR)
            bne         @count
            ldx         #8
            ldy         #HFS_CK_FREE - HFS_CK_LOST
            jsr         HFS_CK_ADD
            bra         @step

@count:
            lda         (HFS_XP)                            ; In use (the bitmap), and marked (the map)
            and         HFS_CK_MASK
            sta         HFS_CK_S
            lda         (HFS_PTR)
            sta         HFS_CK_MO
            and         HFS_CK_MASK
            sta         HFS_CK_M
            lda         HFS_CK_S                            ; Lost: marked, but not in use
            eor         #$FF
            and         HFS_CK_M
            ldy         #HFS_CK_LOST - HFS_CK_LOST
            jsr         HFS_CK_COUNT
            lda         HFS_CK_M                            ; Unmarked: in use, but not marked
            eor         #$FF
            and         HFS_CK_S
            ldy         #HFS_CK_UNMARKED - HFS_CK_LOST
            jsr         HFS_CK_COUNT
            lda         HFS_CK_MO
            ldx         HFS_CK_FIXED
            beq         @kept
            lda         HFS_CK_MASK                         ; Fixing: the map says what's in use
            eor         #$FF
            and         HFS_CK_MO
            ora         HFS_CK_S
            cmp         HFS_CK_MO
            beq         @kept
            sta         (HFS_PTR)
            pha
            jsr         HFS_MAP_CHANGED                     ; (A map block not on the card yet: now it is)
            bcc         :+
            plx                                             ; (Failed: .A = the error)
            jmp         @done
:
            pla

@kept:
            eor         #$FF                                ; Free: the clusters the map has free
            and         HFS_CK_MASK
            ldy         #HFS_CK_FREE - HFS_CK_LOST
            jsr         HFS_CK_COUNT
@step:
            inc         HFS_XP                              ; The next byte
            bne         :+
            inc         HFS_XP + 1
:
            inc         HFS_PTR
            bne         :+
            inc         HFS_PTR + 1
:
            lda         HFS_CK_CNT
            bne         :+
            dec         HFS_CK_CNT + 1
:
            dec         HFS_CK_CNT
            lda         HFS_CK_CNT
            ora         HFS_CK_CNT + 1
            beq         :+
            jmp         @byte
:
@next_block:
            lda         HFS_C + 1                           ; The next map block: 4096 clusters on
            clc
            adc         #>(HFS_BLOCK * 8)
            sta         HFS_C + 1
            bcc         :+
            inc         HFS_C + 2
            bne         :+
            inc         HFS_C + 3
:
            dec         HFS_CK_BLKS
            beq         @end
            jmp         @block

@end:
            clc

@done:
            rts

; Z = 1: no clusters left (HFS_CK_REM = 0).  Modifies: .A
HFS_CK_REM0:
            lda         HFS_CK_REM
            ora         HFS_CK_REM + 1
            ora         HFS_CK_REM + 2
            ora         HFS_CK_REM + 3
            rts

; Add the 1 bits in .A to a count (.Y = its offset from HFS_CK_LOST).  Modifies: .A, .X
HFS_CK_COUNT:
            ldx         #0

@bit:
            cmp         #0
            beq         HFS_CK_ADD
            lsr
            bcc         @bit
            inx
            bra         @bit

; Add .X to a count (.Y = its offset from HFS_CK_LOST: the lost, unmarked, twice or free clusters).
; Modifies: .A
HFS_CK_ADD:
            txa
            clc
            adc         HFS_CK_LOST,Y
            sta         HFS_CK_LOST,Y
            bcc         @done
            lda         HFS_CK_LOST + 1,Y
            adc         #0
            sta         HFS_CK_LOST + 1,Y
            lda         HFS_CK_LOST + 2,Y
            adc         #0
            sta         HFS_CK_LOST + 2,Y
            lda         HFS_CK_LOST + 3,Y
            adc         #0
            sta         HFS_CK_LOST + 3,Y

@done:
            rts

; ****************************************************************************
; A card's HydraFS details, for its ctl file's text (from SD_CTL_READ, page 3): lines added to the text
; being made in the data area (mapped, ZP_IO_REQ at it; SD_N = its length so far, SD_DEV = the card).
; Nothing for a card with no HydraFS on it.
;   hydrafs label=GAMES
;   free 1012 KB of 2044 KB
;   check: lost 0, unmarked 0, twice 0          (after a check of this card; ", fixed" after "check fix")
HFS_CTL_LINES:
            lda         SD_DEV
            sta         HFS_CARD
            jsr         HFS_VOLUME
            bcs         @done                               ; (No HydraFS: nothing to add)
            stz         SD_LBA                              ; The label, from the superblock
            stz         SD_LBA + 1
            stz         SD_LBA + 2
            stz         SD_LBA + 3
            jsr         HFS_LOAD
            bcs         @done
            stz         HFS_LEN
            ldx         #HFS_S_LABEL - HFS_TEXTS
            jsr         HFS_PUT_TEXT
            ldy         #HFS_SB_LABEL

@label:
            lda         (SD_CACHE),Y
            beq         @labelled
            jsr         HFS_PUT
            iny
            cpy         #HFS_SB_LABEL + HFS_NAME_MAX
            bne         @label

@labelled:
            jsr         HFS_LINE_END
            ldx         #HFS_S_FREE - HFS_TEXTS             ; The free clusters and all of them, in KB
            jsr         HFS_PUT_TEXT
            jsr         HFS_CARD_X
            jsr         HFS_PUT_KB_FREE
            ldx         #HFS_S_KB_OF - HFS_TEXTS
            jsr         HFS_PUT_TEXT
            jsr         HFS_CARD_X
            jsr         HFS_PUT_KB_ALL
            ldx         #HFS_S_KB - HFS_TEXTS
            jsr         HFS_PUT_TEXT
            jsr         HFS_LINE_END
            lda         HFS_CK_CARD                         ; The last check, if it was of this card
            cmp         HFS_CARD
            bne         @ok
            ldx         #HFS_S_LOST - HFS_TEXTS
            ldy         #HFS_CK_LOST - HFS_CK_LOST
            jsr         HFS_PUT_COUNT
            ldx         #HFS_S_UNMARKED - HFS_TEXTS
            ldy         #HFS_CK_UNMARKED - HFS_CK_LOST
            jsr         HFS_PUT_COUNT
            ldx         #HFS_S_TWICE - HFS_TEXTS
            ldy         #HFS_CK_TWICE - HFS_CK_LOST
            jsr         HFS_PUT_COUNT
            lda         HFS_CK_FIXED
            beq         :+
            ldx         #HFS_S_FIXED - HFS_TEXTS
            jsr         HFS_PUT_TEXT
:
            jsr         HFS_LINE_END

@ok:
            clc

@done:
            rts

; A text (HFS_TEXTS,X) and a count (HFS_CK_LOST,Y), to the line, and the line out (so a line can be longer
; than HFS_STAT).  Modifies: .A, .X, .Y
HFS_PUT_COUNT:
            jsr         HFS_PUT_TEXT
            ldx         #0
:
            lda         HFS_CK_LOST,Y
            sta         HFS_CL,X
            iny
            inx
            cpx         #4
            bne         :-
            jsr         HFS_PUT_DEC
            bra         HFS_LINE_OUT

; A card's free clusters (HFS_PUT_KB_FREE), or all of them (HFS_PUT_KB_ALL), in KB, to the line (.X = the
; card * 4).  Modifies: .A, .X, .Y
HFS_PUT_KB_FREE:
            ldy         #0
:
            lda         HFS_V_FREE,X
            sta         HFS_CL,Y
            inx
            iny
            cpy         #4
            bne         :-
            bra         HFS_PUT_KB

HFS_PUT_KB_ALL:
            ldy         #0
:
            lda         HFS_V_CLUSTERS,X
            sta         HFS_CL,Y
            inx
            iny
            cpy         #4
            bne         :-

HFS_PUT_KB:
            asl         HFS_CL                              ; (4 KB a cluster)
            rol         HFS_CL + 1
            rol         HFS_CL + 2
            rol         HFS_CL + 3
            asl         HFS_CL
            rol         HFS_CL + 1
            rol         HFS_CL + 2
            rol         HFS_CL + 3
            jmp         HFS_PUT_DEC

.assert     HFS_CLUSTER_BLOCKS * HFS_BLOCK = 4096, error, "HFS_PUT_KB: a cluster is 4 KB"

; Add the text at HFS_TEXTS,X (zero-terminated) to the line.  Modifies: .A, .X
HFS_PUT_TEXT:
            lda         HFS_TEXTS,X
            beq         @done
            jsr         HFS_PUT
            inx
            bra         HFS_PUT_TEXT

@done:
            rts

; End the line (CR LF), and put it out
HFS_LINE_END:
            lda         #ASCII_CR
            jsr         HFS_PUT
            lda         #ASCII_LF
            jsr         HFS_PUT

; The line (HFS_STAT, HFS_LEN bytes) onto the text in the data area (at SD_N); then it's empty again.
; Modifies: .A, .X, .Y
HFS_LINE_OUT:
            ldx         #0
            ldy         SD_N
:
            cpx         HFS_LEN
            beq         :+
            lda         HFS_STAT,X
            sta         (ZP_IO_REQ),Y
            inx
            iny
            bra         :-
:
            sty         SD_N
            stz         HFS_LEN
            rts

HFS_TEXTS:
HFS_S_LABEL:    .byte   "hydrafs label=", 0
HFS_S_FREE:     .byte   "free ", 0
HFS_S_KB_OF:    .byte   " KB of ", 0
HFS_S_KB:       .byte   " KB", 0
HFS_S_LOST:     .byte   "check: lost ", 0
HFS_S_UNMARKED: .byte   ", unmarked ", 0
HFS_S_TWICE:    .byte   ", twice ", 0
HFS_S_FIXED:    .byte   ", fixed", 0

; ****************************************************************************
; Progress, for a long card operation (a full format's free map, a check's passes): "10% 20% ... 100%",
; straight to the console as it goes (the storage task has no fds: WRITE_CHAR sends it to the serial port),
; then a new line.  Only for HFS_PG_MIN steps or more: a short one shows nothing.

HFS_PG_MIN          = 16

; Start: HFS_T4 = the steps.  HFS_PG_STEP = a tenth of them.  Modifies: .A, .X, .Y, HFS_T4
HFS_PG_START:
            lda         #$FF                                ; (Not shown)
            sta         HFS_PG_TENS
            lda         HFS_T4 + 3
            ora         HFS_T4 + 2
            ora         HFS_T4 + 1
            bne         :+
            lda         HFS_T4
            cmp         #HFS_PG_MIN
            bcc         @done
:
            lda         #0                                  ; HFS_T4 / 10, bit by bit (.A = the remainder)
            ldx         #32
@bit:
            asl         HFS_T4
            rol         HFS_T4 + 1
            rol         HFS_T4 + 2
            rol         HFS_T4 + 3
            rol
            cmp         #10
            bcc         :+
            sbc         #10
            inc         HFS_T4                              ; (Its low bit was 0: the quotient's bit)
:
            dex
            bne         @bit
            ldx         #3
:
            lda         HFS_T4,X
            sta         HFS_PG_STEP,X
            sta         HFS_PG_LEFT,X
            dex
            bpl         :-
            stz         HFS_PG_TENS

@done:
            rts

; A step done: every tenth, the next "N0% ".  Preserves .X, .Y.  Modifies: .A
HFS_PG_TICK:
            bit         HFS_PG_TENS
            bmi         @done                               ; (Not shown)
            lda         HFS_PG_LEFT                         ; One less to the next tenth
            bne         @dec0
            lda         HFS_PG_LEFT + 1
            bne         @dec1
            lda         HFS_PG_LEFT + 2
            bne         @dec2
            dec         HFS_PG_LEFT + 3
@dec2:
            dec         HFS_PG_LEFT + 2
@dec1:
            dec         HFS_PG_LEFT + 1
@dec0:
            dec         HFS_PG_LEFT
            lda         HFS_PG_LEFT
            ora         HFS_PG_LEFT + 1
            ora         HFS_PG_LEFT + 2
            ora         HFS_PG_LEFT + 3
            bne         @done
            phx
            ldx         #3                                  ; A tenth: the next one's steps
:
            lda         HFS_PG_STEP,X
            sta         HFS_PG_LEFT,X
            dex
            bpl         :-
            plx
            inc         HFS_PG_TENS
            lda         HFS_PG_TENS
            cmp         #10
            bcs         @done                               ; (100%: HFS_PG_END says it)
            ora         #'0'
            jsr         WRITE_CHAR
            lda         #'0'
            jsr         WRITE_CHAR
            lda         #'%'
            jsr         WRITE_CHAR
            lda         #' '
            jmp         WRITE_CHAR

@done:
            rts

; The end (done, or failed): "100%" and a new line, if progress was shown.  Keeps .A and C (and .X, .Y)
HFS_PG_END:
            bit         HFS_PG_TENS
            bmi         @done
            php
            pha
            lda         #'1'
            jsr         WRITE_CHAR
            lda         #'0'
            jsr         WRITE_CHAR
            jsr         WRITE_CHAR
            lda         #'%'
            jsr         WRITE_CHAR
            lda         #ASCII_CR
            jsr         WRITE_CHAR
            lda         #ASCII_LF
            jsr         WRITE_CHAR
            lda         #$FF
            sta         HFS_PG_TENS
            pla
            plp

@done:
            rts
