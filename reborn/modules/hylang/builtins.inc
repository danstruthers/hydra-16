; ****************************************************************************
; builtins.inc - hylang's built-ins (step 7.2), in the module's second bank: each a constant, BUILTIN0 + 2n (pages 3
; and 4: no object at all), whose name is a symbol with it as its global value.  bi_table has each one's name, its
; flags (BF_*) and its code, in order; builtins_init interns the names, and copies the rest to bi_flags, bi_lo and
; bi_hi.  The first, BI_RUN, has no name: each's and dotimes's bodies are run by it (eval.inc).
;   A built-in's code (but a special form's: eval.inc) is called with its arguments' values in ev_a (a chain), or
; as they're written (BF_RAW), and ev_e the caller's scope; it returns ev_v, its value (an error, if it fails).  It
; may change the registers, but not the stack; what it makes must be held in the registers (roots) while it makes
; more, and on the stack while it evaluates (eval_nested).

BI_RUN          = BUILTIN0      ; The first built-in: (run expr ...)

.zeropage
bi_n:       .res        2                                   ; Numbers: a size (a quotient) ...
bi_m:       .res        2                                   ;   another (a divisor) ...
bi_t:       .res        2                                   ;   one unpacked ...
bi_p:       .res        4                                   ;   a product (a remainder) ...
bi_s:       .res        1                                   ;   and a sign (its top bit)
bi_q:       .res        2                                   ; Text: where it's copied to ...
bi_src:     .res        2                                   ;   and from
eq_a        = bi_n                                          ; (equal's: the two values compared ...
eq_b        = bi_m
eq_t        = bi_s                                          ;   a type ...
eq_sp       = bi_p                                          ;   the stack's top as it began, and its bank ...
eq_bank     = bi_t
eq_n        = bi_p + 2                                      ;   and the pairs on it)

.bss
bi_flags:   .res        256                                 ; Each built-in's flags (BF_*) ...
bi_lo:      .res        256                                 ;   and its code
bi_hi:      .res        256
str_a:      .res        STR_MAX + 2                         ; Two texts, copied (text_copy) ...
str_b:      .res        STR_MAX + 2
str_an:     .res        1                                   ;   and their lengths
str_bn:     .res        1
bi_fd:      .res        1                                   ; load's file
ld_n:       .res        2                                   ; load's text: its length in the buffer ...
ld_lim:     .res        2                                   ;   how much of it's read now ...
ld_from:    .res        2                                   ;   where an expression begins ...
ld_eof:     .res        1                                   ;   and not 0 at the file's end
.segment "CODE2"

; A built-in in bi_table: its name, its flags, its code
.macro BUILTIN name, flags, code
            .byte       .strlen(name), name, flags
            .addr       code
.endmacro

; The built-ins' names, each a symbol whose global value is the built-in; their flags and code, by number.  OUT:
; C = 1, no room
builtins_init:
            LDR         ev_j, bi_table
            stz         ev_i                                ; (Its number)
@next:
            lda         (ev_j)
            cmp         #$FF
            beq         @done
            tay                                             ; (Its name: a symbol, if it has one)
            beq         @named
            clc
            lda         ev_j
            adc         #1
            sta         r0
            lda         ev_j + 1
            adc         #0
            sta         r0 + 1
            lda         #OT_SYMBOL
            jsr         intern
            bcs         @no
            lda         ev_i                                ; (Its value: BUILTIN0 + 2n, page 3 or 4)
            asl
            sta         h_a
            lda         #>BUILTIN0
            adc         #0
            sta         h_a + 1
            jsr         sym_set
@named:
            lda         (ev_j)                              ; (Its flags and code)
            tay
            iny
            ldx         ev_i
            lda         (ev_j),y
            sta         bi_flags,x
            iny
            lda         (ev_j),y
            sta         bi_lo,x
            iny
            lda         (ev_j),y
            sta         bi_hi,x
            iny
            tya
            clc
            adc         ev_j
            sta         ev_j
            bcc         :+
            inc         ev_j + 1
:
            inc         ev_i
            bra         @next
@done:
            clc
@no:
            rts

; ---- Arguments, values, errors

