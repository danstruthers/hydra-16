; ****************************************************************************
; init - the first program (task 1), for phase 1: it says it's up, lists the tasks, runs hello and waits for it,
; then echoes what's typed (Return: a new line; "?": the tasks again).  The shell takes its place in phase 3.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"

            HYX2_PROGRAM "init", main

.zeropage
child:      .res        1

.bss
msg:        .res        32                                  ; An exit message, an error's text

.code
main:
            PRINT       s_up
            jsr         GETPID
            jsr         PUTHEX
            PRINT       s_crlf
            jsr         DBG_PS
            LDR         r0, s_hello                         ; hello, with arguments, and its end
            LDR         r1, s_args
            lda         #0
            jsr         SPAWN
            bcs         @failed
            sta         child
            LDR         r0, msg
            lda         child
            jsr         WAIT
            bcs         @failed
            phx
            PRINT       s_ended                             ; hello ended: code $07 ("bye")
            pla
            jsr         PUTHEX
            PRINT       s_open
            PRINT       msg
            PRINT       s_close
            bra         @echo

@failed:
            pha
            PRINT       s_error
            pla
            LDR         r0, msg
            jsr         ERRSTR
            PRINT       msg
            PRINT       s_crlf
@echo:                                                      ; What's typed, back
            jsr         GETC
            cmp         #CR
            beq         @eol
            cmp         #'?'
            beq         @ps
            jsr         PUTC
            bra         @echo

@eol:
            PRINT       s_crlf
            bra         @echo

@ps:
            PRINT       s_crlf
            jsr         DBG_PS
            bra         @echo

.rodata
s_up:       .byte       "init: up in task ", 0
s_hello:    .byte       "#m/hello", 0
s_args:     .byte       "from init", 0
s_ended:    .byte       "init: hello ended: code $", 0
s_open:     .byte       " (", 0
s_close:    .byte       ")"
s_crlf:     .byte       CR, LF, 0
s_error:    .byte       "init: ", 0
