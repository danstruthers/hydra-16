; ****************************************************************************
; numlib.inc - hylang's numbers on the numbers library (modules/numbers, docs/design/plans/NUMBERS.md's step 3), in
; its third bank.  A number is a fixnum (bit 0 set: -16384 to 16383, worked where it's met, as ever), or a cell of
; PK_SNUM (its length and the number's bytes in the stored format, in its one form: 7 or fewer) or of PK_NUMBER (its
; length and its blob, the bytes: 8 or more).  So a value's form is its number's: an integer in a fixnum's range is
; always a fixnum, a number of 7 bytes or fewer always a PK_SNUM.
;   The library works the rest: each operand's bytes staged in the scratch (the reader's, free while a number is
; worked, as hylang's own registers were: NL_A, NL_B), the result's there too (NL_R), made a value after (nl_value);
; a base's string apart, in the RAM (NL_S, NL_T: the scratch's end is sys's, hysys.inc's, as a call is made).
; A number's text goes to a RAM bank of hylang's own (nl_tbank: 8K, room for a complex number of rationals in
; binary), and out from there.  The library's own bank (r13: its registers, its arena, the base) is another of
; hylang's.  Its errors: NE_BIG "Too big: an integer past 255 bytes"; NE_DIV0 "Division by zero."; the rest each
; caller's own.

