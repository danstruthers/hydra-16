; ****************************************************************************
; heap.inc - the heap and the strings (the fifth bank).  The heap is the logical banks from LB_HEAP, its blocks one
; after another from its start (hp the next's place): each its kind, its size (all of it), the collector's forwarding
; address, then its contents (a value's far address points there).  A string's and a number's contents are their
; length (2 bytes) and their bytes; an array's and a record's, values.  A block may cross from one bank to the next.

.bss
hp:         .res        3                                   ; The heap's next block: a far address
hres:       .res        3                                   ; A block's contents (heap_alloc's)
hsize:      .res        2                                   ; (heap_alloc's: all of it)
hkind:      .res        1
sla:        .res        2                                   ; (Strings' lengths)
slb:        .res        2
sfa:        .res        3                                   ; (A string's far address)
sfb:        .res        3
ad_n:       .res        1                                   ; (arr_make's: dimensions, their lowest and count each,
ad_dims:    .res        4 * 8                               ;   an element's values, the elements' kind, the slot,
ad_esize:   .res        1                                   ;   their count, the next dimension)
ad_kind:    .res        1
ad_slot:    .res        2
ad_count:   .res        2
ad_x:       .res        1
dm_info:    .res        1                                   ; (DIM's: its info, the dimensions whose lowest is given,
dm_mask:    .res        1                                   ;   OPTION BASE, a highest (biased), the dimensions of an
dm_base:    .res        1                                   ;   array, PRESERVE's old elements and their count)
dm_hi:      .res        2
dm_n:       .res        1
dm_ca:      .res        3
dm_cnt:     .res        2

.segment "CODE5"

; The heap emptied
heap_init:
            lda         #HLIM_START
            sta         hlim
            stz         hp
            lda         #WIN
            sta         hp + 1
            lda         #LB_HEAP
            sta         hp + 2
            rts

; A block of kind .Y whose contents are .A/.X bytes: hres their far address.  No room: Out of memory
heap_alloc:
            sty         hkind
            clc
            adc         #HDR
            sta         hsize
            txa
            adc         #0
            sta         hsize + 1
            bcc         :+
            lda         #ER_STRING
            ldx         #0
            jmp         b_error
:
            jsr         heap_room                           ; (Room below the limit: collected, or the limit raised)
            lda         hp                                  ; (The block at hp)
            sta         cb
            lda         hp + 1
            sta         cb + 1
            lda         hp + 2
            sta         cbl
            jsr         cb_sel
            lda         hkind
            jsr         cb_wr
            lda         hsize
            jsr         cb_wr
            lda         hsize + 1
            jsr         cb_wr
            lda         #0
            jsr         cb_wr
            jsr         cb_wr
            jsr         cb_wr
            lda         cb                                  ; (Its contents: here)
            sta         hres
            lda         cb + 1
            sta         hres + 1
            lda         cbl
            sta         hres + 2
            lda         hp                                  ; (hp: past it)
            clc
            adc         hsize
            sta         hp
            lda         hp + 1
            adc         hsize + 1
            sta         hp + 1
@wrap:
            lda         hp + 1                              ; (Past the window: into the banks after)
            cmp         #WIN_END
            bcc         @done
            sec
            sbc         #WIN_END - WIN
            sta         hp + 1
            inc         hp + 2
            lda         hp + 2
            cmp         #LB_HEAP_END
            bcc         @wrap
            lda         #ER_MEMORY
            ldx         #0
            jmp         b_error
@done:
            jmp         code_sel

; Room for hsize bytes at hp below the heap's limit: if not, the heap collected; if still not (or it's more than half
; full after), the limit raised (to LB_HEAP_END: past it, Out of memory)
heap_room:
            jsr         hp_end                              ; (.A: the logical bank its end would be in)
            cmp         hlim
            bcc         @done
            jsr         gc_collect
            jsr         hp_end
            cmp         hlim
            bcc         @half
            inc         a                                   ; (Still not: the limit past it)
            sta         hlim
@half:
            lda         hp + 2                              ; (More than half full: more banks)
            sec
            sbc         #LB_HEAP - 1
            asl         a
            sta         t4
            lda         hlim
            sec
            sbc         #LB_HEAP
            cmp         t4
            bcs         @cap
            lda         hp + 2
            sec
            sbc         #LB_HEAP - 1
            clc
            adc         hlim
            sta         hlim
@cap:
            lda         hlim
            cmp         #LB_HEAP_END + 1
            bcc         :+
            lda         #LB_HEAP_END
            sta         hlim
:
            jsr         hp_end
            cmp         hlim
            bcc         @done
            lda         #ER_MEMORY
            ldx         #0
            jmp         b_error
@done:
            rts

; .A = the logical bank hp + hsize is in
hp_end:
            clc
            lda         hp
            adc         hsize
            lda         hp + 1
            adc         hsize + 1
            ldx         hp + 2
            bcc         :+
            sbc         #0                                  ; (Past 64K: 8 banks more)
            pha
            txa
            clc
            adc         #8
            tax
            pla
:
            cmp         #WIN_END
            bcc         :+
            sbc         #WIN_END - WIN
            inx
            bra         :-
:
            txa
            rts

; A string's (heap_str) or a number's (heap_num) block of .A/.X bytes: its length written; hres it
heap_str:
            ldy         #KB_STR
            bra         heap_lenb
heap_num:
            ldy         #KB_NUM
heap_lenb:
            sta         sla
            stx         sla + 1
            clc
            adc         #2
            bcc         :+
            inx
:
            jsr         heap_alloc
            jsr         cb_at_hres
            lda         sla
            jsr         cb_wr
            lda         sla + 1
            jsr         cb_wr
            jmp         code_sel

; Cursor B at hres (its bank selected)
cb_at_hres:
            lda         hres
            sta         cb
            lda         hres + 1
            sta         cb + 1
            lda         hres + 2
            sta         cbl
            jmp         cb_sel

; ---- Pushes of the prompt's line: a copy of the code's bytes in the heap (PUSHSH, PUSHNH: [op] [n16] [bytes])
push_str_heap:
            ldy         #VT_STR
            bra         push_heap
push_num_heap:
            ldy         #VT_NUM
push_heap:
            phy
            ldy         #1
            lda         (ip),y
            sta         slb
            iny
            lda         (ip),y
            sta         slb + 1
            lda         slb
            ldx         slb + 1
            ply
            phy
            cpy         #VT_STR
            beq         :+
            jsr         heap_num
            bra         :++
:
            jsr         heap_str
:
            lda         ip                                  ; (Its bytes: from ip + 3)
            clc
            adc         #3
            sta         ca
            lda         ip + 1
            adc         #0
            sta         ca + 1
            lda         ipl
            sta         cal
            MOVW        cn, slb
            jsr         far_copy                            ; (cb: past the length heap_lenb wrote)
            pla
            sta         (vsp)
            ldy         #1
            lda         hres
            sta         (vsp),y
            iny
            lda         hres + 1
            sta         (vsp),y
            iny
            lda         hres + 2
            sta         (vsp),y
            jsr         vsp_up
            lda         slb                                 ; (ip past it: 3 + n)
            clc
            adc         #3
            sta         slb
            bcc         :+
            inc         slb + 1
:
            lda         ip
            clc
            adc         slb
            sta         ip
            lda         ip + 1
            adc         slb + 1
            sta         ip + 1
            rts

; ---- Strings

; sla = the length of the string at (vt) (sfa its far address, past its length); "": 0
str_get_a:
            lda         #0
            sta         sla
            sta         sla + 1
            ldy         #3
            lda         (vt),y
            cmp         #NOBANK
            beq         @done
            sta         cal
            sta         sfa + 2
            dey
            lda         (vt),y
            sta         ca + 1
            dey
            lda         (vt),y
            sta         ca
            jsr         ca_sel
            jsr         ca_rd
            sta         sla
            jsr         ca_rd
            sta         sla + 1
            MOVW        sfa, ca
            lda         cal
            sta         sfa + 2
@done:
            jmp         code_sel

; The same for (vu): slb, sfb
str_get_b:
            lda         #0
            sta         slb
            sta         slb + 1
            ldy         #3
            lda         (vu),y
            cmp         #NOBANK
            beq         @done
            sta         cal
            dey
            lda         (vu),y
            sta         ca + 1
            dey
            lda         (vu),y
            sta         ca
            jsr         ca_sel
            jsr         ca_rd
            sta         slb
            jsr         ca_rd
            sta         slb + 1
            MOVW        sfb, ca
            lda         cal
            sta         sfb + 2
@done:
            jmp         code_sel

; The top two strings joined: the first's place the result, the top dropped
str_cat:
            jsr         two_vals
            jsr         str_get_a
            jsr         str_get_b
            lda         slb                                 ; ("" after it: it as it is)
            ora         slb + 1
            beq         @drop
            lda         sla                                 ; ("" before it: the second)
            ora         sla + 1
            bne         @join
            ldy         #4
:
            lda         (vu),y
            sta         (vt),y
            dey
            bpl         :-
@drop:
            MOVW        vsp, vu
            rts
@join:
            lda         sla                                 ; (Their length: 64K at most)
            clc
            adc         slb
            tay
            lda         sla + 1
            adc         slb + 1
            tax
            bcc         :+
            lda         #ER_STRING
            ldx         #0
            jmp         b_error
:
            tya
            jsr         heap_str                            ; (cb: at its bytes)
            jsr         two_vals                            ; (Each again: the collector may have moved them)
            jsr         str_get_a
            jsr         str_get_b
            jsr         cb_after_len
            MOVW        ca, sfa
            lda         sfa + 2
            sta         cal
            MOVW        cn, sla
            jsr         far_copy
            MOVW        ca, sfb
            lda         sfb + 2
            sta         cal
            MOVW        cn, slb
            jsr         far_copy
            jsr         two_vals
            ldy         #1
            lda         hres
            sta         (vt),y
            iny
            lda         hres + 1
            sta         (vt),y
            iny
            lda         hres + 2
            sta         (vt),y
            MOVW        vsp, vu
            rts

; Cursor B at hres's string's bytes (past its length)
cb_after_len:
            jsr         cb_at_hres
            jsr         cb_rd
            jmp         cb_rd

; t4 = the order of the strings at (vt) and (vu): $FF, 0, 1 (by their bytes, a shorter one first if it's the other's
; start)
str_order:
            jsr         str_get_a
            jsr         str_get_b
            MOVW        cb, sfb
            lda         sfb + 2
            sta         cbl
            MOVW        ca, sfa
            lda         sfa + 2
            sta         cal
            MOVW        t0, sla                             ; (t0: the bytes left of a, t0 + 2: of b)
            MOVW        t0 + 2, slb
@byte:
            lda         t0                                  ; (a at its end: below, or equal at b's)
            ora         t0 + 1
            beq         @aend
            lda         t0 + 2
            ora         t0 + 3
            beq         @above
            jsr         ca_sel
            jsr         ca_rd
            sta         t4 + 1
            jsr         cb_sel
            jsr         cb_rd
            cmp         t4 + 1
            beq         @same
            bcc         @above                              ; (b's byte below a's)
            bra         @below
@same:
            lda         t0
            bne         :+
            dec         t0 + 1
:
            dec         t0
            lda         t0 + 2
            bne         :+
            dec         t0 + 3
:
            dec         t0 + 2
            bra         @byte
@aend:
            lda         t0 + 2
            ora         t0 + 3
            beq         @equal
@below:
            lda         #$FF
            bra         @have
@above:
            lda         #1
            bra         @have
@equal:
            lda         #0
@have:
            sta         t4
            jmp         code_sel

; FIXLEN n16: the top string padded with spaces, or cut, to n characters
str_fixlen:
            ldy         #1
            lda         (ip),y
            sta         slb
            iny
            lda         (ip),y
            sta         slb + 1
            jsr         top_vt
            jsr         str_get_a
            lda         sla                                 ; (Its length already: as it is)
            cmp         slb
            bne         :+
            lda         sla + 1
            cmp         slb + 1
            bne         :+
            rts
:
            lda         slb
            ldx         slb + 1
            jsr         heap_str
            jsr         top_vt
            jsr         str_get_a
            jsr         cb_after_len
            MOVW        ca, sfa
            lda         sfa + 2
            sta         cal
            MOVW        cn, slb                             ; (The less of the two copied, then spaces)
            lda         sla + 1
            cmp         slb + 1
            bne         :+
            lda         sla
            cmp         slb
:
            bcs         :+
            MOVW        cn, sla
:
            lda         slb                                 ; (The spaces: n less what's copied)
            sec
            sbc         cn
            sta         t0
            lda         slb + 1
            sbc         cn + 1
            sta         t0 + 1
            jsr         far_copy
            jsr         cb_sel
@pad:
            lda         t0
            ora         t0 + 1
            beq         @put
            lda         #' '
            jsr         cb_wr
            lda         t0
            bne         :+
            dec         t0 + 1
:
            dec         t0
            bra         @pad
@put:
            jsr         top_vt
            ldy         #1
            lda         hres
            sta         (vt),y
            iny
            lda         hres + 1
            sta         (vt),y
            iny
            lda         hres + 2
            sta         (vt),y
            jmp         code_sel

; ---- Arrays

; An array not made yet (the null VT_ARR at (vu): its byte 1 its elements' kind, LK_*) made as it's first used, with
; .A dimensions, each OPTION BASE's lowest to 10
arr_auto:
            sta         ad_n
            ldx         #0
            ldy         #0
@dim:
            lda         optbase
            sta         ad_dims,x
            stz         ad_dims + 1,x
            lda         #11
            sec
            sbc         optbase
            sta         ad_dims + 2,x
            stz         ad_dims + 3,x
            inx
            inx
            inx
            inx
            iny
            cpy         ad_n
            bcc         @dim
            ldy         #1
            lda         (vu),y
            sta         ad_kind
            lda         #1
            sta         ad_esize
            MOVW        ad_slot, vu
            ; (fall through)

; An array made: ad_n dimensions (ad_dims: each its lowest and count, 2 bytes each), ad_esize values an element,
; ad_kind its elements' kind (LK_NUM 0s, LK_STR ""), its VT_ARR put in the slot at ad_slot.  Past 64K: Out of memory
arr_make:
            lda         #1                                  ; (Its elements: the counts' product)
            sta         t0
            stz         t0 + 1
            stz         ad_x
@dim:
            lda         t0
            sta         t4
            lda         t0 + 1
            sta         t4 + 1
            ldx         ad_x
            lda         ad_dims + 2,x
            sta         t4 + 2
            lda         ad_dims + 3,x
            sta         t4 + 3
            jsr         mul_t4
            bcc         @lb902
            jmp         @big
@lb902:
            sta         t0
            stx         t0 + 1
            lda         ad_x                                ; (The next dimension's)
            clc
            adc         #4
            sta         ad_x
            lda         ad_n
            asl         a
            asl         a
            cmp         ad_x
            beq         :+
            bcs         @dim
:
            MOVW        ad_count, t0
            lda         t0                                  ; (Bytes: 2 + 4n + 5 * esize * count)
            sta         t4
            lda         t0 + 1
            sta         t4 + 1
            lda         ad_esize
            sta         t4 + 2
            stz         t4 + 3
            jsr         mul_t4
            bcc         @lb901
            jmp         @big
@lb901:
            sta         t4
            stx         t4 + 1
            lda         #VAL_SIZE
            sta         t4 + 2
            stz         t4 + 3
            jsr         mul_t4
            bcc         @lb900
            jmp         @big
@lb900:
            sta         t0
            stx         t0 + 1
            lda         ad_n
            asl         a
            asl         a
            clc
            adc         #2
            adc         t0
            sta         t0
            bcc         :+
            inc         t0 + 1
            beq         @big
:
            lda         t0
            ldx         t0 + 1
            ldy         #KB_ARR
            jsr         heap_alloc
            jsr         cb_at_hres                          ; (Its dimensions)
            lda         ad_n
            jsr         cb_wr
            lda         ad_esize
            jsr         cb_wr
            lda         ad_n
            asl         a
            asl         a
            tax
            ldy         #0
:
            lda         ad_dims,y
            jsr         cb_wr
            iny
            dex
            bne         :-
@elem:
            lda         ad_count                            ; (Its values: count * esize defaults)
            ora         ad_count + 1
            beq         @made
            ldx         ad_esize
:
            lda         ad_kind
            jsr         cb_default
            dex
            bne         :-
            lda         ad_count
            bne         :+
            dec         ad_count + 1
:
            dec         ad_count
            bra         @elem
@made:
            MOVW        vu, ad_slot                         ; (The slot: its VT_ARR)
            lda         #VT_ARR
            sta         (vu)
            ldy         #1
            lda         hres
            sta         (vu),y
            iny
            lda         hres + 1
            sta         (vu),y
            iny
            lda         hres + 2
            sta         (vu),y
            jmp         code_sel
@big:
            lda         #ER_MEMORY
            ldx         #0
            jmp         b_error

; The default value of kind .A (LK_*) written at cursor B.  Keeps .X
cb_default:
            phx
            tax
            lda         dv_tag,x
            jsr         cb_wr
            lda         #0
            jsr         cb_wr
            jsr         cb_wr
            lda         dv_bank,x
            jsr         cb_wr
            lda         #0
            jsr         cb_wr
            plx
            rts
dv_tag:     .byte       VT_INT, VT_STR, VT_ARR, VT_REC
dv_bank:    .byte       0, NOBANK, NOBANK, NOBANK

; .A/.X = t4/t4+1 * t4+2/t4+3 (16 bits); C = 1 past them
mul_t4:
            lda         #0
            sta         t0 + 2
            sta         t0 + 3
            ldy         #16
@bit:
            asl         t0 + 2
            rol         t0 + 3
            bcs         @over
            asl         t4
            rol         t4 + 1
            bcc         :+
            clc
            lda         t0 + 2
            adc         t4 + 2
            sta         t0 + 2
            lda         t0 + 3
            adc         t4 + 3
            sta         t0 + 3
            bcs         @over
:
            dey
            bne         @bit
            lda         t0 + 2
            ldx         t0 + 3
            clc
            rts
@over:
            sec
            rts

; ---- The fifth bank's statements (fn.inc: ST_B5 on)
st_disp5:
            tax
            jmp         (st_jt5,x)
st_jt5:
            .word       st_dim, st_erase
.assert     * - st_jt5 = (ST_B6 - ST_B5) * 2, error, "st_jt5 and fn.inc's ST_B5 group differ"

; ST_DIM (DIM, REDIM): an array made in its variable.  The stack: each dimension's lowest (if it's given) and highest,
; the variable's VT_REF, then the info: byte 1 the dimensions (1 to 8) | the elements' kind << 4 (LK_NUM, LK_STR) |
; $40 PRESERVE | $80 REDIM; byte 2 the dimensions whose lowest is given (bit i: the ith, from 0); byte 3 OPTION BASE.
; DIM of one made already: Duplicate definition.  PRESERVE: the elements copied, in their order, as many as both have
st_dim:
            jsr         pop_vt                              ; (The info)
            ldy         #1
            lda         (vt),y
            sta         dm_info
            iny
            lda         (vt),y
            sta         dm_mask
            iny
            lda         (vt),y
            sta         dm_base
            jsr         pop_vt                              ; (The variable: its slot)
            ldy         #1
            lda         (vt),y
            sta         ad_slot
            iny
            lda         (vt),y
            sta         ad_slot + 1
            lda         dm_info
            and         #$0F
            sta         ad_n
            dec         a                                   ; (Each dimension, the last first: .X 4 * its number)
            asl         a
            asl         a
            tax
@dim:
            phx
            jsr         pop_s16                             ; (Its highest, biased)
            plx
            lda         t0
            sta         dm_hi
            lda         t0 + 1
            sta         dm_hi + 1
            txa                                             ; (Its lowest: given, or OPTION BASE's)
            lsr         a
            lsr         a
            tay
            lda         dm_mask
            and         bit_of5,y
            beq         @base
            phx
            jsr         pop_s16
            plx
            bra         @lo
@base:
            lda         dm_base
            sta         t0
            lda         #$80
            sta         t0 + 1
@lo:
            lda         t0
            sta         ad_dims,x
            lda         t0 + 1
            eor         #$80
            sta         ad_dims + 1,x
            sec                                             ; (Its count: highest - lowest + 1)
            lda         dm_hi
            sbc         t0
            sta         ad_dims + 2,x
            lda         dm_hi + 1
            sbc         t0 + 1
            sta         ad_dims + 3,x
            bcc         @bad
            inc         ad_dims + 2,x
            bne         :+
            inc         ad_dims + 3,x
            beq         @bad
:
            dex
            dex
            dex
            dex
            bpl         @dim
            MOVW        vu, ad_slot                         ; (Made already?)
            ldy         #3
            lda         (vu),y
            cmp         #NOBANK
            beq         @make
            bit         dm_info
            bmi         :+
            lda         #ER_DUP
            ldx         #0
            jmp         b_error
:
            bvs         @preserve
@make:
            jsr         dm_kind
            jmp         arr_make
@bad:
            lda         #ER_SUBSCRIPT
            ldx         #0
            jmp         b_error
@preserve:
            ldy         #VAL_SIZE - 1                       ; (The old one kept on the stack while the new is made:
:                                                           ;   the collector may move it)
            lda         (vu),y
            sta         (vsp),y
            dey
            bpl         :-
            jsr         vsp_up
            jsr         dm_kind
            jsr         arr_make
            jsr         top_vt                              ; (The old one's values: cursor A, their count)
            MOVW        vu, vt
            jsr         arr_vals
            MOVW        dm_cnt, t4
            lda         ca
            sta         dm_ca
            lda         ca + 1
            sta         dm_ca + 1
            lda         cal
            sta         dm_ca + 2
            MOVW        vu, ad_slot                         ; (The new one's: cursor B)
            jsr         arr_vals
            lda         ca
            sta         cb
            lda         ca + 1
            sta         cb + 1
            lda         cal
            sta         cbl
            lda         dm_ca
            sta         ca
            lda         dm_ca + 1
            sta         ca + 1
            lda         dm_ca + 2
            sta         cal
            lda         t4 + 1                              ; (The fewer: 5 bytes each)
            cmp         dm_cnt + 1
            bne         :+
            lda         t4
            cmp         dm_cnt
:
            bcc         :+
            MOVW        t4, dm_cnt
:
            lda         t4
            sta         cn
            lda         t4 + 1
            sta         cn + 1
            asl         cn
            rol         cn + 1
            asl         cn
            rol         cn + 1
            clc
            lda         cn
            adc         t4
            sta         cn
            lda         cn + 1
            adc         t4 + 1
            sta         cn + 1
            jsr         far_copy
            jsr         pop_vt
            jmp         code_sel

; The elements' kind (dm_info's) and size (a value each), for arr_make
dm_kind:
            lda         dm_info
            lsr         a
            lsr         a
            lsr         a
            lsr         a
            and         #3
            sta         ad_kind
            lda         #1
            sta         ad_esize
            rts

; Cursor A at the first value of the array at (vu) (a VT_ARR, made), t4 its values' count
arr_vals:
            ldy         #1
            lda         (vu),y
            sta         ca
            iny
            lda         (vu),y
            sta         ca + 1
            iny
            lda         (vu),y
            sta         cal
            jsr         ca_sel
            jsr         ca_rd                               ; (Its dimensions; an element's values)
            sta         dm_n
            jsr         ca_rd
            sta         t4
            stz         t4 + 1
:
            jsr         ca_rd                               ; (Each dimension: its lowest, its count)
            jsr         ca_rd
            jsr         ca_rd
            sta         t4 + 2
            jsr         ca_rd
            sta         t4 + 3
            jsr         mul_t4
            sta         t4
            stx         t4 + 1
            dec         dm_n
            bne         :-
            rts

; t0 = the integer taken off the stack, biased (+ 32768), from -32768 to 32767: else Subscript out of range
pop_s16:
            jsr         pop_int16
            ldy         #4                                  ; (Its sign: bit 15's)
            lda         (vt),y
            eor         t0 + 1
            bmi         @bad
            lda         t0 + 1
            eor         #$80
            sta         t0 + 1
            rts
@bad:
            lda         #ER_SUBSCRIPT
            ldx         #0
            jmp         b_error
bit_of5:    .byte       $01, $02, $04, $08, $10, $20, $40, $80

; ST_ERASE: the array in the variable (the VT_REF under its elements' kind) dropped: not made, as before its DIM
st_erase:
            jsr         pop_vt                              ; (The kind)
            ldy         #1
            lda         (vt),y
            pha
            jsr         pop_vt
            ldy         #1
            lda         (vt),y
            sta         vu
            iny
            lda         (vt),y
            sta         vu + 1
            lda         #VT_ARR
            sta         (vu)
            pla
            ldy         #1
            sta         (vu),y
            iny
            lda         #0
            sta         (vu),y
            iny
            lda         #NOBANK
            sta         (vu),y
            rts

; ---- The fifth bank's functions (fn.inc: F_B5 on)
fn_disp5:
            tax
            jmp         (fn_jt5,x)

rt_todo5:
            lda         #73
            ldx         #0
            jmp         b_error
