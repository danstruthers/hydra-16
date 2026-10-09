; ****************************************************************************
; asm.inc - BASIC's inline assembly (the third bank): ASM ... END ASM, blocks of as's language (the asm library's,
; spec/asm.def: the assembler the as program is), assembled as the program's compiled into a bank of their own (8K,
; at $8000 while their code runs); and CALL ASM label [, a [, x [, y]]], which calls one of their labels as SYS calls
; code (RREG the registers after).
;   A block's lines are passed over by both passes (asm_skip, asm_scan: ASM alone on its line starts one, END ASM
; ends it).  Once the code's compiled, asm_build gives the library the blocks' lines in turn, three times (its passes),
; with the program's symbols defined first in each: each global number variable's name (in capitals, as BASIC keeps
; it, and in lower case) the address of its value (5 bytes: its kind, VT_INT 0, then an integer's 4 bytes), and each
; integer CONST's its value (but A, X and Y, the registers' names); then each CALL ASM's label is found (asm_tab: their
; addresses, by the numbers CALL ASM gave them).  An error is the library's text, at its line (ER_ASM: asm_msg).  The
; library takes $5000-$7DFF of this task's RAM while it runs (ASM_RAM: the value stack's top, unused while a program's
; compiled), and its zero page ($22 on) and RAM bank are put back after each call.

ASM_LBL_MAX     = 64            ; CALL ASM's labels, at most
ASM_ORG         = $8000         ; The blocks' code: from here, in its bank (8K)
ASM_TAB         = 9
SC_ASMLBL       = 'M'           ; A CALL ASM's label (scope 0, its name as written): its number (spl), the line it
                                ;   was first named on (spl + 1)

