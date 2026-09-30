.debuginfo

; ****************************************************************************
; HydraFS's sparse files (BIOS ROM page 6, with hfs_srv.s and hfs_write.s, inside `.scope PAGE6`).
;
; A file can have holes: runs of clusters with nothing on the card, which read as zeros.  A hole is an
; extent whose first cluster is HFS_HOLE ($FFFFFFFF, never a cluster), with its clusters: it takes a place in
; the file's extent list as any extent does, and nothing in the free map (freeing a file, and the check,
; pass it by: HFS_EXT_RUN).
;   A write past the end of a file (HFS_EXTEND) makes the file that long first, with zeros: the rest of its
; last cluster written with zeros, the whole clusters after it a hole, and zeros written from the start of
; the write's cluster to where it starts.  So a file's bytes before its end always read as they were
; written, or as zeros.
;   A write into a hole (HFS_FILL) gives its cluster a cluster of the card, written with zeros first, and
; the hole's extent is split round it: {hole, k clusters}, {the cluster, 1}, {hole, the rest}, the extents
; after it moving along the list to make room (HFS_EXT_INSERT).

.segment "HFS_P6"

; ****************************************************************************
; A write past the end of the file

; The file whose entry is at HFS_FP is made SD_POS long, with zeros after its end (a write is about to start
; there).  The write's own SD_POS, SD_LEFT and SD_DONE are kept.  OUT: C = 0; or C = 1, .A = error
HFS_EXTEND:
            ldx         #3                                  ; (The write's, kept)
:
            lda         SD_POS,X
            sta         HFS_WPOS,X
            dex
            bpl         :-
            lda         SD_LEFT
            sta         HFS_WLEFT
            lda         SD_LEFT + 1
            sta         HFS_WLEFT + 1
            lda         SD_DONE
            sta         HFS_WDONE
            lda         #$80                                ; (HFS_W_RANGE writes zeros)
            sta         HFS_WZERO
            ldy         #HFS_E_SIZE                         ; SD_POS = the file's end
            ldx         #0
