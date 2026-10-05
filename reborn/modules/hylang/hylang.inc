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
FIX_MIN         = -16384        ; (A fixnum's range)
FIX_MAX         = 16383

; The pages' kinds (pk): a page of the cell heap holds one kind of cell
PK_NONE         = 0             ; A bank not taken yet
PK_IMM          = 1             ; The immediates' pages
PK_FREE         = 2             ; A free page
PK_SCONS        = 3             ; (The cells' kinds, from here: kd_* say what they are) An S-expression's cons
PK_QCONS        = 4             ; A Q-expression's (a list's) cons
PK_SCOPED       = 5             ; A Q-expression and the scope it was written in
PK_STRING       = 6             ; A string: its length, its blob
PK_BIGNUM       = 7             ; An integer past a fixnum: its sign and length, its blob
PK_FIXED        = 8             ; A fixed decimal: its digits (an integer), its places (a fixnum)
PK_RATIO        = 9             ; A rational: its numerator, its denominator
PK_COMPLEX      = 10            ; A complex number: its real part, its imaginary part
PK_ERROR        = 11            ; An error: its message, its code
PK_PARTIAL      = 12            ; A partially applied built-in: the built-in, the values given it
PK_SYMBOL       = 13            ; A symbol: its name (a blob), its global value, the next in its bucket, flags
PK_ATOM         = 14            ; An atom: the same
PK_FUNC         = 15            ; A function: its formals, its body, its scope, flags
PK_HASH         = 16            ; A hash: its table (a blob), its tags, its count, flags
PK_STREAM       = 17            ; A stream: its fd, its mode, its buffer (a blob), its place
PK_FRAME8       = 18            ; A scope's frame: its parent, its count, its names' values in pairs ...
PK_FRAME16      = 19            ;   of 16 bytes (3 pairs) ...
PK_FRAME32      = 20            ;   32 (7) ...
PK_FRAME64      = 21            ;   and 64 (15)
PK_KINDS        = 22
PK_FIXNUM       = $FF           ; (kind_of's, for a fixnum: no page)
