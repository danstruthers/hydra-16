; ****************************************************************************
; cons.inc - what the console driver's two objects share: cons.s (its first bank: the serial line, the keys, the
; line editor, the files, /pc, srvlib) and vt.s (its second: each window's screen, a VT100 in its banks, and the
; terminals drawn from it).  Each includes it: a name .global here is one object's, used by the other.  A routine
; in the other bank is called through FAR1 or FAR2 (hyx2.inc); data in RAM is either's.

.globalzp   hyx2_to, hyx2_a                                 ; The trampolines between the banks (hyx2.inc)
.global     hyx2_far1, hyx2_far2
