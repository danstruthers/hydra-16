; ****************************************************************************
; HydraFS's writing side (in hfs.s: the storage driver's second bank), ported from the old OS's fs/hfs_write.s:
; the metadata buffer, the free map, growing and freeing files, and the requests that change a disk: R_WRITE,
; R_CREATE, R_REMOVE, R_WSTAT.  (Format and label: format.s.)
;
; There's no journal (see docs/plans/HYDRAFS.md), but the writes go in a safe order: a cluster is marked in
; use before any entry points at it, and an entry stops pointing at clusters before they're marked free.
; So a crash can leave lost clusters (marked in use, but nothing's), never a cluster used twice.

.segment "CODE2"

HFS_BITS:   .byte   $01, $02, $04, $08, $10, $20, $40, $80

; ****************************************************************************
; The metadata buffer (HFS_META, 512 bytes): the free map, extent blocks, entries being written and the
; superblock go through it, so that a write's allocating doesn't evict the file's data from the block
; cache.  A block is changed there, and written back when another block is wanted (HFS_META_GET), or at the
; end of the request (HFS_FINISH), which forgets it too: nothing is kept between requests, so a raw write
; to #d, or a changed card, can't leave it out of date.

; HFS_PTR = the metadata buffer + HFS_OFS.  Modifies: .A
HFS_META_AT:
            lda         HFS_META
            clc
            adc         HFS_OFS
            sta         HFS_PTR
            lda         HFS_META + 1
            adc         HFS_OFS + 1
            sta         HFS_PTR + 1
            rts

; Does the metadata buffer hold block SD_LBA?  OUT: C = 0: it does.  Modifies: .A, .X
HFS_META_SAME:
            lda         HFS_MSTATE
            lsr                                             ; (C = HFS_MS_VALID)
            bcc         @no
            ldx         #3
:
            lda         SD_LBA,X
            cmp         HFS_MBLK,X
            bne         @no
            dex
            bpl         :-
            clc
            rts

@no:
            sec
            rts

; Make block SD_LBA of card HFS_CARD the one in the metadata buffer (the one there written back first, if
; it changed).  OUT: C = 0; or C = 1, .A = a card error.  Modifies: .A, .X, .Y
HFS_META_GET:
            jsr         HFS_META_SAME
            bcc         @done
            jsr         HFS_META_FLUSH
            bcs         @done
            jsr         HFS_META_BUF
            jsr         SD_READ_BLOCK
            bcs         @done
            lda         #HFS_MS_VALID
            jmp         HFS_META_TAG

@done:
            rts

; For a block about to be written whole: block SD_LBA takes the metadata buffer (the one there written back
; first), all zeros, and it counts as changed (HFS_META_NEW); or, for a free map block that isn't on the
; card yet (HFS_MAP_BIT), all zeros as it reads, and not changed (HFS_META_FRESH).
; OUT: C = 0; or C = 1, .A = a card error.  Modifies: .A, .X, .Y
HFS_META_NEW:
            lda         #HFS_MS_VALID | HFS_MS_DIRTY
            .byte       $2C                                 ; (bit abs: skips the lda)
HFS_META_FRESH:
            lda         #HFS_MS_VALID | HFS_MS_FRESH
            pha
            jsr         HFS_META_FLUSH
            bcs         @fail
            stz         HFS_OFS
            stz         HFS_OFS + 1
            jsr         HFS_META_AT
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
            pla
            jmp         HFS_META_TAG

@fail:
            tax
            pla
            txa
            rts

; The metadata buffer holds block SD_LBA, in state .A (HFS_MS_*).  OUT: C = 0.  Modifies: .A, .X
HFS_META_TAG:
            sta         HFS_MSTATE
            ldx         #3
:
            lda         SD_LBA,X
            sta         HFS_MBLK,X
            dex
            bpl         :-
            clc
            rts

; The metadata buffer's block has been changed.  Modifies: .A
HFS_META_CHANGED:
            lda         #HFS_MS_VALID | HFS_MS_DIRTY
            sta         HFS_MSTATE
            rts

; SD_BUF = the metadata buffer, SD_DEV = the card (for SD_READ_BLOCK, SD_WRITE_BLOCK).  Modifies: .A
HFS_META_BUF:
            lda         HFS_META
            sta         SD_BUF
            lda         HFS_META + 1
            sta         SD_BUF + 1
            lda         HFS_CARD
            sta         SD_DEV
            rts

; Write the metadata buffer's block back if it changed, and forget it (and the block cache forgets its
; block too: it may be the same one).  Keeps SD_LBA.
; OUT: C = 0; or C = 1, .A = a card error.  Modifies: .A, .X, .Y
HFS_META_FLUSH:
            bit         HFS_MSTATE
            bmi         :+
            clc                                             ; (Unchanged: nothing to write)
            rts
:
            ldx         #3                                  ; (SD_LBA is the caller's)
:
            lda         SD_LBA,X
            pha
            lda         HFS_MBLK,X
            sta         SD_LBA,X
            dex
            bpl         :-
            jsr         HFS_META_BUF
            jsr         SD_WRITE_BLOCK
            tay                                             ; (The error, if it failed)
            stz         HFS_MSTATE
            stz         SD_CVALID
            pla                                             ; (Pulls change N and Z, not C)
            sta         SD_LBA
            pla
            sta         SD_LBA + 1
            pla
            sta         SD_LBA + 2
            pla
            sta         SD_LBA + 3
            tya
            rts

; The end of every HydraFS request: the card's counters into its superblock if they changed, and the
; metadata buffer written back and forgotten.  Keeps .A and C; if writing fails, C = 1 and .A = its error.
HFS_FINISH:
            php
            pha
            jsr         HFS_SB_SAVE
            bcs         @fail
            jsr         HFS_META_FLUSH
            bcs         @fail
            stz         HFS_MSTATE
            pla
            plp
            rts

@fail:
            stz         HFS_MSTATE
            tax
            pla
            pla
            txa
            sec
            rts

; If the card's counters (HFS_V_FREE, HFS_V_HINT, HFS_V_QID, HFS_V_STAMP, HFS_V_MINIT) changed, put them in
; its superblock (in the metadata buffer).  OUT: C = 0; or C = 1, .A = a card error.  Modifies: .A, .X, .Y
HFS_SB_SAVE:
            lda         HFS_SBDIRTY
            bne         :+
            clc
            rts
:
            stz         HFS_SBDIRTY
            jsr         HFS_SB_GET
            bcs         @done
            V_TO_SB     HFS_SB_FREE, HFS_V_FREE
            V_TO_SB     HFS_SB_HINT, HFS_V_HINT
            V_TO_SB     HFS_SB_NEXT_QID, HFS_V_QID
            V_TO_SB     HFS_SB_STAMP, HFS_V_STAMP
            V_TO_SB     HFS_SB_MAPINIT, HFS_V_MINIT
            jsr         HFS_META_CHANGED
            clc

@done:
            rts

; Card HFS_CARD's superblock (block 0) into the metadata buffer, and HFS_PTR -> it.
; OUT: C = 0; or C = 1, .A = a card error.  Modifies: .A, .X, .Y
HFS_SB_GET:
            stz         SD_LBA
            stz         SD_LBA + 1
            stz         SD_LBA + 2
            stz         SD_LBA + 3
            jsr         HFS_META_GET
            bcs         @done
            stz         HFS_OFS
            stz         HFS_OFS + 1
            jsr         HFS_META_AT
            clc

@done:
            rts

; .X = the disk * 4: where its numbers are in the HFS_V_* arrays.  Modifies: .A
HFS_CARD_X:
            lda         HFS_CARD
            asl
            asl
            tax
            rts

; ****************************************************************************
; The card's counters, and entries

; A modification stamp into HFS_T4: the clock's time (TIME: seconds since 2000-01-01; r0-r3 kept, as the call
; changes them).  The disk's superblock keeps the latest (HFS_V_STAMP: it gets it at the end of the request).
; Modifies: .A, .X, .Y
HFS_TAKE_STAMP:
            ldx         #7                                  ; (r0-r3 kept)
:
            lda         r0,X
            pha
            dex
            bpl         :-
            jsr         TIME
            ldx         #3
:
            lda         r0,X
            sta         HFS_T4,X
            dex
            bpl         :-
            ldx         #0
:
            pla
            sta         r0,X
            inx
            cpx         #8
            bne         :-
            jsr         HFS_CARD_X
            ldy         #0
:
            lda         HFS_T4,Y
            sta         HFS_V_STAMP,X
            inx
            iny
            cpy         #4
            bne         :-
            bra         HFS_TAKEN

; Take the card's next qid id into HFS_T4, and count it on (the superblock gets it at the end of the
; request).  Modifies: .A, .X, .Y
HFS_TAKE_QID:
            jsr         HFS_CARD_X                          ; (.X = the counter: at HFS_V_QID,X)
            ldy         #0
:
            lda         HFS_V_QID,X
            sta         HFS_T4,Y
            inx
            iny
            cpy         #4
            bne         :-
            txa
            sec
            sbc         #4
            tax
            inc         HFS_V_QID,X
            bne         :+
            inc         HFS_V_QID + 1,X
            bne         :+
            inc         HFS_V_QID + 2,X
            bne         :+
            inc         HFS_V_QID + 3,X
:

HFS_TAKEN:
            lda         #1
            sta         HFS_SBDIRTY
            rts

; The entry at HFS_FP has changed: a new qid version and modification stamp.  Modifies: .A, .X, .Y
HFS_TOUCH:
            ldy         #HFS_E_QVER
            lda         (HFS_FP),Y
            clc
            adc         #1
            sta         (HFS_FP),Y
            iny
            lda         (HFS_FP),Y
            adc         #0
            sta         (HFS_FP),Y
            jsr         HFS_TAKE_STAMP
            ldy         #HFS_E_STAMP
            ldx         #0
:
            lda         HFS_T4,X
            sta         (HFS_FP),Y
            iny
            inx
            cpx         #4
            bne         :-
            rts

