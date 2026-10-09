; numbers.inc - the number libraries' entries, constants and call macros, for programs in assembly.  Made by tools/apigen.js from spec/: don't edit
; A call (spec/numbers.def): XCALL, r15 the entry (its address here), r14 its library's bank (MODINFO finds it,
; by the library's name), r13 the libraries' bank (a bank of the program's, which NUM_INIT fills); r0-r3 the
; operands, the result's place and its room; .A/.X the result's length, C = 0; or C = 1 and .A an error (NE_).
; NUMCALL NUM_ADD (an entry of the numbers library's) and MATHCALL MATH_SQRT (the math library's) make one, .A,
; .X and .Y the entry's, from three bytes of the program's: num_bank (r13), num_mod and math_mod (r14), which
; numlib.s's num_open sets (or the program's own code).

.macro NUMCALL     entry
            pha
            lda         #<(entry)
            sta         r15
            lda         #>(entry)
            sta         r15 + 1
            lda         num_mod
            sta         r14
            lda         num_bank
            sta         r13
            pla
            jsr         XCALL
.endmacro

.macro MATHCALL    entry
            pha
            lda         #<(entry)
            sta         r15
            lda         #>(entry)
            sta         r15 + 1
            lda         math_mod
            sta         r14
            lda         num_bank
            sta         r13
            pla
            jsr         XCALL
.endmacro

; ---- numbers: The number system: danlang's and hylang's (integers of any size, fixed decimals, rationals, complex numbers), in the stored format
NUM_INIT        = $A030       ; r13 = the bank
NUM_SET_BASE    = $A033       ; r0 = a base string (its bytes and a 0: "x", "#x", "#16r", "c", "<x", "=16r", "[01]" ...)
                              ; -> .A = what it is: bit 0 decimal (0-9, not balanced, least digit first or negative), bit 1 its prefix written
NUM_GET_BASE    = $A036       ; r2 = a place for its string; r3 = its room
                              ; -> .A/.X = its length (the 0 after it not counted)
NUM_PARSE       = $A039       ; r0 = text; r1 = its length; r4 = a base string (0: the base); .Y bit 0 = 1 if the text is a program's (a bare number must start with a digit 0-9 there), bit 1 = 1 if the whole text must be the number
                              ; -> .A/.X = the number's length; r5 = the characters it used (from the start, spaces around it passed over)
NUM_DISPLAY     = $A03C       ; r0 = a number; r4 = a base string (0: the base)
                              ; -> .A/.X = the text's length
NUM_FORMAT      = $A03F       ; r0 = a format string (its bytes and a 0); r4 = its arguments: a table of 3-byte entries (a kind: 0 a number, 1 a string with a 0 after it; then its address), one for each placeholder, then a kind of $FF
                              ; -> .A/.X = the text's length
NUM_ADD         = $A042       ; r0 = x; r1 = y
                              ; -> .A/.X = the result's length
NUM_SUB         = $A045       ; r0 = x; r1 = y
                              ; -> .A/.X = the result's length
NUM_MUL         = $A048       ; r0 = x; r1 = y
                              ; -> .A/.X = the result's length
NUM_DIV         = $A04B       ; r0 = x; r1 = y
                              ; -> .A/.X = the result's length
NUM_NEG         = $A04E       ; r0 = x
                              ; -> .A/.X = the result's length
NUM_ABS         = $A051       ; r0 = x
                              ; -> .A/.X = the result's length
NUM_CMP         = $A054       ; r0 = x; r1 = y
                              ; -> .A = $FF, 0 or 1 (x below, equal to, above y)
NUM_KIND        = $A057       ; r0 = x
                              ; -> .A = its kind (NK_); .X = its sign ($FF, 0, 1; a complex number's real part's)
NUM_IDIV        = $A05A       ; r0 = x; r1 = y, integers (or numbers equal to them); r5 = a place for the remainder; r6 = its room
                              ; -> .A/.X = the quotient's length (at r2); r6 = the remainder's (at r5).  No room for either: neither written
NUM_GCD         = $A05D       ; r0 = x; r1 = y, integers
                              ; -> .A/.X = the result's length
NUM_POW         = $A060       ; r0 = x; r1 = n, an integer
                              ; -> .A/.X = the result's length
NUM_TRUNCATE    = $A063       ; r0 = x, real
                              ; -> .A/.X = the result's length
NUM_FLOOR       = $A066       ; r0 = x, real
                              ; -> .A/.X = the result's length
NUM_ROUND       = $A069       ; r0 = x, real
                              ; -> .A/.X = the result's length
NUM_TO_FIXED    = $A06C       ; r0 = x, real; .A/.X = places (0 to 10000; $FFFF: 10)
                              ; -> .A/.X = the result's length
NUM_TO_RATIONAL = $A06F       ; r0 = x, real
                              ; -> .A/.X = the result's length
NUM_NUMERATOR   = $A072       ; r0 = x, real
                              ; -> .A/.X = the result's length
NUM_DENOMINATOR = $A075       ; r0 = x, real
                              ; -> .A/.X = the result's length
NUM_COMPLEX     = $A078       ; r0 = re; r1 = im, real numbers
                              ; -> .A/.X = the result's length
NUM_PART        = $A07B       ; r0 = x; .Y = 0 its real part, 1 its imaginary part
                              ; -> .A/.X = the result's length
