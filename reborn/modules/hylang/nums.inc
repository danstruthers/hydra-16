; ****************************************************************************
; nums.inc - hylang's numbers (phase 3: fixnums; phase 5 has the rest), with fn, fexpr, fun, error, the type tests
; and not, and "Not yet", in its third bank.  Each built-in gets its values at argp (cn of them) and gives back ex (C = 1 if there's no room).

.segment "CODE3"

; ---- Helpers (this bank's own)

; ex = the argument 0 (1)
n_arg0:
            ldy         #0
            bra         n_arg
n_arg1:
            ldy         #2
n_arg:
            lda         (argp),y
            sta         ex
            iny
            lda         (argp),y
            sta         ex + 1
            rts

; An error made of the zero-terminated text at .A, .X (in this bank): ex.  OUT: C = 1 if there's no room
n_error:
            sta         hq
            stx         hq + 1
            ldy         #0
:
            lda         (hq),y
            beq         :+
            iny
            bne         :-
:
            sty         hn
            stz         hn + 1
            jsr         error_make
            bcs         :+
            MOVW        ex, hv
:
            rts

; An error's message begun (capture), and ended: ex = the error.  OUT: C = 1 if there's no room
nu_begin:
            jsr         cap_begin
            sta         nu_start
            stx         nu_start + 1
            rts
nu_end:
            lda         nu_start
            ldx         nu_start + 1
            jsr         cap_end
            bcs         @rts
            lda         hv
            ldx         hv + 1
            jsr         kind_of
            cmp         #PK_ERROR
            beq         @done
            jsr         error_wrap
            bcs         @rts
@done:
            MOVW        ex, hv
            clc
@rts:
            rts

; OUT: C = 1 if ex is an error
n_is_err:
            lda         ex
            lsr
            bcs         @no
            ldx         ex + 1
            cpx         #IMM_PAGES
            bcc         @no
            lda         pk,x
            cmp         #PK_ERROR
            beq         @yes
@no:
            clc
            rts
@yes:
            sec
            rts

; OUT: C = 1 if ex is false: NIL or ()
n_is_nil:
            lda         ex + 1
            bne         @no
            lda         ex
            beq         @yes
            cmp         #<SNIL
            beq         @yes
@no:
            clc
            rts
@yes:
            sec
            rts

n_jump:
            jmp         (ev_jv)

; ex's word .Y set to .A, .X
nu_ex_set:
            pha
            phx
            lda         ex
            ldx         ex + 1
            jsr         deref
            tya
            asl
            tay
            plx
            pla
            sta         (hp),y
            iny
            txa
            sta         (hp),y
            rts

; ---- Not yet

; A built-in a later phase makes: "Not yet: 'name'"
bi_notyet:
            jsr         nu_begin
            LDAX        s_notyet
            jsr         out_text
            ldx         bi_n
            FARN        2, bi_name
            lda         #$27
            jsr         out_byte
            jmp         nu_end

; ---- Functions

; (fn {formals} {body}): a function made here (its scope this one): its formals a list of symbols, its body a list
bi_fn:
            lda         #0
            bra         make_fn
; (fexpr {formals} {body}): the same, its arguments to come as they're written
bi_fexpr:
            lda         #FN_FEXPR
make_fn:
            sta         fa_flags
            jsr         arg0                                ; (Its formals: symbols)
            jsr         symbols
            bcc         :+
            LDAX        s_fnformals
            jmp         fn_error
:
            sta         et
            stx         et + 1
            jsr         arg1                                ; (Its body: a list)
            lda         ex
            ldx         ex + 1
            jsr         q_conses
            bcc         :+
            LDAX        s_fnbody
            jmp         fn_error
:
            sta         eu
            stx         eu + 1
; A function made: et its formals, eu its body, ee its scope, fa_flags its flags (and FN_EXTRAS if its body takes
; extra arguments).  OUT: ex = it; C = 1 if there's no room
fn_make:
            jsr         takes_extras
            lda         #PK_FUNC
            jsr         cell_alloc
            bcs         @rts
            MOVW        ex, hv
            lda         et
            ldx         et + 1
            ldy         #0
            jsr         nu_ex_set
            lda         eu
            ldx         eu + 1
            ldy         #1
            jsr         nu_ex_set
            lda         ee
            ldx         ee + 1
            ldy         #2
            jsr         nu_ex_set
            lda         fa_flags
            ldx         #0
            ldy         #3
            jsr         nu_ex_set
            clc
@rts:
            rts

; The error .A, .X (a text: "'fn' ..."), its name 'fn' or 'fexpr' after its quote
fn_error:
            sta         fe_text
            stx         fe_text + 1
            jsr         nu_begin
            lda         #$27
            jsr         out_byte
            lda         fa_flags                            ; ('fn' or 'fexpr')
            bne         :+
            LDAX        s_fn
            bra         :++
:
            LDAX        s_fexpr
:
            jsr         out_text
            lda         fe_text
            ldx         fe_text + 1
            jsr         out_text
            jmp         nu_end

; (fun {name formals...} {body}): NIL, name defined globally as the function (fn {formals...} {body}), made here
bi_fun:
            stz         fa_flags
            jsr         arg0
            jsr         symbols
            bcs         @spec
            sta         et
            stx         et + 1
            ora         et + 1
            beq         @spec
            jsr         arg1
            lda         ex
            ldx         ex + 1
            jsr         q_conses
            bcc         :+
            LDAX        s_funbody
            jmp         n_error
:
            sta         eu
            stx         eu + 1
            lda         et                                  ; (Its name: lk_s; its formals, the rest)
            ldx         et + 1
            jsr         deref
            lda         (hp)
            sta         lk_s
            ldy         #1
            lda         (hp),y
            sta         lk_s + 1
            iny
            lda         (hp),y
            pha
            iny
            lda         (hp),y
            sta         et + 1
            pla
            sta         et
            jsr         fn_make
            bcs         @rts
            lda         lk_s                                ; (Defined globally)
            ldx         lk_s + 1
            jsr         deref
            ldy         #2
            lda         ex
            sta         (hp),y
            iny
            lda         ex + 1
            sta         (hp),y
            stz         ex
            stz         ex + 1
            clc
@rts:
            rts
@spec:
            LDAX        s_funspec
            jmp         n_error

; ex = the argument 0 (1, 2 ...), at argp.  Keeps .X? no
arg0:
            ldy         #0
            bra         arg_y
arg1:
            ldy         #2
            bra         arg_y
arg2:
            ldy         #4
arg_y:
            lda         (argp),y
            sta         ex
            iny
            lda         (argp),y
            sta         ex + 1
            rts

; ex, a list of symbols (a Q-expression).  OUT: C = 0 and .A, .X = its conses; C = 1 if it isn't one
symbols:
            lda         ex
            ldx         ex + 1
            jsr         q_conses
            bcs         @rts
            sta         ht
            stx         ht + 1
            pha
            phx
@item:
            lda         ht
            ora         ht + 1
            beq         @yes
            lda         ht
            ldx         ht + 1
            jsr         deref
            ldy         #1
            lda         (hp),y
            tax
            lda         (hp)
            lsr
            bcs         @no
            cpx         #IMM_PAGES
            bcc         @no
            lda         pk,x
            cmp         #PK_SYMBOL
            bne         @no
            ldy         #2
            lda         (hp),y
            pha
            iny
            lda         (hp),y
            sta         ht + 1
            pla
            sta         ht
            bra         @item
@yes:
            plx
            pla
            clc
@rts:
            rts
@no:
            plx
            pla
            sec
            rts

; fa_flags |= FN_EXTRAS if the body eu names &_ or &1, &2 ... anywhere in it (its lists walked, what's left to
; look at on the stack: fe_n values)
takes_extras:
            stz         fe_n
            stz         fe_n + 1
            lda         eu
            ldx         eu + 1
            jsr         te_push
@next:
            lda         fe_n
            ora         fe_n + 1
            beq         @rts
            jsr         te_pop
            sta         te_v
            stx         te_v + 1
            jsr         list_kind
            cmp         #PK_SCONS
            beq         @cons
            cmp         #PK_QCONS
            beq         @cons
            cmp         #PK_SCOPED
            beq         @scoped
            lda         te_v                                ; (A symbol: its name)
            lsr
            bcs         @next
            ldx         te_v + 1
            cpx         #IMM_PAGES
            bcc         @next
            lda         pk,x
            cmp         #PK_SYMBOL
            bne         @next
            lda         te_v
            jsr         amp_name_p
            bcc         @next
            lda         fa_flags                            ; (It takes extras: what's left dropped)
            ora         #FN_EXTRAS
            sta         fa_flags
@drop:
            lda         fe_n
            ora         fe_n + 1
            beq         @rts
            jsr         te_pop
            bra         @drop
@rts:
            rts
@scoped:
            lda         te_v
            ldx         te_v + 1
            ldy         #0
            jsr         cell_get
            jsr         te_push
            bra         @next
@cons:
            lda         te_v                                ; (Its cdr, then its car, to look at)
            ldx         te_v + 1
            ldy         #1
            jsr         cell_get
            jsr         te_push
            lda         te_v
            ldx         te_v + 1
            ldy         #0
            jsr         cell_get
            jsr         te_push
            bra         @next

te_push:
            jsr         push
            inc         fe_n
            bne         :+
            inc         fe_n + 1
:
            rts

te_pop:
            jsr         pop
            pha
            lda         fe_n
            bne         :+
            dec         fe_n + 1
:
            dec         fe_n
            pla
            rts

; IN: .A, .X = a symbol.  OUT: C = 1 if its name is &_, or & and digits
amp_name_p:
            jsr         deref
            lda         (hp)
            pha
            ldy         #1
            lda         (hp),y
            tax
            pla
            jsr         blob_data
            lda         hn
            cmp         #2
            bcc         @no
            lda         (hp)
            cmp         #'&'
            bne         @no
            ldy         #1
            lda         (hp),y
            cmp         #'_'
            bne         @digits
            lda         hn
            cmp         #2
            beq         @yes
            bra         @no
@digits:
            cpy         hn
            beq         @yes
            lda         (hp),y
            cmp         #'0'
            bcc         @no
            cmp         #'9' + 1
            bcs         @no
            iny
            bra         @digits
@yes:
            sec
            rts
@no:
            clc
            rts


; ex's type, as danlang names it in its errors (Number, Q-Expression ...), out
out_type:
            lda         ex
            lsr
            bcs         @number
            ldx         ex + 1
            cpx         #IMM_PAGES
            bcs         @cell
            ldy         #CHAR_TYPE
            cpx         #>CHAR0
            bcs         @char
            ldy         #T_TYPE
            cpx         #0
            bne         @out
            lda         ex                                  ; (NIL, T, (), exit)
            lsr
            cmp         #5
            bcs         @out
            tax
            ldy         imm_type,x
            bra         @out
@char:
            cpx         #>BUILTIN0
            bcc         @out
            ldy         #FN_TYPE
            bra         @out
@number:
            ldy         #NUM_TYPE
            bra         @out
@cell:
            lda         pk,x
            tax
            ldy         kind_type,x
@out:
            lda         type_hi,y
            tax
            lda         type_lo,y
            jmp         out_text

; ---- Numbers (fixnums, as yet: phase 5 has the rest)

; (+ x...): the numbers' sum (0 for none); or, the first a string or a character, the values as print shows them,
; joined
bi_add:
            stz         nm_acc
            stz         nm_acc + 1
            stz         nm_over
            lda         cn
            ora         cn + 1
            bne         :+
            jmp         nm_done
:
            jsr         n_arg0                                ; (The first a string or a character: joined)
            lda         ex
            ldx         ex + 1
            jsr         kind_of
            cmp         #PK_STRING
            beq         @join
            lda         ex + 1
            cmp         #>CHAR0
            bcc         @sum
            cmp         #>BUILTIN0
            bcs         @sum
            lda         ex
            lsr
            bcs         @sum
@join:
            FARN        2, bo_join
            rts
@sum:
            stz         nm_i
            stz         nm_i + 1
@next:
            jsr         nm_arg
            bcc         @lb13
            jmp         nm_done
@lb13:
            bvs         @notnum
            clc                                             ; (acc + n)
            lda         nm_acc
            adc         nm_n
            sta         nm_acc
            lda         nm_acc + 1
            adc         nm_n + 1
            sta         nm_acc + 1
            bvc         @next
            inc         nm_over
            bra         @next
@notnum:
            LDAX        s_addnum
            jmp         n_error

; (- x...): x less the rest; (- x) is -x; (-) is 0
bi_sub:
            lda         #'-'
            sta         nm_op
            stz         nm_acc
            stz         nm_acc + 1
            stz         nm_over
            stz         nm_i
            stz         nm_i + 1
            jsr         nm_arg
            bcc         :+
            jmp         nm_done
:
            bvc         :+
            jmp         nm_notnum
:
            MOVW        nm_acc, nm_n
            lda         cn                                  ; ((- x): -x)
            cmp         #1
            bne         @next
            lda         cn + 1
            bne         @next
            jsr         nm_negate
            jmp         nm_done
@next:
            jsr         nm_arg
            bcc         :+
            jmp         nm_done
:
            bvc         :+
            jmp         nm_notnum
:
            sec
            lda         nm_acc
            sbc         nm_n
            sta         nm_acc
            lda         nm_acc + 1
            sbc         nm_n + 1
            sta         nm_acc + 1
            bvc         @next
            inc         nm_over
            bra         @next

; (* x...): the numbers' product (1 for none)
bi_mul:
            lda         #'*'
            sta         nm_op
            lda         #1
            sta         nm_acc
            stz         nm_acc + 1
            stz         nm_over
            stz         nm_i
            stz         nm_i + 1
@next:
            jsr         nm_arg
            bcc         :+
            jmp         nm_done
:
            bvc         :+
            jmp         nm_notnum
:
            jsr         nm_times
            bra         @next

; (/ x...): x divided by the rest (exactly: as yet, only when it's whole); (/ x) is 1/x; (/) is 1
bi_div:
            lda         #'/'
            sta         nm_op
            lda         #1
            sta         nm_acc
            stz         nm_acc + 1
            stz         nm_over
            stz         nm_i
            stz         nm_i + 1
            lda         cn
            ora         cn + 1
            bne         :+
            jmp         nm_done
:
            jsr         nm_arg
            bvc         :+
            jmp         nm_notnum
:
            lda         cn                                  ; ((/ x): 1/x)
            cmp         #1
            bne         @first
            lda         cn + 1
            bne         @first
            lda         nm_n
            ora         nm_n + 1
            bne         :+
            jmp         nm_divzero
:
            MOVW        nm_m, nm_n
            lda         #1
            sta         nm_acc
            stz         nm_acc + 1
            bra         @divide
@first:
            MOVW        nm_acc, nm_n
@next:
            jsr         nm_arg
            bcc         :+
            jmp         nm_done
:
            bvc         :+
            jmp         nm_notnum
:
            lda         nm_n
            ora         nm_n + 1
            bne         :+
            jmp         nm_divzero
:
            MOVW        nm_m, nm_n
@divide:
            jsr         nm_divide                           ; (acc / m: exact, or a fraction: not yet)
            bra         @next

; The next argument (nm_i): nm_n its number.  OUT: C = 1 if there are no more; V = 1 if it isn't a number (a
; fixnum)
nm_arg:
            lda         nm_i
            cmp         cn
            lda         nm_i + 1
            sbc         cn + 1
            bcs         @rts
            lda         nm_i
            asl
            tay
            inc         nm_i
            bne         :+
            inc         nm_i + 1
:
            lda         (argp),y
            sta         nm_n
            iny
            lda         (argp),y
            sta         nm_n + 1
            lda         nm_n
            lsr
            bcc         @notnum
            lda         nm_n
            ldx         nm_n + 1
            jsr         unfix
            sta         nm_n
            stx         nm_n + 1
            clc
            clv
@rts:
            rts
@notnum:
            clc
            bit         nm_v
            rts
nm_v:       .byte       $40

; nm_acc = -nm_acc
nm_negate:
            sec
            lda         #0
            sbc         nm_acc
            sta         nm_acc
            lda         #0
            sbc         nm_acc + 1
            sta         nm_acc + 1
            bvc         :+
            inc         nm_over
:
            rts

; nm_acc = nm_acc * nm_n (nm_over if it's past 16 bits)
nm_times:
            stz         nm_sign
            lda         nm_acc + 1                          ; (The magnitudes, the sign apart)
            bpl         :+
            jsr         nm_negate
            inc         nm_sign
:
            lda         nm_n + 1
            bpl         :+
            sec
            lda         #0
            sbc         nm_n
            sta         nm_n
            lda         #0
            sbc         nm_n + 1
            sta         nm_n + 1
            lda         nm_sign
            eor         #1
            sta         nm_sign
:
            stz         nm_p                                ; (nm_p: the product, 32 bits)
            stz         nm_p + 1
            stz         nm_p + 2
            stz         nm_p + 3
            ldx         #16
@bit:
            asl         nm_p
            rol         nm_p + 1
            rol         nm_p + 2
            rol         nm_p + 3
            asl         nm_n
            rol         nm_n + 1
            bcc         @no
            clc
            lda         nm_p
            adc         nm_acc
            sta         nm_p
            lda         nm_p + 1
            adc         nm_acc + 1
            sta         nm_p + 1
            bcc         @no
            inc         nm_p + 2
            bne         @no
            inc         nm_p + 3
@no:
            dex
            bne         @bit
            lda         nm_p + 2                            ; (Past 15 bits: too big)
            ora         nm_p + 3
            bne         @over
            lda         nm_p + 1
            bmi         @over
            MOVW        nm_acc, nm_p
            lda         nm_sign
            beq         :+
            jsr         nm_negate
:
            rts
@over:
            inc         nm_over
            rts

; nm_acc = nm_acc / nm_m, if it's whole (nm_over if it isn't: a fraction, not yet)
nm_divide:
            stz         nm_sign
            lda         nm_acc + 1
            bpl         :+
            jsr         nm_negate
            inc         nm_sign
:
            lda         nm_m + 1
            bpl         :+
            sec
            lda         #0
            sbc         nm_m
            sta         nm_m
            lda         #0
            sbc         nm_m + 1
            sta         nm_m + 1
            lda         nm_sign
            eor         #1
            sta         nm_sign
:
            stz         nm_p                                ; (nm_p: the remainder; nm_acc becomes the quotient)
            stz         nm_p + 1
            ldx         #16
@bit:
            asl         nm_acc
            rol         nm_acc + 1
            rol         nm_p
            rol         nm_p + 1
            sec
            lda         nm_p
            sbc         nm_m
            tay
            lda         nm_p + 1
            sbc         nm_m + 1
            bcc         @no
            sta         nm_p + 1
            sty         nm_p
            inc         nm_acc
@no:
            dex
            bne         @bit
            lda         nm_p                                ; (A remainder: a fraction)
            ora         nm_p + 1
            beq         :+
            inc         nm_over
:
            lda         nm_sign
            beq         :+
            jmp         nm_negate
:
            rts

; The result: nm_acc a fixnum (past one, or a fraction: not yet)
nm_done:
            lda         nm_over
            bne         @past
            lda         nm_acc
            ldx         nm_acc + 1
            jsr         fix_make
            bcs         @past
            MOVW        ex, hv
            clc
            rts
@past:
            LDAX        s_numpast
            jmp         n_error

nm_notnum:
            jsr         nu_begin                           ; ("All parameters to operator '-' must be numbers.")
            LDAX        s_opnum
            jsr         out_text
            lda         nm_op
            jsr         out_byte
            LDAX        s_mustnum
            jsr         out_text
            jmp         nu_end

nm_divzero:
            LDAX        s_divzero
            jmp         n_error

; (1+ x): (+ x 1).  (1- x): (- x 1)
bi_inc:
            lda         #<bi_add
            ldx         #>bi_add
            bra         nm_plus1
bi_dec:
            lda         #<bi_sub
            ldx         #>bi_sub
nm_plus1:
            sta         ev_jv
            stx         ev_jv + 1
            ldy         #1                                  ; (x and 1: the arguments, here)
:
            lda         (argp),y
            sta         nm_args,y
            dey
            bpl         :-
            lda         #<FIX(1)
            sta         nm_args + 2
            stz         nm_args + 3
            LDAX        nm_args
            sta         argp
            stx         argp + 1
            lda         #2
            sta         cn
            jsr         n_jump
            php                                             ; (Its one argument on the stack, still)
            lda         #1
            sta         cn
            plp
            rts

; (abs x): x's magnitude
bi_abs:
            jsr         n_arg0
            lda         ex
            lsr
            bcs         :+
            LDAX        s_absnum
            jmp         n_error
:
            lda         ex + 1
            bpl         :+
            stz         nm_i
            stz         nm_i + 1
            jmp         bi_sub                              ; ((- x), cn 1)
:
            clc
            rts

; (random n): an integer from 0 up to, not including, n (an integer, 1 or more): a 16-bit xorshift's, seeded by
; the tick count as it's first wanted; drawn again while it's past n
bi_random:
            jsr         n_arg0
            lda         ex
            lsr
            bcs         :+
            LDAX        s_randint
            jmp         n_error
:
            lda         ex
            ldx         ex + 1
            jsr         unfix
            sta         nm_n
            stx         nm_n + 1
            txa
            bmi         @low
            ora         nm_n
            bne         :+
@low:
            LDAX        s_randpos
            jmp         n_error
:
            lda         rn_x                                ; (Seeded: the tick count, not 0)
            ora         rn_x + 1
            bne         :+
            jsr         TICKS
            eor         #$5A
            sta         rn_x
            txa
            eor         #$A5
            sta         rn_x + 1
            ora         rn_x
            bne         :+
            inc         rn_x
:
            lda         nm_n                                ; (n - 1, and the least mask that covers it)
            sec
            sbc         #1
            sta         nm_m
            lda         nm_n + 1
            sbc         #0
            sta         nm_m + 1
            stz         nm_p
            stz         nm_p + 1
@grow:
            lda         nm_p
            cmp         nm_m
            lda         nm_p + 1
            sbc         nm_m + 1
            bcs         @draw
            sec
            rol         nm_p
            rol         nm_p + 1
            bra         @grow
@draw:
            lda         rn_x + 1                            ; (The next: xorshift 7, 9, 8)
            lsr
            lda         rn_x
            ror
            eor         rn_x + 1
            sta         rn_x + 1
            ror
            eor         rn_x
            sta         rn_x
            eor         rn_x + 1
            sta         rn_x + 1
            lda         rn_x                                ; (Masked: again if it's n or more)
            and         nm_p
            sta         nm_m
            lda         rn_x + 1
            and         nm_p + 1
            sta         nm_m + 1
            lda         nm_m
            cmp         nm_n
            lda         nm_m + 1
            sbc         nm_n + 1
            bcs         @draw
            lda         nm_m
            ldx         nm_m + 1
            jsr         fix_ax
            sta         ex
            stx         ex + 1
            clc
            rts

; ---- Errors, types

; (error message [code]): an error, its message as print shows message, its code an atom
bi_error:
            jsr         n_arg0                                ; (Its message: a string, as print shows it)
            lda         ex
            ldx         ex + 1
            jsr         kind_of
            cmp         #PK_STRING
            beq         @string
            FARN        2, bo_display                       ; (ex: as print shows it)
            bcs         @rts
@string:
            MOVW        hv, ex
            jsr         error_wrap
            bcs         @rts
            MOVW        ex, hv
            lda         cn                                  ; (Its code)
            cmp         #2
            bcc         @done
            jsr         n_arg1
            lda         ex
            ldx         ex + 1
            jsr         kind_of
            cmp         #PK_ATOM
            beq         :+
            LDAX        s_errcode
            jmp         n_error
:
            lda         hv
            ldx         hv + 1
            jsr         deref
            ldy         #2
            lda         ex
            sta         (hp),y
            iny
            lda         ex + 1
            sta         (hp),y
            MOVW        ex, hv
@done:
            clc
@rts:
            rts

; (error-message x): an error's message (NIL for anything else)
bi_error_message:
            ldy         #0
            bra         error_word
; (error-code x): an error's code (NIL if it has none, or for anything else)
bi_error_code:
            ldy         #1
error_word:
            phy
            jsr         n_arg0
            jsr         n_is_err
            ply
            bcc         nil_out
            lda         ex
            ldx         ex + 1
            jsr         cell_get
            sta         ex
            stx         ex + 1
            clc
            rts

; ex = NIL; C = 0
nil_out:
            stz         ex
            stz         ex + 1
            clc
            rts

; ex = T; C = 0
t_out:
            lda         #<T_VAL
            sta         ex
            stz         ex + 1
            clc
            rts

; ex = T if C = 1, else NIL; C = 0
bool_out:
            bcs         t_out
            bra         nil_out

; (type-of x): x's type, an atom: :number :string :char :atom :symbol :function :error :t :list :sexpr :hash
; :stream :exit
bi_type_of:
            jsr         arg_seen                            ; (ex: x, a scoped one's expression)
            jsr         type_index
            tax
            lda         tyn_hi,x
            sta         hq + 1
            lda         tyn_lo,x
            sta         hq
            lda         tyn_len,x
            sta         hn
            stz         hn + 1
            lda         #PK_ATOM
            jsr         intern
            bcs         @rts
            MOVW        ex, hv
            clc
@rts:
            rts

; ex = the argument 0, seen through a scoped one (its expression)
arg_seen:
            jsr         n_arg0
            lda         ex
            ldx         ex + 1
            jsr         kind_of
            cmp         #PK_SCOPED
            bne         @rts
            lda         ex
            ldx         ex + 1
            ldy         #0
            jsr         cell_get
            pha
            jsr         list_kind                           ; (A list in it: it stays a Q-expression)
            cmp         #PK_QCONS
            beq         @keep
            pla
            sta         ex
            stx         ex + 1
@rts:
            rts
@keep:
            pla
            rts

; OUT: .A = ex's type, an index into the types (TY_*)
TY_NUMBER       = 0
TY_STRING       = 1
TY_CHAR         = 2
TY_ATOM         = 3
TY_SYMBOL       = 4
TY_FUNCTION     = 5
TY_ERROR        = 6
TY_T            = 7
TY_LIST         = 8
TY_SEXPR        = 9
TY_HASH         = 10
TY_STREAM       = 11
TY_EXIT         = 12
type_index:
            lda         ex
            lsr
            bcs         @number
            ldx         ex + 1
            cpx         #IMM_PAGES
            bcs         @cell
            cpx         #>BUILTIN0
            bcs         @function
            cpx         #>CHAR0
            bcs         @char
            lda         ex                                  ; (NIL, T, (), exit)
            lsr
            tax
            lda         imm_ty,x
            rts
@number:
            lda         #TY_NUMBER
            rts
@function:
            lda         #TY_FUNCTION
            rts
@char:
            lda         #TY_CHAR
            rts
@cell:
            lda         pk,x
            tax
            lda         kind_ty,x
            rts

bi_is_t:
            jsr         n_arg0
            lda         ex
            cmp         #<T_VAL
            beq         @lb12
            jmp         nil_out
@lb12:
            lda         ex + 1
            beq         @lb11
            jmp         nil_out
@lb11:
            jmp         t_out
bi_is_nil:
            jsr         n_arg0
            jsr         n_is_nil
            jmp         bool_out
bi_is_num:
            jsr         n_arg0
            jsr         type_index
            cmp         #TY_NUMBER
            bne         @lb10
            jmp         t_out
@lb10:
            jmp         nil_out
bi_is_int:
            jsr         n_arg0
            lda         ex
            lsr
            jmp         bool_out
bi_is_fixed:
            jmp         nil_out                             ; (As yet: no fixed decimals, rationals, complexes)
bi_is_atom:
            lda         #TY_ATOM
            bra         is_type
bi_is_symbol:
            lda         #TY_SYMBOL
            bra         is_type
bi_is_string:
            lda         #TY_STRING
            bra         is_type
bi_is_char:
            lda         #TY_CHAR
            bra         is_type
bi_is_function:
            lda         #TY_FUNCTION
            bra         is_type
bi_is_error:
            lda         #TY_ERROR
            bra         is_type
bi_is_qexpr:
            lda         #TY_LIST
            bra         is_type
bi_is_sexpr:
            lda         #TY_SEXPR
; T if the argument 0's type is .A
is_type:
            pha
            jsr         arg_seen
            jsr         type_index
            sta         ht
            pla
            cmp         ht
            beq         :+
            jmp         nil_out
:
            jmp         t_out

; (not x): T if x is NIL, else NIL
bi_not:
            jsr         n_arg0
            jsr         n_is_nil
            jmp         bool_out

.bss
nu_start:    .res        2                                   ; (An error's capture: where it began)
rn_x:       .res        2                                   ; (random's generator: 0 till it's seeded)

.segment "RODATA3"
; Types as danlang's errors name them (LVal.LEName)
NUM_TYPE        = 0
CHAR_TYPE       = 4
FN_TYPE         = 7
T_TYPE          = 9
type_lo:    .lobytes    tn_number, tn_error, tn_atom, tn_symbol, tn_char, tn_string, tn_hash, tn_function
            .lobytes    tn_stream, tn_t, tn_exit, tn_sexpr, tn_qexpr
type_hi:    .hibytes    tn_number, tn_error, tn_atom, tn_symbol, tn_char, tn_string, tn_hash, tn_function
            .hibytes    tn_stream, tn_t, tn_exit, tn_sexpr, tn_qexpr
imm_type:   .byte       12, 9, 11, 10, 9                    ; (NIL, T, (), exit, unbound)
kind_type:  .byte       9, 9, 9, 11, 12, 12, 5, 0, 0, 0, 0, 1, 7, 3, 2, 7, 6, 8, 9, 9, 9, 9
tn_number:  .byte       "Number", 0
tn_error:   .byte       "Error", 0
tn_atom:    .byte       "Atom", 0
tn_symbol:  .byte       "Symbol", 0
tn_char:    .byte       "Character", 0
tn_string:  .byte       "String", 0
tn_hash:    .byte       "Hash", 0
tn_function: .byte      "Function", 0
tn_stream:  .byte       "Stream", 0
tn_t:       .byte       "T", 0
tn_exit:    .byte       "Exit", 0
tn_sexpr:   .byte       "S-Expression", 0
tn_qexpr:   .byte       "Q-Expression", 0

s_notyet:   .byte       "Not yet: '", 0
s_fn:       .byte       "fn", 0
s_fexpr:    .byte       "fexpr", 0
s_fnformals: .byte      "' expects a list of symbols first", 0
s_fnbody:   .byte       "' expects a QExpr body", 0
s_funspec:  .byte       "'fun' expects {name formals...} first", 0
s_funbody:  .byte       "'fun' expects a QExpr body", 0
s_addnum:   .byte       "'+' adds numbers, or joins values to a string", 0
s_opnum:    .byte       "All parameters to operator '", 0
s_mustnum:  .byte       "' must be numbers.", 0
s_divzero:  .byte       "Division by zero.", 0
s_randint:  .byte       "'random' expects an integer", 0
s_randpos:  .byte       "'random' expects a number above 0", 0
s_numpast:  .byte       "Not yet: a number past a fixnum, or a fraction (phase 5)", 0
s_absnum:   .byte       "Cannot perform 'abs' on non-number", 0
s_errcode:  .byte       "An error's code must be an atom", 0

; The types' names (type-of's), each an atom's
tyn_lo:     .lobytes    ty_number, ty_string, ty_char, ty_atom, ty_symbol, ty_function, ty_error, ty_t, ty_list
            .lobytes    ty_sexpr, ty_hash, ty_stream, ty_exit
tyn_hi:     .hibytes    ty_number, ty_string, ty_char, ty_atom, ty_symbol, ty_function, ty_error, ty_t, ty_list
            .hibytes    ty_sexpr, ty_hash, ty_stream, ty_exit
tyn_len:    .byte       6, 6, 4, 4, 6, 8, 5, 1, 4, 5, 4, 6, 4
ty_number:  .byte       "number"
ty_string:  .byte       "string"
ty_char:    .byte       "char"
ty_atom:    .byte       "atom"
ty_symbol:  .byte       "symbol"
ty_function: .byte      "function"
ty_error:   .byte       "error"
ty_t:       .byte       "t"
ty_list:    .byte       "list"
ty_sexpr:   .byte       "sexpr"
ty_hash:    .byte       "hash"
ty_stream:  .byte       "stream"
ty_exit:    .byte       "exit"
imm_ty:     .byte       TY_LIST, TY_T, TY_SEXPR, TY_EXIT, TY_T
kind_ty:    .byte       TY_T, TY_T, TY_T, TY_SEXPR, TY_LIST, TY_LIST, TY_STRING, TY_NUMBER, TY_NUMBER, TY_NUMBER
            .byte       TY_NUMBER, TY_ERROR, TY_FUNCTION, TY_SYMBOL, TY_ATOM, TY_FUNCTION, TY_HASH, TY_STREAM
            .byte       TY_T, TY_T, TY_T, TY_T

