; ****************************************************************************
; builtins.inc - hylang's built-ins (phase 3): the table of all danlang's (Builtins.cs's Arity: each one's name,
; the arguments it needs and the most it takes, its flags, its code), and the code of those in the first bank.  A
; built-in is the value BUILTIN0 + 2 * its number; each is bound to its name as hylang starts (bi_bind).  Those not
; written yet answer "Not yet" (bi_notyet): the phases that make them replace it.
;   An ordinary built-in's code gets its values at argp (cn of them, on the stack: roots), ee the caller's scope;
; it gives back ex, or C = 1 if there's no room.  A special form's (BF_SPECIAL: forms.inc) and one that runs the
; machine (BF_MACHINE) are jumped to.

ANY             = 255           ; (The most a built-in takes: any number)
B2              = 1             ; (Its code in the second bank, the third or the fourth: BF_BANK's)
B3              = 2
B4              = 3

BI_N            .set 0

; A built-in: its name, the arguments it needs and the most it takes, its flags, its code
.macro BI name, min, max, flags, code
            .ident(.sprintf("bi_min_%d", BI_N)) = min
            .ident(.sprintf("bi_max_%d", BI_N)) = max
            .ident(.sprintf("bi_fl_%d", BI_N)) = flags
            .ident(.sprintf("bi_cd_%d", BI_N)) = code
            .pushseg
            .segment "RODATA2"
            .ident(.sprintf("bi_nm_%d", BI_N)): .byte name, 0
            .popseg
BI_N        .set BI_N + 1
.endmacro

            BI          "fn", 2, 2, B3, bi_fn
            BI          "fun", 2, 2, B3, bi_fun
            BI          "def", 1, ANY, BF_SPECIAL, sf_def
            BI          "set", 1, ANY, BF_SPECIAL, sf_set
            BI          "set!", 1, ANY, BF_SPECIAL, sf_setb
            BI          "let", 1, ANY, BF_SPECIAL, sf_let
            BI          "do", 0, ANY, BF_SPECIAL, sf_do
            BI          "fexpr", 2, 2, B3, bi_fexpr
            BI          "gensym", 0, 1, B2, bo_gensym
            BI          "list", 0, ANY, B2, bl_list
            BI          "head", 1, 1, B2, bl_head
            BI          "tail", 1, 1, B2, bl_tail
            BI          "init", 1, 1, B2, bl_init
            BI          "end", 1, 1, B2, bl_end
            BI          "join", 1, ANY, B2, bl_join
            BI          "eval", 1, 1, BF_SPECIAL, sf_eval
            BI          "len", 1, 1, B2, bl_len
            BI          "item-at", 2, 2, B2, bl_item_at
            BI          "subset", 2, 3, B2, bl_subset
            BI          "reverse", 1, 1, B2, bl_reverse
            BI          "range", 1, 3, B2, bl_range
            BI          "sort", 1, 2, BF_MACHINE, bi_sort
BIN_ADD         = BI_N          ; (+ and *: sum's and product's)
            BI          "+", 0, ANY, B3, bi_add
            BI          "-", 0, ANY, B3, bi_sub
BIN_MUL         = BI_N
            BI          "*", 0, ANY, B3, bi_mul
            BI          "/", 0, ANY, B3, bi_div
            BI          "rational.n", 1, 1, B3, bi_rational_n
            BI          "rational.d", 1, 1, B3, bi_rational_d
            BI          "random", 1, 1, B3, bi_random
            BI          "and", 0, ANY, BF_SPECIAL, sf_and
            BI          "or", 0, ANY, BF_SPECIAL, sf_or
            BI          "while", 1, ANY, BF_SPECIAL, sf_while
            BI          "each", 2, ANY, BF_SPECIAL, sf_each
            BI          "dotimes", 2, ANY, BF_SPECIAL, sf_dotimes
            BI          "try", 1, 2, BF_SPECIAL, sf_try
            BI          "error-message", 1, 1, BF_ERRORS | B3, bi_error_message
            BI          "error-code", 1, 1, BF_ERRORS | B3, bi_error_code
            BI          "if", 2, 3, BF_SPECIAL, sf_if
            BI          "eq", 2, 2, B2, bq_eq
            BI          "neq", 2, 2, B2, bq_neq
            BI          ">", 2, 2, B2, bq_gt
            BI          "<", 2, 2, B2, bq_lt
            BI          "cmp", 2, 2, B2, bq_cmp
            BI          "<=>", 2, ANY, BF_SPECIAL, sf_space
            BI          "load", 1, ANY, BF_MACHINE, bi_load
            BI          "save", 1, ANY, B3, bi_notyet
            BI          "error", 1, 2, B3, bi_error
            BI          "print", 0, ANY, BF_ERRORS | B2, bo_print
            BI          "write", 0, ANY, BF_ERRORS | B2, bo_write
            BI          "output-of", 0, ANY, BF_SPECIAL, sf_output
            BI          "index-of", 2, 2, B2, bl_index_of
            BI          "last-index-of", 2, 2, B2, bl_last_index_of
            BI          "substring", 2, 3, B2, bo_substring
            BI          "char-at", 2, 2, B4, bi_char_at
            BI          "str-split", 1, 2, B4, bi_str_split
            BI          "str-upper", 1, 1, B4, bi_str_upper
            BI          "str-lower", 1, 1, B4, bi_str_lower
            BI          "str-trim", 1, 1, B4, bi_str_trim
            BI          "str-replace", 3, 3, B4, bi_str_replace
            BI          "str-join", 1, 2, B4, bi_str_join
            BI          "str-chars", 1, 1, B4, bi_str_chars
            BI          "str-pad-left", 2, 3, B4, bi_str_pad_left
            BI          "str-pad-right", 2, 3, B4, bi_str_pad_right
            BI          "str-repeat", 2, 2, B4, bi_str_repeat
            BI          "format", 1, ANY, B2, bo_format
            BI          "char-code", 1, 1, B4, bi_char_code
            BI          "code-char", 1, 1, B4, bi_code_char
            BI          "alpha?", 1, 1, B4, bi_alpha_p
            BI          "digit?", 1, 1, B4, bi_digit_p
            BI          "space?", 1, 1, B4, bi_space_p
            BI          "upper?", 1, 1, B4, bi_upper_p
            BI          "lower?", 1, 1, B4, bi_lower_p
            BI          "read", 1, 1, B3, bi_notyet
            BI          "open", 1, 2, B3, bi_notyet
            BI          "close", 1, 1, B3, bi_notyet
            BI          "read-line", 0, 1, B3, bi_notyet
            BI          "read-byte", 0, 1, B3, bi_notyet
            BI          "read-all", 0, 1, B3, bi_notyet
            BI          "seek", 2, 2, B3, bi_notyet
            BI          "tell", 1, 1, B3, bi_notyet
            BI          "print-to", 1, ANY, BF_ERRORS | B3, bi_notyet
            BI          "write-to", 1, ANY, BF_ERRORS | B3, bi_notyet
            BI          "val", 1, 1, B3, bi_val
            BI          "to-fixed", 1, 2, B3, bi_to_fixed
            BI          "to-rational", 1, 1, B3, bi_to_rational
            BI          "truncate", 1, 1, B3, bi_truncate
            BI          "complex", 2, 2, B3, bi_complex
            BI          "to-str", 1, 2, B2, bo_to_str
            BI          "repr", 1, 1, BF_ERRORS | B2, bo_repr
            BI          "to-sym", 1, 1, B2, bo_to_sym
            BI          "to-atom", 1, 1, B2, bo_to_atom
            BI          "fib", 1, 1, B3, bi_fib
            BI          "defined?", 0, 1, BF_SPECIAL, sf_defined
            BI          "type-of", 1, 1, BF_ERRORS | B3, bi_type_of
            BI          "t?", 1, 1, BF_ERRORS | B3, bi_is_t
            BI          "nil?", 1, 1, BF_ERRORS | B3, bi_is_nil
            BI          "num?", 1, 1, BF_ERRORS | B3, bi_is_num
            BI          "fixed?", 1, 1, BF_ERRORS | B3, bi_is_fixed
            BI          "rational?", 1, 1, BF_ERRORS | B3, bi_is_rational
            BI          "int?", 1, 1, BF_ERRORS | B3, bi_is_int
            BI          "complex?", 1, 1, BF_ERRORS | B3, bi_is_complex
            BI          "atom?", 1, 1, BF_ERRORS | B3, bi_is_atom
            BI          "symbol?", 1, 1, BF_ERRORS | B3, bi_is_symbol
            BI          "string?", 1, 1, BF_ERRORS | B3, bi_is_string
            BI          "char?", 1, 1, BF_ERRORS | B3, bi_is_char
            BI          "function?", 1, 1, BF_ERRORS | B3, bi_is_function
            BI          "error?", 1, 1, BF_ERRORS | B3, bi_is_error
            BI          "expr?", 0, 1, BF_SPECIAL, sf_expr
            BI          "qexpr?", 1, 1, BF_ERRORS | B3, bi_is_qexpr
            BI          "sexpr?", 1, 1, BF_ERRORS | B3, bi_is_sexpr
            BI          "hash-create", 0, 1, BF_MACHINE, bi_hash_create
            BI          "hash-get", 2, 2, B4, bi_hash_get
            BI          "hash-put", 2, ANY, BF_MACHINE, bi_hash_put
            BI          "to#", 1, 1, BF_MACHINE, bi_to_hash
            BI          "from#", 1, 1, B4, bi_from_hash
            BI          "hash-key?", 2, 2, B4, bi_hash_key_p
            BI          "hash-keys", 1, 1, B4, bi_hash_keys
            BI          "hash-values", 1, 1, B4, bi_hash_values
            BI          "hash-call", 2, ANY, BF_MACHINE, bi_hash_call
            BI          "hash-clone", 1, ANY, B4, bi_hash_clone
            BI          "hash-remove", 2, 2, B4, bi_hash_remove
            BI          "hash-add-tag", 2, ANY, B4, bi_hash_add_tag
            BI          "hash-lock", 1, ANY, B4, bi_hash_lock
            BI          "hash-make-const", 1, ANY, B4, bi_hash_make_const
            BI          "hash-make-private", 1, ANY, B4, bi_hash_make_private
            BI          "hash-make-not-nil", 1, ANY, B4, bi_hash_make_not_nil
            BI          "hash-tag?", 2, 2, B4, bi_hash_tag_p
            BI          "hash-locked?", 1, 2, B4, bi_hash_locked_p
            BI          "hash-private?", 1, 2, B4, bi_hash_private_p
            BI          "hash-const?", 1, 2, B4, bi_hash_const_p
            BI          "read-file", 1, 1, B3, bi_notyet
            BI          "read-lines", 1, 1, B3, bi_notyet
            BI          "write-file", 1, ANY, B3, bi_notyet
            BI          "append-file", 1, ANY, B3, bi_notyet
            BI          "ls", 0, 1, B3, bi_notyet
            BI          "dir", 0, 1, B3, bi_notyet
            BI          "stat", 1, 1, B3, bi_notyet
            BI          "exists?", 1, 1, B3, bi_notyet
            BI          "dir?", 1, 1, B3, bi_notyet
            BI          "file?", 1, 1, B3, bi_notyet
            BI          "mkdir", 1, 1, B3, bi_notyet
            BI          "remove", 1, 1, B3, bi_notyet
            BI          "rename", 2, 2, B3, bi_notyet
            BI          "copy-file", 2, 2, B3, bi_notyet
            BI          "cd", 0, 1, B3, bi_notyet
            BI          "cwd", 0, 0, B3, bi_notyet
            BI          "glob", 1, 1, B3, bi_notyet
            BI          "run", 1, ANY, B3, bi_notyet
            BI          "sh", 1, 2, B3, bi_notyet
            BI          "sh-out", 1, 2, B3, bi_notyet
            BI          "spawn", 1, ANY, B3, bi_notyet
            BI          "wait", 1, 1, B3, bi_notyet
            BI          "kill", 1, 1, B3, bi_notyet
            BI          "pid", 0, 0, B3, bi_notyet
            BI          "env", 0, 1, B3, bi_notyet
            BI          "setenv", 2, 2, B3, bi_notyet
            BI          "unsetenv", 1, 1, B3, bi_notyet
            BI          "time", 0, 0, B3, bi_notyet
            BI          "date", 0, 1, B3, bi_notyet
            BI          "date-parts", 0, 1, B3, bi_notyet
            BI          "seconds-of", 3, 6, B3, bi_notyet
            BI          "ticks", 0, 0, B3, bi_notyet
            BI          "tick-rate", 0, 0, B3, bi_notyet
            BI          "sleep", 1, 1, B3, bi_notyet
            BI          "bit-and", 1, ANY, B3, bi_bit_and
            BI          "bit-or", 1, ANY, B3, bi_bit_or
            BI          "bit-xor", 1, ANY, B3, bi_bit_xor
            BI          "bit-not", 1, 1, B3, bi_bit_not
            BI          "shl", 2, 2, B3, bi_shl
            BI          "shr", 2, 2, B3, bi_shr
            BI          "bit?", 2, 2, B3, bi_bit_p
            BI          "hex", 1, 2, B3, bi_hex
            BI          "bin", 1, 2, B3, bi_bin
            BI          "lo", 1, 1, B3, bi_lobyte
            BI          "hi", 1, 1, B3, bi_hibyte
            BI          "word", 2, 2, B3, bi_word
            BI          "bytes", 1, 1, B3, bi_bytes
            BI          "from-bytes", 1, 1, B3, bi_from_bytes
            BI          "read-bytes", 2, 2, B3, bi_notyet
            BI          "write-bytes", 2, 2, B3, bi_notyet
            BI          "platform", 0, 0, B3, bi_notyet
            BI          "hydra?", 0, 0, B3, bi_notyet
            BI          "not", 1, 1, B3, bi_not
            BI          "==", 2, 2, B2, bq_eq
            BI          ">=", 2, 2, B2, bq_ge
            BI          "<=", 2, 2, B2, bq_le
            BI          "neg?", 1, 1, B2, bq_negp
            BI          "pos?", 1, 1, B2, bq_posp
            BI          "zero?", 1, 1, B2, bq_zerop
            BI          "one?", 1, 1, B2, bq_onep
            BI          "1+", 1, 1, B3, bi_inc
            BI          "1-", 1, 1, B3, bi_dec
            BI          "abs", 1, 1, B3, bi_abs
            BI          "cons", 2, 2, B2, bl_cons
            BI          "fst", 1, 1, B2, bl_fst
            BI          "snd", 1, 1, B2, bl_snd
            BI          "thd", 1, 1, B2, bl_thd
            BI          "nth", 2, 2, B2, bl_nth
            BI          "last", 1, 1, B2, bl_last
            BI          "take", 2, 2, B2, bl_take
            BI          "drop", 2, 2, B2, bl_drop
            BI          "elem?", 2, 2, B2, bq_elem
            BI          "in?", 2, 2, B2, bq_in
            BI          "map", 2, 2, BF_MACHINE, bi_map
            BI          "filter", 2, 2, BF_MACHINE, bi_filter
            BI          "foldl", 3, 3, BF_MACHINE, bi_foldl
            BI          "foldr", 3, 3, BF_MACHINE, bi_foldr
            BI          "any?", 2, 2, BF_MACHINE, bi_any
            BI          "all?", 2, 2, BF_MACHINE, bi_all
            BI          "find", 2, 2, BF_MACHINE, bi_find
            BI          "count", 2, 2, BF_MACHINE, bi_count_if
            BI          "sum", 1, 1, BF_MACHINE, bi_sum
            BI          "product", 1, 1, BF_MACHINE, bi_product
            BI          "min", 1, ANY, B2, bq_min
            BI          "max", 1, ANY, B2, bq_max

BI_COUNT        = BI_N

.rodata
; The table: each built-in's least and most arguments, flags, code
bi_min:
.repeat BI_COUNT, I
            .byte       .ident(.sprintf("bi_min_%d", I))
.endrep
bi_max:
.repeat BI_COUNT, I
            .byte       .ident(.sprintf("bi_max_%d", I))
.endrep
bi_flags:
.repeat BI_COUNT, I
            .byte       .ident(.sprintf("bi_fl_%d", I))
.endrep
bi_lo:
.repeat BI_COUNT, I
            .byte       <.ident(.sprintf("bi_cd_%d", I))
.endrep
bi_hi:
.repeat BI_COUNT, I
            .byte       >.ident(.sprintf("bi_cd_%d", I))
.endrep

.segment "RODATA2"
; Their names
bi_names_lo:
.repeat BI_COUNT, I
            .byte       <.ident(.sprintf("bi_nm_%d", I))
.endrep
bi_names_hi:
.repeat BI_COUNT, I
            .byte       >.ident(.sprintf("bi_nm_%d", I))
.endrep

.segment "CODE2"
; Each built-in bound to its name (as its global value).  OUT: C = 1 if there's no room
bi_bind:
            ldx         #0
@one:
            cpx         #BI_COUNT
            bcs         @done
            phx
            lda         bi_names_lo,x                       ; (Its name: its length)
            sta         hq
            lda         bi_names_hi,x
            sta         hq + 1
            ldy         #0
:
            lda         (hq),y
            beq         :+
            iny
            bne         :-
:
            sty         hn
            stz         hn + 1
            lda         #PK_SYMBOL
            jsr         intern
            plx
            bcs         @rts
            phx
            lda         hv                                  ; (Its value: BUILTIN0 + 2 * x)
            ldx         hv + 1
            jsr         deref
            pla
            pha
            asl
            ldy         #2
            sta         (hp),y
            lda         #0
            rol
            ora         #>BUILTIN0
            iny
            sta         (hp),y
            plx
            inx
            bra         @one
@done:
            clc
@rts:
            rts

; Built-in .X's name out.  Keeps .X
bi_name:
            phx
            lda         bi_names_lo,x
            pha
            lda         bi_names_hi,x
            tax
            pla
            jsr         out_text
            plx
            rts

.code


.bss
fe_text:    .res        2                                   ; (fn's error's text)
fe_n:       .res        2                                   ; (takes_extras': values on the stack, the one looked at)
te_v:       .res        2
nm_acc:     .res        2                                   ; (The numbers': the result so far, the next number, a
nm_n:       .res        2                                   ;   divisor, a product or remainder, its sign, past a
nm_m:       .res        2                                   ;   fixnum, the operator, the argument at)
nm_p:       .res        4
nm_sign:    .res        1
nm_over:    .res        1
nm_op:      .res        1
nm_i:       .res        2
nm_args:    .res        4                                   ; (1+'s and 1-'s arguments: x and 1)


.code
