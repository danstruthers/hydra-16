; ****************************************************************************
; nmbank.inc - the numbers library's memory: the zero page it borrows for a call ($70-$7F: the caller's saved as a call
; starts and put back as it ends), and the RAM bank its caller gives it (r13: at $8000 while a call runs).  The bank is
; the library's for the program's life: its registers, its state (the base, the precision, the random generator),
; kept between calls, a call's variables, and the arena, where a call's numbers are (its operands copied in, the
; numbers it makes on the way, its result), each in the stored format, a value its address.  The other files put
; their variables in the bank too (NUMBANK); numbers.s puts the arena last.
;   $8000-$87FF     R0-R7
;   $8800-...       the state, the variables
;   ...-$9FFF       the arena; its last NR_DIGS pages a number's digits while it's written (DISPLAY: its operand is
;                   the arena's first)

NUMBERS_MAGIC   = $4E                           ; (The state's first byte once INIT has run: "N")
NREGS           = 8                             ; Registers R0-R7: a page each, a number's bytes least first
NRALL           = 32                            ; (Registers' lengths and signs: R0-R7, and R24-R31, the digits'
                                                ;   pages, which the math library works in too: math.s)
NR_DIGS         = 8                             ; Pages for a number's digits as it's written (2048: a
                                                ;   255-byte integer in base 2 is 2040, in base -2 2042)
BASE_MAX        = 96                            ; A base string at most, its 0 too ("#<=-[" and 80 digits and "]")
DIGITS_START    = 12                            ; The math functions' precision at the start (significant digits)
NSTACK          = 16                            ; The complex numbers' machine: values on its stack at most
NCONST          = 3                             ; The math library's constants kept between calls (pi, log 2, log 10) ...
CONST_BYTES     = 104                           ;   each at 832 bits at most
P10_BYTES       = 42                            ; 10^n at most (n the precision, MAX_DIGITS 100: 333 bits)
ARENA_END       = $A000
DIGS            = ARENA_END - NR_DIGS * 256     ; (The digits' pages)

.macro LDAX     label                           ; .A, .X = an address
            lda         #<(label)
            ldx         #>(label)
.endmacro

; A library of two banks (docs/design/plans/NUMSPEED.md, step 2): code the second bank needs as well is assembled again
; there, in a scope of its own that defines NM_COPY (its variables the first's: each file's NUMBANK block is skipped
; there, .ifndef NM_COPY).  NM_CODE and NM_RODATA: the segment for the code and read-only data, the first bank's or
; the second's.  NMFAR2: the routine in the second bank called from the first (by XCALL: .A, .X, .Y and r0-r13 pass,
; r14 and r15 are XCALL's)
.macro RPAGE                                    ; .A = register .A's page (or the digits' page .A): r_page's,
            clc                                 ;   inline
            adc         #>regs
.endmacro
.macro NM_CODE
.ifdef NM_COPY
.segment "CODE2"
.else
.segment "CODE"
.endif
.endmacro
.macro NM_RODATA
.ifdef NM_COPY
.segment "RODATA2"
.else
.segment "RODATA"
.endif
.endmacro
.macro NMFAR2   routine
            pha
            lda         #<(routine)
            sta         r15
            lda         #>(routine)
            sta         r15 + 1
            lda         ROM_BANK
            inc         a
            sta         r14
            pla
            jsr         XCALL
.endmacro

.zeropage
na:         .res        2                       ; Registers' pages: an operation's operands, its result, its own
nb:         .res        2
nc:         .res        2
nd:         .res        2
np:         .res        2                       ; The caller's memory, as it's read or written
nq:         .res        2                       ; The bank's, as it's copied or checked
nt:         .res        2                       ; A number in the bank, as it's read
nu:         .res        2                       ; The arena, as a number's written in it

.segment "NUMBANK"
regs:       .res        NREGS * 256             ; ($8000) R0-R7

; ---- The state: kept between calls (INIT fills it)
st_magic:   .res        1                       ; NUMBERS_MAGIC once INIT has run
st_base:    .res        BASE_MAX                ; The base: its string, zero-terminated ("d" at the start)
st_digits:  .res        1                       ; The math functions' precision (significant digits: 12)
st_rand:    .res        2                       ; The random generator's state (not 0)
st_cbits:   .res        2 * NCONST              ; The math library's constants, cached: each's bits (0: none yet; INIT
st_const:   .res        NCONST * CONST_BYTES    ;   clears them) and its magnitude's bytes (pi, log 2, log 10)
st_p10n:    .res        1                       ; The math library's rounding: the precision (digits) 10^(n - 1)
st_p10l:    .res        2                       ;   and 10^n are kept for (0: none), their lengths and their
st_p10:     .res        2 * P10_BYTES           ;   bytes (P10_BYTES each)
st_zpfree:  .res        1                       ; OPTIONS's bit 0: $70-$7F not kept over a call (1)
st_fv:      .res        1                       ; The text's bases (nmtext.inc): f_ the state's base, parsed (1)
st_wf:      .res        1                       ;   and w_ the same as f_ (1); both kept between calls
f_radix:    .res        1                       ; f_: the base a call has.  Its size; negative ($FF);
f_neg:      .res        1                       ;   balanced ($FF); least digit first ($80); no two digits
f_bal:      .res        1                       ;   the same but for case (1); its 0's place among its
f_le:       .res        1                       ;   digits; decimal (1: 0-9, none of those three); its
f_ci:       .res        1                       ;   prefix written (1: its string's #); its digits
f_zero:     .res        1
f_dec:      .res        1
f_pre:      .res        1
f_dset:     .res        80
w_radix:    .res        1                       ; w_: the base a number's read or written in, the same
w_neg:      .res        1
w_bal:      .res        1
w_le:       .res        1
w_ci:       .res        1
w_zero:     .res        1
w_dec:      .res        1
w_pre:      .res        1
w_dset:     .res        80
B_SIZE          = w_radix - f_radix
f_spec:     .res        BASE_MAX                ; f_'s string (its prefix: # and the string past its #)
f_body:     .res        1                       ; (Where its string's # ends)

; ---- A call's
v_zp:       .res        16                      ; The caller's $70-$7F, put back as the call ends
v_zpfree:   .res        1                       ; The caller's $70-$7F not kept (1: st_zpfree as it began)
v_bank:     .res        1                       ; The caller's bank ($00), put back as the call ends
v_sp:       .res        1                       ; The call's stack, for nm_fail to go back to
v_len:      .res        2                       ; A number's length, as it's measured or written
v_left:     .res        2                       ; (A copy's count)
v_regs:     .res        8                       ; The caller's r0-r3, kept across a system call
v_ax:       .res        2                       ; The caller's .A, .X as the call began (nm_begin keeps them)
a_top:      .res        2                       ; The arena's first free byte
hv:         .res        2                       ; A number made: its address
