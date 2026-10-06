; ****************************************************************************
; hylang - danlang on the Hydra-16 (docs/hylang.md), written again from scratch.  As yet (phase 7) all of danlang but
; its library: a line read (with the lines that go on with it, while a bracket or a here string is open), evaluated,
; its value printed as danlang's REPL prints it; or, hylang file args..., the file run (args: its path and the args),
; its status 0, 1 after an error (on stderr), or (exit n)'s.  hylang -g collects before every allocation (a test of
; what's kept as a root).  hylang -l is a login shell (phase 12: init's or wstart's, as /lib/shell names it): before
; its first prompt it runs #fx/lib/hylang/login.hl (its namespace made, then /lib/hylang/profile.hl, which turns the
; shell's rule on: shell.hl's shell-line and shell-prompt, which the REPL finds by name).
;   A program of five banks: the evaluator, its special forms, the dispatch and the built-ins that run it in the first
; (eval.inc, forms.inc, builtins.inc); the reader, the printer, the list built-ins and those that write values in the
; second (read.inc, print.inc, lists.inc, eqcmp.inc, valout.inc); the numbers in the third (nums.inc, numreg.inc,
; numval.inc, numtext.inc, numbi.inc, numbits.inc); the hashes and the strings in the fourth (hashes.inc, strs.inc),
; with the most of the RAM code (DATA4, hylang.cfg: copied to the RAM as hylang starts); the streams and the system
; library in the fifth (sys.inc, streams.inc, system.inc).  What every bank calls is in the task's RAM: the heap
; (heap.inc), the output (out.inc), the evaluation stack (stack.inc), and the note handler here; a bank calls another
; through FARN.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "hylang.inc"

            HYX2_PROGRAM "hylang", main, 6

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
.include "numreg.inc"
.include "numval.inc"
.include "numtext.inc"
.include "numbi.inc"
.include "numbits.inc"
.include "hashes.inc"
.include "strs.inc"
.include "sys.inc"
.include "streams.inc"
.include "system.inc"
.include "hydrabi.inc"
.include "hysys.inc"
.include "buffers.inc"

IBUF_SIZE       = 128           ; stdin read this much at a time