NUM_FROM_INT    = $A07E       ; r0/r1 = a 32-bit integer (r0 low); .Y = 1 if it's signed
                              ; -> .A/.X = the result's length
NUM_TO_INT      = $A081       ; r0 = x, an integer (or equal to one)
                              ; -> r4/r5 = its low 32 bits (r4 low); .A = 0 if it fits in 32 bits signed, 1 if only unsigned, 2 if neither
NUM_BITS        = $A084       ; r0 = x; r1 = y, integers; .Y = the operation: 0 and, 1 or, 2 xor, 3 not (x alone: -x - 1), 4 shift left by y, 5 shift right by y (toward minus infinity), 6 bit y's test (a count below 0 the other way; past 16 bits, -32768 or 32767)
                              ; -> .A/.X = the result's length (the test: .A = 0 or 1, no result)
NUM_RANDOM      = $A087       ; r0 = n, a positive integer, or 0 for a fixed decimal from 0 to 1
                              ; -> .A/.X = the result's length
NUM_SEED        = $A08A       ; r0 = a 16-bit seed (0: from the clock and the ticks)
NUM_FIB         = $A08D       ; r0 = n, an integer 0 or more
                              ; -> .A/.X = the result's length
NUM_BYTES       = $A090       ; r0 = bytes; r1 = their count
                              ; -> .A/.X = the number's length
NUM_OPTIONS     = $A093       ; .A = the options: bit 0 = 1 if the zero page the libraries borrow ($70-$7F) needn't be kept over a call

; ---- math: Its functions: square roots, e and logarithms, the circle's, real powers, pi, at a precision
MATH_DIGITS     = $A030       ; .A = a precision (1 to 100 significant digits; 0: none set, the precision given back alone)
                              ; -> .A = the precision (before it was set)
MATH_SQRT       = $A033       ; r0 = x
                              ; -> .A/.X = the result's length
MATH_EXP        = $A036       ; r0 = x
                              ; -> .A/.X = the result's length
MATH_LOG        = $A039       ; r0 = x
                              ; -> .A/.X = the result's length
MATH_TRIG       = $A03C       ; r0 = x, in radians; .Y = 0 sine, 1 cosine, 2 tangent
                              ; -> .A/.X = the result's length
MATH_ATAN       = $A03F       ; r0 = x
                              ; -> .A/.X = the result's length
MATH_RPOW       = $A042       ; r0 = x; r1 = y
                              ; -> .A/.X = the result's length
MATH_PI         = $A045       ; -
                              ; -> .A/.X = the result's length

; ---- constants
NUM_MAX         = 1040        ; The longest number in the stored format (a complex number of two rationals of 255-byte integers: 1039)
NT_POS          = $80         ; The format's tags: $00-$7F an integer -64 to 63; $80 + n - 1, a positive integer of n bytes (1-16)
NT_NEG          = $90         ; A negative integer of 1-16 bytes ($90 + n - 1)
NT_LONG_POS     = $A0         ; A positive integer of 17-255 bytes: a length byte, then its magnitude
NT_LONG_NEG     = $A1         ; A negative one
NT_FIXED        = $B0         ; A fixed decimal of 0-15 places ($B0 + places), then its digits: an integer
NT_FIXED_LONG   = $C0         ; A fixed decimal of 16-65535 places: the places (2 bytes), then its digits
NT_RATIONAL     = $C1         ; A rational: its numerator, then its denominator (above 1), in lowest terms
NT_COMPLEX      = $C2         ; A complex number: its real part, then its imaginary part (not 0)
NT_NONE         = $FF         ; Never a number: a program's own
NE_BIG          = 1           ; Too big: an integer past 255 bytes, a fixed decimal past 65535 places
NE_DIV0         = 2           ; Division by zero
NE_NOTNUM       = 3           ; Not a number: text that isn't one, or bytes that aren't a number in the stored format
NE_ROOM         = 4           ; No room for the result in the place given
NE_DOMAIN       = 5           ; Outside the function's domain
NE_BASE         = 6           ; Not a base: a base string that names none
NE_INT          = 7           ; An integer is needed (a number equal to one will do)
NE_REAL         = 8           ; A real number is needed, not a complex one
NE_INIT         = 9           ; The bank given (r13) isn't one INIT filled
NE_FORMAT       = 10          ; A format's placeholders and its arguments don't match
NK_INT          = 0           ; A kind: an integer
NK_FIXED        = 1           ; A fixed decimal
NK_RATIONAL     = 2           ; A rational
NK_COMPLEX      = 3           ; A complex number
NPARSE_PROGRAM  = 1           ; PARSE's .Y: the text is a program's (a bare number starts with a digit 0-9)
NPARSE_WHOLE    = 2           ; PARSE's .Y: the whole text must be the number
NFMT_NUMBER     = 0           ; FORMAT's arguments: a number's entry (its address after)
NFMT_STRING     = 1           ; A string's (zero-terminated)
NFMT_END        = $FF         ; The table's end
NBIT_AND        = 0           ; BITS's operations (.Y): and
NBIT_OR         = 1           ; Or
NBIT_XOR        = 2           ; Xor
NBIT_NOT        = 3           ; Not (x alone: -x - 1)
NBIT_SHL        = 4           ; Shift left by y
NBIT_SHR        = 5           ; Shift right by y (toward minus infinity)
NBIT_TEST       = 6           ; Bit y's test (.A = 0 or 1, no result)
