; ****************************************************************************
; fdefs.inc - forth's definitions, the core's (forth.s) and its libraries' (forthlib/): the sizes and flags, the
; opcodes the compiler lays down, and the macros for a word's header (in the dictionary's form, a library's in its
; image too) and the data stack.

DS_N            = 32            ; The data stack's cells
F_IMMEDIATE     = $80           ; A header's flags (with the name's length, 0-31) ...
F_HIDDEN        = $40           ;   a definition not finished yet ...
F_INLINE        = $20           ;   a byte after the name: the code's length, copied into a definition, not called
                                ;   (0: called, as any word's) ...
F_COMPILE       = $80           ;   and in that byte, compile-only: interpreted, THROW -14 (Forth's undefined
CODE_MASK       = $7F           ;   interpretation semantics); the rest, the length
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
LIB_MAX         = 16            ; The libraries loaded that libtab keeps (libs, lib, -lib, words), at most
LR_START        = 0             ; A library's record (libtab's): its image's address ...
LR_END          = 2             ;   its end (its BSS's) ...
LR_FIRST        = 4             ;   its first header (whose link is the word before it; 0: it has none) ...
LR_LAST         = 6             ;   its last header ...
LR_WID          = 8             ;   the word list they're in ...
LR_FLAGS        = 10            ;   LRF_* ...
LR_NAME         = 11            ;   and its name (counted: its file's, less its directories and its .fl or .fs)
LR_NAME_MAX     = 10
LR_SIZE         = LR_NAME + 1 + LR_NAME_MAX
LRF_HIDDEN      = $80           ; A library's flags: not searched (-lib) ...
LRF_SOURCE      = $40           ;   Forth source (a .fs that lib loaded), not assembly
JSR_OP          = $20           ; Opcodes the compiler lays down
JMP_OP          = $4C
RTS_OP          = $60
BL_CHAR         = $20

OP_INX          = $E8           ; The opcodes compiled
OP_DEX          = $CA
OP_LDA_IMM      = $A9
OP_STA_ZPX      = $95
OP_LDA_ZPX      = $B5
OP_ORA_ZPX      = $15
OP_BNE          = $D0
OP_BVS          = $70
OP_PLA          = $68
OP_BIT_ZP       = $24
OP_BPL          = $10

; ****************************************************************************
; The headers: HEADER "NAME", flags before a word's code (its label after); HEADERI "NAME", label[, F_COMPILE] for
; an inline word, whose code ends at label_end (an rts there, for EXECUTE); HEADERC "NAME", flags for a compile-only
; word that's called (or, immediate, run), HEADERQC its HEADERQ.  Each header is hdr_N, its link hdr_N-1's (the
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

.macro HEADERI name, label, compile
            HEADER      name, F_INLINE
.ifblank compile
            .byte       .ident(.concat(.string(label), "_end")) - label
.else
            .byte       (.ident(.concat(.string(label), "_end")) - label) | (compile)
.endif
.endmacro

.macro HEADERC name, flags
            HEADER      name, (flags) | F_INLINE
            .byte       F_COMPILE
.endmacro

.macro HEADERQC name, flags
            HEADERQ     name, (flags) | F_INLINE
            .byte       F_COMPILE
.endmacro

; A constant's code, as CONSTANT compiles one (the literal, then rts), so WORDS knows it for a literal and SEE
; shows its number
.macro CONSTCODE value
            dex
            lda         #<(value)
            sta         dlo,x
            lda         #>(value)
            sta         dhi,x
            rts
.endmacro

; Push .A (low) and .Y (high); pop into .A (low) and .Y (high)
.macro PUSHAY
            dex
            sta         dlo,x
            sty         dhi,x
.endmacro
