; ****************************************************************************
; sq.s - (dq0lib) the chunk lookup, a task's squares read (ENGINE-CRITIQUE.md's §4, step 2, 2026-11-14): at ()'s long
; way (AT_FIND: the square's chunk found in the hub's cache, or made, put in the window, the quick way set for it; each
; reader's chunk.c's at_find before), whether a chunk is in the cache (CK_MADE, CK_AT: the denizens' made, made9,
; made10, made16, the senses' cached, the world's own loops before), and for the line here, at () whole (sq_at: the
; quick way too, the task's at.s's in the task) and a square's portal (sq_door: chunk.c's portal_at).  The task's
; state, its vectors and tables, ../../games/dq0/lnzp.inc's struct ats (LZ_ATS); the cache's table the record
; store's (rp_find, the chunks' cache's key and stamp as chunk.c's find had them).
;
; The task's routines called: LZ_MAP's (hub_map: .A a hub's bank put in the window; it changes no call register,
; though .X and .Y may), ats's ASK (ask_maker (M_CHUNK, r0, r1, r2): the maker makes the chunk while the task waits;
; anything changed)

.include "hydra.inc"
            .include    "../games/dq0/lnzp.inc"
            .include    "dq0hub_sq.inc"         ; (The headers' numbers: the build's, games/dq0/hubgen.js)

            .export     at_find, ck_made, ck_at, sq_at, sq_door, map
            .import     rp_find

RT_STAMP    = 8                                 ; (dq0lib.s's struct rpt: the clock's place, the key, the first
RT_K        = 10                                ;   entry looked at)
RT_FROM     = 18
BLOCK       = 21                                ; (Its bytes, through RT_FREE)

.code

