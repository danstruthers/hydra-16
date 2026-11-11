; dice.s - (dq0null) the dice by who asks (the referee skeleton's, ../dq0/referee.h's dice_to): ../dq0/dice.inc;
; and the window's bank read and set (the skeleton's put: getbank, setbank, as dq0harn's bank.s has them)

            .include    "../dq0/dice.inc"

            .export     _getbank, _setbank
BANKREG     = $00                   ; The task's RAM bank register (the bank at $8000)

            .code
_setbank:   sta         BANKREG
            rts

_getbank:   lda         BANKREG
            ldx         #0
            rts