; ev_v = the error whose message is the text at r0 (and rts, for a built-in's error)
bi_error_text:
            jmp         ev_error_text

; h_v, a fixnum: .A/.X its value (signed)
fx_int:
            lda         h_v + 1
            cmp         #$80
            ror
            tax
            lda         h_v
            ror
            rts

; ev_v = the fixnum .A/.X (-16384 to 16383, not checked)
int_fx:
            asl
            ora         #1
            sta         ev_v
            txa
            rol
            sta         ev_v + 1
            rts

; ev_v = the character .A
make_char:
            asl
            sta         ev_v
            lda         #>CHARS
            adc         #0
            sta         ev_v + 1
            rts

; h_v's text (a string's, a character's, or a symbol's or an atom's name) copied to bi_q: .A = its length.  OUT:
; C = 1, it has none
text_copy:
            jsr         type_of
            cmp         #TY_CHAR
            beq         @char
            cmp         #TY_STRING
            beq         @string
            cmp         #TY_SYMBOL
            beq         @name
            cmp         #TY_ATOM
            beq         @name
            sec
            rts
@char:
            lda         h_v + 1                             ; (Its code: (the value - $0100) / 2: page 2's
            lsr                                             ; the top half)
            lsr
            lda         h_v
            ror
            sta         (bi_q)
            lda         #1
            clc
            rts
@string:
            jsr         deref
            ldy         #1
            lda         (h_p),y
            ldx         #2
            bra         @copy
@name:
            jsr         deref
            lda         (h_p)
            cmp         #OT_SYMREF
            bne         :+
            ldy         #3                                  ; (A symbol's reference: its symbol)
            lda         (h_p),y
            tax
            dey
            lda         (h_p),y
            sta         h_v
            stx         h_v + 1
            jsr         deref
:
            ldy         #1
            lda         (h_p),y
            and         #$7F
            ldx         #SYM_NAME
@copy:
            pha                                             ; (.A bytes from h_p + .X)
            txa
            clc
            adc         h_p
            sta         bi_src
            lda         h_p + 1
            adc         #0
            sta         bi_src + 1
            ply
            phy
            beq         @copied
:
            dey
            lda         (bi_src),y
            sta         (bi_q),y
            tya
            bne         :-
@copied:
            pla
            clc
            rts

; h_v's text to str_a (text_a) or str_b (text_b): .A its length (str_an, str_bn).  OUT: C = 1, it has none
text_a:
            LDR         bi_q, str_a
            jsr         text_copy
            sta         str_an
            rts

text_b:
            LDR         bi_q, str_b
            jsr         text_copy
            sta         str_bn
            rts

; str_a against str_b, byte by byte (the shorter first, if one begins the other): .A = $FF, 0 or 1 (less, the same,
; more)
text_cmp:
            ldy         #0
@byte:
            cpy         str_an
            beq         @a_end
            cpy         str_bn
            beq         @more
            lda         str_a,y
            cmp         str_b,y
            bne         @differ
            iny
            bra         @byte
@a_end:
            cpy         str_bn
            beq         @same
@less:
            lda         #$FF
            rts
@differ:
            bcc         @less
@more:
            lda         #1
            rts
@same:
            lda         #0
            rts

; h_v as print shows it, a string: h_v.  OUT: C = 1, h_v an error (too long, or no room)
display_string:
            jsr         cap_start
            jsr         print_display
            jmp         cap_string

; The text of the string h_v added to the message
msg_str:
            LDR         bi_q, str_a
            jsr         text_copy
            bcs         @done
            tax
            ldy         #0
:
            cpx         #0
            beq         @done
            lda         str_a,y
            jsr         msg_char
            iny
            dex
            bra         :-
@done:
            rts

; ---- Functions

; (fn {formals} {body}), (fexpr {formals} {body}): a function, its closure this scope (an fexpr's arguments come
; as they're written, each with the caller's scope)
bi_fn:
            lda         #0
            bra         :+
bi_fexpr:
            lda         #FN_FEXPR
:
            sta         ev_i
            jsr         count_a
            cmp         #2
            bne         @wrong
            jsr         arg_next                            ; (Its formals: ev_r)
            MOVR        ev_r, h_v
            jsr         arg_next                            ; (Its body: ev_s)
            MOVR        ev_s, h_v
            jsr         make_fn
            bcc         :+
@wrong:
            LDR         r0, bs_fn
            jmp         bi_error_text
:
            rts

; A function: its formals ev_r (a Q-expression of symbols), its body ev_s (a Q-expression), its flags ev_i, its
; closure ev_e.  OUT: C = 0, ev_v = it (or no room: the error); C = 1, the formals or the body aren't right
make_fn:
            MOVR        h_v, ev_r
            jsr         type_of
            cmp         #TY_LIST
            bne         @wrong
            MOVR        h_v, ev_s
            jsr         type_of
            cmp         #TY_LIST
            bne         @wrong
            MOVR        h_v, ev_r
            jsr         list_of
            MOVR        ev_r, h_v
            MOVR        ev_t, h_v
@formal:
            lda         ev_t + 1                            ; (Each a symbol, now one that's bound in frames)
            beq         @made
            MOVR        h_v, ev_t
            jsr         car
            jsr         type_of
            cmp         #TY_SYMBOL
            bne         @wrong
            jsr         sym_local
            MOVR        h_v, ev_t
            jsr         cdr
            MOVR        ev_t, h_v
            bra         @formal
@wrong:
            sec
            rts
@made:
            MOVR        h_v, ev_s
            jsr         list_of
            MOVR        ev_s, h_v
            ldy         #PK_C16
            jsr         alloc
            bcc         :+
            MOVR        ev_v, err_room
            clc
            rts
:
            lda         #OT_FUNC
            sta         (h_p)
            ldy         #1
            lda         ev_i
            sta         (h_p),y
            iny
            lda         ev_r
            sta         (h_p),y
            iny
            lda         ev_r + 1
            sta         (h_p),y
            iny
            lda         ev_s
            sta         (h_p),y
            iny
            lda         ev_s + 1
            sta         (h_p),y
            iny
            lda         ev_e
            sta         (h_p),y
            iny
            lda         ev_e + 1
            sta         (h_p),y
            iny
            lda         #0
            sta         (h_p),y
            iny
            sta         (h_p),y
            MOVR        ev_v, h_v
            clc
            rts

; (fun {name formals ...} {body}): name defined (globally, as def does) as (fn {formals ...} {body}), made here
bi_fun:
            jsr         count_a
            cmp         #2
            jne         @wrong
            jsr         arg_next                            ; (Its name, and the formals: a Q-expression of them)
            MOVR        ev_r, h_v
            jsr         type_of
            cmp         #TY_LIST
            bne         @wrong
            lda         ev_r + 1
            beq         @wrong
            jsr         arg_next
            MOVR        ev_s, h_v
            MOVR        h_v, ev_r
            jsr         list_of
            MOVR        ev_r, h_v
            jsr         car
            MOVR        ev_x, h_v
            jsr         type_of
            cmp         #TY_SYMBOL
            bne         @wrong
            MOVR        h_v, ev_r                           ; (The formals: a Q-expression of the rest)
            jsr         cdr
            lda         #OT_QEXPR
            jsr         make_list
            bcs         @room
            MOVR        ev_r, h_v
            stz         ev_i
            jsr         make_fn
            bcs         @wrong
            jsr         ev_is_error
            bcs         @done
            MOVR        h_v, ev_x
            MOVR        h_a, ev_v
            jsr         sym_set
            LDR         ev_v, NIL
@done:
            rts
@wrong:
            LDR         r0, bs_fun
            jmp         bi_error_text
@room:
            jmp         bi_no_room

; (defined? x): T if x (as it's written) is T, or a name with a value here (&1, &_ ...: in this call's own scopes);
; or a Q-expression of names that all have.  NIL if not, or there's no x
bi_defined:
            jsr         arg_next
            bcs         @no
            MOVR        ev_x, h_v
            jsr         is_defined
            bcs         @yes
            MOVR        h_v, ev_x
            jsr         type_of
            cmp         #TY_LIST
            bne         @no
            MOVR        h_v, ev_x
            jsr         list_of
            MOVR        ev_t, h_v
@each:
            lda         ev_t + 1
            beq         @yes
            MOVR        h_v, ev_t
            jsr         car
            MOVR        ev_x, h_v
            jsr         is_defined
            bcc         @no
            MOVR        h_v, ev_t
            jsr         cdr
            MOVR        ev_t, h_v
            bra         @each
@yes:
            sec
            jmp         bi_bool
@no:
            clc
            jmp         bi_bool

; C = 1 if ev_x is T, or a symbol with a value seen from ev_e: &1, &_ ... only in the scopes of this call (up to its
; function's frame)
is_defined:
            lda         ev_x + 1
            bne         :+
            lda         ev_x
            cmp         #<T_VAL
            bne         @no
            sec
            rts
:
            MOVR        h_v, ev_x
            jsr         type_of
            cmp         #TY_SYMBOL
            bne         @no
            MOVR        ev_w, ev_e
            jsr         deref                               ; (& and a name: the call's own)
            ldy         #SYM_NAME
            lda         (h_p),y
            cmp         #'&'
            beq         @amp
@frame:
            lda         ev_w + 1                            ; ---- Any other: in a frame, or globally
            beq         @global
            jsr         find_binding
            bcc         @yes
            jsr         parent_w
            bra         @frame
@global:
            MOVR        h_v, ev_x
            jsr         sym_value
            lda         h_v + 1
            bne         @yes
            lda         h_v
            cmp         #<UNBOUND
            beq         @no
@yes:
            sec
            rts
@no:
            clc
            rts
@amp:
            lda         ev_w + 1                            ; ---- &...: up to the frame with &_
            beq         @no
            jsr         find_binding
            bcc         @yes
            MOVR        h_v, ev_w                           ; (A call's frame: the last)
            jsr         deref
            ldy         #1
            lda         (h_p),y
            and         #FR_CALL
            bne         @no
            jsr         parent_w
            bra         @amp

; ev_w = its frame's parent
parent_w:
            MOVR        h_v, ev_w
            jsr         deref
            ldy         #3
            lda         (h_p),y
            tax
            dey
            lda         (h_p),y
            sta         ev_w
            stx         ev_w + 1
            rts

; (expr? x): T (x is as it's written: an expression)
bi_expr:
            sec
            jmp         bi_bool

; ---- Lists

; (list x ...): a Q-expression of the values, its scope this one
bi_list:
            MOVR        h_v, ev_a
            lda         #OT_QEXPR
            jsr         make_list
            bcs         bi_no_room1
            MOVR        ev_v, h_v
            lda         h_v + 1                             ; (NIL: none)
            beq         :+
            jsr         deref
            ldy         #4
            lda         ev_e
            sta         (h_p),y
            iny
            lda         ev_e + 1
            sta         (h_p),y
:
            rts

bi_no_room1:
            jmp         bi_no_room

; (head list): a list of its first item; (tail list): a list of the rest; (init list): a list of all but its last;
; (end list): a list of its last.  Each keeps the list's scope
bi_head:
            LDR         r1, bs_head
            jsr         list_arg
            jcs         list_done
            MOVR        h_v, ev_r
            jsr         car
            bra         list_one

bi_tail:
            LDR         r1, bs_tail
            jsr         list_arg
            jcs         list_done
            MOVR        h_v, ev_r
            jsr         cdr
            jmp         list_like

bi_end:
            LDR         r1, bs_end
            jsr         list_arg
            jcs         list_done
            MOVR        h_v, ev_r
:
            MOVR        ev_t, h_v                           ; (Its last)
            jsr         cdr
            lda         h_v + 1
            bne         :-
            MOVR        h_v, ev_t
            jsr         car
list_one:
            MOVR        h_a, h_v
            LDR         h_d, NIL
            jsr         cons
            jcs         bi_no_room1
            jmp         list_like

bi_init:
            LDR         r1, bs_init
            jsr         list_arg
            bcs         list_done
            LDR         ev_a, NIL                           ; (A copy of all but the last)
            LDR         ev_t, NIL
@item:
            MOVR        h_v, ev_r
            jsr         cdr
            lda         h_v + 1
            beq         @done
            MOVR        ev_s, h_v
            MOVR        h_v, ev_r
            jsr         car
            MOVR        ev_v, h_v
            jsr         ev_append
            jcs         bi_no_room1
            MOVR        ev_r, ev_s
            bra         @item
@done:
            MOVR        h_v, ev_a
            jmp         list_like
list_done:
            rts

; A list argument, the first, not empty (r1: the built-in's name, quoted, for its errors): ev_x = it, ev_r its
; chain.  OUT: C = 1, it isn't one (ev_v an error)
list_arg:
            jsr         count_a
            cmp         #1
            bcc         @few
            jsr         arg_next
            MOVR        ev_x, h_v
            jsr         type_of
            cmp         #TY_LIST
            bne         @not
            lda         ev_x + 1
            beq         @empty
            MOVR        h_v, ev_x
            jsr         list_of
            MOVR        ev_r, h_v
            clc
            rts
@few:
            LDR         r0, bs_too_few
            jsr         msg_start
            jsr         msg_r1
            bra         @error
@not:
            jsr         msg_r1_start
            LDR         r0, bs_expects_list
            jsr         msg_text
            bra         @error
@empty:
            jsr         msg_r1_start
            LDR         r0, bs_empty
            jsr         msg_text
@error:
            jmp         msg_error

; The message begun with the text at r1; or the text at r1 added
msg_r1_start:
            stz         msg_n
msg_r1:
            MOVR        r0, r1
            jmp         msg_text

; ev_v = a Q-expression of the chain h_v, with ev_x's scope (NIL, if it's empty)
list_like:
            lda         #OT_QEXPR
            jsr         make_list
            jcs         bi_no_room1
            MOVR        ev_v, h_v
            lda         h_v + 1
            beq         @done
            MOVR        h_v, ev_x                           ; (The scope, through bi_t: the two may be in different
            jsr         deref                               ; banks)
            ldy         #4
            lda         (h_p),y
            sta         bi_t
            iny
            lda         (h_p),y
            sta         bi_t + 1
            MOVR        h_v, ev_v
            jsr         deref
            ldy         #4
            lda         bi_t
            sta         (h_p),y
            iny
            lda         bi_t + 1
            sta         (h_p),y
@done:
            rts

; (join list ...): the lists' items in one (its scope the first's that has one)
bi_join:
            jsr         count_a
            cmp         #1
            bcs         :+
            LDR         r0, bs_join_few
            jmp         bi_error_text
:
            LDR         ev_s, NIL                           ; (ev_s: the scope)
            LDR         ev_x, NIL                           ; (ev_x: the last list's chain)
            LDR         ev_t, NIL                           ; (ev_t/ev_w: a copy of those before it)
            LDR         ev_w, NIL
@list:
            jsr         arg_next
            jcs         @joined
            MOVR        ev_r, h_v
            jsr         type_of
            cmp         #TY_LIST
            beq         :+
            LDR         r0, bs_join_list
            jmp         bi_error_text
:
            lda         ev_r + 1
            beq         @list
            lda         ev_s + 1                            ; (Its scope, if there's none yet)
            bne         :+
            MOVR        h_v, ev_r
            jsr         deref
            ldy         #4
            lda         (h_p),y
            sta         ev_s
            iny
            lda         (h_p),y
            sta         ev_s + 1
:
            lda         ev_x + 1                            ; (The last one so far: copied)
            beq         @last
@copy:
            MOVR        h_v, ev_x
            jsr         car
            MOVR        ev_v, h_v
            ldx         #ev_t - ev_x
            jsr         ev_append_x
            bcs         @room
            MOVR        h_v, ev_x
            jsr         cdr
            MOVR        ev_x, h_v
            lda         h_v + 1
            bne         @copy
@last:
            MOVR        h_v, ev_r
            jsr         list_of
            MOVR        ev_x, h_v
            jmp         @list
@joined:
            lda         ev_t + 1                            ; (The copy, then the last's chain)
            bne         :+
            MOVR        h_v, ev_x
            bra         @make
:
            MOVR        h_a, ev_x
            MOVR        h_v, ev_w
            jsr         set_cdr
            MOVR        h_v, ev_t
@make:
            lda         #OT_QEXPR
            jsr         make_list
            bcs         @room
            MOVR        ev_v, h_v
            lda         h_v + 1
            beq         @done
            jsr         deref
            ldy         #4
            lda         ev_s
            sta         (h_p),y
            iny
            lda         ev_s + 1
            sta         (h_p),y
@done:
            rts
@room:
            jmp         bi_no_room

; (len x): a list's items, a string's characters
bi_len:
            jsr         count_a
            cmp         #1
            beq         :+
            LDR         r0, bs_len_one
            jmp         bi_error_text
:
            jsr         arg_next
            jsr         type_of
            cmp         #TY_LIST
            beq         @list
            cmp         #TY_STRING
            beq         @string
            LDR         r0, bs_len_type
            jmp         bi_error_text
@list:
            jsr         list_of
            jsr         count_h
            bra         :+
@string:
            jsr         deref
            ldy         #1
            lda         (h_p),y
:
            ldx         #0
            jmp         int_fx

; (item-at list n): its item n (0 on); NIL past its end
bi_item_at:
            jsr         count_a
            cmp         #2
            bcs         :+
            LDR         r0, bs_item_few
            jmp         bi_error_text
:
            jsr         arg_next
            MOVR        ev_x, h_v
            jsr         type_of
            cmp         #TY_LIST
            beq         :+
            LDR         r0, bs_item_list
            jmp         bi_error_text
:
            jsr         arg_next
            lda         h_v
            lsr
            bcs         :+
            LDR         r0, bs_item_num
            jmp         bi_error_text
:
            jsr         fx_int
            cpx         #0
            bne         @past                               ; (Negative, or past 255)
            sta         ev_c
            MOVR        h_v, ev_x
            jsr         list_of
            ldx         ev_c
@walk:
            lda         h_v + 1
            beq         @past
            cpx         #0
            beq         @found
            phx
            jsr         cdr
            plx
            dex
            bra         @walk
@found:
            jsr         car
            MOVR        ev_v, h_v
            rts
@past:
            LDR         ev_v, NIL
            rts

; (reverse x): a list's items, or a string's characters, the other way round
bi_reverse:
            jsr         count_a
            cmp         #1
            beq         :+
            LDR         r0, bs_reverse_one
            jmp         bi_error_text
:
            jsr         arg_next
            MOVR        ev_x, h_v
            jsr         type_of
            cmp         #TY_STRING
            beq         @string
            cmp         #TY_LIST
            beq         :+
            LDR         r0, bs_reverse_type
            jmp         bi_error_text
:
            MOVR        h_v, ev_x                           ; (A list: each item onto the front)
            jsr         list_of
            MOVR        ev_r, h_v
            LDR         ev_t, NIL
@item:
            lda         ev_r + 1
            beq         @done
            MOVR        h_v, ev_r
            jsr         car
            MOVR        h_a, h_v
            MOVR        h_d, ev_t
            jsr         cons
            bcs         @room
            MOVR        ev_t, h_v
            MOVR        h_v, ev_r
            jsr         cdr
            MOVR        ev_r, h_v
            bra         @item
@done:
            MOVR        h_v, ev_t
            jmp         list_like
@room:
            jmp         bi_no_room
@string:
            MOVR        h_v, ev_x                           ; (A string: str_a backwards, in str_b)
            jsr         text_a
            tax
            ldy         #0
:
            cpx         #0
            beq         :+
            dex
            lda         str_a,x
            sta         str_b,y
            iny
            bra         :-
:
            LDR         r0, str_b
            lda         str_an
            jsr         buf_string
            MOVR        ev_v, h_v
            rts

; (range n), (range from to), (range from to step): the numbers from 0 (or from) up to, not including, to (down to
; it, with a negative step); numbers of any kind
bi_range:
            jsr         count_a
            cmp         #1
            bcc         @count
            cmp         #4
            bcc         :+
@count:
            LDR         r0, bs_range_count
            jmp         bi_error_text
:
            sta         ev_i
            jsr         arg_next                            ; (ev_x: from, ev_s: to, ev_t: step)
            jsr         range_num
            jcs         @done
            LDR         ev_x, 1
            MOVR        ev_s, h_v
            LDR         ev_t, 3
            lda         ev_i
            cmp         #1
            beq         @go
            MOVR        ev_x, ev_s
            jsr         arg_next
            jsr         range_num
            jcs         @done
            MOVR        ev_s, h_v
            lda         ev_i
            cmp         #3
            bne         @go
            jsr         arg_next
            jsr         range_num
            jcs         @done
            MOVR        ev_t, h_v
@go:
            LDR         ev_a, NIL                           ; (ev_a: the list, ev_w its last: range_add)
            LDR         ev_w, NIL
            lda         ev_x                                ; (Not all fixnums: numbers.inc's sums and order)
            and         ev_s
            and         ev_t
            lsr
            jcc         @generic
            lda         ev_t + 1
            bne         @next
            lda         ev_t
            cmp         #1
            jeq         @zero
@next:
            sec                                             ; (from - to: its sign, against the step's)
            lda         ev_x
            sbc         ev_s
            lda         ev_x + 1
            sbc         ev_s + 1
            bvc         :+
            eor         #$80
:
            eor         ev_t + 1
            jpl         @end
            lda         ev_x                                ; (The same: none either way)
            cmp         ev_s
            bne         :+
            lda         ev_x + 1
            cmp         ev_s + 1
            jeq         @end
:
            MOVR        ev_v, ev_x
            jsr         range_add
            jcs         @room
            lda         ev_x                                ; (from + step: past fixnums, numbers.inc's)
            and         #$FE
            clc
            adc         ev_t
            sta         bi_t
            lda         ev_x + 1
            adc         ev_t + 1
            bvs         @gstep
            sta         ev_x + 1
            lda         bi_t
            sta         ev_x
            jmp         @next
@generic:
            MOVR        h_v, ev_t                           ; (The step's sign: the way it goes)
            FARN        3, num_sign_of
            cmp         #0
            beq         @zero
            sta         bi_s
@gtest:
            MOVR        nx, ev_x                            ; (from against to: less, going up; more, going
            MOVR        h_v, ev_s                           ; down)
            FARN        3, num_cmp
            clc
            adc         bi_s
            bne         @end
            MOVR        ev_v, ev_x
            jsr         range_add
            bcs         @room
@gstep:
            MOVR        nx, ev_x
            MOVR        h_v, ev_t
            lda         #NO_ADD
            FARN        3, num_op
            bcs         @gerr
            MOVR        ev_x, h_v
            bra         @gtest
@gerr:
            MOVR        ev_v, h_v
            rts
@zero:
            LDR         r0, bs_range_zero
            jmp         bi_error_text
@end:
            MOVR        h_v, ev_a
            lda         #OT_QEXPR
            jsr         make_list
            bcs         @room
            MOVR        ev_v, h_v
@done:
            rts
@room:
            jmp         bi_no_room

; ev_v onto range's list: ev_a, its last ev_w.  OUT: C = 1, no room
range_add:
            MOVR        h_a, ev_v
            LDR         h_d, NIL
            jsr         cons
            bcs         @no
            lda         ev_a + 1
            bne         :+
            MOVR        ev_a, h_v
            MOVR        ev_w, h_v
            clc
            rts
:
            MOVR        h_a, h_v
            MOVR        h_v, ev_w
            jsr         set_cdr
            MOVR        ev_w, h_a
            clc
@no:
            rts

; h_v, one of range's numbers.  OUT: C = 1, it isn't one (ev_v an error)
range_num:
            jsr         num_check
            bcs         :+
            rts
:
            jsr         h_is_error_v
            bcs         :+
            LDR         r0, bs_range_nums
            jmp         bi_error_text
:
            rts

; ---- Numbers (fixnums, as yet: 7.3 has the rest)

; (+ x ...): the numbers added; but where one of two isn't a number, the two as print shows them, joined (and the
; rest then too).  (- n ...), (* n ...), (/ n ...): the numbers taken, multiplied or divided in turn ((- n) is -n).
; With none, 0 (+, -) or 1 (*, /).  A value too big for a fixnum is an error, as yet, and so is a division that
; isn't exact
bi_add:
            lda         #0
            bra         arith
bi_sub:
            lda         #1
            bra         arith
bi_mul:
            lda         #2
            bra         arith
bi_div:
            lda         #3
arith:
            sta         ev_i
            lda         ev_a + 1
            bne         @first
            ldx         ev_i                                ; (None)
            lda         arith_none,x
            sta         ev_v
            stz         ev_v + 1
            rts
@first:
            jsr         arg_next
            MOVR        ev_v, h_v
            jsr         num_check
            bcc         :+
            lda         ev_i                                ; (Not a number: +'s text)
            jeq         add_first
            jmp         arith_not
:
            lda         ev_a + 1
            bne         @next
            lda         ev_i                                ; ((- n): -n; a fixnum's 2 - (2n + 1))
            cmp         #1
            bne         @done
            lda         ev_v
            lsr
            bcc         @negate
            sec
            lda         #2
            sbc         ev_v
            sta         bi_t
            lda         #0
            sbc         ev_v + 1
            bvs         @negate
            sta         ev_v + 1
            lda         bi_t
            sta         ev_v
@done:
            rts
@negate:
            MOVR        h_v, ev_v
            FARN        3, num_neg
            MOVR        ev_v, h_v
            rts
@next:
            jsr         arg_next
            bcs         @done
            jsr         num_check
            bcc         :+
            lda         ev_i
            jeq         add_text
            jmp         arith_not
:
            lda         ev_v                                ; (Not two fixnums: num_op's)
            and         h_v
            lsr
            bcc         @slow
            lda         ev_i
            beq         @add
            cmp         #2
            beq         @mul
            bcs         @div
            lda         h_v                                 ; ---- - : (2n + 1) - 2m
            and         #$FE
            sta         bi_t
            sec
            lda         ev_v
            sbc         bi_t
            sta         bi_t
            lda         ev_v + 1
            sbc         h_v + 1
            bvs         @slow
            bra         @fixnum
@add:
            lda         ev_v                                ; ---- + : 2n + (2m + 1)
            and         #$FE
            clc
            adc         h_v
            sta         bi_t
            lda         ev_v + 1
            adc         h_v + 1
            bvs         @slow
@fixnum:
            sta         ev_v + 1
            lda         bi_t
            sta         ev_v
            bra         @next
@mul:
            jsr         fx_mul                              ; ---- *
            bcs         @slow
            bra         @next
@div:
            lda         h_v + 1                             ; ---- / (0: $0001)
            bne         :+
            lda         h_v
            cmp         #1
            beq         @zero
:
            jsr         fx_div
            bcc         @next
@slow:
            MOVR        nx, ev_v                            ; ---- Past fixnums, or not one: num_op's (the third
            lda         ev_i                                ; bank)
            FARN        3, num_op
            MOVR        ev_v, h_v
            bcs         :+
            jmp         @next
:
            rts
@zero:
            LDR         r0, bs_div_zero
            jmp         bi_error_text

; C = 1 if h_v isn't a number
num_check:
            lda         h_v
            lsr
            bcs         @yes
            jsr         type_of
            cmp         #TY_NUM
            beq         @yes
            sec
            rts
@yes:
            clc
            rts

; The number h_v's kind: .A = 0 an integer, 1 a fixed decimal, 2 a rational, 3 a complex number
num_kind2:
            lda         h_v
            lsr
            lda         #0
            bcs         @done
            jsr         deref
            lda         (h_p)
            sec
            sbc         #OT_BIG
            tax
            lda         nk2_of,x
@done:
            rts

; All parameters to operator '-' must be numbers.  (ev_i: the operator; an error: itself)
arith_not:
            jsr         h_is_error_v
            bcs         @done
            LDR         r0, bs_op_not1
            jsr         msg_start
            ldx         ev_i
            lda         arith_op,x
            jsr         msg_char
            LDR         r0, bs_op_not2
            jsr         msg_text
            jmp         msg_error
@done:
            rts

; +'s text: the value so far (ev_v, a number) and h_v (not one) as print shows them, joined, then the arguments left
; too (add_first: the first argument, h_v, isn't a number); an error among them, itself
add_text:
            jsr         h_is_error_v
            bcs         add_done
            MOVR        ev_x, h_v
            jsr         cap_start
            MOVR        h_v, ev_v
            jsr         print_display
            MOVR        h_v, ev_x
            bra         add_rest
add_first:
            jsr         h_is_error_v
            bcs         add_done
            jsr         cap_start
add_rest:
            jsr         h_is_error_v
            bcs         add_error
            jsr         print_display
            jsr         arg_next
            bcc         add_rest
            jsr         cap_string
            MOVR        ev_v, h_v
add_done:
            rts
add_error:
            stz         cap_on
            rts

; h_v, an argument, a number.  OUT: C = 1, it isn't (ev_v an error)
num_arg:
            lda         h_v
            lsr
            bcc         :+
            clc
            rts
:
            jsr         h_is_error_v
            bcs         :+
            LDR         r0, bs_numbers
            jsr         bi_error_text
:
            sec
            rts

; ev_v = ev_v * h_v (fixnums).  OUT: C = 1, too big
fx_mul:
            stz         bi_s
            lda         ev_v
            ldx         ev_v + 1
            jsr         fx_unpack
            MOVR        bi_n, bi_t
            lda         h_v
            ldx         h_v + 1
            jsr         fx_unpack
            stz         bi_p + 2                            ; (bi_p = bi_n * bi_t: shifts and adds)
            stz         bi_p + 3
            ldx         #16
@bit:
            lsr         bi_t + 1
            ror         bi_t
            bcc         :+
            clc
            lda         bi_p + 2
            adc         bi_n
            sta         bi_p + 2
            lda         bi_p + 3
            adc         bi_n + 1
            sta         bi_p + 3
:
            ror         bi_p + 3
            ror         bi_p + 2
            ror         bi_p + 1
            ror         bi_p
            dex
            bne         @bit
            lda         bi_p + 2
            ora         bi_p + 3
            bne         @big
            lda         bi_p + 1
            bmi         @big
            MOVR        bi_n, bi_p
            jmp         fx_pack
@big:
            sec
            rts

; ev_v = ev_v / h_v (fixnums, h_v not 0).  OUT: C = 1, V = 0: too big; C = 1, V = 1: not exact
fx_div:
            stz         bi_s
            lda         h_v
            ldx         h_v + 1
            jsr         fx_unpack
            MOVR        bi_m, bi_t
            lda         ev_v
            ldx         ev_v + 1
            jsr         fx_unpack
            MOVR        bi_n, bi_t
            stz         bi_p                                ; (bi_n / bi_m: the quotient in bi_n, the remainder
            stz         bi_p + 1                            ; in bi_p)
            ldx         #16
@bit:
            asl         bi_n
            rol         bi_n + 1
            rol         bi_p
            rol         bi_p + 1
            sec
            lda         bi_p
            sbc         bi_m
            tay
            lda         bi_p + 1
            sbc         bi_m + 1
            bcc         :+
            sta         bi_p + 1
            sty         bi_p
            inc         bi_n
:
            dex
            bne         @bit
            lda         bi_p
            ora         bi_p + 1
            bne         @inexact
            jmp         fx_pack
@inexact:
            bit         @v                                  ; (V = 1)
            sec
@v:
            rts

; The fixnum .A/.X unpacked: bi_t = its size (0-16384), bi_s's top bit flipped if it's negative
fx_unpack:
            sta         bi_t
            txa
            cmp         #$80
            ror
            sta         bi_t + 1
            ror         bi_t
            bit         bi_t + 1
            bpl         @done
            lda         bi_s
            eor         #$80
            sta         bi_s
            sec
            lda         #0
            sbc         bi_t
            sta         bi_t
            lda         #0
            sbc         bi_t + 1
            sta         bi_t + 1
@done:
            rts

; bi_n (a size, 0-32767), its sign bi_s's top bit, as a fixnum: ev_v.  OUT: C = 1, too big (V = 0)
fx_pack:
            bit         bi_s
            bpl         :+
            sec
            lda         #0
            sbc         bi_n
            sta         bi_n
            lda         #0
            sbc         bi_n + 1
            sta         bi_n + 1
:
            lda         bi_n + 1                            ; (-16384 to 16383: its top two bits the same)
            asl
            eor         bi_n + 1
            bmi         @big
            lda         bi_n
            asl
            ora         #1
            sta         ev_v
            lda         bi_n + 1
            rol
            sta         ev_v + 1
            clc
            rts
@big:
            clv
            sec
            rts

; (< a b), (> a b): T or NIL; (cmp a b): -1, 0 or 1, as a is less than b, the same, or more (compare's order)
bi_lt:
            lda         #0
            bra         order
bi_gt:
            lda         #1
            bra         order
bi_cmp:
            lda         #2
order:
            sta         ev_i
            jsr         count_a
            cmp         #2
            beq         :+
            LDR         r0, bs_two
            jmp         bi_error_text
:
            jsr         arg_next
            MOVR        ev_x, h_v
            jsr         arg_next
            lda         #8                                  ; (Lists in lists: so deep, at most)
            sta         ev_c
            jsr         compare
            ldx         ev_i
            cpx         #2
            beq         @number
            cmp         order_true,x
            beq         @t
            LDR         ev_v, NIL
            rts
@t:
            LDR         ev_v, T_VAL
            rts
@number:
            asl                                             ; (-1, 0, 1: 2n + 1)
            ora         #1
            sta         ev_v
            and         #$80
            beq         :+
            lda         #$FF
:
            sta         ev_v + 1
            rts

; ev_x against h_v (danlang's CompareTo): .A = $FF, 0 or 1 (less, the same, more).  Numbers by their values;
; strings and characters, symbols and atoms, errors by their texts; lists (of a kind, and a length) item by item;
; anything else as the REPL shows it.  (ev_c: how deep lists may go yet)
compare:
            lda         ev_x                                ; ---- Numbers
            and         h_v
            lsr
            bcc         @texts
            sec
            lda         ev_x
            sbc         h_v
            sta         bi_t
            lda         ev_x + 1
            sbc         h_v + 1
            bvc         :+
            eor         #$80
:
            bmi         @less
            ora         bi_t
            beq         @same
            lda         #1
            rts
@less:
            lda         #$FF
            rts
@same:
            lda         #0
            rts
@texts:
            MOVR        ev_w, h_v                           ; ---- Numbers: by value (num_cmp, the third bank)
            jsr         num_check
            bcs         @not_nums
            MOVR        h_v, ev_x
            jsr         num_check
            bcs         @not_nums
            MOVR        nx, ev_x
            MOVR        h_v, ev_w
            FARN        3, num_cmp
            rts
@not_nums:
            MOVR        h_v, ev_w                           ; ---- Texts
            jsr         type_of
            sta         bi_s
            MOVR        h_v, ev_x
            jsr         type_of
            jsr         cmp_kind
            sta         bi_t
            lda         bi_s
            jsr         cmp_kind
            cmp         bi_t
            bne         @shown
            cmp         #1
            beq         @text
            cmp         #2
            beq         @error
            cmp         #3
            beq         @lists
@shown:
            MOVR        h_v, ev_x                           ; ---- As the REPL shows them
            jsr         cap_start
            jsr         print_repr
            stz         cap_on
            lda         cap_n
            sta         str_an
            tax
:
            dex
            bmi         :+
            lda         cap_buf,x
            sta         str_a,x
            bra         :-
:
            MOVR        h_v, ev_w
            jsr         cap_start
            jsr         print_repr
            stz         cap_on
            lda         cap_n
            sta         str_bn
            tax
:
            dex
            bmi         :+
            lda         cap_buf,x
            sta         str_b,x
            bra         :-
:
            jmp         text_cmp
@error:
            jsr         err_msg_x                           ; (Errors: their messages)
@text:
            MOVR        h_v, ev_x
            jsr         text_a
            MOVR        h_v, ev_w
            jsr         text_b
            jmp         text_cmp
@lists:
            lda         ev_c                                ; ---- Lists of a kind: item by item, if they're
            beq         @shown                              ; of a length (not 0), and not too deep
            MOVR        h_v, ev_x
            jsr         type_of
            sta         bi_s
            MOVR        h_v, ev_w
            jsr         type_of
            cmp         bi_s
            jne         @shown
            MOVR        h_v, ev_x
            jsr         list_of
            jsr         count_h
            sta         bi_s
            jeq         @shown
            MOVR        h_v, ev_w
            jsr         list_of
            jsr         count_h
            cmp         bi_s
            jne         @shown
            MOVR        h_v, ev_x                           ; (Their chains)
            jsr         list_of
            MOVR        ev_x, h_v
            MOVR        h_v, ev_w
            jsr         list_of
            MOVR        ev_w, h_v
            dec         ev_c
@item:
            lda         ev_x + 1
            beq         @items_same
            lda         ev_x + 1                            ; (Each pair of items: the chains kept on the 6502's
            pha                                             ; stack)
            lda         ev_x
            pha
            lda         ev_w + 1
            pha
            lda         ev_w
            pha
            MOVR        h_v, ev_x
            jsr         car
            MOVR        bi_q, h_v
            MOVR        h_v, ev_w
            jsr         car
            MOVR        ev_x, bi_q
            jsr         compare
            plx
            stx         ev_w
            plx
            stx         ev_w + 1
            plx
            stx         ev_x
            plx
            stx         ev_x + 1
            cmp         #0
            bne         @items_done
            MOVR        h_v, ev_x
            jsr         cdr
            MOVR        ev_x, h_v
            MOVR        h_v, ev_w
            jsr         cdr
            MOVR        ev_w, h_v
            bra         @item
@items_same:
            lda         #0
@items_done:
            inc         ev_c
            rts

; compare's kinds of the type .A: 1 texts (strings, characters, symbols, atoms), 2 errors, 3 lists, 0 the rest
cmp_kind:
            tax
            lda         cmp_kinds - 1,x
            rts

; ev_x and ev_w, errors: their messages, strings
err_msg_x:
            MOVR        h_v, ev_x
            jsr         deref
            ldy         #3
            lda         (h_p),y
            tax
            dey
            lda         (h_p),y
            sta         ev_x
            stx         ev_x + 1
            MOVR        h_v, ev_w
            jsr         deref
            ldy         #3
            lda         (h_p),y
            tax
            dey
            lda         (h_p),y
            sta         ev_w
            stx         ev_w + 1
            rts

; (eq a b), (neq a b): T or NIL, as a and b are equal (equal's way), or not
bi_eq:
            lda         #0
            bra         :+
bi_neq:
            lda         #1
:
            sta         ev_i
            jsr         count_a
            cmp         #2
            beq         :+
            LDR         r0, bs_two
            jmp         bi_error_text
:
            jsr         arg_next
            MOVR        ev_x, h_v
            jsr         arg_next
            jsr         equal
            lda         #0
            rol
            eor         ev_i
            lsr
            jmp         bi_bool

; C = 1 if ev_x and h_v are equal (danlang's Equals): of a type, and numbers the same, strings and characters the
; same text, symbols and atoms the same name, errors the same message, functions the same built-in (or formals and
; body); lists and S-expressions of a length, their items equal in turn.  (The pairs to compare yet are on the
; evaluation stack: nothing's made, so nothing's collected)
equal:
            MOVR        eq_sp, ev_sp
            lda         ev_bank
            sta         eq_bank
            stz         eq_n
            stz         eq_n + 1
            MOVR        eq_a, ev_x
            MOVR        eq_b, h_v
@pair:
            lda         eq_a                                ; (The same value)
            cmp         eq_b
            bne         @diff
            lda         eq_a + 1
            cmp         eq_b + 1
            beq         @same
@diff:
            MOVR        h_v, eq_b
            jsr         type_of
            sta         eq_t
            MOVR        h_v, eq_a
            jsr         type_of
            cmp         eq_t
            bne         @no
            cmp         #TY_STRING
            beq         @text
            cmp         #TY_SYMBOL
            beq         @text
            cmp         #TY_ERROR
            beq         @error
            cmp         #TY_LIST
            jeq         @lists
            cmp         #TY_SEXPR
            jeq         @lists
            cmp         #TY_PAIR
            jeq         @chains
            cmp         #TY_FUNC
            jeq         @func
            cmp         #TY_NUM
            beq         @num
@no:
            MOVR        ev_sp, eq_sp                        ; (Not equal: the stack as it was)
            lda         eq_bank
            sta         ev_bank
            clc
            rts
@text:
            MOVR        h_v, eq_a                           ; ---- Strings, symbols (a reference and its symbol)
            jsr         text_a
            MOVR        h_v, eq_b
            jsr         text_b
            jsr         text_cmp
            cmp         #0
            bne         @no
@same:
            lda         eq_n                                ; ---- The next pair, if there's one
            ora         eq_n + 1
            bne         @pop
            sec
            rts
@pop:
            lda         eq_n
            bne         :+
            dec         eq_n + 1
:
            dec         eq_n
            POPV        eq_b
            POPV        eq_a
            jmp         @pair
@error:
            ldy         #2                                  ; ---- Errors: their messages
            jsr         eq_field
            jmp         @diff
@num:
            MOVR        nx, eq_a                            ; ---- Numbers: by value
            MOVR        h_v, eq_b
            FARN        3, num_cmp
            cmp         #0
            bne         @no
            jmp         @same
@lists:
            ldy         #2                                  ; ---- Lists: their chains
            jsr         eq_field
@chains:
            lda         eq_a + 1                            ; ---- Chains: each pair of items
            ora         eq_b + 1
            beq         @same
            lda         eq_a + 1
            jeq         @no
            lda         eq_b + 1
            jeq         @no
            jsr         ev_deep
            jcs         @no
            MOVR        h_v, eq_a
            jsr         car
            PUSHV       h_v
            MOVR        h_v, eq_b
            jsr         car
            PUSHV       h_v
            jsr         eq_more
            MOVR        h_v, eq_a
            jsr         cdr
            MOVR        eq_a, h_v
            MOVR        h_v, eq_b
            jsr         cdr
            MOVR        eq_b, h_v
            bra         @chains
@func:
            lda         eq_a + 1                            ; ---- Functions: a built-in, itself; else the formals
            cmp         #>BUILTIN_END                       ; and the bodies
            jcc         @no
            lda         eq_b + 1
            cmp         #>BUILTIN_END
            jcc         @no
            ldy         #4                                  ; (The bodies, to compare after ...
            jsr         eq_field
            PUSHV       eq_a
            PUSHV       eq_b
            jsr         eq_more
            ldy         #2                                  ;   and the formals now)
            jsr         eq_fields
            jmp         @chains

; One more pair on the stack, for equal
eq_more:
            inc         eq_n
            bne         :+
            inc         eq_n + 1
:
            rts

; eq_a and eq_b = their fields at .Y (two objects': errors' messages, lists' chains, functions' formals and bodies;
; eq_fields: ev_x's and ev_w's, the objects eq_field keeps there)
eq_field:
            MOVR        ev_x, eq_a
            MOVR        ev_w, eq_b
eq_fields:
            MOVR        h_v, ev_x
            jsr         deref
            lda         (h_p),y
            sta         eq_a
            iny
            lda         (h_p),y
            sta         eq_a + 1
            dey
            MOVR        h_v, ev_w
            jsr         deref
            lda         (h_p),y
            sta         eq_b
            iny
            lda         (h_p),y
            sta         eq_b + 1
            rts

; ---- Output

; (print x ...): each as print shows it (a string's text as it is), a space between them, then a new line; (write
; x ...): nothing between them, and no new line.  NIL
bi_print:
            lda         #1
            bra         :+
bi_write:
            lda         #0
:
            sta         ev_i
            stz         ev_c                                ; (None yet)
@next:
            jsr         arg_next
            bcs         @end
            lda         ev_c
            and         ev_i
            beq         :+
            lda         #' '
            jsr         out_char
:
            lda         #1
            sta         ev_c
            jsr         print_display
            bra         @next
@end:
            lda         ev_i
            beq         :+
            jsr         out_newline
:
            LDR         ev_v, NIL
            rts

; (repr x): x as the REPL shows it, a string
bi_repr:
            jsr         count_a
            cmp         #1
            beq         :+
            LDR         r0, bs_repr_one
            jmp         bi_error_text
:
            jsr         arg_next
            jsr         cap_start
            jsr         print_repr
            jsr         cap_string
            MOVR        ev_v, h_v
            rts

; (to-str x [base]): x as print shows it, a string; a number in the base (bases.inc: "#x", "x", "#16r" ...)
bi_to_str:
            jsr         count_a
            cmp         #1
            beq         :+
            bcs         @base
            LDR         r0, bs_to_str_few
            jmp         bi_error_text
:
            jsr         arg_next
            jsr         display_string
            MOVR        ev_v, h_v
            rts
@base:
            jsr         arg_next                            ; (A number, and a string)
            MOVR        ev_x, h_v
            jsr         num_check
            bcc         :+
            LDR         r0, bs_to_str_num
            jmp         bi_error_text
:
            jsr         arg_next
            jsr         type_of
            cmp         #TY_STRING
            beq         :+
            LDR         r0, bs_to_str_str
            jmp         bi_error_text
:
            jsr         text_b
            LDR         bi_q, str_b                         ; (bs_spec: bases.inc's)
            lda         str_bn
            sta         bs_spec_n
            jsr         cap_start
            FARN        3, bn_to_base
            bcc         :+
            stz         cap_on
            MOVR        ev_v, h_v
            rts
:
            jsr         cap_string
            MOVR        ev_v, h_v
            rts

; (format text x ...): the text with each {} the next x, as print shows it ({{ and }}: a brace)
bi_format:
            jsr         count_a
            cmp         #1
            bcs         :+
            LDR         r0, bs_format_few
            jmp         bi_error_text
:
            jsr         arg_next
            jsr         text_a
            bcc         :+
            LDR         r0, bs_format_str
            jmp         bi_error_text
:
            jsr         cap_start
            ldy         #0
@byte:
            cpy         str_an
            beq         @end
            lda         str_a,y
            iny
            cmp         #'{'
            beq         @open
            cmp         #'}'
            bne         @out
            cpy         str_an                              ; (}}: })
            beq         @out
            cmp         str_a,y
            bne         @out
            iny
@out:
            jsr         out_char
            bra         @byte
@open:
            cpy         str_an
            beq         @out
            ldx         str_a,y
            cpx         #'{'                                ; ({{: {)
            bne         :+
            iny
            bra         @out
:
            cpx         #'}'
            bne         @out
            iny                                             ; ({}: the next argument)
            phy
            jsr         arg_next
            bcs         @few
            jsr         print_display
            ply
            bra         @byte
@few:
            ply
            stz         cap_on
            LDR         r0, bs_format_args_few
            jmp         bi_error_text
@end:
            jsr         cap_string
            MOVR        ev_v, h_v
            lda         ev_a + 1
            beq         :+
            LDR         r0, bs_format_args_many
            jmp         bi_error_text
:
            rts

; ---- Errors

; (error message [code]): an error, its message the message as print shows it, its code the atom code (or NIL)
bi_error:
            jsr         count_a
            cmp         #1
            bcc         @wrong
            cmp         #3
            bcs         @wrong
            jsr         arg_next
            jsr         display_string
            bcc         :+
            MOVR        ev_v, h_v
            rts
:
            MOVR        ev_x, h_v
            LDR         ev_t, NIL
            jsr         arg_next
            bcs         @make
            MOVR        ev_t, h_v
            jsr         type_of
            cmp         #TY_ATOM
            bne         @code
@make:
            MOVR        h_a, ev_x
            MOVR        h_d, ev_t
            jsr         make_error
            bcc         :+
            jmp         bi_no_room
:
            MOVR        ev_v, h_v
            rts
@wrong:
            LDR         r0, bs_error
            jmp         bi_error_text
@code:
            LDR         r0, bs_error_code
            jmp         bi_error_text

; (error-message x), (error-code x): an error's message, or its code (an atom, or NIL); NIL if x isn't an error
bi_error_message:
            ldx         #2
            bra         :+
bi_error_code:
            ldx         #4
:
            stx         ev_i
            jsr         count_a
            cmp         #1
            beq         :+
            LDR         r0, bs_one
            jmp         bi_error_text
:
            jsr         arg_next
            jsr         h_is_error
            bcs         :+
            LDR         ev_v, NIL
            rts
:
            ldy         ev_i
            lda         (h_p),y
            sta         ev_v
            iny
            lda         (h_p),y
            sta         ev_v + 1
            rts

; ---- Types

; (type-of x): an atom naming its type: :number, :list, :string ...
bi_type_of:
            jsr         count_a
            cmp         #1
            beq         :+
            LDR         r0, bs_type_of_one
            jmp         bi_error_text
:
            jsr         arg_next
            jsr         type_of
            tax
            lda         type_names_lo - 1,x
            sta         r0
            lda         type_names_hi - 1,x
            sta         r0 + 1
            lda         #OT_ATOM
            jsr         intern_counted
            bcc         :+
            jmp         bi_no_room
:
            MOVR        ev_v, h_v
            rts

; (t? x), (nil? x), (num? x) ...: T if x is one, else NIL (int?, fixed?, rational?, complex?: an error if x isn't a
; number)
bi_is_t:
            ldx         #0
            bra         is_type
bi_is_nil:
            ldx         #1
            bra         is_type
bi_is_num:
            ldx         #2
            bra         is_type
bi_is_atom:
            ldx         #3
            bra         is_type
bi_is_symbol:
            ldx         #4
            bra         is_type
bi_is_string:
            ldx         #5
            bra         is_type
bi_is_char:
            ldx         #6
            bra         is_type
bi_is_function:
            ldx         #7
            bra         is_type
bi_is_error:
            ldx         #8
            bra         is_type
bi_is_qexpr:
            ldx         #9
            bra         is_type
bi_is_sexpr:
            ldx         #10
            bra         is_type
bi_is_int:
            ldx         #11
            bra         is_type
bi_is_fixed:
            ldx         #12
            bra         is_type
bi_is_rational:
            ldx         #13
            bra         is_type
bi_is_complex:
            ldx         #14
is_type:
            stx         ev_i
            jsr         count_a
            cmp         #1
            beq         :+
            LDR         r0, bs_one
            jmp         bi_error_text
:
            jsr         arg_next
            ldx         ev_i
            cpx         #1
            bne         :+
            lda         h_v                                 ; (nil?: NIL itself)
            ora         h_v + 1
            cmp         #1
            jmp         bi_bool_not
:
            jsr         type_of
            ldx         ev_i
            cpx         #11
            bcs         @number
            cmp         is_types,x
            beq         @yes
            clc
            jmp         bi_bool
@yes:
            sec
            jmp         bi_bool
@number:
            cmp         #TY_NUM                             ; (A fixnum: an int, as yet)
            beq         :+
            jsr         h_is_error_v
            bcs         @done
            LDR         r0, bs_not_number
            jmp         bi_error_text
:
            jsr         num_kind2                           ; (int? 11, fixed? 12, rational? 13, complex? 14: kinds
            clc                                             ; 0 to 3)
            adc         #11
            cmp         ev_i
            beq         @yes
            clc
            jmp         bi_bool
@done:
            rts

; ev_v = T if C = 0, else NIL
bi_bool_not:
            rol
            eor         #1
            lsr
            jmp         bi_bool

; ---- Strings and characters

; (char-at s n): the string's character n (0 on)
bi_char_at:
            jsr         count_a
            cmp         #2
            beq         :+
            LDR         r0, bs_char_at_args
            jmp         bi_error_text
:
            jsr         arg_next
            jsr         str_arg
            bcs         @done
            jsr         arg_next
            jsr         int_arg
            bcs         @done
            cpx         #0
            bne         @outside
            cmp         str_an
            bcs         @outside
            tax
            lda         str_a,x
            jmp         make_char
@outside:
            LDR         r0, bs_char_at_outside
            jmp         bi_error_text
@done:
            rts

; h_v, a string (or a character) argument: its text in str_a.  OUT: C = 1, it isn't one (ev_v an error)
str_arg:
            jsr         h_is_error_v
            bcs         @error
            jsr         type_of
            cmp         #TY_STRING
            beq         :+
            cmp         #TY_CHAR
            bne         @not
:
            jsr         text_a
            clc
            rts
@not:
            LDR         r0, bs_expects_string
            jsr         bi_error_text
@error:
            sec
            rts

; h_v, a number argument: .A/.X its value.  OUT: C = 1, it isn't one (ev_v an error)
int_arg:
            jsr         num_arg
            bcs         :+
            jsr         fx_int
            clc
:
            rts

; (char-code c): a character's code (or a string of one's)
bi_char_code:
            jsr         count_a
            cmp         #1
            beq         :+
            LDR         r0, bs_one
            jmp         bi_error_text
:
            jsr         arg_next
            jsr         str_arg
            bcs         @done
            lda         str_an
            cmp         #1
            beq         :+
            LDR         r0, bs_char_code_one
            jmp         bi_error_text
:
            lda         str_a
            ldx         #0
            jmp         int_fx
@done:
            rts

; (code-char n): the character with the code n (0-255)
bi_code_char:
            jsr         count_a
            cmp         #1
            beq         :+
            LDR         r0, bs_one
            jmp         bi_error_text
:
            jsr         arg_next
            jsr         int_arg
            bcs         @done
            cpx         #0
            bne         :+
            jmp         make_char
:
            LDR         r0, bs_code_char
            jmp         bi_error_text
@done:
            rts

; (substring s from [n]): the string's characters from from on (n of them, at most)
bi_substring:
            jsr         count_a
            cmp         #2
            bcc         @few
            cmp         #4
            bcc         :+
            LDR         r0, bs_substring_many
            jmp         bi_error_text
@few:
            LDR         r0, bs_substring_few
            jmp         bi_error_text
:
            jsr         arg_next
            jsr         str_arg
            bcs         @done
            jsr         arg_next
            jsr         int_arg
            bcs         @done
            cpx         #0
            bne         @outside
            cmp         str_an
            beq         :+
            bcs         @outside
:
            sta         bi_n                                ; (from)
            sec
            lda         str_an
            sbc         bi_n
            sta         bi_m                                ; (n: the rest, or less)
            jsr         arg_next
            bcs         @make
            jsr         int_arg
            bcs         @done
            cpx         #0
            bne         @negative
            cmp         bi_m
            bcs         @make
            sta         bi_m
@make:
            clc
            lda         #<str_a
            adc         bi_n
            sta         r0
            lda         #>str_a
            adc         #0
            sta         r0 + 1
            lda         bi_m
            jsr         buf_string
            MOVR        ev_v, h_v
@done:
            rts
@negative:
            bpl         @make                               ; (More than 255: the rest)
@outside:
            LDR         r0, bs_substring_outside
            jmp         bi_error_text

; (index-of x y), (last-index-of x y): where y is in x, first or last (a list's item equal to it, a string in a
; string), or -1
bi_index_of:
            lda         #0
            bra         :+
bi_last_index_of:
            lda         #1
:
            sta         ev_i
            jsr         count_a
            cmp         #2
            beq         :+
            LDR         r0, bs_two
            jmp         bi_error_text
:
            jsr         arg_next
            MOVR        ev_s, h_v
            jsr         arg_next
            MOVR        ev_t, h_v
            MOVR        h_v, ev_s
            jsr         type_of
            cmp         #TY_LIST
            jeq         @list
            cmp         #TY_STRING
            bne         @wrong
            MOVR        h_v, ev_t
            jsr         type_of
            cmp         #TY_STRING
            beq         :+
            cmp         #TY_CHAR
            bne         @wrong
:
            MOVR        h_v, ev_s                           ; ---- A string in a string
            jsr         text_a
            MOVR        h_v, ev_t
            jsr         text_b
            lda         #$FF
            sta         bi_n                                ; (Where it's found: none yet)
            ldx         #0
@at:
            txa                                             ; (At .X: the rest long enough?)
            clc
            adc         str_bn
            bcs         @found
            cmp         str_an
            beq         :+
            bcs         @found
:
            stx         bi_t
            ldy         #0
@byte:
            cpy         str_bn
            beq         @match
            tya
            clc
            adc         bi_t
            tax
            lda         str_a,x
            cmp         str_b,y
            bne         @no_match
            iny
            bra         @byte
@no_match:
            ldx         bi_t
            inx
            bra         @at
@match:
            ldx         bi_t
            stx         bi_n
            lda         ev_i
            beq         @found
            inx
            bra         @at
@found:
            lda         bi_n
            ldx         #0
            cmp         #$FF
            bne         :+
            dex
:
            jmp         int_fx
@wrong:
            LDR         r0, bs_index_of_types
            jmp         bi_error_text
@list:
            MOVR        h_v, ev_s                           ; ---- An item of a list
            jsr         list_of
            MOVR        ev_s, h_v
            lda         #$FF
            sta         bi_n + 1                            ; (Where: none yet)
            stz         ev_c
@item:
            lda         ev_s + 1
            beq         @listed
            MOVR        h_v, ev_s
            jsr         car
            MOVR        ev_x, h_v
            MOVR        h_v, ev_t
            jsr         equal
            bcc         :+
            lda         ev_c
            sta         bi_n + 1
            lda         ev_i
            beq         @listed
:
            MOVR        h_v, ev_s
            jsr         cdr
            MOVR        ev_s, h_v
            inc         ev_c
            bra         @item
@listed:
            lda         bi_n + 1
            sta         bi_n
            bra         @found

; ---- Reading

; (read text): the expressions in the text, unevaluated, a Q-expression of them: (eval (read "+ 1 2")) is 3
bi_read:
            jsr         count_a
            cmp         #1
            beq         :+
            LDR         r0, bs_read_one
            jmp         bi_error_text
:
            jsr         arg_next
            jsr         str_arg
            bcs         @done
            ldx         str_an                              ; (Its text in the text buffer)
:
            dex
            bmi         :+
            lda         str_a,x
            sta         text,x
            bra         :-
:
            LDR         r0, text
            lda         str_an
            ldx         #0
            jsr         read_start
            LDR         ev_a, NIL
            LDR         ev_t, NIL
@expr:
            jsr         read_expr
            bcs         @end
            MOVR        ev_v, h_v
            jsr         ev_append
            bcc         @expr
            jmp         bi_no_room
@end:
            cmp         #RD_ERR
            beq         @error
            cmp         #RD_MORE
            beq         @more
            MOVR        h_v, ev_a
            lda         #OT_QEXPR
            jsr         make_list
            bcc         :+
            jmp         bi_no_room
:
            MOVR        ev_v, h_v
@done:
            rts
@more:
            LDR         r0, bs_read_more                    ; ('read': missing a close)
            jmp         bi_error_text
@error:
            MOVR        ev_v, h_v                           ; ('read': and the reader's message)
            LDR         r0, bs_read_error
            jmp         prefix_error

; ev_v = the error ev_v, its message after the text at r0 (prefix_msg: after the message begun); its code kept
prefix_error:
            jsr         msg_start
prefix_msg:
            MOVR        h_v, ev_v
            jsr         deref
            ldy         #3
            lda         (h_p),y
            tax
            dey
            lda         (h_p),y
            sta         h_v
            stx         h_v + 1
            jsr         msg_str
            MOVR        h_v, ev_v                           ; (Its code)
            jsr         deref
            ldy         #5
            lda         (h_p),y
            pha
            dey
            lda         (h_p),y
            pha
            jsr         msg_error
            pla
            sta         h_d
            pla
            sta         h_d + 1
            jsr         ev_is_error
            bcc         :+
            jsr         deref
            ldy         #4
            lda         h_d
            sta         (h_p),y
            iny
            lda         h_d + 1
            sta         (h_p),y
:
            rts

; (load path ...): each file's expressions read (all of them, before any is evaluated: a reading error is the
; file's, and nothing of it runs), then evaluated in turn at the top (GLOBAL).  The last value; or an error, its
; message after the path ("path: ...").  A path without a / is /lib/hylang's too, and .hl may be left off
bi_load:
            jsr         count_a
            cmp         #1
            bcs         :+
            LDR         r0, bs_load_few
            jmp         bi_error_text
:
            LDR         ev_v, NIL
@file:
            jsr         arg_next
            jcs         @done
            MOVR        ev_s, h_v
            jsr         type_of
            cmp         #TY_STRING
            beq         :+
            LDR         r0, bs_load_string
            jmp         bi_error_text
:
            PUSHV       ev_a                                ; (The paths left, and this one)
            PUSHV       ev_s
            stz         ev_c
            jsr         load_read                           ; (ev_t: its expressions)
            bcs         @fail
            LDR         ev_v, NIL
@expr:
            lda         ev_t + 1
            jeq         @next
            MOVR        h_v, ev_t
            jsr         cdr
            PUSHV       h_v
            MOVR        h_v, ev_t
            jsr         car
            MOVR        ev_x, h_v
            LDR         ev_e, GLOBAL
            jsr         eval_nested
            POPV        ev_t
            lda         ev_v + 1                            ; (exit: the end, its status 0)
            bne         :+
            lda         ev_v
            cmp         #<EXIT_VAL
            bne         :+
            jsr         out_flush
            LDR         r0, 0
            lda         #0
            jmp         EXITS
:
            jsr         ev_is_error
            bcc         @expr
            stz         ev_c
@fail:
            POPV        ev_s                                ; ("path: message", but the system's own errors)
            POPV        ev_a
            lda         ev_c
            bne         @done
            LDR         r0, bs_none
            jsr         msg_start
            MOVR        h_v, ev_s
            jsr         msg_str
            lda         #':'
            jsr         msg_char
            lda         #' '
            jsr         msg_char
            jmp         prefix_msg
@done:
            rts
@next:
            POPV        ev_s
            POPV        ev_a
            jmp         @file

; The file ev_s names, read: ev_t = its expressions (a chain).  OUT: C = 1, ev_v an error (it can't be opened or
; read; a reading error).  Its text goes through the text buffer, as much as there's room for, taken up to its last
; whole line each time, an expression not finished there kept for the next
load_read:
            jsr         load_open
            bcc         :+
            rts
:
            sta         bi_fd
            stz         ld_n
            stz         ld_n + 1
            stz         ld_eof
            LDR         ev_a, NIL
            LDR         ev_t, NIL
@fill:
            sec                                             ; ---- More of the file, after what's there
            lda         #<TEXT_SIZE
            sbc         ld_n
            sta         r1
            lda         #>TEXT_SIZE
            sbc         ld_n + 1
            sta         r1 + 1
            ora         r1
            jeq         @long
            clc
            lda         #<text
            adc         ld_n
            sta         r0
            lda         #>text
            adc         ld_n + 1
            sta         r0 + 1
            lda         bi_fd
            jsr         READ
            jcs         @syserr
            stx         bi_t + 1
            sta         bi_t
            ora         bi_t + 1
            bne         :+
            inc         ld_eof
:
            clc
            lda         ld_n
            adc         bi_t
            sta         ld_n
            lda         ld_n + 1
            adc         bi_t + 1
            sta         ld_n + 1
            MOVR        ld_lim, ld_n                        ; ---- How much to read: all, at its end; else to the
            lda         ld_eof                              ; last line's end
            bne         @read
@back:
            lda         ld_lim
            ora         ld_lim + 1
            beq         @fill
            lda         ld_lim
            bne         :+
            dec         ld_lim + 1
:
            dec         ld_lim
            clc
            lda         #<text
            adc         ld_lim
            sta         bi_q
            lda         #>text
            adc         ld_lim + 1
            sta         bi_q + 1
            lda         (bi_q)
            cmp         #LF
            bne         @back
            inc         ld_lim                              ; (Past the LF)
            bne         @read
            inc         ld_lim + 1
@read:
            LDR         r0, text
            lda         ld_lim
            ldx         ld_lim + 1
            jsr         read_start
@expr:
            MOVR        ld_from, rd_p
            jsr         read_expr
            bcs         @stop
            MOVR        ev_v, h_v
            jsr         ev_append
            bcc         @expr
            jsr         load_close
            MOVR        ev_v, err_room
            sec
            rts
@stop:
            cmp         #RD_ERR
            beq         @error
            cmp         #RD_MORE
            beq         @more
            MOVR        ld_from, ld_lim                     ; (All read: what's after it kept)
            bra         @keep
@more:
            lda         ld_eof                              ; (An expression not finished: kept from its start)
            bne         @missing
            sec
            lda         ld_from
            sbc         #<text
            sta         ld_from
            lda         ld_from + 1
            sbc         #>text
            sta         ld_from + 1
            ora         ld_from
            bne         @keep
            lda         ld_n                                ; (From the buffer's start: it's full)
            cmp         #<TEXT_SIZE
            bne         @keep
            lda         ld_n + 1
            cmp         #>TEXT_SIZE
            beq         @long
@keep:
            jsr         load_keep
            lda         ld_eof
            bne         @done
            jmp         @fill
@done:
            jsr         load_close
            MOVR        ev_t, ev_a
            clc
            rts
@long:
            LDR         r0, bs_load_long
            bra         :+
@missing:
            LDR         r0, bs_missing
:
            jsr         load_close
            jmp         bi_error_text
@error:
            MOVR        ev_v, h_v
            jsr         load_close
            sec
            rts
@syserr:
            pha
            jsr         load_close
            pla
            jmp         sys_error

; ld_lim fixed for load_keep: what's from ld_from on, moved to the buffer's start (ld_n = its length)
load_keep:
            sec
            lda         ld_n
            sbc         ld_from
            sta         ld_n
            lda         ld_n + 1
            sbc         ld_from + 1
            sta         ld_n + 1
            clc                                             ; (From text + ld_from to text)
            lda         #<text
            adc         ld_from
            sta         bi_src
            lda         #>text
            adc         ld_from + 1
            sta         bi_src + 1
            LDR         bi_q, text
            ldx         ld_n + 1
            ldy         #0
@page:
            cpx         #0
            beq         @part
:
            lda         (bi_src),y
            sta         (bi_q),y
            iny
            bne         :-
            inc         bi_src + 1
            inc         bi_q + 1
            dex
            bra         @page
@part:
            cpy         ld_n
            beq         @done
            lda         (bi_src),y
            sta         (bi_q),y
            iny
            bra         @part
@done:
            rts

; The file at ev_s's path opened (as it is; with .hl; then, if it has no /, in /lib/hylang): .A = its fd.  OUT:
; C = 1, ev_v the error (the path's, as the first try had it)
load_open:
            MOVR        h_v, ev_s
            jsr         text_a
            stz         bi_s                                ; (The tries: 0 as it is, 1 .hl, 2 /lib/hylang/, 3 both)
            stz         bi_p                                ; (The first's error)
@try:
            ldx         #0                                  ; (The path, made in str_b)
            lda         bi_s
            and         #2
            beq         @name
            ldy         #0
:
            lda         bs_lib,y
            beq         @name
            sta         str_b,x
            inx
            iny
            bra         :-
@name:
            ldy         #0
:
            cpy         str_an
            beq         :+
            lda         str_a,y
            sta         str_b,x
            inx
            iny
            bra         :-
:
            lda         bi_s
            and         #1
            beq         @made
            ldy         #0
:
            lda         bs_hl,y
            sta         str_b,x
            beq         @open
            inx
            iny
            bra         :-
@made:
            stz         str_b,x
@open:
            LDR         r0, str_b
            lda         #O_READ
            jsr         OPEN
            bcc         @done
            ldx         bi_s
            bne         :+
            sta         bi_p                                ; (The first's error)
:
            inc         bi_s
            lda         bi_s
            cmp         #4
            bcs         @fail
            cmp         #2
            bne         @try
            ldy         str_an                              ; (A / in it: not /lib/hylang's)
@slash:
            dey
            bmi         @try
            lda         str_a,y
            cmp         #'/'
            beq         @fail
            bra         @slash
@fail:
            lda         #1                                  ; (The system's error: as it is)
            sta         ev_c
            lda         bi_p
            jmp         sys_error
@done:
            rts

; load's file closed
load_close:
            lda         bi_fd
            jmp         CLOSE

; ev_v = the system's error .A, its message after ev_s's text ("path: not found").  OUT: C = 1
sys_error:
            pha
            LDR         r0, bs_none
            jsr         msg_start
            MOVR        h_v, ev_s
            jsr         msg_str
            lda         #':'
            jsr         msg_char
            lda         #' '
            jsr         msg_char
            pla
            LDR         r0, str_b
            jsr         ERRSTR
            LDR         r0, str_b
            jsr         msg_text
            jsr         msg_error
            sec
            rts

.segment "RODATA2"
arith_none: .byte       1, 1, 3, 3                          ; (+ and -: 0; * and /: 1)
arith_op:   .byte       "+-*/"
order_true: .byte       $FF, 1                              ; (<: less; >: more)
nk2_of:     .byte       0, 2, 1, 3                          ; (OT_BIG, OT_RAT, OT_FIX, OT_COMPLEX: num_kind2's)

; compare's kinds, by type (TY_*): 1 a text, 2 an error, 3 a list, 0 the rest
cmp_kinds:  .byte       0, 3, 0, 0, 0, 1, 1, 1, 1, 3, 2, 0, 0, 0

; The types is_type's predicates ask about (TY_*: t?, nil? (aside), num?, atom?, symbol? ...)
is_types:   .byte       TY_T, 0, TY_NUM, TY_ATOM, TY_SYMBOL, TY_STRING, TY_CHAR, TY_FUNC, TY_ERROR, TY_LIST, TY_SEXPR

; type-of's names (TY_*), each counted
type_names_lo: .lobytes tn_number, tn_list, tn_t, tn_exit, tn_function, tn_char, tn_string, tn_symbol, tn_atom
            .lobytes    tn_sexpr, tn_error, tn_unknown, tn_unknown, tn_unknown
type_names_hi: .hibytes tn_number, tn_list, tn_t, tn_exit, tn_function, tn_char, tn_string, tn_symbol, tn_atom
            .hibytes    tn_sexpr, tn_error, tn_unknown, tn_unknown, tn_unknown
tn_number:  .byte       6, "number"
tn_list:    .byte       4, "list"
tn_t:       .byte       1, "t"
tn_exit:    .byte       4, "exit"
tn_function: .byte      8, "function"
tn_char:    .byte       4, "char"
tn_string:  .byte       6, "string"
tn_symbol:  .byte       6, "symbol"
tn_atom:    .byte       4, "atom"
tn_sexpr:   .byte       5, "sexpr"
tn_error:   .byte       5, "error"
tn_unknown: .byte       7, "unknown"

; The built-ins, in their numbers' order (BI_RUN first)
bi_table:
            BUILTIN     "", BF_SPECIAL, sf_run
            BUILTIN     "if", BF_SPECIAL, sf_if
            BUILTIN     "do", BF_SPECIAL, sf_do
            BUILTIN     "and", BF_SPECIAL, sf_and
            BUILTIN     "or", BF_SPECIAL, sf_or
            BUILTIN     "let", BF_SPECIAL, sf_let
            BUILTIN     "try", BF_SPECIAL, sf_try
            BUILTIN     "eval", BF_SPECIAL, sf_eval
            BUILTIN     "def", BF_SPECIAL, sf_def
            BUILTIN     "set", BF_SPECIAL, sf_set
            BUILTIN     "set!", BF_SPECIAL, sf_setbang
            BUILTIN     "while", BF_SPECIAL, sf_while
            BUILTIN     "each", BF_SPECIAL, sf_each
            BUILTIN     "dotimes", BF_SPECIAL, sf_dotimes
            BUILTIN     "output-of", BF_SPECIAL, sf_output
            BUILTIN     "<=>", BF_SPECIAL, sf_space
            BUILTIN     "defined?", BF_RAW, bi_defined
            BUILTIN     "expr?", BF_RAW, bi_expr
            BUILTIN     "fn", 0, bi_fn
            BUILTIN     "fexpr", 0, bi_fexpr
            BUILTIN     "fun", 0, bi_fun
            BUILTIN     "list", 0, bi_list
            BUILTIN     "head", 0, bi_head
            BUILTIN     "tail", 0, bi_tail
            BUILTIN     "init", 0, bi_init
            BUILTIN     "end", 0, bi_end
            BUILTIN     "join", 0, bi_join
            BUILTIN     "len", 0, bi_len
            BUILTIN     "item-at", 0, bi_item_at
            BUILTIN     "reverse", 0, bi_reverse
            BUILTIN     "range", 0, bi_range
            BUILTIN     "+", 0, bi_add
            BUILTIN     "-", 0, bi_sub
            BUILTIN     "*", 0, bi_mul
            BUILTIN     "/", 0, bi_div
            BUILTIN     "truncate", BF_BANK3, bn_truncate
            BUILTIN     "complex", BF_BANK3, bn_complex
            BUILTIN     "val", BF_BANK3, bn_val
            BUILTIN     "to-fixed", BF_BANK3, bn_to_fixed
            BUILTIN     "to-rational", BF_BANK3, bn_to_rational
            BUILTIN     "rational.n", BF_BANK3, bn_rational_n
            BUILTIN     "rational.d", BF_BANK3, bn_rational_d
            BUILTIN     "random", BF_BANK3, bn_random
            BUILTIN     "fib", BF_BANK3, bn_fib
            BUILTIN     "<", BF_ERRORS, bi_lt
            BUILTIN     ">", BF_ERRORS, bi_gt
            BUILTIN     "cmp", BF_ERRORS, bi_cmp
            BUILTIN     "eq", BF_ERRORS, bi_eq
            BUILTIN     "neq", BF_ERRORS, bi_neq
            BUILTIN     "print", BF_ERRORS, bi_print
            BUILTIN     "write", BF_ERRORS, bi_write
            BUILTIN     "repr", BF_ERRORS, bi_repr
            BUILTIN     "to-str", 0, bi_to_str
            BUILTIN     "format", 0, bi_format
            BUILTIN     "error", 0, bi_error
            BUILTIN     "error-message", BF_ERRORS, bi_error_message
            BUILTIN     "error-code", BF_ERRORS, bi_error_code
            BUILTIN     "type-of", BF_ERRORS, bi_type_of
            BUILTIN     "t?", BF_ERRORS, bi_is_t
            BUILTIN     "nil?", BF_ERRORS, bi_is_nil
            BUILTIN     "num?", BF_ERRORS, bi_is_num
            BUILTIN     "atom?", BF_ERRORS, bi_is_atom
            BUILTIN     "symbol?", BF_ERRORS, bi_is_symbol
            BUILTIN     "string?", BF_ERRORS, bi_is_string
            BUILTIN     "char?", BF_ERRORS, bi_is_char
            BUILTIN     "function?", BF_ERRORS, bi_is_function
            BUILTIN     "error?", BF_ERRORS, bi_is_error
            BUILTIN     "qexpr?", BF_ERRORS, bi_is_qexpr
            BUILTIN     "sexpr?", BF_ERRORS, bi_is_sexpr
            BUILTIN     "int?", BF_ERRORS, bi_is_int
            BUILTIN     "fixed?", BF_ERRORS, bi_is_fixed
            BUILTIN     "rational?", BF_ERRORS, bi_is_rational
            BUILTIN     "complex?", BF_ERRORS, bi_is_complex
            BUILTIN     "char-at", 0, bi_char_at
            BUILTIN     "char-code", 0, bi_char_code
            BUILTIN     "code-char", 0, bi_code_char
            BUILTIN     "substring", 0, bi_substring
            BUILTIN     "index-of", 0, bi_index_of
            BUILTIN     "last-index-of", 0, bi_last_index_of
            BUILTIN     "read", 0, bi_read
            BUILTIN     "load", 0, bi_load
            .byte       $FF

bs_none:    .byte       0
bs_lib:     .byte       "/lib/hylang/", 0
bs_hl:      .byte       ".hl", 0
bs_fn:      .byte       "'fn' expects {formals} and {body}", 0
bs_fun:     .byte       "'fun' expects {name formals...} and a body", 0
bs_head:    .byte       "'head'", 0
bs_tail:    .byte       "'tail'", 0
bs_init:    .byte       "'init'", 0
bs_end:     .byte       "'end'", 0
bs_too_few: .byte       "Too few parameters passed to ", 0
bs_expects_list: .byte  " expects a QExpr", 0
bs_empty:   .byte       " passed an empty list", 0
bs_join_few: .byte      "Invalid number of parameters passed to 'join'", 0
bs_join_list: .byte     "Invalid parameter passed to 'join'.  Expected QExpr.", 0
bs_len_one: .byte       "'len' requires 1 parameter", 0
bs_len_type: .byte      "'len' requires parameter of type string, list or hash", 0
bs_item_few: .byte      "Too few parameters passed to 'item-at'", 0
bs_item_list: .byte     "First 'item-at' parameter must be a QExpr", 0
bs_item_num: .byte      "Second 'item-at' parameter must be a Number", 0
bs_reverse_one: .byte   "'reverse' requires 1 parameter", 0
bs_reverse_type: .byte  "'reverse' expects a QExpr or a String", 0
bs_range_count: .byte   "'range' expects 1 to 3 Numbers", 0
bs_range_nums: .byte    "'range' expects Numbers", 0
bs_range_zero: .byte    "'range' step cannot be 0", 0
bs_numbers: .byte       "Expects numbers", 0
bs_op_not1: .byte       "All parameters to operator '", 0
bs_op_not2: .byte       "' must be numbers.", 0
bs_big:     .byte       "A number too big (as yet)", 0
bs_div_zero: .byte      "Division by zero.", 0
bs_inexact: .byte       "A division that isn't exact (as yet)", 0
bs_two:     .byte       "Expects 2 parameters", 0
bs_one:     .byte       "Expects 1 parameter", 0
bs_repr_one: .byte      "'repr' requires 1 parameter", 0
bs_to_str_few: .byte    "Too few parameters passed to 'to-str'", 0
bs_to_str_num: .byte    "Only a Number has a base for 'to-str'", 0
bs_to_str_str: .byte    "Second 'to-str' parameter must be a String", 0
bs_format_few: .byte    "Too few parameters passed to 'format'", 0
bs_format_str: .byte    "'format' expects a String", 0
bs_format_args_few: .byte "'format' has too few arguments for its {}s", 0
bs_format_args_many: .byte "'format' has more arguments than {}s", 0
bs_error:   .byte       "'error' expects a message and maybe a code", 0
bs_error_code: .byte    "An error's code must be an atom", 0
bs_type_of_one: .byte   "'type-of' requires 1 parameter", 0
bs_not_number: .byte    "Not a Number", 0
bs_char_at_args: .byte  "'char-at' expects a String and an index", 0
bs_char_at_outside: .byte "'char-at': the index is outside the string", 0
bs_expects_string: .byte "Expects a String", 0
bs_char_code_one: .byte "'char-code' expects one character", 0
bs_code_char: .byte     "'code-char': that isn't a character's code", 0
bs_substring_few: .byte "Too few parameters passed to 'substring'", 0
bs_substring_many: .byte "Too many parameters passed to 'substring'", 0
bs_substring_outside: .byte "'substring': outside the string", 0
bs_index_of_types: .byte "'index-of' expects a String and a String", 0
bs_read_one: .byte      "'read' requires 1 parameter", 0
bs_read_more: .byte     "'read': missing a close", 0
bs_read_error: .byte    "'read': ", 0
bs_missing: .byte       "missing a close", 0
bs_load_few: .byte      "'load' supplied too few parameters", 0
bs_load_string: .byte   "'load' passed non-string parameter(s)", 0
bs_load_long: .byte     "An expression too long to read", 0
.code
