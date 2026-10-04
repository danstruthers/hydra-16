; ****************************************************************************
; hylang - danlang on the Hydra-16 (docs/hylang.md; the plan's §17, phase 7).  As yet (step 7.2b) its REPL: each line
; typed (or the text from rc's <) is read, with as many lines after it as an expression that isn't finished wants (a
; list or a here string open), and evaluated as danlang's REPL does, the expressions in it an S-expression: so
; "+ 1 2" is 3, and "(+ 1 2)" too.  Its value is shown, "=> " and its REPL form (a reading error as "=> Error: ...").
; exit ends it, and so does the input's end; (exit n) ends it with the status n.  Ctrl-C at the prompt is a new one,
; and while an expression is evaluated, the error interrupted.
;   The parts: heap.inc (the values, the heap, the collector: step 7.1), obj.inc (the objects: symbols, lists,
; errors), io.inc (output through a buffer; lines in) and env.inc (scopes, names, the evaluation stack, errors'
; messages), in the task's RAM, where every bank sees them; eval.inc (the evaluator: step 7.2b) and this REPL, in the
; module's first bank; read.inc (the reader), print.inc (the printer) and builtins.inc (the built-ins) in its second.
;   The rest is in library modules (HT_LIBRARY: hylang.cfg links them, each a module of its own), assembled with it,
; so each calls the others' routines (FARN, with the library's bank: hyx2_bank3 and hyx2_bank4, found as it starts):
; hylnum, the numbers (bignum.inc, numbers.inc and bases.inc: step 7.3), which it can't start without, and hylstr,
; strings and hashes (strings.inc and hashes.inc: step 7.4a), whose built-ins are there only if it is.

.include "hydra.inc"
.include "hw.inc"
.include "hyx2.inc"
.include "macros.inc"

            HYX2_PROGRAM "hylang", main, 2

.macpack longbranch

TEXT_SIZE       = 4096          ; The text being read: the REPL's line (or lines that go on), a file load reads, read's

.include "heap.inc"                                         ; (The task's RAM)
.include "obj.inc"
.include "io.inc"
.include "env.inc"
.include "eval.inc"                                         ; (The first bank)
.segment "CODE2"                                            ; (The second)
.include "read.inc"
.include "print.inc"
.include "builtins.inc"
            HYX2_LIBRARY "hylnum", "NUMHEAD", "NUM"           ; (The library hylnum)
.segment "CODE3"
.include "bignum.inc"
.include "numbers.inc"
.include "bases.inc"
            HYX2_LIBRARY "hylstr", "STRHEAD", "STR"           ; (The library hylstr)
.segment "CODE4"
.include "strings.inc"
.include "hashes.inc"

.bss
text:       .res        TEXT_SIZE
text_n:     .res        2
hyx2_bank3: .res        1                                   ; hylnum's bank (FARN 3's) ...
hyx2_bank4: .res        1                                   ;   and hylstr's (FARN 4's; 0: it isn't there)
lib_entry:  .res        1                                   ; (lib_find's: the module directory's entry ...
lib_me:     .res        ME_SIZE                             ;   and what MODINFO says of it)
.code

main:
            HYX2_BANKS_INIT
            jsr         lib_find
            lda         hyx2_bank3
            bne         :+
            PRINT       "hylang: its library hylnum isn't in the ROM"
            bra         @fail
:
            jsr         heap_init
            bcc         :+
            PRINT       "hylang: no room for its heap"
            bra         @fail
:
            jsr         obj_init
            jsr         eval_init
            bcc         @started
            PRINT       "hylang: no room"
@fail:
            PRINT       s_crlf                              ; (It can't start: status 1)
            LDR         r0, 0
            lda         #1
            jmp         EXITS
@started:
            LDR         r0, s_startup                       ; ---- The library: globals.hl
            jsr         set_text
            jsr         run_text
            jsr         ev_is_error
            bcc         :+
            jsr         show
:
            LDR         r0, s_banner
            jsr         out_text
@new:
            stz         text_n                              ; ---- A new expression's lines
            stz         text_n + 1
            stz         ev_intr
            ldx         #EV_REGS - 1                        ; (The registers let go: what the last line left there,
:                                                           ; a failed call's bindings ..., isn't kept)
            stz         ev_x,x
            dex
            bpl         :-
            LDR         r0, s_prompt
@prompt:
            jsr         out_text
@line:
            sec                                             ; (Room for a line: 255 at most)
            lda         #<TEXT_SIZE
            sbc         text_n
            tay
            lda         #>TEXT_SIZE
            sbc         text_n + 1
            beq         :+
            ldy         #255
:
            cpy         #2
            bcs         :+
            LDR         r0, s_too_long
            jsr         ev_error_text
            jsr         show
            bra         @new
:
            clc
            lda         #<text
            adc         text_n
            sta         r0
            lda         #>text
            adc         text_n + 1
            sta         r0 + 1
            jsr         in_line
            bcc         :+
            cmp         #E_INTR                             ; (Ctrl-C: a new line, and a new prompt)
            jne         @end
            jsr         out_newline
            bra         @new
:
            cmp         #0
            jeq         @end
            clc
            adc         text_n
            sta         text_n
            bcc         :+
            inc         text_n + 1
:
            jsr         text_start                          ; ---- Whole?  (Every expression in it read to its end)
@check:
            FAR2        read_expr
            bcc         @check
            cmp         #RD_MORE
            bne         @whole
            LDR         r0, s_more
            bra         @prompt
@whole:
            jsr         run_text                            ; ---- Evaluated, and its value shown (exit: the end)
            jsr         show
            lda         ev_v + 1
            jne         @new
            lda         ev_v
            cmp         #<EXIT_VAL
            jne         @new
@end:
            jsr         out_flush
            lda         #0
            rts

; The libraries' banks, from the module directory (MODINFO): hylnum's (hyx2_bank3) and hylstr's (hyx2_bank4); 0,
; one that isn't there
lib_find:
            stz         hyx2_bank3
            stz         hyx2_bank4
            stz         lib_entry
@entry:
            LDR         r0, lib_me
            lda         lib_entry
            jsr         MODINFO
            bcs         @done
            lda         lib_me + ME_TYPE
            cmp         #HT_LIBRARY
            bne         @next
            ldx         #0                                  ; (hylnum?)
:
            lda         lib_me + ME_NAME,x
            cmp         s_hylnum,x
            bne         @str
            inx
            cmp         #0
            bne         :-
            lda         lib_me + ME_BANK
            sta         hyx2_bank3
            bra         @next
@str:
            ldx         #0                                  ; (hylstr?)
:
            lda         lib_me + ME_NAME,x
            cmp         s_hylstr,x
            bne         @next
            inx
            cmp         #0
            bne         :-
            lda         lib_me + ME_BANK
            sta         hyx2_bank4
@next:
            inc         lib_entry
            bra         @entry
@done:
            rts

; The text's expressions read, and evaluated as one S-expression (danlang's REPL's way: "+ 1 2" is 3): ev_v, its
; value (or the error reading it)
run_text:
            jsr         text_start
            LDR         ev_a, NIL
            LDR         ev_t, NIL
@each:
            FAR2        read_expr
            bcs         @last
            MOVR        ev_v, h_v
            jsr         ev_append
            bcc         @each
@room:
            MOVR        ev_v, err_room
            rts
@last:
            cmp         #RD_ERR
            bne         :+
            MOVR        ev_v, h_v
            rts
:
            MOVR        h_v, ev_a
            lda         #OT_SEXPR
            jsr         make_list
            bcs         @room
            MOVR        ev_x, h_v
            LDR         ev_e, GLOBAL
            jmp         eval_nested

; The text the zero-terminated text at r0 (255 bytes at most)
set_text:
            ldy         #0
:
            lda         (r0),y
            beq         :+
            sta         text,y
            iny
            bne         :-
:
            sty         text_n
            stz         text_n + 1
            rts

; The reader started on the text
text_start:
            LDR         r0, text
            lda         text_n
            ldx         text_n + 1
            FAR2        read_start
            rts

; "=> ", ev_v as the REPL shows it, and a new line
show:
            MOVR        h_v, ev_v
            LDR         r0, s_arrow
            jsr         out_text
            FAR2        print_repr
            jmp         out_newline

.rodata
s_banner:   .byte       "hylang (danlang on the Hydra-16), exit to end", LF, 0
s_prompt:   .byte       "hylang> ", 0
s_more:     .byte       TAB, "< ", 0
s_arrow:    .byte       "=> ", 0
s_too_long: .byte       "Lines too long", 0
s_crlf:     .byte       CR, LF, 0
s_startup:  .byte       "load ", '"', "globals", '"', 0
s_hylnum:   .byte       "hylnum", 0
s_hylstr:   .byte       "hylstr", 0
.code