NL_ROOM         = NUM_MAX                                   ; (A number staged: the most its bytes are)
NL_A            = scratch                                   ; (The operands, the result ...
NL_B            = scratch + NL_ROOM
NL_R            = scratch + 2 * NL_ROOM
NL_BASE_MAX     = 96                                        ; (A base's string: the most its bytes are)
NL_TEXT         = $8000                                     ; (A number's text: in nl_tbank)
NL_TEXT_ROOM    = $2000
NK_NOT          = $FF                                       ; (n_kind's: not a number)
NLEVELS         = 6                                         ; (Abort points set at once, at most)

.macro NLCALL entry                                         ; The library's entry called (nl_call: .A, .X, .Y pass)
            LDR         r15, entry
            jsr         nl_call
.endmacro

.bss
NL_S:       .res        NL_BASE_MAX                         ; A base's string (DISPLAY's, SET_BASE's ...)
NL_T:       .res        NL_BASE_MAX                         ; (Another: base's, the one before)
nl_regs:    .res        32                                  ; (The call registers r0-r15 kept: nl_rsave's)
nl_mod:     .res        1                                   ; The library's module's bank (the paged ROM's)
nl_math:    .res        1                                   ; The math library's (0: there's none)
nl_bank:    .res        1                                   ; Its RAM bank (r13)
nl_tbank:   .res        1                                   ; hylang's bank for a number's text
nl_dec:     .res        1                                   ; 1: the base decimal, its prefix not written (fixnums
                                                            ;   read and written by hylang itself, as ever)
nl_flags:   .res        1                                   ; (SET_BASE's: bit 0 decimal, bit 1 its prefix written)
nl_len:     .res        2                                   ; (A number's bytes: how many)
nl_v:       .res        2                                   ; (A value worked)
nl_w:       .res        2                                   ; (Another: nl_two's second)
nl_i:       .res        2                                   ; (A count)
n_abt:      .res        1                                   ; <> 0: num_parse's number was too big (the reader's)
n_lvl:      .res        1                                   ; Abort points set: each one's 65C02 stack and root stack
n_spa:      .res        NLEVELS                             ;   and its captures (open, their length: a capture begun
n_rsa:      .res        NLEVELS                             ;   after it is given up)
n_cpa:      .res        NLEVELS
n_cla:      .res        NLEVELS
n_cha:      .res        NLEVELS
n_res:      .res        1                                   ; (num_parse's answer: NP_*)

NP_NUM          = 0             ; num_parse's answers: a number (hv) ...
NP_NOT          = 1             ;   not a number (a symbol, as the reader has it) ...
NP_DIVZ         = 2             ;   a rational below 0 (1/0)

.segment "CODE3"

; ---- The library

; The library found (the module directory's "numbers"), its bank and the text's taken, INIT; the base decimal.  OUT:
; C = 1 if it isn't there, or there's no bank for it
nl_init:
            stz         nl_i
@find:
            LDR         r0, NL_A                            ; (Each entry: in the scratch)
            lda         nl_i
            jsr         MODINFO
            bcs         @rts
            ldx         #0
:
            lda         NL_A + ME_NAME,x
            cmp         s_numbers,x
            bne         @next
            inx
            cmp         #0
            bne         :-
            lda         NL_A + ME_TYPE
            cmp         #HT_LIBRARY
            bne         @next
            lda         NL_A + ME_BANK
            sta         nl_mod
            lda         #1
            jsr         BANKS_ALLOC
            bcs         @rts
            sta         nl_bank
            lda         #1
            jsr         BANKS_ALLOC
            bcs         @rts
            sta         nl_tbank
            NLCALL      NUM_INIT
            lda         #1
            sta         nl_dec
            sta         nl_flags
            stz         nl_math                             ; (The math library: its bank, or 0)
            stz         nl_i
@math:
            LDR         r0, NL_A
            lda         nl_i
            jsr         MODINFO
            bcs         @done
            ldx         #0
:
            lda         NL_A + ME_NAME,x
            cmp         s_math,x
            bne         @mnext
            inx
            cmp         #0
            bne         :-
            lda         NL_A + ME_TYPE
            cmp         #HT_LIBRARY
            bne         @mnext
            lda         NL_A + ME_BANK
            sta         nl_math
            bra         @done
@mnext:
            inc         nl_i
            bra         @math
@done:
            clc
@rts:
            rts
@next:
            inc         nl_i
            jmp         @find

; The library's entry r15 called (XCALL): r13 its bank, r14 its module's; .A, .X, .Y and r0-r12 pass both ways, C
; and .A its answer
nl_call:
            pha
            lda         nl_bank
            sta         r13
            lda         nl_mod
            sta         r14
            pla
            jmp         XCALL

; The math library's entry r15 called (XCALL): r13 the libraries' bank, r14 its module's
nl_mcall:
            pha
            lda         nl_bank
            sta         r13
            lda         nl_math
            sta         r14
            pla
            jmp         XCALL

; An error for the library's .A (C = 1): NE_BIG, too big; NE_DIV0, a division by zero; any other, "Not a number"
; (ex; C = 1 if there's no room for it)
nl_error:
            cmp         #NE_DIV0
            bne         :+
            LDAX        s_divzero
            jmp         n_error
:
            cmp         #NE_BIG
            bne         :+
            LDAX        s_toobig
            jmp         n_error
:
            LDAX        s_nlnot
            jmp         n_error

; ---- Abort points (hylang's): a built-in that may run out of room deep inside (an error's message, a capture) sets
; one as it starts (jsr n_enter, first thing), ends through n_leave; n_nomem goes back to it, C = 1


; An abort point set: the entry that calls it goes back to its caller from n_nomem, the root stack and the captures
; as they were.  Keeps .Y
n_enter:
            tsx                                             ; (The entry's stack: past this call's return)
            inx
            inx
            txa
            ldx         n_lvl
            sta         n_spa,x
            lda         rsp
            sta         n_rsa,x
            lda         cap_on
            sta         n_cpa,x
            lda         cap_len
            sta         n_cla,x
            lda         cap_len + 1
            sta         n_cha,x
            inc         n_lvl
            rts

; The entry's end: its abort point dropped.  Keeps .A, .X, .Y, C
n_leave:
            php
            dec         n_lvl
            plp
            rts

; No room: back to the abort point, C = 1
n_nomem:
            pla                                             ; (Its return, kept)
            sta         nl_w
            pla
            sta         nl_w + 1
            dec         n_lvl
            ldx         n_lvl
            lda         n_rsa,x
            sta         rsp
            lda         n_cpa,x
            sta         cap_on
            lda         n_cla,x
            sta         cap_len
            lda         n_cha,x
            sta         cap_len + 1
            lda         n_spa,x
            tax
            txs
            sec
            rts

; The call registers r0-r15 kept (a system call's, as it's set: numbi.inc's n_to_u32 ...).  Keeps .A, .X, .Y
nl_rsave:
            pha
            phx
            ldx         #31
:
            lda         r0,x
            sta         nl_regs,x
            dex
            bpl         :-
            plx
            pla
            rts

; And as they were.  Keeps .A, .X, .Y, the flags
nl_rrest:
            php
            pha
            phx
            ldx         #31
:
            lda         nl_regs,x
            sta         r0,x
            dex
            bpl         :-
            plx
            pla
            plp
            rts

; ---- Values and their bytes

; The number .A, .X (a fixnum, or a PK_NUMBER cell) as its bytes in the stored format at nd (the scratch).  OUT: C =
; 0 and .A, .X = their count (nl_len); C = 1 if it isn't a number.  Keeps nd
nl_put:
            sta         nl_v
            stx         nl_v + 1
            lsr
            bcc         @cell
            lda         nl_v                                ; (A fixnum: its bytes made)
            ldx         nl_v + 1
            jsr         unfix
            jsr         nl_fixbytes
            clc
            rts
@cell:
            lda         nl_v
            ldx         nl_v + 1
            jsr         kind_of
            jsr         nl_bytes                            ; (Its bytes: hp, nl_len of them)
            bcs         @no
            ldy         #0
            ldx         nl_len + 1
            beq         @last
@page:
            lda         (hp),y
            sta         (nd),y
            iny
            bne         @page
            inc         hp + 1
            inc         nd + 1
            dex
            bne         @page
@last:
            cpy         nl_len
            beq         :+
            lda         (hp),y
            sta         (nd),y
            iny
            bra         @last
:
            lda         nd + 1                              ; (nd back to where it was)
            sec
            sbc         nl_len + 1
            sta         nd + 1
            lda         nl_len
            ldx         nl_len + 1
            clc
            rts
@no:
            sec
            rts

; nl_v's number's bytes, a cell of kind .A (kind_of's: PK_SNUM, its own; PK_NUMBER, its blob's): hp = them (their
; bank in the window), nl_len = how many.  OUT: C = 1 if it's neither
nl_bytes:
            cmp         #PK_SNUM
            beq         @short
            cmp         #PK_NUMBER
            bne         @no
            jsr         nl_blob
            clc
            rts
@short:
            lda         nl_v
            ldx         nl_v + 1
            jsr         deref
            lda         (hp)
            sta         nl_len
            stz         nl_len + 1
            inc         hp                                  ; (A cell of 8 bytes: its bytes in its page)
            clc
            rts
@no:
            sec
            rts

; nl_v's (a PK_NUMBER cell's) blob: hp = its bytes (its bank in the window), nl_len = how many
nl_blob:
            lda         nl_v
            ldx         nl_v + 1
            jsr         deref
            lda         (hp)
            sta         nl_len
            ldy         #1
            lda         (hp),y
            sta         nl_len + 1
            iny
            lda         (hp),y
            pha
            iny
            lda         (hp),y
            tax
            pla
            jmp         blob_data

; The integer .A, .X (-16384 to 16383) as its bytes at nd.  OUT: .A, .X = their count (1-3; nl_len)
nl_fixbytes:
            stz         nl_len + 1
            cpx         #0                                  ; (-64 to 63: the tag alone, 7 bits)
            bne         :+
            cmp         #64
            bcs         @big
            bra         @small
:
            cpx         #$FF
            bne         @big
            cmp         #$C0
            bcc         @big
@small:
            and         #$7F
            sta         (nd)
            lda         #1
            bra         @len
@big:
            ldy         #NT_POS                             ; (Its magnitude, a byte or two, and its sign's tag)
            cpx         #$80
            bcc         :+
            ldy         #NT_NEG
            eor         #$FF                                ; (-n: its magnitude)
            clc
            adc         #1
            pha
            txa
            eor         #$FF
            adc         #0
            tax
            pla
:
            sty         nl_i
            phy
            ldy         #1
            sta         (nd),y
            ply
            cpx         #0
            beq         :+
            txa
            ldy         #2
            sta         (nd),y
            lda         nl_i
            ora         #1                                  ; (Two bytes: the tag's count 1 more)
            sta         (nd)
            lda         #3
            bra         @len
:
            lda         nl_i
            sta         (nd)
            lda         #2
@len:
            sta         nl_len
            ldx         #0
            rts

; hv = the number whose bytes are at nd (.A, .X of them, in its one form): a fixnum if it's an integer in a fixnum's
; range, else a cell of PK_SNUM (7 bytes or fewer: the bytes in it) or of PK_NUMBER (its blob, the bytes).  OUT: C = 1
; if there's no room.  It may collect (nd's bytes, the scratch's, are kept)
nl_value:
            sta         nl_len
            stx         nl_len + 1
            lda         (nd)
            cmp         #NT_POS
            bcs         @big
            ldx         #0                                  ; (-64 to 63: the tag, 7 bits)
            cmp         #$40
            bcc         :+
            ora         #$80
            ldx         #$FF
:
            jsr         fix_make
            clc
            rts
@big:
            and         #$EE                                ; (One byte or two, $80 $81 $90 $91: a fixnum maybe)
            cmp         #NT_POS
            bne         @cell
            ldy         #1
            lda         (nd),y
            sta         nl_v
            stz         nl_v + 1
            lda         (nd)
            and         #1
            beq         :+
            iny
            lda         (nd),y
            bmi         @cell
            sta         nl_v + 1
:
            lda         (nd)
            and         #NT_NEG - NT_POS
            beq         :+
            sec                                             ; (Negative: -its magnitude)
            lda         #0
            sbc         nl_v
            sta         nl_v
            lda         #0
            sbc         nl_v + 1
            sta         nl_v + 1
:
            lda         nl_v
            ldx         nl_v + 1
            jsr         fix_make
            bcs         @cell
            rts
@cell:
            lda         nl_len + 1                          ; (7 bytes or fewer: in the cell)
            bne         @blob
            lda         nl_len
            cmp         #8
            bcs         @blob
            lda         #PK_SNUM
            jsr         cell_alloc
            bcs         @rts
            lda         hv
            ldx         hv + 1
            jsr         deref
            lda         nl_len
            sta         (hp)
            inc         hp
            MOVW        hq, nd
            MOVW        hn, nl_len
            jsr         copy_in
            clc
            rts
@blob:
            lda         #PK_NUMBER
            jsr         cell_alloc
            bcs         @rts
            MOVW        hn, nl_len
            jsr         blob_alloc
            bcs         @rts
            lda         hv
            ldx         hv + 1
            jsr         deref
            lda         nl_len
            sta         (hp)
            ldy         #1
            lda         nl_len + 1
            sta         (hp),y
            iny
            lda         hb
            sta         (hp),y
            iny
            lda         hb + 1
            sta         (hp),y
            lda         hb
            ldx         hb + 1
            jsr         blob_data
            MOVW        hq, nd
            MOVW        hn, nl_len
            jsr         copy_in
            clc
@rts:
            rts

; ex = nl_value's (the bytes at nd: .A, .X of them).  OUT: C = 1 if there's no room
nl_ex:
            jsr         nl_value
            bcs         :+
            MOVW        ex, hv
:
            rts

; Two numbers staged: .A, .X at NL_A, nl_w at NL_B (r0, r1; r2 = NL_A, r3 = NL_ROOM: the result in the first's
; place).  OUT: C = 1 if either isn't a number
nl_two:
            pha
            LDR         nd, NL_A
            pla
            jsr         nl_put
            bcs         :+
            LDR         nd, NL_B
            lda         nl_w
            ldx         nl_w + 1
            jsr         nl_put
            bcs         :+
            LDR         r0, NL_A
            LDR         r1, NL_B
            LDR         r2, NL_A
            LDR         r3, NL_ROOM
            clc
:
            rts

; One number staged: .A, .X at NL_A (r0; r2 = NL_R, r3 = NL_ROOM).  OUT: C = 1 if it isn't a number
nl_one:
            pha
            LDR         nd, NL_A
            pla
            jsr         nl_put
            bcs         :+
            LDR         r0, NL_A
            LDR         r2, NL_R
            LDR         r3, NL_ROOM
            clc
:
            rts

; ex = the library's result at NL_R (.A, .X its length; C = 1, .A an error: nl_error's).  OUT: C = 1 if there's no
; room
nl_result:
            bcc         :+
            jmp         nl_error
:
            pha
            LDR         nd, NL_R
            pla
            jmp         nl_ex

; ex = the result at NL_A (as nl_result)
nl_result_a:
            bcc         :+
            jmp         nl_error
:
            pha
            LDR         nd, NL_A
            pla
            jmp         nl_ex

; ---- Kinds

; IN: .A, .X = a value.  OUT: .A = its number kind (NK_*: NK_NOT if it isn't a number), Z = 1 if it's an integer.
; Keeps .Y
n_kind:
            sta         nl_v
            stx         nl_v + 1
            lsr
            bcs         @int
            lda         nl_v
            ldx         nl_v + 1
            jsr         kind_of
            phy
            jsr         nl_bytes
            ply
            bcs         @not
            lda         (hp)
            cmp         #NT_FIXED
            bcc         @int
            cmp         #NT_RATIONAL
            bcc         @fix
            beq         @rat
            lda         #NK_COMPLEX
            rts
@fix:
            lda         #NK_FIXED
            rts
@rat:
            lda         #NK_RATIONAL
            rts
@int:
            lda         #NK_INT
            rts
@not:
            lda         #NK_NOT
            rts

; ---- The base

; nl_dec and nl_flags from SET_BASE's .A (its base: decimal? its prefix written?)
nl_flags_set:
            sta         nl_flags
            stz         nl_dec
            cmp         #1                                  ; (Decimal, its prefix not written)
            bne         :+
            inc         nl_dec
:
            rts

; NL_S = the string .A, .X (a string, or a character), zero-terminated, if it's NL_BASE_MAX - 1 bytes at most.  OUT:
; C = 1 if it isn't a string, or it's longer (one longer is no base: ".", none)
nl_spec:
            jsr         n_text_of
            bcs         @rts
            lda         hn + 1
            bne         @long
            ldy         #0
:
            cpy         hn
            beq         :+
            cpy         #NL_BASE_MAX - 1
            bcs         @long
            lda         (hq),y
            sta         NL_S,y
            iny
            bra         :-
:
            lda         #0
            sta         NL_S,y
            clc
@rts:
            rts
@long:
            lda         #'.'                                ; (No base: ".")
            sta         NL_S
            stz         NL_S + 1
            clc
            rts

; ---- Text

; The number .A, .X (a fixnum, or a PK_NUMBER cell) out as text: in the base (nl_spc 0), or the base at NL_S (nl_spc
; 1), DISPLAY's, through nl_tbank.  OUT: C = 1, ex an error, if the base names none or it's too big to write
nl_display:
            pha
            LDR         nd, NL_A
            pla
            jsr         nl_put
            jsr         nl_disp_a
            bcs         :+
            jmp         nl_emit
:
            rts

; The number whose bytes are at NL_A as text in nl_tbank (nl_i bytes of it, from NL_TEXT): in the base (nl_spc 0), or
; the base at NL_S (nl_spc 1).  Past 8K, what fits; too big in its base, in decimal with #d.  OUT: C = 1, ex an error
; ("Invalid base specifier ...", too big), if it can't be written
nl_disp_a:
            LDR         r0, NL_A
            LDR         r4, 0
            lda         nl_spc
            beq         :+
            LDR         r4, NL_S
:
            LDR         r2, NL_TEXT
            LDR         r3, NL_TEXT_ROOM
            lda         $00                                 ; (The text to nl_tbank)
            pha
            lda         nl_tbank
            sta         $00
            NLCALL      NUM_DISPLAY
            bcc         @made
            cmp         #NE_ROOM                            ; (No room, 8K: what there was room for)
            bne         :+
            LDR         nl_i, NL_TEXT_ROOM
            bra         @done
:
            cmp         #NE_BIG                             ; (Too big in its base: in decimal, #d, the prefix's)
            bne         @error
            lda         #'#'
            sta         NL_S
            lda         #'d'
            sta         NL_S + 1
            stz         NL_S + 2
            LDR         r4, NL_S
            NLCALL      NUM_DISPLAY
            bcs         @error
@made:
            sta         nl_i
            stx         nl_i + 1
@done:
            pla
            sta         $00
            clc
            rts
@error:
            tax
            pla
            sta         $00
            txa
            cmp         #NE_BASE
            beq         :+
            jsr         nl_error
            sec
            rts
:
            jsr         nu_begin                            ; ("Invalid base specifier #=x")
            LDAX        s_badbase
            jsr         out_text
            LDAX        NL_S
            jsr         out_text
            jsr         nu_end
            sec
            rts

; nl_i bytes of text out, from nl_tbank's NL_TEXT (out_byte keeps the window)
nl_emit:
            lda         $00
            pha
            lda         nl_tbank
            sta         $00
            LDR         nd, NL_TEXT
@byte:
            lda         nl_i
            ora         nl_i + 1
            beq         @done
            lda         (nd)
            jsr         out_byte
            inc         nd
            bne         :+
            inc         nd + 1
:
            lda         nl_i
            bne         :+
            dec         nl_i + 1
:
            dec         nl_i
            bra         @byte
@done:
            pla
            sta         $00
            clc
            rts

; The number .A, .X out, as print and repr show it (print.inc's): in the base; save's form (pm 2), with the base's
; prefix if it isn't decimal (as danlang's save writes it, so it reads back in any base)
num_print:
            sta         nl_v
            stx         nl_v + 1
            stz         nl_spc
            lda         pm
            cmp         #2
            bne         @out
            lda         nl_flags                            ; (save's: decimal, as print has it, without its prefix:
            lsr                                             ;   d; else # and the base's string)
            bcc         @prefix
            LDAX        s_decimal
            jsr         nl_spec_ax
            bra         @spec
@prefix:
            lda         #'#'
            sta         NL_S
            LDR         r2, NL_S + 1
            LDR         r3, NL_BASE_MAX - 1
            NLCALL      NUM_GET_BASE
            lda         NL_S + 1
            cmp         #'#'
            bne         @spec
            ldx         #0                                  ; (It has its #: as it is)
:
            lda         NL_S + 1,x
            sta         NL_S,x
            beq         @spec
            inx
            bra         :-
@spec:
            inc         nl_spc
@out:
            lda         nl_v
            ldx         nl_v + 1
            jsr         nl_display
            rts

; NL_S = the zero-terminated text at .A, .X (this bank's)
nl_spec_ax:
            sta         nd
            stx         nd + 1
            ldy         #0
:
            lda         (nd),y
            sta         NL_S,y
            beq         :+
            iny
            bra         :-
:
            rts

; The word at hq (hn bytes: the reader's) read as a number, in the base, as a program's text (a bare one starts with
; a digit 0-9): n_res = NP_NUM and hv the number; NP_NOT, it isn't one; NP_DIVZ, a rational of 0 below (1/0); n_abt
; <> 0, too big.  Decimal integers in a fixnum's range read here (the base decimal, its prefix not written).  OUT: C =
; 1 if there's no room
num_parse:
            stz         n_abt
            lda         #NP_NOT
            sta         n_res
            lda         nl_dec
            beq         @lib
            jsr         nl_fixword
            bcs         @lib
            stz         n_res
            clc
            rts
@lib:
            MOVR        r0, hq
            MOVR        r1, hn
            LDR         r4, 0
            LDR         r2, NL_R
            LDR         r3, NL_ROOM
            LDR         r15, NUM_PARSE
            ldy         #3                                  ; (A program's text, the whole word)
            jsr         nl_call
            bcc         @num
            cmp         #NE_DIV0
            bne         :+
            lda         #NP_DIVZ
            sta         n_res
            clc
            rts
:
            cmp         #NE_BIG
            bne         :+
            inc         n_abt
:
            clc
            rts
@num:
            pha
            LDR         nd, NL_R
            pla
            jsr         nl_value
            bcs         :+
            stz         n_res
:
            rts

; The word at hq (hn bytes) a decimal integer in a fixnum's range ([+-]?[0-9]+)?  OUT: C = 0 and hv it; C = 1 if not
nl_fixword:
            lda         hn + 1
            beq         @lb201
            jmp         @no
@lb201:
            ldy         #0
            stz         nl_v                                ; (Its magnitude, its sign)
            stz         nl_v + 1
            stz         nl_i
            lda         hn
            bne         @lb200
            jmp         @no
@lb200:
            lda         (hq)
            cmp         #'-'
            bne         :+
            dec         nl_i
            iny
            bra         @digit
:
            cmp         #'+'
            bne         @digit
            iny
@digit:
            cpy         hn
            beq         @no
@next:
            lda         (hq),y
            sec
            sbc         #'0'
            cmp         #10
            bcs         @no
            pha                                             ; (n = n * 10 + it: past 16384, not here)
            lda         nl_v + 1
            cmp         #>1639
            bcc         :+
            bne         @pull
            lda         nl_v
            cmp         #<1639
            bcs         @pull
:
            lda         nl_v                                ; (* 10: * 8 + * 2)
            asl
            sta         nl_len
            lda         nl_v + 1
            rol
            sta         nl_len + 1
            asl         nl_v
            rol         nl_v + 1
            asl         nl_v
            rol         nl_v + 1
            asl         nl_v
            rol         nl_v + 1
            clc
            lda         nl_v
            adc         nl_len
            sta         nl_v
            lda         nl_v + 1
            adc         nl_len + 1
            sta         nl_v + 1
            pla
            clc
            adc         nl_v
            sta         nl_v
            bcc         :+
            inc         nl_v + 1
:
            iny
            cpy         hn
            bne         @next
            lda         nl_i                                ; (Its sign)
            beq         :+
            sec
            lda         #0
            sbc         nl_v
            sta         nl_v
            lda         #0
            sbc         nl_v + 1
            sta         nl_v + 1
:
            lda         nl_v
            ldx         nl_v + 1
            jmp         fix_make
@pull:
            pla
@no:
            sec
            rts

.bss
nl_spc:     .res        1                                   ; (nl_display's: 1, the base at NL_S)

.segment "RODATA3"
s_math:     .byte       "math", 0
s_numbers:  .byte       "numbers", 0
s_decimal:  .byte       "d", 0
s_toobig:   .byte       "Too big: an integer past 255 bytes", 0
s_nlnot:    .byte       "Not a number", 0

.segment "CODE3"
