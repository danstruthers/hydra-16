; ****************************************************************************
; zp.inc - BASIC's zero page ($22-$7F: the program's), first, so every bank addresses it as zero page

.zeropage
ca:         .res        2                                   ; The far memory's cursor A: an address in the window ...
cal:        .res        1                                   ;   and its logical bank (ram.inc)
cb:         .res        2                                   ; Cursor B
cbl:        .res        1
cn:         .res        2                                   ; far_copy's count
ip:         .res        2                                   ; The interpreter's next op, in the window (run.inc) ...
ipl:        .res        1                                   ;   and its logical bank
vsp:        .res        2                                   ; The value stack's top: the next value's place
frp:        .res        2                                   ; The frame: its first slot
vt:         .res        2                                   ; A value's place (a slot's, the stack's)
vu:         .res        2
t0:         .res        4                                   ; (Scratch)
t4:         .res        4
lp:         .res        2                                   ; The line, where it's read (lex.inc)
tsp:        .res        2                                   ; A string token's bytes (in lbuf)
tstart:     .res        2                                   ; The token's start
sp0:        .res        2                                   ; (The symbols': comp.inc)
np:         .res        2                                   ; (A number's bytes: num.inc)
pe:         .res        2                                   ; (CALL's: the procedure's entry)
argp:       .res        2                                   ; The arguments after the options (a script's path first)
