; ****************************************************************************
; numbits.inc - hylang's bits and bytes, in its third bank, as danlang's (SystemBuiltins.cs): bit-and, bit-or,
; bit-xor, bit-not, shl, shr, bit? on integers of any size, in two's complement (fixnums worked here, the rest by the
; numbers library's BITS); hex and bin; lo, hi, word; bytes and from-bytes; and the number tests (int?, fixed?,
; rational?, complex?).  An argument that must be an integer (or a number equal to one) and isn't is the error
; "'shl''s count must be an integer", its code :inval.

.bss
n_bop:      .res        1                                   ; (BITS's operation: 0 and, 1 or, 2 xor ...)
n_iw:       .res        2                                   ; (An integer argument's error: its words; named?)
n_inam:     .res        1
n_ia:       .res        1                                   ; (An integer argument's place)
n_w:        .res        2                                   ; (hex's width; word's bytes)
n_neg:      .res        1                                   ; (hex's: its number below 0)
n_cw:       .res        2                                   ; (A shift's count's words)
n_bl:       .res        2                                   ; (from-bytes': the list's conses left)

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
            cmp         #NK_FIXED
            bne         @not
            lda         n_v                                 ; (A fixed decimal of no places)
            ldx         n_v + 1
            jsr         nl_tag
            cmp         #NT_FIXED
            bne         @not
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

; (bit-and n...), (bit-or n...), (bit-xor n...): the integers' bits, in two's complement: fixnums' here, the rest's by
; BITS (the result so far at NL_A, the next at NL_B)
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
            stz         nm_i                                ; (Each an integer)
@check:
            lda         nm_i
            cmp         cn
            bcs         @first
            asl
            tay
            LDAX        s_bargument
            jsr         nb_intn
            bcc         :+
            jmp         n_fail
:
            inc         nm_i
            bra         @check
@first:
            jsr         n_arg0                              ; (Fixnums while they last: ex)
            stz         nm_i
@fix:
            inc         nm_i
            lda         nm_i
            cmp         cn
            bcs         @fixed
            asl
            tay
            lda         ex                                  ; (Both fixnums: here)
            and         (argp),y
            lsr
            bcc         @lib
            lda         n_bop
            beq         @and
            cmp         #1
            beq         @or
            lda         ex
            eor         (argp),y
            ora         #1
            sta         ex
            iny
            lda         ex + 1
            eor         (argp),y
            bra         @set
@and:
            lda         ex
            and         (argp),y
            sta         ex
            iny
            lda         ex + 1
            and         (argp),y
            bra         @set
@or:
            lda         ex
            ora         (argp),y
            sta         ex
            iny
            lda         ex + 1
            ora         (argp),y
@set:
            sta         ex + 1
            bra         @fix
@fixed:
            lda         ex                                  ; (One, not a fixnum: the integer it is, 2.0's 2)
            lsr
            bcs         :+
            lda         ex
            ldx         ex + 1
            jsr         n_intval
            bcc         :++
            jmp         n_nomem
:
            clc
            jmp         n_leave
:
            MOVW        ex, hv
            clc
            jmp         n_leave
@lib:
            LDR         nd, NL_A                            ; (The rest by BITS: ex so far)
            lda         ex
            ldx         ex + 1
            jsr         nl_put
@next:
            lda         nm_i
            cmp         cn
            bcs         @made
            asl
            tay
            LDR         nd, NL_B
            lda         (argp),y
            pha
            iny
            lda         (argp),y
            tax
            pla
            jsr         nl_put
            LDR         r0, NL_A
            LDR         r1, NL_B
            LDR         r2, NL_A
            LDR         r3, NL_ROOM
            LDR         r15, NUM_BITS
            ldy         n_bop
            jsr         nl_call
            bcs         @error
            sta         nl_len
            stx         nl_len + 1
            inc         nm_i
            bra         @next
@made:
            lda         nl_len
            ldx         nl_len + 1
            clc
            jsr         nl_result_a
            jmp         n_leave
@error:
            jsr         nl_error
            jmp         n_fail

; (bit-not n): -n - 1
bi_bit_not:
            jsr         n_enter
            LDAX        s_bargument
            ldy         #0
            jsr         nb_intn
            bcc         :+
            jmp         n_fail
:
            lsr                                             ; (A fixnum: its bits the other way, its own 1 kept)
            bcc         @lib
            lda         n_v
            eor         #$FE
            sta         ex
            lda         n_v + 1
            eor         #$FF
            sta         ex + 1
            clc
            jmp         n_leave
@lib:
            lda         n_v
            ldx         n_v + 1
            jsr         nl_one
            LDR         r15, NUM_BITS
            ldy         #3
            jsr         nl_call
            jsr         nl_result
            jmp         n_leave

; (shl n k): n times 2 to the k (k below 0: shr's).  (shr n k): n over 2 to the k, down (toward minus infinity).
; (bit? n k): bit k of n (two's complement): T if it's 1.  BITS's, k 16 bits (past them -32768 or 32767)
bi_shl:
            lda         #4
            bra         nb_shift
bi_shr:
            lda         #5
nb_shift:
            sta         n_bop
            jsr         n_enter
            LDAX        s_bcount
            jsr         nb_shargs
            bcc         :+
            jmp         n_fail
:
            jsr         nb_shcall
            jsr         nl_result
            jmp         n_leave
bi_bit_p:
            lda         #6
            sta         n_bop
            jsr         n_enter
            LDAX        s_bbit
            jsr         nb_shargs
            bcc         :+
            jmp         n_fail
:
            jsr         nb_shcall
            bcs         @error
            cmp         #0
            bne         :+
            jsr         nil_out
            jmp         n_leave
:
            jsr         t_out
            jmp         n_leave
@error:
            jsr         nl_error
            jmp         n_fail

; The arguments n and k (k's words in its error: .A, .X) integers?  OUT: C = 1 and ex the error if not
nb_shargs:
            sta         n_cw
            stx         n_cw + 1
            LDAX        s_bnumber
            ldy         #0
            jsr         nb_intn
            bcs         @rts
            lda         n_cw
            ldx         n_cw + 1
            ldy         #2
            jmp         nb_intn
@rts:
            rts

; BITS n_bop on the arguments n (NL_A) and k (NL_B): C, .A, .X its answer, the result at NL_R
nb_shcall:
            jsr         n_arg1
            MOVW        nl_w, ex
            jsr         n_arg0
            lda         ex
            ldx         ex + 1
            jsr         nl_two
            LDR         r2, NL_R
            LDR         r15, NUM_BITS
            ldy         n_bop
            jmp         nl_call

; (hex n [width]), (bin n [width]): n's digits in base 16, or 2 (upper case), at least width of them (0s first); a -
; before a negative one's
bi_hex:
            LDAX        s_hexbase
            bra         nb_digits
bi_bin:
            LDAX        s_binbase
nb_digits:
            jsr         nl_spec_ax                          ; (Its base, NL_S: "x" or "b")
            jsr         n_enter
            LDAX        s_bnumber
            ldy         #0
            jsr         nb_intn
            bcc         :+
            jmp         n_fail
:
            lda         #1                                  ; (Its width: 1)
            sta         n_w
            stz         n_w + 1
            lda         cn
            cmp         #2
            bcc         @width
            LDAX        s_bwidth
            ldy         #2
            jsr         nb_intt
            bcc         :+
            jmp         n_fail
:
            jsr         n_whole
            sta         n_w
            stx         n_w + 1
@width:
            jsr         n_arg0                              ; (|n| at NL_A, its sign apart)
            lda         ex
            ldx         ex + 1
            jsr         n_sign3
            stz         n_neg
            cmp         #0
            bne         :+
            dec         n_neg
:
            lda         ex
            ldx         ex + 1
            jsr         nl_one
            lda         n_neg
            beq         :+
            LDR         r2, NL_A
            NLCALL      NUM_NEG
:
            lda         #1                                  ; (Its digits: in nl_tbank, nl_i of them)
            sta         nl_spc
            jsr         nl_disp_a
            bcc         :+
            jmp         n_fail
:
            jsr         cap_begin
            sta         nu_start
            stx         nu_start + 1
            lda         n_neg
            beq         :+
            lda         #'-'
            jsr         out_byte
:
            sec                                             ; (0s: width less the digits, if that's above 0)
            lda         n_w
            sbc         nl_i
            sta         n_w
            lda         n_w + 1
            sbc         nl_i + 1
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
            jsr         nl_emit
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
            lda         #1
nb_lohi:
            sta         n_bop
            jsr         n_enter
            LDAX        s_bnumber
            ldy         #0
            jsr         nb_intn
            bcc         :+
            jmp         n_fail
:
            jsr         nb_low16
            ldy         n_bop
            lda         n_w,y
            ldx         #0
            jsr         fix_ax
            sta         ex
            stx         ex + 1
            clc
            jmp         n_leave

; n_w = the integer .A, .X's low 16 bits (two's complement): a fixnum's, or TO_INT's
nb_low16:
            sta         n_v
            stx         n_v + 1
            lsr
            bcc         @lib
            lda         n_v
            ldx         n_v + 1
            jsr         unfix
            sta         n_w
            stx         n_w + 1
            rts
@lib:
            lda         n_v
            ldx         n_v + 1
            jsr         nl_one
            NLCALL      NUM_TO_INT
            MOVR        n_w, r4
            rts

; (word lo hi): lo's low byte and hi's, a word
bi_word:
            jsr         n_enter
            LDAX        s_lowbyte
            ldy         #0
            jsr         nb_intt
            bcc         :+
            jmp         n_fail
:
            jsr         nb_low16
            lda         n_w
            pha
            LDAX        s_highbyte
            ldy         #2
            jsr         nb_intt
            bcc         :+
            pla
            jmp         n_fail
:
            jsr         nb_low16
            lda         n_w
            sta         n_w + 1
            pla
            sta         n_w
            MOVR        n_w32, n_w                          ; (0 to 65535: n_u32's)
            stz         n_w32 + 2
            stz         n_w32 + 3
            jsr         n_u32
            bcc         :+
            jmp         n_nomem
:
            MOVW        ex, hv
            clc
            jmp         n_leave

; ---- Bytes

; (bytes s): a string's (or a character's) bytes, a list
bi_bytes:
            jsr         n_arg0
            lda         ex
            ldx         ex + 1
            jsr         n_bytes_of
            bcc         :+
            LDAX        s_bytesstr
            jmp         n_error
:
            MOVW        n_t, hn                             ; (Its bytes from the last: each consed on)
            stz         cf
            stz         cf + 1
@byte:
            lda         n_t
            ora         n_t + 1
            beq         @made
            lda         n_t
            bne         :+
            dec         n_t + 1
:
            dec         n_t
            jsr         n_arg0                              ; (Its bytes again: a collection may have moved them)
            lda         ex
            ldx         ex + 1
            jsr         n_bytes_of
            clc
            lda         hq
            adc         n_t
            sta         hq
            lda         hq + 1
            adc         n_t + 1
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
            jsr         kind_of
            cmp         #PK_BUFFER
            bne         @list
            jsr         cap_begin                           ; (A buffer: its bytes, a string)
            sta         nu_start
            stx         nu_start + 1
            lda         ex
            ldx         ex + 1
            jsr         n_bytes_of
            MOVW        hp, hq
:
            lda         hn
            ora         hn + 1
            beq         @made
            lda         (hp)
            jsr         out_byte
            inc         hp
            bne         :+
            inc         hp + 1
:
            lda         hn
            bne         :+
            dec         hn + 1
:
            dec         hn
            bra         :---
@list:
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
            sta         n_bl
            stx         n_bl + 1
            jsr         cap_begin
            sta         nu_start
            stx         nu_start + 1
@item:
            lda         n_bl                                ; (n_bl: its conses left)
            ora         n_bl + 1
            beq         @made
            lda         n_bl
            ldx         n_bl + 1
            ldy         #0
            jsr         cell_get
            sta         n_x
            stx         n_x + 1
            jsr         n_whole                             ; (An integer: as nb_intt has it)
            bcs         @notint
            cpx         #0
            bne         @range
            jsr         out_byte
            lda         n_bl
            ldx         n_bl + 1
            ldy         #1
            jsr         cell_get
            sta         n_bl
            stx         n_bl + 1
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
            lda         #NK_FIXED
            bra         nb_is
bi_is_rational:
            lda         #NK_RATIONAL
            bra         nb_is
bi_is_complex:
            lda         #NK_COMPLEX
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
s_hexbase:  .byte       "x", 0
s_binbase:  .byte       "b", 0

.segment "CODE3"