; AT_FIND: square r1 (x), r2 (y) of stratum r0: its chunk found in the hub's cache (or the maker asked for it), put in
; the window (the slot this task's in use), C, cx_, cy_ and at ()'s quick way set for it.  OUT: .A .X the square (its
; kind's byte), or 0 (outside the stratum).  Modifies r0-r13
at_find:
            jsr         stratum                         ; r3 the stratum, r4 ox, r5 oy: outside, none
            bcc         :+
            lda         #0
            tax
            rts
:
            lda         r4                              ; cx_, cy_; r4 cx, r5 cy
            and         #CW - 1
            ldy         #ATS_CX
            sta         (LZ_ATS),y
            lda         r5
            and         #CW - 1
            iny
            sta         (LZ_ATS),y
            jsr         shift
            ldy         #ATS_LASTK                      ; The slot found last?
            lda         (LZ_ATS),y
            cmp         #$FF
            beq         @find
            iny
            lda         (LZ_ATS),y                      ; (LSN)
            cmp         r0
            bne         @find
            ldx         #0
:
            iny                                         ; (LCX, LCY: r4, r5)
            lda         (LZ_ATS),y
            cmp         r4,x
            bne         @find
            inx
            cpx         #4
            bne         :-
            jmp         @have
@find:
            lda         r3 + 1                          ; (Kept over the lookups and the maker: s, sn, cx, cy)
            pha
            lda         r3
            pha
            lda         r0
            pha
            ldx         #3
:
            lda         r4,x
            pha
            dex
            bpl         :-
            jsr         find                            ; Found in the cache, or made and found
            cmp         #$FF
            bne         @found
            tsx                                         ; (The frame: $0101 cx, $0103 cy, $0105 sn)
            lda         $0105,x
            sta         r0
            ldy         #0
:
            lda         $0101,x
            sta         r1,y
            inx
            iny
            cpy         #4
            bne         :-
            jsr         ask
            jsr         frame
            jsr         find
@found:
            jsr         frame                           ; (k kept: the frame back, and gone)
            tay
            tsx
            txa
            clc
            adc         #7
            tax
            txs
            tya
            ldy         #ATS_LASTK
            sta         (LZ_ATS),y
            iny
            lda         r0
            sta         (LZ_ATS),y                      ; (LSN)
            ldx         #0
:
            iny                                         ; (LCX, LCY)
            lda         r4,x
            sta         (LZ_ATS),y
            inx
            cpx         #4
            bne         :-
            ldy         #ATS_LASTK                      ; Its bank and its chunk (CACHE_BANK (k), CHUNK_AT (k): k / 3,
            lda         (LZ_ATS),y                      ;   k % 3, as chunk.c's loop had them)
            ldx         #PLAN_BANKS + 1
:
            cmp         #3
            bcc         :+
            sbc         #3
            inx
            bra         :-
:
            tay
            lda         chunk_lo,y
            sta         r6
            lda         chunk_hi,y
            sta         r6 + 1
            txa
            ldy         #ATS_LBANK
            sta         (LZ_ATS),y
            iny
            lda         r6
            sta         (LZ_ATS),y                      ; (LCHUNK)
            iny
            lda         r6 + 1
            sta         (LZ_ATS),y
@have:
            ldy         #ATS_LBANK                      ; In the window
            lda         (LZ_ATS),y
            jsr         map
            ldy         #ATS_LCHUNK                     ; C, the quick way's chunk and its squares
            lda         (LZ_ATS),y
            sta         r6
            iny
            lda         (LZ_ATS),y
            sta         r6 + 1
            ldy         #ATS_C
            jsr         put6
            ldy         #ATS_CHUNK
            jsr         put6
            lda         r6
            clc
            adc         #<CH_TILE
            sta         r6
            lda         r6 + 1
            adc         #>CH_TILE
            sta         r6 + 1
            ldy         #ATS_TILE
            jsr         put6
            ldy         #SS_X0 + 7                      ; The stratum's bounds (x0, y0, w, h)
:
            lda         (r3),y
            iny
            iny
            iny
            sta         (LZ_ATS),y
            dey
            dey
            dey
            dey
            cpy         #SS_X0
            bcs         :-
.assert     ATS_X0 = SS_X0 + 3 && SS_Y0 = SS_X0 + 2 && SS_W = SS_X0 + 4 && SS_H = SS_X0 + 6, error, "sq.s: a stratum's bounds"
.assert     ATS_Y0 = ATS_X0 + 2 && ATS_W = ATS_X0 + 4 && ATS_H = ATS_X0 + 6, error, "sq.s: ats's bounds"
            ldx         #5                              ; The chunk's first square in it: cx, cy << CW_BITS
:
            asl         r4
            rol         r4 + 1
            asl         r5
            rol         r5 + 1
            dex
            bne         :-
            ldy         #ATS_OX
            ldx         #0
:
            lda         r4,x
            sta         (LZ_ATS),y
            iny
            inx
            cpx         #4
            bne         :-
.assert     ATS_OY = ATS_OX + 2, error, "sq.s: ats's ox, oy"
            ldy         #ATS_LBANK
            lda         (LZ_ATS),y
            ldy         #ATS_BANK
            sta         (LZ_ATS),y
            ldy         #ATS_SN
            lda         r0
            sta         (LZ_ATS),y
            ; (fall through)

; The square in the chunk: at_tile (r6) + cy_ * CW + cx_ in .A .X
square:
            ldy         #ATS_CY
            lda         (LZ_ATS),y
            lsr         a
            lsr         a
            lsr         a
            sta         r7                              ; (cy_ * 32's high byte)
            lda         (LZ_ATS),y
            asl         a
            asl         a
            asl         a
            asl         a
            asl         a
            dey
            ora         (LZ_ATS),y                      ; (CX)
            clc
            adc         r6
            pha
            lda         r7
            adc         r6 + 1
            tax
            pla
            rts

; r6 into ats at .Y (2 bytes)
put6:
            lda         r6
            sta         (LZ_ATS),y
            iny
            lda         r6 + 1
            sta         (LZ_ATS),y
            rts

; The frame AT_FIND keeps over its lookups back in r0 (sn), r3 (s), r4 (cx), r5 (cy): the caller's caller's (.A kept)
frame:
            pha
            tsx
            lda         $0104,x                         ; ($0101 .A, $0102-$0103 the return, then cx ...)
            sta         r4
            lda         $0105,x
            sta         r4 + 1
            lda         $0106,x
            sta         r5
            lda         $0107,x
            sta         r5 + 1
            lda         $0108,x
            sta         r0
            lda         $0109,x
            sta         r3
            lda         $010A,x
            sta         r3 + 1
            pla
            rts

; The maker asked for the chunk (ats's ASK: r0 sn, r1 cx, r2 cy)
ask:
            ldy         #ATS_ASK
            lda         (LZ_ATS),y
            sta         r12
            iny
            lda         (LZ_ATS),y
            sta         r12 + 1
            jmp         (r12)

; A hub's bank .A in the window (LZ_MAP's: the task's hub_map)
map:
            jmp         (LZ_MAP)

; Chunk r4 (cx), r5 (cy) of stratum r0 in the cache (the world bank in the window): .A its slot (its stamp made the
; newest, the slot this task's in use), or $FF.  Modifies r0-r13
find:
            lda         #WORLD_BANK
            jsr         map
            sec
            jsr         look
            cmp         #$FF
            beq         :+
            pha
            ldy         #ATS_ME
            lda         (LZ_ATS),y
            tay
            pla
            sta         HUB_WINDOW + W_INUSE,y
:
            rts

; The cache's table (in the window) looked through by the record store's RP_FIND (C: the entry's stamp made the
; newest, else none): chunk r4, r5 of stratum r0.  .A its slot, or $FF.  Its block (dq0lib.s's struct rpt) on the stack
look:
            php                                     ; (C)
            tsx
            txa
            sec
            sbc         #BLOCK
            tax
            txs
            inx
            stx         r1
            lda         #1
            sta         r1 + 1
            ldy         #RT_K - 1
:
            lda         block,y
            sta         (r1),y
            dey
            bpl         :-
            tsx                                     ; (The C kept: none, no stamp)
            lda         $0100 + BLOCK + 1,x
            lsr         a
            bcs         :+
            ldy         #RT_STAMP
            lda         #0
            sta         (r1),y
            iny
            sta         (r1),y
:
            ldy         #RT_K + 1                   ; The key: sn, cx, cy
            lda         r0
            sta         (r1),y
            ldx         #0
:
            iny
            lda         r4,x
            sta         (r1),y
            inx
            cpx         #4
            bne         :-
            lda         #0                          ; (RT_FROM, RT_NF: from the first, no free one wanted)
            ldy         #RT_FROM
            sta         (r1),y
            iny
            sta         (r1),y
            lda         r1
            sta         r0
            lda         r1 + 1
            sta         r0 + 1
            jsr         rp_find
            tay
            tsx
            txa
            clc
            adc         #BLOCK + 1
            tax
            txs
            tya
            rts
block:      .byte       0, NCSLOTS, CS_SIZE, $3E        ; (chunk.c's cst: the slot's key its sn, cx and cy, bytes 1-5;
            .word       HUB_WINDOW + W_CSLOT            ;   the world bank's table, its stamp, the clock)
            .byte       CS_STAMP, 0
            .word       HUB_WINDOW + W_STAMP

; Stratum r0 (r3), square r1, r2's place in it (r4 ox, r5 oy); C = 1 outside it
stratum:
            ldy         #ATS_STRATA
            lda         (LZ_ATS),y
            sta         r3
            iny
            lda         (LZ_ATS),y
            sta         r3 + 1
            ldx         r0
            beq         @s
:
            lda         r3
            clc
            adc         #<SS
            sta         r3
            lda         r3 + 1
            adc         #>SS
            sta         r3 + 1
            dex
            bne         :-
@s:
            ldx         #0
            ldy         #SS_X0
:
            lda         r1,x                            ; (x - x0, then y - y0)
            sec
            sbc         (r3),y
            sta         r4,x
            iny
            lda         r1 + 1,x
            sbc         (r3),y
            sta         r4 + 1,x
            iny
            inx
            inx
            cpx         #4
            bne         :-
            ldy         #SS_W                           ; (Not below w, h: outside)
            lda         r4
            cmp         (r3),y
            iny
            lda         r4 + 1
            sbc         (r3),y
            bcs         :+
            iny
            lda         r5
            cmp         (r3),y
            iny
            lda         r5 + 1
            sbc         (r3),y
:
            rts
.assert     r2 = r1 + 2 && r5 = r4 + 2, error, "sq.s: the call registers' order"

; r4, r5 >> CW_BITS
shift:
            ldx         #5
:
            lsr         r4 + 1
            ror         r4
            lsr         r5 + 1
            ror         r5
            dex
            bne         :-
            rts

; CK_MADE: chunk r1 (cx), r2 (cy) of stratum r0 in the cache (made): .A 1, else 0 (its stamp left as it is); hub_map
; (0) after.  Modifies r0-r13
ck_made:
            ldx         #3
:
            lda         r1,x
            sta         r4,x
            dex
            bpl         :-
made:
            lda         #WORLD_BANK
            jsr         map
            clc
            jsr         look
            sta         r6
            lda         #0
            jsr         map
            lda         r6
            inc         a                               ; ($FF: none, 0)
            beq         :+
            lda         #1
:
            rts

; CK_AT: square r1 (x), r2 (y) of stratum r0's chunk in the cache: .A 1, else 0 (outside the stratum too).  Modifies
; r0-r13
ck_at:
            jsr         stratum
            lda         #0
            bcs         :+
            jsr         shift
            bra         made
:
            rts

; at () whole, the line's: square r1 (x), r2 (y) of stratum r0, the quick way (the task's at.s's: a square of the
; chunk found last) or the long (AT_FIND).  OUT: .A .X the square (Z: none).  Modifies r0-r13
sq_at:
            ldy         #ATS_SN                         ; The stratum found last?
            lda         (LZ_ATS),y
            cmp         r0
            bne         @long
            ldx         #0                              ; x - x0 then y - y0, each in the bounds and in the chunk
            ldy         #ATS_X0
@xy:
            lda         r1,x
            sec
            sbc         (LZ_ATS),y
            sta         r3
            iny
            lda         r1 + 1,x
            sbc         (LZ_ATS),y
            sta         r3 + 1
            iny
            iny
            iny
            lda         r3                              ; (W, H)
            cmp         (LZ_ATS),y
            iny
            lda         r3 + 1
            sbc         (LZ_ATS),y
            bcs         @long
            iny
            iny
            iny
            lda         r3                              ; (OX, OY)
            sec
            sbc         (LZ_ATS),y
            sta         r4,x
            iny
            lda         r3 + 1
            sbc         (LZ_ATS),y
            bne         @long
            lda         r4,x
            cmp         #CW
            bcs         @long
            inx
            inx
            cpx         #4
            beq         @in
            ldy         #ATS_Y0
            bra         @xy
@long:
            jmp         at_find
@in:
            lda         r4                              ; cx_, cy_
            ldy         #ATS_CX
            sta         (LZ_ATS),y
            lda         r5
            iny
            sta         (LZ_ATS),y
            ldy         #ATS_BANK                       ; Its chunk in the window, C
            lda         (LZ_ATS),y
            jsr         map
            ldy         #ATS_CHUNK
            lda         (LZ_ATS),y
            sta         r6
            iny
            lda         (LZ_ATS),y
            sta         r6 + 1
            ldy         #ATS_C
            jsr         put6
square0:
            ldy         #ATS_TILE                       ; The square
            lda         (LZ_ATS),y
            sta         r6
            iny
            lda         (LZ_ATS),y
            sta         r6 + 1
            jmp         square
.assert     ATS_W = ATS_X0 + 4 && ATS_OX = ATS_W + 4 && ATS_Y0 = ATS_X0 + 2 && ATS_OY = ATS_OX + 2, error, "sq.s: ats's"

; The portal on at ()'s square (C's at cx_, cy_: chunk.c's portal_at), or none: .A .X (Z: none).  Modifies r6, r7
sq_door:
            ldy         #ATS_C
            lda         (LZ_ATS),y
            clc
            adc         #<CH_PORTAL
            sta         r6
            iny
            lda         (LZ_ATS),y
            adc         #>CH_PORTAL
            sta         r6 + 1
            lda         r6                              ; (nportals, the byte before: C + CH_NPORTALS)
            sec
            sbc         #CH_PORTAL - CH_NPORTALS
            sta         r7
            lda         r6 + 1
            sbc         #0
            sta         r7 + 1
            lda         (r7)
            beq         @none
            tax
@p:
            ldy         #ATS_CX
            lda         (LZ_ATS),y
            ldy         #PO_X
            cmp         (r6),y
            bne         @next
            ldy         #ATS_CY
            lda         (LZ_ATS),y
            ldy         #PO_Y
            cmp         (r6),y
            beq         @found
@next:
            lda         r6
            clc
            adc         #PO_SIZE
            sta         r6
            bcc         :+
            inc         r6 + 1
:
            dex
            bne         @p
@none:
            lda         #0
            tax
            rts
@found:
            lda         r6
            ldx         r6 + 1                          ; (Not 0: Z clear)
            rts

.rodata
chunk_lo:   .byte       <HUB_WINDOW, <(HUB_WINDOW + CHUNK_SIZE), <(HUB_WINDOW + 2 * CHUNK_SIZE)
chunk_hi:   .byte       >HUB_WINDOW, >(HUB_WINDOW + CHUNK_SIZE), >(HUB_WINDOW + 2 * CHUNK_SIZE)
