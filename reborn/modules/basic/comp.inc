; ****************************************************************************
; comp.inc - the compiler's core (the second bank): the code emitted (cursor B, at the code's end), the program's lines
; read (from the text region, a _ at a line's end joining the next), the symbols (a hash table of names in the symbol
; region: variables, arrays, procedures, labels, constants, types, each in its scope), labels and their fixups, the
; line table (each statement's code address and line, for errors), and the compilation of the program (two passes:
; the procedures and types found first, so a call needs no DECLARE; then the code) and of a line at the prompt.

SYM_BUCKETS     = 0             ; The symbol region: 256 buckets (2 bytes each: an entry, 0 none) ...
SYM_FIRST       = 512           ;   then the entries, upwards ...
SYM_END         = $8000         ;   and the fixups, downwards from its end (32K: 4 banks)
SPL             = 10            ; An entry's payload (spl)
GLOB_MAX        = 4000          ; The globals' room (800 values)
PROC_MAX        = 128
PE_SIZE         = 8             ; A procedure's entry (procs): its code (2), parameters (1), locals (1), its locals'
                                ;   kinds' code address (2), flags (1), its result's kind (1)
BLK_MAX         = 32            ; Blocks open at once (IF, FOR, DO ...)
BE_SIZE         = 14            ; (A block's entry: its kind, then its own: stmt.inc; its line, the last two)
BE_LINE         = 12

; A symbol's class (scls)
SC_NUM          = 'N'           ; A number's variable
SC_STR          = 'S'           ; A string's
SC_REC          = 'R'           ; A record's
SC_NARR         = 'A'           ; Arrays: of numbers, strings, records
SC_SARR         = 'B'
SC_RARR         = 'Q'
SC_PROC         = 'P'           ; A SUB or FUNCTION (scope 0)
SC_LABEL        = 'L'           ; A label or a line number (its procedure's scope)
SC_CONST        = 'C'           ; A constant (scope 0)
SC_TYPE         = 'T'           ; A TYPE (scope 0)

; A variable's payload: its slot (2: a global's address, or a frame's slot), flags (1), its type (1: a record's
; type, a string's fixed length's flag), its fixed length or type's info (2)
SP_SLOT         = 0
SP_FLAGS        = 2
SP_TYPE         = 3
SP_INFO         = 4
SF_SHARED       = $01           ; (DIM SHARED: every procedure's)
SF_DECL         = $02           ; (Declared: DIM, AS)
SF_FIXED        = $04           ; (A string of a fixed length: SP_INFO)
; A label's: defined (1), its code address (2), its DATA's place (2), its line (2)
SL_DEF          = 0
SL_CODE         = 1
SL_DATA         = 3
; A procedure's: its number (1), FUNCTION (1: 0 a SUB, 1 a number's FUNCTION, 2 a string's), its parameters (1),
; its parameters' kinds' place in the symbol region (2: a byte each, PK_*)
SPR_NUM         = 0
SPR_FUNC        = 1
SPR_NP          = 2
SPR_PK          = 3
PK_NUM          = 0             ; A parameter's kinds: a number, a string, an array of numbers or strings, a record
PK_STR          = 1
PK_NARR         = 2
PK_SARR         = 3
PK_REC          = 4             ; (| its type << 3: a record's, or an array of them with PK_RARR)
PK_RARR         = 5
; A constant's: its value's kind (1: 0 an integer, 1 a number, 2 a string), its integer (4) or its bytes' length (2),
; then (past the payload) its bytes
; A local's kinds (the locals' list after a procedure: CALL's): as the values' defaults
LK_NUM          = 0
LK_STR          = 1
LK_ARR          = 2
LK_REC          = 3
LK_SARR         = 4             ; (An array of strings: its VT_ARR not made yet, its byte 1 LK_STR)
LK_RARR         = 5             ; (An array of records: byte 1 LK_REC; with LK_REC, | the type << 3: its byte 4)


.bss
cmode:      .res        1                                   ; 0: the program; 1: a line at the prompt
cpass:      .res        1                                   ; 1, 2: the pass
c_s:        .res        1                                   ; The 6502's stack as the compilation began (an error's
                                                            ;   way out)
cline:      .res        2                                   ; The line read (from 1)
inc_n:      .res        1                                   ; INCLUDE's files: those loaded (pass 1), the INCLUDE
inc_seq:    .res        1                                   ;   lines read in this pass, the text past them, each
inc_top:    .res        2                                   ;   file's text and path, those being read (each one's
inc_at:     .res        INC_MAX * 2                         ;   src_pos and cline after its INCLUDE), its name, its
inc_path:   .res        INC_MAX * 64                        ;   quote
inc_dep:    .res        1
inc_stk:    .res        INC_DEPTH * 4
inc_nm:     .res        PATH_MAX + 1
inc_q:      .res        1
src_pos:    .res        2                                   ; The text's next line (a text region address)
cscope:     .res        1                                   ; The scope: 0 the main program's, else a procedure's + 1
cproc:      .res        1                                   ; (Its procedure's number)
nprocs:     .res        1
gnext:      .res        2                                   ; The next global's address
gbase:      .res        2                                   ; The globals' first
lnext:      .res        1                                   ; The next local's slot (a procedure's)
lkinds:     .res        256                                 ; Its locals' kinds (their slots')
symtop:     .res        2                                   ; The symbols' next entry ...
fixtop:     .res        2                                   ;   and the fixups' last
scls:       .res        1                                   ; sym_find's and sym_add's class, scope, entry, payload
sscope:     .res        1
sent:       .res        2
spl:        .res        SPL
shash:      .res        1
ltop:       .res        2                                   ; The line table's next entry (a region address)
dtop:       .res        2                                   ; DATA's next item
blk:        .res        BLK_MAX * BE_SIZE                   ; The blocks open ...
nblk:       .res        1
defstr:     .res        4                                   ; DEFSTR's letters (a bit each)
optbase:    .res        1                                   ; OPTION BASE (0, 1)
procs:      .res        PROC_MAX * PE_SIZE                  ; The procedures (CALL's table)
ex_text:    .res        2                                   ; (ER_EXPECT's: what was expected)
cerr_col:   .res        2                                   ; (An error's column)
stmt_ok:    .res        1                                   ; (The statement's end checked)
cpc_t:      .res        1                                   ; (cpc_get's and ca_addr's)
lref_data:  .res        1                                   ; (label_ref's: <> 0, its DATA's place, not its code's)
cconst:     .res        1                                   ; <> 0: a CONST's value compiled (no variables, FUNCTIONs)

.segment "CODE2"

; ---- Errors

; A compilation's error: .A/.X its code.  Said with the file, line (and the column, at the prompt) by err_entry
c_error:
            jmp         b_error

; Syntax error, with what was expected: the string at .A/.X (zero-terminated)
c_expect:
            sta         ex_text
            stx         ex_text + 1
            lda         #ER_EXPECT
            ldx         #0
            jmp         b_error

; ---- Cursors on region addresses

; Cursor A (ca_reg) or B (cb_reg) at the region address t0 of the region whose first logical bank is .A; its bank
; selected
ca_reg:
            sta         cal
            lda         t0 + 1
            lsr         a
            lsr         a
            lsr         a
            lsr         a
            lsr         a
            clc
            adc         cal
            sta         cal
            lda         t0 + 1
            and         #$1F
            ora         #WIN
            sta         ca + 1
            lda         t0
            sta         ca
            jmp         ca_sel
cb_reg:
            sta         cbl
            lda         t0 + 1
            lsr         a
            lsr         a
            lsr         a
            lsr         a
            lsr         a
            clc
            adc         cbl
            sta         cbl
            lda         t0 + 1
            and         #$1F
            ora         #WIN
            sta         cb + 1
            lda         t0
            sta         cb
            jmp         cb_sel

; t0 = cursor A's region address (in the region whose first logical bank is .A)
ca_addr:
            sta         cpc_t
            lda         cal
            sec
            sbc         cpc_t
            asl         a
            asl         a
            asl         a
            asl         a
            asl         a
            sta         t0 + 1
            lda         ca + 1
            and         #$1F
            ora         t0 + 1
            sta         t0 + 1
            lda         ca
            sta         t0
            rts

; ---- The code: emitted at cursor B

; Code emitted from the start of the program's region (cmode 0) or the prompt's (1)
emit_init:
            stz         t0
            stz         t0 + 1
            lda         cmode
            beq         :+
            lda         #LB_IMM
:
            jmp         cb_reg

; Room for an op of .A bytes (and the BNK after it) in this bank: else BNK, and on at the next.  Keeps .A
emit_room:
            pha
            clc
            adc         cb
            lda         cb + 1
            adc         #0
            cmp         #WIN_END - 1                        ; (The op's end and a BNK below $9FFF: room)
            bcc         @room
            bne         @next
            pla
            pha
            clc
            adc         cb
            cmp         #$FF                                ; ($9Fxx: room while cb + n < $9FFF)
            bcc         @room
@next:
            lda         cbl                                 ; (The region's last bank: too big)
            and         #7
            cmp         #7
            beq         @full
            lda         #OP_BNK
            jsr         cb_sel
            sta         (cb)
            stz         cb
            lda         #WIN
            sta         cb + 1
            inc         cbl
            jsr         cb_sel
@room:
            pla
            rts
@full:
            lda         #ER_MEMORY
            ldx         #0
            jmp         c_error

; .A emitted (its op's room asked first).  Keeps .A, .X, .Y
emit:
            pha
            phx
            jsr         cb_sel
            plx
            pla
            jmp         cb_wr

; .A/.X (low, high) emitted
emit_w:
            jsr         emit
            txa
            jmp         emit

; An op of .A, its room asked for .X bytes in all
emit_op:
            pha
            txa
            jsr         emit_room
            pla
            jmp         emit

; .A/.X = the code's next address (cursor B's)
cpc_get:
            lda         cbl
            and         #7
            asl         a
            asl         a
            asl         a
            asl         a
            asl         a
            sta         cpc_t
            lda         cb + 1
            and         #$1F
            ora         cpc_t
            tax
            lda         cb
            rts

; The code address .A/.X put at the code address t0 (a jump's, a fixup's).  Keeps cursor B
patch_w:
            pha
            phx
            lda         #LB_CODE
            jsr         ca_reg
            plx
            pla
            jsr         ca_wr
            txa
            jsr         ca_wr
            jmp         cb_sel

; A jump op .A to an address not known yet: its operand's code address in t0 (for patch_w)
emit_fwd:
            ldx         #3
            jsr         emit_op
            jsr         cpc_get
            sta         t0
            stx         t0 + 1
            lda         #0
            tax
            jmp         emit_w

; A jump op .A to the code address t4/t4+1 (known)
emit_jmp:
            ldx         #3
            jsr         emit_op
            lda         t4
            ldx         t4 + 1
            jmp         emit_w

; ---- The program's lines (the text region: each its length, then its bytes; a length of $FF its end)

; The next line into lbuf (zero-terminated; a line ending in " _" goes on with the next), cline on.  OUT: C = 1 at
; the text's end
line_next:
            LDW         lp, lbuf
            stz         t4 + 2                              ; (Lines joined: none yet)
@more:
            MOVW        t0, src_pos
            lda         #LB_TEXT
            jsr         ca_reg
            jsr         ca_rd                               ; (Its length)
            cmp         #$FF
            bne         :+
            lda         t4 + 2                              ; (The end: after a _, the line so far)
            bne         @done
            lda         inc_dep                             ; (An INCLUDE's file's end: on after its INCLUDE)
            beq         @end
            jsr         inc_pop
            bra         @more
@end:
            sec
            jmp         code_sel
:
            inc         cline
            bne         :+
            inc         cline + 1
:
            sta         t4 + 3
            tax
            lda         lp                                  ; (Room: below LBUF_SIZE)
            sec
            sbc         #<lbuf
            sta         t4
            lda         lp + 1
            sbc         #>lbuf
            sta         t4 + 1
            txa
            clc
            adc         t4
            lda         t4 + 1
            adc         #0
            cmp         #>LBUF_SIZE
            bcc         :+
            lda         #ER_TOOLONG
            ldx         #0
            jmp         c_error
:
            ldy         #0
            cpx         #0
            beq         @copied
:
            jsr         ca_rd
            sta         (lp),y
            iny
            dex
            bne         :-
@copied:
            lda         #0
            sta         (lp),y
            sec                                             ; (src_pos past it: 1 + its length)
            lda         src_pos
            adc         t4 + 3
            sta         src_pos
            bcc         :+
            inc         src_pos + 1
:
            cpy         #2                                  ; A _ at its end, after a space: the next goes on
            bcc         @done
            dey
            lda         (lp),y
            cmp         #'_'
            bne         @done
            dey
            lda         (lp),y
            cmp         #' '
            bne         @done
            iny                                             ; (The _ taken off: the next line after the space)
            tya
            clc
            adc         lp
            sta         lp
            bcc         :+
            inc         lp + 1
:
            inc         t4 + 2
            jmp         @more
@done:
            LDW         lp, lbuf
            lda         inc_dep                             ; (The program's first line #!: a script's, passed over)
            bne         :+
            lda         cline + 1
            bne         :+
            lda         cline
            cmp         #1
            bne         :+
            lda         lbuf
            cmp         #'#'
            bne         :+
            lda         lbuf + 1
            cmp         #'!'
            bne         :+
            stz         lbuf
:
            jsr         inc_check                           ; (INCLUDE "f": f's lines next)
            bcc         :+
            jmp         line_next
:
            clc
            jmp         code_sel

; The INCLUDE being read done: src_pos and cline after it again
inc_pop:
            dec         inc_dep
            lda         inc_dep
            asl         a
            asl         a
            tax
            lda         inc_stk,x
            sta         src_pos
            lda         inc_stk + 1,x
            sta         src_pos + 1
            lda         inc_stk + 2,x
            sta         cline
            lda         inc_stk + 3,x
            sta         cline + 1
            rts

; Is lbuf an INCLUDE line: INCLUDE "f", or QuickBASIC's '$INCLUDE: 'f' (or REM $INCLUDE: 'f')?  Then f's lines are
; read next (pass 1 loads f into the text past the program's: inc_load; pass 2 finds it there, by its order), C = 1;
; else C = 0
inc_check:
            ldy         #0
            jsr         inc_sp
            lda         lbuf,y
            cmp         #$27
            beq         @rem
            ldx         #iw_rem - iw
            jsr         inc_word
            bcc         @meta
            ldx         #iw_inc - iw
            jsr         inc_word
            bcs         @no
            lda         #'"'
            bra         @name
@rem:
            iny
@meta:
            jsr         inc_sp
            ldx         #iw_meta - iw
            jsr         inc_word
            bcs         @no
            jsr         inc_sp
            lda         lbuf,y
            cmp         #':'
            bne         @no
            iny
            lda         #$27
@name:
            sta         inc_q                               ; (Its name, in its quotes)
            jsr         inc_sp
            lda         lbuf,y
            cmp         inc_q
            bne         @no
            iny
            ldx         #0
:
            lda         lbuf,y
            beq         @no
            cmp         inc_q
            beq         @got
            cpx         #PATH_MAX
            bcs         @bad
            sta         inc_nm,x
            inx
            iny
            bra         :-
@no:
            clc
            rts
@bad:
            lda         #ER_BADNAME
            ldx         #0
            jmp         c_error
@got:
            cpx         #0
            beq         @bad
            stz         inc_nm,x
            lda         cpass                               ; Pass 1: loaded
            cmp         #1
            bne         @find
            lda         inc_n
            cmp         #INC_MAX
            bcc         :+
            lda         #ER_TOOMANY
            ldx         #0
            jmp         c_error
:
            FARN        7, inc_load                         ; (C = 1: .A/.X the error)
            bcc         @find
            jmp         c_error
@find:
            lda         inc_dep                             ; (Where to go on after it, kept)
            cmp         #INC_DEPTH
            bcc         :+
            lda         #ER_STACK
            ldx         #0
            jmp         c_error
:
            asl         a
            asl         a
            tax
            lda         src_pos
            sta         inc_stk,x
            lda         src_pos + 1
            sta         inc_stk + 1,x
            lda         cline
            sta         inc_stk + 2,x
            lda         cline + 1
            sta         inc_stk + 3,x
            inc         inc_dep
            lda         inc_seq                             ; (Its file: by the INCLUDEs' order)
            inc         inc_seq
            pha
            asl         a
            tax
            lda         inc_at,x
            sta         src_pos
            lda         inc_at + 1,x
            sta         src_pos + 1
            pla                                             ; (Its lines: INC_LINE + its number * INC_STEP on)
            asl         a
            asl         a
            asl         a
            clc
            adc         #>INC_LINE
            sta         cline + 1
            stz         cline
            sec
            rts

; lbuf's spaces at .Y passed
inc_sp:
            lda         lbuf,y
            cmp         #' '
            beq         :+
            cmp         #9
            bne         @end
:
            iny
            bra         inc_sp
@end:
            rts

; The word at iw + .X (capitals, 0 after it) at lbuf + .Y, in either case?  C = 0 yes, .Y past it; C = 1 no
inc_word:
            phy
@c:
            lda         iw,x
            beq         @yes
            eor         lbuf,y
            and         #$DF
            bne         @no
            inx
            iny
            bra         @c
@yes:
            pla
            clc
            rts
@no:
            ply
            sec
            rts
iw:
iw_rem:     .byte       "REM", 0
iw_inc:     .byte       "INCLUDE", 0
iw_meta:    .byte       "$INCLUDE", 0

; ---- Symbols

; shash = the hash of tname (tnl characters)
sym_hash:
            lda         #0
            ldx         #0
:
            cpx         tnl
            beq         :+
            asl         a
            adc         #0
            eor         tname,x
            inx
            bra         :-
:
            sta         shash
            rts

; Cursor A at the symbol address .A/.X
sym_ca:
            sta         t0
            stx         t0 + 1
            lda         #LB_SYM
            jmp         ca_reg

; The symbol of tname, of class scls in scope sscope.  OUT: C = 0, sent its entry and spl its payload; C = 1 none.
; The code's bank selected after
sym_find:
            jsr         sym_hash
            lda         shash                               ; (Its bucket: 2 * hash)
            asl         a
            ldx         #0
            bcc         :+
            inx
:
            jsr         sym_ca
            jsr         ca_rd
            sta         sent
            jsr         ca_rd
            sta         sent + 1
@entry:
            lda         sent
            ora         sent + 1
            bne         :+
            sec
            jmp         code_sel
:
            lda         sent
            ldx         sent + 1
            jsr         sym_ca
            jsr         ca_rd                               ; (Its next, kept)
            sta         t4
            jsr         ca_rd
            sta         t4 + 1
            jsr         ca_rd
            cmp         scls
            bne         @next
            jsr         ca_rd
            cmp         sscope
            bne         @next
            jsr         ca_rd
            cmp         tnl
            bne         @next
            ldx         #0
:
            cpx         tnl
            beq         @found
            jsr         ca_rd
            cmp         tname,x
            bne         @next
            inx
            bra         :-
@found:
            ldx         #0                                  ; (Its payload)
:
            jsr         ca_rd
            sta         spl,x
            inx
            cpx         #SPL
            bne         :-
            clc
            jmp         code_sel
@next:
            MOVW        sent, t4
            bra         @entry

; A symbol made of tname, scls, sscope and spl (first in its bucket): sent.  The code's bank selected after
sym_add:
            lda         symtop + 1                          ; (Room: below the fixups)
            clc
            adc         #1
            cmp         fixtop + 1
            bcc         :+
            lda         #ER_MEMORY
            ldx         #0
            jmp         c_error
:
            jsr         sym_hash
            lda         shash                               ; Its bucket's first, its next
            asl         a
            ldx         #0
            bcc         :+
            inx
:
            sta         t4 + 2
            stx         t4 + 3
            jsr         sym_ca
            jsr         ca_rd
            sta         t4
            jsr         ca_rd
            sta         t4 + 1
            MOVW        sent, symtop
            lda         sent                                ; (The entry)
            ldx         sent + 1
            jsr         sym_ca
            lda         t4
            jsr         ca_wr
            lda         t4 + 1
            jsr         ca_wr
            lda         scls
            jsr         ca_wr
            lda         sscope
            jsr         ca_wr
            lda         tnl
            jsr         ca_wr
            ldx         #0
:
            cpx         tnl
            beq         :+
            lda         tname,x
            jsr         ca_wr
            inx
            bra         :-
:
            ldx         #0
:
            lda         spl,x
            jsr         ca_wr
            inx
            cpx         #SPL
            bne         :-
            lda         #LB_SYM                             ; (symtop: past it)
            jsr         ca_addr
            MOVW        symtop, t0
            lda         t4 + 2                              ; (The bucket: it first)
            ldx         t4 + 3
            jsr         sym_ca
            lda         sent
            jsr         ca_wr
            lda         sent + 1
            jsr         ca_wr
            jmp         code_sel

; sent's payload written again from spl.  The code's bank selected after
sym_put:
            lda         sent
            ldx         sent + 1
            jsr         sym_ca
            ldx         #5                                  ; (Past its next, class, scope and length)
:
            jsr         ca_rd
            dex
            bne         :-
            tax                                             ; (... and its name)
            beq         @pl
:
            jsr         ca_rd
            dex
            bne         :-
@pl:
            ldx         #0
:
            lda         spl,x
            jsr         ca_wr
            inx
            cpx         #SPL
            bne         :-
            jmp         code_sel

; The symbols emptied (the buckets cleared)
sym_reset:
            LDW         t0, SYM_BUCKETS
            lda         #LB_SYM
            jsr         ca_reg
            ldx         #0
:
            lda         #0
            jsr         ca_wr
            jsr         ca_wr
            dex
            bne         :-
            LDW         symtop, SYM_FIRST
            LDW         fixtop, SYM_END
            jmp         code_sel

; spl cleared
spl_clear:
            ldx         #SPL - 1
:
            stz         spl,x
            dex
            bpl         :-
            rts

; ---- Slots: a global's (gnext on), a local's (lnext on)

; A new global's address: .A/.X, its default value (its kind .Y: LK_*) written there.  None left: Out of memory
global_new:
            lda         gnext
            sta         t4
            clc
            adc         #VAL_SIZE
            sta         gnext
            lda         gnext + 1
            sta         t4 + 1
            adc         #0
            sta         gnext + 1
            lda         gnext                               ; (Past its room?)
            sec
            sbc         gbase
            lda         gnext + 1
            sbc         gbase + 1
            cmp         #>GLOB_MAX
            bcc         :+
            lda         #ER_MEMORY
            ldx         #0
            jmp         c_error
:
            tya
            jsr         default_value                       ; (At t4: its kind's default)
            lda         t4
            ldx         t4 + 1
            rts

; The default value of kind .A (LK_*) at (t4): 0, "", an array or a record not made yet
default_value:
            phy
            pha
            and         #7
            tax
            lda         lk_tag,x
            sta         (t4)
            ldy         #1
            lda         lk_b1,x
            sta         (t4),y
            iny
            lda         #0
            sta         (t4),y
            iny
            lda         lk_bank,x
            sta         (t4),y
            iny
            pla                                             ; (A record's type, an array of them's: byte 4)
            lsr         a
            lsr         a
            lsr         a
            sta         (t4),y
            ply
            rts
lk_tag:     .byte       VT_INT, VT_STR, VT_ARR, VT_REC, VT_ARR, VT_ARR
lk_bank:    .byte       0, NOBANK, NOBANK, NOBANK, NOBANK, NOBANK
lk_b1:      .byte       0, 0, LK_NUM, 0, LK_STR, LK_REC

; A new local's slot: .A, its kind .Y.  Past 255 slots: Out of memory
local_new:
            ldx         lnext
            cpx         #$FF
            bne         :+
            lda         #ER_MEMORY
            ldx         #0
            jmp         c_error
:
            tya
            sta         lkinds,x
            txa
            inc         lnext
            rts

; A variable's slot made, for tname in class scls (scope sscope), its kind .Y: a global's (scope 0, or STATIC's) or a
; local's; the symbol added (sent, spl).  OUT: spl's slot
var_new:
            jsr         spl_clear
            lda         sscope
            beq         @global
            lda         pr_static                           ; (SUB ... STATIC: a global's slot, in its scope)
            bne         @global
            jsr         local_new
            sta         spl + SP_SLOT
            stz         spl + SP_SLOT + 1
            jmp         sym_add
@global:
            jsr         global_new
            sta         spl + SP_SLOT
            stx         spl + SP_SLOT + 1
            jmp         sym_add

; ---- Labels and fixups

; The label of tname (scope cscope) at the code's next address (and DATA's next item): defined.  Defined twice:
; Duplicate label
label_def:
            lda         #SC_LABEL
            sta         scls
            lda         cscope
            sta         sscope
            jsr         sym_find
            bcs         @new
            lda         spl + SL_DEF
            beq         @set
            lda         #ER_DUPLABEL
            ldx         #0
            jmp         c_error
@new:
            jsr         spl_clear
            jsr         sym_add
@set:
            lda         #1
            sta         spl + SL_DEF
            jsr         cpc_get
            sta         spl + SL_CODE
            stx         spl + SL_CODE + 1
            MOVW        spl + SL_DATA, dtop
            jmp         sym_put

; A label's code address emitted (tname, in scope cscope): known now, or a fixup kept for the program's end
label_ref:
            lda         #SC_LABEL
            sta         scls
            lda         cscope
            sta         sscope
            jsr         sym_find
            bcc         :+
            lda         cmode                               ; (At the prompt: the program's, or Label not defined)
            bne         @undef
            jsr         spl_clear                           ; (Not seen yet: made, undefined)
            jsr         sym_add
:
            lda         spl + SL_DEF
            bne         :+
            lda         cmode
            beq         @fix
@undef:
            lda         #ER_LABEL
            ldx         #0
            jmp         c_error
:
            lda         lref_data                           ; (Its DATA's place, or its code's)
            beq         :+
            lda         spl + SL_DATA
            ldx         spl + SL_DATA + 1
            jmp         emit_w
:
            lda         spl + SL_CODE
            ldx         spl + SL_CODE + 1
            jmp         emit_w
@fix:
            lda         fixtop                              ; (The fixup: 6 bytes below the last)
            sec
            sbc         #6
            sta         fixtop
            bcs         :+
            dec         fixtop + 1
:
            lda         fixtop + 1
            cmp         symtop + 1
            bne         :+
            lda         #ER_MEMORY
            ldx         #0
            jmp         c_error
:
            lda         fixtop
            ldx         fixtop + 1
            jsr         sym_ca
            jsr         cpc_get                             ; (Where it goes, the label, the line)
            jsr         ca_wr
            txa
            jsr         ca_wr
            lda         sent
            jsr         ca_wr
            lda         sent + 1
            jsr         ca_wr
            lda         cline
            jsr         ca_wr
            lda         cline + 1                           ; (Bit 15: its DATA's place is wanted)
            ldx         lref_data
            beq         :+
            ora         #$80
:
            jsr         ca_wr
            jsr         code_sel
            lda         #0
            tax
            jmp         emit_w

; Every fixup done (the program's end): its label's address put in the code; a label never defined: Label not
; defined, at the line that named it
fix_all:
@each:
            lda         fixtop + 1
            cmp         #>SYM_END
            bne         :+
            lda         fixtop
            cmp         #<SYM_END
            bne         :+
            stz         lref_data
            rts
:
            lda         fixtop
            ldx         fixtop + 1
            jsr         sym_ca
            jsr         ca_rd
            sta         t4
            jsr         ca_rd
            sta         t4 + 1
            jsr         ca_rd
            sta         sent
            jsr         ca_rd
            sta         sent + 1
            jsr         ca_rd
            sta         cline
            jsr         ca_rd
            pha                                             ; (Bit 15: its DATA's place)
            and         #$7F
            sta         cline + 1
            pla
            and         #$80
            sta         lref_data
            lda         sent                                ; (The label's payload)
            ldx         sent + 1
            jsr         sym_ca
            ldx         #4
:
            jsr         ca_rd
            dex
            bne         :-
            jsr         ca_rd                               ; (Its length, then its name)
            tax
            beq         :++
:
            jsr         ca_rd
            dex
            bne         :-
:
            jsr         ca_rd                               ; (SL_DEF, SL_CODE, SL_DATA)
            beq         @undef
            jsr         ca_rd
            pha
            jsr         ca_rd
            tax
            pla
            ldy         lref_data
            beq         :+
            jsr         ca_rd
            pha
            jsr         ca_rd
            tax
            pla
:
            pha
            MOVW        t0, t4
            pla
            jsr         patch_w
            lda         fixtop
            clc
            adc         #6
            sta         fixtop
            bcs         @lb900
            jmp         @each
@lb900:
            inc         fixtop + 1
            jmp         @each
@undef:
            jsr         code_sel
            lda         #ER_LABEL
            ldx         #0
            jmp         c_error

; ---- The line table: a statement's code address and line (cline), at each statement's start (the program's)
lt_add:
            lda         cmode
            bne         @done
            MOVW        t0, ltop
            lda         #LB_LT
            jsr         ca_reg
            jsr         cpc_get
            jsr         ca_wr
            txa
            jsr         ca_wr
            lda         cline
            jsr         ca_wr
            lda         cline + 1
            jsr         ca_wr
            lda         ltop
            clc
            adc         #4
            sta         ltop
            bcc         :+
            inc         ltop + 1
:
            jsr         code_sel
@done:
            rts

; ---- The compilation

; An empty program's state: no symbols, globals, procedures, lines or data (NEW; the start)
prog_init:
            jsr         sym_reset
            stz         nprocs
            jsr         glob_init
            stz         ltop
            stz         ltop + 1
            stz         dtop
            stz         dtop + 1
            jsr         data_end                            ; (READ: none, Out of DATA)
            stz         dptr
            stz         dptr + 1
            stz         optbase
            stz         defstr
            stz         defstr + 1
            stz         defstr + 2
            stz         defstr + 3
            ; (fall through)

; The globals from gbase: the reserved ones first (G_RSV of them, each 0), the program's after
glob_init:
            MOVW        t4, gbase
            ldy         #G_RSV * VAL_SIZE - 1
            lda         #0
:
            sta         (t4),y
            dey
            bpl         :-
            clc
            lda         gbase
            adc         #G_RSV * VAL_SIZE
            sta         gnext
            lda         gbase + 1
            adc         #0
            sta         gnext + 1
            rts

; DATA's items' end: a length of $FFFF after the last
data_end:
            MOVW        t0, dtop
            lda         #LB_DATA
            jsr         ca_reg
            lda         #$FF
            jsr         ca_wr
            jsr         ca_wr
            jmp         code_sel

; The program compiled (the text region's lines), from its first line; its globals made; its ASM blocks assembled
; (asm.inc).  An error: err_entry's (with its line); C = 0 done
compile_prog:
            tsx
            stx         c_s
            stz         cmode
            jsr         c_begin
            FARN        3, asm_free                         ; (ASM's code before: given back)
            jsr         sym_reset
            stz         nprocs
            jsr         glob_init
            stz         ltop
            stz         ltop + 1
            stz         dtop
            stz         dtop + 1
            stz         defstr
            stz         defstr + 1
            stz         defstr + 2
            stz         defstr + 3
            stz         optbase
            stz         ntypes                              ; (No TYPEs yet)
            stz         ty_top
            stz         inc_n                               ; (INCLUDE's files: past the program's text)
            FARN        7, tx_endfind
            clc
            lda         tx_end
            adc         #1
            sta         inc_top
            lda         tx_end + 1
            adc         #0
            sta         inc_top + 1
            lda         #1                                  ; Pass 1: the procedures and types
            sta         cpass
            jsr         c_lines
            lda         #2                                  ; Pass 2: the code
            sta         cpass
            jsr         emit_init
            jsr         c_lines
            lda         nblk                                ; (A block left open)
            beq         :+
            jsr         blk_unclosed
:
            lda         ty_open                             ; (A TYPE without END TYPE)
            beq         :+
            lda         #ER_BLOCK
            ldx         #0
            jmp         c_error
:
            jsr         lt_add                              ; (Its end: a statement's place too)
            lda         #OP_END
            ldx         #1
            jsr         emit_op
            jsr         fix_all
            jsr         data_end
            lda         nasm                                ; (ASM's blocks assembled, CALL ASM's labels found)
            ora         asm_nlbl
            beq         :+
            FARN        3, asm_build
:
            clc
            rts

; A pass over the program's lines
c_lines:
            stz         src_pos
            stz         src_pos + 1
            stz         cline
            stz         cline + 1
            stz         cscope
            stz         nblk
            stz         stmt_ok
            stz         ty_open
            stz         asm_open
            stz         inc_seq
            stz         inc_dep
            stz         defstr                              ; (DEFSTR's letters: none at its start)
            stz         defstr + 1
            stz         defstr + 2
            stz         defstr + 3
            lda         #'d'                                ; (The program's numbers: read in decimal at its start)
            sta         cbase
            stz         cbase + 1
@line:
            jsr         line_next
            bcs         @done
            lda         cpass
            cmp         #1
            beq         :+
            jsr         c_line
            bra         @line
:
            jsr         c_scan
            bra         @line
@done:
            lda         asm_open                            ; (ASM without END ASM: said at its start)
            beq         :+
            stz         asm_open
            MOVW        cline, asm_l0
            lda         #ER_BLOCK
            ldx         #0
            jmp         c_error
:
            rts

; The compiler's state for a line or a program: the base its numbers are read in
c_begin:
            stz         cs_sp
            stz         asm_open
            stz         nblk
            stz         cconst
            stz         lref_data
            rts

; A line at the prompt compiled (lbuf), into the prompt's code region, after the program's symbols
compile_line:
            tsx
            stx         c_s
            lda         #1
            sta         cmode
            sta         cpass
            inc         cpass
            stz         cscope
            jsr         c_begin
            stz         ty_open
            stz         cline
            stz         cline + 1
            jsr         emit_init
            LDW         lp, lbuf
            jsr         c_line
            lda         nblk
            beq         :+
            jsr         blk_unclosed
:
            lda         #OP_END
            ldx         #1
            jsr         emit_op
            clc
            rts

; A line of the program compiled (pass 2): its label or number, then its statements (stmt.inc)
c_line:
            lda         asm_open                            ; (An ASM block's lines: passed over, to END ASM)
            beq         :+
            FARN        3, asm_skip
            rts
:
            lda         ty_open                             ; (A TYPE's lines: passed over)
            beq         :+
            FARN        3, ty_skip
            rts
:
            jsr         lex_next                            ; A number first: its label
            lda         tk
            cmp         #TK_NUM
            bne         @label
            lda         cmode                               ; (At the prompt: the prompt's own, not here)
            bne         @syntax
            jsr         num_label
            jsr         label_def
            jsr         lex_next
            bra         @stmts
@label:
            cmp         #TK_NAME                            ; A name and a colon: a label
            bne         @stmts
            lda         tsuf
            bne         @stmts
            lda         (lp)
            cmp         #':'
            bne         @stmts
            lda         cmode
            bne         @stmts
            jsr         label_def
            jsr         lp_inc
            jsr         lex_next
@stmts:
            lda         tk
            beq         @done
            jsr         lt_add
            jsr         c_stmt                              ; (stmt.inc: the statement, its next token read)
            lda         tk
            beq         @done
            cmp         #TK_OP
            bne         @syntax
            lda         tkv
            cmp         #':'
            bne         @syntax
            jsr         lex_next
            bra         @stmts
@done:
            rts
@syntax:
            lda         #ER_SYNTAX
            ldx         #0
            jmp         c_error

; tname = a line number's digits (the token was the number: its text at tstart)
num_label:
            ldy         #0
:
            lda         (tstart),y
            jsr         is_digit
            bcs         :+
            sta         tname,y
            iny
            cpy         #NAME_MAX
            bcc         :-
:
            sty         tnl
            stz         tsuf
            rts

; A line in pass 1: SUB and FUNCTION (their names and parameters), TYPE (its fields), DEFSTR (its letters), ASM (its
; block's lines passed over)
c_scan:
            lda         asm_open                            ; (An ASM block's lines: passed over, to END ASM)
            beq         :+
            FARN        3, asm_skip
            rts
:
            lda         ty_open                             ; (A TYPE's fields: the third bank's)
            beq         :+
            FARN        3, scan_field
            rts
:
            jsr         lex_next
            lda         tk
            cmp         #TK_NUM                             ; (A line number: past it)
            bne         :+
            jsr         lex_next
:
            lda         tk
            cmp         #TK_KW
            bne         @done
            lda         tkv
            cmp         #K_ASM                              ; (ASM: its block's lines passed over)
            bne         @declare
            FARN        3, asm_scan
            rts
@declare:
            cmp         #K_DECLARE                          ; (DECLARE SUB ...: as SUB)
            bne         :+
            jsr         lex_next
            lda         tk
            cmp         #TK_KW
            bne         @done
            lda         tkv
:
            cmp         #K_SUB
            beq         @proc
            cmp         #K_FUNCTION
            beq         @proc
            cmp         #K_TYPE
            bne         :+
            FARN        3, scan_type
            rts
:
            FARN        3, scan_defs                        ; (DEFSTR and the like: their letters)
@done:
            rts
@proc:
            jmp         scan_proc

; ---- Blocks: their stack (IF, FOR, DO, WHILE, SELECT, SUB, FUNCTION: stmt.inc)

; A block left open at the end: its error (FOR without NEXT ...)
blk_unclosed:
            lda         nblk
            dec         a
            jsr         blk_entry
            ldy         #BE_LINE                            ; (Said at its start)
            lda         (t4),y
            sta         cline
            iny
            lda         (t4),y
            sta         cline + 1
            lda         (t4)                               ; (Its kind: the error for it)
            tax
            lda         blk_errs,x
            ldx         #0
            jmp         c_error
blk_errs:   .byte       ER_BLOCK, ER_BLOCK, ER_FORNEXT, ER_BLOCK, ER_WHILE, ER_BLOCK, ER_BLOCK, ER_BLOCK, ER_BLOCK
