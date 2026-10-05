; ****************************************************************************
; hylang - danlang on the Hydra-16 (docs/hylang.md), written again from scratch.  As yet (phase 4) its REPL, its
; evaluator, the list and type built-ins and the library's: a line read (with the lines that go on with it, while a
; bracket or a here string is open), evaluated, its value printed as danlang's REPL prints it.  hylang -g collects
; before every allocation (a test of what's kept as a root).
;   A program of four banks: the evaluator, its special forms, the dispatch and the built-ins that run it in the
; first (eval.inc, forms.inc, builtins.inc); the reader, the printer, the list built-ins and those that write values
; in the second (read.inc, print.inc, lists.inc, eqcmp.inc, valout.inc); the numbers in the third; strings, hashes,
; streams and the system in the fourth, with the most of the RAM code (DATA4, hylang.cfg: copied to the RAM as hylang
; starts).  What every bank calls is in the task's RAM: the heap (heap.inc), the output (out.inc), the evaluation
; stack (stack.inc), and the note handler here; a bank calls another through FARN.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "hylang.inc"

            HYX2_PROGRAM "hylang", main, 4

HL_DATA4        = 1             ; (The RAM code in DATA4: hylang.cfg)

.include "heap.inc"
.include "regs.inc"
.include "out.inc"
.include "stack.inc"
.include "read.inc"
.include "print.inc"
.include "eval.inc"
.include "forms.inc"
.include "builtins.inc"
.include "lists.inc"
.include "eqcmp.inc"
.include "valout.inc"
.include "nums.inc"

IBUF_SIZE       = 128           ; stdin read this much at a time

.zeropage
lp:         .res        2                                   ; read_line's place in text

.bss
ibuf:       .res        IBUF_SIZE                           ; stdin's bytes read, how many, and the next
ilen:       .res        1
ipos:       .res        1
intr:       .res        1                                   ; Ctrl-C ($80), noted by the note handler
rl_any:     .res        1                                   ; (read_line's: some of a line read)
stress:     .res        1                                   ; (hylang -g)

.segment "DATA4"
; The note handler, in RAM (any bank may be at $A000 when a note comes): Ctrl-C (NOTE_INTERRUPT) noted in intr;
; another, the default
notes:
            cmp         #NOTE_INTERRUPT
            bne         @default
            lda         #$80
            sta         intr
            clc
            rts
@default:
            sec
            rts

.code

; The REPL: a line read (and more lines while it wants them), evaluated, its value printed (=> its repr), till exit
; or stdin's end.  Ctrl-C gives the line up
main:
            HYX2_BANKS_INIT
            FARN        4, data4_init                       ; (The most of the RAM code: from the fourth bank)
            stz         stress
            lda         r0                                  ; (hylang -g: stress)
            ora         r0 + 1
            beq         @args
            lda         (r0)
            cmp         #'-'
            bne         @args
            ldy         #1
            lda         (r0),y
            cmp         #'g'
            bne         @args
            iny
            lda         (r0),y
            bne         @args
            inc         stress
@args:
            stz         olen
            stz         ilen
            stz         ipos
            stz         intr
            jsr         cap_reset
            LDR         r0, notes
            jsr         NOTIFY
            jsr         heap_init                           ; (The heap, the capture bank, the machine, the built-ins)
            bcs         @noroom
            lda         #1
            jsr         BANKS_ALLOC
            bcs         @noroom
            sta         cap_bank
            lda         #<hl_roots                          ; (The reader's levels, the machine's stack: roots)
            sta         gc_hook
            lda         #>hl_roots
            sta         gc_hook + 1
            jsr         ev_init
            bcs         @noroom
            FARN        2, bi_bind
            bcc         :+
@noroom:
            LDAX        s_noheap
            jsr         out_text
            lda         #1
            jmp         quit
:
            lda         stress
            sta         gc_stress
            LDAX        s_banner
            jsr         out_text
@expr:
            stz         t_len
            stz         t_len + 1
            stz         cl_len
@prompt:
            lda         cl_len
            bne         @more
            LDAX        s_prompt
            jsr         out_text
            bra         @wait
@more:
            lda         #TAB                                ; (Another line wanted: the closers it wants)
            jsr         out_byte
            jsr         out_closers
            LDAX        s_more
            jsr         out_text
@wait:
            jsr         out_flush
            lda         gc_fresh                            ; (A collection while the prompt waits, if pages were
            beq         @line                               ;   made a kind's since the last)
            jsr         gc_collect
@line:
            stz         intr
            jsr         read_line
            cmp         #1
            bne         :+
            jmp         @end
:
            cmp         #2
            bne         @lb15
            jmp         @ctrlc
@lb15:
            cmp         #3
            bne         @lb14
            jmp         @long
@lb14:
            lda         #0                                  ; (The text's end: a 0)
            sta         (lp)
            FARN        2, read_text
            bcs         @nomem
            cmp         #RD_MORE
            beq         @prompt
            cmp         #RD_ERROR
            beq         @show
            MOVW        ex, hv                              ; (The line's items, a call: evaluated)
            stz         ee
            stz         ee + 1
            jsr         ev_run
            MOVW        hv, ex
@show:
            LDAX        s_is
            jsr         out_text
            lda         hv
            ldx         hv + 1
            ldy         #0
            FARN        2, print_val
            lda         #LF
            jsr         out_byte
            lda         hv
            cmp         #<EXIT_VAL
            bne         @next
            lda         hv + 1
            cmp         #>EXIT_VAL
            bne         @next
            lda         #0
            jmp         quit
@next:
            stz         ex                                  ; (Nothing kept from the line)
            stz         ex + 1
            stz         hv
            stz         hv + 1
            jmp         @expr
@ctrlc:
            lda         #LF
            jsr         out_byte
            jmp         @expr
@long:
            LDAX        s_long
            jsr         out_text
            jmp         @expr
@nomem:
            LDAX        s_nomemline
            jsr         out_text
            jmp         @expr
@end:
            lda         t_len                               ; (stdin's end: exit, or what an open expression is
            ora         t_len + 1                           ;   missing)
            bne         @missing
            LDAX        s_bye
            jsr         out_text
            lda         #0
            jmp         quit
@missing:
            LDAX        s_missingl
            jsr         out_text
            jsr         out_closers
            lda         #LF
            jsr         out_byte
            lda         #0
; The end: the output written, and the status .A
quit:
            pha
            jsr         out_flush
            LDR         r0, 0
            pla
            jmp         EXITS

; closers out (the brackets an open expression wants)
out_closers:
            ldx         #0
@closer:
            cpx         cl_len
            bcs         @done
            lda         closers,x
            jsr         out_byte
            inx
            bra         @closer
@done:
            rts

; A line from stdin added to text, with its LF (a CR dropped).  OUT: .A = 0, a line (lp at its end); 1, stdin's end,
; nothing read; 2, Ctrl-C; 3, too long for text (the rest of the line dropped)
read_line:
            lda         #<text
            clc
            adc         t_len
            sta         lp
            lda         #>text
            adc         t_len + 1
            sta         lp + 1
            stz         rl_any
@byte:
            jsr         getc_in
            bcs         @end
            cmp         #CR
            beq         @byte
@add:
            ldx         t_len + 1
            cpx         #>TEXT_SIZE
            bcs         @full
            sta         (lp)
            inc         lp
            bne         :+
            inc         lp + 1
:
            inc         t_len
            bne         :+
            inc         t_len + 1
:
            ldx         #1
            stx         rl_any
            cmp         #LF
            bne         @byte
            lda         #0
            rts
@end:
            cmp         #E_INTR
            beq         @intr
            lda         rl_any                              ; (The end after some of a line: the line, its LF
            beq         @eof                                ;   added)
            stz         rl_any
            lda         #LF
            bra         @add
@eof:
            lda         #1
            rts
@intr:
            lda         #2
            rts
@full:
            cmp         #LF                                 ; (The rest of the line dropped)
            beq         @long
            jsr         getc_in
            bcc         @full
@long:
            lda         #3
            rts

; stdin's next byte.  OUT: C = 0, .A = it; or C = 1 and .A = E_INTR (Ctrl-C), or 0 (its end, or an error)
getc_in:
            ldx         ipos
            cpx         ilen
            bcc         @have
            jsr         out_flush                           ; (What's out first: a prompt)
            LDR         r0, ibuf
            LDR         r1, IBUF_SIZE
            lda         #0
            jsr         READ
            bcs         @error
            sta         ilen
            stz         ipos
            cmp         #0
            beq         @end
            ldx         #0
@have:
            lda         ibuf,x
            inx
            stx         ipos
            clc
            rts
@error:
            stz         ilen
            stz         ipos
            cmp         #E_INTR
            beq         @intr
@end:
            lda         #0
@intr:
            sec
            rts

.rodata
s_banner:   .byte       "hylang (danlang on the Hydra-16), phase 4: its built-ins and lists", LF
            .byte       "Type 'exit' to Exit", LF, LF, 0
s_prompt:   .byte       "hylang> ", 0
s_more:     .byte       " <", 0
s_is:       .byte       "=> ", 0
s_bye:      .byte       "=> exit", LF, 0
s_missingl: .byte       "=> Error: missing ", 0
s_long:     .byte       "=> Error: Too long: an expression of more than 4096 bytes", LF, 0
s_nomemline: .byte      "=> Error: out of memory", LF, 0
s_noheap:   .byte       "hylang: no room for its heap", LF, 0

.segment "CODE3"                                            ; (The numbers: phase 5)
bank_three:
            rts

.segment "CODE4"                                            ; (Strings, hashes, streams, the system: phases 6, 7)
; DATA4 (hylang.cfg's: the most of the RAM code, kept in this bank) copied to the task's RAM, as hylang starts
.import __DATA4_LOAD__, __DATA4_RUN__, __DATA4_SIZE__
data4_init:
            lda         #<__DATA4_LOAD__
            sta         hq
            lda         #>__DATA4_LOAD__
            sta         hq + 1
            lda         #<__DATA4_RUN__
            sta         hb
            lda         #>__DATA4_RUN__
            sta         hb + 1
            ldy         #0
            ldx         #>__DATA4_SIZE__                    ; (Its whole pages ...
            beq         @part
@page:
            lda         (hq),y
            sta         (hb),y
            iny
            bne         @page
            inc         hq + 1
            inc         hb + 1
            dex
            bne         @page
@part:
            ldx         #<__DATA4_SIZE__                    ;   and the rest)
            beq         @done
:
            lda         (hq),y
            sta         (hb),y
            iny
            dex
            bne         :-
@done:
            rts