:
            lda         (HFS_FP),Y
            sta         SD_POS,X
            iny
            inx
            cpx         #4
            bne         :-
            lda         SD_POS + 1                          ; Inside a cluster: zeros to its end (or to the
            and         #>(HFS_CLUSTER_BLOCKS * HFS_BLOCK - 1)  ;   write, if that's sooner)
            ora         SD_POS
            beq         @holes
            sec                                             ; SD_LEFT = the bytes to the cluster's end
            lda         #0
            sbc         SD_POS
            sta         SD_LEFT
            lda         SD_POS + 1
            and         #>(HFS_CLUSTER_BLOCKS * HFS_BLOCK - 1)
            sta         SD_TMP
            lda         #>(HFS_CLUSTER_BLOCKS * HFS_BLOCK)
            sbc         SD_TMP
            sta         SD_LEFT + 1
            jsr         HFS_W_GAP                           ; HFS_CL = the gap: the write's start - SD_POS
            lda         HFS_CL + 2                          ; ... if that's less
            ora         HFS_CL + 3
            bne         :+
            lda         HFS_CL
            cmp         SD_LEFT
            lda         HFS_CL + 1
            sbc         SD_LEFT + 1
            bcs         :+
            lda         HFS_CL
            sta         SD_LEFT
            lda         HFS_CL + 1
            sta         SD_LEFT + 1
:
            stz         SD_DONE
            jsr         HFS_W_RANGE
            bcc         @far3
            jmp         @done
@far3:

@holes:                                                     ; Whole clusters to the write's: a hole
            jsr         HFS_W_GAP
            ldx         #HFS_CL - HFS_CL                    ; (HFS_CL >> 12: the gap in clusters, as SD_POS
            ldy         #HFS_CSHIFT + 9                     ;   is on a cluster's start now)
            jsr         HFS_CL_SHR
            ldx         #3
:
            lda         HFS_CL,X
            sta         HFS_WK,X
            dex
            bpl         :-

@hole:
            lda         HFS_WK                              ; Holes of 65535 clusters at most
            ora         HFS_WK + 1
            ora         HFS_WK + 2
            ora         HFS_WK + 3
            beq         @tail
            lda         #$FF                                ; HFS_XNEW = {HFS_HOLE, min(HFS_WK, 65535)}
            ldx         #HFS_EXT_SIZE - 1
:
            sta         HFS_XNEW,X
            dex
            bpl         :-
            lda         HFS_WK + 2
            ora         HFS_WK + 3
            bne         :+
            lda         HFS_WK
            sta         HFS_XNEW + 4
            lda         HFS_WK + 1
            sta         HFS_XNEW + 5
:
            sec                                             ; HFS_WK less them
            lda         HFS_WK
            sbc         HFS_XNEW + 4
            sta         HFS_WK
            lda         HFS_WK + 1
            sbc         HFS_XNEW + 5
            sta         HFS_WK + 1
            lda         HFS_WK + 2
            sbc         #0
            sta         HFS_WK + 2
            lda         HFS_WK + 3
            sbc         #0
            sta         HFS_WK + 3
            lda         HFS_XNEW + 4                        ; SD_POS on past them, to the file's end: their
            sta         HFS_CL                              ;   clusters * 4096 (* 16, a byte up)
            lda         HFS_XNEW + 5
            sta         HFS_CL + 1
            stz         HFS_CL + 2
            ldx         #4
:
            asl         HFS_CL
            rol         HFS_CL + 1
            rol         HFS_CL + 2
            dex
            bne         :-
            clc
            lda         SD_POS + 1
            adc         HFS_CL
            sta         SD_POS + 1
            lda         SD_POS + 2
            adc         HFS_CL + 1
            sta         SD_POS + 2
            lda         SD_POS + 3
            adc         HFS_CL + 2
            sta         SD_POS + 3
            jsr         HFS_LAST_EXT                        ; The hole's extent, at the end of the list
            bcs         @done
            jsr         HFS_APPEND_EXT
            bcs         @done
            jsr         HFS_SIZE_POS                        ; (The file is that long now)
            jmp         @hole

@tail:
            jsr         HFS_W_GAP                           ; Zeros from the write's cluster's start to it
            lda         HFS_CL                              ;   (what's left: under a cluster)
            sta         SD_LEFT
            lda         HFS_CL + 1
            sta         SD_LEFT + 1
            ora         SD_LEFT
            beq         @ok
            stz         SD_DONE
            jsr         HFS_W_RANGE
            bcs         @done

@ok:
            clc

@done:
            php                                             ; The write's own again
            pha
            ldx         #3
:
            lda         HFS_WPOS,X
            sta         SD_POS,X
            dex
            bpl         :-
            lda         HFS_WLEFT
            sta         SD_LEFT
            lda         HFS_WLEFT + 1
            sta         SD_LEFT + 1
            lda         HFS_WDONE
            sta         SD_DONE
            stz         HFS_WZERO
            pla
            plp
            rts

; HFS_CL = the write's start (HFS_WPOS) - SD_POS.  Modifies: .A
HFS_W_GAP:
            sec
            lda         HFS_WPOS
            sbc         SD_POS
            sta         HFS_CL
            lda         HFS_WPOS + 1
            sbc         SD_POS + 1
            sta         HFS_CL + 1
            lda         HFS_WPOS + 2
            sbc         SD_POS + 2
            sta         HFS_CL + 2
            lda         HFS_WPOS + 3
            sbc         SD_POS + 3
            sta         HFS_CL + 3
            rts

; Shift the 4 bytes at HFS_CL + .X right by .Y bits.  Modifies: .Y
HFS_CL_SHR:
            lsr         HFS_CL + 3,X
            ror         HFS_CL + 2,X
            ror         HFS_CL + 1,X
            ror         HFS_CL,X
            dey
            bne         HFS_CL_SHR
            rts

; The file whose entry is at HFS_FP is SD_POS long.  Modifies: .A, .X, .Y
HFS_SIZE_POS:
            ldy         #HFS_E_SIZE
            ldx         #0
:
            lda         SD_POS,X
            sta         (HFS_FP),Y
            iny
            inx
            cpx         #4
            bne         :-
            rts

; ****************************************************************************
; A write into a hole

; The cluster HFS_FILE_BLOCK found in a hole (HFS_IN_HOLE: HFS_HOLEP -> the hole's extent, in the entry at
; HFS_FP, or in the cache if HFS_XBC is the extent block it's in; HFS_CL = the cluster's place in the hole,
; HFS_XLEN = the hole's clusters) gets a cluster of the card, written with zeros, and the hole's extent is
; split round it.  OUT: C = 0; or C = 1, .A = ERR_IO_FULL or a card error
HFS_FILL:
            ldx         #3                                  ; Where the hole's extent is (the cache may not
:                                                           ;   hold it for long): HFS_POSB = its extent
            lda         HFS_XBC,X                           ;   block (0: the entry), HFS_POSO = its offset
            sta         HFS_POSB,X                          ;   there
            dex
            bpl         :-
            ldx         #HFS_FP                             ; (The entry, or the cache)
            lda         HFS_XBC
            ora         HFS_XBC + 1
            ora         HFS_XBC + 2
            ora         HFS_XBC + 3
            beq         :+
            ldx         #SD_CACHE
:
            sec
            lda         HFS_HOLEP
            sbc         0,X
            sta         HFS_POSO
            lda         HFS_HOLEP + 1
            sbc         1,X
            sta         HFS_POSO + 1
            lda         HFS_CL                              ; HFS_HK = the cluster's place in the hole (k),
            sta         HFS_HK                              ;   HFS_HN = the hole's clusters (n)
            lda         HFS_CL + 1
            sta         HFS_HK + 1
            lda         HFS_XLEN
            sta         HFS_HN
            lda         HFS_XLEN + 1
            sta         HFS_HN + 1
            lda         #$FF                                ; A cluster (any)
            ldx         #3
:
            sta         HFS_C,X
            dex
            bpl         :-
            jsr         HFS_ALLOC
            bcc         @far2
            jmp         @done
@far2:
            jsr         HFS_C_LBA                           ; Its blocks: zeros
            ldx         #HFS_CLUSTER_BLOCKS
:
            phx
            jsr         HFS_META_NEW                        ; (All zeros, to be written: it writes the one
            plx                                             ;   before)
            bcc         @far1
            jmp         @done
@far1:
            inc         SD_LBA
            bne         :+
            inc         SD_LBA + 1
            bne         :+
            inc         SD_LBA + 2
            bne         :+
            inc         SD_LBA + 3
:
            dex
            bne         :--
            jsr         HFS_META_FLUSH                      ; (The last)
            bcs         @done
            stz         HFS_ENTCHG
            jsr         HFS_XNEW_C                          ; The extents in the hole's place: the cluster,
            lda         HFS_HK                              ;   with {hole, k} before it (k > 0), and {hole,
            ora         HFS_HK + 1                          ;   n - k - 1} after it (if that's > 0)
            beq         @first
            jsr         HFS_XREP_XNEW                       ; (The cluster's goes after the first hole)
            lda         HFS_HK
            sta         HFS_XNEW + 4
            lda         HFS_HK + 1
            sta         HFS_XNEW + 5
            jsr         HFS_XNEW_HOLE
            jsr         HFS_POS_SWAP                        ; {hole, k} in the hole's place
            bcs         @done
            jsr         HFS_XREP_XNEW                       ; The cluster's after it
            jsr         HFS_EXT_INSERT
            bcs         @done
            bra         @rest

@first:
            jsr         HFS_POS_SWAP                        ; The cluster's in the hole's place
            bcs         @done

@rest:
            clc                                             ; The rest of the hole: n - k - 1 clusters
            lda         HFS_HK
            adc         #1
            sta         HFS_XNEW + 4
            lda         HFS_HK + 1
            adc         #0
            sta         HFS_XNEW + 5
            sec
            lda         HFS_HN
            sbc         HFS_XNEW + 4
            sta         HFS_XNEW + 4
            lda         HFS_HN + 1
            sbc         HFS_XNEW + 5
            sta         HFS_XNEW + 5
            ora         HFS_XNEW + 4
            beq         @entry
            jsr         HFS_XNEW_HOLE
            jsr         HFS_EXT_INSERT                      ; (After the cluster's)
            bcs         @done

@entry:
            lda         HFS_ENTCHG                          ; The entry, if its extents changed
            beq         @ok
            jsr         HFS_ENT_PUT
            bcs         @done

@ok:
            clc

@done:
            rts

; HFS_XNEW's first cluster = HFS_HOLE: a hole.  Modifies: .A, .X
HFS_XNEW_HOLE:
            lda         #$FF
            ldx         #3
:
            sta         HFS_XNEW,X
            dex
            bpl         :-
            rts

; Swap HFS_XNEW and HFS_XREP (the extent put by for later).  Modifies: .A, .X, .Y
HFS_XREP_XNEW:
            ldx         #HFS_EXT_SIZE - 1
:
            lda         HFS_XNEW,X
            ldy         HFS_XREP,X
            sta         HFS_XREP,X
            tya
            sta         HFS_XNEW,X
            dex
            bpl         :-
            rts

; ****************************************************************************
; The file's extent list, a place at a time: a place is HFS_POSB (the extent block: 0 for the entry, at
; HFS_FP) and HFS_POSO (the extent's offset there)

; Put the extent HFS_XNEW right after the one at the place HFS_POSB/HFS_POSO, the ones after that each moving
; a place on (the last to a new place at the end: HFS_APPEND_EXT).  The place becomes the new extent's.
; OUT: C = 0; or C = 1, .A = error.  Modifies: .A, .X, .Y
HFS_EXT_INSERT:
            jsr         HFS_POS_NEXT
            bcc         :+
            cmp         #0                                  ; (C = 1)
            bne         @done                               ; (A card error)
            bra         @end                                ; (It was the last: HFS_XNEW goes at the end)
:
            ldx         #HFS_POS_SIZE - 1                   ; (The new extent's place: this one)
:
            lda         HFS_POSB,X
            sta         HFS_POSK,X
            dex
            bpl         :-

@move:
            jsr         HFS_POS_SWAP                        ; HFS_XNEW here, and what was here in it
            bcs         @done
            jsr         HFS_POS_NEXT
            bcc         @move
            cmp         #0
            bne         @done
            jsr         HFS_END_APPEND                      ; The last one, at the end
            bcs         @done
            ldx         #HFS_POS_SIZE - 1
:
            lda         HFS_POSK,X
            sta         HFS_POSB,X
            dex
            bpl         :-
            clc

@done:
            rts

@end:
            jsr         HFS_END_APPEND
            bcs         @done
            jsr         HFS_LAST_EXT                        ; (Its place: the last)
            bcs         @done
            ldx         #3
:
            lda         HFS_LASTB,X
            sta         HFS_POSB,X
            dex
            bpl         :-
            lda         HFS_LASTO
            sta         HFS_POSO
            lda         HFS_LASTO + 1
            sta         HFS_POSO + 1
            clc
            rts

; HFS_XNEW, after the file's last extent.  OUT: C = 0; or C = 1, .A = error.  Modifies: .A, .X, .Y
HFS_END_APPEND:
            jsr         HFS_LAST_EXT
            bcs         @done
            jsr         HFS_APPEND_EXT
            bcs         @done
            lda         #1                                  ; (It may be in the entry: HFS_APPEND_EXT has
            sta         HFS_ENTCHG                          ;   written it then, but the entry changes again)

@done:
            rts

; Swap HFS_XNEW and the extent at the place.  OUT: C = 0; or C = 1, .A = a card error.  Modifies: .A, .X, .Y
HFS_POS_SWAP:
            jsr         HFS_POS_PTR
            bcs         @done
            ldy         #HFS_EXT_SIZE - 1
:
            lda         (HFS_PTR),Y
            tax
            lda         HFS_XNEW,Y
            sta         (HFS_PTR),Y
            txa
            sta         HFS_XNEW,Y
            dey
            bpl         :-
            jsr         HFS_POS_IN_ENTRY
            bne         :+
            lda         #1                                  ; (The entry: it's written at the end)
            sta         HFS_ENTCHG
            clc
            rts
:
            jsr         HFS_META_CHANGED
            clc

@done:
            rts

; HFS_PTR -> the extent at the place (an extent block's in the metadata buffer).
; OUT: C = 0; or C = 1, .A = a card error.  Modifies: .A, .X, .Y
HFS_POS_PTR:
            jsr         HFS_POS_IN_ENTRY
            bne         @block
            lda         HFS_FP
            clc
            adc         HFS_POSO
            sta         HFS_PTR
            lda         HFS_FP + 1
            adc         #0
            sta         HFS_PTR + 1
            clc
            rts

@block:
            jsr         HFS_POS_BLOCK
            bcs         @done
            lda         HFS_POSO
            sta         HFS_OFS
            lda         HFS_POSO + 1
            sta         HFS_OFS + 1
            jsr         HFS_META_AT
            clc

@done:
            rts

; The place's extent block into the metadata buffer.  OUT: C = 0; or C = 1, .A = a card error
HFS_POS_BLOCK:
            ldx         #3
:
            lda         HFS_POSB,X
            sta         SD_LBA,X
            dex
            bpl         :-
            jmp         HFS_META_GET

; Is the place in the entry (HFS_POSB = 0)?  OUT: Z = 1: it is.  Modifies: .A
HFS_POS_IN_ENTRY:
            lda         HFS_POSB
            ora         HFS_POSB + 1
            ora         HFS_POSB + 2
            ora         HFS_POSB + 3
            rts

; The place on to the next extent in the list (the entry's two, then each extent block's, as many as its
; count says).  OUT: C = 0; or C = 1, .A = 0: there's none after it; or C = 1, .A = a card error.
; Modifies: .A, .X, .Y
HFS_POS_NEXT:
            jsr         HFS_POS_IN_ENTRY
            bne         @block
            lda         HFS_POSO                            ; In the entry: its first extent, then its
            cmp         #HFS_E_EXT1                         ;   second (if that's used)
            bne         @to_blocks
            lda         #HFS_E_EXT2
            sta         HFS_POSO
            ldy         #HFS_E_EXT2 + 4
            lda         (HFS_FP),Y
            iny
            ora         (HFS_FP),Y
            bne         @ok
            bra         @none

@to_blocks:                                                 ; Then its first extent block's first
            ldy         #HFS_E_EXTBLK
            ldx         #0
:
            lda         (HFS_FP),Y
            sta         HFS_POSB,X
            iny
            inx
            cpx         #4
            bne         :-
            jsr         HFS_POS_IN_ENTRY
            beq         @none                               ; (None: HFS_POSB is 0 again)
            bra         @first

@block:
            jsr         HFS_POS_BLOCK                       ; In an extent block: the next in it, if its
            bcs         @done                               ;   count has one more
            clc
            lda         HFS_POSO
            adc         #HFS_EXT_SIZE
            sta         HFS_POSO
            lda         HFS_POSO + 1
            adc         #0
            sta         HFS_POSO + 1
            jsr         HFS_POS_INDEX                       ; (.A = its index)
            ldy         #HFS_X_COUNT
            cmp         (HFS_PTR),Y                         ; (HFS_PTR -> the block: HFS_POS_INDEX)
            bcc         @ok
            ldy         #HFS_X_NEXT                         ; No: the next block's first
            ldx         #0
:
            lda         (HFS_PTR),Y
            sta         HFS_POSB,X
            iny
            inx
            cpx         #4
            bne         :-
            jsr         HFS_POS_IN_ENTRY
            beq         @none

@first:
            lda         #HFS_X_FIRST
            sta         HFS_POSO
            stz         HFS_POSO + 1
            jsr         HFS_POS_BLOCK                       ; (Used, if its count isn't 0)
            bcs         @done
            stz         HFS_OFS
            stz         HFS_OFS + 1
            jsr         HFS_META_AT
            ldy         #HFS_X_COUNT
            lda         (HFS_PTR),Y
            beq         @none

@ok:
            clc
            rts

@none:
            lda         #0
            sec

@done:
            rts

; .A = the index of the extent at HFS_POSO in its block ((HFS_POSO - HFS_X_FIRST) / 6), and HFS_PTR -> the
; block (in the metadata buffer).  Modifies: .A, .X
HFS_POS_INDEX:
            stz         HFS_OFS
            stz         HFS_OFS + 1
            jsr         HFS_META_AT
            sec
            lda         HFS_POSO
            sbc         #HFS_X_FIRST
            tax
            lda         HFS_POSO + 1
            sbc         #0
            lsr                                             ; (/ 2: under 256 then)
            txa
            ror
            ldx         #0                                  ; (/ 3)
:
            cmp         #3
            bcc         :+
            sbc         #3
            inx
            bra         :-
:
            txa
            rts
