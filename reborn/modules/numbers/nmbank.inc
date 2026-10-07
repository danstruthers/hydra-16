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
NR_DIGS         = 7                             ; Pages for a number's digits as it's written
BASE_MAX        = 96                            ; A base string at most, its 0 too ("#<=-[" and 80 digits and "]")
DIGITS_START    = 12                            ; The math functions' precision at the start (significant digits)
NSTACK          = 16                            ; The complex numbers' machine: values on its stack at most
ARENA_END       = $A000
DIGS            = ARENA_END - NR_DIGS * 256     ; (The digits' pages)

.macro LDAX     label                           ; .A, .X = an address
            lda         #<(label)
            ldx         #>(label)
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

; ---- A call's
v_zp:       .res        16                      ; The caller's $70-$7F, put back as the call ends
v_bank:     .res        1                       ; The caller's bank ($00), put back as the call ends
v_sp:       .res        1                       ; The call's stack, for nm_fail to go back to
v_len:      .res        2                       ; A number's length, as it's measured or written
v_left:     .res        2                       ; (A copy's count)
v_regs:     .res        8                       ; The caller's r0-r3, kept across a system call
a_top:      .res        2                       ; The arena's first free byte
hv:         .res        2                       ; A number made: its address