; Write the entry at HFS_FP to its place, HFS_LOC, on the card (through the metadata buffer), and give
; every open file's copy of that entry the same.  OUT: C = 0, .A = 0; or C = 1, .A = a card error.
; Modifies: .X, .Y
HFS_ENT_PUT:
            ldx         #3
:
            lda         HFS_LOC,X
            sta         SD_LBA,X
            dex
            bpl         :-
            jsr         HFS_META_GET
            bcs         @done
            jsr         HFS_LOC_OFS
            jsr         HFS_META_AT
            ldy         #HFS_ENTRY_SIZE - 1
:
            lda         (HFS_FP),Y
            sta         (HFS_PTR),Y
            dey
            bpl         :-
            jsr         HFS_META_CHANGED
            jsr         HFS_SYNC
            lda         #0
            clc

@done:
            rts

; Give every open file whose entry is the one at HFS_LOC (on card HFS_CARD) a copy of the entry at HFS_FP
; (which may be one of them).  Modifies: .A, .X, .Y
HFS_SYNC:
            ldx         #0

@find:
            jsr         HFS_SLOT_FIND
            bcs         @done
            ldy         #HFS_ENTRY_SIZE - 1
:
            lda         (HFS_FP),Y
            sta         (HFS_PTR),Y
            dey
            bpl         :-
            inx
            bra         @find

@done:
            rts

; Empty the entry at HFS_FP: size 0, no extents.  Modifies: .A, .Y
HFS_EMPTY:
            lda         #0
            ldy         #HFS_E_SIZE + 3
:
            sta         (HFS_FP),Y
            dey
            cpy         #HFS_E_SIZE - 1
            bne         :-
            ldy         #HFS_ENTRY_SIZE - 1
:
            sta         (HFS_FP),Y
            dey
            cpy         #HFS_E_EXT1 - 1
            bne         :-
            rts

; ****************************************************************************
; The free map: 1 bit per cluster (1 = in use), 4096 clusters per map block

; Find cluster HFS_C's bit in the free map (its map block into the metadata buffer).  A map block past the
; ones written (HFS_V_MINIT: a quick format's) isn't read: it's all free (HFS_META_FRESH), until it changes
; (HFS_MAP_CHANGED writes it then).
; OUT: C = 0: HFS_PTR -> its byte, .A = its bit; or C = 1, .A = a card error.  Modifies: .X, .Y
HFS_MAP_BIT:
            lda         HFS_C + 1                           ; SD_LBA = the map's first block + HFS_C >> 12
            lsr
            lsr
            lsr
            lsr
            sta         SD_LBA
            lda         HFS_C + 2
            asl
            asl
            asl
            asl
            ora         SD_LBA
            sta         SD_LBA
            lda         HFS_C + 2
            lsr
            lsr
            lsr
            lsr
            sta         SD_LBA + 1
            lda         HFS_C + 3
            asl
            asl
            asl
            asl
            ora         SD_LBA + 1
            sta         SD_LBA + 1
            lda         HFS_C + 3
            lsr
            lsr
            lsr
            lsr
            sta         SD_LBA + 2
            stz         SD_LBA + 3
            jsr         HFS_CARD_X
            lda         SD_LBA                              ; C = 1: a map block not written yet
            cmp         HFS_V_MINIT,X
            lda         SD_LBA + 1
            sbc         HFS_V_MINIT + 1,X
            lda         SD_LBA + 2
            sbc         HFS_V_MINIT + 2,X
            lda         SD_LBA + 3
            sbc         HFS_V_MINIT + 3,X
            php
            clc
            lda         SD_LBA
            adc         HFS_V_MAP,X
            sta         SD_LBA
            lda         SD_LBA + 1
            adc         HFS_V_MAP + 1,X
            sta         SD_LBA + 1
            lda         SD_LBA + 2
            adc         HFS_V_MAP + 2,X
            sta         SD_LBA + 2
            lda         SD_LBA + 3
            adc         HFS_V_MAP + 3,X
            sta         SD_LBA + 3
            plp
            bcs         @fresh
            jsr         HFS_META_GET
            bcs         @done
            bra         @at

@fresh:
            jsr         HFS_META_SAME                       ; (In the buffer already: as it is, changed or not)
            bcc         @at
            jsr         HFS_META_FRESH
            bcs         @done

@at:
            lda         HFS_C + 1                           ; HFS_OFS = (HFS_C & 4095) >> 3: its byte
            and         #$0F
            sta         HFS_OFS + 1
            lda         HFS_C
            lsr         HFS_OFS + 1
            ror
            lsr         HFS_OFS + 1
            ror
            lsr         HFS_OFS + 1
            ror
            sta         HFS_OFS
            jsr         HFS_META_AT
            lda         HFS_C
            and         #7
            tax
            lda         HFS_BITS,X
            clc

@done:
            rts

; Mark cluster HFS_C in use in the free map, and count it off the card's free clusters.
; OUT: C = 0; or C = 1, .A = a card error.  Modifies: .A, .X, .Y
HFS_MAP_SET:
            jsr         HFS_MAP_BIT
            bcs         @done
            ora         (HFS_PTR)
            sta         (HFS_PTR)
            jsr         HFS_MAP_CHANGED                     ; (.X = the card * 4)
            bcs         @done
            sec
            lda         HFS_V_FREE,X
            sbc         #1
            sta         HFS_V_FREE,X
            lda         HFS_V_FREE + 1,X
            sbc         #0
            sta         HFS_V_FREE + 1,X
            lda         HFS_V_FREE + 2,X
            sbc         #0
            sta         HFS_V_FREE + 2,X
            lda         HFS_V_FREE + 3,X
            sbc         #0
            sta         HFS_V_FREE + 3,X
            clc

@done:
            rts

; Mark cluster HFS_C free in the free map, and count it back in.  OUT: as HFS_MAP_SET
HFS_MAP_CLR:
            jsr         HFS_MAP_BIT
            bcs         @done
            eor         #$FF
            and         (HFS_PTR)
            sta         (HFS_PTR)
            jsr         HFS_MAP_CHANGED
            bcs         @done
            inc         HFS_V_FREE,X
            bne         :+
            inc         HFS_V_FREE + 1,X
            bne         :+
            inc         HFS_V_FREE + 2,X
            bne         :+
            inc         HFS_V_FREE + 3,X
:
            clc

@done:
            rts

; (The map block in the metadata buffer and the free count changed.)  A map block that isn't on the card yet
; goes on it first (HFS_MAP_WRITTEN); if that fails, the change is dropped.
; OUT: C = 0: .X = the card * 4; or C = 1, .A = a card error.  Modifies: .A, .Y (writing a map block)
HFS_MAP_CHANGED:
            bit         HFS_MSTATE                          ; (V = HFS_MS_FRESH)
            bvc         :+
            jsr         HFS_MAP_WRITTEN
            bcc         :+
            stz         HFS_MSTATE                          ; (Forgotten: not written)
            rts
:
            jsr         HFS_META_CHANGED                    ; (Not fresh any more)
            lda         #1
            sta         HFS_SBDIRTY
            jsr         HFS_CARD_X
            clc
            rts

.assert     HFS_MS_FRESH = $40, error, "HFS_MAP_CHANGED tests HFS_MS_FRESH with bit (V)"

; A free map block in the metadata buffer that isn't on the card yet (HFS_MS_FRESH) is about to change: zeros
; go on the card for it, and for the map blocks before it that aren't there either, and then the superblock's
; count of map blocks written (HFS_SB_MAPINIT) takes it in, before its change can be written.  So the card
; never has a map block with bits in it past the count (which would read as free: used twice), and a crash
; can only leave a block of zeros counted.  Through HFS_ZBUF, so the metadata buffer and the block cache's
; contents stay as they are.  OUT: C = 0; or C = 1, .A = a card error.  Keeps HFS_PTR, HFS_OFS.
; Modifies: .A, .X, .Y
HFS_MAP_WRITTEN:
            jsr         HFS_CARD_X                          ; HFS_MW = the block's place in the map
            sec
            ldy         #0
:
            lda         HFS_MBLK,Y
            sbc         HFS_V_MAP,X
            sta         HFS_MW,Y
            inx
            iny
            tya                                             ; (Keeps C)
            eor         #4
            bne         :-
            jsr         @zbuf                               ; Zeros
            ldy         #0
            tya
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

@block:                                                     ; Map blocks HFS_V_MINIT ... HFS_MW
            jsr         HFS_CARD_X
            lda         HFS_MW                              ; (Past HFS_MW: done)
            cmp         HFS_V_MINIT,X
            lda         HFS_MW + 1
            sbc         HFS_V_MINIT + 1,X
            lda         HFS_MW + 2
            sbc         HFS_V_MINIT + 2,X
            lda         HFS_MW + 3
            sbc         HFS_V_MINIT + 3,X
            bcc         @count
            clc                                             ; SD_LBA = the map's first block + it
            ldy         #0
:
            lda         HFS_V_MAP,X
            adc         HFS_V_MINIT,X
            sta         SD_LBA,Y
            inx
            iny
            tya
            eor         #4
            bne         :-
            jsr         SD_WRITE_BLOCK
            bcs         @done
            jsr         HFS_CARD_X                          ; Written: counted
            inc         HFS_V_MINIT,X
            bne         @block
            inc         HFS_V_MINIT + 1,X
            bne         @block
            inc         HFS_V_MINIT + 2,X
            bne         @block
            inc         HFS_V_MINIT + 3,X
            bra         @block

@count:                                                     ; The superblock, with the new count: read it,
            stz         SD_LBA                              ;   put the count in, write it
            stz         SD_LBA + 1
            stz         SD_LBA + 2
            stz         SD_LBA + 3
            jsr         SD_READ_BLOCK
            bcs         @done
            jsr         HFS_CARD_X
            ldy         #HFS_SB_MAPINIT
:
            lda         HFS_V_MINIT,X
            sta         (SD_BUF),Y
            inx
            iny
            cpy         #HFS_SB_MAPINIT + 4
            bne         :-
            jsr         SD_WRITE_BLOCK
            stz         SD_CVALID                           ; (The cache may have held one of these blocks)

@done:
            rts

@zbuf:                                                      ; SD_BUF = HFS_ZBUF, SD_DEV = the card
            lda         HFS_ZBUF
            sta         SD_BUF
            lda         HFS_ZBUF + 1
            sta         SD_BUF + 1
            lda         HFS_CARD
            sta         SD_DEV
            rts

; Is HFS_C one of the card's clusters (under its count)?  OUT: C = 0: it is.  Modifies: .A, .X
HFS_C_OK:
            jsr         HFS_CARD_X
            lda         HFS_C
            cmp         HFS_V_CLUSTERS,X
            lda         HFS_C + 1
            sbc         HFS_V_CLUSTERS + 1,X
            lda         HFS_C + 2
            sbc         HFS_V_CLUSTERS + 2,X
            lda         HFS_C + 3
            sbc         HFS_V_CLUSTERS + 3,X
            rts

; HFS_C = 0.  Modifies: nothing else
HFS_C_ZERO:
            stz         HFS_C
            stz         HFS_C + 1
            stz         HFS_C + 2
            stz         HFS_C + 3
            rts

; Allocate a cluster: HFS_C, if it's free (the one after a file's last, so the file stays in one piece),
; or else the first free one from the card's hint on (round to the start).  It's marked in use.
; IN: HFS_C = the one wanted ($FFFFFFFF: any)
; OUT: C = 0: HFS_C = the cluster; or C = 1, .A = E_NOSPC or a card error.  Modifies: .A, .X, .Y
HFS_ALLOC:
            jsr         HFS_C_OK
            bcs         @scan
            jsr         HFS_MAP_BIT
            bcc         @far17
            jmp         @done
