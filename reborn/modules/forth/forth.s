; ****************************************************************************
; forth - HyForth, rebuilt (docs/reimplementation-from-scratch.md, §16): a Forth 2012 system, a program run in place
; from its paged ROM module.  `forth` at rc's prompt starts it; `forth <file` runs a file of source (its lines from
; stdin, as typed ones).
;   Subroutine threaded: a word's execution token is its code's address, and a definition is a run of `jsr xt`
; (literals and IF's test compiled inline; a few short words, the return stack's among them, copied in whole: F_INLINE).
; The data stack is the program's zero page, low bytes and high bytes apart (dlo, dhi), indexed by .X, which every
; word keeps as the stack pointer (DS_N: empty; it grows down); the return stack is the 6502's.  The dictionary is the
; task's RAM after the BSS, to DICT_END; the words in ROM have their headers beside their code, chained into the same
; list as the ones defined in RAM.  A header: the link (2: the one before, 0 at the first), the name's length and
; flags (1: F_IMMEDIATE, F_HIDDEN, F_INLINE), the name (as typed: found ignoring case), then (F_INLINE) the code's
; length; the code, its xt, follows.
;   Input: stdin, a line at a time (the console's cooked lines, or a file's, through rc's <); output: fd 1, buffered.
; Errors are THROWs (Exception), caught in QUIT: the message, both stacks emptied, and on with the next line.
;   The parts: fcore.inc (stacks, arithmetic, memory), fmath.inc (multiplication and division), ftext.inc (input,
; output, numbers, strings, parsing), fcomp.inc (the compiler: definitions, control flow, defining words), finterp.inc
; (the text interpreter, QUIT, CATCH and THROW, EVALUATE).  Their words are in that order in the dictionary.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"

            HYX2_PROGRAM "forth", main

DS_N            = 32            ; The data stack's cells
F_IMMEDIATE     = $80           ; A header's flags (with the name's length, 0-31) ...
F_HIDDEN        = $40           ;   a definition not finished yet ...
F_INLINE        = $20           ;   code copied into a definition, not called (its length after the name)
LEN_MASK        = $1F
DICT_END        = $7F00         ; The dictionary's end (the break's)
TIB_SIZE        = 128           ; A line of input at most
IBUF_SIZE       = 256           ; stdin's bytes, read ahead
OBUF_SIZE       = 128           ; The output, waiting to be written
HOLD_SIZE       = 70            ; Pictured numeric output (2 * 32 + 2 for a binary double, and more)
PAD_SIZE        = 100
WBUF_SIZE       = 257           ; WORD's counted string (255 at most, and a space after it)
SBUF_SIZE       = 100           ; S" while interpreting: two of them, in turn
JSR_OP          = $20           ; Opcodes the compiler lays down
JMP_OP          = $4C
RTS_OP          = $60
BL_CHAR         = $20

.zeropage
dlo:        .res        DS_N                                ; The data stack: low bytes ...
dhi:        .res        DS_N                                ;   and high bytes (.X: the top's index)
w:          .res        2                                   ; Scratch pointers ...
w2:         .res        2
w3:         .res        2
tmp:        .res        2                                   ;   and numbers
tmp2:       .res        2
tmp3:       .res        2
xsave:      .res        1                                   ; .X, kept over a system call or a tsx
cnt:        .res        1
here:       .res        2                                   ; The dictionary's next byte

.bss
latest:     .res        2                                  ; The FORTH word list's last header
lastxt:     .res        2                                   ; The definition being made: its xt (RECURSE, DOES>) ...
lasthdr:    .res        2                                   ;   and its header (; shows it)
state:      .res        2                                   ; STATE: 0 interpreting, -1 compiling
base:       .res        2                                   ; BASE
to_in:      .res        2                                   ; >IN
src_addr:   .res        2                                   ; SOURCE: the input buffer ...
src_len:    .res        2                                   ;   its length ...
src_id:     .res        2                                   ;   and SOURCE-ID: 0 stdin, -1 a string (EVALUATE)
handler:    .res        2                                   ; CATCH's frame (the 6502's stack pointer at it), or 0
rsp0:       .res        1                                   ; The 6502's stack pointer as QUIT starts
interactive: .res       1                                   ; <> 0: stdin is the console (prompts)
leaves:     .res        2                                   ; The LEAVEs of the DO being compiled, a chain
hld:        .res        2                                   ; Pictured numeric output: the next char's place
throw_name: .res        2                                   ; The undefined word (its address and length)
throw_nlen: .res        1
abort_msg:  .res        2                                   ; ABORT"'s message (a counted string)
ctlfd:      .res        1                                   ; /dev/consctl's fd (KEY's raw mode), $FF: not open
sbuf_n:     .res        1                                   ; S" while interpreting: the buffer last used
ilen:       .res        2                                   ; stdin's bytes read ahead: how many ...
ipos:       .res        2                                   ;   and the next
olen:       .res        1                                   ; The output's bytes waiting
ibuf:       .res        IBUF_SIZE
tib:        .res        TIB_SIZE
obuf:       .res        OBUF_SIZE
holdbuf:    .res        HOLD_SIZE
hold_end:
pad:        .res        PAD_SIZE
wbuf:       .res        WBUF_SIZE
sbuf:       .res        SBUF_SIZE * 2
numacc:     .res        4                                   ; (Numbers: an accumulator, 32 bits ...
numtmp:     .res        4                                   ;   and another)
dict:                                                       ; The dictionary, from here

