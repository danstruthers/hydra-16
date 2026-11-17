; ****************************************************************************
; cmdmean_c.s - the command's meaning's calls (games/dq0c/cmdmean.s) for C, for the tests' harnesses (cmdmean.c):
; cc65's calling convention to the meaning's (its table's pointer in cm_w).
; ****************************************************************************

            .setcpu     "65C02"
            .export     _cmd_winit, _cmd_mean, _cmd_roles, _cmd_wthings
            .import     cm_winit, cm_mean, cm_w, cm_cmd, cm_roles, cm_wthings

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

; void __fastcall__ cmd_roles (unsigned char ok): the last line matched's roles (ok: cmd_match's verdict)
.proc _cmd_roles
            jmp         cm_roles
.endproc

; void cmd_wthings (void): the game's things' words marked the world's (after cmd_winit, and each cmd_world)
.proc _cmd_wthings
            jmp         cm_wthings
.endproc

; const unsigned char *__fastcall__ cmd_mean (unsigned char face): the last line matched's struct cmd
.proc _cmd_mean
            jsr         cm_mean
            lda         #<cm_cmd
            ldx         #>cm_cmd
            rts
.endproc