@far17:
            and         (HFS_PTR)
            bne         @far16
            jmp         @take
@far16:

@scan:
            jsr         HFS_CARD_X                          ; From the hint (HFS_C), and all the clusters
            ldy         #0                                  ;   (HFS_N4) at most
:
            lda         HFS_V_HINT,X
            sta         HFS_C,Y
            lda         HFS_V_CLUSTERS,X
            sta         HFS_N4,Y
            inx
            iny
            cpy         #4
            bne         :-
            jsr         HFS_C_OK
            bcc         @look
            jsr         HFS_C_ZERO

@look:
            jsr         HFS_MAP_BIT
            bcc         @far15
            jmp         @done
@far15:
            sta         HFS_T4                              ; (Its bit)
            and         (HFS_PTR)
            bne         @used
            jsr         HFS_C_OK                            ; (A bit past the last cluster isn't one)
            bcc         @take

@used:
            ldx         #1                                  ; On by 1; or by 8 from a byte's first bit,
            lda         HFS_T4                              ;   when the whole byte is in use
            cmp         #$01
            bne         @step
            lda         (HFS_PTR)
            cmp         #$FF
            bne         @step
            ldx         #8

@step:
            stx         HFS_T4
            clc
            lda         HFS_C
            adc         HFS_T4
            sta         HFS_C
            bcc         :+
            inc         HFS_C + 1
            bne         :+
            inc         HFS_C + 2
            bne         :+
            inc         HFS_C + 3
:
            sec
            lda         HFS_N4
            sbc         HFS_T4
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
            bcc         @full                               ; (Every one looked at)
            ora         HFS_N4 + 2
            ora         HFS_N4 + 1
            ora         HFS_N4
            beq         @full
            jsr         HFS_C_OK
            bcc         @look
            jsr         HFS_C_ZERO                          ; (Past the last: round to the first)
            bra         @look

@take:
            jsr         HFS_MAP_SET
            bcs         @done
            jsr         HFS_CARD_X                          ; The hint: the one after it
            clc
            lda         HFS_C
            adc         #1
            sta         HFS_V_HINT,X
            lda         HFS_C + 1
            adc         #0
            sta         HFS_V_HINT + 1,X
            lda         HFS_C + 2
            adc         #0
            sta         HFS_V_HINT + 2,X
            lda         HFS_C + 3
            adc         #0
            sta         HFS_V_HINT + 3,X
            clc

@done:
            rts

@full:
            lda         #E_NOSPC
            sec
            rts

; SD_LBA = cluster HFS_C's first block: the data area + HFS_C * 8.  Modifies: .A, .X
HFS_C_LBA:
            ldx         #3
:
            lda         HFS_C,X
            sta         SD_LBA,X
            dex
            bpl         :-
            ldx         #HFS_CSHIFT
:
            asl         SD_LBA
            rol         SD_LBA + 1
            rol         SD_LBA + 2
            rol         SD_LBA + 3
            dex
            bne         :-
            jsr         HFS_CARD_X
            clc
            lda         SD_LBA
            adc         HFS_V_DATA,X
            sta         SD_LBA
            lda         SD_LBA + 1
            adc         HFS_V_DATA + 1,X
            sta         SD_LBA + 1
            lda         SD_LBA + 2
            adc         HFS_V_DATA + 2,X
            sta         SD_LBA + 2
            lda         SD_LBA + 3
            adc         HFS_V_DATA + 3,X
            sta         SD_LBA + 3
            rts

; ****************************************************************************
; Growing a file

; Give the file whose entry is at HFS_FP (at HFS_LOC) a cluster more at its end.  Its last extent grows if
; the cluster after it is free; or else the cluster starts a new extent (HFS_APPEND_EXT).  Then the entry is
; written (with the file's size as it is now).
; OUT: C = 0; or C = 1, .A = E_NOSPC or a card error.  Modifies: .A, .X, .Y
HFS_GROW:
            jsr         HFS_LAST_EXT
            bcc         @far4
            jmp         @done
@far4:
            lda         #$FF                                ; HFS_D = the cluster after the last extent
            sta         HFS_D                               ;   (none, or it can't grow: $FFFFFFFF, which
            sta         HFS_D + 1                           ;   is never free)
            sta         HFS_D + 2
            sta         HFS_D + 3
            lda         HFS_XLEN
            ora         HFS_XLEN + 1
            beq         @want
            lda         HFS_XLEN
            and         HFS_XLEN + 1
            cmp         #$FF
            beq         @want                               ; (As long as an extent can be)
            lda         HFS_XCL
            and         HFS_XCL + 1
            and         HFS_XCL + 2
            and         HFS_XCL + 3
            cmp         #HFS_HOLE
            beq         @want                               ; (A hole: its clusters aren't the card's)
            clc
            lda         HFS_XCL
            adc         HFS_XLEN
            sta         HFS_D
            lda         HFS_XCL + 1
            adc         HFS_XLEN + 1
            sta         HFS_D + 1
            lda         HFS_XCL + 2
            adc         #0
            sta         HFS_D + 2
            lda         HFS_XCL + 3
            adc         #0
            sta         HFS_D + 3

@want:
            ldx         #3                                  ; Ask for that one
:
            lda         HFS_D,X
            sta         HFS_C,X
            dex
            bpl         :-
            jsr         HFS_ALLOC
            bcs         @done
            ldx         #3                                  ; Got it?  The last extent grows
:
            lda         HFS_C,X
            cmp         HFS_D,X
            bne         @new
            dex
            bpl         :-
            inc         HFS_XLEN
            bne         :+
            inc         HFS_XLEN + 1
:
            jsr         HFS_LAST_PTR
            bcs         @done
            ldy         #4                                  ; (Its length: after its first cluster)
            lda         HFS_XLEN
            sta         (HFS_PTR),Y
            iny
            lda         HFS_XLEN + 1
            sta         (HFS_PTR),Y
            jmp         HFS_ENT_PUT

@new:
            jsr         HFS_XNEW_C
            jmp         HFS_APPEND_EXT

@done:
            rts

; A new extent, HFS_XNEW, after the file's last one (HFS_LAST_EXT found it): the entry's first or second,
; or in its last extent block, or in a new extent block (in a cluster of its own) when that's full or it
; has none.  OUT: C = 0; or C = 1, .A = error.  Modifies: .A, .X, .Y
HFS_APPEND_EXT:
            lda         HFS_LASTB
            ora         HFS_LASTB + 1
            ora         HFS_LASTB + 2
            ora         HFS_LASTB + 3
            bne         @in_block
            ldy         #HFS_E_EXT1
            lda         HFS_LASTO
            beq         @in_entry                           ; (No extents yet: the first)
            ldy         #HFS_E_EXT2
            cmp         #HFS_E_EXT1
            beq         @in_entry                           ; (The first is used: the second)
            jsr         HFS_EXT_BLOCK_NEW                   ; Both are: its first extent block
            bcc         @far14
            jmp         @done
@far14:
            ldy         #HFS_E_EXTBLK                       ; (SD_LBA = the block)
            ldx         #0
:
            lda         SD_LBA,X
            sta         (HFS_FP),Y
            iny
            inx
            cpx         #4
            bne         :-
            jmp         HFS_ENT_PUT

@in_entry:
            tya                                             ; HFS_PTR -> the extent in the entry
            clc
            adc         HFS_FP
            sta         HFS_PTR
            lda         HFS_FP + 1
            adc         #0
            sta         HFS_PTR + 1
            jsr         HFS_EXT_PUT
            jmp         HFS_ENT_PUT

@in_block:
            lda         HFS_LASTO                           ; Room in the last extent block?
            cmp         #<(HFS_X_FIRST + (HFS_X_MAX - 1) * HFS_EXT_SIZE)
            lda         HFS_LASTO + 1
            sbc         #>(HFS_X_FIRST + (HFS_X_MAX - 1) * HFS_EXT_SIZE)
            bcc         @append
            jsr         HFS_EXT_BLOCK_NEW                   ; No: a new one after it
            bcs         @done
            ldx         #3                                  ; (HFS_T4 = the new block)
:
            lda         SD_LBA,X
            sta         HFS_T4,X
            dex
            bpl         :-
            jsr         HFS_LASTB_GET                       ; The last one points at it (and is written
            bcs         @done                               ;   after it)
            stz         HFS_OFS
            stz         HFS_OFS + 1
            jsr         HFS_META_AT
            ldy         #HFS_X_NEXT + 3
            ldx         #3
:
            lda         HFS_T4,X
            sta         (HFS_PTR),Y
            dey
            dex
            bpl         :-
            jsr         HFS_META_CHANGED
            clc
            rts

@append:
            jsr         HFS_LASTB_GET
            bcs         @done
            lda         #HFS_X_COUNT                        ; One more extent in it
            sta         HFS_OFS
            stz         HFS_OFS + 1
            jsr         HFS_META_AT
            lda         (HFS_PTR)
            inc
            sta         (HFS_PTR)
            clc                                             ; The new one: after the last
            lda         HFS_LASTO
            adc         #HFS_EXT_SIZE
            sta         HFS_OFS
            lda         HFS_LASTO + 1
            adc         #0
            sta         HFS_OFS + 1
            jsr         HFS_META_AT
            jsr         HFS_EXT_PUT
            jsr         HFS_META_CHANGED
            clc

@done:
            rts

; The extent HFS_XNEW at HFS_PTR.  Modifies: .A, .Y
HFS_EXT_PUT:
            ldy         #0
:
            lda         HFS_XNEW,Y
            sta         (HFS_PTR),Y
            iny
            cpy         #HFS_EXT_SIZE
            bne         :-
            rts

; HFS_XNEW = {HFS_C, 1 cluster}.  Modifies: .A, .X
HFS_XNEW_C:
            ldx         #3
:
            lda         HFS_C,X
            sta         HFS_XNEW,X
            dex
            bpl         :-
            lda         #1
            sta         HFS_XNEW + 4
            stz         HFS_XNEW + 5
            rts

; A new extent block, holding one extent, HFS_XNEW, in a cluster of its own (its first block).
; OUT: C = 0: SD_LBA = the block (in the metadata buffer, to be written); or C = 1, .A = error.
; Modifies: .A, .X, .Y
HFS_EXT_BLOCK_NEW:
            ldx         #3                                  ; HFS_D = the data cluster, while another is
:                                                           ;   allocated for the block
            lda         HFS_C,X
            sta         HFS_D,X
            lda         #$FF
            sta         HFS_C,X
            dex
            bpl         :-
            jsr         HFS_ALLOC
            bcs         @done
            jsr         HFS_C_LBA
            jsr         HFS_META_NEW                        ; (All zeros: no next block)
            bcs         @done
            ldx         #3
:
            lda         HFS_D,X
            sta         HFS_C,X
            dex
            bpl         :-
            lda         #HFS_X_COUNT                        ; One extent in it
            sta         HFS_OFS
            stz         HFS_OFS + 1
            jsr         HFS_META_AT
            lda         #1
            sta         (HFS_PTR)
            lda         #HFS_X_FIRST
            sta         HFS_OFS
            jsr         HFS_META_AT
            jsr         HFS_EXT_PUT
            clc

@done:
            rts

; Find the last extent of the file whose entry is at HFS_FP: HFS_XCL / HFS_XLEN = it; HFS_LASTB = the
; extent block it's in (0: the entry), HFS_LASTO = its offset there (0: the file has no extents; in an
; extent block with none in it, the offset before its first).
; OUT: C = 0; or C = 1, .A = a card error.  Modifies: .A, .X, .Y
HFS_LAST_EXT:
            ldx         #HFS_EXT_SIZE - 1                   ; (None yet)
:
            stz         HFS_XCL,X
            dex
            bpl         :-
            ldx         #3
:
            stz         HFS_LASTB,X
            dex
            bpl         :-
            stz         HFS_LASTO
            stz         HFS_LASTO + 1
            ldy         #HFS_E_EXTBLK                       ; Extent blocks?  Then the last one's last
            ldx         #0
:
            lda         (HFS_FP),Y
            sta         HFS_XBLK,X
            iny
            inx
            cpx         #4
            bne         :-
            lda         HFS_XBLK
            ora         HFS_XBLK + 1
            ora         HFS_XBLK + 2
            ora         HFS_XBLK + 3
            bne         @chain
            ldy         #HFS_E_EXT2 + 4                     ; Or else the entry's second, or its first
            lda         (HFS_FP),Y
            iny
            ora         (HFS_FP),Y
            beq         :+
            lda         #HFS_E_EXT2
            bra         @in_entry
:
            ldy         #HFS_E_EXT1 + 4
            lda         (HFS_FP),Y
            iny
            ora         (HFS_FP),Y
            beq         @none
            lda         #HFS_E_EXT1

@in_entry:
            sta         HFS_LASTO
            tay
            ldx         #0
:
            lda         (HFS_FP),Y
            sta         HFS_XCL,X
            iny
            inx
            cpx         #HFS_EXT_SIZE
            bne         :-

@none:
            clc
            rts

@chain:
            ldx         #3                                  ; Down the chain to its last block
:
            lda         HFS_XBLK,X
            sta         SD_LBA,X
            dex
            bpl         :-
            jsr         HFS_LOAD
            bcc         @far13
            jmp         @done
@far13:
            ldy         #HFS_X_NEXT + 3
            lda         (SD_CACHE),Y
            dey
            ora         (SD_CACHE),Y
            dey
            ora         (SD_CACHE),Y
            dey
            ora         (SD_CACHE),Y
            beq         @last
            ldx         #0                                  ; (.Y = HFS_X_NEXT)
:
            lda         (SD_CACHE),Y
            sta         HFS_XBLK,X
            iny
            inx
            cpx         #4
            bne         :-
            bra         @chain

@last:
            ldx         #3
:
            lda         HFS_XBLK,X
            sta         HFS_LASTB,X
            dex
            bpl         :-
            ldy         #HFS_X_COUNT
            lda         (SD_CACHE),Y                        ; (84 at most)
            bne         :+
            lda         #HFS_X_FIRST - HFS_EXT_SIZE         ; None in it: as if there were one before the
            sta         HFS_LASTO                           ;   first
            clc
            rts
:
            dec                                             ; HFS_LASTO = HFS_X_FIRST + (count - 1) * 6
            sta         HFS_LASTO
            asl         HFS_LASTO                           ; (* 2 ...
            rol         HFS_LASTO + 1
            lda         HFS_LASTO
            ldx         HFS_LASTO + 1
            asl         HFS_LASTO                           ;   + * 4)
            rol         HFS_LASTO + 1
            clc
            adc         HFS_LASTO
            sta         HFS_LASTO
            txa
            adc         HFS_LASTO + 1
            sta         HFS_LASTO + 1
            lda         HFS_LASTO
            clc
            adc         #HFS_X_FIRST
            sta         HFS_LASTO
            bcc         :+
            inc         HFS_LASTO + 1
:
            lda         SD_CACHE                            ; The extent itself
            clc
            adc         HFS_LASTO
            sta         HFS_PTR
            lda         SD_CACHE + 1
            adc         HFS_LASTO + 1
            sta         HFS_PTR + 1
            ldy         #0
:
            lda         (HFS_PTR),Y
            sta         HFS_XCL,Y
            iny
            cpy         #HFS_EXT_SIZE
            bne         :-
            clc

@done:
            rts

; HFS_PTR -> the last extent HFS_LAST_EXT found: in the entry (HFS_FP), or in its extent block, which is
; read into the metadata buffer and counted as changed (the caller changes it).
; OUT: C = 0; or C = 1, .A = a card error.  Modifies: .A, .X, .Y
HFS_LAST_PTR:
            lda         HFS_LASTB
            ora         HFS_LASTB + 1
            ora         HFS_LASTB + 2
            ora         HFS_LASTB + 3
            bne         @block
            lda         HFS_FP
            clc
            adc         HFS_LASTO
            sta         HFS_PTR
            lda         HFS_FP + 1
            adc         #0
            sta         HFS_PTR + 1
            clc
            rts

@block:
            jsr         HFS_LASTB_GET
            bcs         @done
            lda         HFS_LASTO
            sta         HFS_OFS
            lda         HFS_LASTO + 1
            sta         HFS_OFS + 1
            jsr         HFS_META_AT
            jsr         HFS_META_CHANGED
            clc

@done:
            rts

; The file's last extent block (HFS_LASTB) into the metadata buffer.  OUT: C = 0; or C = 1, .A = error
HFS_LASTB_GET:
            ldx         #3
:
            lda         HFS_LASTB,X
            sta         SD_LBA,X
            dex
            bpl         :-
            jmp         HFS_META_GET

; ****************************************************************************
; Freeing a file's clusters

; Empty the file whose entry is at HFS_FP (at HFS_LOC).  The entry is written first, with no extents and
; size 0 (and a new qid version and stamp), then its clusters are freed, so a crash between them leaves
; lost clusters, not ones in use twice.  OUT: C = 0; or C = 1, .A = error.  Modifies: .A, .X, .Y
HFS_TRUNCATE:
            ldy         #HFS_ENTRY_SIZE - 1                 ; Its extents, to free after
:
            lda         (HFS_FP),Y
            sta         HFS_NEWE,Y
            dey
            bpl         :-
            jsr         HFS_EMPTY
            jsr         HFS_TOUCH
            jsr         HFS_ENT_PUT
            bcc         HFS_FREE_OLD
            rts

; Free the clusters of the extents in the entry saved in HFS_NEWE (HFS_FP stays as it is).
; OUT: C = 0; or C = 1, .A = error.  Modifies: .A, .X, .Y
HFS_FREE_OLD:
            lda         HFS_FP
            pha
            lda         HFS_FP + 1
            pha
            LOAD_ADDR   HFS_NEWE, HFS_FP
            jsr         HFS_FREE_ALL
            tax                                             ; (Pulls change N and Z, not C)
            pla
            sta         HFS_FP + 1
            pla
            sta         HFS_FP
            txa
            rts

; Free the clusters of the file whose entry is at HFS_FP, and its extent blocks' (the entry itself isn't
; changed).  OUT: C = 0; or C = 1, .A = a card error.  Modifies: .A, .X, .Y
HFS_FREE_ALL:
            LOAD_ADDR   HFS_FREE_RUN, HFS_RUN_VEC

