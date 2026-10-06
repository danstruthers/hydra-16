; ****************************************************************************
; numbits.inc - hylang's bits and bytes (phase 5), in its third bank, as danlang's (SystemBuiltins.cs): bit-and,
; bit-or, bit-xor, bit-not, shl, shr, bit? on integers of any size, in two's complement; hex and bin; lo, hi, word;
; bytes and from-bytes; and the number tests (int?, fixed?, rational?, complex?).  An argument that must be an
; integer (or a number equal to one) and isn't is the error "'shl''s count must be an integer", its code :inval.

.bss
n_bop:      .res        1                                   ; (bit-and's 0, bit-or's 1, bit-xor's 2)
n_iw:       .res        2                                   ; (An integer argument's error: its words; named?)
n_inam:     .res        1
n_k:        .res        2                                   ; (A shift's count, and what its error calls it)
n_cw:       .res        2
n_ia:       .res        1                                   ; (An integer argument's place)
n_w:        .res        2                                   ; (hex's width)

.segment "CODE3"

; ---- Integer arguments

; The argument at .Y (its place: 0, 2 ...) an integer (or a number equal to one: a fixed decimal of no places).  IN:
; .A, .X = the words of its error, after the built-in's name (nb_intn: "'shl''s count") or as they are (nb_intt: "A
; width").  OUT: C = 0, .A, .X = it; C = 1 and ex = the error "... must be an integer" (:inval)
nb_intn:
            sty         n_ia
            ldy         #$FF
            bra         nb_int0
nb_intt:
            sty         n_ia
            ldy         #0
nb_int0:
            sty         n_inam
            sta         n_iw
            stx         n_iw + 1
            ldy         n_ia
            lda         (argp),y
            sta         n_v
            iny
            lda         (argp),y
            sta         n_v + 1
            lda         n_v
            ldx         n_v + 1
            jsr         n_kind
            beq         @int
            cmp         #NK_FIX
            bne         @not
            lda         n_v
            ldx         n_v + 1
            ldy         #1
            jsr         cell_get
            cmp         #<FIX(0)
            bne         @not
            cpx         #0
            bne         @not
            lda         n_v
            ldx         n_v + 1
            ldy         #0
            jsr         cell_get
            clc
            rts
@int:
            lda         n_v
            ldx         n_v + 1
            clc
            rts
@not:
            jsr         nu_begin
            lda         n_inam
            beq         @words
            lda         #$27                                ; ('name''s ...)
            jsr         out_byte
            ldx         bi_n
            FARN        2, bi_name
            LDAX        s_ints
            jsr         out_text
@words:
            lda         n_iw
            ldx         n_iw + 1
            jsr         out_text
            LDAX        s_mustint
            jsr         out_text
; ex = the error the capture begun at nu_start says, its code :inval.  OUT: C = 1 (no room: n_nomem)
nb_inval:
            jsr         nu_end
            bcs         @nomem
            LDHQ        s_inval, 5
            lda         #PK_ATOM
            jsr         intern
            bcs         @nomem
            lda         ex
            ldx         ex + 1
            jsr         deref
            ldy         #2
            lda         hv
            sta         (hp),y
            iny
            lda         hv + 1
            sta         (hp),y
            sec                                             ; (C = 1: ex the error)
            rts
@nomem:
            jmp         n_nomem

; ex, an error made, the entry's value (C = 0)
n_fail:
            clc
            jmp         n_leave

; ---- Bits

; (bit-and n...), (bit-or n...), (bit-xor n...): the integers' bits, in two's complement
bi_bit_and:
            lda         #0
            bra         nb_bits
bi_bit_or:
            lda         #1
            bra         nb_bits
bi_bit_xor:
            lda         #2
nb_bits:
            sta         n_bop
            jsr         n_enter
            lda         #0
            sta         nrc
            ldy         #0
            jsr         nb_bitarg
            bcs         n_fail
            stz         nm_i
@next:
            inc         nm_i
            lda         nm_i
            cmp         cn
            bcs         @made
            lda         #2
            sta         nrc
            lda         nm_i
            asl
            tay
            jsr         nb_bitarg
            bcs         n_fail
            ldx         #0                                  ; (Both in two's complement, a byte past the longer)
            lda         r_len,x
            ldx         #2
            cmp         r_len,x
            bcs         :+
            lda         r_len,x
:
            inc
            bne         :+
            jmp         n_big
:
            sta         n_m
            lda         #0
            sta         nrc
            jsr         r_tc
            lda         #2
            sta         nrc
            jsr         r_tc
            lda         #0
            sta         nra
            lda         #2
            sta         nrb
            jsr         r_ab
            ldy         #0
@byte:
            lda         (na),y
            ldx         n_bop
            beq         @and
            dex
            beq         @or
            eor         (nb),y
            bra         @set
@and:
            and         (nb),y
            bra         @set
@or:
            ora         (nb),y
@set:
            sta         (na),y
            iny
            cpy         n_m
            bne         @byte
            lda         #0
            sta         nrc
            jsr         r_untc
            bra         @next
@made:
            lda         #0
            sta         nrc
            jsr         r_store
            MOVW        ex, hv
            clc
@leave:
            jmp         n_leave

; R(nrc) = the argument at .Y ("'bit-and''s argument must be an integer")
nb_bitarg:
            LDAX        s_bargument
            jsr         nb_intn
            bcs         @rts
            jsr         r_load
            clc
@rts:
            rts

; (bit-not n): -n - 1
bi_bit_not:
            jsr         n_enter
            lda         #0
            sta         nrc
            ldy         #0
            jsr         nb_bitarg
            bcc         @lb230
            jmp         n_fail
@lb230:
            ldx         #0
            jsr         r_negate
            jsr         nb_less1
            jsr         r_store
            MOVW        ex, hv
            clc
@leave:
            jmp         n_leave

; R0 = R0 - 1 (R2: 1)
nb_less1:
            lda         #2
            sta         nrc
            lda         #1
            ldx         #0
            jsr         r_set16
            lda         #0
            sta         nra
            sta         nrc
            lda         #2
            sta         nrb
            jmp         r_sub

; (shl n k): n times 2 to the k (k below 0: shr's).  (shr n k): n over 2 to the k, down (toward minus infinity)
bi_shl:
            lda         #0
            bra         nb_shift
bi_shr:
            lda         #$FF
nb_shift:
            sta         n_bop
            ldy         #<s_bcount
            sty         n_cw
            ldy         #>s_bcount
            sty         n_cw + 1
            jsr         n_enter
            jsr         nb_shargs
            bcc         @lb229
            jmp         n_fail
@lb229:
            jsr         nb_shift0
            jsr         r_store
            MOVW        ex, hv
            clc
@leave:
            jmp         n_leave

; (bit? n k): bit k of n (two's complement): T if it's 1
bi_bit_p:
            lda         #$FF
            sta         n_bop
            lda         #<s_bbit
            sta         n_cw
            lda         #>s_bbit
            sta         n_cw + 1
            jsr         n_enter
            jsr         nb_shargs
            bcc         @lb228
            jmp         n_fail
@lb228:
            jsr         nb_shift0
            ldx         #0                                  ; (n >> k: its bit 0, its magnitude's)
            lda         r_len,x
            beq         @nil
            lda         #0
            jsr         r_page
            sta         na + 1
            stz         na
            lda         (na)
            lsr
            bcc         @nil
            lda         #<T_VAL
            bra         @made
@nil:
            lda         #0
@made:
            sta         ex
            stz         ex + 1
            clc
@leave:
            jmp         n_leave

; R0 = the argument 0 (the number), n_k = the argument 1 (the count: 16 bits, past them -32768 or 32767).  OUT: C = 1
; and ex the error if either isn't an integer
nb_shargs:
            lda         #0
            sta         nrc
            LDAX        s_bnumber
            ldy         #0
            jsr         nb_intn
            bcs         @rts
            jsr         r_load
            lda         n_cw
            ldx         n_cw + 1
            ldy         #2
            jsr         nb_intn
            bcs         @rts
            jsr         n_whole
            sta         n_k
            stx         n_k + 1
            clc
@rts:
            rts

; R0 shifted by n_k: left (n_bop 0) or right (n_bop $FF; a negative count the other way)
nb_shift0:
            lda         n_k + 1
            bpl         @way
            sec                                             ; (A negative count: the other way)
            lda         #0
            sbc         n_k
            sta         n_k
            lda         #0
            sbc         n_k + 1
            sta         n_k + 1
            lda         n_bop
            eor         #$FF
            sta         n_bop
@way:
            lda         #0
            sta         nrc
            lda         n_bop
            beq         r_shl
            ldx         #0                                  ; (Right, and negative: -(((|n| - 1) >> k) + 1))
            lda         r_neg,x
            bne         @lb227
            jmp         r_shr
@lb227:
            jsr         r_negate
            jsr         nb_less1
            jsr         r_shr
            lda         #0
            sta         nra
            sta         nrc
            lda         #2
            sta         nrb
            jsr         r_add
            ldx         #0
            jmp         r_negate

; |nrc| shifted left n_k bits.  Too big: n_big
r_shl:
            jsr         r_c
            ldx         nrc
            lda         r_len,x
            bne         @lb226
            jmp         @rts
@lb226:
            lda         n_k                                 ; (Whole bytes: n_k / 8)
            sta         n_t
            lda         n_k + 1
            lsr
            ror         n_t
            lsr
            ror         n_t
            lsr
            ror         n_t
            cmp         #0
            bne         @big
            lda         r_len,x
            clc
            adc         n_t
            bcs         @big
            cmp         #255
            bcs         @big
            sta         n_lc                                ; (Its bytes up by n_t)
            ldy         r_len,x
@up:
            dey
            cpy         #$FF
            beq         @zeros
            lda         (nc),y
            pha
            tya
            clc
            adc         n_t
            tay
            pla
            sta         (nc),y
            tya
            sec
            sbc         n_t
            tay
            bra         @up
@zeros:
            ldy         n_t
            beq         :+
            lda         #0
@zero:
            dey
            sta         (nc),y
            bne         @zero
:
            ldy         n_lc                                ; (A byte more, 0, for the bits)
            lda         #0
            sta         (nc),y
            iny
            tya
            sta         r_len,x
            lda         n_k
            and         #7
            beq         @norm
            sta         n_bits
@bit:
            ldy         #0
            ldx         nrc
            lda         r_len,x
            tax
            clc
@shift:
            lda         (nc),y
            rol
            sta         (nc),y
            iny
            dex
            bne         @shift
            dec         n_bits
            bne         @bit
@norm:
            jmp         r_normc
@big:
            jmp         n_big
@rts:
            rts

; |nrc| shifted right n_k bits
r_shr:
            jsr         r_c
            ldx         nrc
            lda         n_k + 1                             ; (Whole bytes: n_k / 8; all of them, 0)
            cmp         #8
            bcs         @zero
            lda         n_k
            sta         n_t
            lda         n_k + 1
            lsr
            ror         n_t
            lsr
            ror         n_t
            lsr
            ror         n_t
            lda         n_t
            cmp         r_len,x
            bcs         @zero
            ldy         #0                                  ; (Its bytes down by n_t)
@down:
            tya
            clc
            adc         n_t
            cmp         r_len,x
            bcs         @len
            phy
            tay
            lda         (nc),y
            ply
            sta         (nc),y
            iny
            bra         @down
@len:
            tya
            sta         r_len,x
            lda         n_k
            and         #7
            beq         @norm
            sta         n_bits
@bit:
            ldx         nrc
            ldy         r_len,x
            clc
@shift:
            dey
            lda         (nc),y
            ror
            sta         (nc),y
            tya
            bne         @shift
            dec         n_bits
            bne         @bit
@norm:
            jmp         r_normc
@zero:
            jmp         r_clear

; nrc in two's complement in n_m bytes (its bytes to n_m 0, then if it's negative each inverted and 1 added)
r_tc:
            ldx         nrc
            jsr         r_zext
            jsr         r_c
            ldx         nrc
            lda         r_neg,x
            bne         r_inv1
            rts
; Its n_m bytes inverted and 1 added
r_inv1:
            ldy         #0
            ldx         n_m
            sec
@byte:
            lda         (nc),y
            eor         #$FF
            adc         #0
            sta         (nc),y
            iny
            dex
            bne         @byte
@rts:
            rts

; nrc from two's complement in n_m bytes: negative if its top bit is set
r_untc:
            jsr         r_c
            ldx         nrc
            lda         n_m
            sta         r_len,x
            stz         r_neg,x
            tay
            dey
            lda         (nc),y
            bpl         @norm
            jsr         r_inv1
            ldx         nrc
            lda         #$FF
            sta         r_neg,x
@norm:
            jmp         r_normc

; (hex n [width]), (bin n [width]): n's digits in base 16, or 2 (upper case), at least width of them (0s first); a -
; before a negative one's
bi_hex:
            lda         #16
            bra         nb_digits
bi_bin:
            lda         #2
nb_digits:
            sta         n_radix
            jsr         n_enter
            lda         #0                                  ; (n: R0)
            sta         nrc
            LDAX        s_bnumber
            ldy         #0
            jsr         nb_intn
            bcc         @lb225
            jmp         n_fail
@lb225:
            jsr         r_load
            lda         #1                                  ; (Its width: 1)
            sta         n_w
            stz         n_w + 1
            lda         cn
            cmp         #2
            bcc         :+
            LDAX        s_bwidth
            ldy         #2
            jsr         nb_intt
            bcc         @lb224
            jmp         n_fail
@lb224:
            jsr         n_whole
            sta         n_w
            stx         n_w + 1
:
            jsr         cap_begin
            sta         nu_start
            stx         nu_start + 1
            ldx         #0
            lda         r_neg,x
            beq         :+
            stz         r_neg,x
            lda         #'-'
            jsr         out_byte
:
            lda         #NR_DIG                             ; (Its digits, least first: n_np of them)
            jsr         r_page
            sta         nd + 1
            stz         nd
            stz         n_np
            stz         n_np + 1
@digit:
            lda         n_radix
            jsr         r_divsmall
            tay
            lda         n_std,y
            sta         (nd)
            inc         nd
            bne         :+
            inc         nd + 1
:
            inc         n_np
            bne         :+
            inc         n_np + 1
            lda         n_np + 1
            cmp         #NR_DIGS
            bcc         :+
            jmp         n_big
:
            ldx         #0
            lda         r_len,x
            bne         @digit
            sec                                             ; (0s: width less the digits, if that's above 0)
            lda         n_w
            sbc         n_np
            sta         n_w
            lda         n_w + 1
            sbc         n_np + 1
            sta         n_w + 1
@pad:
            lda         n_w + 1
            bmi         @out
            ora         n_w
            beq         @out
            lda         #'0'
            jsr         out_byte
            lda         n_w
            bne         :+
            dec         n_w + 1
:
            dec         n_w
            bra         @pad
@out:
            jsr         np_prev                             ; (Most first)
            jsr         out_byte
            lda         n_np
            bne         :+
            dec         n_np + 1
:
            dec         n_np
            lda         n_np
            ora         n_np + 1
            bne         @out
            lda         nu_start
            ldx         nu_start + 1
            jsr         cap_end
            bcc         :+
            jmp         n_nomem
:
            MOVW        ex, hv
            clc
            jmp         n_leave

; (lo n): n's low byte (n bit-and 255).  (hi n): its next ((n shr 8) bit-and 255)
bi_lobyte:
            lda         #0
            bra         nb_lohi
bi_hibyte:
            lda         #8
nb_lohi:
            sta         n_k
            stz         n_k + 1
            jsr         n_enter
            lda         #0
            sta         nrc
            LDAX        s_bnumber
            ldy         #0
            jsr         nb_intn
            bcc         @lb223
            jmp         n_fail
@lb223:
            jsr         r_load
            lda         #$FF
            sta         n_bop
            jsr         nb_shift0
            jsr         nb_lo
            ldx         #0
            jsr         fix_ax
            sta         ex
            stx         ex + 1
            clc
            jmp         n_leave

; .A = R0's low byte, in two's complement
nb_lo:
            ldx         #0
            lda         r_len,x
            beq         @rts
            lda         #0
            jsr         r_page
            sta         na + 1
            stz         na
            lda         (na)
            ldy         r_neg,x
            beq         @rts
            eor         #$FF
            inc
@rts:
            rts

; (word lo hi): lo's low byte and hi's, a word
bi_word:
            jsr         n_enter
            lda         #0
            sta         nrc
            LDAX        s_lowbyte
            ldy         #0
            jsr         nb_intt
            bcc         @lb222
            jmp         n_fail
@lb222:
            jsr         r_load
            jsr         nb_lo
            sta         n_w
            lda         #0
            sta         nrc
            LDAX        s_highbyte
            ldy         #2
            jsr         nb_intt
            bcc         @lb221
            jmp         n_fail
@lb221:
            jsr         r_load
            jsr         nb_lo
            sta         n_w + 1
            lda         #0
            sta         nrc
            jsr         r_c
            ldx         #0
            lda         #2
            sta         r_len,x
            stz         r_neg,x
            lda         n_w
            sta         (nc)
            ldy         #1
            lda         n_w + 1
            sta         (nc),y
            jsr         r_store
            MOVW        ex, hv
            clc
            jmp         n_leave

; ---- Bytes

; (bytes s): a string's (or a character's) bytes, a list
bi_bytes:
            jsr         n_arg0
            lda         ex
            ldx         ex + 1
            jsr         n_text_of
            bcc         :+
            LDAX        s_bytesstr
            jmp         n_error
:
            MOVW        n_np, hn                            ; (Its bytes from the last: each consed on)
            stz         cf
            stz         cf + 1
@byte:
            lda         n_np
            ora         n_np + 1
            beq         @made
            lda         n_np
            bne         :+
            dec         n_np + 1
:
            dec         n_np
            jsr         n_arg0                              ; (Its bytes again: a collection may have moved them)
            lda         ex
            ldx         ex + 1
            jsr         n_text_of
            clc
            lda         hq
            adc         n_np
            sta         hq
            lda         hq + 1
            adc         n_np + 1
            sta         hq + 1
            lda         (hq)
            ldx         #0
            jsr         fix_ax
            sta         et
            stx         et + 1
            MOVW        eu, cf
            lda         #PK_QCONS
            jsr         make2
            bcs         @rts
            MOVW        cf, hv
            bra         @byte
@made:
            MOVW        ex, cf
            clc
@rts:
            rts

; (from-bytes l): a string of a list of bytes (each an integer, 0-255)
bi_from_bytes:
            jsr         n_enter
            jsr         n_arg0
            lda         ex
            ldx         ex + 1
            jsr         q_conses
            bcc         :+
            jsr         nu_begin                            ; ("'from-bytes' expects a list of bytes", :inval)
            LDAX        s_frombytes
            jsr         out_text
            jsr         nb_inval
            jmp         n_fail
:
            sta         n_np
            stx         n_np + 1
            jsr         cap_begin
            sta         nu_start
            stx         nu_start + 1
@item:
            lda         n_np                                ; (n_np: its conses left)
            ora         n_np + 1
            beq         @made
            lda         n_np
            ldx         n_np + 1
            ldy         #0
            jsr         cell_get
            sta         n_x
            stx         n_x + 1
            jsr         n_whole                             ; (An integer: as nb_intt has it)
            bcs         @notint
            cpx         #0
            bne         @range
            jsr         out_byte
            lda         n_np
            ldx         n_np + 1
            ldy         #1
            jsr         cell_get
            sta         n_np
            stx         n_np + 1
            bra         @item
@made:
            lda         nu_start
            ldx         nu_start + 1
            jsr         cap_end
            bcc         :+
            jmp         n_nomem
:
            MOVW        ex, hv
            clc
@leave:
            jmp         n_leave
@notint:
            lda         nu_start
            ldx         nu_start + 1
            jsr         cap_drop
            jsr         nu_begin
            LDAX        s_abyte
            jsr         out_text
            LDAX        s_mustint
            jsr         out_text
            jsr         nb_inval
            jmp         n_fail
@range:
            lda         nu_start                            ; ("256 isn't a byte")
            ldx         nu_start + 1
            jsr         cap_drop
            jsr         nu_begin
            lda         n_x
            ldx         n_x + 1
            ldy         #0
            FARN        2, print_val
            LDAX        s_notbyte
            jsr         out_text
            jsr         nb_inval
            jmp         n_fail

; ---- The number tests

; (int? x): an integer?  (fixed? x), (rational? x), (complex? x): a fixed decimal?  a rational?  a complex number?
bi_is_int:
            lda         #NK_INT
            bra         nb_is
bi_is_fixed:
            lda         #NK_FIX
            bra         nb_is
bi_is_rational:
            lda         #NK_RAT
            bra         nb_is
bi_is_complex:
            lda         #NK_CPX
nb_is:
            sta         n_c
            jsr         n_arg0
            lda         ex
            ldx         ex + 1
            jsr         n_kind
            cmp         n_c
            beq         :+
            jmp         nil_out
:
            jmp         t_out

.segment "RODATA3"
s_ints:     .byte       "''s ", 0
s_bargument: .byte       "argument", 0
s_bnumber:   .byte       "number", 0
s_bcount:    .byte       "count", 0
s_bbit:      .byte       "bit", 0
s_bwidth:    .byte       "A width", 0
s_lowbyte:  .byte       "A low byte", 0
s_highbyte: .byte       "A high byte", 0
s_abyte:    .byte       "a byte", 0
s_mustint:  .byte       " must be an integer", 0
s_inval:    .byte       "inval"
s_bytesstr: .byte       "'bytes' expects a String", 0
s_frombytes: .byte      "'from-bytes' expects a list of bytes", 0
s_notbyte:  .byte       " isn't a byte", 0

.segment "CODE3"
