.debuginfo

; ****************************************************************************
; The HydraFS server's writing side (BIOS ROM page 6, with hfs_srv.s, inside `.scope PAGE6`): the metadata
; buffer, the free map, growing and freeing files, and the requests that change a card: H9_WRITE,
; H9_CREATE, H9_REMOVE, H9_WSTAT.  (Format and label are on page 3: hfs_format.s.)
;
; There's no journal (see docs/plans/HYDRAFS.md), but the writes go in a safe order: a cluster is marked in
; use before any entry points at it, and an entry stops pointing at clusters before they're marked free.
; So a crash can leave lost clusters (marked in use, but nothing's), never a cluster used twice.

.segment "HFS_P6"

HFS_BITS:   .byte   $01, $02, $04, $08, $10, $20, $40, $80

; ****************************************************************************
; The metadata buffer (HFS_META, 512 bytes): the free map, extent blocks, entries being written and the
; superblock go through it, so that a write's allocating doesn't evict the file's data from the block
; cache.  A block is changed there, and written back when another block is wanted (HFS_META_GET), or at the
; end of the request (HFS_FINISH), which forgets it too: nothing is kept between requests, so a raw write
; to /dev/sd, or a changed card, can't leave it out of date.

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
            jsr         HFS_CARD_X
            ldy         #HFS_SB_FREE

@number:                                                    ; (4 of 4 bytes: 4 apart in the block, 32 in RAM)
            lda         HFS_V_FREE,X
            sta         (HFS_PTR),Y
            iny
            inx
            txa
            and         #3
            bne         @number
            txa
            clc
            adc         #32 - 4
            tax
            cpy         #HFS_SB_MAPINIT + 4
            bne         @number
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

; .X = the card * 4: where its numbers are in the HFS_V_* arrays.  Modifies: .A
HFS_CARD_X:
            lda         HFS_CARD
            asl
            asl
            tax
            rts

; ****************************************************************************
; The card's counters, and entries

; Take the card's next qid id (HFS_TAKE_QID) or modification stamp (HFS_TAKE_STAMP) into HFS_T4, and count
; it on (the superblock gets it at the end of the request).  Modifies: .A, .X, .Y
HFS_TAKE_STAMP:
            lda         #HFS_V_STAMP - HFS_V_QID
            bra         HFS_TAKE

HFS_TAKE_QID:
            lda         #0

HFS_TAKE:
            sta         HFS_T4                              ; (.X = the counter: at HFS_V_QID,X)
            lda         HFS_CARD
            asl
            asl
            clc
            adc         HFS_T4
            tax
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
            txa
            clc
            adc         #HFS_FHDR_SIZE
            tax
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
; OUT: C = 0: HFS_C = the cluster; or C = 1, .A = ERR_IO_FULL or a card error.  Modifies: .A, .X, .Y
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
            lda         #ERR_IO_FULL
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
; the cluster after it is free; or else the cluster starts a new extent (HFS_NEW_EXT).  Then the entry is
; written (with the file's size as it is now).
; OUT: C = 0; or C = 1, .A = ERR_IO_FULL or a card error.  Modifies: .A, .X, .Y
HFS_GROW:
            jsr         HFS_LAST_EXT
            bcs         @done
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
            jmp         HFS_NEW_EXT

@done:
            rts

; A new extent, {HFS_C, 1 cluster}, after the file's last one (HFS_LAST_EXT): the entry's first or second,
; or in its last extent block, or in a new extent block (in a cluster of its own) when that's full or it
; has none.  OUT: C = 0; or C = 1, .A = error.  Modifies: .A, .X, .Y
HFS_NEW_EXT:
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

; The extent {HFS_C, 1 cluster} at HFS_PTR.  Modifies: .A, .Y
HFS_EXT_PUT:
            ldy         #0
:
            lda         HFS_C,Y
            sta         (HFS_PTR),Y
            iny
            cpy         #4
            bne         :-
            lda         #1
            sta         (HFS_PTR),Y
            iny
            lda         #0
            sta         (HFS_PTR),Y
            rts

; A new extent block, holding one extent {HFS_C, 1 cluster}, in a cluster of its own (its first block).
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
            bcs         @done                               ;   other buffer)
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

; The extent at (HFS_XP),Y, to the routine (an unused one, with no clusters, isn't).  OUT: as the routine's
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
            bne         HFS_RUN
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
; The requests that change a card

; H9_WRITE: at the fd's offset (at the end of the file if it's append-only), the part of a block at a time;
; the file gets a cluster more when it needs one (HFS_GROW).  A write can't start past the end of the file.
HFS_WRITE_REQ:
            jsr         HFS_FID_CHECK
            bcc         :+
            rts
:
            ldy         #HFS_E_MODE                         ; (A directory is never opened for writing: the
            lda         (HFS_FP),Y                          ;   fd's mode says whether it may be written)
            and         #HFS_M_DIR
            beq         :+
            lda         #ERR_IO_MODE
            sec
            rts
:
            lda         HFS_FHDR + HFS_H_FLAGS,X            ; Its first write since it was opened: a new qid
            bne         :+                                  ;   version and stamp (the entry goes to the card
            lda         #HFS_HF_WRITTEN | HFS_HF_DIRTY      ;   when it's closed, or gets a cluster)
            sta         HFS_FHDR + HFS_H_FLAGS,X
            jsr         HFS_TOUCH
:
            jsr         HFS_REQ_ARGS
            ldy         #HFS_E_MODE
            lda         (HFS_FP),Y
            and         #HFS_M_APPEND
            beq         @where
            ldy         #HFS_E_SIZE                         ; Append-only: at its end, wherever the fd is
            ldx         #0
:
            lda         (HFS_FP),Y
            sta         SD_POS,X
            iny
            inx
            cpx         #4
            bne         :-

@where:
            jsr         HFS_TAIL                            ; (C = 0: SD_POS is past the end: a hole)
            bcs         HFS_W_PIECE
            jsr         IO_SRV_UNMAP
            lda         #ERR_IO_BAD_REQ
            sec
            rts

HFS_W_PIECE:
            lda         SD_LEFT
            ora         SD_LEFT + 1
            bne         :+
            jsr         HFS_SYNC                            ; All written: the other copies of the entry get
            jmp         HFS_READ_DONE                       ;   its size too; the count done goes back
:
            jsr         HFS_FILE_BLOCK                      ; The block byte SD_POS goes in: a cluster more
            bcc         @have                               ;   first, if the file has none there yet
            cmp         #ERR_IO_EOF
            beq         @far10
            jmp         HFS_W_ERROR
@far10:
            jsr         HFS_GROW
            bcc         @far9
            jmp         HFS_W_ERROR
@far9:
            jsr         HFS_FILE_BLOCK
            bcc         @far8
            jmp         HFS_W_ERROR
@far8:

@have:
            lda         SD_POS + 1                          ; SD_N = the bytes to this block's end ...
            and         #1
            eor         #1
            tax
            lda         SD_POS
            eor         #$FF
            clc
            adc         #1
            sta         SD_N
            bne         :+
            inx
:
            stx         SD_N + 1
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
            stz         SD_CVALID                           ; (The cache is about to hold it)
            bra         @copy

@load:
            jsr         HFS_LOAD
            bcs         HFS_W_ERROR

@copy:
            lda         SD_POS                              ; SD_SRC = the cache + (SD_POS & 511)
            clc
            adc         SD_CACHE
            sta         SD_SRC
            lda         SD_POS + 1
            and         #1
            adc         SD_CACHE + 1
            sta         SD_SRC + 1
            lda         SD_DONE                             ; SD_DST = the data area + SD_DONE
            sta         SD_DST
            lda         ZP_IO_REQ + 1
            inc
            sta         SD_DST + 1
            ldy         #0
:
            lda         (SD_DST),Y                          ; SD_N bytes (1-256): the data area -> the cache
            sta         (SD_SRC),Y
            iny
            cpy         SD_N                                ; (SD_N = 256: 0, so .Y wraps round to it)
            bne         :-
            lda         SD_CACHE                            ; ... -> the card
            sta         SD_BUF
            lda         SD_CACHE + 1
            sta         SD_BUF + 1
            lda         HFS_CARD
            sta         SD_DEV
            jsr         SD_WRITE_BLOCK
            bcs         @write_error
            ldx         #3                                  ; The cache holds that block now
:
            lda         SD_LBA,X
            sta         SD_CBLOCK,X
            dex
            bpl         :-
            lda         HFS_CARD
            sta         SD_CCARD
            lda         #1
            sta         SD_CVALID
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
            jmp         HFS_W_PIECE

@write_error:
            stz         SD_CVALID                           ; (The cache and the card may differ)

HFS_W_ERROR:
            jsr         IO_SRV_UNMAP
            sec
            rts

; H9_CREATE: make a file or directory ("/N/dir/name") and open it; a file that's there already is emptied
; and opened instead, as in Plan 9.
HFS_CREATE_REQ:
            ldx         SD_CLIENT
            jsr         IO_SRV_MAP
            ldy         #IO_BLK_MODE
            lda         (ZP_IO_REQ),Y
            sta         SD_OP                               ; The open mode
            ldy         #IO_BLK_PERM
            lda         (ZP_IO_REQ),Y
            and         #HFS_M_DIR | HFS_M_APPEND | HFS_M_RO
            sta         HFS_PERM                            ; The new file's mode
            inc         ZP_IO_REQ + 1                       ; The data area: the name
            jsr         HFS_CREATE
            dec         ZP_IO_REQ + 1
            jmp         IO_SRV_UNMAP                        ; (It keeps .A and C)

HFS_CREATE:
            ldy         #0                                  ; The last '/': the new name comes after it
            ldx         #0

@end:
            lda         (ZP_IO_REQ),Y
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
            lda         (ZP_IO_REQ),Y                       ; Not empty, "." or ".."
            beq         @bad_name
            cmp         #'.'
            bne         @name_ok
            iny
            lda         (ZP_IO_REQ),Y
            beq         @bad_name
            cmp         #'.'
            bne         @name_ok
            iny
            lda         (ZP_IO_REQ),Y
            beq         @bad_name

@name_ok:
            ldy         HFS_NAMEAT                          ; Walk to the directory: the name ends at the '/'
            dey
            lda         #0
            sta         (ZP_IO_REQ),Y
            jsr         HFS_WALK
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
            cmp         #ERR_IO_NOT_FOUND
            bne         @fail
            jmp         HFS_CREATE_NEW

@bad_name:
            lda         #ERR_IO_NAME
            sec
            rts

@not_found:
            lda         #ERR_IO_NOT_FOUND

@fail:
            sec

@done:
            rts

; The name is there already: a file, made again as a file, is emptied and opened.  A directory, or a file
; where a directory was asked for, can't be (ERR_IO_EXISTS)
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
            and         #<~IO_MODE_TRUNC
            sta         SD_OP
            jmp         HFS_TAKE_SLOT

@exists:
            lda         #ERR_IO_EXISTS
            sec
            rts

@read_only:
            lda         #ERR_IO_MODE
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
            cmp         #ERR_IO_NOT_FOUND
            beq         @far7
            jmp         @fail
@far7:
            ldx         #3                                  ; None free: at its end
:
            lda         HFS_ENT + HFS_E_SIZE,X
            sta         SD_POS,X
            dex
            bpl         :-
            jsr         HFS_FILE_BLOCK
            bcc         @at_end
            cmp         #ERR_IO_EOF
            beq         @far6
            jmp         @fail
@far6:
            jsr         HFS_GROW                            ; (HFS_FP -> HFS_ENT, HFS_LOC = its place)
            bcc         @far5
            jmp         @fail
@far5:
            jsr         HFS_FILE_BLOCK
            bcc         @far4
            jmp         @fail
@far4:

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
            lda         (ZP_IO_REQ),Y
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
            bcs         @fail
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
            and         #<~IO_MODE_TRUNC
            sta         SD_OP
            lda         HFS_PERM
            bpl         :+
            lda         SD_OP
            and         #<~IO_MODE_WRITE
            sta         SD_OP
:
            jmp         HFS_TAKE_NEW

@fail:
            rts

; H9_REMOVE: remove a file, or an empty directory: not a card's root, and not a file that's open.  The entry
; goes first (its extents kept, to free after), then its clusters, then its directory's qid version and stamp.
HFS_REMOVE_REQ:
            ldx         SD_CLIENT
            jsr         IO_SRV_MAP
            inc         ZP_IO_REQ + 1
            jsr         HFS_WALK
            dec         ZP_IO_REQ + 1
            jsr         IO_SRV_UNMAP
            bcs         @done
            lda         HFS_DEPTH
            beq         @bad_req                            ; (A card's root)
            ldx         #0
            jsr         HFS_SLOT_FIND
            bcc         @busy                               ; (Open)
            lda         HFS_ENT + HFS_E_MODE
            bpl         @remove
            lda         #HFS_SCAN_USED                      ; A directory: empty?
            sta         HFS_SCAN
            jsr         HFS_DIR_SCAN
            bcc         @not_empty
            cmp         #ERR_IO_NOT_FOUND
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

@bad_req:
            lda         #ERR_IO_BAD_REQ
            sec
            rts

@busy:
            lda         #ERR_IO_BUSY
            sec
            rts

@not_empty:
            lda         #ERR_IO_NOT_EMPTY

@fail:
            sec

@done:
            rts

; H9_WSTAT: rename (in its directory, where the new name mustn't be yet), and set the mode bits
; (HFS_M_APPEND, HFS_M_RO; a directory stays one).  A 0 first name byte keeps the name, and a mode of $FF the
; mode; the record's other fields are left alone.
HFS_WSTAT_REQ:
            jsr         HFS_FID_CHECK
            bcc         :+
            rts
:
            ldx         SD_CLIENT
            jsr         IO_SRV_MAP
            inc         ZP_IO_REQ + 1                       ; The data area: the record
            jsr         HFS_WSTAT
            dec         ZP_IO_REQ + 1
            jmp         IO_SRV_UNMAP

HFS_WSTAT:
            lda         (ZP_IO_REQ)
            bne         :+
            jmp         HFS_WSTAT_MODE                      ; (No new name)
:
            ldy         #0                                  ; The name: 1-31 characters, no '/'

@len:
            lda         (ZP_IO_REQ),Y
            beq         @len_ok
            cmp         #'/'
            bne         @far3
            jmp         @bad_name
@far3:
            iny
            cpy         #HFS_NAME_MAX + 1
            bne         @len
            jmp         @bad_name

@len_ok:
            sty         HFS_LEN
            sty         HFS_ELEM
            lda         (ZP_IO_REQ)                         ; Not "." or ".."
            cmp         #'.'
            bne         @name_ok
            cpy         #1
            bne         @far2
            jmp         @bad_name
@far2:
            ldy         #1
            lda         (ZP_IO_REQ),Y
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
            lda         HFS_LOC                             ; A card's root is in no directory: it has no
            ora         HFS_LOC + 1                         ;   name to change
            ora         HFS_LOC + 2
            ora         HFS_LOC + 3
            bne         :+
            lda         HFS_LOC + 4
            cmp         #HFS_SB_ROOT / HFS_ENTRY_SIZE
            beq         @bad_req
:
            lda         HFS_FID                             ; Its directory: is the name there?
            asl
            asl
            asl
            asl
            tax
            lda         HFS_FHDR + HFS_H_PIDX,X
            sta         HFS_LOC + 4
            lda         HFS_FHDR + HFS_H_PBLK,X
            sta         HFS_LOC
            lda         HFS_FHDR + HFS_H_PBLK + 1,X
            sta         HFS_LOC + 1
            lda         HFS_FHDR + HFS_H_PBLK + 2,X
            sta         HFS_LOC + 2
            lda         HFS_FHDR + HFS_H_PBLK + 3,X
            sta         HFS_LOC + 3
            jsr         HFS_ENT_READ
            bcs         @done
            stz         HFS_ELEM                            ; (The name: from the record's start)
            lda         #HFS_SCAN_NAME
            sta         HFS_SCAN
            jsr         HFS_DIR_SCAN
            bcc         @exists
            cmp         #ERR_IO_NOT_FOUND
            bne         @fail
            jsr         HFS_FID_CHECK                       ; (HFS_FP and HFS_LOC: the file's again)
            ldy         #0                                  ; The new name, zero-padded

@copy:
            lda         (ZP_IO_REQ),Y
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
            lda         #ERR_IO_EXISTS
            sec
            rts

@bad_name:
            lda         #ERR_IO_NAME
            sec
            rts

@bad_req:
            lda         #ERR_IO_BAD_REQ

@fail:
            sec

@done:
            rts

; The mode bits, unless the record's mode is $FF; then the entry (a new qid version and stamp) is written
HFS_WSTAT_MODE:
            ldy         #IO_ST_MODE
            lda         (ZP_IO_REQ),Y
            cmp         #$FF
            beq         @write
            and         #HFS_M_APPEND | HFS_M_RO
            sta         HFS_T4
            ldy         #HFS_E_MODE
            lda         (HFS_FP),Y
            and         #HFS_M_DIR
            ora         HFS_T4
            sta         (HFS_FP),Y

@write:
            jsr         HFS_TOUCH
            jmp         HFS_ENT_PUT