; A snapshot (the module hysnap, a library of data: tools/hysnap.js makes it at the build, from this hylang with its
; library loaded): after its header, at SN_INDEX, its index: "HYSN", the id of the hylang it's of (snap_id's), its
; cell banks and blob banks, PSTATE's size, a mask of each cell bank's pages kept (bit n: page n), each blob bank's
; length; then, at SN_DATA, PSTATE's bytes, each page kept (512 bytes), each blob bank's bytes, each of them from a
; 256-byte boundary (so none crosses a bank's end)
SN_INDEX        = $A000 + 48
SN_IDX_SIZE     = 10 + 32 + 32
SN_DATA         = $A100

.zeropage
lp:         .res        2                                   ; read_line's place in text

.bss
ibuf:       .res        IBUF_SIZE                           ; stdin's bytes read, how many, and the next
ilen:       .res        1
ipos:       .res        1
intr:       .res        1                                   ; Ctrl-C ($80), a note for on-note's function ($40): the
nt_f:       .res        2                                   ;   note handler's; on-note's function (NIL: none) ...
nt_pend:    .res        4                                   ;   the notes for it (a bit each, 0-31) ...
nt_n:       .res        1                                   ;   and the one it's given
hold_n:     .res        1                                   ; (hold's PREEMPT_OFFs not yet undone)
kd_ctl:     .res        1                                   ; (key's: the console's consctl, raw, and a read of it that
kd_nb:      .res        1                                   ;   doesn't wait, fds + 1; 0: not open)
rl_any:     .res        1                                   ; (read_line's: some of a line read)
sh_at:      .res        1                                   ; (sh_rule's: the line's first character)
stress:     .res        1                                   ; (hylang -g)
login:      .res        1                                   ; (hylang -l)
hl_argp:    .res        2                                   ; (Its arguments after -g and -l: args, a script's path
                                                            ;   first)
lib_text:   .res        1                                   ; <> 0: no snapshot: its library loaded as text
sn_me:      .res        ME_SIZE                             ; (snap_find's: a module's entry; the one at)
sn_i:       .res        1
sn_home:    .res        1                                   ; (snap_restore's: this bank, the snapshot's bank at,
sn_rbank:   .res        1                                   ;   its id, its counts, pages and lengths (its index's),
sn_key:     .res        2                                   ;   where it's at, where it goes, how many bytes)
sn_idx:     .res        SN_IDX_SIZE
sn_src:     .res        2
sn_dst:     .res        2
sn_len:     .res        2
sn_k:       .res        1
sn_j:       .res        1
sn_mask:    .res        2

.import __PSTATE_RUN__, __PSTATE_SIZE__

.segment "DATA4"
; The note handler, in RAM (any bank may be at $A000 when a note comes; the system calls it as hylang is switched
; in, between any two instructions): with on-note's function, each note noted for it (nt_pend, intr's bit 6: the
; evaluator calls it, ev_note); else Ctrl-C (NOTE_INTERRUPT) noted in intr's bit 7, another the default
notes:
            ldx         nt_f
            bne         @pend
            ldx         nt_f + 1
            bne         @pend
            cmp         #NOTE_INTERRUPT
            bne         @default
            lda         #$80
            tsb         intr
            clc
            rts
@default:
            sec
            rts
@pend:
            tax                                             ; (Its bit)
            and         #7
            tay
            txa
            lsr
            lsr
            lsr
            tax
            lda         bit_of,y
            ora         nt_pend,x
            sta         nt_pend,x
            lda         #$40
            tsb         intr
            clc
            rts

.segment "DATA"
sn_lowk:    .byte       1                                   ; (sn_step's addresses' low bytes 0-15, as assembled)

; The heap and the evaluator's state (PSTATE) as the snapshot whose first bank is .A has them, its banks taken anew
; (the heap's tables made, its value registers NIL).  In RAM (DATA: the fourth bank's room is DATA4's): it maps the
; snapshot's banks at $A000.  OUT: C = 0; C =
; 1, .A = 0 if it isn't a snapshot of this hylang (nothing changed), else .A = the error (E_NOMEM: no bank left)
snap_restore:
            sta         sn_rbank
            lda         snap_id                             ; (This hylang's id, from its first bank)
            sta         sn_key
            lda         snap_id + 1
            sta         sn_key + 1
            lda         $01
            sta         sn_home
            lda         sn_rbank
            sta         $01
            lda         SN_INDEX                            ; ("HYSN", this hylang's id, PSTATE's size)
            cmp         #'H'
            bne         @not
            lda         SN_INDEX + 1
            cmp         #'Y'
            bne         @not
            lda         SN_INDEX + 2
            cmp         #'S'
            bne         @not
            lda         SN_INDEX + 3
            cmp         #'N'
            bne         @not
            lda         SN_INDEX + 4
            cmp         sn_key
            bne         @not
            lda         SN_INDEX + 5
            cmp         sn_key + 1
            bne         @not
            lda         SN_INDEX + 8
            cmp         #<__PSTATE_SIZE__
            bne         @not
            lda         SN_INDEX + 9
            cmp         #>__PSTATE_SIZE__
            beq         :+
@not:
            lda         sn_home
            sta         $01
            lda         #0
            sec
            rts
:
            ldx         #SN_IDX_SIZE - 1                    ; (Its index, kept)
:
            lda         SN_INDEX,x
            sta         sn_idx,x
            dex
            bpl         :-
            jsr         heap_tables
            lda         #<SN_DATA                           ; (PSTATE)
            sta         sn_src
            lda         #>SN_DATA
            sta         sn_src + 1
            lda         #<__PSTATE_RUN__
            sta         sn_dst
            lda         #>__PSTATE_RUN__
            sta         sn_dst + 1
            lda         #<__PSTATE_SIZE__
            sta         sn_len
            lda         #>__PSTATE_SIZE__
            sta         sn_len + 1
            jsr         sn_copy
            stz         sn_k                                ; (Each cell bank: a bank of its own, the pages kept)
@cells:
            lda         sn_k
            cmp         sn_idx + 6
            bcs         @blobs
            lda         #1
            jsr         BANKS_ALLOC
            bcc         @lb521
            jmp         @full
@lb521:
            ldx         sn_k
            sta         cell_bank,x
            sta         $00
            pha
            txa
            asl
            asl
            asl
            asl
            tax
            pla
            ldy         #16
:
            sta         bank_of,x
            inx
            dey
            bne         :-
            lda         sn_k                                ; (Its mask: a bit a page, page 0's first)
            asl
            tax
            lda         sn_idx + 10,x
            sta         sn_mask
            lda         sn_idx + 11,x
            sta         sn_mask + 1
            stz         sn_j
@page:
            lsr         sn_mask + 1                         ; (Page sn_j kept? It and the kept ones after it, at
            ror         sn_mask                             ;   once: 512 bytes each, to $8000 + 512 * sn_j)
            bcc         @nopage
            stz         sn_dst
            lda         sn_j
            asl
            ora         #$80
            sta         sn_dst + 1
            stz         sn_len
            lda         #2
            sta         sn_len + 1
@run:
            inc         sn_j
            lsr         sn_mask + 1
            ror         sn_mask
            bcc         @copy
            inc         sn_len + 1
            inc         sn_len + 1
            bra         @run
@copy:
            jsr         sn_copy
@nopage:
            inc         sn_j
            lda         sn_j
            cmp         #16
            bcc         @page
            inc         sn_k
            bra         @cells
@blobs:
            stz         sn_k                                ; (Each blob bank: a bank of its own, its bytes)
@blob:
            lda         sn_k
            cmp         sn_idx + 7
            bcs         @done
            lda         #1
            jsr         BANKS_ALLOC
            bcs         @full
            ldx         sn_k
            sta         blob_bank,x
            sta         $00
            txa
            asl
            tax
            lda         sn_idx + 42,x
            sta         sn_len
            lda         sn_idx + 43,x
            sta         sn_len + 1
            stz         sn_dst
            lda         #$80
            sta         sn_dst + 1
            jsr         sn_copy
            inc         sn_k
            bra         @blob
@done:
            lda         sn_home
            sta         $01
            clc
            rts
@full:
            pha
            lda         sn_home
            sta         $01
            pla
            sec
            rts

; sn_len bytes from sn_src (its bank sn_rbank, at $A000: $01 set) to sn_dst (RAM, or the window: $00 set): 256 at a
; time, 16 loads and stores a step (their addresses set in the code: about 10 cycles a byte), then the rest; sn_src
; then on to the next 256-byte boundary (past $DFFF: the next bank's $A000; a snapshot's runs start at 256-byte
; boundaries, so none crosses a bank's end).  data4_init's too
sn_copy:
            lda         sn_len + 1
            bne         :+
            jmp         sn_rest
:
            lda         sn_src                              ; (Both from 256-byte boundaries, the addresses' low bytes
            ora         sn_dst                              ;   0-15 already: their high bytes set; else all set)
            bne         :+
            lda         sn_lowk
            beq         @lb532
            jmp         sn_high
@lb532:
:
            stz         sn_lowk
            jsr         sn_setsrc
            jsr         sn_setdst
            lda         sn_src
            ora         sn_dst
            bne         sn_run
            inc         sn_lowk
sn_run:
            ldy         #0
            clc                                             ; (C stays 0 till .Y is back to 0)
sn_step:
.repeat 16, I
            lda         $A000 + I,y
            sta         $8000 + I,y
.endrepeat
            tya
            adc         #16
            tay
            bne         sn_step
            inc         sn_dst + 1
            jsr         sn_next
            dec         sn_len + 1
            bne         :+
            jmp         sn_rest
:
            lda         sn_src                              ; (Both from 256-byte boundaries: the addresses' high
            ora         sn_dst                              ;   bytes set)
            bne         sn_on
sn_high:
            lda         sn_src + 1
.repeat 16, I
            sta         sn_step + 6 * I + 2
.endrepeat
            lda         sn_dst + 1
.repeat 16, I
            sta         sn_step + 6 * I + 5
.endrepeat
            jmp         sn_run
sn_on:
.repeat 16, I
            inc         sn_step + 6 * I + 5                 ; (The stores: 256 on)
.endrepeat
            lda         sn_src + 1                          ; (The loads: 256 on, or the next bank's)
            cmp         #$A0
            beq         :+
.repeat 16, I
            inc         sn_step + 6 * I + 2
.endrepeat
            jmp         sn_run
:
            jsr         sn_setsrc
            jmp         sn_run
sn_rest:
            ldy         sn_len                              ; (The rest: from its last down, one less each)
            beq         sn_end
            lda         sn_src
            sec
            sbc         #1
            sta         sn_r3 + 1
            lda         sn_src + 1
            sbc         #0
            sta         sn_r3 + 2
            lda         sn_dst
            sec
            sbc         #1
            sta         sn_w3 + 1
            lda         sn_dst + 1
            sbc         #0
            sta         sn_w3 + 2
sn_r3:      lda         $A000,y
sn_w3:      sta         $8000,y
            dey
            bne         sn_r3
            jsr         sn_next
sn_end:
            rts

; sn_step's loads' addresses: sn_src + 0 ... 15 (sn_setdst: its stores', sn_dst + 0 ... 15)
sn_setsrc:
            ldx         #0
            ldy         #0
:
            txa
            clc
            adc         sn_src
            sta         sn_step + 1,y
            lda         sn_src + 1
            adc         #0
            sta         sn_step + 2,y
            tya
            adc         #6
            tay
            inx
            cpx         #16
            bne         :-
            rts

sn_setdst:
            ldx         #0
            ldy         #0
:
            txa
            clc
            adc         sn_dst
            sta         sn_step + 4,y
            lda         sn_dst + 1
            adc         #0
            sta         sn_step + 5,y
            tya
            adc         #6
            tay
            inx
            cpx         #16
            bne         :-
            rts

; sn_src on 256 bytes (past $DFFF: the next bank's $A000)
sn_next:
            inc         sn_src + 1
            lda         sn_src + 1
            cmp         #$E0
            bcc         :+
            lda         #$A0
            sta         sn_src + 1
            inc         sn_rbank
            lda         sn_rbank
            sta         $01
:
            rts

.code

; The REPL: a line read (and more lines while it wants them), evaluated, its value printed (=> its repr), till exit
; or stdin's end.  Ctrl-C gives the line up
main:
            HYX2_BANKS_INIT
            FARN        4, data4_init                       ; (The most of the RAM code: from the fourth bank)
            stz         stress
            stz         login
            MOVW        hl_argp, r0
@option:
            lda         hl_argp                             ; (hylang -g: stress; -l: a login shell)
            ora         hl_argp + 1
            beq         @args
            MOVW        r0, hl_argp
            lda         (r0)
            cmp         #'-'
            bne         @args
            ldy         #2
            lda         (r0),y
            bne         @args
            dey
            lda         (r0),y
            ldx         #0
            cmp         #'g'
            beq         :+
            inx
            cmp         #'l'
            bne         @args
:
            inc         stress,x                            ; (stress, login)
            clc                                             ; (The rest: past it and its 0)
            lda         hl_argp
            adc         #3
            sta         hl_argp
            bcc         @option
            inc         hl_argp + 1
            bra         @option
@args:
            stz         olen
            stz         ilen
            stz         ipos
            stz         intr
            stz         nt_f
            stz         nt_f + 1
            jsr         cap_reset
            LDR         r0, notes
            jsr         NOTIFY
            stz         lib_text                            ; (The heap: a snapshot's, with its library; or made)
            lda         hyx2_bank6                          ; (The snapshot: where rom.txt puts it, the bank after
            inc         a                                   ;   hylang's last; else the module directory's hysnap)
            jsr         snap_restore
            bcc         @heap
            cmp         #0                                  ; (Not this hylang's: the heap made; no room: the end)
            beq         @lb531
            jmp         @noroom
@lb531:
            jsr         snap_find
            bcs         @made
            jsr         snap_restore
            bcc         @heap
            cmp         #0
            bne         @noroom
@made:
            jsr         pstate_zero
            jsr         heap_init
            bcs         @noroom
            inc         lib_text
@heap:
            lda         #1                                  ; (The capture bank, the machine)
            jsr         BANKS_ALLOC
            bcs         @noroom
            sta         cap_bank
            lda         #<hl_roots                          ; (The reader's levels, the machine's stack: roots)
            sta         gc_hook
            lda         #>hl_roots
            sta         gc_hook + 1
            jsr         ev_init
            lda         lib_text                            ; (No snapshot: the symbols, the built-ins, the library)
            beq         @init
            jsr         ev_syms
            bcs         @noroom
            FARN        2, bi_bind
            bcs         @noroom
            FARN        5, y_std
            bcs         @noroom
            jsr         lib_load
            bcs         @noroom
@init:
            FARN        5, y_init                           ; (stdin, stdout, stderr; args; a script's (load path))
            bcc         :+
@noroom:
            LDAX        s_noheap
            jsr         out_text
            lda         #1
            jmp         quit
:
            stz         gc_fresh                            ; (No collection at the first prompt: what y_init made
            lda         stress                              ;   is kept)
            sta         gc_stress
            lda         hv                                  ; (A script: run, its status 0, or 1 after an error
            ora         hv + 1                              ;   (on stderr), or (exit n)'s)
            beq         @repl
            MOVW        ex, hv
            stz         ee
            stz         ee + 1
            jsr         ev_run
            jsr         is_err
            lda         #0
            bcc         :+
            FARN        5, y_errout
            lda         #1
:
            jmp         quit
@repl:
            LDAX        s_banner
            jsr         out_text
            lda         login                               ; (A login shell: login.hl's)
            beq         @expr
            jsr         sh_login
@expr:
            stz         t_len
            stz         t_len + 1
            stz         cl_len
@prompt:
            lda         cl_len
            bne         @more
            jsr         sh_prompt                           ; (shell-prompt's, or hylang's)
            bcc         @wait
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
            GC_CALL
@line:
            lda         #$80                                ; (Ctrl-C's noted: dropped; a note for on-note's kept)
            trb         intr
            jsr         raw_off
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
            lda         cl_len                              ; (An expression's first line: an rc command line, the
            bne         @hylang                             ;   shell's?)
            jsr         sh_rule
            bcc         @hylang
            stz         ee                                  ; ((shell-line "line"): its value dropped, but an error)
            stz         ee + 1
            jsr         ev_run
            jsr         is_err
            bcc         @shdone
            FARN        5, y_errout
@shdone:
            jmp         @next
@hylang:
            FARN        2, read_text
            bcs         @nomem
            cmp         #RD_MORE
            bne         @lb721
            jmp         @prompt
@lb721:
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

; The line in text, an expression's first: an rc command line, the shell's (phase 12)?  When shell-line is a
; function (shell.hl's: the shell's rule on), a line whose first character (past blanks) isn't (, { or [, nor one
; right against ( or {, is rc's.  OUT: C = 1, ex = (shell-line "the line"), its blanks and its LF trimmed; C = 0,
; hylang's (an empty line too; with no room, the reader's to say so)
sh_rule:
            ldy         #0                                  ; (Its first character, past blanks)
:
            lda         text,y
            cmp         #' '
            beq         @blank
            cmp         #TAB
            bne         @first
@blank:
            iny
            bne         :-
@hylang:
            clc
            rts
@first:
            cmp         #LF
            beq         @hylang
            cmp         #'('
            beq         @hylang
            cmp         #'{'
            beq         @hylang
            cmp         #'['
            beq         @hylang
            lda         text + 1,y
            cmp         #'('
            beq         @hylang
            cmp         #'{'
            beq         @hylang
            sty         sh_at
            LDHQ        s_shline, S_SHLINE_N                ; (shell-line: a function?)
            lda         #PK_SYMBOL
            jsr         intern
            bcs         @hylang
            lda         hv
            ldx         hv + 1
            jsr         root_push
            bcs         @hylang
            jsr         sh_fn
            jsr         kind_of
            cmp         #PK_FUNC
            bne         @pop
@trim:
            lda         lp                                  ; (The line's end: its LF and blanks trimmed)
            bne         :+
            dec         lp + 1
:
            dec         lp
            lda         (lp)
            cmp         #' ' + 1
            bcc         @trim
            lda         #<text                              ; (The line, a string)
            clc
            adc         sh_at
            sta         hq
            lda         #>text
            adc         #0
            sta         hq + 1
            sec
            lda         lp
            sbc         hq
            sta         hn
            lda         lp + 1
            sbc         hq + 1
            sta         hn + 1
            inc         hn
            bne         :+
            inc         hn + 1
:
            jsr         string_make
            bcs         @pop
            MOVW        et, hv                              ; ((shell-line "line"))
            stz         eu
            stz         eu + 1
            lda         #PK_SCONS
            jsr         make2
            bcs         @pop
            MOVW        eu, hv
            jsr         sh_fn
            sta         et
            stx         et + 1
            lda         #PK_SCONS
            jsr         make2
            bcs         @pop
            jsr         root_pop
            MOVW        ex, hv
            sec
            rts
@pop:
            jsr         root_pop
            clc
            rts

; The prompt shell-prompt gives (shell.hl's: a string, or a function that gives one), out.  OUT: C = 1 if it's none
sh_prompt:
            lda         #$80                                ; (A Ctrl-C noted: dropped first, as at a line)
            trb         intr
            LDHQ        s_shprompt, S_SHPROMPT_N
            lda         #PK_SYMBOL
            jsr         intern
            bcs         @none
            lda         hv
            ldx         hv + 1
            jsr         root_push
            bcs         @none
            jsr         sh_fn
            sta         ex
            stx         ex + 1
            jsr         kind_of
            cmp         #PK_FUNC
            bne         @value
            MOVW        et, ex                              ; ((shell-prompt): its value)
            stz         eu
            stz         eu + 1
            lda         #PK_SCONS
            jsr         make2
            bcs         @pop
            MOVW        ex, hv
            stz         ee
            stz         ee + 1
            jsr         ev_run
@value:
            lda         ex
            ldx         ex + 1
            jsr         kind_of
            cmp         #PK_STRING
            bne         @pop
            jsr         root_pop
            lda         ex
            ldx         ex + 1
            ldy         #1
            FARN        2, print_val
            stz         ex
            stz         ex + 1
            clc
            rts
@pop:
            jsr         root_pop
@none:
            stz         ex
            stz         ex + 1
            sec
            rts

; A login shell's start: (load "#fx/lib/hylang/login.hl"), an error out on stderr
sh_login:
            LDHQ        s_login, S_LOGIN_N
            jsr         string_make
            bcs         @rts
            MOVW        et, hv
            stz         eu
            stz         eu + 1
            lda         #PK_SCONS
            jsr         make2
            bcs         @rts
            MOVW        eu, hv
            lda         #<(BUILTIN0 + 2 * BIN_LOAD)
            sta         et
            lda         #>(BUILTIN0 + 2 * BIN_LOAD)
            sta         et + 1
            lda         #PK_SCONS
            jsr         make2
            bcs         @rts
            MOVW        ex, hv
            stz         ee
            stz         ee + 1
            jsr         ev_run
            jsr         is_err
            bcc         @rts
            FARN        5, y_errout
@rts:
            stz         ex
            stz         ex + 1
            rts

; .A, .X = shell-line's global value (the symbol root_push kept last)
sh_fn:
            ldy         rsp
            dey
            lda         rs_lo,y
            ldx         rs_hi,y
            jsr         deref
            ldy         #3
            lda         (hp),y
            tax
            dey
            lda         (hp),y
            rts

; The library: (load "globals") (/lib/hylang/globals.hl: danlang's), its error out on stderr; then a collection, so
; what's kept is compact (a snapshot's, tools/hysnap.js: it takes hylang's heap at its first prompt).  OUT: C = 1 if
; there's no room
lib_load:
            LDHQ        s_globals, s_globals_n
            jsr         string_make
            bcs         @rts
            MOVW        et, hv                              ; ((load "globals"))
            stz         eu
            stz         eu + 1
            lda         #PK_SCONS
            jsr         make2
            bcs         @rts
            MOVW        eu, hv
            lda         #<(BUILTIN0 + 2 * BIN_LOAD)
            sta         et
            lda         #>(BUILTIN0 + 2 * BIN_LOAD)
            sta         et + 1
            lda         #PK_SCONS
            jsr         make2
            bcs         @rts
            MOVW        ex, hv
            stz         ee
            stz         ee + 1
            jsr         ev_run
            jsr         is_err
            bcc         :+
            FARN        5, y_errout
:
            stz         ex
            stz         ex + 1
            stz         hv
            stz         hv + 1
            GC_CALL
            jmp         lib_done
@rts:
            rts

; (lib_load's end, the library loaded: where tools/hysnap.js takes the snapshot)
lib_done:
            clc
            rts

; PSTATE zeroed (the system zeroes the BSS, not it: what a snapshot replaces), for a heap made anew
pstate_zero:
            lda         #<__PSTATE_RUN__
            sta         hp
            lda         #>__PSTATE_RUN__
            sta         hp + 1
            lda         #0
            tay
            ldx         #>__PSTATE_SIZE__                   ; (Its whole pages ...
            beq         @part
@page:
            sta         (hp),y
            iny
            bne         @page
            inc         hp + 1
            dex
            bne         @page
@part:
            ldy         #<__PSTATE_SIZE__                   ;   and the rest)
            beq         @done
:
            dey
            sta         (hp),y
            bne         :-
@done:
            rts

; The snapshot: the module hysnap's first bank (.A).  OUT: C = 1 if there's none
snap_find:
            stz         sn_i
@entry:
            LDR         r0, sn_me
            lda         sn_i
            jsr         MODINFO
            bcs         @rts
            lda         sn_me + ME_TYPE
            cmp         #HT_LIBRARY
            bne         @next
            ldx         #0
:
            lda         sn_me + ME_NAME,x
            cmp         s_hysnap,x
            bne         @next
            inx
            cmp         #0
            bne         :-
            lda         sn_me + ME_BANK
            clc
            rts
@next:
            inc         sn_i
            bne         @entry
            sec
@rts:
            rts

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
            bne         @end
            bit         intr                                ; (Not Ctrl-C's: a note for on-note's, read again)
            bpl         getc_in
            bra         @intr
@end:
            lda         #0
@intr:
            sec
            rts

; key's raw mode off: the console's consctl closed (rawoff, as it's the last), and key?'s read of it
raw_off:
            lda         kd_ctl
            beq         :+
            dec         a
            jsr         CLOSE
            stz         kd_ctl
:
            lda         kd_nb
            beq         :+
            dec         a
            jsr         CLOSE
            stz         kd_nb
:
            rts

.rodata
s_banner:   .byte       "hylang (danlang on the Hydra-16)", LF
            .byte       "Type 'exit' to Exit", LF, LF, 0
s_prompt:   .byte       "hylang> ", 0
s_shline:   .byte       "shell-line"
S_SHLINE_N  = * - s_shline
s_shprompt: .byte       "shell-prompt"
S_SHPROMPT_N = * - s_shprompt
s_login:    .byte       "#fx/lib/hylang/login.hl"
S_LOGIN_N   = * - s_login
s_more:     .byte       " <", 0
s_is:       .byte       "=> ", 0
s_bye:      .byte       "=> exit", LF, 0
s_missingl: .byte       "=> Error: missing ", 0
s_long:     .byte       "=> Error: Too long: an expression of more than 4096 bytes", LF, 0
s_nomemline: .byte      "=> Error: out of memory", LF, 0
s_noheap:   .byte       "hylang: no room for its heap", LF, 0
s_globals:  .byte       "hylib"                             ; (globals.hl, then hylang's own)
s_globals_n = * - s_globals
s_hysnap:   .byte       "hysnap", 0
snap_id:    .word       0                                   ; (This hylang's id: tools/hysnap.js patches it in, a CRC)

.segment "CODE3"                                            ; (The numbers: phase 5)
bank_three:
            rts

.segment "CODE5"                                            ; (Streams, I/O and the system library: phase 7)
bank_five:
            rts

.segment "CODE6"                                            ; (The collector, and the system calls: phase 10)
bank_six:
            rts

.segment "CODE4"                                            ; (Strings and hashes: phase 6; the RAM code's image)
; DATA4 (hylang.cfg's: the most of the RAM code, kept in this bank) copied to the task's RAM, as hylang starts
.import __DATA4_LOAD__, __DATA4_RUN__, __DATA4_SIZE__
data4_init:
            lda         #<__DATA4_LOAD__                    ; (By sn_copy: in DATA, which the system has copied)
            sta         sn_src
            lda         #>__DATA4_LOAD__
            sta         sn_src + 1
            lda         #<__DATA4_RUN__
            sta         sn_dst
            lda         #>__DATA4_RUN__
            sta         sn_dst + 1
            lda         #<__DATA4_SIZE__
            sta         sn_len
            lda         #>__DATA4_SIZE__
            sta         sn_len + 1
            lda         $01
            sta         sn_rbank
            jmp         sn_copy
