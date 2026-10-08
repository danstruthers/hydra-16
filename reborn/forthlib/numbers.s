; ****************************************************************************
; numbers.s - HyForth's numbers (/lib/forth/numbers.fl: lib numbers): hylang's number system (integers of any size,
; fixed decimals, rationals, complex numbers, the math functions), on the numbers and math libraries (modules/numbers,
; modules/math: docs/design/plans/NUMBERS.md's step 5).  A number stack of its own, as Forth's floating-point stack
; is: in a RAM bank of the program's (8K at $8000), its entries one after another from $8000, each a number in the
; stored format (nst_at: where each starts), copied as Forth copies cells.  Its words have hylang's names, n
; before them where Forth has the name (n+ n- n* n/ n. ...).  A word Forth doesn't read as a cell or a double that
; is a number (1.25, 2/3, #xFF, 2i, 100000000000000000000: the core's num_call, before THROW -13) goes on the number
; stack, or is compiled, its bytes after a jsr (nlit; nlit_p, theirs in the dictionary, a definition's in a code
; bank).  BASE is the base of cells and numbers alike: as it changes (or hex or decimal is said: the core's num_base
; 0), the library's base is set to it ("d", "x", "16r" ...: nst_sync) when a number is next read or shown (num_base
; BASE then); set-base selects any of the library's bases, BASE then
; its digits' count (36 at most), and, if it isn't a radix (a prefix shown, balanced, least digit first, digits of
; its own), cells are read and shown by the library too (num_custom: the core's number, n_text, u_text, d_text).
;   The bank is selected only while a word works on it (nst_sel, nst_back), as the word that called it may be a
; definition's in a code bank; a string a word is given is copied out first, as it may be in a bank too.  Errors
; THROW: NE_BIG -11, NE_DIV0 -10, NE_ROOM -44 (as is the stack full), NE_DOMAIN -46, the rest -24; the number
; stack empty, -45; no math library, -21.  nvariable's numbers are in memory.fl's heap (loaded with this one, if it
; isn't): a variable's cell 0 (the number 0) or a block's address, the block its number's length (2) and bytes.

.include "forthlib.inc"
.include "numbers.inc"

NS_MAX          = 64                                        ; The number stack's entries, at most
NS_START        = $8000                                     ; Its bank's bytes
NS_END          = $A000
NBUF_SIZE       = NUM_MAX                                   ; A number's bytes, or text (n>str's, nbytes' ...)
NBASE_SIZE      = 96                                        ; A base string, its 0 too
.assert <NS_END = 0, error, "nst_fits"

.macro NCALL entry                                          ; The numbers library's entry (nin_a, nin_x, nin_y in)
            LDR         r15, entry
            jsr         nm_call
.endmacro

.macro PUSHI value                                          ; A cell pushed
            lda         #<(value)
            ldy         #>(value)
            PUSHAY
.endmacro

.bss
nmod:       .res        1                                   ; The numbers library's module (its paged ROM bank) ...
mmod:       .res        1                                   ;   the math library's (0: none) ...
lbank:      .res        1                                   ;   their RAM bank (r13: INIT's)
nsbank:     .res        1                                   ; The number stack's bank ...
nst_dep:     .res        1                                   ;   its entries ...
nst_top:     .res        2                                   ;   the byte after them ...
nst_at:      .res        NS_MAX * 2                          ;   where each starts (the deepest first)
nst_prev:    .res        1                                   ; The bank selected before it (nst_sel's) ...
nst_in:      .res        1                                   ;   <> 0: it's selected
nin_a:      .res        1                                   ; A call's .A, .X, .Y ...
nin_x:      .res        1
nin_y:      .res        1
nlen:       .res        2                                   ;   and its answer's (a result's length)
dg_var:     .res        2                                   ; digits: the precision ...
dg_set:     .res        1                                   ;   and the math library's
xt_alloc:   .res        2                                   ; memory.fl's allocate and resize
xt_resize:  .res        2
show_base:  .res        2                                   ; nst_show's base string (0: the base)
t_at:       .res        2                                   ; Text in the bank, to be typed: where, how long
t_len:      .res        2
v_at:       .res        2                                   ; n!'s variable, its block, its number's length
v_old:      .res        2
n_t:        .res        2                                   ; (A length)
d_len:      .res        2                                   ; (nst_del's)
d_idx:      .res        1
n_cnt:      .res        1                                   ; (A count)
n_flag:     .res        1                                   ; (A flag: the math library's call; a double ...)
minfo:      .res        ME_SIZE                             ; (MODINFO's)
nbase:      .res        NBASE_SIZE                          ; A base string
nbuf:       .res        NBUF_SIZE                           ; A number's bytes, or text
.code

; Its start: the libraries found (the modules numbers and math), their RAM bank and the number stack's taken, INIT;
; the core's vectors; memory.fl loaded if it isn't
lib_init:
            stz         nmod
            stz         mmod
            stz         n_cnt
@find:
            LDR         r0, minfo
            lda         n_cnt
            stx         xsave
            jsr         MODINFO
            ldx         xsave
            bcs         @found
            lda         minfo + ME_TYPE
            cmp         #HT_LIBRARY
            bne         @next
            LDR         w, s_numbers
            jsr         mi_name
            bne         :+
            lda         minfo + ME_BANK
            sta         nmod
:
            LDR         w, s_math
            jsr         mi_name
            bne         @next
            lda         minfo + ME_BANK
            sta         mmod
@next:
            inc         n_cnt
            bra         @find
@found:
            lda         nmod                                ; (No numbers library: unsupported)
            bne         :+
            lda         #<-21
            jmp         throw_a
:
            stx         xsave
            lda         #1
            jsr         BANKS_ALLOC
            bcc         @lb207
            jmp         @nobank
@lb207:
            sta         lbank
            lda         #1
            jsr         BANKS_ALLOC
            bcs         @nobank
            sta         nsbank
            ldx         xsave
            NCALL       NUM_INIT
            stz         nst_dep
            LDR         nst_top, NS_START
            stz         nst_in
            lda         #10                                 ; (The library's base decimal: BASE's, as it was)
            sta         num_base
            stz         num_base + 1
            lda         #12                                 ; (The precision: 12 digits, INIT's)
            sta         dg_var
            sta         dg_set
            stz         dg_var + 1
            LDR         num_vec, nw_hook
            LDR         num_dov, nval_run
            LDR         num_to, nw_store
            stz         num_custom
            jsr         find_mem                            ; memory.fl
            bcc         @rts
            PUSHI       s_reqmem
            PUSHI       s_reqmem_end - s_reqmem
            jsr         evaluate
            jsr         find_mem
            bcc         @rts
            lda         #<-21
            jmp         throw_a
@nobank:
            ldx         xsave
            lda         #<-59
            jmp         throw_a
@rts:
            rts

; Is minfo's name the string at w (a 0 after it)?  OUT: Z = 1 yes
mi_name:
            ldy         #0
:
            lda         (w),y
            cmp         minfo + ME_NAME,y
            bne         :+
            iny
            cmp         #0
            bne         :-
:
            rts

; memory.fl's allocate and resize, found (xt_alloc, xt_resize): C = 0; or C = 1, they aren't there
find_mem:
            PUSHI       s_alloc
            PUSHI       8
            jsr         find_name
            inx
            inx
            bcs         @rts
            jsr         hdr_xt
            MOVR        xt_alloc, w2
            PUSHI       s_resize
            PUSHI       6
            jsr         find_name
            inx
            inx
            bcs         @rts
            jsr         hdr_xt
            MOVR        xt_resize, w2
            clc
@rts:
            rts

s_numbers:  .byte       "numbers", 0
s_math:     .byte       "math", 0
s_alloc:    .byte       "allocate"
s_resize:   .byte       "resize"
s_reqmem:   .byte       "require memory.fl"
s_reqmem_end:

; ---- The libraries, the bank

; The number stack's bank selected (nst_back: the one before again).  Keeps .X
nst_sel:
            lda         RAM_BANK
            sta         nst_prev
            lda         nsbank
            sta         RAM_BANK
            lda         #1
            sta         nst_in
            rts

; The bank before it selected again.  Keeps .A, .X, .Y, C
nst_back:
            pha
            lda         nst_prev
            sta         RAM_BANK
            stz         nst_in
            pla
            rts

; THROW .A (a code from -1 to -255), the bank before the number stack's selected first
nst_throw:
            ldy         nst_in
            beq         :+
            jsr         nst_back
:
            jmp         throw_a

; The numbers library's entry r15 called (nm_call), or the math library's (mt_call): .A, .X, .Y nin_a, nin_x, nin_y,
; r13 their bank, r14 the module.  OUT: nlen its .A, .X (a result's length); its error THROWn.  Keeps .X
nm_call:
            jsr         nm_try
            bcs         nm_err
            rts
mt_call:
            jsr         mt_try
            bcs         nm_err
            rts

; The library's error .A THROWn
nm_err:
            cmp         #NE_FORMAT + 1
            bcc         :+
            lda         #0
:
            tay
            lda         err_map,y
            jmp         nst_throw
err_map:    .byte       <-24, <-11, <-10, <-24, <-44, <-46, <-24, <-24, <-24, <-24, <-24

; As nm_call and mt_call, but OUT: C = 1, .A its error (not THROWn)
nm_try:
            lda         nmod
            bra         :+
mt_try:
            lda         mmod
:
            sta         r14
            lda         lbank
            sta         r13
            stx         xsave
            ldy         nin_y
            ldx         nin_x
            lda         nin_a
            jsr         XCALL
            sta         nlen
            stx         nlen + 1
            ldx         xsave
            rts

; The math library's call next, or the numbers library's (n_flag bit 7: the math's)
lib_call:
            bit         n_flag
            bmi         mt_call
            bra         nm_call

; The math library there, and its precision digits' (1 to 100: else THROW -24).  Keeps r15
mt_ready:
            lda         mmod
            bne         :+
            lda         #<-21
            jmp         nst_throw
:
            lda         dg_var + 1
            bne         @bad
            lda         dg_var
            beq         @bad
            cmp         #101
            bcs         @bad
            cmp         dg_set
            beq         @rts
            sta         nin_a
            lda         r15
            pha
            lda         r15 + 1
            pha
            LDR         r15, MATH_DIGITS
            jsr         mt_call
            pla
            sta         r15 + 1
            pla
            sta         r15
            lda         nin_a
            sta         dg_set
@rts:
            rts
@bad:
            lda         #<-24
            jmp         nst_throw

; BASE given the library, if it has changed since it was last: the radix's base ("d", "x", "b", "o", or "Nr", 2
; to 80; another is left as it was), and cells no longer read and shown by the library (num_custom 0).  Keeps .X
nst_sync:
            lda         base
            cmp         num_base
            bne         @set
            lda         base + 1
            cmp         num_base + 1
            bne         @set
            rts
@set:
            stz         num_custom
            lda         base
            sta         num_base
            lda         base + 1
            sta         num_base + 1
            bne         @rts
            lda         base
            ldy         #'d'
            cmp         #10
            beq         @one
            ldy         #'x'
            cmp         #16
            beq         @one
            ldy         #'b'
            cmp         #2
            beq         @one
            ldy         #'o'
            cmp         #8
            beq         @one
            cmp         #2
            bcc         @rts
            cmp         #81
            bcs         @rts
            ldy         #'0' - 1                            ; (Its tens and units, then r)
            sec
:
            iny
            sbc         #10
            bcs         :-
            adc         #10 + '0'
            sta         nbase + 1
            sty         nbase
            lda         #'r'
            sta         nbase + 2
            stz         nbase + 3
            bra         @call
@one:
            sty         nbase
            stz         nbase + 1
@call:
            LDR         r0, nbase
            LDR         r15, NUM_SET_BASE
            jsr         nm_try
@rts:
            rts

; ---- The number stack (its bank selected, but where it says)

; At least .A entries, or THROW -45.  Keeps .X
nst_need:
            cmp         nst_dep
            beq         :+
            bcs         @under
:
            rts
@under:
            lda         #<-45
            jmp         nst_throw

; Room for one more entry, or THROW -44.  Keeps .X
nst_room:
            lda         nst_dep
            cmp         #NS_MAX
            bcs         nst_full
            rts
nst_full:
            lda         #<-44
            jmp         nst_throw

; Room for tmp bytes more, or THROW -44.  Keeps .X
nst_fits:
            clc
            lda         nst_top
            adc         tmp
            tay
            lda         nst_top + 1
            adc         tmp + 1
            bcs         nst_full
            cmp         #>NS_END
            bcc         :+
            bne         nst_full
            cpy         #0
            bne         nst_full
:
            rts

; Entry .A from the top (0 the top): its address, .A/.Y.  Keeps .X
nst_addr:
            eor         #$FF
            clc
            adc         nst_dep
            asl
            tay
            lda         nst_at,y
            pha
            lda         nst_at + 1,y
            tay
            pla
            rts

; Entry .A from the top: w2 its address, tmp its length; .Y its index * 2.  Keeps .X
nst_ent:
            eor         #$FF
            clc
            adc         nst_dep
            asl
            tay
            lda         nst_at,y
            sta         w2
            lda         nst_at + 1,y
            sta         w2 + 1
            phy
            iny
            iny
            tya
            lsr
            cmp         nst_dep                              ; (The top's end: nst_top)
            bcc         :+
            lda         nst_top
            ldy         nst_top + 1
            bra         @len
:
            lda         nst_at,y
            pha
            lda         nst_at + 1,y
            tay
            pla
@len:
            sec
            sbc         w2
            sta         tmp
            tya
            sbc         w2 + 1
            sta         tmp + 1
            ply
            rts

; r2 the stack's end, r3 the room after it.  Keeps .X
nst_r23:
            lda         nst_top
            sta         r2
            lda         nst_top + 1
            sta         r2 + 1
            sec
            lda         #<NS_END
            sbc         nst_top
            sta         r3
            lda         #>NS_END
            sbc         nst_top + 1
            sta         r3 + 1
            rts

; r0 the top's address, r1 the one below's (nst_r01); or r0 and r1 the top's (nst_r00).  Keeps .X
nst_r01:
            lda         #1
            jsr         nst_addr
            sta         r0
            sty         r0 + 1
            lda         #0
            jsr         nst_addr
            sta         r1
            sty         r1 + 1
            rts
nst_r00:
            lda         #0
            jsr         nst_addr
            sta         r0
            sty         r0 + 1
            sta         r1
            sty         r1 + 1
            rts

; tmp bytes copied from w2 to w (w at or below w2: forward).  Modifies w, w2, tmp + 1.  Keeps .X
nst_copy:
            ldy         #0
            lda         tmp + 1
            beq         @part
@page:
            lda         (w2),y
            sta         (w),y
            iny
            bne         @page
            inc         w + 1
            inc         w2 + 1
            dec         tmp + 1
            bne         @page
@part:
            cpy         tmp
            beq         @done
            lda         (w2),y
            sta         (w),y
            iny
            bra         @part
@done:
            rts

; The result (at nst_top, nlen long) pushed.  Keeps .X
nst_push:
            lda         nst_dep
            asl
            tay
            lda         nst_top
            sta         nst_at,y
            clc
            adc         nlen
            sta         nst_top
            lda         nst_top + 1
            sta         nst_at + 1,y
            adc         nlen + 1
            sta         nst_top + 1
            inc         nst_dep
            rts

; The number at w2 (tmp bytes, at or above nst_top) pushed, copied down to nst_top.  Keeps .X
nst_put:
            MOVR        w, nst_top
            MOVR        nlen, tmp
            jsr         nst_copy
            bra         nst_push

; The top .A entries replaced by the result (at nst_top, nlen long), moved down to the first one's place.  Keeps .X
nst_rep:
            sta         n_cnt
            sec
            lda         nst_dep
            sbc         n_cnt
            sta         nst_dep
            asl
            tay
            MOVR        w2, nst_top
            lda         nst_at,y
            sta         nst_top
            lda         nst_at + 1,y
            sta         nst_top + 1
            MOVR        tmp, nlen
            bra         nst_put

; The top dropped.  Keeps .X
nst_drop:
            dec         nst_dep
            lda         nst_dep
            asl
            tay
            lda         nst_at,y
            sta         nst_top
            lda         nst_at + 1,y
            sta         nst_top + 1
            rts

; A copy of entry .A from the top pushed.  Keeps .X
nst_pick:
            pha
            inc
            jsr         nst_need
            jsr         nst_room
            pla
            jsr         nst_ent
            jsr         nst_fits
            bra         nst_put

; Entry .A from the top (1 or more) taken out, those above it moved down.  Keeps .X
nst_del:
            jsr         nst_ent
            sty         d_idx
            MOVR        d_len, tmp
            MOVR        w, w2
            clc
            lda         w2
            adc         tmp
            sta         w2
            lda         w2 + 1
            adc         tmp + 1
            sta         w2 + 1
            sec
            lda         nst_top
            sbc         w2
            sta         tmp
            lda         nst_top + 1
            sbc         w2 + 1
            sta         tmp + 1
            jsr         nst_copy
            ldy         d_idx
@fix:
            iny
            iny
            tya
            lsr
            cmp         nst_dep
            bcs         @done
            sec
            lda         nst_at,y
            sbc         d_len
            sta         nst_at - 2,y
            lda         nst_at + 1,y
            sbc         d_len + 1
            sta         nst_at - 1,y
            bra         @fix
@done:
            dec         nst_dep
            sec
            lda         nst_top
            sbc         d_len
            sta         nst_top
            lda         nst_top + 1
            sbc         d_len + 1
            sta         nst_top + 1
            rts

; The top's bytes copied to nbuf (nlen of them), and dropped (the bank selected and back)
top_buf:
            jsr         nst_sel
            lda         #1
            jsr         nst_need
            lda         #0
            jsr         nst_ent
            MOVR        nlen, tmp
            LDR         w, nbuf
            jsr         nst_copy
            jsr         nst_drop
            jmp         nst_back

; The number at w (its length (2), then its bytes: in the dictionary) pushed
push_w:
            ldy         #1
            lda         (w),y
            sta         tmp + 1
            lda         (w)
            sta         tmp
            clc
            lda         w
            adc         #2
            sta         w2
            lda         w + 1
            adc         #0
            sta         w2 + 1
            jsr         nst_sel
            jsr         nst_room
            jsr         nst_fits
            jsr         nst_put
            jmp         nst_back

; The number 0 pushed
push_zero:
            jsr         nst_sel
            jsr         nst_room
            LDR         tmp, 1
            jsr         nst_fits
            MOVR        w, nst_top
            lda         #0
            sta         (w)
            LDR         nlen, 1
            jsr         nst_push
            jmp         nst_back

; ---- The library's calls on the stack: r15 the entry, nin_* what it takes in .A, .X, .Y

; The top replaced by the numbers library's r15 of it (nm_un; mt_un, the math library's); r1 the top too
nm_un:
            stz         n_flag
            bra         :+
mt_un:
            jsr         mt_ready
            lda         #$80
            sta         n_flag
:
            jsr         nst_sel
            lda         #1
            jsr         nst_need
            jsr         nst_r00
            jsr         nst_r23
            jsr         lib_call
            lda         #1
            jsr         nst_rep
            jmp         nst_back

; The top two replaced by the numbers library's r15 of them (nm_bin; mt_bin, the math library's): r0 the one
; below, r1 the top
nm_bin:
            stz         n_flag
            bra         :+
mt_bin:
            jsr         mt_ready
            lda         #$80
            sta         n_flag
:
            jsr         nst_sel
            lda         #2
            jsr         nst_need
            jsr         nst_r01
            jsr         nst_r23
            jsr         lib_call
            lda         #2
            jsr         nst_rep
            jmp         nst_back

; The numbers library's r15 pushed (nm_new; mt_new, the math library's)
nm_new:
            stz         n_flag
            bra         :+
mt_new:
            jsr         mt_ready
            lda         #$80
            sta         n_flag
:
            jsr         nst_sel
            jsr         nst_room
            jsr         nst_r23
            jsr         lib_call
            jsr         nst_push
            jmp         nst_back

; u (the top cell) pushed on the number stack, a number
u_push:
            lda         dlo,x
            sta         r0
            lda         dhi,x
            sta         r0 + 1
            inx
            stz         r1
            stz         r1 + 1
            stz         nin_y
            LDR         r15, NUM_FROM_INT
            bra         nm_new

; ---- The words: the number stack

            HEADER      "ndepth", 0
nw_depth:                                                   ; ( -- u )
            lda         nst_dep
            ldy         #0
            PUSHAY
            rts

            HEADER      "ndrop", 0
nw_drop:                                                    ; ( N: x -- )
            lda         #1
            jsr         nst_need
            jmp         nst_drop

            HEADER      "ndup", 0
nw_dup:                                                     ; ( N: x -- x x )
            lda         #0
            bra         pick_w

            HEADER      "nover", 0
nw_over:                                                    ; ( N: x y -- x y x )
            lda         #1
pick_w:
            pha
            jsr         nst_sel
            pla
            jsr         nst_pick
            jmp         nst_back

            HEADER      "nswap", 0
nw_swap:                                                    ; ( N: x y -- y x ): x copied on top, then taken out
            lda         #1
            bra         :+

            HEADER      "nrot", 0
nw_rot:                                                     ; ( N: x y z -- y z x )
            lda         #2
:
            pha
            jsr         nst_sel
            pla
            pha
            jsr         nst_pick
            pla
            inc
            jsr         nst_del
            jmp         nst_back

; ---- Arithmetic

            HEADER      "n+", 0
nw_add:                                                     ; ( N: x y -- x+y )
            LDR         r15, NUM_ADD
            jmp         nm_bin

            HEADER      "n-", 0
nw_sub:
            LDR         r15, NUM_SUB
            jmp         nm_bin

            HEADER      "n*", 0
nw_mul:
            LDR         r15, NUM_MUL
            jmp         nm_bin

            HEADER      "n/", 0
nw_div:                                                     ; ( N: x y -- x/y ): exact (2/3), as hylang's /
            LDR         r15, NUM_DIV
            jmp         nm_bin

            HEADER      "n/mod", 0
nw_divmod:                                                  ; ( N: x y -- r q ): integers', q toward zero, r x's sign
            jsr         nst_sel
            lda         #2
            jsr         nst_need
            jsr         nst_r01
            jsr         nst_r23
            lsr         r3 + 1                              ; (The remainder at the stack's end, the quotient after
            ror         r3                                  ;   half the room)
            MOVR        r5, r2
            MOVR        r6, r3
            MOVR        t_at, r2
            clc
            lda         r2
            adc         r3
            sta         r2
            lda         r2 + 1
            adc         r3 + 1
            sta         r2 + 1
            NCALL       NUM_IDIV
            MOVR        v_at, r2                            ; (The quotient: where, how long)
            MOVR        v_old, nlen
            MOVR        tmp, r6                             ; The remainder ...
            jsr         nst_drop
            jsr         nst_drop
            MOVR        w2, t_at
            jsr         nst_put
            MOVR        w2, v_at                            ;   and the quotient
            MOVR        tmp, v_old
            jsr         nst_put
            jmp         nst_back

            HEADER      "nnegate", 0
nw_negate:
            LDR         r15, NUM_NEG
            jmp         nm_un

            HEADER      "nabs", 0
nw_abs:
            LDR         r15, NUM_ABS
            jmp         nm_un

            HEADER      "ngcd", 0
nw_gcd:
            LDR         r15, NUM_GCD
            jmp         nm_bin

            HEADER      "npow", 0
nw_pow:                                                     ; ( N: x y -- x^y ): any real y (the math library's;
            lda         mmod                                ;   without it, a whole y)
            beq         :+
            LDR         r15, MATH_RPOW
            jmp         mt_bin
:
            LDR         r15, NUM_POW
            jmp         nm_bin

; ---- Comparisons and tests

; The top two's order (CMP: $FF, 0, 1: .A, its flags), dropped
n_cmp:
            jsr         nst_sel
            lda         #2
            jsr         nst_need
            jsr         nst_r01
            NCALL       NUM_CMP
            jsr         nst_drop
            jsr         nst_drop
            jsr         nst_back
            lda         nlen
            rts

; ( -- true | false )
push_true:
            lda         #$FF
            tay
            PUSHAY
            rts
push_false:
            lda         #0
            tay
            PUSHAY
            rts

            HEADER      "ncompare", 0
nw_compare:                                                 ; ( -- -1 | 0 | 1 ) ( N: x y -- )
            jsr         n_cmp
            ldy         #0
            cmp         #$FF
            bne         :+
            dey
:
            PUSHAY
            rts

            HEADER      "n=", 0
nw_eq:                                                      ; ( -- flag ) ( N: x y -- )
            jsr         n_cmp
            beq         push_true
            bra         push_false

            HEADER      "n<", 0
nw_less:
            jsr         n_cmp
            bmi         push_true
            bra         push_false

            HEADER      "n>", 0
nw_greater:
            jsr         n_cmp
            cmp         #1
            beq         push_true
            bra         push_false

            HEADER      "n0=", 0
nw_zeq:                                                     ; ( -- flag ) ( N: x -- )
            jsr         push_zero
            bra         nw_eq

            HEADER      "n0<", 0
nw_zless:
            jsr         push_zero
            bra         nw_less

; The top's kind (NK_), dropped: .A
n_kind:
            jsr         nst_sel
            lda         #1
            jsr         nst_need
            jsr         nst_r00
            NCALL       NUM_KIND
            jsr         nst_drop
            jsr         nst_back
            lda         nlen
            rts

            HEADER      "int?", 0
nw_intq:                                                    ; ( -- flag ) ( N: x -- )
            lda         #NK_INT
            bra         :+

            HEADER      "fixed?", 0
nw_fixedq:
            lda         #NK_FIXED
            bra         :+

            HEADER      "rational?", 0
nw_ratq:
            lda         #NK_RATIONAL
            bra         :+

            HEADER      "complex?", 0
nw_cpxq:
            lda         #NK_COMPLEX
:
            sta         n_cnt
            jsr         n_kind
            cmp         n_cnt
            beq         :+
            jmp         push_false
:
            jmp         push_true

; ---- Conversions

            HEADER      "s>n", 0
nw_s2n:                                                     ; ( n -- ) ( N: -- x )
            lda         dlo,x
            sta         r0
            lda         dhi,x
            sta         r0 + 1
            inx
            asl                                             ; (Its sign, the high cell)
            lda         #0
            adc         #$FF
            eor         #$FF
            sta         r1
            sta         r1 + 1
            bra         :+

            HEADER      "d>n", 0
nw_d2n:                                                     ; ( d -- ) ( N: -- x )
            lda         dlo,x
            sta         r1
            lda         dhi,x
            sta         r1 + 1
            lda         dlo + 1,x
            sta         r0
            lda         dhi + 1,x
            sta         r0 + 1
            inx
            inx
:
            lda         #1
            sta         nin_y
            LDR         r15, NUM_FROM_INT
            jmp         nm_new

; The top's integer part (toward zero), as a machine integer: r4/r5, .A TO_INT's (0 it fits in 32 bits signed, 1
; unsigned, 2 neither), dropped
n_int:
            jsr         nst_sel
            lda         #1
            jsr         nst_need
            jsr         nst_r00
            jsr         nst_r23
            NCALL       NUM_TRUNCATE
            MOVR        r0, nst_top
            NCALL       NUM_TO_INT
            jsr         nst_drop
            jsr         nst_back
            lda         nlen
            rts

            HEADER      "n>s", 0
nw_n2s:                                                     ; ( -- n ) ( N: x -- ): -32768 to 65535, or THROW -11
            jsr         n_int
            bne         n_range
            lda         r5
            ora         r5 + 1
            beq         :+
            lda         r5
            and         r5 + 1
            cmp         #$FF
            bne         n_range
            lda         r4 + 1
            bpl         n_range
:
            lda         r4
            ldy         r4 + 1
            PUSHAY
            rts
n_range:
            lda         #<-11
            jmp         throw_a

            HEADER      "n>d", 0
nw_n2d:                                                     ; ( -- d ) ( N: x -- ): 32 bits, signed or not
            jsr         n_int
            cmp         #2
            bcs         n_range
            lda         r4
            ldy         r4 + 1
            PUSHAY
            lda         r5
            ldy         r5 + 1
            PUSHAY
            rts

            HEADER      "truncate", 0
nw_truncate:
            LDR         r15, NUM_TRUNCATE
            jmp         nm_un

            HEADER      "nfloor", 0
nw_floor:
            LDR         r15, NUM_FLOOR
            jmp         nm_un

            HEADER      "nround", 0
nw_round:
            LDR         r15, NUM_ROUND
            jmp         nm_un

            HEADER      "to-fixed", 0
nw_tofixed:                                                 ; ( u -- ) ( N: x -- y ): u places, cut short
            lda         dlo,x
            sta         nin_a
            lda         dhi,x
            sta         nin_x
            inx
            LDR         r15, NUM_TO_FIXED
            jmp         nm_un

            HEADER      "to-rational", 0
nw_torat:
            LDR         r15, NUM_TO_RATIONAL
            jmp         nm_un

            HEADER      "rational.n", 0
nw_ratn:
            LDR         r15, NUM_NUMERATOR
            jmp         nm_un

            HEADER      "rational.d", 0
nw_ratd:
            LDR         r15, NUM_DENOMINATOR
            jmp         nm_un

            HEADER      "complex", 0
nw_complex:                                                 ; ( N: re im -- z )
            LDR         r15, NUM_COMPLEX
            jmp         nm_bin

            HEADER      "nrandom", 0
nw_random:                                                  ; ( N: n -- r ): 0 to n - 1; n 0: a fixed decimal
            lda         mmod                                ;   from 0 to 1, of digits' digits
            beq         :+
            jsr         mt_ready
:
            LDR         r15, NUM_RANDOM
            jmp         nm_un

            HEADER      "nseed", 0
nw_seed:                                                    ; ( u -- ): the generator seeded (0: from the clock)
            lda         dlo,x
            sta         r0
            lda         dhi,x
            sta         r0 + 1
            inx
            NCALL       NUM_SEED
            rts

            HEADER      "nfib", 0
nw_fib:
            LDR         r15, NUM_FIB
            jmp         nm_un

; ---- Bits

            HEADER      "nand", 0
nw_and:
            lda         #0
            bra         :+

            HEADER      "nor", 0
nw_or:
            lda         #1
            bra         :+

            HEADER      "nxor", 0
nw_xor:
            lda         #2
:
            sta         nin_y
            LDR         r15, NUM_BITS
            jmp         nm_bin

            HEADER      "ninvert", 0
nw_invert:
            lda         #3
            sta         nin_y
            LDR         r15, NUM_BITS
            jmp         nm_un

            HEADER      "nlshift", 0
nw_lshift:                                                  ; ( u -- ) ( N: x -- y )
            lda         #4
            bra         :+

            HEADER      "nrshift", 0
nw_rshift:                                                  ; ( u -- ) ( N: x -- y ): toward minus infinity
            lda         #5
:
            pha
            jsr         u_push
            pla
            sta         nin_y
            LDR         r15, NUM_BITS
            jmp         nm_bin

            HEADER      "nbit?", 0
nw_bitq:                                                    ; ( u -- flag ) ( N: x -- )
            jsr         u_push
            jsr         nst_sel
            jsr         nst_r01
            lda         #6
            sta         nin_y
            NCALL       NUM_BITS
            jsr         nst_drop
            jsr         nst_drop
            jsr         nst_back
            lda         nlen
            beq         :+
            jmp         push_true
:
            jmp         push_false

            HEADER      "nbytes", 0
nw_bytes:                                                   ; ( -- c-addr u ) ( N: x -- ): its stored format's
            jsr         top_buf                             ;   bytes, in a buffer of its own
            PUSHI       nbuf
            lda         nlen
            ldy         nlen + 1
            PUSHAY
            rts

            HEADER      "nfrom-bytes", 0
nw_frombytes:                                               ; ( c-addr u -- ) ( N: -- x ): a number's bytes
            lda         dlo,x
            sta         tmp
            sta         r1
            cmp         #<(NBUF_SIZE + 1)
            lda         dhi,x
            sta         tmp + 1
            sta         r1 + 1
            sbc         #>(NBUF_SIZE + 1)
            bcc         :+
            lda         #<-24
            jmp         throw_a
:
            lda         dlo + 1,x
            sta         w2
            lda         dhi + 1,x
            sta         w2 + 1
            inx
            inx
            LDR         w, nbuf
            jsr         nst_copy
            LDR         r0, nbuf
            LDR         r15, NUM_BYTES
            jmp         nm_new

; ---- Text

; ( c-addr u -- ): the string to nbase, a 0 after it (95 bytes at most: THROW -24)
base_str:
            lda         dhi,x
            bne         @long
            lda         dlo,x
            cmp         #NBASE_SIZE
            bcs         @long
            sta         n_t
            lda         dlo + 1,x
            sta         w2
            lda         dhi + 1,x
            sta         w2 + 1
            inx
            inx
            ldy         n_t
            lda         #0
            sta         nbase,y
:
            dey
            cpy         #$FF
            beq         :+
            lda         (w2),y
            sta         nbase,y
            bra         :-
:
            rts
@long:
            lda         #<-24
            jmp         throw_a

; The top as text in the bank, in show_base (0: the base), dropped: t_at, t_len; the bank selected
nst_show:
            jsr         nst_sync
            jsr         nst_sel
            lda         #1
            jsr         nst_need
            jsr         nst_r00
            jsr         nst_r23
            MOVR        r4, show_base
            NCALL       NUM_DISPLAY
            MOVR        t_at, nst_top
            MOVR        t_len, nlen
            jmp         nst_drop

; The text t_at, t_len long, in the bank (selected), typed (through nbuf: the bank before selected for TYPE), the
; bank before selected
nst_type:
            lda         t_len
            ora         t_len + 1
            beq         @done
            lda         t_len                               ; (nbuf's size of it at most)
            ldy         t_len + 1
            cmp         #<NBUF_SIZE
            pha
            tya
            sbc         #>NBUF_SIZE
            pla
            bcc         :+
            lda         #<NBUF_SIZE
            ldy         #>NBUF_SIZE
:
            sta         tmp
            sty         tmp + 1
            sta         n_t
            sty         n_t + 1
            MOVR        w2, t_at
            LDR         w, nbuf
            jsr         nst_copy
            clc
            lda         t_at
            adc         n_t
            sta         t_at
            lda         t_at + 1
            adc         n_t + 1
            sta         t_at + 1
            sec
            lda         t_len
            sbc         n_t
            sta         t_len
            lda         t_len + 1
            sbc         n_t + 1
            sta         t_len + 1
            jsr         nst_back
            PUSHI       nbuf
            lda         n_t
            ldy         n_t + 1
            PUSHAY
            jsr         type
            jsr         nst_sel
            bra         nst_type
@done:
            jmp         nst_back

            HEADER      "n.", 0
nw_dot:                                                     ; ( N: x -- ): in the base, a space after it
            stz         show_base
            stz         show_base + 1
n_dot:
            jsr         nst_show
            jsr         nst_type
            jmp         space

            HEADER      "n.base", 0
nw_dotbase:                                                 ; ( c-addr u -- ) ( N: x -- ): in that base
            jsr         base_str
            LDR         show_base, nbase
            bra         n_dot

            HEADER      "n>str", 0
nw_tostr:                                                   ; ( -- c-addr u ) ( N: x -- ): in the base, in a
            stz         show_base                           ;   buffer of its own (1040 characters at most:
            stz         show_base + 1                       ;   THROW -17)
            jsr         nst_show
            lda         t_len
            cmp         #<(NBUF_SIZE + 1)
            lda         t_len + 1
            sbc         #>(NBUF_SIZE + 1)
            bcc         :+
            lda         #<-17
            jmp         nst_throw
:
            MOVR        w2, t_at
            LDR         w, nbuf
            MOVR        tmp, t_len
            jsr         nst_copy
            jsr         nst_back
            PUSHI       nbuf
            lda         t_len
            ldy         t_len + 1
            PUSHAY
            rts

            HEADER      "n.s", 0
nw_dots:                                                    ; ( -- ): "<depth> numbers", the top last
            lda         #'<'
            jsr         emit_a
            lda         nst_dep
            ldy         #0
            PUSHAY
            jsr         u_text
            jsr         type
            lda         #'>'
            jsr         emit_a
            jsr         space
            lda         nst_dep
            sta         n_cnt
@item:
            lda         n_cnt
            beq         @done
            dec         n_cnt
            jsr         nst_sync
            jsr         nst_sel
            lda         n_cnt                               ; (Entry n_cnt from the top)
            jsr         nst_addr
            sta         r0
            sty         r0 + 1
            jsr         nst_r23
            stz         r4
            stz         r4 + 1
            NCALL       NUM_DISPLAY
            MOVR        t_at, nst_top
            MOVR        t_len, nlen
            jsr         nst_type
            jsr         space
            bra         @item
@done:
            rts

            HEADER      ">n", 0
nw_ton:                                                     ; ( c-addr u -- flag ) ( N: -- x | ): the whole text a
            lda         dhi,x                               ;   number, in the base (255 characters at most)
            bne         @long
            lda         dlo,x
            sta         tmp
            sta         n_t
            stz         tmp + 1
            lda         dlo + 1,x
            sta         w2
            lda         dhi + 1,x
            sta         w2 + 1
            inx
            LDR         w, nbuf
            jsr         nst_copy
            jsr         nst_sync
            jsr         nst_sel
            jsr         nst_room
            LDR         r0, nbuf
            lda         n_t
            sta         r1
            stz         r1 + 1
            stz         r4
            stz         r4 + 1
            lda         #2                                  ; (The whole text)
            sta         nin_y
            jsr         nst_r23
            LDR         r15, NUM_PARSE
            jsr         nm_try
            bcc         :+
            cmp         #NE_NOTNUM
            beq         @no
            jmp         nm_err
:
            jsr         nst_push
            jsr         nst_back
            jmp         true_tos
@no:
            jsr         nst_back
            jmp         zero_tos
@long:
            inx
            jmp         zero_tos

            HEADER      "set-base", 0
nw_setbase:                                                 ; ( c-addr u -- ): the library's base (not one: THROW
            jsr         base_str                            ;   -24), and BASE its digits' count (36 at most)
            LDR         r0, nbase
            NCALL       NUM_SET_BASE
            jsr         base_kind
            sta         base
            stz         base + 1
            sta         num_base
            stz         num_base + 1
            lda         #0
            bcc         :+
            lda         #$80
:
            sta         num_custom
            rts

            HEADER      "get-base", 0
nw_getbase:                                                 ; ( -- c-addr u ): the library's base
            jsr         nst_sync
            LDR         r2, nbase
            LDR         r3, NBASE_SIZE
            NCALL       NUM_GET_BASE
            PUSHI       nbase
            lda         nlen
            ldy         nlen + 1
            PUSHAY
            rts

; The base string in nbase (n_t long, SET_BASE's: a base): .A its digits' count (36 at most); C = 0 if it's a radix
; as BASE has one (no modifiers, a named base or "Nr" of 2 to 36: its digits 0-9 and A-Z, the most significant
; first, no prefix shown); else C = 1.  Keeps .X
base_kind:
            phx
            ldy         #0
            stz         n_flag                              ; (Its modifiers: # < > = + -)
@mod:
            lda         nbase,y
            ldx         #MODS_N - 1
:
            cmp         mods,x
            beq         @skip
            dex
            bpl         :-
            cmp         #'['
            beq         @set
            cmp         #'9' + 1
            bcc         @radix
            ora         #$20                                ; (A letter: lower case)
            ldx         #NAMED_N - 1
:
            cmp         named,x
            beq         :+
            dex
            bpl         :-
            lda         #10
            bra         @custom
:
            lda         named_n,x
            cpx         #PLAIN_N
            bcs         @custom
            bra         @plain
@skip:
            dec         n_flag
            iny
            bra         @mod
@radix:                                                     ; (Its digits: 1 or 2)
            and         #$0F
            sta         tmp
            iny
            lda         nbase,y
            cmp         #'9' + 1
            bcs         :+
            and         #$0F
            pha
            lda         tmp
            asl
            asl
            adc         tmp
            asl
            sta         tmp
            pla
            clc
            adc         tmp
            sta         tmp
:
            lda         tmp
            cmp         #37
            bcs         @custom
@plain:
            bit         n_flag
            bmi         @custom
            plx
            clc
            rts
@set:                                                       ; ([digits]: the length less the brackets and modifiers)
            sty         tmp
            sec
            lda         n_t
            sbc         #2
            sbc         tmp
@custom:
            cmp         #37
            bcc         :+
            lda         #36
:
            plx
            sec
            rts

mods:       .byte       "#<>=+-"
MODS_N      = * - mods
named:      .byte       "btqvfsondxz", "cegijmky"
NAMED_N     = * - named
PLAIN_N     = 11
named_n:    .byte       2, 3, 4, 5, 6, 7, 8, 9, 10, 16, 36, 3, 5, 7, 3, 5, 13, 27, 53

            HEADER      "nformat", 0
nw_format:                                                  ; ( c-addr u -- ) ( N: x1 ... xn -- ): the format
            lda         dlo,x                               ;   string typed, each placeholder ({}, {x} ...) a
            sta         tmp                                 ;   number, the deepest first
            sta         n_t
            cmp         #<NBUF_SIZE
            lda         dhi,x
            sta         tmp + 1
            sta         n_t + 1
            sbc         #>NBUF_SIZE
            bcc         :+
            lda         #<-24
            jmp         throw_a
:
            lda         dlo + 1,x
            sta         w2
            lda         dhi + 1,x
            sta         w2 + 1
            inx
            inx
            LDR         w, nbuf
            jsr         nst_copy
            clc                                             ; (A 0 after it)
            lda         #<nbuf
            adc         n_t
            sta         w
            lda         #>nbuf
            adc         n_t + 1
            sta         w + 1
            lda         #0
            sta         (w)
            jsr         f_count
            jsr         nst_sync
            jsr         nst_sel
            lda         n_cnt
            jsr         nst_need
            lda         n_cnt                               ; The arguments' table at the stack's end: 3 bytes
            sta         tmp                                 ;   each (0, the number's address), then $FF
            asl
            sta         n_t
            lda         #0
            rol
            sta         tmp + 1
            sec
            lda         tmp
            adc         n_t
            sta         tmp
            bcc         :+
            inc         tmp + 1
:
            jsr         nst_fits
            MOVR        w, nst_top
            lda         n_cnt
            sta         d_idx
@arg:
            lda         d_idx
            beq         @args
            dec         d_idx
            lda         d_idx                               ; (Entry d_idx from the top: the deepest first)
            jsr         nst_addr
            phy
            ldy         #1
            sta         (w),y
            pla
            iny
            sta         (w),y
            lda         #0
            sta         (w)
            clc
            lda         w
            adc         #3
            sta         w
            bcc         @arg
            inc         w + 1
            bra         @arg
@args:
            lda         #$FF
            sta         (w)
            inc         w
            bne         :+
            inc         w + 1
:
            LDR         r0, nbuf
            MOVR        r4, nst_top
            MOVR        r2, w
            sec
            lda         #<NS_END
            sbc         w
            sta         r3
            lda         #>NS_END
            sbc         w + 1
            sta         r3 + 1
            NCALL       NUM_FORMAT
            MOVR        t_at, r2
            MOVR        t_len, nlen
@drop:
            lda         n_cnt
            beq         :+
            dec         n_cnt
            jsr         nst_drop
            bra         @drop
:
            jmp         nst_type

; The placeholders in nbuf's format string (a 0 after it), as the library's FORMAT has them: n_cnt
f_count:
            stz         n_cnt
            LDR         w, nbuf
@char:
            lda         (w)
            beq         @done
            jsr         @inc
            cmp         #'{'
            beq         @open
            cmp         #'}'
            bne         @char
            lda         (w)                                 ; (}}: a brace)
            cmp         #'}'
            bne         @char
            jsr         @inc
            bra         @char
@open:
            lda         (w)                                 ; ({{: a brace)
            cmp         #'{'
            bne         :+
            jsr         @inc
            bra         @char
:
            MOVR        w3, w                               ; (A } after it: a placeholder; else { itself)
@find:
            lda         (w3)
            beq         @char
            inc         w3
            bne         :+
            inc         w3 + 1
:
            cmp         #'}'
            bne         @find
            inc         n_cnt
            MOVR        w, w3
            bra         @char
@done:
            rts
@inc:
            inc         w
            bne         :+
            inc         w + 1
:
            rts

; ---- The math functions

            HEADER      "nsqrt", 0
nw_sqrt:
            LDR         r15, MATH_SQRT
            jmp         mt_un

            HEADER      "nexp", 0
nw_exp:
            LDR         r15, MATH_EXP
            jmp         mt_un

            HEADER      "nlog", 0
nw_log:
            LDR         r15, MATH_LOG
            jmp         mt_un

            HEADER      "nsin", 0
nw_sin:
            lda         #0
            bra         :+

            HEADER      "ncos", 0
nw_cos:
            lda         #1
            bra         :+

            HEADER      "ntan", 0
nw_tan:
            lda         #2
:
            sta         nin_y
            LDR         r15, MATH_TRIG
            jmp         mt_un

            HEADER      "natan", 0
nw_atan:
            LDR         r15, MATH_ATAN
            jmp         mt_un

            HEADER      "npi", 0
nw_pi:                                                      ; ( N: -- pi )
            LDR         r15, MATH_PI
            jmp         mt_new

            HEADER      "digits", 0
nw_digits:                                                  ; ( -- a-addr ): the precision (1 to 100: 12 at the
            PUSHI       dg_var                              ;   start) of the math functions' results
            rts

; ---- Literals, constants, variables

; A number literal's code: the number after the jsr (its length (2), its bytes) pushed; nlit_p, a definition's in a
; code bank: the number's address after the jsr (in the dictionary: comp_num's)
nlit_p:
            jsr         inline_ptr
            jmp         push_w
nlit:
            stx         xsave
            tsx
            lda         $0101,x                             ; (The definition's return address: the jsr's last
            sta         w                                   ;   byte; + 1 the number)
            lda         $0102,x
            sta         w + 1
            inc         w
            bne         :+
            inc         w + 1
:
            ldy         #1                                  ; Past it: + its length + 2
            lda         (w),y
            sta         tmp + 1
            lda         (w)
            sec
            adc         w
            sta         $0101,x
            lda         tmp + 1
            adc         w + 1
            sta         $0102,x
            ldx         xsave
            jmp         push_w

; The number in nbuf (nlen long) compiled, as a literal (nlit; in a code bank, nlit_p and its bytes in the
; dictionary, as comp_str has a string)
comp_num:
            bit         cmode
            bpl         @inline
            lda         #<nlit_p
            ldy         #>nlit_p
            jsr         comp_jsr
            lda         dhere
            ldy         dhere + 1
            jsr         comma_ay
            jsr         here_swap
            jsr         comp_bytes
            jmp         here_swap
@inline:
            lda         #JSR_OP
            jsr         ccomma_a
            lda         #<nlit
            ldy         #>nlit
            jsr         comma_ay
; nbuf's number (nlen long) into the dictionary: its length (2), its bytes
comp_bytes:
            lda         nlen
            ldy         nlen + 1
            jsr         comma_ay
            LDR         w3, nbuf
            MOVR        tmp3, nlen
@byte:
            lda         tmp3
            ora         tmp3 + 1
            beq         @done
            lda         (w3)
            jsr         ccomma_a
            inc         w3
            bne         :+
            inc         w3 + 1
:
            lda         tmp3
            bne         :+
            dec         tmp3 + 1
:
            dec         tmp3
            bra         @byte
@done:
            rts

            HEADERC     "nliteral", F_IMMEDIATE
nw_literal:                                                 ; ( N: x -- ): compiled, a literal
            jsr         top_buf
            jmp         comp_num

            HEADER      "nconstant", 0
nw_constant:                                                ; ( "name" -- ) ( N: x -- ): name pushes it
            jsr         top_buf
            lda         #<ncon_run
            ldy         #>ncon_run
            jsr         make_word
            jmp         comp_bytes

; An NCONSTANT's code: its number after the jsr pushed
ncon_run:
            pla
            sta         w
            pla
            sta         w + 1
            inc         w
            bne         :+
            inc         w + 1
:
            jmp         push_w

            HEADER      "nvariable", 0
nw_variable:                                                ; ( "name" -- ): a cell, 0 (the number 0) or its
            jmp         variable                            ;   number's block (in memory.fl's heap)

            HEADER      "nvalue", 0
nw_value:                                                   ; ( "name" -- ) ( N: x -- ): name pushes it, TO name
            lda         #<nval_run                          ;   sets it (n! of its cell)
            ldy         #>nval_run
            jsr         make_word
            lda         here
            ldy         here + 1
            PUSHAY
            lda         #0
            tay
            jsr         comma_ay
            bra         nw_store

; An NVALUE's code: the number of its cell, after the jsr, pushed (as n@)
nval_run:
            pla
            clc
            adc         #1
            sta         tmp
            pla
            adc         #0
            tay
            lda         tmp
            PUSHAY
            bra         nw_fetch

            HEADER      "n@", 0
nw_fetch:                                                   ; ( a-addr -- ) ( N: -- x )
            lda         dlo,x
            sta         w
            lda         dhi,x
            sta         w + 1
            inx
            ldy         #1
            lda         (w),y
            tay
            lda         (w)
            sta         w
            sty         w + 1
            ora         w + 1
            beq         :+
            jmp         push_w
:
            jmp         push_zero

            HEADER      "n!", 0
nw_store:                                                   ; ( a-addr -- ) ( N: x -- ): its block resized (or
            jsr         nst_sel                              ;   allocated) to the number, and it copied there
            lda         #1
            jsr         nst_need
            lda         #0
            jsr         nst_ent
            jsr         nst_back
            MOVR        n_t, tmp
            lda         dlo,x
            sta         v_at
            sta         w
            lda         dhi,x
            sta         v_at + 1
            sta         w + 1
            inx
            ldy         #1
            lda         (w),y
            sta         v_old + 1
            lda         (w)
            sta         v_old
            clc                                             ; (Its length and 2)
            lda         n_t
            adc         #2
            sta         tmp
            lda         n_t + 1
            adc         #0
            sta         tmp + 1
            lda         v_old
            ora         v_old + 1
            beq         @new
            lda         v_old
            ldy         v_old + 1
            PUSHAY
            lda         tmp
            ldy         tmp + 1
            PUSHAY
            jsr         @resize
            bra         @got
@new:
            lda         tmp
            ldy         tmp + 1
            PUSHAY
            jsr         @alloc
@got:
            lda         dlo,x                               ; ( a-addr' ior ): its ior THROWn
            ora         dhi,x
            beq         :+
            jmp         throw
:
            inx
            MOVR        w, v_at                             ; The variable's cell its block
            lda         dlo,x
            sta         (w)
            sta         w2
            ldy         #1
            lda         dhi,x
            sta         (w),y
            sta         w2 + 1
            inx
            lda         n_t                                 ; The block: the length, the bytes
            sta         (w2)
            lda         n_t + 1
            sta         (w2),y
            clc
            lda         w2
            adc         #2
            sta         w
            lda         w2 + 1
            adc         #0
            sta         w + 1
            jsr         nst_sel
            lda         #0
            jsr         nst_ent
            jsr         nst_copy
            jsr         nst_drop
            jmp         nst_back
@alloc:
            jmp         (xt_alloc)
@resize:
            jmp         (xt_resize)

; ---- The core's (num_vec: num_call's): .A the call

nw_hook:
            cmp         #1
            bcs         :+
            jmp         nh_lit
:
            bne         :+
            jmp         nh_read
:
            jmp         nh_text

; 0: the word throw_name names, a number in the base (a program's text: a bare one starts with a digit), pushed or
; compiled: C = 0; not one, C = 1.  A double Forth read past 32 bits (num_ovf: 4294967296.), its integer
nh_lit:
            MOVR        w2, throw_name
            LDR         w, nbuf
            lda         throw_nlen
            sta         tmp
            sta         n_t
            stz         tmp + 1
            jsr         nst_copy
            lda         num_ovf                             ; (A double past 32 bits: its integer)
            beq         :+
            ldy         n_t
            lda         nbuf - 1,y
            cmp         #'.'
            bne         :+
            dec         n_t
:
            jsr         nst_sync
            jsr         nst_sel
            lda         state
            ora         state + 1
            bne         :+
            jsr         nst_room
:
            LDR         r0, nbuf
            lda         n_t
            sta         r1
            stz         r1 + 1
            stz         r4
            stz         r4 + 1
            lda         #3                                  ; (A program's, the whole text)
            sta         nin_y
            jsr         nst_r23
            LDR         r15, NUM_PARSE
            jsr         nm_try
            bcc         @num
            cmp         #NE_NOTNUM
            beq         :+
            jmp         nm_err
:
            jsr         nst_back
            sec
            rts
@num:
            lda         state
            ora         state + 1
            bne         @comp
            jsr         nst_push
            jsr         nst_back
            clc
            rts
@comp:
            MOVR        w2, nst_top
            LDR         w, nbuf
            MOVR        tmp, nlen
            jsr         nst_copy
            jsr         nst_back
            jsr         comp_num
            clc
            rts

; 1: ( c-addr u -- n 1 | d 2 | 0 ): the interpreter's number, read by the library (its base not a radix): an integer
; of the text (less a . at its end: a double); with a Forth prefix ($, %, # and a digit), Forth's (C = 1)
nh_read:
            jsr         nst_sync
            lda         num_custom
            bne         @lb206
            jmp         @forth
@lb206:
            lda         dlo + 1,x
            sta         w2
            lda         dhi + 1,x
            sta         w2 + 1
            lda         dhi,x
            beq         @lb205
            jmp         @not
@lb205:
            lda         dlo,x
            bne         @lb204
            jmp         @not
@lb204:
            sta         tmp
            sta         n_t
            stz         tmp + 1
            ldy         #0
            lda         (w2)
            cmp         #'-'
            bne         :+
            iny
            lda         (w2),y
:
            cmp         #'$'
            bne         @lb203
            jmp         @forth
@lb203:
            cmp         #'%'
            bne         @lb202
            jmp         @forth
@lb202:
            cmp         #'#'
            bne         @lib
            iny
            lda         (w2),y
            cmp         #'-'
            bne         @lb201
            jmp         @forth
@lb201:
            cmp         #'0'
            bcc         @lib
            cmp         #'9' + 1
            bcs         @lb200
            jmp         @forth
@lb200:
@lib:
            LDR         w, nbuf
            jsr         nst_copy
            stz         n_flag
            ldy         n_t                                 ; (A . at its end: a double)
            lda         nbuf - 1,y
            cmp         #'.'
            bne         :+
            dec         n_t
            dec         n_flag
:
            jsr         nst_sel
            LDR         r0, nbuf
            lda         n_t
            sta         r1
            stz         r1 + 1
            stz         r4
            stz         r4 + 1
            lda         #3
            sta         nin_y
            jsr         nst_r23
            LDR         r15, NUM_PARSE
            jsr         nm_try
            bcs         @notb
            MOVR        r0, nst_top
            LDR         r15, NUM_KIND
            jsr         nm_try
            bcs         @notb
            lda         nlen
            cmp         #NK_INT
            bne         @notb
            LDR         r15, NUM_TO_INT
            jsr         nm_try
            bcs         @notb
            jsr         nst_back
            bit         n_flag
            bmi         @dbl
            lda         nlen                                ; (A cell: -32768 to 65535)
            bne         @not
            lda         r5
            ora         r5 + 1
            beq         :+
            lda         r5
            and         r5 + 1
            cmp         #$FF
            bne         @not
            lda         r4 + 1
            bpl         @not
:
            lda         r4
            sta         dlo + 1,x
            lda         r4 + 1
            sta         dhi + 1,x
            lda         #1
            sta         dlo,x
            stz         dhi,x
            clc
            rts
@dbl:
            lda         nlen
            cmp         #2
            bcs         @not
            lda         r4
            sta         dlo + 1,x
            lda         r4 + 1
            sta         dhi + 1,x
            lda         r5
            sta         dlo,x
            lda         r5 + 1
            sta         dhi,x
            PUSHI       2
            clc
            rts
@notb:
            jsr         nst_back
@not:
            inx
            stz         dlo,x
            stz         dhi,x
            clc
            rts
@forth:
            sec
            rts

; 2, 3, 4: ( n -- c-addr u ), ( u -- c-addr u ), ( d -- c-addr u ): the cells as text, by the library (its base not
; a radix); C = 0.  Its base a radix: C = 1
nh_text:
            sta         n_cnt
            jsr         nst_sync
            lda         num_custom
            bne         :+
            sec
            rts
:
            lda         n_cnt
            cmp         #4
            beq         @d
            lda         dlo,x
            sta         r0
            lda         dhi,x
            sta         r0 + 1
            stz         r1
            stz         r1 + 1
            ldy         #0
            lda         n_cnt
            cmp         #3
            beq         :+
            iny
            lda         dhi,x
            bpl         :+
            lda         #$FF
            sta         r1
            sta         r1 + 1
:
            inx
            bra         @go
@d:
            lda         dlo,x
            sta         r1
            lda         dhi,x
            sta         r1 + 1
            lda         dlo + 1,x
            sta         r0
            lda         dhi + 1,x
            sta         r0 + 1
            inx
            inx
            ldy         #1
@go:
            sty         nin_y
            jsr         nst_sel
            jsr         nst_r23
            NCALL       NUM_FROM_INT
            MOVR        r0, nst_top                          ; Its text after it
            clc
            lda         r2
            adc         nlen
            sta         r2
            lda         r2 + 1
            adc         nlen + 1
            sta         r2 + 1
            sec
            lda         #<NS_END
            sbc         r2
            sta         r3
            lda         #>NS_END
            sbc         r2 + 1
            sta         r3 + 1
            stz         r4
            stz         r4 + 1
            NCALL       NUM_DISPLAY
            MOVR        w2, r2
            LDR         w, nbuf
            MOVR        tmp, nlen
            jsr         nst_copy
            jsr         nst_back
            PUSHI       nbuf
            lda         nlen
            ldy         nlen + 1
            PUSHAY
            clc
            rts