; Call the routine at HFS_RUN_VEC for each run of clusters the file whose entry is at HFS_FP uses: its
; extents (HFS_XCL = the first cluster, HFS_XLEN = how many), then each extent block's own cluster (a run of
; 1).  The routine returns C = 1 (.A = the error) to stop.  (HydraFS's check uses it too, to mark them.)
; OUT: C = 0; or C = 1, .A = error.  Modifies: .A, .X, .Y
HFS_EACH_RUN:
            lda         HFS_FP
            sta         HFS_XP
            lda         HFS_FP + 1
            sta         HFS_XP + 1
            ldy         #HFS_E_EXT1
            jsr         HFS_EXT_RUN
            bcc         :+
            jmp         @done
:
            ldy         #HFS_E_EXT2
            jsr         HFS_EXT_RUN
            bcc         :+
            jmp         @done
:
            ldy         #HFS_E_EXTBLK
            ldx         #0
:
            lda         (HFS_FP),Y
            sta         HFS_XBLK,X
            iny
            inx
            cpx         #4
            bne         :-

@block:
            lda         HFS_XBLK
            ora         HFS_XBLK + 1
            ora         HFS_XBLK + 2
            ora         HFS_XBLK + 3
            bne         @far11
            jmp         @all
@far11:
            ldx         #3
:
            lda         HFS_XBLK,X
            sta         SD_LBA,X
            dex
            bpl         :-
            jsr         HFS_LOAD                            ; (Into the cache: the free map goes through the
            bcc         :+                                  ;   other buffer)
            jmp         @done
:
            ldy         #HFS_X_COUNT
            lda         (SD_CACHE),Y
            sta         SD_TMP
            lda         SD_CACHE
            clc
            adc         #HFS_X_FIRST
            sta         HFS_XP
            lda         SD_CACHE + 1
            adc         #0
            sta         HFS_XP + 1

@ext:
            lda         SD_TMP
            beq         @own
            dec         SD_TMP
            ldy         #0
            jsr         HFS_EXT_RUN
            bcs         @done
            lda         HFS_XP
            clc
            adc         #HFS_EXT_SIZE
            sta         HFS_XP
            bcc         @ext
            inc         HFS_XP + 1
            bra         @ext

@own:
            jsr         HFS_CARD_X                          ; The block's own cluster: (it - the data area) / 8
            sec
            lda         HFS_XBLK
            sbc         HFS_V_DATA,X
            sta         HFS_XCL
            lda         HFS_XBLK + 1
            sbc         HFS_V_DATA + 1,X
            sta         HFS_XCL + 1
            lda         HFS_XBLK + 2
            sbc         HFS_V_DATA + 2,X
            sta         HFS_XCL + 2
            lda         HFS_XBLK + 3
            sbc         HFS_V_DATA + 3,X
            sta         HFS_XCL + 3
            ldx         #HFS_CSHIFT
:
            lsr         HFS_XCL + 3
            ror         HFS_XCL + 2
            ror         HFS_XCL + 1
            ror         HFS_XCL
            dex
            bne         :-
            lda         #1
            sta         HFS_XLEN
            stz         HFS_XLEN + 1
            jsr         HFS_RUN
            bcs         @done
            ldy         #HFS_X_NEXT                         ; The next block (the cache still holds this one,
            ldx         #0                                  ;   though it may have forgotten which it is)
:
            lda         (SD_CACHE),Y
            sta         HFS_XBLK,X
            iny
            inx
            cpx         #4
            bne         :-
            jmp         @block

@all:
            clc

@done:
            rts

; The extent at (HFS_XP),Y, to the routine (an unused one, with no clusters, isn't; nor is a hole, whose
; clusters aren't the card's).  OUT: as the routine's
HFS_EXT_RUN:
            ldx         #0
:
            lda         (HFS_XP),Y
            sta         HFS_XCL,X
            iny
            inx
            cpx         #HFS_EXT_SIZE
            bne         :-
            lda         HFS_XLEN
            ora         HFS_XLEN + 1
            beq         @none
            lda         HFS_XCL
            and         HFS_XCL + 1
            and         HFS_XCL + 2
            and         HFS_XCL + 3
            cmp         #HFS_HOLE
            bne         HFS_RUN

@none:
            clc
            rts

HFS_RUN:
            jmp         (HFS_RUN_VEC)

; HFS_EACH_RUN's routine for freeing: HFS_XLEN clusters from HFS_XCL, marked free.
; OUT: C = 0; or C = 1, .A = a card error.  Modifies: .A, .X, .Y
HFS_FREE_RUN:
            ldx         #3
:
            lda         HFS_XCL,X
            sta         HFS_C,X
            dex
            bpl         :-

@one:
            lda         HFS_XLEN
            ora         HFS_XLEN + 1
            beq         @freed
            jsr         HFS_MAP_CLR
            bcs         @done
            inc         HFS_C
            bne         :+
            inc         HFS_C + 1
            bne         :+
            inc         HFS_C + 2
            bne         :+
            inc         HFS_C + 3
:
            lda         HFS_XLEN
            bne         :+
            dec         HFS_XLEN + 1
:
            dec         HFS_XLEN
            bra         @one

@freed:
            clc

@done:
            rts

; Shift the 4 bytes at HFS_C + .X right by .Y bits.  Modifies: .Y
HFS_SHR:
            lsr         HFS_C + 3,X
            ror         HFS_C + 2,X
            ror         HFS_C + 1,X
            ror         HFS_C,X
            dey
            bne         HFS_SHR
            rts

; ****************************************************************************
; The requests that change a disk

; R_WRITE: at the request's offset (at the end of the file if it's append-only), the part of a block at a time;
; the file gets a cluster more when it needs one (HFS_GROW).  A write past the end of the file makes it that
; long first, with zeros (and a hole: HFS_EXTEND); a write into a hole fills its cluster in (HFS_FILL).
HFS_WRITE_REQ:
            jsr         HFS_FID_CHECK
            bcc         :+
            rts