.bss
asm_open:   .res        1                                   ; A block open (its lines passed over) ...
asm_l0:     .res        2                                   ;   the line it started on
nasm:       .res        1                                   ; The program's blocks (pass 2's)
asm_nlbl:   .res        1                                   ; CALL ASM's labels so far
asm_tab:    .res        ASM_LBL_MAX * 2                     ; Their addresses (asm_build's)
asm_bank:   .res        1                                   ; The blocks' code's bank ...
asm_have:   .res        1                                   ;   1: it's taken
asm_mod:    .res        1                                   ; The asm library's module (ASMCALL's)
asm_msg:    .res        128                                 ; An error's text, the library's (ER_ASM's)
as_pass:    .res        1                                   ; (asm_build's pass)
as_b:       .res        1                                   ; (asm_walk's: the bucket ...
as_ent:     .res        2                                   ;   the entry, the next ...
as_next:    .res        2
as_cls:     .res        1                                   ;   its class ...
as_scope:   .res        1                                   ;   scope ...
as_len:     .res        1                                   ;   its name's length ...
as_name:    .res        NAME_MAX + 2                        ;   and characters (a 0 after them) ...
as_pl:      .res        SPL                                 ;   its payload ...
as_vec:     .res        2                                   ;   and the routine for each)
as_low:     .res        1                                   ; (asm_def's: letters made lower case)
as_info:    .res        ME_SIZE                             ; (asm_find's: a module's entry, its index)
as_idx:     .res        1

.segment "CODE3"

; The asm library's entry called (XCALL, its module asm_mod): .A and C its, both ways; .X and .Y not
.macro ACALL entry
            ldx         #<(entry)
            ldy         #>(entry)
            jsr         asm_call
.endmacro

asm_call:
            stx         r15
            sty         r15 + 1
            ldx         asm_mod
            stx         r14
            jmp         XCALL

; ---- A line's words: ASM and END ASM, alone on their lines, read from the text (a block's lines aren't BASIC's)

; .Y = lbuf's text past its blanks and a line number (and the blanks after it)
asm_text:
            ldy         #0
            jsr         @blanks
            lda         lbuf,y
            cmp         #'0'
            bcc         @done
            cmp         #'9' + 1
            bcs         @done
:
            iny
            lda         lbuf,y
            cmp         #'0'
            bcc         @blanks
            cmp         #'9' + 1
            bcc         :-
@blanks:
            lda         lbuf,y
            cmp         #' '
            beq         :+
            cmp         #ASM_TAB
            bne         @done
:
            iny
            bra         @blanks
@done:
            rts

; C = 0 if lbuf's line is ASM alone (or with a ' comment)
asm_isstart:
            jsr         asm_text
            ldx         #s_asmw - s_words
            jsr         asm_word
            bcs         :+
            jmp         asm_rest
:
            rts

; C = 0 if lbuf's line is END ASM alone
asm_isend:
            jsr         asm_text
            ldx         #s_endw - s_words
            jsr         asm_word
            bcs         @no
            lda         lbuf,y                              ; (A blank or more between)
            cmp         #' '
            beq         :+
            cmp         #ASM_TAB
            bne         @no
:
            iny
            lda         lbuf,y
            cmp         #' '
            beq         :-
            cmp         #ASM_TAB
            beq         :-
            ldx         #s_asmw - s_words
            jsr         asm_word
            bcs         @no
            jmp         asm_rest
@no:
            sec
            rts

; C = 0 and .Y past it if lbuf's text at .Y is the word at s_words + .X (in either case), and it ends there
asm_word:
            lda         s_words,x
            beq         @end
            lda         lbuf,y
            cmp         #'a'
            bcc         :+
            cmp         #'z' + 1
            bcs         :+
            and         #$DF
:
            cmp         s_words,x
            bne         @no
            iny
            inx
            bra         asm_word
@end:
            lda         lbuf,y                              ; (Not a longer name's start)
            cmp         #'_'
            beq         @no
            cmp         #'.'
            beq         @no
            cmp         #'0'
            bcc         @yes
            cmp         #'9' + 1
            bcc         @no
            and         #$DF
            cmp         #'A'
            bcc         @yes
            cmp         #'Z' + 1
            bcc         @no
@yes:
            clc
            rts
@no:
            sec
            rts

; C = 0 if lbuf's text from .Y on is blanks, then its end or a ' comment
asm_rest:
            lda         lbuf,y
            beq         @yes
            cmp         #' '
            beq         @next
            cmp         #ASM_TAB
            beq         @next
            cmp         #$27
            beq         @yes
            sec
            rts
@next:
            iny
            bra         asm_rest
@yes:
            clc
            rts

s_words:
s_asmw:     .byte       "ASM", 0
s_endw:     .byte       "END", 0

; ---- The passes over the program's lines

; A block's line passed over (comp.inc's c_scan and c_line, while asm_open): END ASM its end
asm_skip:
            jsr         asm_isend
            bcs         :+
            stz         asm_open
:
            rts

; Pass 1: ASM's line (c_scan's: its first word ASM): its block's lines passed over to END ASM
asm_scan:
            jsr         asm_isstart
            bcs         asm_rts
asm_opened:
            lda         #1
            sta         asm_open
            lda         cline
            sta         asm_l0
            lda         cline + 1
            sta         asm_l0 + 1
asm_rts:
            rts

; Pass 2: ASM, alone on its line (and not at the prompt): its block's lines passed over to END ASM, assembled once
; the code's compiled
cs_asm:
            lda         cmode
            beq         :+
            lda         #ER_DIRECT
            ldx         #0
            jmp         b_error
:
            jsr         asm_isstart
            bcc         :+
            jmp         syn3
:
            inc         nasm
            bra         asm_opened

