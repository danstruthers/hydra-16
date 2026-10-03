; ****************************************************************************
; macros.inc - small helpers for programs in assembly (with hydra.inc).

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

; PRINT label: the string at label to stdout (PUTS).  Modifies .A, .Y, r0
.macro PRINT    label
            LDR         r0, label
            jsr         PUTS
.endmacro

CR              = $0D
LF              = $0A
TAB             = $09
