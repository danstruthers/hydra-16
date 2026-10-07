; ****************************************************************************
; cons.inc - what the console driver's two objects share: cons.s (its first bank: the serial line, the keys, the
; line editor, the files, /pc, srvlib) and vt.s (its second: each window's screen, a VT100 in its banks, and the
; terminals drawn from it).  Each includes it: a name .global here is one object's, used by the other.  A routine
; in the other bank is called through FAR1 or FAR2 (hyx2.inc); data in RAM is either's.

WIN_MAX         = 4             ; Windows
INQ_SIZE        = 64            ; Each window's keys, waiting to be read
IOBUF           = 64            ; A write's bytes, a part at a time
ANS_SIZE        = 32            ; Each window's answers (DA, DSR ...: vt.s's, for a raw reader), at most
LBL_SIZE        = 32            ; Each window's label (OSC 0 and 2's title), with its zero
TERM_SERIAL     = 1             ; term: the windows shown on the serial port ...
TERM_SCREEN     = 2             ;   and on the screen
WIN_COLS        = 127           ; A window's size at most: its columns (a row's last cell is its meta) ...
WIN_ROWS        = 64            ;   its rows (a plane's) ...
WIN_MIN_COLS    = 10            ;   and at least (a terminal's size smaller than this isn't taken)
WIN_MIN_ROWS    = 2
COL_DEF         = $07           ; The colours at first: light grey on black (a cell's: the background << 4 | the
                                ;   foreground, conio's 0-15)
F_BOLD          = $01           ; The rendition (a cell's): bold ...
F_DIM           = $02           ;   faint ...
F_UL            = $04           ;   underlined ...
F_BLINK         = $08           ;   blinking ...
F_REV           = $10           ;   reversed ...
F_INVIS         = $20           ;   invisible ...
F_PROT          = $40           ;   protected (DECSCA: W2)
CH_BAR          = $01           ; A window's chrome on a terminal (w_chr: the serial port's in bits 0-2, the screen's
CH_HEAD         = $02           ;   in bits 4-6): the bar, its header, its footer
CH_FOOT         = $04
CH_ALL          = $07
CH_LIVE         = $70           ; (The bits acted on: the screen's; the serial port's come with W4b)
BAR_TOP         = 1             ; bar_pos: the bar at the terminals' top, or their bottom (0: none)
BAR_BOTTOM      = 2
CR_BAR          = 0             ; A chrome row (chr_render's): the bar, a header, a footer
CR_HEAD         = 1
CR_FOOT         = 2
CR_MAX          = 128           ; A chrome row's cells, at most (as wide as its terminal, CR_MAX - 1 at most)
STAT_SIZE       = 128           ; A window's status line, its characters with their zero
ACT_BELL        = $01           ; w_act: a window not shown rang the bell (! in the chrome) ...
ACT_OUT         = $02           ;   or was written to, with monitor on (+)
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
VM_CKM          = $40           ; A window's modes (vt.s's v_mode): the cursor keys' application mode (DECCKM) ...
VM_KPAM         = $80           ;   the keypad's (DECKPAM) ...
VM2_VT52        = $02           ;   and (v_mode2) VT52 mode (DECANM reset)

; ---- cons.s's, for vt.s
.globalzp   tx_head, tx_tail, w_in, ser_rd, cnt, lw
.global     tx_buf, iobuf, term, scr_st, scr_fd, bell, inq, w_iqh, w_iqt, w_jump
.global     lay_cols, lay_rows, scr_chk                     ; A window's size (vt_new's, vt_resize's); the screen's
                                                            ;   to be looked at
.global     ser_cols, ser_rows, scr_cols, scr_rows          ; The terminals' sizes
.global     w_chr, bar_pos, chr_dirty, w_act, w_mon, w_stat ; The chrome (W4): each window's on each terminal, the
                                                            ;   bar's place, the terminals' to be drawn again (bit 0
                                                            ;   the serial port, 1 the screen), activity, the status
                                                            ;   lines
.global     cr_c, cr_a, cr_f, cr_n, chr_pass, chr_render    ; A chrome row rendered (FAR1 chr_render)
.globalzp   hyx2_to, hyx2_a                                 ; The trampolines between the banks (hyx2.inc)
.global     hyx2_far1, hyx2_far2

; ---- vt.s's, for cons.s (FAR2)
.global     vt_init, vt_keymodes, vt_new, vt_free, vt_write, vt_put, vt_pump, vt_text, vt_resize, vt_size, vt_cursor
.global     v_cols, v_rows, v_leds                          ; (The loaded window's: chr_render's %c, %r, %L)
.global     ans_n, ans_r, ans_buf, lbl_buf                 ; Each window's answers, its label
.global     ts_ser, ts_scr                                  ; Each terminal's state: 0 following the shown window,
                                                            ;   1 to be painted (all of it), 2 being painted
