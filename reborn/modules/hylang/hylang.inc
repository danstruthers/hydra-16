; ****************************************************************************
; hylang.inc - hylang's values and its kinds of cell (heap.inc says what they are).  A value is a word: a fixnum
; (bit 0 set), an immediate (a reference below IMM_PAGES * 256) or a cell's reference.

; The immediates
NIL             = $0000         ; The empty list (false)
T_VAL           = $0002         ; T
SNIL            = $0004         ; The empty S-expression, ()
EXIT_VAL        = $0006         ; exit
UNBOUND         = $0008         ; (A symbol's global value when it has none)
CHAR0           = $0200         ; The characters: CHAR0 + 2 * the code
BUILTIN0        = $0400         ; The built-ins: BUILTIN0 + 2 * the number
IMM_PAGES       = 6             ; (The pages below this one are the immediates': no memory behind them)

.define FIX(n) (((n) * 2 + 1) & $FFFF)                      ; A fixnum's value

; What follows in the collector's segment: hylang's sixth bank (CODE6: entered by GC_CALL, a far call), or, in another
; module (t_heap's), the heap's (DATA)
.macro GC_SEG
.ifdef HL_DATA4
.segment "CODE6"
.else
.segment "DATA"
.endif
.endmacro

; A collection (gc_collect), from any bank or the RAM
.macro GC_CALL
.ifdef HL_DATA4
            FARN        6, gc_collect
.else
            jsr         gc_collect
.endif
.endmacro

; What follows in the segment of the state a snapshot keeps (hylang's PSTATE, hylang.cfg: the heap's and the
; evaluator's; tools/hysnap.js copies it out, snap_restore back), or, in another module (t_heap's), in the BSS
.macro PSTATE_SEG
.ifdef HL_DATA4
.segment "PSTATE"
.else
.bss
.endif
.endmacro

; .A, .X = an address (low, high)
.macro LDAX addr
            lda         #<(addr)
            ldx         #>(addr)
.endmacro

; A word copied: to = from
.macro MOVW to, from
            lda         from
            sta         to
            lda         from + 1
            sta         to + 1
.endmacro

; hq = an address, hn = a count (intern's, string_make's)
.macro LDHQ addr, count
            lda         #<(addr)
            sta         hq
            lda         #>(addr)
            sta         hq + 1
            lda         #<(count)
            sta         hn
            lda         #>(count)
            sta         hn + 1
.endmacro
FIX_MIN         = -16384        ; (A fixnum's range)
FIX_MAX         = 16383

; The pages' kinds (pk): a page of the cell heap holds one kind of cell
PK_NONE         = 0             ; A bank not taken yet
PK_IMM          = 1             ; The immediates' pages
PK_FREE         = 2             ; A free page
PK_SCONS        = 3             ; (The cells' kinds, from here: kd_* say what they are) An S-expression's cons
PK_QCONS        = 4             ; A Q-expression's (a list's) cons
PK_SCOPED       = 5             ; A Q-expression and the scope it was written in (NIL: the global one); an fexpr's
                                ;   argument (an S-expression, a symbol) and its caller's scope
PK_STRING       = 6             ; A string: its length, its blob
PK_BIGNUM       = 7             ; An integer past a fixnum: its sign and length, its blob
PK_FIXED        = 8             ; A fixed decimal: its digits (an integer), its places (a fixnum)
PK_RATIO        = 9             ; A rational: its numerator, its denominator
PK_COMPLEX      = 10            ; A complex number: its real part, its imaginary part
PK_ERROR        = 11            ; An error: its message, its code
PK_PARTIAL      = 12            ; A partially applied built-in: the built-in, the values given it
PK_SYMBOL       = 13            ; A symbol: its name (a blob), its global value, the next in its bucket, flags
PK_ATOM         = 14            ; An atom: the same
PK_FUNC         = 15            ; A function: its formals, its body, its scope, flags (and its formals' count)
PK_HASH         = 16            ; A hash: its items (its entries, then its tags), a proxy's hash, its count, flags
PK_STREAM       = 17            ; A stream: its fd and its flags (a word)
PK_FRAME8       = 18            ; A scope's frame: its parent (NIL: the global scope), the cell its pairs go on in
PK_FRAME16      = 19            ;   (NIL: none), then names and values in pairs (a name NIL: no more), of 8 bytes (1
PK_FRAME32      = 20            ;   pair), 16 (3), 32 (7) ...
PK_FRAME64      = 21            ;   and 64 (15)
PK_BUFFER       = 22            ; A buffer: its length, its blob (a string's, but changed in place: buffers.inc)
PK_KINDS        = 23
PK_FIXNUM       = $FF           ; (kind_of's, for a fixnum: no page)

; A symbol's flags (its word 3)
SF_LOCAL        = $01           ; It's been bound in a frame (else it's looked up at its global value at once)

; A function's flags (its word 3's low byte; its high byte its formals counted, as it's made)
FN_FEXPR        = $01           ; Its arguments come as they're written (each with its caller's scope)
FN_EXTRAS       = $02           ; Its body names &_ or &1, &2 ...: it takes arguments past its formals

; A built-in's flags (builtins.inc's bi_flags)
BF_SPECIAL      = $80           ; A special form: its arguments as they're written (its code runs the machine)
BF_ERRORS       = $40           ; It takes errors as values (error?, type-of, repr ...)
BF_MACHINE      = $20           ; Its code runs the machine (map ...): jumped to, its values on the stack
BF_FAST         = $10           ; Fixnums worked in the evaluator's bank first (eval.inc's bi_fast), without a far call
BF_BANK         = $07           ; Its code's bank, less 1 (called through FARN: it gives back a value)
