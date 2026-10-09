; ****************************************************************************
; core.s - the assembler (phase 9's as, now the asm library's: spec/asm.def), whose callers are the as program
; (modules/as: FILE, a source to a program) and BASIC's ASM blocks: a program for the W65C02S from ca65's language,
; as the SDK's sources are written (sdk/asm): a RAM program (HYX2: SPAWN reads it at $0800), or with AF_RAW the bytes
; alone (no header) from .org's address ($0800 if there's none); with AF_LABELS a labels file too, out.lbl (ld65's
; -Ln form: db's l and dis's -l read it).
;   Its language: ca65's for instructions, labels (name:, cheap @name: between normal labels, : unnamed with :+ and
; :- to them), constants (=, :=), expressions (expr.s) and the commoner directives (stmt.s), with macros and .if,
; .include and .incbin; enough to assemble the SDK's include files (hydra.inc, hyx2.inc, macros.inc: /lib/as) as
; they are.  A file .include names is found as it's named, then beside the file that names it, then in /lib/as.
;   As its own linker, it lays the segments out as sdk/asm/hyx2.cfg does (ld65's): ZEROPAGE from $22; from $0800
; HEADER, CODE, RODATA, DATA, then BSS; with ld65's names for them (__DATA_LOAD__, __DATA_RUN__, __DATA_SIZE__,
; __BSS_RUN__, __BSS_SIZE__, __RAM_START__, __RAM_SIZE__, __RAM_LAST__ ...) and HYX2_RAM defined.
;   Three passes over the source: the first finds each segment's size (its labels' places not yet known), the second
; each symbol's value, the third the bytes.  An address takes the zero page form only if it's known by then in the
; pass and under $100, so each pass gives each line the same size.  Errors are said as "as: file:line: what" (fd 2),
; and the pass they're found in is the last; the status is 1.
;   Its parts: core.s (this: the entries; the input, a stack of frames, files and macros' uses; errors; the output),
; expr.s (the scanner, expressions, the heap of RAM banks, the symbol table) and stmt.s (statements, instructions,
; directives, macros, segments).  Its symbols and macros are in RAM banks (the heap: HEAP_BANKS), its image too
; (IMG_BANKS), and each file it reads, read once (the cache: CBANKS), all given back as a call ends.
;   A library has no RAM of its own: its state (the BSS: asm.cfg's RAM, ASM_RAM to ASM_RAM_END) is in its caller's
; RAM, which a caller lends it for the call, and its zero page ($22 on, ZP_LIB bytes) is the caller's, kept as a
; call starts (zsave) and put back as it ends, with the caller's RAM bank.

.include "as.inc"
.include "asmlib.inc"

BANKREG         = $00           ; The task's RAM bank register
IMG_AT          = $0800         ; The image's start (a HYX2 RAM program's, and -b's without .org) ...
IMG_LEN         = $7800         ;   and its room
RAM_END         = $8000         ; A RAM program's end, at most
ZP_START        = $22           ; The program's zero page
ZP_LEN          = $5E
CFILES          = 48            ; The cache's files (each name tried, there or not), at most ...
CBANKS          = 40            ;   and its banks (320K)

.import     __BSS_RUN__, __BSS_SIZE__
.export     asm_file, asm_begin, asm_pass, asm_define, asm_line, asm_end, asm_symbol, asm_image, asm_done, asm_error

.zeropage
fp:         .res        2                                   ; A frame's buffer; getline's page of a file
mp:         .res        2                                   ; (The messages': a string ...
mn:         .res        2                                   ;   and a number)

.bss
zsave:      .res        ZP_LIB                              ; The caller's zero page ($22 on), kept in a call ...
bsave:      .res        1                                   ;   and its RAM bank
bss_from:                                                   ; (A call's start clears the rest)
status:     .res        1                                   ; 1 after an error (the exit status's)
quiet:      .res        1                                   ; AF_QUIET: messages kept (errbuf), not said
lmode:      .res        1                                   ; 1: the source given a line at a time (BEGIN's) ...
lline:      .res        2                                   ;   the line's number (its messages')
errbuf:     .res        160                                 ; The first message since BEGIN (AF_QUIET's)
ihave:      .res        1                                   ; 1: the image's banks are taken
pass:       .res        1                                   ; 1, 2, 3
fraw:       .res        1                                   ; -b
flbl:       .res        1                                   ; -l
nerrors:    .res        1
line:       .res        LINE_MAX + 1                        ; The line
li:         .res        1                                   ;   and the scanner's place in it
lx:         .res        1                                   ; (getline's: the line's length so far ...
gfr:        .res        1                                   ;   the frame ...
gq:         .res        1                                   ;   $FF in quotes ...
gqc:        .res        1                                   ;   which)
frames:     .res        1                                   ; The input's frames: how many ...
ftype:      .res        FRAMES                              ;   each one's kind (FR_*) ...
fll:        .res        FRAMES                              ;   the number of its line ...
flh:        .res        FRAMES
fcf:        .res        FRAMES                              ;   a file's cache entry ...
fbk:        .res        FRAMES                              ;   its place: the bank (cbanks's) ...
fpg:        .res        FRAMES                              ;   page ...
fofs:       .res        FRAMES                              ;   and where in it
fnextl:     .res        FRAMES                              ;   a macro's use: its next line ...
fnexth:     .res        FRAMES
fdefl:      .res        FRAMES                              ;   and its definition
fdefh:      .res        FRAMES
fcdep:      .res        FRAMES                              ;   and the .if levels it starts in (.exitmacro's)
fbuf:       .res        FRAMES * 256                        ; Each frame's buffer (a macro's arguments)
cfiles:     .res        1                                   ; The cache: its files (by name) ...
cmiss:      .res        CFILES                              ;   each one's 1 if it's not there ...
cfirst:     .res        CFILES                              ;   its first bank (cbanks's) ...
cebk:       .res        CFILES                              ;   its end (a 0 there): the bank ...
cepg:       .res        CFILES                              ;   page ...
ceofs:      .res        CFILES                              ;   and where in it ...
cname:      .res        CFILES * FNAME_LEN                  ;   and its name
ncb:        .res        1                                   ; The cache's banks ...
cbanks:     .res        CBANKS
fload:      .res        1                                   ; (find_load's: 1; find_include's: 0 ...
fbad:       .res        1                                   ;   and 1 after no room)
cfd:        .res        1                                   ; (cload's: the file ...
cent:       .res        1                                   ;   its entry ...
cpos:       .res        2                                   ;   where in its bank ...
cr0:        .res        2                                   ;   and r0)
imgbase:    .res        2                                   ; The image's start
ibank:      .res        1                                   ; Its banks' first (IMG_BANKS of them)
ramlast:    .res        2                                   ; The program's end (BSS's)
srcname:    .res        FNAME_LEN                           ; file.s
outname:    .res        FNAME_LEN                           ; out
path:       .res        FNAME_LEN + 8                       ; A file's name (find_include's; out.lbl)
msg:        .res        160                                 ; An error's text ...
mlen:       .res        1                                   ;   its length
nbuf:       .res        5                                   ; (A number's digits)
ofd:        .res        1                                   ; An output's fd
ooff:       .res        2                                   ; (write_out's: the offset in the image ...
oleft:      .res        2                                   ;   what's left of it ...
olen:       .res        2                                   ;   and a write's)
lbuf:       .res        TOK_MAX + 12                        ; A line of the labels file

.code

; ---- The entries: each keeps its caller's zero page (the library's, $22 on: zsave) and its RAM bank (bsave), and
; gives them back as it ends

; FILE: r0 = the source's name, r1 = the output's, .A = the flags (AF_RAW, AF_LABELS).  OUT: .A = the status (0, or
; 1 after an error, each said on fd 2), C = 1 if it's 1
asm_file:
            jsr         enter
            pha
            jsr         begin
            pla
            pha
            and         #AF_RAW
            sta         fraw
            pla
            and         #AF_LABELS
            sta         flbl
            LDR         mp, srcname                         ; (The names: FNAME_LEN bytes at most, the 0 too)
            jsr         argcpy
            lda         r1
            sta         r0
            lda         r1 + 1
            sta         r0 + 1
            LDR         mp, outname
            jsr         argcpy
            jsr         heap_init                           ; The heap's banks, and the image's
            bcs         @room
            lda         #IMG_BANKS
            jsr         BANKS_ALLOC
            bcs         @room
            sta         ibank
            inc         ihave
            lda         #<IMG_AT
            sta         imgbase
            lda         #>IMG_AT
            sta         imgbase + 1
            lda         #1                                  ; The passes
            sta         pass
@pass:
            jsr         run
            lda         nerrors
            bne         @end                                ; (Each said, and the status 1)
            inc         pass
            lda         pass
            cmp         #4
            bne         @pass
            jsr         write_out
            lda         flbl
            beq         @end
            jsr         write_labels
@end:
            jsr         free_all
            lda         status
            jmp         leave
@room:
            LDR         r0, s_room
            jsr         lib_warn
            bra         @end

; The caller's zero page and RAM bank kept.  Keeps .A, .X, .Y
enter:
            pha
            phx
            ldx         #ZP_LIB - 1
:
            lda         ZP_FIRST,X
            sta         zsave,X
            dex
            bpl         :-
            lda         BANKREG
            sta         bsave
            plx
            pla
            rts

; ... and given back.  Keeps .A; OUT: C = 1 if .A isn't 0
leave:
            pha
            ldx         #ZP_LIB - 1
:
            lda         zsave,X
            sta         ZP_FIRST,X
            dex
            bpl         :-
            lda         bsave
            sta         BANKREG
            pla
            cmp         #1
            rts

; A call's state cleared (the BSS past zsave and bsave: what a program's start had as 0)
begin:
            LDR         mp, bss_from
@page:
            lda         mp + 1
            cmp         #>(__BSS_RUN__ + __BSS_SIZE__)
            bcc         :+
            lda         mp
            cmp         #<(__BSS_RUN__ + __BSS_SIZE__)
            bcs         @done
:
            lda         #0
            sta         (mp)
            inc         mp
            bne         @page
            inc         mp + 1
            bra         @page
@done:
            rts

; The banks a call took, given back: the heap's, the image's, the cache's
free_all:
            jsr         heap_free
            lda         ihave
            beq         :+
            lda         ibank
            ldx         #IMG_BANKS
            jsr         BANKS_FREE
            stz         ihave
:
            ldx         #0
@cache:
            cpx         ncb
            bcs         @done
            phx
            lda         cbanks,X
            ldx         #1
            jsr         BANKS_FREE
            plx
            inx
            bra         @cache
@done:
            stz         ncb
            rts

; ---- The source a line at a time (BASIC's ASM blocks): BEGIN, then for each pass PASS, the caller's symbols
; (DEFINE), its lines (LINE) and END; then SYMBOL for its labels, IMAGE for its bytes, DONE

; BEGIN: r0 = the origin (the image's first address: $8000, a bank's code), r1 = the source's name (for messages:
; "name:line: what"), .A = the flags (AF_QUIET: messages kept for ERROR, not said).  The bytes alone, no header.
; OUT: C = 1 if there's no room (RAM banks)
asm_begin:
            jsr         enter
            pha
            jsr         begin
            pla
            and         #AF_QUIET
            sta         quiet
            lda         #1
            sta         fraw
            sta         lmode
            lda         r0
            sta         imgbase
            lda         r0 + 1
            sta         imgbase + 1
            lda         r1
            sta         r0
            lda         r1 + 1
            sta         r0 + 1
            LDR         mp, srcname
            jsr         argcpy
            jsr         heap_init
            bcs         @room
            lda         #IMG_BANKS
            jsr         BANKS_ALLOC
            bcs         @room
            sta         ibank
            inc         ihave
            lda         #0
            jmp         leave
@room:
            jsr         free_all
            LDR         r0, s_room
            jsr         lib_warn
            lda         #1
            jmp         leave

; PASS: .A = the pass (1, 2, 3): its start (the segments from the origin, the symbols' scope, no input yet)
asm_pass:
            jsr         enter
            sta         pass
            stz         lline
            stz         lline + 1
            jsr         sym_init
            jsr         stmt_init
            stz         frames
            jsr         bases
            lda         #0
            jmp         leave

; DEFINE: r0 = a name (TOK_MAX characters at most), r1/r2 = its value (32 bits): a symbol of the caller's, in this
; pass (after PASS: in each).  OUT: C = 1 if there's no room
asm_define:
            jsr         enter
            lda         r1
            sta         val
            lda         r1 + 1
            sta         val + 1
            lda         r2
            sta         val + 2
            lda         r2 + 1
            sta         val + 3
            ldy         #TOK_MAX                            ; (Its length checked)
:
            lda         (r0),Y
            beq         :+
            dey
            bpl         :-
            lda         #1
            jmp         leave
:
            jsr         sym_link
            lda         #0
            rol         a
            jmp         leave

; LINE: r0 = a line (zero-terminated, 255 bytes at most), r1 = its number (its messages'): assembled in this pass,
; and what it starts (an .include, a macro's use) to its end.  OUT: C = 1 if it had an error
asm_line:
            jsr         enter
            lda         r1
            sta         lline
            lda         r1 + 1
            sta         lline + 1
            lda         nerrors
            pha
            jsr         lcopy
            jsr         statement
@more:
            lda         frames
            beq         @done
            jsr         getline
            bcs         @done
            jsr         statement
            bra         @more
@done:
            pla
            cmp         nerrors
            lda         #0
            bcs         :+                                  ; (No more errors than before: 0)
            lda         #1
:
            jmp         leave

; The line at r0 into line, as getline gives a file's: its comment (but its ;) and its blanks but one between words
; left out (outside "" and ''), a tab a blank; 255 bytes at most
lcopy:
            ldy         #0
            ldx         #0
            stz         gq
@byte:
            lda         (r0),Y
            beq         @end
            iny
            bit         gq                                  ; (In quotes: as it is, to the same quote)
            bpl         @out
            cmp         gqc
            bne         @keep
            stz         gq
            bra         @keep
@out:
            cmp         #TAB
            bne         :+
            lda         #' '
:
            cmp         #' '
            bne         @word
            cpx         #0                                  ; (No blank at the start, one between words)
            beq         @byte
            lda         line - 1,X
            cmp         #' '
            beq         @byte
            lda         #' '
            bra         @keep
@word:
            cmp         #'"'
            beq         @quote
            cmp         #$27
            beq         @quote
            cmp         #';'
            bne         @keep
            sta         line,X                              ; (A comment: its ; kept, the rest left out)
            inx
            bra         @end
@quote:
            sta         gqc
            dec         gq
@keep:
            cpx         #LINE_MAX
            bcs         @byte
            sta         line,X
            inx
            bra         @byte
@end:
            lda         #0
            sta         line,X
            stz         li
            rts

; END: the pass ended (an .if or .macro not ended said; pass 1's sizes kept).  OUT: C = 1 if it had an error
asm_end:
            jsr         enter
            jsr         seg_end
            lda         nerrors
            beq         :+
            lda         #1
:
            jmp         leave

; SYMBOL: r0 = a name.  OUT: C = 0, r1/r2 = its value (32 bits); C = 1 if there's no such symbol
asm_symbol:
            jsr         enter
            ldy         #0
:
            lda         (r0),Y
            sta         tok,Y
            beq         :+
            iny
            cpy         #TOK_MAX
            bne         :-
            lda         #1
            jmp         leave
:
            sty         tlen
            jsr         sym_find
            bcc         :+
            lda         #1
            jmp         leave
:
            ldy         #SY_VAL
            lda         (hp),Y
            sta         r1
            iny
            lda         (hp),Y
            sta         r1 + 1
            iny
            lda         (hp),Y
            sta         r2
            iny
            lda         (hp),Y
            sta         r2 + 1
            lda         #0
            jmp         leave

; IMAGE: OUT: .A = the image's first bank (the origin's bytes at $8000 in it: its first 8K), r0 = its length (from the
; origin to its data's end), r1 = its BSS's end (the first address past it)
asm_image:
            jsr         enter
            clc
            lda         sbasel + SEG_BSS
            adc         ssizel + SEG_BSS
            sta         r1
            lda         sbaseh + SEG_BSS
            adc         ssizeh + SEG_BSS
            sta         r1 + 1
            clc
            lda         sbasel + SEG_DATA
            adc         ssizel + SEG_DATA
            tax
            lda         sbaseh + SEG_DATA
            adc         ssizeh + SEG_DATA
            tay
            sec
            txa
            sbc         imgbase
            sta         r0
            tya
            sbc         imgbase + 1
            sta         r0 + 1
            lda         ibank
            jmp         leave

; DONE: .A = 1 to keep the image's first bank (the caller's from now on: the code in it), 0 not.  The banks the
; session took, given back
asm_done:
            jsr         enter
            cmp         #1
            bne         :+
            lda         ihave                               ; (The image's first kept: its others given back)
            beq         :+
            lda         ibank
            inc         a
            ldx         #IMG_BANKS - 1
            jsr         BANKS_FREE
            stz         ihave
:
            jsr         free_all
            lda         #0
            jmp         leave

; ERROR: OUT: r0 = the first message since BEGIN (AF_QUIET's: "name:line: what"), zero-terminated ("" if there's none)
asm_error:
            jsr         enter
            LDR         r0, errbuf
            lda         #0
            jmp         leave

; The name at r0 into the buffer at mp (FNAME_LEN bytes at most, its 0 too)
argcpy:
            ldy         #0
:
            lda         (r0),Y
            sta         (mp),Y
            beq         :+
            iny
            cpy         #FNAME_LEN - 1
            bne         :-
            lda         #0
            sta         (mp),Y
:
            rts

; ---- A pass
; ---- A pass

run:
            jsr         sym_init
            jsr         stmt_init
            stz         frames
            jsr         bases
            lda         fraw                                ; HYX2_RAM (a RAM program, for hyx2.inc)
            bne         :+
            lda         #1
            jsr         setval
            LDR         r0, s_hyx2ram
            jsr         sym_link
:
            LDR         r0, srcname
            jsr         push_file
            bcs         @end
@line:
            jsr         getline
            bcs         @end
            jsr         statement
            lda         nerrors
            cmp         #MAX_ERRORS
            bcc         @line
            LDR         r0, s_toomany
            jsr         lib_warn
@end:
            jmp         seg_end

; The segments' bases: pass 1's (ZEROPAGE at $22, the rest at the image's start, where they'll not be on the zero
; page); then each after the one before it, by pass 1's sizes, as hyx2.cfg lays them out, and ld65's names for them
bases:
            lda         #ZP_START
            sta         sbasel + SEG_ZP
            stz         sbaseh + SEG_ZP
            ldx         #SEG_HEADER
:
            lda         imgbase
            sta         sbasel,X
            lda         imgbase + 1
            sta         sbaseh,X
            inx
            cpx         #SEGS
            bne         :-
            lda         pass
            cmp         #1
            bne         :+
            rts
:
            ldx         #SEG_HEADER
@seg:
            clc
            lda         sbasel,X
            adc         ssizel,X
            sta         sbasel + 1,X
            lda         sbaseh,X
            adc         ssizeh,X
            sta         sbaseh + 1,X
            inx
            cpx         #SEG_BSS
            bne         @seg
            clc                                             ; The end: BSS's
            lda         sbasel + SEG_BSS
            adc         ssizel + SEG_BSS
            sta         ramlast
            lda         sbaseh + SEG_BSS
            adc         ssizeh + SEG_BSS
            sta         ramlast + 1
            lda         pass                                ; (Said in pass 2 alone)
            cmp         #2
            bne         @names
            lda         ssizeh + SEG_ZP                     ; The zero page: $5E bytes at most
            bne         @zpbig
            lda         ssizel + SEG_ZP
            cmp         #ZP_LEN + 1
            bcs         @zpbig
@ram:
            lda         fraw                                ; A RAM program: to $8000 at most
            bne         @names
            lda         ramlast
            cmp         imgbase
            lda         ramlast + 1
            sbc         imgbase + 1
            bcc         @big                                ; (Past $FFFF)
            lda         ramlast + 1
            cmp         #>RAM_END
            bcc         @names
            bne         @big
            lda         ramlast
            beq         @names
@big:
            LDR         r0, s_big
            jsr         err
            bra         @names
@zpbig:
            LDR         r0, s_zpbig
            jsr         err
            bra         @ram
@names:
            ldx         #SEG_DATA                           ; ld65's names
            jsr         segval
            LDR         r0, s_dataload
            jsr         sym_link
            LDR         r0, s_datarun
            jsr         sym_link
            ldx         #SEG_DATA
            jsr         sizeval
            LDR         r0, s_datasize
            jsr         sym_link
            ldx         #SEG_BSS
            jsr         segval
            LDR         r0, s_bssrun
            jsr         sym_link
            LDR         r0, s_bssload
            jsr         sym_link
            ldx         #SEG_BSS
            jsr         sizeval
            LDR         r0, s_bsssize
            jsr         sym_link
            lda         imgbase
            ldx         imgbase + 1
            jsr         setval2
            LDR         r0, s_ramstart
            jsr         sym_link
            sec
            lda         #<RAM_END
            sbc         imgbase
            pha
            lda         #>RAM_END
            sbc         imgbase + 1
            tax
            pla
            jsr         setval2
            LDR         r0, s_ramsize
            jsr         sym_link
            lda         ramlast
            ldx         ramlast + 1
            jsr         setval2
            LDR         r0, s_ramlast
            jsr         sym_link
            lda         #ZP_START
            jsr         setval
            LDR         r0, s_zpstart
            jsr         sym_link
            lda         #ZP_LEN
            jsr         setval
            LDR         r0, s_zpsize
            jsr         sym_link
            clc
            lda         #ZP_START
            adc         ssizel + SEG_ZP
            jsr         setval
            LDR         r0, s_zplast
            jmp         sym_link

; val = segment .X's base (segval), its size (sizeval); .A (setval), .A/.X (setval2)
segval:
            lda         sbasel,X
            pha
            lda         sbaseh,X
            bra         :+
sizeval:
            lda         ssizel,X
            pha
            lda         ssizeh,X
:
            tax
            pla
            bra         setval2
setval:                                                     ; (val = .A)
            ldx         #0
setval2:
            sta         val
            stx         val + 1
            stz         val + 2
            stz         val + 3
            rts

; ---- The input: a stack of frames (a file, or a macro's use).  A file is read whole the first time it's named, into
; RAM banks (the cache: cbanks), and each pass takes its lines from there

; A new frame on the stack (its kind still to be set): .X its index, p1 (and fp) at its buffer.  C = 1: too deep
; (said)
new_frame:
            ldx         frames
            cpx         #FRAMES
            bcs         @deep
            inc         frames
            stz         fll,X
            stz         flh,X
            lda         cdepth
            sta         fcdep,X
            jsr         frame_buf
            clc
            rts
@deep:
            LDR         r0, s_deep
            jsr         err
            sec
            rts

; p1 and fp at frame .X's buffer.  Keeps .X
frame_buf:
            lda         #<fbuf
            sta         fp
            sta         p1
            txa
            clc
            adc         #>fbuf
            sta         fp + 1
            sta         p1 + 1
            rts

; The file named at r0 (found as find_load finds it) on the input, at its start.  C = 1: not found, too deep, or no
; room (said)
push_file:
            jsr         find_load
            bcs         @done
            pha
            jsr         new_frame
            pla
            bcs         @done
            sta         fcf,X
            tay
            lda         #FR_FILE
            sta         ftype,X
            lda         cfirst,Y
            sta         fbk,X
            lda         #>BANK_AT
            sta         fpg,X
            stz         fofs,X
            clc
@done:
            rts

; mp = file frame .X's name (its cache entry's).  Keeps .X
fname_at:
            phx
            lda         fcf,X
            tax
            jsr         cname_at
            plx
            rts

; mp = cache entry .X's name.  Keeps .X
cname_at:
            stz         mp + 1                              ; (.X * FNAME_LEN: 64)
            txa
            ldy         #6
:
            asl         a
            rol         mp + 1
            dey
            bne         :-
            clc
            adc         #<cname
            sta         mp
            lda         mp + 1
            adc         #>cname
            sta         mp + 1
            rts

; The file named at r0, found: as it's named; else (a name not starting with / or #) beside the file being read,
; then in /lib/as.  find_include opens it to read: OUT: C = 0, .A = its fd.  find_load has it in the cache (read
; the first time; each name tried, there or not, kept there): OUT: C = 0, .A = its entry.  C = 1: not found (said),
; or no room (said).  Keeps r0
find_include:
            stz         fload
            bra         find
find_load:
            lda         #1
            sta         fload
find:
            stz         fbad
            ldx         #0                                  ; As it's named
            jsr         addname
            bcs         @none
            jsr         trypath
            bcc         @done
            lda         (r0)
            cmp         #'/'
            beq         @none
            cmp         #'#'
            beq         @none
            ldx         frames                              ; Beside the file being read: its name to its last /
@frame:
            dex
            bmi         @lib
            lda         ftype,X
            cmp         #FR_FILE
            bne         @frame
            jsr         fname_at
            ldy         #0
            ldx         #0                                  ; (.X: past its last /)
@byte:
            lda         (mp),Y
            beq         :+
            sta         path,Y
            iny
            cmp         #'/'
            bne         @byte
            tya
            tax
            bra         @byte
:
            cpx         #0
            beq         @lib
            jsr         addname
            bcs         @lib
            jsr         trypath
            bcc         @done
@lib:
            ldx         #0                                  ; In /lib/as
:
            lda         s_libas,X
            sta         path,X
            beq         :+
            inx
            bra         :-
:
            jsr         addname
            bcs         @none
            jsr         trypath
            bcc         @done
@none:
            lda         fbad                                ; (No room: said)
            bne         @fail
            jsr         msg_where                           ; "not found: name"
            LDR         mp, s_notfound
            jsr         msg_str
            lda         r0
            sta         mp
            lda         r0 + 1
            sta         mp + 1
            jsr         msg_str
            jsr         say
@fail:
            sec
@done:
            rts

; The name at r0 into path from .X on.  C = 1: too long
addname:
            ldy         #0
:
            lda         (r0),Y
            sta         path,X
            beq         :+
            iny
            inx
            cpx         #FNAME_LEN - 1
            bne         :-
            sec
            rts
:
            clc
            rts

; The file at path tried: find_load's, in the cache (its entry made the first time: read whole, or noted not there);
; find_include's, opened.  OUT: C = 0, .A = the entry or the fd; C = 1: not there (or no room: fbad).  Keeps r0
trypath:
            lda         fload
            beq         openpath
            jsr         clook                               ; Known already?
            bcs         @new
            lda         cmiss,X
            cmp         #1                                  ; (C = 1: not there)
            txa
            rts
@new:
            ldx         cfiles                              ; A new entry: its name ...
            cpx         #CFILES
            bcs         @room
            jsr         cname_at
            ldy         #0
:
            lda         path,Y
            sta         (mp),Y
            beq         :+
            iny
            bra         :-
:
            inc         cfiles
            lda         #1
            sta         cmiss,X
            phx
            jsr         openpath                            ;   then the file read, if it's there
            plx
            bcs         @done
            stz         cmiss,X
            jmp         cload
@room:
            LDR         r0, s_files
            jsr         err
            inc         fbad
            sec
@done:
            rts

; path opened to read.  OUT: C = 0, .A = the fd; C = 1: not there.  Keeps r0
openpath:
            lda         r0
            pha
            lda         r0 + 1
            pha
            LDR         r0, path
            lda         #O_READ
            jsr         OPEN
            tax
            pla
            sta         r0 + 1
            pla
            sta         r0
            txa
            rts

; C = 0: path is cache entry .X; C = 1: it's not in the cache
clook:
            ldx         #0
@entry:
            cpx         cfiles
            bcs         @no
            jsr         cname_at
            ldy         #0
:
            lda         path,Y
            cmp         (mp),Y
            bne         @next
            iny
            cmp         #0
            bne         :-
            clc
            rts
@next:
            inx
            bra         @entry
@no:
            rts

; The file open at fd .A read whole into the cache, for entry .X (its banks from the next in cbanks; a 0 after its
; last byte), and closed.  OUT: C = 0, .A = the entry; C = 1: no room, or it can't be read (said; fbad).  Keeps r0
cload:
            sta         cfd
            stx         cent
            lda         r0
            sta         cr0
            lda         r0 + 1
            sta         cr0 + 1
            lda         ncb
            sta         cfirst,X
            stz         cpos                                ; (cpos: where in its bank)
            stz         cpos + 1
            jsr         cbank
            bcs         @room
@read:
            lda         cpos                                ; To its bank's end
            sta         r0
            lda         cpos + 1
            clc
            adc         #>BANK_AT
            sta         r0 + 1
            sec
            lda         #<BANK_LEN
            sbc         cpos
            sta         r1
            lda         #>BANK_LEN
            sbc         cpos + 1
            sta         r1 + 1
            lda         cfd
            jsr         READ
            bcs         @ioerr
            sta         olen
            stx         olen + 1
            ora         olen + 1
            beq         @end
            clc
            lda         cpos
            adc         olen
            sta         cpos
            lda         cpos + 1
            adc         olen + 1
            sta         cpos + 1
            cmp         #>BANK_LEN
            bne         @read
            stz         cpos                                ; (Its bank full: another)
            stz         cpos + 1
            jsr         cbank
            bcc         @read
@room:
            LDR         r0, s_room
            bra         @fail
@ioerr:
            LDR         r0, s_ioerr
@fail:
            jsr         err
            inc         fbad
            ldx         cent
            lda         #1
            sta         cmiss,X
            jsr         @close
            sec
            rts
@end:
            ldx         cent                                ; Its end: a 0 there
            lda         ncb
            dec         a
            sta         cebk,X
            lda         cpos + 1
            clc
            adc         #>BANK_AT
            sta         cepg,X
            sta         mp + 1
            lda         cpos
            sta         ceofs,X
            sta         mp
            lda         #0
            sta         (mp)
            jsr         @close
            lda         cent
            clc
            rts
@close:
            lda         cfd
            jsr         CLOSE
            lda         cr0
            sta         r0
            lda         cr0 + 1
            sta         r0 + 1
            rts

; Another bank for the cache, selected.  C = 1: none
cbank:
            ldx         ncb
            cpx         #CBANKS
            bcs         @no
            lda         #1
            jsr         BANKS_ALLOC
            bcs         @no
            ldx         ncb
            sta         cbanks,X
            sta         BANKREG
            inc         ncb
            clc
@no:
            rts

; The next line into line (li = 0): the innermost frame's (a file's line, or a line of a macro's, its parameters
; made its arguments), a frame that's done taken off.  A file's line comes without its comment (but its ;) and its
; blanks but one between words (outside "" and ''); a 0 in it is a blank.  C = 1: there's none (the source's end)
getline:
            stz         li
@frame:
            ldx         frames
            bne         :+
            sec
            rts
:
            dex
            lda         ftype,X
            cmp         #FR_MACRO
            bne         @file
            jsr         mline
            bcc         @got
            ldx         frames                              ; (Its lines all used, or .exitmacro: the .ifs it
            dec         frames                              ;   started gone too)
            lda         fcdep - 1,X
            sta         cdepth
            bra         @frame
@got:
            clc
            rts
@file:
            stx         gfr
            ldy         fbk,X                               ; Its place: its bank ...
            lda         cbanks,Y
            sta         BANKREG
            stz         fp                                  ;   the page ...
            lda         fpg,X
            sta         fp + 1
            ldy         fofs,X                              ;   and where in it
            ldx         #0                                  ; (.X: in line; .Y: in the page)
            stz         gq
@byte:
            lda         (fp),Y
            beq         @zero0
            iny
            beq         @page
@byte1:
            cmp         #'0'                                ; (Most bytes: kept at once)
            bcc         @low
            cmp         #';'
            beq         @semi
@keep:
            sta         line,X
            inx
            bne         @byte
            dex                                             ; (Past 255: left out)
            bra         @byte
@page:
            jsr         nextpage
            bra         @byte1
@zero0:
            jmp         @zero
@low:
            cmp         #' '
            beq         @blank
            cmp         #LF
            beq         @eol
            cmp         #TAB
            beq         @blank
            cmp         #'"'
            beq         @quote
            cmp         #$27
            beq         @quote
            cmp         #CR
            bne         @keep
            bra         @byte
@blank:
            bit         gq
            bmi         @keep                               ; (In quotes: as it is)
            cpx         #0                                  ; One blank (none at the start) ...
            beq         @blanks
            lda         #' '
            sta         line,X
            inx
            bne         @blanks
            dex
@blanks:
            lda         (fp),Y                              ;   for the blanks there
            beq         @zero0
            iny
            beq         @bpage
@blanks1:
            cmp         #' '
            beq         @blanks
            cmp         #TAB
            beq         @blanks
            jmp         @byte1
@bpage:
            jsr         nextpage
            bra         @blanks1
@quote:
            bit         gq                                  ; (" or ': in quotes to the same again)
            bmi         :+
            sta         gqc
            dec         gq
            bra         @keep
:
            cmp         gqc
            bne         @keep
            stz         gq
            bra         @keep
@semi:
            bit         gq
            bmi         @keep
            sta         line,X                              ; A comment: its ; kept, the rest passed by
            inx
            bne         @comment
            dex
@comment:
            lda         (fp),Y
            beq         @zero
            iny
            beq         @cpage
@comment1:
            cmp         #LF
            bne         @comment
@eol:
            stx         lx
            ldx         gfr
@end:
            tya                                             ; (Its place kept)
            sta         fofs,X
            lda         fp + 1
            sta         fpg,X
            ldy         lx
            lda         #0
            sta         line,Y
            inc         fll,X
            bne         :+
            inc         flh,X
:
            clc
            rts
@cpage:
            jsr         nextpage
            bra         @comment1
@zero:
            stx         lx                                  ; A 0: the file's end (its entry's), or one in it
            ldx         gfr
            lda         fbk,X
            pha
            lda         fcf,X
            tax
            pla
            cmp         cebk,X
            bne         @nul
            lda         fp + 1
            cmp         cepg,X
            bne         @nul
            tya
            cmp         ceofs,X
            beq         @eof
@nul:
            ldx         lx                                  ; (One in it: a blank)
            iny
            bne         :+
            jsr         nextpage
:
            lda         #' '
            jmp         @blank
@eof:
            ldx         gfr
            lda         lx                                  ; The end: a last line without its LF ...
            bne         @end
            dec         frames                              ;   or the frame taken off
            jmp         @frame

; getline's page after the last (fp): the next page of its bank, or the next bank's first.  Keeps .A, .X, .Y
nextpage:
            inc         fp + 1
            pha
            lda         fp + 1
            cmp         #>(BANK_AT + BANK_LEN)
            bne         @done
            lda         #>BANK_AT
            sta         fp + 1
            phx
            ldx         gfr
            inc         fbk,X
            lda         fbk,X
            tax
            lda         cbanks,X
            sta         BANKREG
            plx
@done:
            pla
            rts

; ---- Errors (each keeps p1, p2, p3 and hp)

; An error, counted: "as: file:line: the message at r0" (fd 2)
err:
            jsr         msg_start
            jmp         say

; An error: "as: file:line: the message at r0: the name in tok" (not a cheap local's scope)
errtok:
            jsr         msg_start
            LDR         mp, s_colon
            jsr         msg_str
            ldy         tlen
            lda         tok
            cmp         #'@'
            bne         :+
            dey
            dey
:
            sty         mn
            ldy         #0
:
            cpy         mn
            beq         :+
            lda         tok,Y
            jsr         msg_ch
            iny
            bra         :-
:
            jmp         say

; A warning (in pass 3 alone), not counted: "as: file:line: warning: the message at r0"
warn:
            lda         pass
            cmp         #3
            beq         :+
            rts
:
            jsr         msg_where
            LDR         mp, s_warn
            jsr         msg_str
            jsr         msg_r0
            LDR         r0, msg                             ; (Not an error: the status kept)
            jmp         lib_say

; msg = "file:line: " and the message at r0
msg_start:
            jsr         msg_where
msg_r0:
            lda         r0
            sta         mp
            lda         r0 + 1
            sta         mp + 1
            jmp         msg_str

; msg = "file:line: " (the innermost file being read), if there's one
msg_where:
            stz         mlen
            stz         msg
            ldx         frames
@frame:
            dex
            bmi         @done
            lda         ftype,X
            cmp         #FR_FILE
            bne         @frame
            jsr         fname_at
            jsr         msg_str
            lda         #':'
            jsr         msg_ch
            lda         fll,X
            sta         mn
            lda         flh,X
            sta         mn + 1
            jsr         msg_num
            LDR         mp, s_colon
            jmp         msg_str
@done:
            lda         lmode                               ; (The caller's line: its source's name, its number)
            beq         :+
            LDR         mp, srcname
            jsr         msg_str
            lda         #':'
            jsr         msg_ch
            lda         lline
            sta         mn
            lda         lline + 1
            sta         mn + 1
            jsr         msg_num
            LDR         mp, s_colon
            jmp         msg_str
:
            rts

; The string at mp on msg's end.  Keeps .X
msg_str:
            ldy         #0
:
            lda         (mp),Y
            beq         :+
            jsr         msg_ch
            iny
            bne         :-
:
            rts

; .A on msg's end (158 bytes at most).  Keeps .X, .Y
msg_ch:
            phx
            ldx         mlen
            cpx         #158
            bcs         :+
            sta         msg,X
            stz         msg + 1,X
            inc         mlen
:
            plx
            rts

; The number mn (16 bits) in decimal on msg's end.  Keeps .X
msg_num:
            phx
            ldx         #0                                  ; (Its digits, the lowest first)
@digit:
            lda         #0                                  ; mn = mn / 10, the remainder in .A
            ldy         #16
:
            asl         mn
            rol         mn + 1
            rol         a
            cmp         #10
            bcc         :+
            sbc         #10
            inc         mn
:
            dey
            bne         :--
            ora         #'0'
            sta         nbuf,X
            inx
            lda         mn
            ora         mn + 1
            bne         @digit
:
            dex
            lda         nbuf,X
            jsr         msg_ch
            cpx         #0
            bne         :-
            plx
            rts

; msg said ("as: " and it, on fd 2), and counted
say:
            LDR         r0, msg
            jsr         lib_warn
            inc         nerrors
            rts

; The message at r0 said: "as: " and it, on fd 2; the status 1 (lib_warn), or as it was (lib_say)
lib_warn:
            lda         #1
            sta         status
lib_say:
            lda         quiet                               ; (AF_QUIET: the first kept, not said)
            beq         @say
            lda         errbuf
            bne         @kept
            ldy         #0
:
            lda         (r0),Y
            sta         errbuf,Y
            beq         @kept
            iny
            cpy         #159
            bne         :-
            lda         #0
            sta         errbuf,Y
@kept:
            rts
@say:
            lda         r0
            pha
            lda         r0 + 1
            pha
            LDR         r0, s_as
            jsr         puts2
            pla
            sta         r0 + 1
            pla
            sta         r0
            jsr         puts2
            LDR         r0, s_nl
            jmp         puts2

; Error .A about the name at r0: "as: name: why" on fd 2; the status 1
lib_err:
            pha
            jsr         lib_warn0
            LDR         r0, s_colon
            jsr         puts2
            pla
            pha
            LDR         r0, msg
            pla
            jsr         ERRSTR
            LDR         r0, msg
            jsr         puts2
            LDR         r0, s_nl
            jmp         puts2
lib_warn0:                                                  ; ("as: " and the name, the status 1)
            lda         #1
            sta         status
            lda         r0
            pha
            lda         r0 + 1
            pha
            LDR         r0, s_as
            jsr         puts2
            pla
            sta         r0 + 1
            pla
            sta         r0
            jmp         puts2

; The string at r0 on fd 2 (AF_QUIET: not)
puts2:
            lda         quiet
            beq         :+
            rts
:
            ldy         #0
:
            lda         (r0),Y
            beq         :+
            iny
            bne         :-
:
            sty         r1
            stz         r1 + 1
            lda         #2
            jmp         WRITE

; ---- The image

; .A at address p3 in the image (said if it's outside it).  Keeps p1, p2
emit_at:
            pha
            sec
            lda         p3
            sbc         imgbase
            sta         p3
            lda         p3 + 1
            sbc         imgbase + 1
            sta         p3 + 1
            bcc         @out
            cmp         #>IMG_LEN
            bcs         @out
            lsr         a                                   ; Its bank: the offset >> 13
            lsr         a
            lsr         a
            lsr         a
            lsr         a
            clc
            adc         ibank
            sta         BANKREG
            lda         p3 + 1
            and         #>(BANK_LEN - 1)
            ora         #>BANK_AT
            sta         p3 + 1
            pla
            sta         (p3)
            rts
@out:
            pla
            LDR         r0, s_outside
            jmp         err

; The image, from its start to DATA's end, into out (made if it's not there)
write_out:
            LDR         r0, outname
            jsr         create
            bcc         :+
            jmp         @fail
:
            sta         ofd
            clc                                             ; Its length
            lda         sbasel + SEG_DATA
            adc         ssizel + SEG_DATA
            tax
            lda         sbaseh + SEG_DATA
            adc         ssizeh + SEG_DATA
            tay
            sec
            txa
            sbc         imgbase
            sta         oleft
            tya
            sbc         imgbase + 1
            sta         oleft + 1
            stz         ooff
            stz         ooff + 1
@chunk:
            lda         oleft                               ; A bank's part at a time
            ora         oleft + 1
            bne         :+
            lda         ofd
            jmp         CLOSE
:
            lda         ooff + 1                            ; Its bank
            lsr         a
            lsr         a
            lsr         a
            lsr         a
            lsr         a
            clc
            adc         ibank
            sta         BANKREG
            lda         ooff                                ; r0: where in the window
            sta         r0
            lda         ooff + 1
            and         #>(BANK_LEN - 1)
            ora         #>BANK_AT
            sta         r0 + 1
            sec                                             ; olen: to the bank's end ...
            lda         #0
            sbc         ooff
            sta         olen
            lda         ooff + 1
            and         #>(BANK_LEN - 1)
            sta         olen + 1
            lda         #>BANK_LEN
            sbc         olen + 1
            sta         olen + 1
            cmp         oleft + 1                           ;   or what's left, if that's less
            bcc         @write
            bne         :+
            lda         olen
            cmp         oleft
            bcc         @write
:
            lda         oleft
            sta         olen
            lda         oleft + 1
            sta         olen + 1
@write:
            lda         olen
            sta         r1
            lda         olen + 1
            sta         r1 + 1
            lda         ofd
            jsr         WRITE
            bcs         @wfail
            sta         olen                                ; (Written)
            stx         olen + 1
            ora         olen + 1
            beq         @wfail
            clc
            lda         ooff
            adc         olen
            sta         ooff
            lda         ooff + 1
            adc         olen + 1
            sta         ooff + 1
            sec
            lda         oleft
            sbc         olen
            sta         oleft
            lda         oleft + 1
            sbc         olen + 1
            sta         oleft + 1
            jmp         @chunk
@wfail:
            pha
            lda         ofd
            jsr         CLOSE
            pla
@fail:
            ldx         #<outname
            stx         r0
            ldx         #>outname
            stx         r0 + 1
            jmp         lib_err

; The file named at r0 opened to write, emptied (made if it's not there).  OUT: C = 0, .A = its fd; or C = 1, .A =
; the error
create:
            lda         r0
            pha
            lda         r0 + 1
            pha
            lda         #O_WRITE | O_TRUNC
            jsr         OPEN
            tax
            pla
            sta         r0 + 1
            pla
            sta         r0
            txa
            bcc         :+
            cmp         #E_NOENT
            sec
            bne         :+
            lda         #O_WRITE
            ldx         #0
            jsr         CREATE
:
            rts

; out.lbl: each label, "al 00XXXX .name" (ld65's -Ln form; not cheap locals)
write_labels:
            ldy         #0                                  ; Its name
:
            lda         outname,Y
            sta         path,Y
            beq         :+
            iny
            bra         :-
:
            ldx         #0
:
            lda         s_lbl,X
            sta         path,Y
            beq         :+
            iny
            inx
            bra         :-
:
            LDR         r0, path
            jsr         create
            bcs         @fail
            sta         ofd
            lda         #<lbl_one
            ldx         #>lbl_one
            jsr         sym_each
            lda         ofd
            jmp         CLOSE
@fail:
            pha
            LDR         r0, path
            pla
            jmp         lib_err

; The symbol at hp (sym_each's): its line in the labels file, if it's a label and not a cheap local
lbl_one:
            ldy         #SY_FLAGS
            lda         (hp),Y
            and         #SF_LABEL
            beq         @done
            ldy         #SY_NAME
            lda         (hp),Y
            cmp         #'@'
            beq         @done
            ldx         #0                                  ; "al 00"
:
            lda         s_al,X
            beq         :+
            sta         lbuf,X
            inx
            bra         :-
:
            ldy         #SY_VAL + 1                         ; Its value: 4 hex digits
            lda         (hp),Y
            jsr         hex2
            ldy         #SY_VAL
            lda         (hp),Y
            jsr         hex2
            lda         #' '
            sta         lbuf,X
            inx
            lda         #'.'
            sta         lbuf,X
            inx
            ldy         #SY_LEN                             ; Its name
            lda         (hp),Y
            sta         mn
            ldy         #SY_NAME
:
            lda         (hp),Y
            sta         lbuf,X
            inx
            iny
            dec         mn
            bne         :-
            lda         #LF
            sta         lbuf,X
            inx
            stx         r1
            stz         r1 + 1
            LDR         r0, lbuf
            lda         ofd
            jsr         WRITE
@done:
            rts

; .A as two hex digits into lbuf from .X on
hex2:
            pha
            lsr         a
            lsr         a
            lsr         a
            lsr         a
            jsr         :+
            pla
            and         #$0F
:
            cmp         #10
            bcc         :+
            adc         #'A' - '0' - 10 - 1
:
            adc         #'0'
            sta         lbuf,X
            inx
            rts

.rodata
s_as:       .byte       "as: ", 0
s_nl:       .byte       LF, 0
s_room:     .byte       "out of memory (RAM banks)", 0
s_toomany:  .byte       "too many errors", 0
s_deep:     .byte       ".include and macros nested too deep", 0
s_files:    .byte       "too many files", 0
s_ioerr:    .byte       "a file can't be read", 0
s_notfound: .byte       "not found: ", 0
s_colon:    .byte       ": ", 0
s_warn:     .byte       "warning: ", 0
s_outside:  .byte       "a byte outside the image (32K at most)", 0
s_zpbig:    .byte       "the zero page's $5E bytes overrun", 0
s_big:      .byte       "too big: past $8000", 0
s_libas:    .byte       "/lib/as/", 0
s_lbl:      .byte       ".lbl", 0
s_al:       .byte       "al 00", 0
s_hyx2ram:  .byte       "HYX2_RAM", 0
s_dataload: .byte       "__DATA_LOAD__", 0
s_datarun:  .byte       "__DATA_RUN__", 0
s_datasize: .byte       "__DATA_SIZE__", 0
s_bssrun:   .byte       "__BSS_RUN__", 0
s_bssload:  .byte       "__BSS_LOAD__", 0
s_bsssize:  .byte       "__BSS_SIZE__", 0
s_ramstart: .byte       "__RAM_START__", 0
s_ramsize:  .byte       "__RAM_SIZE__", 0
s_ramlast:  .byte       "__RAM_LAST__", 0
s_zpstart:  .byte       "__ZP_START__", 0
s_zpsize:   .byte       "__ZP_SIZE__", 0
s_zplast:   .byte       "__ZP_LAST__", 0

