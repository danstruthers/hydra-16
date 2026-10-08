; ****************************************************************************
; main.inc - BASIC's start and its top (the first bank): its options and arguments, its memory, the libraries; a
; script (basic file args: loaded, compiled, run, its status) or the prompt (a line read, compiled and run at once);
; and every error's way in (err_entry: from any bank, however deep), said and back to the top.

IBUF_SIZE       = 128           ; stdin read this much at a time
STK_TOP         = $7E00         ; The value stack's end (its last frame's room)
STACK_SPARE     = 320           ; (Room kept past the last frame: an expression's values)

.bss
run_s:      .res        1                                   ; The 6502's stack at run_code (END's way back)
top_s:      .res        1                                   ;   and at the top (an error's)
in_comp:    .res        1                                   ; <> 0: compiling
script:     .res        1                                   ; <> 0: a script's run
interactive: .res       1                                   ; <> 0: stdin is the console
login:      .res        1                                   ; <> 0: basic -l
stk_base:   .res        2                                   ; The value stack's first
stk_lim:    .res        2                                   ;   and its last frame's
cont_ip:    .res        2                                   ; STOP's (and an error's) place, for CONT, its value stack
cont_vsp:   .res        2                                   ;   and frame; <> 0 CONT can go on (cont_ok)
cont_frp:   .res        2
cont_ok:    .res        1
ibuf:       .res        IBUF_SIZE                           ; stdin's bytes read
ilen:       .res        1
ipos:       .res        1
got_intr:   .res        1                                   ; <> 0: stdin's read ended by Ctrl-C
sr_buf:     .res        SR_SIZE                             ; (FSTAT's)

.code

main:
            HYX2_BANKS_INIT
            lda         #1
            sta         ofd
            stz         olen
            stz         intr
            stz         ilen
            stz         ipos
            lda         #80
            sta         owidth
            MOVW        argp, r0
            LDR         r0, notes
            jsr         NOTIFY
            lda         argp                                ; (-l: a login shell)
            ora         argp + 1
            beq         @opts
            lda         (argp)
            cmp         #'-'
            bne         @opts
            ldy         #2
            lda         (argp),y
            bne         @opts
            dey
            lda         (argp),y
            cmp         #'l'
            bne         @opts
            inc         login
            lda         argp
            clc
            adc         #3
            sta         argp
            bcc         @opts
            inc         argp + 1
@opts:
            FARN        4, num_init                         ; The libraries
            bcc         :+
            LDR         r0, s_nolib
            lda         #1
            jmp         EXITS
:
            lda         #<(__BSS_RUN__ + __BSS_SIZE__)      ; The globals after the BSS, then the value stack
            sta         gbase
            lda         #>(__BSS_RUN__ + __BSS_SIZE__)
            sta         gbase + 1
            clc
            lda         gbase
            adc         #<GLOB_MAX
            sta         stk_base
            lda         gbase + 1
            adc         #>GLOB_MAX
            sta         stk_base + 1
            LDW         stk_lim, STK_TOP - STACK_SPARE
            lda         #'d'                                ; (Numbers typed: read in decimal at the start)
            sta         cbase
            stz         cbase + 1
            FARN        5, heap_init
            FARN        2, prog_init
            FARN        7, text_init
            LDR         r0, sr_buf                          ; (stdin the console?)
            lda         #0
            jsr         FSTAT
            bcs         :+
            lda         sr_buf + SR_DEV
            cmp         #'c'
            bne         :+
            inc         interactive
:
            lda         login                               ; (basic -l: the login, the profile opened)
            beq         :+
            FARN        8, sh_login
:
            lda         argp                                ; A script: basic file args
            ora         argp + 1
            beq         prompt
            lda         (argp)
            beq         prompt
            inc         script
            tsx
            stx         top_s
            FARN        7, script_load
            jsr         prog_compile
            jsr         prog_run
            jsr         out_flush
            FARN        6, con_cooked
            LDR         r0, 0
            lda         end_status
            jmp         EXITS
s_nolib:    .byte       "no numbers library", 0

; ---- The prompt: a line read, run at once

prompt:
            tsx
            stx         top_s
@line:
            jsr         code_sel
            lda         sh_src                              ; (The profile's lines first)
            beq         @console
            FARN        8, sh_getline
            bcc         @have
@console:
            lda         interactive
            beq         @read
            lda         login                               ; (The shell's: its directory first)
            beq         :+
            FARN        8, sh_prompt
            bra         @read
:
            lda         #'>'
            jsr         out_byte
            lda         #' '
            jsr         out_byte
@read:
            jsr         out_flush
            FARN        6, con_cooked                       ; (The console's lines cooked: a line read)
            jsr         read_line
            bcc         @have
            jsr         out_flush
            LDR         r0, 0
            lda         #0
            jmp         EXITS
@have:
            FARN        8, sh_line                          ; (rc's, or the shell's own: run already)
            bcc         @lb903
            jmp         @line
@lb903:
            LDW         lp, lbuf                            ; (An empty line: nothing)
            ldy         #0
:
            lda         lbuf,y
            bne         @lb902
            jmp         @line
@lb902:
            cmp         #' '
            bne         :+
            iny
            bra         :-
:
            jsr         is_digit_main
            bcs         @now
            FARN        7, text_line                        ; (A line with a number first: into the program)
            jmp         @line
@now:
            FARN        7, prompt_cmd                       ; (RUN, LIST ...: C = 0 done)
            bcs         @lb901
            jmp         @line
@lb901:
            lda         #1
            sta         in_comp
            FARN        2, compile_line
            stz         in_comp
            stz         in_prog                             ; (The prompt's line: its errors not the program's)
            jsr         prompt_stack
            lda         #<(LB_IMM << 13)
            ldx         #>(LB_IMM << 13)
            jsr         run_code
            bcs         @lb900
            jmp         @line
@lb900:
            jmp         @line

is_digit_main:
            cmp         #'0'
            bcc         @no
            cmp         #'9' + 1
            bcs         @no
            clc
            rts
@no:
            sec
            rts

; The prompt's line's value stack: over a stopped program's (CONT goes on with it), else empty
prompt_stack:
            lda         cont_ok
            beq         stack_reset
            MOVW        vsp, cont_vsp
            MOVW        frp, cont_frp
            rts

; The value stack emptied: no frame (the main program's)
stack_reset:
            MOVW        vsp, stk_base
            MOVW        frp, stk_base
            rts

; The program compiled (its text), as RUN and a script do
prog_compile:
            lda         #1
            sta         in_comp
            FARN        2, compile_prog
            stz         in_comp
            rts

; The program run from its start: its globals as they were made, the value stack empty.  OUT: C = 1 STOPped
prog_run:
            FARN        6, fl_closeall                      ; (A run before's files closed)
            FARN        4, base_dec                         ; (Its numbers in decimal, as its text's are at its start)
            stz         g_win                               ; (Its WINDOW none yet: its values made again)
            stz         end_status
            lda         #1                                  ; (The program's: ON ERROR none yet)
            sta         in_prog
            stz         eh_in
            lda         #$FF
            sta         eh_addr
            sta         eh_addr + 1
            stz         dptr                                ; (READ: from DATA's first item)
            stz         dptr + 1
            stz         cont_ok
            jsr         stack_reset
            lda         #0
            tax
run_go:
            jsr         run_code
            bcs         :+                                  ; (STOPped: as it was, for CONT)
            FARN        6, fl_closeall                      ; (Its end: its files closed)
            clc
:
            rts

; CONT: the stopped program on (C = 1: STOPped again).  Nothing stopped: Cannot continue
prog_cont:
            lda         cont_ok
            bne         :+
            lda         #ER_CONT
            ldx         #0
            jmp         err_entry
:
            stz         cont_ok
            MOVW        vsp, cont_vsp
            MOVW        frp, cont_frp
            lda         #1
            sta         in_prog
            lda         cont_ip
            ldx         cont_ip + 1
            bra         run_go

; ---- stdin: a line at a time

; A line from stdin into lbuf (zero-terminated, its LF and CR dropped).  OUT: C = 1 at the end
read_line:
            ldx         #0
@byte:
            phx
            jsr         getc_in
            plx
            bcs         @end
            cmp         #CR
            beq         @byte
            cmp         #LF
            beq         @done
            cpx         #<(LBUF_SIZE - 1)                   ; (Past a line's room: the rest dropped)
            bcs         @byte
            sta         lbuf,x
            inx
            bra         @byte
@end:
            cpx         #0                                  ; (The end after some of a line: the line)
            beq         :+
@done:
            stz         lbuf,x
            clc
            rts
:
            sec
            rts

; stdin's next byte.  OUT: C = 0, .A it; C = 1 its end (or Ctrl-C: E_INTR)
getc_in:
            ldx         ipos
            cpx         ilen
            bcc         @have
            jsr         out_flush
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
            lda         #$80                                ; (Ctrl-C at the prompt: the line dropped)
            trb         intr
            sta         got_intr                            ; (INPUT's: Break)
            sec
            rts
@end:
            sec
            rts

; ---- Errors: .A/.X the code, from b_error (any bank, this one at $A000 again)

err_entry:
            sta         err_code
            stx         err_code + 1
            jsr         code_sel
            stz         err_line
            stz         err_line + 1
            lda         in_comp                             ; Compiling: its line (the program's)
            beq         @run
            stz         in_comp
            lda         cmode
            bne         @say
            MOVW        err_line, cline
            bra         @say
@run:
            lda         running                             ; (Not running: the prompt's own, no line)
            beq         @say
            jsr         ip_get                              ; Running: its statement's line (the program's code)
            sta         t0
            stx         t0 + 1
            FARN        7, lt_find
            lda         in_prog                             ; (ON ERROR's handler takes it: the program's, not in the
            beq         @say                                ;   handler already, not Ctrl-C)
            lda         eh_in
            bne         @cont
            lda         eh_addr + 1
            cmp         #$FF
            beq         @cont
            lda         err_code + 1
            bne         :+
            lda         err_code
            cmp         #ER_BREAK
            beq         @cont
:
            jmp         err_trap
@cont:
            jsr         cont_set                            ; (Not taken: CONT's state)
@say:
err_said:
            stz         running
            FARN        7, err_say
            ; (fall through)

; Back to the top after an error: the prompt, or a script's end (status 1)
top_error:
            ldx         top_s
            txs
            jsr         out_flush
            FARN        6, con_cooked
            lda         script
            beq         :+
            LDR         r0, 0
            lda         #1
            jmp         EXITS
:
            jmp         prompt
