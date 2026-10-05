; ****************************************************************************
; hylang - danlang on the Hydra-16 (docs/hylang.md), written again from scratch.  As yet (phase 2) its REPL reads and
; prints: a line's items read (with the lines that go on with it, while a bracket or a here string is open), and what
; it read printed as danlang's REPL prints a value, not evaluated yet (phase 3).  hylang -g collects before every
; allocation (a test of the collector).
;   A program of four banks: the evaluator, the dispatch and the hot built-ins in the first; the reader, the printer
; and the list built-ins in the second; the numbers in the third; strings, hashes, streams and the system in the
; fourth.  heap.inc is in the task's RAM, where each bank calls it, as are the output and the note handler here; a
; bank calls another through FARN.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "hylang.inc"

            HYX2_PROGRAM "hylang", main, 4

.include "heap.inc"
.include "read.inc"
.include "print.inc"

IBUF_SIZE       = 128           ; stdin read this much at a time
OBUF_SIZE       = 128           ; The output written this much at a time

.zeropage
op:         .res        2                                   ; out_text's text
lp:         .res        2                                   ; read_line's place in text

.bss
ibuf:       .res        IBUF_SIZE                           ; stdin's bytes read, how many, and the next
ilen:       .res        1
ipos:       .res        1
obuf:       .res        OBUF_SIZE                           ; The output not written yet
olen:       .res        1
intr:       .res        1                                   ; Ctrl-C ($80), noted by the note handler
rl_any:     .res        1                                   ; (read_line's: some of a line read)
stress:     .res        1                                   ; (hylang -g)

.segment "DATA"
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

; .A out (out_flush writes it).  Keeps .X, .Y
out_byte:
            phy
            ldy         olen
            sta         obuf,y
            iny
            sty         olen
            cpy         #OBUF_SIZE
            bcc         :+
            jsr         out_flush
:
            ply
            rts

; What's out written, to stdout (a failure: no matter).  Keeps .X, .Y and the window ($00)
out_flush:
            lda         olen
            beq         @done
            phx
            phy
            sta         r1
            stz         r1 + 1
            lda         $00
            pha
            LDR         r0, obuf
            lda         #1
            jsr         WRITE
            pla
            sta         $00
            stz         olen
            ply
            plx
@done:
            rts

; The zero-terminated text at .A, .X (low, high: in the task's RAM, or in the caller's bank) out.  Keeps .Y
out_text:
            sta         op
            stx         op + 1
            phy
            ldy         #0
@byte:
            lda         (op),y
            beq         @done
            jsr         out_byte
            iny
            bne         @byte
@done:
            ply
            rts

.code

; The REPL: a line read (and more lines while it wants them), what it read printed (=> its repr: phase 3 evaluates
; it), till exit or stdin's end.  Ctrl-C gives the line up
main:
            HYX2_BANKS_INIT
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
            LDR         r0, notes
            jsr         NOTIFY
            jsr         heap_init
            bcc         :+
            lda         #<s_noheap
            ldx         #>s_noheap
            jsr         out_text
            lda         #1
            jmp         quit
:
            lda         stress
            sta         gc_stress
            lda         #<rd_roots                          ; (The reader's levels: roots while it reads)
            sta         gc_hook
            lda         #>rd_roots
            sta         gc_hook + 1
            lda         #<s_banner
            ldx         #>s_banner
            jsr         out_text
@expr:
            stz         t_len
            stz         t_len + 1
            stz         cl_len
@prompt:
            lda         cl_len
            bne         @more
            lda         #<s_prompt
            ldx         #>s_prompt
            jsr         out_text
            bra         @wait
@more:
            lda         #TAB                                ; (Another line wanted: the closers it wants)
            jsr         out_byte
            jsr         out_closers
            lda         #<s_more
            ldx         #>s_more
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
            beq         @ctrlc
            cmp         #3
            beq         @long
            lda         #0                                  ; (The text's end: a 0)
            sta         (lp)
            FARN        2, read_text
            bcs         @nomem
            cmp         #RD_MORE
            beq         @prompt
            cmp         #RD_ERROR
            beq         @show
            jsr         one_item
@show:
            lda         #<s_is
            ldx         #>s_is
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
            jmp         @expr
@ctrlc:
            lda         #LF
            jsr         out_byte
            jmp         @expr
@long:
            lda         #<s_long
            ldx         #>s_long
            jsr         out_text
            jmp         @expr
@nomem:
            lda         #<s_nomem
            ldx         #>s_nomem
            jsr         out_text
            jmp         @expr
@end:
            lda         t_len                               ; (stdin's end: exit, or what an open expression is
            ora         t_len + 1                           ;   missing)
            bne         @missing
            lda         #<s_bye
            ldx         #>s_bye
            jsr         out_text
            lda         #0
            jmp         quit
@missing:
            lda         #<s_missing
            ldx         #>s_missing
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

; hv, a line's S-expression: if it has one item, that item (as evaluating (x) gives x)
one_item:
            lda         hv + 1
            cmp         #IMM_PAGES
            bcc         @done
            tax
            lda         pk,x
            cmp         #PK_SCONS
            bne         @done
            lda         hv
            ldx         hv + 1
            ldy         #1
            jsr         cell_get
            cmp         #0
            bne         @done
            cpx         #0
            bne         @done
            lda         hv
            ldx         hv + 1
            ldy         #0
            jsr         cell_get
            sta         hv
            stx         hv + 1
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
s_banner:   .byte       "hylang (danlang on the Hydra-16), phase 2: it reads, and prints what it read", LF
            .byte       "Type 'exit' to Exit", LF, LF, 0
s_prompt:   .byte       "hylang> ", 0
s_more:     .byte       " <", 0
s_is:       .byte       "=> ", 0
s_bye:      .byte       "=> exit", LF, 0
s_missing:  .byte       "=> Error: missing ", 0
s_long:     .byte       "=> Error: Too long: an expression of more than 4096 bytes", LF, 0
s_nomem:    .byte       "=> Error: out of memory", LF, 0
s_noheap:   .byte       "hylang: no room for its heap", LF, 0

.segment "CODE3"                                            ; (The numbers: phase 5)
bank_three:
            rts

.segment "CODE4"                                            ; (Strings, hashes, streams, the system: phases 6, 7)
bank_four:
            rts
