; ****************************************************************************
; fdefs.inc - forth's definitions, the core's (forth.s) and its libraries' (forthlib/): the sizes and flags, the
; opcodes the compiler lays down, and the macros for a word's header (in the dictionary's form, a library's in its
; image too) and the data stack.

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
