; ****************************************************************************
; cmdmean_c.s - the command's meaning's calls (games/dq0c/cmdmean.s) for C, for the tests' harnesses (cmdmean.c):
; cc65's calling convention to the meaning's (its table's pointer in cm_w).
; ****************************************************************************

            .setcpu     "65C02"
            .export     _cmd_winit, _cmd_mean
            .import     cm_winit, cm_mean, cm_w, cm_cmd

            .code

; int __fastcall__ cmd_winit (const unsigned char *words): 1, or 0 if it's not a words' table, or not the index's
.proc _cmd_winit
            sta         cm_w
            stx         cm_w+1
            jsr         cm_winit
            lda         #0
            tax
            bcs         :+
            lda         #1
:           rts
.endproc

; const unsigned char *__fastcall__ cmd_mean (unsigned char face): the last line matched's struct cmd
.proc _cmd_mean
            jsr         cm_mean
            lda         #<cm_cmd
            ldx         #>cm_cmd
            rts
.endproc
