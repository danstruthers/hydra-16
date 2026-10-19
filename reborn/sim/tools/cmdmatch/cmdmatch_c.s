; ****************************************************************************
; cmdmatch_c.s - the command matcher's calls (games/dq0c/cmdmatch.s) for C (cmdmatch.h): cc65's calling convention
; (the last argument in .A/.X, the others on the C stack) to the matcher's (its tables' and lists' pointers in cm_g,
; cm_i and cm_a).  For the tests' harnesses (sim/tools/cmdmatch); the game's parser (dq0c) is assembly throughout.
; ****************************************************************************

            .setcpu     "65C02"
            .export     _cmd_init, _cmd_world, _cmd_match
            .import     cm_init, cm_world, cm_match, cm_g, cm_i, cm_a
            .import     popax

            .code

; Three pointers, last first: .A/.X into cm_a, then the C stack's into cm_i and cm_g
.proc args3
            sta         cm_a
            stx         cm_a+1
            jsr         popax
            sta         cm_i
            stx         cm_i+1
            jsr         popax
            sta         cm_g
            stx         cm_g+1
            rts
.endproc

; int __fastcall__ cmd_init (const unsigned char *grammar, const unsigned char *index, const unsigned char *aliases)
.proc _cmd_init
            jsr         args3
            jsr         cm_init
            lda         #0
            tax
            bcs         :+
            lda         #1
:           rts
.endproc

; void __fastcall__ cmd_world (const char *world, const char *names, const char *deities)
.proc _cmd_world
            jsr         args3
            jmp         cm_world
.endproc

; int __fastcall__ cmd_match (const char *line): its .A/.X are cm_match's, and so is what it gives back
_cmd_match  = cm_match
