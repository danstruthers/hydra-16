; ****************************************************************************
; bigzp.s - a test child (bankprog.h; t_bank's) whose zero page is more than its caller gives its children (40
; bytes; t_bank gives 16): refused as it's loaded (BE_ZP)
; ****************************************************************************

            .setcpu     "65C02"
            .export     _run

            .zeropage
big:        .res        40

            .code
_run:       sta         big
            ldx         #0
            rts
