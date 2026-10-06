; ****************************************************************************
; regs.inc - hylang's zero page registers that more than one part uses (the evaluator's, eval.inc's: the machine's
; values, which the collector marks, and a call's arguments), declared before the code that uses them

.zeropage
ex:         .res        2                                   ; The expression, or its value
ee:         .res        2                                   ; Its scope: a frame (NIL: the global scope)
cf:         .res        2                                   ; A call's function ...
cr:         .res        2                                   ;   its arguments not evaluated yet (a special form's:
et:         .res        2                                   ;   all, as written); values kept while more is made
eu:         .res        2
cn:         .res        2                                   ; A call's arguments evaluated so far (on the stack) ...
argp:       .res        2                                   ;   and, as a built-in's called, the first's address

q_pa:       .res        2                                   ; (eqcmp.inc's q_strcmp: the strings' bytes)
q_pb:       .res        2
na:         .res        2                                   ; (The numbers': registers' bytes, numcore.inc's)
nb:         .res        2
nc:         .res        2
nd:         .res        2
