; ****************************************************************************
; nmbank.inc - the numbers library's memory: the zero page it borrows for a call ($70-$7F: the caller's saved as a call
; starts and put back as it ends), and the RAM bank its caller gives it (r13: at $8000 while a call runs).  The bank is
; the library's own for the program's life: its registers and buffers, scratch for a call, and its state (the base,
; the precision, the random generator), kept between calls.

NUMBERS_MAGIC   = $4E                           ; (The state block's first byte once INIT has run: "N")
NREGS           = 8                             ; Registers R0-R7: a page each, a number's bytes least first
NR_DIGS         = 7                             ; Pages for a number's digits as it's written
BASE_MAX        = 96                            ; A base string at most, its 0 too ("#<=-[" and 80 digits and "]")
DIGITS_START    = 12                            ; The math functions' precision at the start (significant digits)

.zeropage
na:         .res        2                       ; Registers' pages: an operation's operands, its result, its own
nb:         .res        2
nc:         .res        2
nd:         .res        2
np:         .res        2                       ; The caller's memory, as it's read or written
nq:         .res        2                       ; The bank's, as it's copied
nt:         .res        2                       ; Scratch: a count, a length
nu:         .res        2

.segment "NUMBANK"
regs:       .res        NREGS * 256             ; ($8000) R0-R7
digs:       .res        NR_DIGS * 256           ; ($8800) A number's digits, as it's written
text:       .res        256                     ; ($8F00) A number's text, as it's read

; ---- The state: kept between calls (INIT fills it)
st_magic:   .res        1                       ; NUMBERS_MAGIC once INIT has run
st_base:    .res        BASE_MAX                ; The base: its string, zero-terminated ("d" at the start)
st_digits:  .res        1                       ; The math functions' precision (significant digits: 12)
st_rand:    .res        2                       ; The random generator's state (not 0)

; ---- A call's
v_zp:       .res        16                      ; The caller's $70-$7F, put back as the call ends
v_bank:     .res        1                       ; The caller's bank ($00), put back as the call ends
v_sp:       .res        1                       ; The call's stack, for nm_fail to go back to
r_len:      .res        NREGS                   ; Each register's length (bytes: 0, it's zero) ...
r_neg:      .res        NREGS                   ;   and its sign ($FF: negative)
v_len:      .res        2                       ; A number's length, as it's measured or written
v_left:     .res        2                       ; (A copy's count)
v_regs:     .res        8                       ; The caller's r0-r3, kept across a system call

; ---- Numbers in the stored format: two operands copied in, a result made
num_a:      .res        NUM_MAX
num_b:      .res        NUM_MAX
num_r:      .res        NUM_MAX
