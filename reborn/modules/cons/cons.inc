; ****************************************************************************
; cons.inc - what the console driver's two objects share: cons.s (its first bank: the serial line, the keys, the
; line editor, the files, /pc, srvlib) and vt.s (its second: each window's screen, a VT100 in its banks, and the
; terminals drawn from it).  Each includes it: a name .global here is one object's, used by the other.  A routine
; in the other bank is called through FAR1 or FAR2 (hyx2.inc); data in RAM is either's.

WIN_MAX         = 4             ; Windows
INQ_SIZE        = 64            ; Each window's keys, waiting to be read
IOBUF           = 64            ; A write's bytes, a part at a time
TERM_SERIAL     = 1             ; term: the windows shown on the serial port ...
TERM_SCREEN     = 2             ;   and on the screen
VT_ROOM         = 48            ; The send ring's room a byte of a shown window's output needs (what one byte can
                                ;   make: a sequence passed on, or a cursor and colours set and a character)
BEL             = $07
BS              = $08
HT              = $09
VT              = $0B
FF              = $0C
SO              = $0E
SI              = $0F
CAN             = $18
SUB             = $1A
ESC             = $1B
DEL             = $7F

; ---- cons.s's, for vt.s
.globalzp   tx_head, tx_tail, w_in, ser_rd, cnt, lw
.global     tx_buf, iobuf, term, scr_st, scr_fd, bell, inq, w_iqh, w_iqt
.globalzp   hyx2_to, hyx2_a                                 ; The trampolines between the banks (hyx2.inc)
.global     hyx2_far1, hyx2_far2

; ---- vt.s's, for cons.s (FAR2)
.global     vt_init, vt_new, vt_free, vt_write, vt_put, vt_pump, vt_text
.global     ts_ser, ts_scr                                  ; Each terminal's state: 0 following the shown window,
                                                            ;   1 to be painted (all of it), 2 being painted
