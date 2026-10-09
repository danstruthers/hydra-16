; ****************************************************************************
; expr.s - as's scanner, expressions, heap, symbol table and unnamed labels (as.s has the whole story).
;   The scanner reads the line (line, from li on): blanks, names (tok, tlen: a cheap local's, @name, with its scope's
; two bytes after it), numbers ($hex, %binary, decimal, 'c'), strings (sbuf, slen).  A ; and what's after it are
; the line's end.
;   An expression is ca65's: 32 bits, signed; its operators by precedence, highest first: unary + - ~ < > ^ ! (and
; .bitnot .not); * / .mod & ^ << >> (.bitand .bitxor .shl .shr); + - | (.bitor); = <> < > <= >=; && .and .xor;
; || .or.  Its terms: numbers, names, * (the place assembled at), :+ and :- (unnamed labels), ( ), and .strlen,
; .defined (.def), .blank, .match, .lobyte, .hibyte, .bankbyte, .loword, .hiword.  Its value is val; eu <> 0 if a
; name in it isn't defined yet (in any pass: 0 stands in), ef <> 0 if one's defined only later in this pass (the
; last pass's value stands in).  An instruction takes the zero page form of an address only when neither is set:
; so each pass makes the same choice.
;   The heap is RAM banks, a bump allocator: a ref is a bank's index (bits 13-15) and an offset in it.  The symbol
; table is 256 chains (by a hash of the name) of symbols in it (SY_*: as.inc).  The unnamed labels' places are in a
; bank of their own, in order: the next one defined is ucount's; :+ is it, :- the one before.

.include "as.inc"

BANKREG         = $00           ; The task's RAM bank register

; val pushed on the stack; lhs = what was pushed (C kept)
.macro PUSHV
            lda         val + 3
            pha
            lda         val + 2
            pha
            lda         val + 1
            pha
            lda         val
            pha
.endmacro

.macro POPL
            pla
            sta         lhs
            pla
            sta         lhs + 1
            pla
            sta         lhs + 2
            pla
            sta         lhs + 3
.endmacro

.zeropage
hp:         .res        2                                   ; The heap: a ref's bytes, in the bank window
p1:         .res        2
p2:         .res        2
p3:         .res        2

.bss
tok:        .res        TOK_MAX + 3                         ; A name ...
tlen:       .res        1                                   ;   its length (with a local's scope)
sbuf:       .res        LINE_MAX + 1                        ; A string ...
slen:       .res        1                                   ;   its length
val:        .res        4                                   ; An expression's value ...
lhs:        .res        4                                   ;   (an operator's left side)
eu:         .res        1                                   ;   a name in it undefined ...
ef:         .res        1                                   ;   and defined later in this pass
scope:      .res        2                                   ; Cheap locals' scope: the normal labels so far
uhave:      .res        1                                   ; (1: ubank is taken)
hbank:      .res        HEAP_BANKS                          ; The heap's banks ...
hbanks:     .res        1                                   ;   how many ...
hidx:       .res        1                                   ;   the one being filled ...
hoff:       .res        2                                   ;   its next byte
headl:      .res        256                                 ; The symbol table's chains
headh:      .res        256
hash:       .res        1
tf1:        .res        1                                   ; (tfind's: tok's second byte)
sref:       .res        2                                   ; sym_find's symbol: its ref
ubank:      .res        1                                   ; The unnamed labels' bank ...
ucount:     .res        2                                   ;   how many this pass has defined ...
utotal:     .res        2                                   ;   and the last pass
t32:        .res        4                                   ; (Scratch: multiply and divide)
sign:       .res        1
kbuf1:      .res        16                                  ; (.match's: each argument's tokens' kinds)
kbuf2:      .res        16
kn1:        .res        1
kn2:        .res        1
eachv:      .res        2                                   ; (sym_each's routine)
eachb:      .res        1

.code

; ---- The scanner

; .A = the line's byte at li (0: its end, or a ;).  Z: the end.  Keeps .X
peek:
            ldy         li
            lda         line,Y
            cmp         #';'
            bne         :+
            lda         #0
:
            cmp         #0
            rts

; li on one (past the byte peek gave)
eat:
            inc         li
            rts

; li past blanks
skipws:
            ldy         li
:
            lda         line,Y
            cmp         #' '
            beq         :+
            cmp         #TAB
            bne         :++
:
            iny
            bne         :--
:
            sty         li
            rts

; Z = 1: nothing more on the line but blanks (and a comment)
atend:
            jsr         skipws
            jmp         peek

; .A, past blanks, is the next byte?  Then li past it, C = 0; else C = 1.  Keeps .X
expect:
            sta         p3
            jsr         skipws
            jsr         peek
            cmp         p3
            bne         :+
            jsr         eat
            clc
            rts
:
            sec
            rts

; .A in lower case.  Keeps .X, .Y
lower:
            cmp         #'A'
            bcc         :+
            cmp         #'Z' + 1
            bcs         :+
            ora         #$20
:
            rts

; C = 1: .A is a letter, a digit or _ (a name's byte after its first).  Keeps .A, .X, .Y
isname:
            cmp         #$80
            bcs         @no
            phx
            tax
            bit         ctype,X                             ; (V: CT_NAME)
            plx
            clc
            bvc         @no
            sec
            rts
@no:
            clc
            rts

; A name at li (its first byte a letter, _, @ or .) into tok (tlen), li past it; a cheap local (@name) with its
; scope's two bytes after it.  C = 1: none there (li past blanks), or a long one (said)
getid:
            jsr         skipws
            ldy         li
            ldx         line,Y
            bmi         @no
            bit         ctype,X                             ; (N: CT_FIRST)
            bpl         @no
@scan:
            iny                                             ; Its end
            ldx         line,Y
            bmi         @end
            bit         ctype,X                             ; (V: CT_NAME)
            bvs         @scan
@end:
            tya
            sec
            sbc         li
            cmp         #TOK_MAX + 1
            bcs         @long
            sta         tlen
            ldx         #0                                  ; Its bytes
            ldy         li
:
            lda         line,Y
            sta         tok,X
            iny
            inx
            cpx         tlen
            bne         :-
            sty         li
            lda         tok
            cmp         #'@'
            bne         :+
            lda         scope                               ; (A cheap local: its scope after it)
            sta         tok,X
            lda         scope + 1
            sta         tok + 1,X
            inx
            inx
            stx         tlen
:
            clc
            rts
@long:
            LDR         r0, s_long
            jsr         err
@no:
            sec
            rts

; tok (tlen bytes, its letters in lower case) the string at r0?  C = 1: it is.  Keeps .X
tokis:
            ldy         #0
:
            lda         (r0),Y
            beq         @end
            cpy         tlen
            bcs         @no
            sta         p3
            lda         tok,Y
            jsr         lower
            cmp         p3
            bne         @no
            iny
            bra         :-
@end:
            cpy         tlen
            beq         @yes
@no:
            clc
            rts
@yes:
            sec
            rts

; The entry of the table at p2 (each: a name's address, then a routine's; 0 after the last) whose name is tok
; (either case).  OUT: C = 1, p2 = its routine; C = 0: none
tfind:
            lda         tok + 1                             ; (Its second byte, first: most differ there)
            jsr         lower
            sta         tf1
            ldy         #0
@entry:
            lda         (p2),Y
            sta         r0
            iny
            lda         (p2),Y
            beq         @no
            sta         r0 + 1
            iny
            phy
            ldy         #1
            lda         (r0),Y
            cmp         tf1
            bne         @next
            jsr         tokis
            bcs         @yes
@next:
            ply
            iny
            iny
            bra         @entry
@yes:
            ply
            lda         (p2),Y
            tax
            iny
            lda         (p2),Y
            sta         p2 + 1
            stx         p2
            sec
            rts
@no:
            clc
            rts

; A string at li ("..."), its bytes into sbuf (slen; a 0 after them).  C = 1: none (li past blanks), or no
; closing " (said)
getstr:
            jsr         skipws
            ldy         li
            lda         line,Y
            cmp         #'"'
            bne         @no
            ldx         #0
@byte:
            iny
            lda         line,Y
            beq         @open
            cmp         #'"'
            beq         @end
            sta         sbuf,X
            inx
            bra         @byte
@end:
            iny
            sty         li
            stx         slen
            stz         sbuf,X
            clc
            rts
@open:
            LDR         r0, s_quote
            jsr         err
@no:
            sec
            rts

; A number at li ($hex, %binary, decimal, 'c': li on its first byte) into val, li past it.  C = 1: a bad one (said)
getnum:
            ldy         li
            lda         line,Y
            stz         val
            stz         val + 1
            stz         val + 2
            stz         val + 3
            cmp         #'$'
            beq         @hex
            cmp         #'%'
            beq         @bin
            cmp         #$27                                ; (')
            beq         @char
@dec:
            lda         line,Y
            sec
            sbc         #'0'
            cmp         #10
            bcs         @done
            pha
            jsr         x10
            pla
            jsr         addd
            iny
            bra         @dec
@hex:
            iny
            ldx         #0                                  ; (Its digits)
@hdig:
            lda         line,Y
            jsr         lower
            sec
            sbc         #'0'
            cmp         #10
            bcc         :+
            sbc         #'a' - '0' - 10
            cmp         #10
            bcc         @hend
            cmp         #16
            bcs         @hend
:
            pha
            lda         #4
            jsr         shlv
            pla
            ora         val
            sta         val
            inx
            iny
            bra         @hdig
@bin:
            iny
            ldx         #0
@bdig:
            lda         line,Y
            sec
            sbc         #'0'
            cmp         #2
            bcs         @hend
            pha
            lda         #1
            jsr         shlv
            pla
            ora         val
            sta         val
            inx
            iny
            bra         @bdig
@hend:
            cpx         #0
            beq         @bad
@done:
            lda         line,Y                              ; (A name's byte straight after it: not a number)
            jsr         isname
            bcs         @bad
            sty         li
            clc
            rts
@char:
            iny
            lda         line,Y
            beq         @bad
            sta         val
            iny
            lda         line,Y
            cmp         #$27
            bne         :+
            iny                                             ; (Its closing ')
:
            sty         li
            clc
            rts
@bad:
            LDR         r0, s_number
            jsr         err
            sec
            rts

; val = val * 10.  Keeps .Y
x10:
            lda         #1                                  ; val * 2 ...
            jsr         shlv
            ldx         #3                                  ;   kept ...
:
            lda         val,X
            sta         t32,X
            dex
            bpl         :-
            lda         #2                                  ;   then * 8, + the * 2
            jsr         shlv
            clc
            ldx         #0
:
            lda         val,X
            adc         t32,X
            sta         val,X
            inx
            txa
            eor         #4
            bne         :-
            rts

; val = val + .A.  Keeps .Y
addd:
            clc
            adc         val
            sta         val
            bcc         :+
            inc         val + 1
            bne         :+
            inc         val + 2
            bne         :+
            inc         val + 3
:
            rts

; val = val << .A.  Keeps .X, .Y
shlv:
            phx
            tax
            beq         @done
:
            asl         val
            rol         val + 1
            rol         val + 2
            rol         val + 3
            dex
            bne         :-
@done:
            plx
            rts

; ---- Expressions

; An expression at li into val (eu, ef: its names' state).  C = 1: a bad one (said)
expr:
            stz         eu
            stz         ef
            ; (on into e_or)

e_or:
            jsr         e_and
            bcs         @done
@loop:
            ldx         #ops_or - optab
            jsr         opm
            bcs         @ok
            PUSHV
            jsr         e_and
            POPL
            bcs         @done
            jsr         bools
            lda         lhs
            ora         val
            jsr         setbool
            bra         @loop
@ok:
            clc
@done:
            rts

e_and:
            jsr         e_cmp
            bcs         @done
@loop:
            ldx         #ops_and - optab
            jsr         opm
            bcs         @ok
            pha
            PUSHV
            jsr         e_cmp
            POPL
            pla
            bcs         @done
            pha
            jsr         bools
            pla
            cmp         #OP_LAND
            bne         :+
            lda         lhs
            and         val
            bra         :++
:
            lda         lhs                                 ; (.xor)
            eor         val
:
            jsr         setbool
            bra         @loop
@ok:
            clc
@done:
            rts

e_cmp:
            jsr         e_add
            bcs         @done
@loop:
            ldx         #ops_cmp - optab
            jsr         opm
            bcs         @ok
            pha
            PUSHV
            jsr         e_add
            POPL
            pla
            bcs         @done
            jsr         compare
            bra         @loop
@ok:
            clc
@done:
            rts

e_add:
            jsr         e_mul
            bcs         @done
@loop:
            ldx         #ops_add - optab
            jsr         opm
            bcs         @ok
            pha
            PUSHV
            jsr         e_mul
            POPL
            pla
            bcs         @done
            jsr         binop
            bra         @loop
@ok:
            clc
@done:
            rts

e_mul:
            jsr         e_un
            bcs         @done
@loop:
            ldx         #ops_mul - optab
            jsr         opm
            bcs         @ok
            pha
            PUSHV
            jsr         e_un
            POPL
            pla
            bcs         @done
            jsr         binop
            bcs         @done
            bra         @loop
@ok:
            clc
@done:
            rts

; A unary operator and its operand, or a term
e_un:
            ldx         #ops_un - optab
            jsr         opm
            bcs         e_term
            pha
            jsr         e_un
            pla
            bcs         :+
            jsr         unop
            clc
:
            rts

; A term: ( expr ), a number, *, an unnamed label, a name, a function
e_term:
            jsr         skipws
            jsr         peek
            cmp         #'('
            beq         @paren
            cmp         #'$'
            beq         @num
            cmp         #'%'
            beq         @num
            cmp         #$27
            beq         @num
            cmp         #'0'
            bcc         @other
            cmp         #'9' + 1
            bcc         @num
@other:
            cmp         #'*'
            bne         :+
            jsr         eat
            jmp         here_val
:
            cmp         #':'
            bne         :+
            jmp         unnamed
:
            cmp         #'.'
            bne         :+
            jmp         func
:
            jsr         getid
            bcc         symbol
            jmp         e_bad
@num:
            jmp         getnum
@paren:
            jsr         eat
            jsr         e_or
            bcs         :+
            lda         #')'
            jsr         expect
            bcs         e_bad
:
            rts

; The value of the name in tok
symbol:
            jsr         sym_find
            bcs         @undef
            ldy         #SY_FLAGS
            lda         (hp),Y
            and         #SF_MACRO
            bne         @macro
            lda         (hp),Y
            and         #SF_PASS
            beq         @undef
            cmp         pass
            beq         :+
            inc         ef                                  ; (Defined later in this pass: the last's value)
:
            ldy         #SY_VAL + 3
:
            lda         (hp),Y
            sta         val - SY_VAL,Y
            dey
            cpy         #SY_VAL
            bcs         :-
            clc
            rts
@undef:
            inc         eu
            stz         val
            stz         val + 1
            stz         val + 2
            stz         val + 3
            lda         pass                                ; (Pass 1: it may come later)
            cmp         #1
            beq         :+
            LDR         r0, s_undef
            jsr         errtok
            sec
            rts
:
            clc
            rts
@macro:
            LDR         r0, s_macro
            jsr         errtok
            sec
            rts

e_bad:
            LDR         r0, s_expr
            jsr         err
            sec
            rts

; val = the place being assembled at (the segment's base and offset)
here_val:
            ldx         cseg
            clc
            lda         sbasel,X
            adc         soffl,X
            sta         val
            lda         sbaseh,X
            adc         soffh,X
            sta         val + 1
            stz         val + 2
            stz         val + 3
            clc
            rts

; An unnamed label's place: :+ (the next), :++ (the one after) ..., :- (the last), :-- ...
unnamed:
            jsr         eat
            jsr         peek
            stz         p2                                  ; (p2: how many + or -)
            cmp         #'+'
            beq         @fwd
            cmp         #'-'
            bne         e_bad
:
            jsr         eat
            inc         p2
            jsr         peek
            cmp         #'-'
            beq         :-
            sec                                             ; p1 = ucount - them
            lda         ucount
            sbc         p2
            sta         p1
            lda         ucount + 1
            sbc         #0
            sta         p1 + 1
            bcs         @get
            bra         @none
@fwd:
:
            jsr         eat
            inc         p2
            jsr         peek
            cmp         #'+'
            beq         :-
            dec         p2                                  ; p1 = ucount + them - 1
            clc
            lda         ucount
            adc         p2
            sta         p1
            lda         ucount + 1
            adc         #0
            sta         p1 + 1
            inc         ef                                  ; (Later in this pass)
            lda         pass
            cmp         #1
            bne         :+
            lda         #0                                  ; (Pass 1: not known yet)
            jmp         setv
:
            lda         p1 + 1                              ; (Past the last pass's: none such)
            cmp         utotal + 1
            bcc         @get
            bne         @none
            lda         p1
            cmp         utotal
            bcs         @none
@get:
            jsr         uaddr
            lda         (p1)
            sta         val
            ldy         #1
            lda         (p1),Y
            sta         val + 1
            stz         val + 2
            stz         val + 3
            clc
            rts
@none:
            LDR         r0, s_unnamed
            jsr         err
            sec
            rts

; p1 = unnamed label p1's place in its bank (selected)
uaddr:
            lda         ubank
            sta         BANKREG
            asl         p1
            rol         p1 + 1
            lda         p1 + 1
            and         #>(BANK_LEN - 1)
            ora         #>BANK_AT
            sta         p1 + 1
            rts

; An unnamed label here (:).  C = 1: too many (said)
un_def:
            lda         ucount
            sta         p1
            lda         ucount + 1
            sta         p1 + 1
            cmp         #>(BANK_LEN / 2)
            bcs         @full
            jsr         uaddr
            jsr         here_val
            lda         val
            sta         (p1)
            ldy         #1
            lda         val + 1
            sta         (p1),Y
            inc         ucount
            bne         :+
            inc         ucount + 1
:
            clc
            rts
@full:
            LDR         r0, s_unfull
            jsr         err
            sec
            rts

; ---- Functions: .strlen ( ), .defined ( ) ...

func:
            jsr         getid
            LDR         p2, funcs
            jsr         tfind
            bcc         @unknown
            jmp         (p2)
@unknown:
            LDR         r0, s_unsup
            jsr         errtok
            sec
            rts

; The ( after a function's name, and the ) after its argument.  C = 1: none (said)
fopen:
            lda         #'('
            bra         :+
fclose:
            lda         #')'
:
            jsr         expect
            bcc         :+
            jmp         e_bad
:
            rts

f_strlen:
            jsr         fopen
            bcs         @done
            jsr         getstr
            bcs         @done
            lda         slen
            jsr         setv
            jsr         fclose
@done:
            rts

f_defined:
            jsr         fopen
            bcs         @done
            jsr         getid
            bcc         :+
            jmp         e_bad
:
            jsr         defined
            jsr         setbool
            jsr         fclose
@done:
            rts

; .A <> 0: the name in tok is defined in this pass so far (or a macro)
defined:
            jsr         sym_find
            bcs         @no
            ldy         #SY_FLAGS
            lda         (hp),Y
            and         #SF_MACRO
            bne         @yes
            lda         (hp),Y
            and         #SF_PASS
            cmp         pass
            beq         @yes
@no:
            lda         #0
            rts
@yes:
            lda         #1
            rts

f_blank:
            jsr         fopen
            bcs         @done
            jsr         blank
            jsr         setbool
            bne         @close
            ldx         #0                                  ; (Not blank: its tokens passed by, to the ) at the top)
@skip:
            jsr         peek
            beq         @close
            cmp         #'('
            bne         :+
            inx
:
            cmp         #')'
            bne         @next
            dex
            bmi         @close
@next:
            jsr         eat
            bra         @skip
@close:
            jsr         fclose
@done:
            rts

; .A <> 0: blank: { } with nothing in it, or nothing before the ) (li past the { }, or at the ))
blank:
            jsr         skipws
            jsr         peek
            cmp         #'{'
            bne         @plain
            jsr         eat
            jsr         skipws
            jsr         peek
            cmp         #'}'
            php
:
            jsr         peek                                ; (Past the })
            beq         :+
            jsr         eat
            cmp         #'}'
            bne         :-
:
            plp
            bra         @answer
@plain:
            cmp         #')'
@answer:
            beq         :+
            lda         #0
            rts
:
            lda         #1
            rts

; .match (a, b): 1 if a's tokens are the same kinds as b's
f_match:
            jsr         fopen
            bcs         @done
            ldx         #0
            jsr         kinds
            stx         kn1
            lda         #','
            jsr         expect
            bcs         @bad
            ldx         #16
            jsr         kinds
            txa
            sec
            sbc         #16
            sta         kn2
            lda         #0
            ldx         kn1
            cpx         kn2
            bne         @answer
@cmp:
            dex
            bmi         @same
            lda         kbuf1,X
            cmp         kbuf2,X
            beq         @cmp
            lda         #0
            bra         @answer
@same:
            lda         #1
@answer:
            jsr         setbool
            jsr         fclose
@done:
            rts
@bad:
            jmp         e_bad

; One argument's tokens' kinds into kbuf1 + .X on (.X past them, 16 at most): { tokens }, or tokens to a , or )
kinds:
            jsr         skipws
            jsr         peek
            stz         p3 + 1                              ; (In { })
            cmp         #'{'
            bne         @tok
            jsr         eat
            inc         p3 + 1
@tok:
            jsr         skipws
            jsr         peek
            beq         @end
            ldy         p3 + 1
            bne         :+
            cmp         #','
            beq         @end
            cmp         #')'
            beq         @end
            bra         @kind
:
            cmp         #'}'
            bne         @kind
            jsr         eat
            bra         @end
@kind:
            cmp         #'"'
            bne         :+
            phx
            jsr         getstr
            plx
            lda         #1
            bra         @put
:
            cmp         #'$'
            beq         @num
            cmp         #'%'
            beq         @num
            cmp         #'0'
            bcc         :+
            cmp         #'9' + 1
            bcs         :+
@num:
            phx
            jsr         getnum
            plx
            lda         #2
            bra         @put
:
            phx
            jsr         getid
            plx
            bcs         :+
            lda         #3
            bra         @put
:
            jsr         peek                                ; (Anything else: itself)
            pha
            jsr         eat
            pla
@put:
            cpx         #32
            bcs         @tok
            sta         kbuf1,X
            inx
            bra         @tok
@end:
            rts

f_lobyte:
            lda         #OP_LO
            bra         fbyte
f_hibyte:
            lda         #OP_HI
            bra         fbyte
f_bank:
            lda         #OP_BANK
            bra         fbyte
f_loword:
            lda         #OP_LOW
            bra         fbyte
f_hiword:
            lda         #OP_HIW
fbyte:
            pha
            jsr         fopen
            bcs         :+
            jsr         e_or
            bcs         :+
            jsr         fclose
            bcs         :+
            pla
            jsr         unop
            clc
            rts
:
            pla
            sec
            rts

; ---- Operators

OP_LOR          = 1
OP_LAND         = 2
OP_LXOR         = 3
OP_EQ           = 4
OP_NE           = 5
OP_LT           = 6
OP_GT           = 7
OP_LE           = 8
OP_GE           = 9
OP_ADD          = 10
OP_SUB          = 11
OP_OR           = 12
OP_MUL          = 13
OP_DIV          = 14
OP_MOD          = 15
OP_AND          = 16
OP_XOR          = 17
OP_SHL          = 18
OP_SHR          = 19
OP_NEG          = 20
OP_PLUS         = 21
OP_NOT          = 22
OP_LO           = 23
OP_HI           = 24
OP_BANK         = 25
OP_LNOT         = 26
OP_LOW          = 27
OP_HIW          = 28

; The operator at li from the table at optab + .X (each: its length, its bytes, its code; 0 after the last), li
; past it.  A .name needs a non-name byte after it; | and & not a second one.  OUT: C = 0, .A = its code; C = 1:
; none (li past blanks)
opm:
            jsr         skipws
            ldy         li                                  ; (Not a byte that starts one: none, at once)
            lda         line,Y
            bmi         @none
            tay
            lda         ctype,Y                             ; (CT_OP)
            lsr         a
            bcc         @none
@entry:
            lda         optab,X
            beq         @none
            sta         p3                                  ; (Its bytes to match)
            ldy         li
            stx         p3 + 1                              ; (Where it starts)
@byte:
            lda         line,Y
            jsr         lower
            cmp         optab + 1,X
            bne         @next
            iny
            inx
            dec         p3
            bne         @byte
            ldx         p3 + 1                              ; All of it: what's after it
            lda         optab + 1,X
            cmp         #'.'
            bne         :+
            lda         line,Y
            jsr         isname
            bcs         @next
:
            lda         optab,X                             ; (A single | or &: not doubled)
            cmp         #1
            bne         @ok
            lda         optab + 1,X
            cmp         #'|'
            beq         :+
            cmp         #'&'
            bne         @ok
:
            cmp         line,Y
            beq         @next
@ok:
            sty         li
            lda         optab,X                             ; (Its code: after its bytes)
            clc
            adc         p3 + 1
            tax
            lda         optab + 1,X
            clc
            rts
@next:
            ldx         p3 + 1                              ; Past it: its length, bytes and code
            lda         optab,X
            clc
            adc         #2
            adc         p3 + 1
            tax
            bra         @entry
@none:
            sec
            rts

; bools: lhs and val each made 0 or 1 (for the logical operators)
bools:
            lda         lhs
            ora         lhs + 1
            ora         lhs + 2
            ora         lhs + 3
            beq         :+
            lda         #1
:
            sta         lhs
            lda         val
            ora         val + 1
            ora         val + 2
            ora         val + 3
            beq         :+
            lda         #1
:
            sta         val
            rts

; val = .A <> 0 (1 or 0)
setbool:
            cmp         #0
            beq         setv
            lda         #1
            ; (on into setv)

; val = .A
setv:
            sta         val
            stz         val + 1
            stz         val + 2
            stz         val + 3
            clc
            rts

; val = .A = 0 (1 or 0)
nbool:
            cmp         #0
            beq         :+
            lda         #0
            jmp         setv
:
            lda         #1
            jmp         setv

; val = lhs .A val: = <> < > <= >= (signed), 1 or 0
compare:
            pha
            ldx         #0                                  ; t32 = lhs - val
            ldy         #4
            sec
:
            lda         lhs,X
            sbc         val,X
            sta         t32,X
            inx
            dey
            bne         :-
            bvc         :+                                  ; (Less, signed: its high byte's N xor V)
            eor         #$80
:
            and         #$80
            sta         sign
            lda         t32
            ora         t32 + 1
            ora         t32 + 2
            ora         t32 + 3
            sta         p3                                  ; (0: equal)
            pla
            cmp         #OP_EQ
            bne         :+
            lda         p3
            jmp         nbool
:
            cmp         #OP_NE
            bne         :+
            lda         p3
            jmp         setbool
:
            cmp         #OP_LT
            bne         :+
            lda         sign
            jmp         setbool
:
            cmp         #OP_GE
            bne         :+
            lda         sign
            jmp         nbool
:
            cmp         #OP_GT                              ; (Greater: not less, not equal)
            bne         :+
            lda         sign
            bne         @no
            lda         p3
            jmp         setbool
:
            lda         sign                                ; (<=: less, or equal)
            bne         @yes
            lda         p3
            jmp         nbool
@yes:
            lda         #1
            jmp         setv
@no:
            lda         #0
            jmp         setv

; val = lhs .A val: + - | * / .mod & ^ << >>.  C = 1: division by 0 (said)
binop:
            cmp         #OP_ADD
            bne         @sub
            clc
            ldx         #0
:
            lda         lhs,X
            adc         val,X
            sta         val,X
            inx
            txa
            eor         #4
            bne         :-
            clc
            rts
@sub:
            cmp         #OP_SUB
            bne         @or
            sec
            ldx         #0
:
            lda         lhs,X
            sbc         val,X
            sta         val,X
            inx
            txa
            eor         #4
            bne         :-
            clc
            rts
@or:
            cmp         #OP_OR
            bne         @and
            ldx         #3
:
            lda         lhs,X
            ora         val,X
            sta         val,X
            dex
            bpl         :-
            clc
            rts
@and:
            cmp         #OP_AND
            bne         @xor
            ldx         #3
:
            lda         lhs,X
            and         val,X
            sta         val,X
            dex
            bpl         :-
            clc
            rts
@xor:
            cmp         #OP_XOR
            bne         @shl
            ldx         #3
:
            lda         lhs,X
            eor         val,X
            sta         val,X
            dex
            bpl         :-
            clc
            rts
@shl:
            cmp         #OP_SHL
            bne         @shr
            jsr         count
            jsr         swapv
            txa
            jsr         shlv
            clc
            rts
@shr:
            cmp         #OP_SHR
            bne         @mul
            jsr         count
            jsr         swapv
:
            dex
            bmi         :+
            lsr         val + 3
            ror         val + 2
            ror         val + 1
            ror         val
            bra         :-
:
            clc
            rts
@mul:
            cmp         #OP_MUL
            bne         @div
            jmp         mul
@div:
            jmp         divmod

; .X = a shift's count (val's low byte, 32 at most)
count:
            ldx         val
            lda         val + 1
            ora         val + 2
            ora         val + 3
            bne         :+
            cpx         #33
            bcc         :++
:
            ldx         #32
:
            rts

; val <-> lhs.  Keeps .X
swapv:
            phx
            ldx         #3
:
            lda         val,X
            ldy         lhs,X
            sta         lhs,X
            tya
            sta         val,X
            dex
            bpl         :-
            plx
            rts

; val = lhs * val (its low 32 bits)
mul:
            ldx         #3                                  ; t32 = val; val = 0
:
            lda         val,X
            sta         t32,X
            stz         val,X
            dex
            bpl         :-
            ldy         #32
@bit:
            asl         val                                 ; val * 2 ...
            rol         val + 1
            rol         val + 2
            rol         val + 3
            asl         t32                                 ;   + lhs, for each bit of t32, its highest first
            rol         t32 + 1
            rol         t32 + 2
            rol         t32 + 3
            bcc         :++
            clc
            ldx         #0
:
            lda         val,X
            adc         lhs,X
            sta         val,X
            inx
            txa
            eor         #4
            bne         :-
:
            dey
            bne         @bit
            clc
            rts

; val = lhs / val, or lhs .mod val (.A): signed, the quotient towards 0, the remainder lhs's sign.  C = 1: by 0
; (said; not while a name in it isn't known yet: 0)
divmod:
            pha
            lda         val
            ora         val + 1
            ora         val + 2
            ora         val + 3
            bne         @go
            pla
            lda         eu
            ora         ef
            beq         :+
            lda         #0
            jmp         setv
:
            LDR         r0, s_div0
            jsr         err
            sec
            rts
@go:
            lda         lhs + 3                             ; sign: the quotient's (bit 7), the remainder's (bit 6)
            and         #$80
            sta         sign
            lsr         a
            ora         sign
            sta         sign
            lda         lhs + 3
            bpl         :+
            jsr         negl
:
            lda         val + 3
            bpl         :+
            lda         sign
            eor         #$80
            sta         sign
            jsr         negv
:
            ldx         #3                                  ; Unsigned: t32 the remainder, lhs the quotient
:
            stz         t32,X
            dex
            bpl         :-
            ldy         #32
@bit:
            asl         lhs
            rol         lhs + 1
            rol         lhs + 2
            rol         lhs + 3
            rol         t32
            rol         t32 + 1
            rol         t32 + 2
            rol         t32 + 3
            sec                                             ; t32 >= val: taken away, and a 1
            ldx         #0
:
            lda         t32,X
            sbc         val,X
            pha
            inx
            txa
            eor         #4
            bne         :-
            bcc         @restore
            ldx         #3
:
            pla
            sta         t32,X
            dex
            bpl         :-
            inc         lhs
            bra         @nextbit
@restore:
            pla
            pla
            pla
            pla
@nextbit:
            dey
            bne         @bit
            pla
            cmp         #OP_MOD
            beq         @mod
            ldx         #3
:
            lda         lhs,X
            sta         val,X
            dex
            bpl         :-
            bit         sign
            bpl         :+
            jsr         negv
:
            clc
            rts
@mod:
            ldx         #3
:
            lda         t32,X
            sta         val,X
            dex
            bpl         :-
            bit         sign
            bvc         :+
            jsr         negv
:
            clc
            rts

; val = -val; lhs = -lhs
negv:
            ldx         #0
            sec
:
            lda         #0
            sbc         val,X
            sta         val,X
            inx
            txa
            eor         #4
            bne         :-
            rts

negl:
            ldx         #0
            sec
:
            lda         #0
            sbc         lhs,X
            sta         lhs,X
            inx
            txa
            eor         #4
            bne         :-
            rts

; val = .A val: - + ~ < > ^ ! .loword .hiword
unop:
            cmp         #OP_NEG
            bne         :+
            jmp         negv
:
            cmp         #OP_NOT
            bne         @lo
            ldx         #3
:
            lda         val,X
            eor         #$FF
            sta         val,X
            dex
            bpl         :-
            rts
@lo:
            cmp         #OP_LO
            bne         @hi
            lda         val
            jmp         setv
@hi:
            cmp         #OP_HI
            bne         @bank
            lda         val + 1
            jmp         setv
@bank:
            cmp         #OP_BANK
            bne         @lnot
            lda         val + 2
            jmp         setv
@lnot:
            cmp         #OP_LNOT
            bne         @low
            lda         val
            ora         val + 1
            ora         val + 2
            ora         val + 3
            jmp         nbool
@low:
            cmp         #OP_LOW
            bne         @hiw
            stz         val + 2
            stz         val + 3
            rts
@hiw:
            cmp         #OP_HIW
            bne         @plus
            lda         val + 2
            sta         val
            lda         val + 3
            sta         val + 1
            stz         val + 2
            stz         val + 3
@plus:
            rts

; C = 0: val is an address on the zero page, known in this pass so far (so every pass agrees)
val_zp:
            lda         eu
            ora         ef
            ora         val + 1
            ora         val + 2
            ora         val + 3
            bne         :+
            clc
            rts
:
            sec
            rts

; C = 0: val known (no name in it undefined, nor defined only later).  Else C = 1, said
number_ok:
            lda         eu
            ora         ef
            bne         :+
            clc
            rts
:
            LDR         r0, s_notyet
            jsr         err
            sec
            rts

; ---- The heap

; Its first bank, and the unnamed labels'; the symbol table empty.  C = 1: no bank
heap_init:
            stz         hbanks
            jsr         newbank
            bcs         @done
            stz         hidx
            lda         #1                                  ; (Ref 0 is none)
            sta         hoff
            stz         hoff + 1
            lda         #1
            jsr         BANKS_ALLOC                         ; The unnamed labels'
            bcs         @done
            sta         ubank
            inc         uhave
            stz         utotal
            stz         utotal + 1
            ldx         #0
:
            stz         headl,X
            stz         headh,X
            inx
            bne         :-
            clc
@done:
            rts

; The heap's banks, and the unnamed labels', given back
heap_free:
            ldx         #0
:
            cpx         hbanks
            bcs         :+
            phx
            lda         hbank,X
            ldx         #1
            jsr         BANKS_FREE
            plx
            inx
            bra         :-
:
            stz         hbanks
            lda         uhave
            beq         :+
            lda         ubank
            ldx         #1
            jsr         BANKS_FREE
            stz         uhave
:
            rts

; Another bank for the heap.  C = 1: none
newbank:
            ldx         hbanks
            cpx         #HEAP_BANKS
            bcs         :+
            lda         #1
            jsr         BANKS_ALLOC
            bcs         :+
            ldx         hbanks
            sta         hbank,X
            inc         hbanks
            clc
:
            rts

; .A bytes from the heap: hp at them (their bank selected), sref and .A/.X = their ref.  C = 1: no room (said)
halloc:
            sta         p3
            clc                                             ; Room in this bank?
            adc         hoff
            lda         hoff + 1
            adc         #0
            cmp         #>BANK_LEN
            bcc         @here
            ldx         hidx                                ; The next bank
            inx
            cpx         hbanks
            bcc         :+
            phx
            jsr         newbank
            plx
            bcs         @full
:
            stx         hidx
            stz         hoff
            stz         hoff + 1
@here:
            lda         hidx                                ; ref = its index << 13 | hoff
            asl         a
            asl         a
            asl         a
            asl         a
            asl         a
            ora         hoff + 1
            sta         sref + 1
            lda         hoff
            sta         sref
            clc                                             ; hoff += .A
            adc         p3
            sta         hoff
            bcc         :+
            inc         hoff + 1
:
            lda         sref
            ldx         sref + 1
            jmp         hsel
@full:
            LDR         r0, s_heap
            jsr         err
            sec
            rts

; hp = the heap's ref .A/.X, its bank selected.  Keeps .A, .X, .Y.  C = 0
hsel:
            pha
            phx
            phy
            sta         hp
            txa
            lsr         a
            lsr         a
            lsr         a
            lsr         a
            lsr         a
            tay
            lda         hbank,Y
            sta         BANKREG
            txa
            and         #>(BANK_LEN - 1)
            ora         #>BANK_AT
            sta         hp + 1
            ply
            plx
            pla
            clc
            rts

; ---- The symbol table

; The symbol named tok (tlen): C = 0, hp at it (sref: its ref); C = 1: none (hash: its chain's)
sym_find:
            lda         #0                                  ; The hash
            ldx         #0
:
            asl         a
            adc         #0
            eor         tok,X
            inx
            cpx         tlen
            bne         :-
            sta         hash
            tax
            lda         headl,X
            ldy         headh,X
@chain:
            sta         sref
            sty         sref + 1
            ora         sref + 1
            beq         @none
            lda         sref
            ldx         sref + 1
            jsr         hsel
            ldy         #SY_LEN
            lda         (hp),Y
            cmp         tlen
            bne         @next
            ldx         #0
            ldy         #SY_NAME
:
            lda         (hp),Y
            cmp         tok,X
            bne         @next
            iny
            inx
            cpx         tlen
            bne         :-
            clc
            rts
@next:
            ldy         #SY_NEXT
            lda         (hp),Y
            pha
            iny
            lda         (hp),Y
            tay
            pla
            bra         @chain
@none:
            sec
            rts

; The symbol named tok: hp at it (found, or made: undefined, in its hash's chain).  C = 1: no room (said)
sym_def:
            jsr         sym_find
            bcs         :+
            rts
:
            lda         tlen
            clc
            adc         #SY_NAME
            jsr         halloc
            bcs         @done
            ldx         hash
            ldy         #SY_NEXT
            lda         headl,X
            sta         (hp),Y
            iny
            lda         headh,X
            sta         (hp),Y
            lda         sref
            sta         headl,X
            lda         sref + 1
            sta         headh,X
            lda         #0
            ldy         #SY_VAL
:
            sta         (hp),Y
            iny
            cpy         #SY_SEG
            bne         :-
            lda         #$FF
            sta         (hp),Y
            iny
            lda         tlen
            sta         (hp),Y
            ldx         #0
            ldy         #SY_NAME
:
            lda         tok,X
            sta         (hp),Y
            iny
            inx
            cpx         tlen
            bne         :-
            clc
@done:
            rts

; The symbol at hp = val, defined in this pass; its flags | .A, its segment .X
sym_set:
            sta         p3
            stx         p3 + 1
            ldy         #SY_VAL
:
            lda         val - SY_VAL,Y
            sta         (hp),Y
            iny
            cpy         #SY_VAL + 4
            bne         :-
            ldy         #SY_FLAGS
            lda         (hp),Y
            and         #$FF ^ SF_PASS
            ora         pass
            ora         p3
            sta         (hp),Y
            iny
            lda         p3 + 1
            sta         (hp),Y
            rts

; One of as's own symbols (__DATA_LOAD__, HYX2_RAM ...) or its caller's (DEFINE's): the name at r0 = val, defined,
; unless the source has defined it itself (a label of that name: redef).  C = 1: no room
sym_link:
            ldy         #0
:
            lda         (r0),Y
            sta         tok,Y
            beq         :+
            iny
            bra         :-
:
            sty         tlen
            jsr         sym_def
            bcs         :+
            ldy         #SY_FLAGS                           ; (The source's own already, a pass before: kept)
            lda         (hp),Y
            bit         #SF_LINK
            bne         @set
            and         #SF_PASS | SF_MACRO
            bne         @kept
@set:
            lda         #SF_LINK
            ldx         #$FF
            jsr         sym_set
@kept:
            clc
:
            rts

; .A/.X = the value (its low word) of the symbol at hp
sym_get:
            ldy         #SY_VAL
            lda         (hp),Y
            pha
            iny
            lda         (hp),Y
            tax
            pla
            rts

; The routine at .A/.X for each symbol, hp at it (its bank selected)
sym_each:
            sta         eachv
            stx         eachv + 1
            stz         eachb
@bucket:
            ldx         eachb
            lda         headl,X
            ldy         headh,X
@chain:
            sta         p2
            sty         p2 + 1
            ora         p2 + 1
            beq         @next
            lda         p2
            ldx         p2 + 1
            jsr         hsel
            jsr         @call
            lda         p2                                  ; (Its bank again: the routine may change it)
            ldx         p2 + 1
            jsr         hsel
            ldy         #SY_NEXT
            lda         (hp),Y
            pha
            iny
            lda         (hp),Y
            tay
            pla
            bra         @chain
@next:
            inc         eachb
            bne         @bucket
            rts
@call:
            jmp         (eachv)

; Each pass's start: no cheap local scope yet, no unnamed labels (the last pass's count kept for :+)
sym_init:
            stz         scope
            stz         scope + 1
            lda         ucount
            ora         ucount + 1
            beq         :+
            lda         ucount
            sta         utotal
            lda         ucount + 1
            sta         utotal + 1
:
            stz         ucount
            stz         ucount + 1
            rts

.rodata
optab:
ops_or:
            .byte       2, "||", OP_LOR
            .byte       3, ".or", OP_LOR
            .byte       0
ops_and:
            .byte       2, "&&", OP_LAND
            .byte       4, ".and", OP_LAND
            .byte       4, ".xor", OP_LXOR
            .byte       0
ops_cmp:
            .byte       2, "<>", OP_NE
            .byte       2, "<=", OP_LE
            .byte       2, ">=", OP_GE
            .byte       1, "=", OP_EQ
            .byte       1, "<", OP_LT
            .byte       1, ">", OP_GT
            .byte       0
ops_add:
            .byte       1, "+", OP_ADD
            .byte       1, "-", OP_SUB
            .byte       1, "|", OP_OR
            .byte       6, ".bitor", OP_OR
            .byte       0
ops_mul:
            .byte       1, "*", OP_MUL
            .byte       1, "/", OP_DIV
            .byte       4, ".mod", OP_MOD
            .byte       1, "&", OP_AND
            .byte       1, "^", OP_XOR
            .byte       2, "<<", OP_SHL
            .byte       2, ">>", OP_SHR
            .byte       7, ".bitand", OP_AND
            .byte       7, ".bitxor", OP_XOR
            .byte       4, ".shl", OP_SHL
            .byte       4, ".shr", OP_SHR
            .byte       0
ops_un:
            .byte       1, "-", OP_NEG
            .byte       1, "+", OP_PLUS
            .byte       1, "~", OP_NOT
            .byte       1, "<", OP_LO
            .byte       1, ">", OP_HI
            .byte       1, "^", OP_BANK
            .byte       1, "!", OP_LNOT
            .byte       7, ".bitnot", OP_NOT
            .byte       4, ".not", OP_LNOT
            .byte       0
.assert     * - optab < 256, error, "opm's tables in 256 bytes"

; Each byte to $7F: what it may be in a name or an expression (CT_*)
CT_FIRST        = $80           ; A name's first byte (a letter, _, @, .) ...
CT_NAME         = $40           ;   or one after it (a letter, a digit, _)
CT_OP           = $01           ; An operator's first
ctype:
.repeat 128, I
            .byte       ((I >= 'A' .and I <= 'Z') .or (I >= 'a' .and I <= 'z') .or I = '_') * (CT_FIRST | CT_NAME) | (I >= '0' .and I <= '9') * CT_NAME | (I = '@' .or I = '.') * CT_FIRST | (I = '|' .or I = '&' .or I = '<' .or I = '>' .or I = '=' .or I = '+' .or I = '-' .or I = '*' .or I = '/' .or I = '^' .or I = '~' .or I = '!' .or I = '.') * CT_OP
.endrepeat

funcs:
            .word       s_f_strlen, f_strlen
            .word       s_f_defined, f_defined
            .word       s_f_def, f_defined
            .word       s_f_blank, f_blank
            .word       s_f_match, f_match
            .word       s_f_lobyte, f_lobyte
            .word       s_f_hibyte, f_hibyte
            .word       s_f_bank, f_bank
            .word       s_f_loword, f_loword
            .word       s_f_hiword, f_hiword
            .word       0, 0
s_f_strlen: .byte       ".strlen", 0
s_f_defined: .byte      ".defined", 0
s_f_def:    .byte       ".def", 0
s_f_blank:  .byte       ".blank", 0
s_f_match:  .byte       ".match", 0
s_f_lobyte: .byte       ".lobyte", 0
s_f_hibyte: .byte       ".hibyte", 0
s_f_bank:   .byte       ".bankbyte", 0
s_f_loword: .byte       ".loword", 0
s_f_hiword: .byte       ".hiword", 0
s_long:     .byte       "a name of more than 32 characters", 0
s_quote:    .byte       "a string with no closing quote", 0
s_number:   .byte       "a bad number", 0
s_expr:     .byte       "a bad expression", 0
s_undef:    .byte       "undefined", 0
s_macro:    .byte       "a macro in an expression", 0
s_unnamed:  .byte       "no such unnamed label", 0
s_unfull:   .byte       "too many unnamed labels", 0
s_unsup:    .byte       "a function as doesn't know", 0
s_div0:     .byte       "division by 0", 0
s_notyet:   .byte       "a value not known yet (a name defined after it)", 0
s_heap:     .byte       "out of memory (the heap's banks)", 0
