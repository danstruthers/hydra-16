; ****************************************************************************
; as.inc - the assembler's constants, and what its parts share (core.s: the entries, the input and the output; expr.s:
; the scanner, expressions, the symbol table and the heap; stmt.s: statements, instructions, directives, macros and
; segments), the asm library's (modules/asm).  Each includes it: a name .global here is one part's, used by the
; others.  See core.s.

.include "hydra.inc"
.include "macros.inc"

BANK_AT         = $8000         ; The RAM bank window (RAM_BANK selects the bank)
BANK_LEN        = $2000
LINE_MAX        = 255           ; A line's bytes, at most
TOK_MAX         = 32            ; A name's (a cheap local's: its scope's two bytes after it)
FRAMES          = 16            ; Input frames (files and macro expansions, one inside another), at most
FNAME_LEN       = 64            ; A file frame's name (PATH_MAX + 1)
HEAP_BANKS      = 8             ; The heap's banks, at most (symbols and macros: 64K)
IMG_BANKS       = 4             ; The image's ($0800 to $8000)
CONDS           = 16            ; .if inside .if, at most
SEGSTACK        = 8             ; .pushseg, at most
MAX_ERRORS      = 30
MAX_PARAMS      = 9             ; A macro's parameters, at most
ZP_FIRST        = $22           ; The library's zero page (the caller's, kept in a call: core.s's zsave) ...
ZP_LIB          = 16            ;   its bytes (asm.cfg's ZP)

; Segments, as sdk/asm/hyx2.cfg lays them out
SEG_ZP          = 0             ; ZEROPAGE: from $22
SEG_HEADER      = 1             ; HEADER: from $0800 ...
SEG_CODE        = 2             ;   then CODE ...
SEG_RODATA      = 3             ;   RODATA ...
SEG_DATA        = 4             ;   DATA ...
SEG_BSS         = 5             ;   and BSS
SEGS            = 6

; A symbol (in the heap): next (a heap ref), value (4), flags, segment, the name's length, the name
SY_NEXT         = 0
SY_VAL          = 2
SY_FLAGS        = 6
SY_SEG          = 7
SY_LEN          = 8
SY_NAME         = 9
SF_PASS         = $03           ; The pass it was last defined in (0: none yet)
SF_LABEL        = $04           ; A label (for the labels file)
SF_MACRO        = $08           ; A macro (its value: its definition's heap ref)
SF_LINK         = $10           ; One of the linker's (__DATA_LOAD__ ...): as's own

; Input frames
FR_FILE         = 1
FR_MACRO        = 2

; Conditions (cstate): this level's
CS_ON           = $01           ;   lines assembled ...
CS_DONE         = $02           ;   a branch taken already ...
CS_OUTER        = $04           ;   and the level it's in assembling

.globalzp   hp, p1, p2, p3

; core.s
.global     pass, fraw, line, li, nerrors, err, errtok, warn, push_file, find_include, new_frame, frame_buf
.global     frames, ftype, fnextl, fnexth, fdefl, fdefh, fbuf, imgbase, emit_at
; expr.s
.global     skipws, atend, getid, tok, tlen, sbuf, slen, getstr, expr, val, eu, ef, scope, tokis, isname, here_val
.global     sym_find, sym_def, sym_set, sym_get, sym_link, sym_each, sref, halloc, hsel, heap_init, heap_free, sym_init, un_def
.global     lower, peek, eat, expect, number_ok, val_zp, defined, tfind
; stmt.s
.global     statement, stmt_init, cdepth, mcol, cseg, soffl, soffh, sbasel, sbaseh, ssizel, ssizeh, seg_end, mline
