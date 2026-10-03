; ****************************************************************************
; macros.inc - small helpers for programs in assembly (with hydra.inc).  They define no labels (an unnamed one would
; shift the :+ and :- of the code around them), so they work from * where they need an address.

; LDR reg, value: a call register (r0-r15) = a 16-bit value, such as an address.  Modifies .A
.macro LDR      reg, value
            lda         #<(value)
            sta         reg
            lda         #>(value)
            sta         reg + 1
.endmacro

; MOVR to, from: a call register (or any zero-page word) = another.  Modifies .A
.macro MOVR     to, from
            lda         from
            sta         to
            lda         from + 1
            sta         to + 1
.endmacro

; PRINT label, or PRINT "text": the string at label, or the text, to stdout (PUTS).  Modifies .A, .Y, r0
.macro PRINT    arg
.if .match({arg}, "")
            lda         #<(* + 14)                          ; (The text: after these 8 bytes, the jsr and the jmp)
            sta         r0
            lda         #>(* + 10)
            sta         r0 + 1
            jsr         PUTS
            jmp         * + 3 + .strlen(arg) + 1
            .byte       arg, 0
.else
            LDR         r0, arg
            jsr         PUTS
.endif
.endmacro

; CALL name: a system call (a jsr to its slot in the jump table, as jsr name is)
.macro CALL     name
            jsr         name
.endmacro

; CHECK label: after a call, on to label if it failed (C = 1: .A the error), however far it is
.macro CHECK    label
            bcc         * + 5
            jmp         label
.endmacro

CR              = $0D
LF              = $0A
TAB             = $09
