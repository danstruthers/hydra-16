; ****************************************************************************
; forth - HyForth, rebuilt (docs/reimplementation-from-scratch.md, §16): a Forth 2012 system, a program run in place
; from its paged ROM module.  `forth` at rc's prompt starts it; `forth <file` runs a file of source (its lines from
; stdin, as typed ones).  The word sets: Core and Core Extension, Exception, File Access, Facility, String,
; Search-Order and Programming-Tools, with their extensions (but the editor's, the assembler's and EKEY's); and the
; Hydra's words: a sys- word for each system call a program makes (made from the specification), SH and RUN, banks
; and segments.  Ctrl-C (a note) is THROW -28 at the next word, loop or wait.  A module of two banks: the second has
; the sys- words' table, which their headers are made from in RAM as forth starts.
;   Subroutine threaded: a word's execution token is its code's address, and a definition is a run of `jsr xt`
; (literals and IF's test compiled inline; a few short words, the return stack's among them, copied in whole: F_INLINE).
; The data stack is the program's zero page, low bytes and high bytes apart (dlo, dhi), indexed by .X, which every
; word keeps as the stack pointer (DS_N: empty; it grows down); the return stack is the 6502's.  The dictionary is the
; task's RAM after the BSS, to DICT_END; the words in ROM have their headers beside their code, chained into the same
; list as the ones defined in RAM (FORTH's: a word list is a chain of headers).  A header: the link (2: the one before,
; 0 at the first), the name's length and flags (1: F_IMMEDIATE, F_HIDDEN, F_INLINE), the name (as typed: found
; ignoring case), then (F_INLINE) the code's length; the code, its xt, follows.  A header's address is its nt.
;   Input: stdin, a line at a time (the console's cooked lines, or a file's, through rc's <), or a file's (INCLUDED),
; or a string's (EVALUATE): the source before a nested one is kept on the source stack.  Output: fd 1, buffered.  A
; fileid is the system's fd; an ior is 0, or -512 less the system's error code (Gforth's way).  Errors are THROWs
; (Exception), caught in QUIT: the message (with the file and line, from a file), both stacks emptied, the files
; being included closed, and on with the next line.
;   The parts: fcore.inc (stacks, arithmetic, memory), fmath.inc (multiplication and division), ftext.inc (input,
; output, numbers, strings, parsing), fcomp.inc (the compiler: definitions, control flow, defining words), finterp.inc
; (the text interpreter, QUIT, CATCH and THROW, EVALUATE), fsearch.inc (word lists and the search order), ffile.inc
; (files, and including them), fstring.inc (strings, the Facility words), ftools.inc (the Programming-Tools words),
; fhydra.inc (the Hydra's words).  Their words are in that order in the dictionary, then the sys- words (in RAM).
; The second bank: fsys.inc, and the table (obj/gen/forthsys.inc, tools/apigen.js's).

.include "hydra.inc"
.include "hw.inc"
.include "hyx2.inc"
.include "macros.inc"

            HYX2_PROGRAM "forth", main, 2

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
ORDER_MAX       = 8             ; The search order's word lists, at most
SRC_MAX         = 16            ; Sources nested (the source stack's records)
INC_MAX         = 8             ; Files included, nested
LINE_MAX        = 128           ; A file's line, at most (a longer one: the rest is the next) ...
LINE_BUF        = LINE_MAX + 2  ;   in a buffer of that and 2 (READ-LINE's)
LNAME_SIZE      = 32            ; A file being included: its name (counted, 31 at most)
INCN_SIZE       = 256           ; The names of the files INCLUDED (REQUIRED's)
SUBST_SIZE      = 255           ; REPLACES's names and texts (offsets in a byte)
PATH_SIZE       = 128           ; A file's name, for the system
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
p1:         .res        2                                   ; Pointers (strings, SEE)
p2:         .res        2
intr:       .res        1                                   ; $80: Ctrl-C came (the note handler's), for THROW -28

.bss
forth_wl:   .res        4                                   ; FORTH-WORDLIST: a word list is its last header (0: none),
                                                            ;   then the word list made before it (0: none)
wl_last:    .res        2                                   ; The word lists, newest first (a chain: FORTH's last)
current:    .res        2                                   ; The compilation word list
order_n:    .res        1                                   ; The search order: how many word lists ...
order:      .res        ORDER_MAX * 2                       ;   and they (the first searched first)
lastxt:     .res        2                                   ; The definition being made: its xt (RECURSE, DOES>) ...
lasthdr:    .res        2                                   ;   and its header (; shows it)
state:      .res        2                                   ; STATE: 0 interpreting, -1 compiling
base:       .res        2                                   ; BASE
src_addr:   .res        2                                   ; The input source (SRC_SIZE bytes, in this order: the
src_len:    .res        2                                   ;   source stack's records are copies): SOURCE ...
to_in:      .res        2                                   ;   >IN ...
src_id:     .res        2                                   ;   SOURCE-ID: 0 stdin, -1 a string, or a fileid ...
src_pos:    .res        4                                   ;   a file's: where the line SOURCE holds starts ...
src_cons:   .res        2                                   ;   the bytes it took (its end too) ...
src_line:   .res        2                                   ;   its number (stdin's too) ...
src_close:  .res        1                                   ;   <> 0: a file, closed at its end ...
src_fdep:   .res        1                                   ;   and the files being included (1 ...: this file's)
SRC_SIZE    = * - src_addr
ssp:        .res        1                                   ; The source stack's records (EVALUATE, INCLUDE-FILE)
sstack:     .res        SRC_SIZE * SRC_MAX
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
err_noted:  .res        1                                   ; <> 0: an error's place noted (THROW's), for QUIT: its
err_line:   .res        2                                   ;   line, and its file (0: stdin; 1 ...: lnames's)
err_fdep:   .res        1
throw_named: .res       1                                   ; <> 0: an OS error is throw_name's (INCLUDED's)
inc_named:  .res        1                                   ; <> 0: INCLUDED has named the file to come
rl_fd:      .res        1                                   ; READ-LINE's: the fd, the bytes read, the line's length
rl_n:       .res        2
rl_len:     .res        2
cond_lvl:   .res        1                                   ; [IF]'s skipping: how deep, and whether [ELSE] ends it
cond_else:  .res        1
incn_len:   .res        2                                   ; The files INCLUDED (REQUIRED's): their bytes in incn
subst_len:  .res        1                                   ; REPLACES's: their bytes in substs
raw:        .res        1                                   ; <> 0: the console in raw mode (KEY, KEY?)
kq_fd:      .res        1                                   ; KEY?'s fd (/dev/cons, non-blocking), $FF: not open
key_pend:   .res        1                                   ; <> 0: KEY? has a key (key_char) for KEY
key_char:   .res        1
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
lbufs:      .res        LINE_BUF * INC_MAX                  ; The files being included: each one's line ...
lnames:     .res        LNAME_SIZE * INC_MAX                ;   and name (counted: INCLUDED's)
incn:       .res        INCN_SIZE                           ; The files INCLUDED: counted names
substs:     .res        SUBST_SIZE                          ; REPLACES's: each a counted name, then a counted text
pathbuf:    .res        PATH_SIZE                           ; A file's name, zero-terminated (for the system)
statbuf:    .res        SR_SIZE                             ; A stat record
zbufs:      .res        PATH_SIZE * 2                       ; >Z's two buffers, in turn ...
zbuf_n:     .res        1                                   ;   the one last used
argbuf:     .res        ARGS_MAX                            ; SH's and RUN's program's arguments
sys_a:      .res        1                                   ; A sys- word's call: .A, .X and .Y, in and out ...
sys_x:      .res        1
sys_y:      .res        1
sys_p:      .res        1                                   ;   its flags (C: it failed) ...
sys_f:      .res        1                                   ;   its descriptor's flags ($80: an ior) ...
sys_to:     .res        2                                   ;   and its address
.assert     sys_y = sys_a + 2 .and sys_x = sys_a + 1, error, "sys_a, sys_x, sys_y: in that order (sys_pop, sys_push)"
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

.segment "DATA"
; The note handler, in RAM (either bank may be at $A000 when a note comes): Ctrl-C (NOTE_INTERRUPT) noted in intr,
; for the next word, loop or wait to THROW -28, forth going on; another note, the default
notes:
            cmp         #NOTE_INTERRUPT
            bne         :+
            lda         #$80
            sta         intr
            clc
            rts
:
            sec
            rts

.code
; ****************************************************************************
; The start: the dictionary after the BSS, its end claimed (BREAK), decimal, stdin a console or not, Ctrl-C a
; THROW; the sys- words' headers made (the second bank's sys_build); then QUIT
main:
            HYX2_BANKS_INIT
            LDR         r0, DICT_END
            jsr         BREAK
            LDR         r0, notes
            jsr         NOTIFY
            ldx         #DS_N
            lda         #<dict
            sta         here
            lda         #>dict
            sta         here + 1
            lda         #<forth_last                        ; FORTH: the ROM's words, and the order and
            sta         forth_wl                            ;   definitions in it alone
            lda         #>forth_last
            sta         forth_wl + 1
            stz         forth_wl + 2
            stz         forth_wl + 3
            lda         #<forth_wl
            sta         wl_last
            sta         current
            ldy         #>forth_wl
            sty         wl_last + 1
            sty         current + 1
            jsr         only
            ldy         #SRC_SIZE - 1                       ; The source: stdin (no line yet), none nested
:
            lda         #0
            sta         src_addr,y
            dey
            bpl         :-
            stz         ssp
            stz         err_noted
            stz         throw_named
            stz         inc_named
            stz         incn_len
            stz         incn_len + 1
            stz         subst_len
            stz         raw
            stz         key_pend
            stz         intr
            stz         zbuf_n
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
            sta         kq_fd
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
            FAR2        sys_build
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
.include "fsearch.inc"
.include "ffile.inc"
.include "fstring.inc"
.include "ftools.inc"
.include "fhydra.inc"

forth_last  = .ident(.sprintf("hdr_%d", hdr_n))             ; (The last ROM header: the word list's start)

; ****************************************************************************
; The second bank
.segment "CODE2"
.include "fsys.inc"
.segment "RODATA2"
.include "forthsys.inc"