; ****************************************************************************
; The headers: HEADER "NAME", flags before a word's code (its label after); HEADERI "NAME", label for an inline
; word, whose code ends at label_end (an rts there, for EXECUTE).  Each header is hdr_N, its link hdr_N-1's (the
; first's 0): hdr_n counts them
hdr_n       .set        0

.macro HEADER name, flags
.ident(.sprintf("hdr_%d", hdr_n + 1)):
.if hdr_n = 0
            .word       0
.else
            .word       .ident(.sprintf("hdr_%d", hdr_n))
.endif
hdr_n       .set        hdr_n + 1
            .byte       (flags) | .strlen(name)
            .byte       name
.endmacro

; HEADERQ "NAME", flags: a name ending in a " (NAME", as ca65's strings can't hold one)
.macro HEADERQ name, flags
.ident(.sprintf("hdr_%d", hdr_n + 1)):
            .word       .ident(.sprintf("hdr_%d", hdr_n))
hdr_n       .set        hdr_n + 1
            .byte       (flags) | (.strlen(name) + 1)
            .byte       name, $22
.endmacro

.macro HEADERI name, label
            HEADER      name, F_INLINE
            .byte       .ident(.concat(.string(label), "_end")) - label
.endmacro

; Push .A (low) and .Y (high); pop into .A (low) and .Y (high)
.macro PUSHAY
            dex
            sta         dlo,x
            sty         dhi,x
.endmacro

.code
; ****************************************************************************
; The start: the dictionary after the BSS, its end claimed (BREAK), decimal, stdin a console or not; then QUIT
main:
            LDR         r0, DICT_END
            jsr         BREAK
            ldx         #DS_N
            lda         #<dict
            sta         here
            lda         #>dict
            sta         here + 1
            lda         #<forth_last
            sta         latest
            lda         #>forth_last
            sta         latest + 1
            lda         #10
            sta         base
            stz         base + 1
            stz         handler
            stz         handler + 1
            stz         olen
            stz         ilen
            stz         ilen + 1
            stz         ipos
            stz         ipos + 1
            stz         sbuf_n
            lda         #$FF
            sta         ctlfd
            stz         interactive                         ; The console on stdin: prompts
            stx         xsave
            LDR         r0, pad                             ; (Its stat record, in pad: unused yet)
            lda         #0
            jsr         FSTAT
            ldx         xsave
            bcs         :+
            lda         pad + SR_DEV
            cmp         #'c'
            bne         :+
            inc         interactive
            jsr         banner
:
            tsx
            stx         rsp0
            ldx         #DS_N
            jmp         quit

banner:
            LDR         w, s_banner
            jmp         type_z

s_banner:   .byte       "HyForth (Forth 2012), BYE to end", LF, 0

.include "fcore.inc"
.include "fmath.inc"
.include "ftext.inc"
.include "fcomp.inc"
.include "finterp.inc"

forth_last  = .ident(.sprintf("hdr_%d", hdr_n))             ; (The last ROM header: the word list's start)