:
            ldy         #HFS_E_MODE                         ; (A directory is never opened for writing: the
            lda         (HFS_FP),Y                          ;   fid's mode says whether it may be written)
            bpl         :+
            lda         #E_ISDIR
            sec
            rts
:
            lda         HFS_H_FLAGS,X                       ; Its first write since it was opened: a new qid
            bne         :+                                  ;   version and stamp (the entry goes to the disk
            lda         #HFS_HF_WRITTEN | HFS_HF_DIRTY      ;   when it's clunked, or gets a cluster)
            sta         HFS_H_FLAGS,X
            jsr         HFS_TOUCH
:
            jsr         HFS_REQ_ARGS
            ldy         #HFS_E_MODE
            lda         (HFS_FP),Y
            and         #HFS_M_APPEND
            beq         @where
            ldy         #HFS_E_SIZE                         ; Append-only: at its end, wherever the fid is
            ldx         #0
:
            lda         (HFS_FP),Y
            sta         SD_POS,X
            iny
            inx
            cpx         #4
            bne         :-

@where:
            lda         SD_LEFT                             ; (Nothing to write: nothing changes)
            ora         SD_LEFT + 1
            beq         @written
            jsr         HFS_TAIL                            ; (C = 0: SD_POS is past the end: a gap)
            bcs         :+
            jsr         HFS_EXTEND                          ; The file that long first, with zeros
            bcs         HFS_W_ERROR
:
            jsr         HFS_W_RANGE
            bcs         HFS_W_ERROR

@written:
            jsr         HFS_SYNC                            ; All written: the other copies of the entry get
            jmp         HFS_READ_DONE                       ;   its size too; the count done goes back

; An error, part way (the disk full, say): what was written stays, and is the answer, if there was any (the next
; request gets the error); or the error
HFS_W_ERROR:
            pha
            jsr         HFS_SYNC
            pla
            jmp         HFS_READ_ERR

; Write SD_LEFT bytes from the client (at SD_DONE in its buffer; or zeros, if HFS_WZERO says so: HFS_EXTEND) to
; the file whose entry is at HFS_FP, at SD_POS, the part of a block at a time; SD_POS, SD_DONE and SD_LEFT go on.
; OUT: C = 0; or C = 1, .A = error.  Modifies: .A, .X, .Y
HFS_W_RANGE:
            lda         SD_LEFT
            ora         SD_LEFT + 1
            bne         :+
            clc
            rts
:
            jsr         HFS_FILE_BLOCK                      ; The block byte SD_POS goes in: a cluster more
            bcc         @have                               ;   first, if the file has none there yet, or
            cmp         #HFS_EOF                            ;   the hole's cluster filled in
            beq         @grow
            cmp         #HFS_IN_HOLE
            beq         :+
            jmp         @error
:
            jsr         HFS_FILL
            bra         @again

@grow:
            jsr         HFS_GROW

@again:
            bcc         :+
            jmp         @error
:
            jsr         HFS_FILE_BLOCK
            bcc         @have
            jmp         @error

@have:
            jsr         HFS_BLOCK_N                         ; SD_N = the bytes to this block's end ...
            stz         HFS_CL + 2                          ; ... but no more than the count has
            stz         HFS_CL + 3
            lda         SD_LEFT
            sta         HFS_CL
            lda         SD_LEFT + 1
            sta         HFS_CL + 1
            jsr         HFS_N_MIN
            lda         SD_POS                              ; A block all past the end of the file isn't read
            bne         @load                               ;   first: none of its bytes are the file's
            lda         SD_POS + 1
            and         #1
            bne         @load
            jsr         HFS_AT_END
            bcc         @load
            stz         SD_CVALID                           ; (The block buffer is about to hold it)
            bra         @copy

@load:
            jsr         HFS_LOAD
            bcc         @copy
            jmp         @error

@copy:
            lda         SD_POS                              ; SD_SRC = the block buffer + (SD_POS & 511)
            clc
            adc         SD_CACHE
            sta         SD_SRC
            lda         SD_POS + 1
            and         #1
            adc         SD_CACHE + 1
            sta         SD_SRC + 1
            bit         HFS_WZERO
            bmi         @zeros
            MOVR        r0, SD_SRC                          ; The client's bytes (at SD_DONE in its buffer) ->
            clc                                             ;   the block
            lda         TASK_INBOX + RQ_BUF
            adc         SD_DONE
            sta         r1
            lda         TASK_INBOX + RQ_BUF + 1
            adc         SD_DONE + 1
            sta         r1 + 1
            MOVR        r2, SD_N
            jsr         CLIENT_READ
            bra         @copied

@zeros:                                                     ; (Or zeros: SD_N can be a whole block here, as
            lda         #0                                  ;   HFS_EXTEND's count isn't the request's)
            ldy         #0
            ldx         SD_N + 1                            ; Whole pages ...
            beq         @part
:
            sta         (SD_SRC),Y
            iny
            bne         :-
            inc         SD_SRC + 1
            dex
            bne         :-

@part:
            ldy         SD_N                                ; ... and the rest (0-255)
            beq         @copied
:
            dey
            sta         (SD_SRC),Y
            cpy         #0
            bne         :-

@copied:
            lda         SD_CACHE                            ; ... -> the disk (and the block buffer holds it)
            sta         SD_BUF
            lda         SD_CACHE + 1
            sta         SD_BUF + 1
            lda         HFS_CARD
            sta         SD_DEV
            jsr         SD_WRITE_BLOCK
            bcs         @write_error
            jsr         HFS_ADVANCE
            jsr         HFS_AT_END                          ; Past its end?  The file is this long now
            bcc         @next
            ldy         #HFS_E_SIZE
            ldx         #0
:
            lda         SD_POS,X
            sta         (HFS_FP),Y
            iny
            inx
            cpx         #4
            bne         :-

@next:
            jmp         HFS_W_RANGE

@write_error:
            stz         SD_CVALID                           ; (The block buffer and the disk may differ)

@error:
            sec
            rts

; R_CREATE: make a file or directory (its name: HFS_NAME's, "/N/dir/name") and open it; a file that's there
; already is emptied and opened instead, as in Plan 9.  Its mode: RQ_PERM (DM_DIR, DM_APPEND: HFS_M_DIR,
; HFS_M_APPEND)
HFS_CREATE_REQ:
            lda         TASK_INBOX + RQ_PERM
            and         #HFS_M_DIR | HFS_M_APPEND
            sta         HFS_PERM                            ; The new file's mode
            jsr         HFS_NAME
            bcc         HFS_CREATE
            rts

.assert     HFS_M_DIR = DM_DIR .and HFS_M_APPEND = $40, error, "HFS_CREATE_REQ: RQ_PERM's high byte is the mode's (DM_DIR, DM_APPEND)"

HFS_CREATE:
            ldy         #0                                  ; The last '/': the new name comes after it
            ldx         #0

@end:
            lda         (HFS_NM),Y
            beq         @ended
            cmp         #'/'
            bne         :+
            tya
            tax
:
            iny
            bne         @end

@ended:
            cpx         #2                                  ; ("/N/name": the directory is "/N" at least)
            bcc         @bad_name
            inx
            stx         HFS_NAMEAT
            txa
            tay
            lda         (HFS_NM),Y                          ; Not empty, "." or ".."
            beq         @bad_name
            cmp         #'.'
            bne         @name_ok
            iny
            lda         (HFS_NM),Y
            beq         @bad_name
            cmp         #'.'
            bne         @name_ok
            iny
            lda         (HFS_NM),Y
            beq         @bad_name

@name_ok:
            ldy         HFS_NAMEAT                          ; Walk to the directory: the name ends at the '/'
            dey
            lda         #0
            sta         (HFS_NM),Y
            jsr         HFS_WALK
            bcs         @done
            ldy         HFS_NAMEAT                          ; (The name whole again)
            dey
            lda         #'/'
            sta         (HFS_NM),Y
            jsr         HFS_RO_DISK                         ; (Not on the ROM disk)
            bcs         @done
            lda         HFS_ENT + HFS_E_MODE
            bpl         @not_found                          ; (Not a directory)
            ldx         #4                                  ; HFS_DLOC = the directory's place
:
            lda         HFS_LOC,X
            sta         HFS_DLOC,X
            dex
            bpl         :-
            lda         HFS_NAMEAT                          ; The name there already?
            sta         HFS_ELEM
            lda         #HFS_SCAN_NAME
            sta         HFS_SCAN
            jsr         HFS_DIR_SCAN
            bcs         :+
            jmp         HFS_CREATE_OLD
:
            cmp         #E_NOENT
            bne         @fail
            jmp         HFS_CREATE_NEW

@bad_name:
            lda         #E_INVAL
            sec
            rts

@not_found:
            lda         #E_NOENT

@fail:
            sec

@done:
            rts

; The name is there already: a file, made again as a file, is emptied and opened.  A directory, or a file
; where a directory was asked for, can't be (E_EXIST)
HFS_CREATE_OLD:
            ldx         #4
:
            lda         HFS_NLOC,X
            sta         HFS_LOC,X
            lda         HFS_DLOC,X
            sta         HFS_PLOC,X
            dex
            bpl         :-
            jsr         HFS_ENT_READ
            bcs         @done
            lda         HFS_ENT + HFS_E_MODE
            ora         HFS_PERM
            bmi         @exists                             ; (HFS_M_DIR, in either)
            lda         HFS_ENT + HFS_E_MODE
            and         #HFS_M_RO
            bne         @read_only
            jsr         HFS_TRUNCATE
            bcs         @done
            lda         SD_OP                               ; (It's empty already)
            and         #<~O_TRUNC
            sta         SD_OP
            jmp         HFS_TAKE_SLOT

@exists:
            lda         #E_EXIST
            sec
            rts

@read_only:
            lda         #E_PERM
            sec

@done:
            rts

; A new entry, in a free one in the directory (HFS_ENT, at HFS_DLOC), or else at its end (the directory
; gets a cluster more if it needs one).  The new entry is written first, then the directory's (with its new
; size, when the entry is at its end), so a crash between them leaves no half-made entry in it.
HFS_CREATE_NEW:
            stz         HFS_GREW
            lda         #HFS_SCAN_FREE
            sta         HFS_SCAN
            jsr         HFS_DIR_SCAN
            bcc         @place
            cmp         #E_NOENT
            beq         :+
            jmp         @fail
:
            ldx         #3                                  ; None free: at its end
:
            lda         HFS_ENT + HFS_E_SIZE,X
            sta         SD_POS,X
            dex
            bpl         :-
            jsr         HFS_FILE_BLOCK
            bcc         @at_end
            cmp         #HFS_EOF
            beq         :+
            jmp         @fail
:
            jsr         HFS_GROW                            ; (HFS_FP -> HFS_ENT, HFS_LOC = its place)
            bcc         :+
            jmp         @fail
:
            jsr         HFS_FILE_BLOCK
            bcc         @at_end
            jmp         @fail

@at_end:
            ldx         #3                                  ; HFS_NLOC = the block, and the index in it
:
            lda         SD_LBA,X
            sta         HFS_NLOC,X
            dex
            bpl         :-
            lda         SD_POS + 1
            and         #1
            asl
            asl
            sta         HFS_NLOC + 4
            lda         SD_POS
            lsr
            lsr
            lsr
            lsr
            lsr
            lsr
            ora         HFS_NLOC + 4
            sta         HFS_NLOC + 4
            inc         HFS_GREW

@place:
            ldy         #HFS_ENTRY_SIZE - 1                 ; The new entry: its name and mode, and a new qid
            lda         #0                                  ;   and stamp
:
            sta         HFS_NEWE,Y
            dey
            bpl         :-
            ldy         HFS_NAMEAT
            ldx         #0
:
            lda         (HFS_NM),Y
            sta         HFS_NEWE,X
            iny
            inx
            cpx         HFS_LEN
            bne         :-
            lda         HFS_PERM
            sta         HFS_NEWE + HFS_E_MODE
            jsr         HFS_TAKE_QID
            ldx         #3
:
            lda         HFS_T4,X
            sta         HFS_NEWE + HFS_E_QID,X
            dex
            bpl         :-
            jsr         HFS_TAKE_STAMP
            ldx         #3
:
            lda         HFS_T4,X
            sta         HFS_NEWE + HFS_E_STAMP,X
            dex
            bpl         :-
            jsr         HFS_SB_SAVE                         ; (The counters first: a qid is never used twice)
            bcc         :+
            jmp         @fail
:
            ldx         #4                                  ; Write it
:
            lda         HFS_NLOC,X
            sta         HFS_LOC,X
            dex
            bpl         :-
            LOAD_ADDR   HFS_NEWE, HFS_FP
            jsr         HFS_ENT_PUT
            bcs         @fail
            ldx         #4                                  ; Then the directory's: bigger, if the new one is
:                                                           ;   at its end, a new qid version and stamp
            lda         HFS_DLOC,X
            sta         HFS_LOC,X
            sta         HFS_PLOC,X
            dex
            bpl         :-
            LOAD_ADDR   HFS_ENT, HFS_FP
            lda         HFS_GREW
            beq         :+
            clc
            lda         HFS_ENT + HFS_E_SIZE
            adc         #HFS_ENTRY_SIZE
            sta         HFS_ENT + HFS_E_SIZE
            bcc         :+
            inc         HFS_ENT + HFS_E_SIZE + 1
            bne         :+
            inc         HFS_ENT + HFS_E_SIZE + 2
            bne         :+
            inc         HFS_ENT + HFS_E_SIZE + 3
:
            jsr         HFS_TOUCH
            jsr         HFS_ENT_PUT
            bcs         @fail
            ldy         #HFS_ENTRY_SIZE - 1                 ; And open the new one (a directory for reading)
:
            lda         HFS_NEWE,Y
            sta         HFS_ENT,Y
            dey
            bpl         :-
            ldx         #4
:
            lda         HFS_NLOC,X
            sta         HFS_LOC,X
            dex
            bpl         :-
            lda         SD_OP                               ; (Nothing to empty)
            and         #<~O_TRUNC
            sta         SD_OP
            lda         HFS_PERM
            bpl         :+
            lda         SD_OP
            and         #<~O_RW_MASK
            sta         SD_OP
:
            jmp         HFS_TAKE_NEW

@fail:
            rts

; R_REMOVE: remove a file, or an empty directory: not a disk's root, and not a file that's open.  The entry
; goes first (its extents kept, to free after), then its clusters, then its directory's qid version and stamp.
HFS_REMOVE_REQ:
            jsr         HFS_NAME
            bcs         :+
            jsr         HFS_WALK
            bcc         HFS_REMOVE_AT
:
            rts

; Remove what HFS_WALK found.  OUT: C = 0; or C = 1, .A = error
HFS_REMOVE_AT:
            jsr         HFS_RO_DISK                         ; (Not on the ROM disk)
            bcs         @done
            lda         HFS_DEPTH
            beq         @root                               ; (A disk's root)
            ldx         #0
            jsr         HFS_SLOT_FIND
            bcc         @busy                               ; (Open)
            lda         HFS_ENT + HFS_E_MODE
            bpl         @remove
            lda         #HFS_SCAN_USED                      ; A directory: empty?
            sta         HFS_SCAN
            jsr         HFS_DIR_SCAN
            bcc         @not_empty
            cmp         #E_NOENT
            bne         @fail

@remove:
            ldy         #HFS_ENTRY_SIZE - 1
:
            lda         HFS_ENT,Y
            sta         HFS_NEWE,Y
            lda         #0
            sta         HFS_ENT,Y
            dey
            bpl         :-
            jsr         HFS_ENT_PUT                         ; (HFS_FP -> HFS_ENT, HFS_LOC = its place)
            bcs         @done
            jsr         HFS_FREE_OLD
            bcs         @done
            jsr         HFS_POP                             ; Its directory
            bcs         @done
            jsr         HFS_TOUCH
            jmp         HFS_ENT_PUT

@root:
            lda         #E_PERM
            sec
            rts

@busy:
            lda         #E_BUSY
            sec
            rts

@not_empty:
            lda         #E_NOTEMPTY

@fail:
            sec

@done:
            rts

; R_WSTAT: a stat record (the client's: RQ_BUF) changes the file: its name (SR_NAME: in its directory, where the
; new name mustn't be yet; a 0 first byte keeps it), and its mode (SR_MODE: no w bits, read-only; DM_APPEND,
; append-only; a directory stays one; $FFFF keeps it), and its length (SR_LENGTH: a file's; $FFFFFFFF keeps it).
; The record's other fields are left alone.
HFS_WSTAT_REQ:
            jsr         HFS_FID_CHECK
            bcs         :+
            jsr         HFS_RO_DISK                         ; (Not on the ROM disk)
            bcc         :++
:
            rts
:
            LDR         r0, HFS_STAT                        ; The record
            MOVR        r1, TASK_INBOX + RQ_BUF
            LDR         r2, SR_SIZE
            jsr         CLIENT_READ
            stz         HFS_STAT + SR_NAME + HFS_NAME_MAX   ; (A name: 31 at most)
            LDR         HFS_NM, HFS_STAT

HFS_WSTAT:
            lda         (HFS_NM)
            bne         :+
            jmp         HFS_WSTAT_MODE                      ; (No new name)
:
            ldy         #0                                  ; The name: 1-31 characters, no '/'

@len:
            lda         (HFS_NM),Y
            beq         @len_ok
            cmp         #'/'
            bne         :+
            jmp         @bad_name
:
            iny
            cpy         #HFS_NAME_MAX + 1
            bne         @len
            jmp         @bad_name

@len_ok:
            sty         HFS_LEN
            sty         HFS_ELEM
            lda         (HFS_NM)                            ; Not "." or ".."
            cmp         #'.'
            bne         @name_ok
            cpy         #1
            bne         :+
            jmp         @bad_name
:
            ldy         #1
            lda         (HFS_NM),Y
            cmp         #'.'
            bne         @name_ok
            lda         HFS_LEN
            cmp         #2
            beq         @bad_name

@name_ok:
            lda         HFS_FP                              ; Its name already?  Then only the mode
            sta         HFS_PTR
            lda         HFS_FP + 1
            sta         HFS_PTR + 1
            jsr         HFS_NAME_EQ
            bcs         :+
            jmp         HFS_WSTAT_MODE
:
            lda         HFS_LOC                             ; A disk's root is in no directory: it has no
            ora         HFS_LOC + 1                         ;   name to change
            ora         HFS_LOC + 2
            ora         HFS_LOC + 3
            bne         :+
            lda         HFS_LOC + 4
            cmp         #HFS_SB_ROOT / HFS_ENTRY_SIZE
            beq         @root
:
            ldx         HFS_FID                             ; Its directory: is the name there?
            lda         HFS_H_PIDX,X
            sta         HFS_LOC + 4
            txa
            asl
            asl
            tax
            lda         HFS_H_PBLK,X
            sta         HFS_LOC
            lda         HFS_H_PBLK + 1,X
            sta         HFS_LOC + 1
            lda         HFS_H_PBLK + 2,X
            sta         HFS_LOC + 2
            lda         HFS_H_PBLK + 3,X
            sta         HFS_LOC + 3
            jsr         HFS_ENT_READ
            bcs         @done
            stz         HFS_ELEM                            ; (The name: from the record's start)
            lda         #HFS_SCAN_NAME
            sta         HFS_SCAN
            jsr         HFS_DIR_SCAN
            bcc         @exists
            cmp         #E_NOENT
            bne         @fail
            jsr         HFS_FID_CHECK                       ; (HFS_FP and HFS_LOC: the file's again)
            ldy         #0                                  ; The new name, zero-padded

@copy:
            lda         (HFS_NM),Y
            sta         (HFS_FP),Y
            beq         @pad
            iny
            bra         @copy

@pad:
            iny
            cpy         #HFS_NAME_MAX + 1
            beq         HFS_WSTAT_MODE
            lda         #0
            sta         (HFS_FP),Y
            bra         @pad

@exists:
            lda         #E_EXIST
            sec
            rts

@bad_name:
            lda         #E_INVAL
            sec
            rts

@root:
            lda         #E_PERM

@fail:
            sec

@done:
            rts

; The length (HFS_WSTAT_LEN); then the mode, unless the record's is $FFFF: read-only if it has no w bits,
; append-only if it has DM_APPEND (a directory stays one); then the entry (a new qid version and stamp) is written
HFS_WSTAT_MODE:
            jsr         HFS_WSTAT_LEN
            bcc         :+
            rts
:
            lda         HFS_STAT + SR_MODE
            and         HFS_STAT + SR_MODE + 1
            cmp         #$FF
            beq         @write
            lda         HFS_STAT + SR_MODE + 1
            and         #HFS_M_APPEND                       ; (DM_APPEND)
            sta         HFS_T4
            lda         HFS_STAT + SR_MODE
            and         #$92                                ; (Its w bits: 0222)
            bne         :+
            lda         #HFS_M_RO
            tsb         HFS_T4
:
            ldy         #HFS_E_MODE
            lda         (HFS_FP),Y
            and         #HFS_M_DIR
            ora         HFS_T4
            sta         (HFS_FP),Y

@write:
            jsr         HFS_TOUCH
            jmp         HFS_ENT_PUT

; The length, unless the record's is $FFFFFFFF: a file's (not a directory's, nor a read-only one's) made that, as
; Plan 9's wstat does: longer, with zeros after its end (HFS_EXTEND, as a write past it would); or shorter
; (HFS_SHRINK).  OUT: C = 0; or C = 1, .A = error
HFS_WSTAT_LEN:
            lda         HFS_STAT + SR_LENGTH
            and         HFS_STAT + SR_LENGTH + 1
            and         HFS_STAT + SR_LENGTH + 2
            and         HFS_STAT + SR_LENGTH + 3
            cmp         #$FF
            beq         @keep
            ldy         #HFS_E_MODE
            lda         (HFS_FP),Y
            bit         #HFS_M_DIR
            bne         @dir
            and         #HFS_M_RO
            bne         @ro
            ldy         #HFS_E_SIZE                         ; Shorter than it is?
            ldx         #0
            sec
:
            lda         HFS_STAT + SR_LENGTH,X
            sbc         (HFS_FP),Y
            iny
            inx
            txa                                             ; (cpx would change C)
            eor         #4
            bne         :-
            bcc         HFS_SHRINK
            ldx         #3                                  ; No: the file made that long (as long as it is:
:                                                           ;   nothing)
            lda         HFS_STAT + SR_LENGTH,X
            sta         SD_POS,X
            dex
            bpl         :-
            jmp         HFS_EXTEND

@keep:
            clc
            rts

@dir:
            lda         #E_ISDIR
            sec
            rts

@ro:
            lda         #E_PERM
            sec
            rts

; The file at HFS_FP cut to the record's length: its size first (the entry written), then its clusters past the
; new end freed, from its last extent back.  Each extent goes out of the list (an extent block left empty, out of
; the chain) or is cut short before its clusters are freed, so a crash leaves lost clusters, never ones in use
; twice.  OUT: C = 0; or C = 1, .A = error
HFS_SHRINK:
            ldy         #HFS_E_SIZE                         ; The size
            ldx         #0
:
            lda         HFS_STAT + SR_LENGTH,X
            sta         (HFS_FP),Y
            iny
            inx
            cpx         #4
            bne         :-
            jsr         HFS_ENT_PUT
            bcc         :+
            rts
:
            clc                                             ; HFS_KEEP = the clusters kept: (size + 4095) >> 12
            lda         HFS_STAT + SR_LENGTH
            adc         #<(HFS_CLUSTER_BLOCKS * HFS_BLOCK - 1)
            lda         HFS_STAT + SR_LENGTH + 1
            adc         #>(HFS_CLUSTER_BLOCKS * HFS_BLOCK - 1)
            sta         HFS_KEEP
            lda         HFS_STAT + SR_LENGTH + 2
            adc         #0
            sta         HFS_KEEP + 1
            lda         HFS_STAT + SR_LENGTH + 3
            adc         #0
            sta         HFS_KEEP + 2
            stz         HFS_KEEP + 3
            rol         HFS_KEEP + 3                        ; (The carry: a 33rd bit)
            ldx         #HFS_CSHIFT + 9 - 8                 ; (>> 8 so far: then >> 4)
:
            lsr         HFS_KEEP + 3
            ror         HFS_KEEP + 2
            ror         HFS_KEEP + 1
            ror         HFS_KEEP
            dex
            bne         :-
            ldx         #3                                  ; HFS_TOTAL = the clusters the extents have
:
            stz         HFS_TOTAL,X
            stz         HFS_POSB,X
            dex
            bpl         :-
            lda         #HFS_E_EXT1
            sta         HFS_POSO
            stz         HFS_POSO + 1

@sum:
            jsr         HFS_POS_PTR
            bcc         :+
            rts
:
            ldy         #4
            lda         (HFS_PTR),Y
            clc
            adc         HFS_TOTAL
            sta         HFS_TOTAL
            iny
            lda         (HFS_PTR),Y
            adc         HFS_TOTAL + 1
            sta         HFS_TOTAL + 1
            bcc         :+
            inc         HFS_TOTAL + 2
            bne         :+
            inc         HFS_TOTAL + 3
:
            jsr         HFS_POS_NEXT
            bcc         @sum
            cmp         #0
            beq         @cut
            sec                                             ; (A card error)
            rts

@cut:
            lda         HFS_KEEP                            ; The last extent, while there are too many
            cmp         HFS_TOTAL
            lda         HFS_KEEP + 1
            sbc         HFS_TOTAL + 1
            lda         HFS_KEEP + 2
            sbc         HFS_TOTAL + 2
            lda         HFS_KEEP + 3
            sbc         HFS_TOTAL + 3
            bcc         :+
            clc
            rts
:
            jsr         HFS_LAST_EXT
            bcc         :+
            rts
:
            sec                                             ; HFS_TOTAL = the clusters before it
            lda         HFS_TOTAL
            sbc         HFS_XLEN
            sta         HFS_TOTAL
            lda         HFS_TOTAL + 1
            sbc         HFS_XLEN + 1
            sta         HFS_TOTAL + 1
            lda         HFS_TOTAL + 2
            sbc         #0
            sta         HFS_TOTAL + 2
            lda         HFS_TOTAL + 3
            sbc         #0
            sta         HFS_TOTAL + 3
            lda         HFS_TOTAL                           ; Some of it kept (HFS_KEEP - HFS_TOTAL > 0)?
            cmp         HFS_KEEP
            lda         HFS_TOTAL + 1
            sbc         HFS_KEEP + 1
            lda         HFS_TOTAL + 2
            sbc         HFS_KEEP + 2
            lda         HFS_TOTAL + 3
            sbc         HFS_KEEP + 3
            bcc         @short
            jsr         HFS_EXT_DROP                        ; No: all of it out of the list, and freed
            bcc         @cut
            rts

@short:
            jsr         HFS_LAST_PTR                        ; It's cut short: HFS_KEEP - HFS_TOTAL clusters
            bcs         @done
            sec
            lda         HFS_KEEP
            sbc         HFS_TOTAL
            sta         HFS_HK
            ldy         #4
            sta         (HFS_PTR),Y
            lda         HFS_KEEP + 1
            sbc         HFS_TOTAL + 1
            sta         HFS_HK + 1
            iny
            sta         (HFS_PTR),Y
            jsr         HFS_ENT_PUT                         ; (Its entry, in case it's there)
            bcs         @done
            sec                                             ; The rest of it freed: HFS_XLEN - kept clusters
            lda         HFS_XLEN                            ;   from HFS_XCL + kept
            sbc         HFS_HK
            sta         HFS_XLEN
            lda         HFS_XLEN + 1
            sbc         HFS_HK + 1
            sta         HFS_XLEN + 1
            jsr         HFS_IS_HOLE
            beq         @ok
            clc
            lda         HFS_XCL
            adc         HFS_HK
            sta         HFS_XCL
            lda         HFS_XCL + 1
            adc         HFS_HK + 1
            sta         HFS_XCL + 1
            bcc         :+
            inc         HFS_XCL + 2
            bne         :+
            inc         HFS_XCL + 3
:
            jmp         HFS_FREE_RUN

@ok:
            clc

@done:
            rts

; Free the clusters of the extent HFS_XCL / HFS_XLEN, unless it's a hole.  OUT: as HFS_FREE_RUN
HFS_FREE_EXT:
            jsr         HFS_IS_HOLE
            beq         :+
            jmp         HFS_FREE_RUN
:
            clc
            rts

; Is the extent HFS_XCL a hole?  OUT: Z = 1: it is.  Modifies: .A
HFS_IS_HOLE:
            lda         HFS_XCL
            and         HFS_XCL + 1
            and         HFS_XCL + 2
            and         HFS_XCL + 3
            cmp         #HFS_HOLE
            rts

; The extent HFS_LAST_EXT found taken out of the file's list, then its clusters freed: out of the entry (the
; entry written), or its extent block's count one less.  An extent block that leaves empty goes out of the chain
; (the link to it made 0, in the entry or in the block before it), and its cluster is freed too.
; OUT: C = 0; or C = 1, .A = error.  Modifies: .A, .X, .Y
HFS_EXT_DROP:
            lda         HFS_LASTB
            ora         HFS_LASTB + 1
            ora         HFS_LASTB + 2
            ora         HFS_LASTB + 3
            bne         @block
            lda         HFS_LASTO                           ; In the entry: no clusters there now
            clc
            adc         #HFS_EXT_SIZE - 1
            tay
            ldx         #HFS_EXT_SIZE
            lda         #0
:
            sta         (HFS_FP),Y
            dey
            dex
            bne         :-
            jsr         HFS_ENT_PUT
            bcc         HFS_FREE_EXT
            rts

@block:
            jsr         HFS_LASTB_GET                       ; One less in its block
            bcc         :+
            rts
:
            lda         #HFS_X_COUNT
            sta         HFS_OFS
            stz         HFS_OFS + 1
            jsr         HFS_META_AT
            lda         (HFS_PTR)
            beq         @empty                              ; (None: an empty block)
            dec
            sta         (HFS_PTR)
            jsr         HFS_META_CHANGED
            lda         (HFS_PTR)
            bne         HFS_FREE_EXT

@empty:
            lda         HFS_FP                              ; Empty: the link to it, the entry's ...
            sta         HFS_PTR
            lda         HFS_FP + 1
            sta         HFS_PTR + 1
            ldy         #HFS_E_EXTBLK
            jsr         HFS_IS_LASTB
            bne         @chain
            lda         #0
            ldy         #HFS_E_EXTBLK + 3
:
            sta         (HFS_FP),Y
            dey
            cpy         #HFS_E_EXTBLK - 1
            bne         :-
            jsr         HFS_ENT_PUT
            bcc         @free
            rts

@chain:
            ldy         #HFS_E_EXTBLK                       ; ... or a block's, down the chain
            ldx         #0
:
            lda         (HFS_FP),Y
            sta         SD_LBA,X
            iny
            inx
            cpx         #4
            bne         :-

@link:
            jsr         HFS_META_GET                        ; (SD_LBA: a block in the chain)
            bcc         :+
            rts
:
            stz         HFS_OFS
            stz         HFS_OFS + 1
            jsr         HFS_META_AT
            ldy         #HFS_X_NEXT                         ; Its link: to the empty one?
            jsr         HFS_IS_LASTB
            beq         @unlink
            ldy         #HFS_X_NEXT                         ; No: on to the block it links to
            ldx         #0
:
            lda         (HFS_PTR),Y
            sta         SD_LBA,X
            iny
            inx
            cpx         #4
            bne         :-
            lda         SD_LBA
            ora         SD_LBA + 1
            ora         SD_LBA + 2
            ora         SD_LBA + 3
            bne         @link
            lda         #E_INVAL                            ; (Not in the chain: never)
            sec
            rts

@unlink:
            lda         #0
            ldy         #HFS_X_NEXT + 3
:
            sta         (HFS_PTR),Y
            dey
            bpl         :-
            jsr         HFS_META_CHANGED

@free:
            jsr         HFS_FREE_EXT                        ; Its extent's clusters (it was the last in it)
            bcs         @done
            jsr         HFS_CARD_X                          ; Then the block's own: (it - the data area) / 8
            sec
            lda         HFS_LASTB
            sbc         HFS_V_DATA,X
            sta         HFS_XCL
            lda         HFS_LASTB + 1
            sbc         HFS_V_DATA + 1,X
            sta         HFS_XCL + 1
            lda         HFS_LASTB + 2
            sbc         HFS_V_DATA + 2,X
            sta         HFS_XCL + 2
            lda         HFS_LASTB + 3
            sbc         HFS_V_DATA + 3,X
            sta         HFS_XCL + 3
            ldx         #HFS_CSHIFT
:
            lsr         HFS_XCL + 3
            ror         HFS_XCL + 2
            ror         HFS_XCL + 1
            ror         HFS_XCL
            dex
            bne         :-
            lda         #1
            sta         HFS_XLEN
            stz         HFS_XLEN + 1
            jmp         HFS_FREE_RUN

@done:
            rts

; Is the link at (HFS_PTR),Y (4 bytes) to block HFS_LASTB?  OUT: Z = 1: it is.  Modifies: .A, .X, .Y
HFS_IS_LASTB:
            ldx         #0
:
            lda         (HFS_PTR),Y
            cmp         HFS_LASTB,X
            bne         @done
            iny
            inx
            cpx         #4
            bne         :-

@done:
            rts
