; ****************************************************************************
; builtins.inc - hylang's built-ins (phase 3): the table of all danlang's (Builtins.cs's Arity: each one's name,
; the arguments it needs and the most it takes, its flags, its code), and the code of those in the first bank.  A
; built-in is the value BUILTIN0 + 2 * its number; each is bound to its name as hylang starts (bi_bind).  The first
; 256 are worked by their number (a byte) as it is; those past them (the second table: BUILTIN2 and up) are ordinary
; ones whose code is in another bank, which the evaluator calls its own way (eval.inc's ev_bapply2) and the bytecode
; machine through it.
;   An ordinary built-in's code gets its values at argp (cn of them, on the stack: roots), ee the caller's scope;
; it gives back ex, or C = 1 if there's no room.  A special form's (BF_SPECIAL: forms.inc) and one that runs the
; machine (BF_MACHINE) are jumped to.

ANY             = 255           ; (The most a built-in takes: any number)
B2              = 1             ; (Its code in the second bank, the third, the fourth or the fifth: BF_BANK's)
B3              = 2
B4              = 3
B5              = 4
B6              = 5

BI_N            .set 0

; A built-in: its name, the arguments it needs and the most it takes, its flags, its code
.macro BI name, min, max, flags, code
.if BI_N >= 256
.assert     ((flags) & (BF_SPECIAL | BF_ERRORS | BF_MACHINE | BF_FAST)) = 0 .and ((flags) & BF_BANK) <> 0, error, .sprintf("builtins.inc: %s: past the first 256, an ordinary built-in in another bank", name)
.endif
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

BIN_FN          = BI_N          ; (These the bytecode machine's compiler knows: vm.inc's)
            BI          "fn", 2, 2, B3, bi_fn
BIN_FUN         = BI_N
            BI          "fun", 2, 2, B3, bi_fun
BIN_DEF         = BI_N
            BI          "def", 1, ANY, BF_SPECIAL, sf_def
BIN_SET         = BI_N
            BI          "set", 1, ANY, BF_SPECIAL, sf_set
BIN_SETB        = BI_N
            BI          "set!", 1, ANY, BF_SPECIAL, sf_setb
BIN_LET         = BI_N
            BI          "let", 1, ANY, BF_SPECIAL, sf_let
BIN_DO          = BI_N
            BI          "do", 0, ANY, BF_SPECIAL, sf_do
BIN_FEXPR       = BI_N
            BI          "fexpr", 2, 2, B3, bi_fexpr
            BI          "gensym", 0, 1, B2, bo_gensym
BIN_LIST        = BI_N
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
BIN_ADD         = BI_N          ; (+ and *: sum's and product's; those BF_FAST: bi_fast's)
            BI          "+", 0, ANY, B3 | BF_FAST, bi_add
BIN_SUB         = BI_N
            BI          "-", 0, ANY, B3 | BF_FAST, bi_sub
BIN_MUL         = BI_N
            BI          "*", 0, ANY, B3, bi_mul
            BI          "/", 0, ANY, B3, bi_div
            BI          "rational.n", 1, 1, B3, bi_rational_n
            BI          "rational.d", 1, 1, B3, bi_rational_d
            BI          "random", 1, 1, B3, bi_random
BIN_AND         = BI_N
            BI          "and", 0, ANY, BF_SPECIAL, sf_and
BIN_OR          = BI_N
            BI          "or", 0, ANY, BF_SPECIAL, sf_or
BIN_WHILE       = BI_N
            BI          "while", 1, ANY, BF_SPECIAL, sf_while
BIN_EACH        = BI_N
            BI          "each", 2, ANY, BF_SPECIAL, sf_each
BIN_DOTIMES     = BI_N
            BI          "dotimes", 2, ANY, BF_SPECIAL, sf_dotimes
BIN_TRY         = BI_N
            BI          "try", 1, 2, BF_SPECIAL, sf_try
            BI          "error-message", 1, 1, BF_ERRORS | B3, bi_error_message
            BI          "error-code", 1, 1, BF_ERRORS | B3, bi_error_code
BIN_IF          = BI_N
            BI          "if", 2, 3, BF_SPECIAL, sf_if
            BI          "eq", 2, 2, B2, bq_eq
            BI          "neq", 2, 2, B2, bq_neq
BIN_GT          = BI_N
            BI          ">", 2, 2, B2 | BF_FAST, bq_gt
BIN_LT          = BI_N
            BI          "<", 2, 2, B2 | BF_FAST, bq_lt
            BI          "cmp", 2, 2, B2, bq_cmp
            BI          "<=>", 2, ANY, BF_SPECIAL, sf_space
BIN_LOAD        = BI_N          ; (A script's: hylang file)
            BI          "load", 1, ANY, BF_MACHINE, bi_load
            BI          "save", 1, ANY, B5, bi_save
            BI          "error", 1, 2, B3, bi_error
            BI          "print", 0, ANY, BF_ERRORS | B2, bo_print
            BI          "write", 0, ANY, BF_ERRORS | B2, bo_write
            BI          "output-of", 0, ANY, BF_SPECIAL, sf_output
            BI          "index-of", 2, 2, B2, bl_index_of
            BI          "last-index-of", 2, 2, B2, bl_last_index_of
            BI          "substring", 2, 3, B2, bo_substring
BIN_CHARAT      = BI_N          ; (char-at, char-code and the bits': vmx.inc's, BCALL's templates)
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
BIN_CHARCODE    = BI_N
            BI          "char-code", 1, 1, B4, bi_char_code
            BI          "code-char", 1, 1, B4, bi_code_char
            BI          "alpha?", 1, 1, B4, bi_alpha_p
            BI          "digit?", 1, 1, B4, bi_digit_p
            BI          "space?", 1, 1, B4, bi_space_p
            BI          "upper?", 1, 1, B4, bi_upper_p
            BI          "lower?", 1, 1, B4, bi_lower_p
            BI          "read", 1, 1, B5, bi_read
            BI          "open", 1, 2, B5, bi_open
            BI          "close", 1, 1, B5, bi_close
            BI          "read-line", 0, 1, B5, bi_read_line
            BI          "read-byte", 0, 1, B5, bi_read_byte
            BI          "read-all", 0, 1, B5, bi_read_all
            BI          "seek", 2, 2, B5, bi_seek
            BI          "tell", 1, 1, B5, bi_tell
            BI          "print-to", 1, ANY, BF_ERRORS | B5, bi_print_to
            BI          "write-to", 1, ANY, BF_ERRORS | B5, bi_write_to
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
BIN_HASH_CREATE = BI_N
            BI          "hash-create", 0, 1, BF_MACHINE, bi_hash_create
            BI          "hash-get", 2, 2, B4, bi_hash_get
BIN_HASH_PUT    = BI_N
            BI          "hash-put", 2, ANY, BF_MACHINE, bi_hash_put
BIN_TO_HASH     = BI_N
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
            BI          "read-file", 1, 1, B5, bi_read_file
            BI          "read-lines", 1, 1, B5, bi_read_lines
            BI          "write-file", 1, ANY, B5, bi_write_file
            BI          "append-file", 1, ANY, B5, bi_append_file
            BI          "ls", 0, 1, B5, bi_ls
            BI          "dir", 0, 1, B5, bi_dir
            BI          "stat", 1, 1, B5, bi_stat
            BI          "exists?", 1, 1, B5, bi_exists_p
            BI          "dir?", 1, 1, B5, bi_dir_p
            BI          "file?", 1, 1, B5, bi_file_p
            BI          "mkdir", 1, 1, B5, bi_mkdir
            BI          "remove", 1, 1, B5, bi_remove
            BI          "rename", 2, 2, B5, bi_rename
            BI          "copy-file", 2, 2, B5, bi_copy_file
            BI          "cd", 0, 1, B5, bi_cd
            BI          "cwd", 0, 0, B5, bi_cwd
            BI          "glob", 1, 1, B5, bi_glob
            BI          "run", 1, ANY, B5, bi_run
            BI          "sh", 1, 2, B5, bi_sh
            BI          "sh-out", 1, 2, B5, bi_sh_out
            BI          "spawn", 1, ANY, B5, bi_spawn
            BI          "wait", 1, 1, B5, bi_wait
            BI          "kill", 1, 1, B5, bi_kill
            BI          "pid", 0, 0, B5, bi_pid
            BI          "env", 0, 1, B5, bi_env
            BI          "setenv", 2, 2, B5, bi_setenv
            BI          "unsetenv", 1, 1, B5, bi_unsetenv
            BI          "time", 0, 0, B5, bi_time
            BI          "date", 0, 1, B5, bi_date
            BI          "date-parts", 0, 1, B5, bi_date_parts
            BI          "seconds-of", 3, 6, B5, bi_seconds_of
            BI          "ticks", 0, 0, B5, bi_ticks
            BI          "tick-rate", 0, 0, B5, bi_tick_rate
            BI          "sleep", 1, 1, B5, bi_sleep
BIN_BITAND      = BI_N
            BI          "bit-and", 1, ANY, B3, bi_bit_and
BIN_BITOR       = BI_N
            BI          "bit-or", 1, ANY, B3, bi_bit_or
BIN_BITXOR      = BI_N
            BI          "bit-xor", 1, ANY, B3, bi_bit_xor
            BI          "bit-not", 1, 1, B3, bi_bit_not
BIN_SHL         = BI_N
            BI          "shl", 2, 2, B3, bi_shl
BIN_SHR         = BI_N
            BI          "shr", 2, 2, B3, bi_shr
            BI          "bit?", 2, 2, B3, bi_bit_p
            BI          "hex", 1, 2, B3, bi_hex
            BI          "bin", 1, 2, B3, bi_bin
            BI          "lo", 1, 1, B3, bi_lobyte
            BI          "hi", 1, 1, B3, bi_hibyte
            BI          "word", 2, 2, B3, bi_word
            BI          "bytes", 1, 1, B3, bi_bytes
            BI          "from-bytes", 1, 1, B3, bi_from_bytes
            BI          "read-bytes", 2, 2, B5, bi_read_bytes
            BI          "write-bytes", 2, 4, B5, bi_write_bytes
            BI          "platform", 0, 0, B5, bi_platform
            BI          "hydra?", 0, 0, B5, bi_hydra_p
            BI          "not", 1, 1, B3, bi_not
BIN_EQ          = BI_N
            BI          "==", 2, 2, B2 | BF_FAST, bq_eq
BIN_GE          = BI_N
            BI          ">=", 2, 2, B2 | BF_FAST, bq_ge
BIN_LE          = BI_N
            BI          "<=", 2, 2, B2 | BF_FAST, bq_le
            BI          "neg?", 1, 1, B2, bq_negp
            BI          "pos?", 1, 1, B2, bq_posp
BIN_ZEROP       = BI_N
            BI          "zero?", 1, 1, B2 | BF_FAST, bq_zerop
BIN_ONEP        = BI_N
            BI          "one?", 1, 1, B2 | BF_FAST, bq_onep
BIN_INC         = BI_N
            BI          "1+", 1, 1, B3 | BF_FAST, bi_inc
BIN_DEC         = BI_N
            BI          "1-", 1, 1, B3 | BF_FAST, bi_dec
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
BIN_MAP         = BI_N          ; (map to product: forms.inc's walks, W_MAP on; the machine's for BCALL, vm.inc's)
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
            BI          "sysinfo", 0, 0, B5, bi_sysinfo     ; (The Hydra's: hydra.inc)
            BI          "mods", 0, 0, B5, bi_mods
            BI          "errstr", 1, 1, B5, bi_errstr
            BI          "ps", 0, 0, B5, bi_ps
            BI          "task-info", 1, 1, B5, bi_task_info
            BI          "yield", 0, 0, B5, bi_yield
            BI          "sleep-until", 1, 1, B5, bi_sleep_until
            BI          "peek", 1, 1, B5, bi_peek
            BI          "poke", 2, 2, B5, bi_poke
            BI          "peek-word", 1, 1, B5, bi_peek_word
            BI          "poke-word", 2, 2, B5, bi_poke_word
            BI          "banks", 0, 0, B5, bi_banks
            BI          "bank-alloc", 1, 1, B5, bi_bank_alloc
            BI          "bank-free", 2, 2, B5, bi_bank_free
            BI          "bank-read", 3, 3, B5, bi_bank_read
            BI          "bank-write", 3, 3, B5, bi_bank_write
            BI          "seg-create", 1, 1, B5, bi_seg_create
            BI          "seg-attach", 1, 1, B5, bi_seg_attach
            BI          "seg-detach", 1, 1, B5, bi_seg_detach
            BI          "seg-read", 4, 4, B5, bi_seg_read
            BI          "seg-write", 4, 4, B5, bi_seg_write
            BI          "free", 0, 0, B5, bi_free
            BI          "bind", 2, 4, B5, bi_bindns
            BI          "mount", 2, 5, B5, bi_mount
            BI          "unmount", 1, 2, B5, bi_unmount
            BI          "ns", 0, 0, B5, bi_ns
            BI          "newns", 0, 0, BF_SPECIAL, sf_newns
            BI          "note", 2, 2, B5, bi_note
            BI          "note-group", 2, 2, B5, bi_note_group
            BI          "on-note", 1, 1, B5, bi_on_note
            BI          "key", 0, 0, B5, bi_key
            BI          "key?", 0, 0, B5, bi_keyp
            BI          "hold", 0, ANY, BF_SPECIAL, sf_hold
BIN_SYS         = BI_N          ; (The sys- functions': hysys.inc's sys_lazy)
            BI          "sys", 1, ANY, B6, bi_sys
            BI          "buffer", 1, 2, B6, bi_buffer           ; (Buffers: buffers.inc's; buffer? a type's test)
            BI          "buffer?", 1, 1, BF_ERRORS | B3, bi_is_buffer
            BI          "buffer-get", 2, 2, B6, bi_buffer_get
BIN_BUFPUT      = BI_N
            BI          "buffer-put", 3, 3, B6, bi_buffer_put
            BI          "buffer-fill", 2, 4, B6, bi_buffer_fill
            BI          "buffer-copy", 3, 5, B6, bi_buffer_copy
            BI          "read-buffer", 2, 4, B6, bi_read_buffer
.assert     BI_N <= 256, error, "builtins.inc: the first table's 256 built-ins at most"
BI_N        .set 256        ; (The second table: ordinary built-ins in a bank, BUILTIN2 and up)
            BI          "base", 0, 1, B3, bi_base              ; (The numbers': numbi.inc's)
            BI          "number-bytes", 1, 1, B3, bi_number_bytes
            BI          "bytes-number", 1, 1, B3, bi_bytes_number
            BI          "sqrt", 1, 1, B3, bi_sqrt               ; (The math functions: numbi.inc's, the math library's)
            BI          "exp", 1, 1, B3, bi_exp
            BI          "log", 1, 1, B3, bi_log
            BI          "sin", 1, 1, B3, bi_sin
            BI          "cos", 1, 1, B3, bi_cos
            BI          "tan", 1, 1, B3, bi_tan
            BI          "atan", 1, 1, B3, bi_atan
            BI          "pi", 0, 0, B3, bi_pi
            BI          "digits", 0, 1, B3, bi_digits
            BI          "pow", 2, 2, B3, bi_pow

BI_COUNT        = BI_N
.repeat 256, I                                              ; (The first table's unused entries: none)
.ifndef .ident(.sprintf("bi_min_%d", I))
            .ident(.sprintf("bi_min_%d", I)) = 0
            .ident(.sprintf("bi_max_%d", I)) = 0
            .ident(.sprintf("bi_fl_%d", I)) = 0
            .ident(.sprintf("bi_cd_%d", I)) = 0
            .ident(.sprintf("bi_nm_%d", I)) = 0
.endif
.endrep
.assert     BI_COUNT <= 512, error, "builtins.inc: 512 built-ins at most (BUILTIN0 + 2 * n, below IMM_PAGES)"
.if BI_COUNT > 256
BI_FIRST        = 256           ; (The first table's: its entries here, and in the bytecode machine's bank; the second's
.else                           ;   in the second bank, bi_xinfo's)
BI_FIRST        = BI_COUNT
.endif

.rodata
; The table: each built-in's least and most arguments, flags, code (the first 256's: the second table's in the second
; bank, bi_xinfo's)
bi_min:
.repeat BI_FIRST, I
            .byte       .ident(.sprintf("bi_min_%d", I))
.endrep
bi_max:
.repeat BI_FIRST, I
            .byte       .ident(.sprintf("bi_max_%d", I))
.endrep
bi_flags:
.repeat BI_FIRST, I
            .byte       .ident(.sprintf("bi_fl_%d", I))
.endrep
bi_lo:
.repeat BI_FIRST, I
            .byte       <.ident(.sprintf("bi_cd_%d", I))
.endrep
bi_hi:
.repeat BI_FIRST, I
            .byte       >.ident(.sprintf("bi_cd_%d", I))
.endrep

.segment "RODATA2"
; The second table's: built-in 256 + I's least and most arguments, flags, code
.if BI_COUNT > 256
bi_xmin:
.repeat BI_COUNT - 256, I
            .byte       .ident(.sprintf("bi_min_%d", 256 + I))
.endrep
bi_xmax:
.repeat BI_COUNT - 256, I
            .byte       .ident(.sprintf("bi_max_%d", 256 + I))
.endrep
bi_xflags:
.repeat BI_COUNT - 256, I
            .byte       .ident(.sprintf("bi_fl_%d", 256 + I))
.endrep
bi_xlo:
.repeat BI_COUNT - 256, I
            .byte       <.ident(.sprintf("bi_cd_%d", 256 + I))
.endrep
bi_xhi:
.repeat BI_COUNT - 256, I
            .byte       >.ident(.sprintf("bi_cd_%d", 256 + I))
.endrep
.endif
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
; Each built-in bound to its name (as its global value): the first table's (bi_t2 0), the second's (bi_t2 1).
; OUT: C = 1 if there's no room
bi_bind:
            stz         bi_t2
            ldx         #0
@one:
            phx
            lda         bi_t2                               ; (Its name: its length)
            bne         :+
            lda         bi_names_lo,x
            sta         hq
            lda         bi_names_hi,x
            sta         hq + 1
            bra         :++
:
            lda         bi_names_lo + 256,x
            sta         hq
            lda         bi_names_hi + 256,x
            sta         hq + 1
:
            lda         hq + 1                              ; (None: an unused entry's)
            bne         :+
            plx
            bra         @next
:
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
            lda         hv                                  ; (Its value: BUILTIN0 + 2 * x, BUILTIN2 + 2 * x)
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
            ldx         bi_t2
            beq         :+
            ora         #>BUILTIN2
:
            iny
            sta         (hp),y
            plx
@next:
            inx
            lda         bi_t2
            bne         @second
            txa                                             ; (The first table: till .X is 0 again, or its last)
            beq         @first_done
            .if BI_COUNT < 256
            cpx         #<BI_COUNT
            bne         @one
            clc
            rts
            .else
            bra         @one
            .endif
@first_done:
            .if BI_COUNT > 256
            inc         bi_t2                               ; (The second table)
            ldx         #0
            bra         @one
            .else
            clc
            rts
            .endif
@second:
            cpx         #<(BI_COUNT - 256)
            bne         @one
            clc
@rts:
            rts

; The second table's built-in .X (the built-in 256 + .X): bx_min, bx_max, bx_flags, bx_lo, bx_hi its entry.  Keeps .X
bi_xinfo:
.if BI_COUNT > 256
            lda         bi_xmin,x
            sta         bx_min
            lda         bi_xmax,x
            sta         bx_max
            lda         bi_xflags,x
            sta         bx_flags
            lda         bi_xlo,x
            sta         bx_lo
            lda         bi_xhi,x
            sta         bx_hi
.endif
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

; The second table's built-in .X's name out (the built-in 256 + .X).  Keeps .X
bi_name2:
            phx
            lda         bi_names_lo + 256,x
            pha
            lda         bi_names_hi + 256,x
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