; CALL ASM label [, a [, x [, y]]] (procs.inc's CALL: ASM current): an ASM block's label called, as SYS calls code
; (.A, .X and .Y given, else 0; RREG's after).  Its label: its name as it's written (the assembler's case; a keyword's
; spelling too: double), a number
; of its own (asm_tab's), found once the blocks are assembled; at the prompt, one the program's CALL ASM named
cs_callasm:
            C2          lex_next
            lda         tk                                  ; (A name, or a keyword's: the assembler's own names)
            cmp         #TK_NAME
            beq         :+
            cmp         #TK_KW
            bne         @syntax
:
            lda         tsuf
            beq         :+
@syntax:
            jmp         syn3
:
            ldy         #0                                  ; (Its name as written)
:
            lda         (tstart),y
            sta         tname,y
            iny
            cpy         tnl
            bne         :-
            lda         #SC_ASMLBL
            sta         scls
            stz         sscope
            C2          sym_find
            bcc         @have
            lda         cmode                               ; (At the prompt: the program's labels alone)
            beq         :+
            lda         #ER_LABEL
            ldx         #0
            jmp         b_error
:
            lda         asm_nlbl                            ; (A new one: its number, its line)
            cmp         #ASM_LBL_MAX
            bcc         :+
            lda         #ER_MEMORY
            ldx         #0
            jmp         b_error
:
            C2          spl_clear
            lda         asm_nlbl
            sta         spl
            inc         asm_nlbl
            lda         cline
            sta         spl + 1
            lda         cline + 1
            sta         spl + 2
            lda         #SC_ASMLBL
            sta         scls
            stz         sscope
            C2          sym_add
@have:
            lda         spl
            jsr         emit_i8
            C2          lex_next
            lda         #3                                  ; (.A, .X, .Y: 0 if not given)
            sta         c3_t
@arg:
            lda         #','
            jsr         is_op3
            bne         @zero
            C2          lex_next
            C2          expr_num
            bra         @next
@zero:
            lda         #0
            jsr         emit_i8
@next:
            dec         c3_t
            bne         @arg
            lda         #ST_ASMCALL
            jmp         emit_st3

; ---- The blocks assembled (compile_prog's, once the code's compiled, if the program has a block or a CALL ASM)

; The blocks' code before given back (compile_prog's, as it starts)
asm_free:
            lda         asm_have
            beq         :+
            lda         asm_bank
            ldx         #1
            jsr         BANKS_FREE
            stz         asm_have
:
            stz         nasm
            stz         asm_nlbl
            stz         asm_open
            rts

; The blocks given the asm library a line at a time, three passes, the program's symbols defined in each; the code
; kept, a bank of its own (asm_bank); each CALL ASM's label found (asm_tab).  An error: ER_ASM, the library's text, at
; its line
asm_build:
            stz         cont_ok                             ; (Its RAM the library's a while: CONT's state gone)
            jsr         asm_find
            bcc         :+
            LDW         t0, s_noasm
            jmp         asm_say
:
            LDW         r0, ASM_ORG
            stz         as_name                             ; (Its name: none, its messages' lines BASIC's)
            LDW         r1, as_name
            lda         #AF_QUIET
            ACALL       ASM_BEGIN
            bcc         :+
            lda         #ER_MEMORY
            ldx         #0
            jmp         b_error
:
            lda         #1
            sta         as_pass
@pass:
            lda         as_pass
            ACALL       ASM_PASS
            LDW         as_vec, asm_def
            jsr         asm_walk                            ; (The program's symbols)
            jsr         asm_lines
            bcs         @fail
            ACALL       ASM_END
            bcc         :+
            jsr         @last                               ; (An .if left open ...: at the last block)
@fail:
            jmp         asm_fail
:
            inc         as_pass
            lda         as_pass
            cmp         #4
            bne         @pass
            ACALL       ASM_IMAGE
            sta         asm_bank
            lda         r1 + 1                              ; (Its BSS's end too: the bank's 8K at most)
            cmp         #>$A000
            bcc         :+
            bne         @big
            lda         r1
            beq         :+
@big:
            lda         #0
            ACALL       ASM_DONE
            jsr         @last
            LDW         t0, s_asmbig
            jmp         asm_say
:
            LDW         as_vec, asm_lbl                     ; (CALL ASM's labels)
            jsr         asm_walk
            lda         #1                                  ; (The code's bank kept)
            ACALL       ASM_DONE
            lda         #1
            sta         asm_have
            jmp         code_sel
@last:
            lda         asm_l0                              ; (An error after the lines: said at the last block's
            sta         cline                               ;   ASM)
            lda         asm_l0 + 1
            sta         cline + 1
            rts

; An error the library said: its text (its ":line: " left out: the line's BASIC's) as ER_ASM's, at the line
asm_fail:
            ACALL       ASM_ERROR
            ldy         #0
            lda         (r0)                                ; (":12: what": past its ": ")
            cmp         #':'
            bne         @copy
:
            iny
            lda         (r0),y
            beq         @whole
            cmp         #':'
            bne         :-
            iny
            lda         (r0),y
            cmp         #' '
            bne         @copy
            iny
            bra         @copy
@whole:
            ldy         #0
@copy:
            ldx         #0
:
            lda         (r0),y
            sta         asm_msg,x
            beq         :+
            iny
            inx
            cpx         #127
            bne         :-
            stz         asm_msg,x
:
            lda         #0
            ACALL       ASM_DONE
            lda         #ER_ASM
            ldx         #0
            jmp         b_error

; The text at t0 (this bank's) as ER_ASM's
asm_say:
            ldy         #0
:
            lda         (t0),y
            sta         asm_msg,y
            beq         :+
            iny
            bra         :-
:
            lda         #ER_ASM
            ldx         #0
            jmp         b_error

; The program's lines read again (as c_lines reads them): each block's given the library (LINE), its number its
; messages'.  OUT: C = 1 if one had an error
asm_lines:
            stz         src_pos
            stz         src_pos + 1
            stz         cline
            stz         cline + 1
            stz         inc_seq
            stz         inc_dep
            stz         asm_open
@line:
            C2          line_next
            bcs         @done
            lda         asm_open
            bne         @in
            jsr         asm_isstart
            bcs         @line
            lda         #1
            sta         asm_open
            ldx         #s_code_end - s_code                ; (Each block from CODE: .code first)
:
            lda         s_code,x
            sta         lbuf,x
            dex
            bpl         :-
            bra         @give
@in:
            jsr         asm_isend
            bcs         :+
            stz         asm_open
            bra         @line
:
            jsr         asm_text                            ; (Its text: a line number left out)
@at:
            tya
            clc
            adc         #<lbuf
            sta         r0
            lda         #>lbuf
            adc         #0
            sta         r0 + 1
            lda         cline
            sta         r1
            lda         cline + 1
            sta         r1 + 1
            ACALL       ASM_LINE
            bcc         @line
            rts
@done:
            clc
            rts
@give:
            ldy         #0
            bra         @at

s_code:     .byte       ".code"
s_code_end: .byte       0

; Each symbol (the symbol region's buckets, each one's chain) read into as_*, then the routine at as_vec called
asm_walk:
            stz         as_b
@bucket:
            lda         as_b                                ; (Its first: 2 bytes at 2 * the bucket)
            asl         a
            sta         t0
            lda         #0
            rol         a
            sta         t0 + 1
            lda         #LB_SYM
            C2          ca_reg
            jsr         ca_rd
            sta         as_ent
            jsr         ca_rd
            sta         as_ent + 1
@entry:
            lda         as_ent
            ora         as_ent + 1
            beq         @next
            MOVW        t0, as_ent
            lda         #LB_SYM
            C2          ca_reg
            jsr         ca_rd                               ; (Its next, class, scope, name, payload)
            sta         as_next
            jsr         ca_rd
            sta         as_next + 1
            jsr         ca_rd
            sta         as_cls
            jsr         ca_rd
            sta         as_scope
            jsr         ca_rd
            sta         as_len
            ldx         #0
:
            cpx         as_len
            beq         :+
            jsr         ca_rd
            sta         as_name,x
            inx
            bra         :-
:
            stz         as_name,x
            ldx         #0
:
            jsr         ca_rd
            sta         as_pl,x
            inx
            cpx         #SPL
            bne         :-
            jsr         @call
            MOVW        as_ent, as_next
            bra         @entry
@next:
            inc         as_b
            beq         :+
            jmp         @bucket
:
            jmp         code_sel
@call:
            jmp         (as_vec)

; Each pass's symbols (asm_walk's): a global number variable's its value's address, an integer CONST's its value;
; each name in capitals and in lower case, if it's one the assembler reads (letters, digits and _, a letter first)
; and not A, X or Y (the registers')
asm_def:
            lda         as_scope
            bne         @no0
            lda         as_cls
            cmp         #SC_NUM
            beq         @var
            cmp         #SC_CONST
            bne         @no0
            lda         as_pl                               ; (An integer's: kind 0)
            beq         :+
@no0:
            rts
:
            lda         as_pl + 1
            sta         r1
            lda         as_pl + 2
            sta         r1 + 1
            lda         as_pl + 3
            sta         r2
            lda         as_pl + 4
            sta         r2 + 1
            bra         @name
@var:
            lda         as_pl + SP_SLOT
            sta         r1
            lda         as_pl + SP_SLOT + 1
            sta         r1 + 1
            stz         r2
            stz         r2 + 1
@name:
            lda         as_len                              ; (A, X, Y: the registers')
            cmp         #1
            bne         :+
            lda         as_name
            cmp         #'A'
            beq         @no
            cmp         #'X'
            beq         @no
            cmp         #'Y'
            beq         @no
:
            lda         as_name                             ; (A letter first; letters, digits, _ after)
            cmp         #'A'
            bcc         @no
            cmp         #'Z' + 1
            bcs         @no
            ldx         #0
@char:
            lda         as_name,x
            beq         @ok
            cmp         #'_'
            beq         :+
            cmp         #'0'
            bcc         @no
            cmp         #'9' + 1
            bcc         :+
            cmp         #'A'
            bcc         @no
            cmp         #'Z' + 1
            bcs         @no
:
            inx
            bra         @char
@ok:
            LDW         r0, as_name
            ACALL       ASM_DEFINE                          ; (In capitals ...)
            stz         as_low                              ;   and in lower case, if that's another
            ldx         #0
@low:
            lda         as_name,x
            beq         :++
            cmp         #'A'
            bcc         :+
            cmp         #'Z' + 1
            bcs         :+
            ora         #$20
            sta         as_name,x
            inc         as_low
:
            inx
            bra         @low
:
            lda         as_low
            beq         @no
            LDW         r0, as_name
            ACALL       ASM_DEFINE
@no:
            rts

; Each CALL ASM's label (asm_walk's): its address, the assembler's, in asm_tab (by its number); not there, an error
; at the line it was first named on
asm_lbl:
            lda         as_cls
            cmp         #SC_ASMLBL
            bne         @done
            LDW         r0, as_name
            ACALL       ASM_SYMBOL
            bcs         @none
            lda         as_pl                               ; (asm_tab + 2 * its number)
            asl         a
            tax
            lda         r1
            sta         asm_tab,x
            lda         r1 + 1
            sta         asm_tab + 1,x
@done:
            rts
@none:
            lda         as_pl + 1
            sta         cline
            lda         as_pl + 2
            sta         cline + 1
            lda         #0
            ACALL       ASM_DONE
            ldx         #0                                  ; ("label: no such label in an ASM block")
:
            lda         as_name,x
            sta         asm_msg,x
            beq         :+
            inx
            bra         :-
:
            ldy         #0
:
            lda         s_asmlbl,y
            sta         asm_msg,x
            beq         :+
            inx
            iny
            bra         :-
:
            lda         #ER_ASM
            ldx         #0
            jmp         b_error

; asm_mod = the asm library's module (MODINFO: "asm").  C = 1: there's none
asm_find:
            stz         as_idx
@entry:
            LDW         r0, as_info
            lda         as_idx
            jsr         MODINFO
            bcs         @rts
            inc         as_idx
            lda         as_info + ME_TYPE
            cmp         #HT_LIBRARY
            bne         @entry
            ldx         #0
:
            lda         s_asmlib,x
            cmp         as_info + ME_NAME,x
            bne         @entry
            inx
            cmp         #0
            bne         :-
            lda         as_info + ME_BANK
            sta         asm_mod
            clc
@rts:
            rts

s_asmlib:   .byte       "asm", 0
s_noasm:    .byte       "no asm library", 0
s_asmbig:   .byte       "the blocks past 8K", 0
s_asmlbl:    .byte       ": no such label in an ASM block", 0
