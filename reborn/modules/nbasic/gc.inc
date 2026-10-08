; ****************************************************************************
; gc.inc - the heap's collector (the fifth bank): mark and slide (Lisp 2's way).  What's live is what the roots reach:
; the globals and the value stack (their values that point into the heap: VT_NUM, VT_STR, VT_ARR, VT_REC with a heap
; bank), and through a live array's or record's values the strings and numbers they point at (an array's or a
; record's values never point at another array or record: a record in a record, or in an array, is its fields).
; Then each live block's new place (its forwarding address, in its header), every value pointing at one made to point
; at its new place (the roots', the live arrays' and records'), and the blocks slid down there.  heap_alloc collects
; when the heap would pass its limit (hlim, a logical bank), and takes more banks (to LB_HEAP_END) when it's still
; more than half full after.

HLIM_START      = LB_HEAP + 2   ; The heap's first limit: two banks (16K)

.bss
hlim:       .res        1                                   ; The heap's limit: its banks below this logical one
gfree:      .res        3                                   ; (The next live block's new place)
gp:         .res        3                                   ; (A block, as the heap's walked)
gsize:      .res        2                                   ; (Its size)
gkind:      .res        1
gval:       .res        5                                   ; (A value from a block)
gcnt:       .res        2                                   ; (A block's values left)
gvp:        .res        3                                   ; (A value's place in a block)
gmode:      .res        1                                   ; (0: marking; 1: updating)
gcs:        .res        1                                   ; (Collections so far)
gdims:      .res        1                                   ; (An array's dimensions)

.segment "CODE5"

; The heap collected
gc_collect:
            inc         gcs
            stz         gmode                               ; 1. Mark
            jsr         gc_roots
            jsr         gc_forward                          ; 2. Each live block's new place
            lda         #1                                  ; 3. Every reference to its new place
            sta         gmode
            jsr         gc_roots
            jsr         gc_blocks
            jsr         gc_slide                            ; 4. The blocks slid down
            jmp         code_sel

; Each root's value (the globals', then the value stack's) marked or updated (gmode)
gc_roots:
            MOVW        gv, gbase
@glob:
            lda         gv
            cmp         gnext
            lda         gv + 1
            sbc         gnext + 1
            bcs         @stack
            jsr         gc_value
            jsr         gv_next
            bra         @glob
@stack:
            MOVW        gv, stk_base
@stk:
            lda         gv
            cmp         vsp
            lda         gv + 1
            sbc         vsp + 1
            bcs         @done
            jsr         gc_value
            jsr         gv_next
            bra         @stk
@done:
            rts

gv_next:
            clc
            lda         gv
            adc         #VAL_SIZE
            sta         gv
            bcc         :+
            inc         gv + 1
:
            rts

; The value at (gv) (in the RAM): marked (gmode 0) or updated (1), if it points into the heap
gc_value:
            lda         (gv)
            cmp         #VT_NUM
            beq         :+
            cmp         #VT_STR
            beq         :+
            cmp         #VT_ARR
            beq         :+
            cmp         #VT_REC
            bne         @no
:
            ldy         #3
            lda         (gv),y
            cmp         #LB_HEAP
            bcc         @no
            cmp         #LB_HEAP_END
            bcs         @no
            jsr         gv_block                            ; (Cursor B at its block)
            lda         gmode
            bne         @update
            jmp         gc_mark
@update:
            jsr         cb_fwd                              ; (Its block's new place, past the header: t0-t2)
            ldy         #1
            lda         t0
            sta         (gv),y
            iny
            lda         t0 + 1
            sta         (gv),y
            iny
            lda         t0 + 2
            sta         (gv),y
@no:
            rts

; Cursor B at the block of the value at (gv): its far address less HDR
gv_block:
            ldy         #1
            lda         (gv),y
            sta         cb
            iny
            lda         (gv),y
            sta         cb + 1
            iny
            lda         (gv),y
            sta         cbl
            ; (fall through)
; Cursor B back HDR bytes (into the bank before, at the window's start)
cb_less_hdr:
            sec
            lda         cb
            sbc         #HDR
            sta         cb
            lda         cb + 1
            sbc         #0
            cmp         #WIN
            bcs         :+
            adc         #WIN_END - WIN
            dec         cbl
:
            sta         cb + 1
            jmp         cb_sel

; t0-t2 = the forwarding address in the block at cursor B, past its header (the value's new far address)
cb_fwd:
            ldy         #3
            lda         (cb),y                              ; (The header: in this bank? It may cross: read by the
            sta         t0                                  ;   cursor instead when it's near the window's end)
            lda         cb + 1
            cmp         #WIN_END - 1
            bcc         @here
            lda         cb
            cmp         #$100 - HDR
            bcc         @here
            jsr         cb_rd                               ; (Near the end: read across)
            jsr         cb_rd
            jsr         cb_rd
            jsr         cb_rd
            sta         t0
            jsr         cb_rd
            sta         t0 + 1
            jsr         cb_rd
            sta         t0 + 2
            bra         @plus
@here:
            iny
            lda         (cb),y
            sta         t0 + 1
            iny
            lda         (cb),y
            sta         t0 + 2
@plus:
            clc                                             ; (+ HDR)
            lda         t0
            adc         #HDR
            sta         t0
            lda         t0 + 1
            adc         #0
            cmp         #WIN_END
            bcc         :+
            sbc         #WIN_END - WIN
            inc         t0 + 2
:
            sta         t0 + 1
            rts

; The block at cursor B marked; an array's or a record's values' strings and numbers marked too
gc_mark:
            lda         (cb)
            bmi         @done                               ; (Marked already)
            ora         #KB_MARK
            sta         (cb)
            and         #$7F
            cmp         #KB_ARR
            beq         @values
            cmp         #KB_REC
            bne         @done
@values:
            jsr         blk_values                          ; (Cursor A at its values, gcnt of them)
@each:
            lda         gcnt
            ora         gcnt + 1
            beq         @done
            jsr         ca_sel
            ldy         #0
:
            jsr         ca_rd
            sta         gval,y
            iny
            cpy         #VAL_SIZE
            bne         :-
            lda         gval                                ; (A string's or a number's in the heap: marked)
            cmp         #VT_STR
            beq         :+
            cmp         #VT_NUM
            bne         @next
:
            lda         gval + 3
            cmp         #LB_HEAP
            bcc         @next
            cmp         #LB_HEAP_END
            bcs         @next
            lda         gval + 1
            sta         cb
            lda         gval + 2
            sta         cb + 1
            lda         gval + 3
            sta         cbl
            jsr         cb_less_hdr
            lda         (cb)
            ora         #KB_MARK
            sta         (cb)
@next:
            lda         gcnt
            bne         :+
            dec         gcnt + 1
:
            dec         gcnt
            bra         @each
@done:
            rts

; Cursor A at the values of the block at cursor B (an array's: past its dimensions), gcnt their count
blk_values:
            ldy         #1                                  ; (Its size)
            lda         cb
            sta         ca
            lda         cb + 1
            sta         ca + 1
            lda         cbl
            sta         cal
            jsr         ca_sel
            jsr         ca_rd                               ; (Its kind)
            and         #$7F
            sta         gkind
            jsr         ca_rd
            sta         gsize
            jsr         ca_rd
            sta         gsize + 1
            jsr         ca_rd                               ; (Past the forwarding address)
            jsr         ca_rd
            jsr         ca_rd
            sec                                             ; (Its contents: size - HDR)
            lda         gsize
            sbc         #HDR
            sta         gcnt
            lda         gsize + 1
            sbc         #0
            sta         gcnt + 1
            lda         gkind
            cmp         #KB_ARR
            bne         @count
            jsr         ca_rd                               ; (An array's dimensions: 2 + 4n bytes)
            sta         gdims
            jsr         ca_rd
            lda         gdims
            asl         a
            asl         a
            tax
            beq         :++
:
            jsr         ca_rd
            dex
            bne         :-
:
            lda         gcnt                                ; (2 + 4n off the count)
            sec
            sbc         #2
            sta         gcnt
            bcs         :+
            dec         gcnt + 1
:
            lda         gdims
            asl         a
            asl         a
            sta         t4
            lda         gcnt
            sec
            sbc         t4
            sta         gcnt
            bcs         @count
            dec         gcnt + 1
@count:
            lda         gcnt                                ; (The bytes / 5: the values)
            ldx         gcnt + 1
            jsr         div5
            sta         gcnt
            stx         gcnt + 1
            rts

; .A/.X = .A/.X / 5
div5:
            sta         t4
            stx         t4 + 1
            lda         #0
            ldx         #16
@bit:
            asl         t4
            rol         t4 + 1
            rol         a
            cmp         #5
            bcc         :+
            sbc         #5
            inc         t4
:
            dex
            bne         @bit
            lda         t4
            ldx         t4 + 1
            rts

; 2. Each live block's new place: the heap walked from its start, each marked one's forwarding address the next
; free place
gc_forward:
            jsr         gp_start
            MOVW        gfree, gp
            lda         gp + 2
            sta         gfree + 2
@block:
            jsr         gp_end
            beq         @done
            jsr         gp_header                           ; (gkind, gsize; cursor A past them)
            lda         gkind
            bpl         @next
            jsr         ca_sel
            lda         gfree
            jsr         ca_wr
            lda         gfree + 1
            jsr         ca_wr
            lda         gfree + 2
            jsr         ca_wr
            lda         gsize                               ; (The next free place: past it)
            ldx         gsize + 1
            jsr         far_add_free
@next:
            jsr         gp_past
            bra         @block
@done:
            rts

; 3. Every live array's and record's values updated (the heap walked, each marked KB_ARR or KB_REC block)
gc_blocks:
            jsr         gp_start
@block:
            jsr         gp_end
            bne         @lb902
            jmp         @done
@lb902:
            jsr         gp_header
            lda         gkind
            bmi         @lb901
            jmp         @next
@lb901:
            and         #$7F
            cmp         #KB_ARR
            beq         :+
            cmp         #KB_REC
            beq         @lb900
            jmp         @next
@lb900:
:
            lda         gp                                  ; (Cursor B at it: blk_values from there)
            sta         cb
            lda         gp + 1
            sta         cb + 1
            lda         gp + 2
            sta         cbl
            jsr         blk_values
@value:
            lda         gcnt
            ora         gcnt + 1
            beq         @next
            lda         ca                                  ; (Its place, kept: written back there)
            sta         gvp
            lda         ca + 1
            sta         gvp + 1
            lda         cal
            sta         gvp + 2
            jsr         ca_sel
            ldy         #0
:
            jsr         ca_rd
            sta         gval,y
            iny
            cpy         #VAL_SIZE
            bne         :-
            lda         ca                                  ; (The cursor after it, kept)
            pha
            lda         ca + 1
            pha
            lda         cal
            pha
            LDW         gv, gval                            ; (Updated as a root's is, then written back)
            jsr         gc_value
            lda         gvp
            sta         ca
            lda         gvp + 1
            sta         ca + 1
            lda         gvp + 2
            sta         cal
            jsr         ca_sel
            ldy         #0
:
            lda         gval,y
            jsr         ca_wr
            iny
            cpy         #VAL_SIZE
            bne         :-
            pla
            sta         cal
            pla
            sta         ca + 1
            pla
            sta         ca
            lda         gcnt
            bne         :+
            dec         gcnt + 1
:
            dec         gcnt
            bra         @value
@next:
            jsr         gp_past
            jmp         @block
@done:
            rts

; 4. The marked blocks slid down to their places (their marks cleared); hp past the last
gc_slide:
            jsr         gp_start
            MOVW        gfree, gp
            lda         gp + 2
            sta         gfree + 2
@block:
            jsr         gp_end
            beq         @done
            jsr         gp_header
            lda         gkind
            bpl         @next
            lda         gp                                  ; (From its place to gfree, all of it)
            sta         ca
            lda         gp + 1
            sta         ca + 1
            lda         gp + 2
            sta         cal
            lda         gfree
            sta         cb
            lda         gfree + 1
            sta         cb + 1
            lda         gfree + 2
            sta         cbl
            MOVW        cn, gsize
            jsr         far_copy
            lda         gfree                               ; (Its mark cleared at its new place)
            sta         cb
            lda         gfree + 1
            sta         cb + 1
            lda         gfree + 2
            sta         cbl
            jsr         cb_sel
            lda         (cb)
            and         #$7F
            sta         (cb)
            lda         gsize
            ldx         gsize + 1
            jsr         far_add_free
@next:
            jsr         gp_past
            bra         @block
@done:
            lda         gfree
            sta         hp
            lda         gfree + 1
            sta         hp + 1
            lda         gfree + 2
            sta         hp + 2
            rts

; gp = the heap's start
gp_start:
            stz         gp
            lda         #WIN
            sta         gp + 1
            lda         #LB_HEAP
            sta         gp + 2
            rts

; Z = 1 if gp is hp (the last block's end)
gp_end:
            lda         gp
            cmp         hp
            bne         :+
            lda         gp + 1
            cmp         hp + 1
            bne         :+
            lda         gp + 2
            cmp         hp + 2
:
            rts

; gkind, gsize = the block at gp's; cursor A past them (at its forwarding address)
gp_header:
            lda         gp
            sta         ca
            lda         gp + 1
            sta         ca + 1
            lda         gp + 2
            sta         cal
            jsr         ca_sel
            jsr         ca_rd
            sta         gkind
            jsr         ca_rd
            sta         gsize
            jsr         ca_rd
            sta         gsize + 1
            rts

; gp past its block (gsize)
gp_past:
            clc
            lda         gp
            adc         gsize
            sta         gp
            lda         gp + 1
            adc         gsize + 1
@wrap:
            cmp         #WIN_END
            bcc         :+
            sbc         #WIN_END - WIN
            inc         gp + 2
            bra         @wrap
:
            sta         gp + 1
            rts

; gfree += .A/.X
far_add_free:
            clc
            adc         gfree
            sta         gfree
            txa
            adc         gfree + 1
@wrap:
            cmp         #WIN_END
            bcc         :+
            sbc         #WIN_END - WIN
            inc         gfree + 2
            bra         @wrap
:
            sta         gfree + 1
            rts
