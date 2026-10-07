; ****************************************************************************
; vt - the console driver's second bank (its first: cons.s): each window's screen, kept as cells in the driver's
; RAM banks, a VT100 that writes them, and the terminals drawn from them (docs/design/plans/WINDOWS.md, W1).
;
; A window's screen: three of the task's banks (vw_bank, a run from BANKS_ALLOC), planes of 64 rows of 128 cells
; at $8000 (a row's cells at $8000 + 128 * the row): the characters (a byte each: the font's, ISO-8859-15; $00-$1F
; the DEC Special Graphics set's 32, its $5F-$7E), the colours (the background << 4 | the foreground: conio's
; 0-15) and the rendition (F_*).  The 64 rows are a pool and the screen a map into it (vw_maps, a page's quarter a
; window's screen, main and alternate: vmap the one in use; the screen's rows
; from v_sb0 on, the scrollback's before them, its last v_sbn the newest last).  So a scroll moves the map's
; entries, not the rows: a row that goes off the top of the whole screen (or of a region at its top) joins the
; scrollback, and the oldest scrollback row comes in, blanked, at the region's bottom.  A window's size is the
; layout's (cons.s: the smaller of the terminals it's shown on; 127 x 64 at most), the pool's other rows its
; scrollback: 40 at 80 x 24.  Its history (wctl's history N, W6c), more: sets of 64 rows, three banks each like the
; pool's (vw_hb), a ring the scrollback's oldest row is copied into as it goes round (hist_put).  A resize turns the ring (vt_resize), as xterm keeps the cursor's row: the scrollback's
; rows come down onto a taller screen, a shorter one's rows go off its top into it.
;   Its state (the parser's, the cursor, the margins, the rendition, the modes, the character sets, the saved
; cursor, the tab stops, the map) is vs_*; each window's is kept in vt_save, a page each, and loaded (vt_load) as
; it's written to or read.
;
; The parser is Paul Williams' DEC-compatible state machine (vt100.net): C0 controls act in the middle of a
; sequence, CAN and SUB end one, a sequence that isn't known is taken whole and dropped, and the strings (OSC, DCS,
; SOS, PM, APC) end at ST (OSC also at BEL).  Done here, W1: the C0 controls; ESC 7, 8, D, E, H, M, Z, c, = and >,
; # 8, the character sets (( ) * + with B, A, 0, 1, 2); CSI @ A B C D E F G H I J K L M P S T X Z ` a b c d e f g h
; l m n r s u, and ! p (DECSTR); the modes 4 (IRM) and 20 (LNM), ?1 (DECCKM), ?3 (DECCOLM: no 132 columns, the
; screen cleared), ?4 (DECSCLM, kept), ?5 (DECSCNM, kept), ?6 (DECOM), ?7 (DECAWM) and ?25 (DECTCEM); SGR 0-8,
; 21-28, 30-39, 40-49 (38 and 48's 256 colours and RGB as the nearest of the 16), 90-97, 100-107; DA, DECID and DSR
; 5 and 6, answered into the window's keys.  An LF is CR LF (the tty's onlcr), as the console's always was.
;
; The terminals (the serial port, and the screen: vid's #v/term) show the window shown (w_in).  Each follows it
; (ts_ser, ts_scr 0) or is to be painted (1; the serial port, being painted: 2).  Following, the window's output goes
; to it as it's parsed.  To the serial port, the bytes as they came (a sequence whole, at its end), so the PC's
; terminal shows what the cells have; but not the console's own: the reports asked for (the console answers),
; DECCKM and DECKPAM (the console decodes the keys).  To the screen, what makes vid's terminal show the cells,
; vid's cursor tracked (scr_x, scr_y): a character where it goes (a CUP first if vid's cursor isn't there), the
; colours as SGR's canonical form as they change, a scroll of a region as the window's scrolls, erasing as ED and
; EL; what vid can't do (inserting and deleting, scrolls by count, REP ...) has it painted instead.  Painting (a
; window shown, a terminal turned on, the line /ser's no more): the terminal cleared, each row's cells to its last
; that isn't blank (SGR as it changes; DEC graphics in ESC ( 0, on the screen as ASCII), then the margins, the modes,
; the cursor.  The serial port's as the send ring has room, at each request's end (the shown window's writers
; waiting meanwhile: cons.s), the screen's all at once.

.include "hydra.inc"
.include "hw.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "cons.inc"

POOL            = WIN_ROWS      ; A window's rows: a plane's (128 cells each)
HIST_MAX        = 128           ; A window's history's rows, at most (two sets of 64: its lines then fit a byte)
VT_NPAR         = 16            ; A sequence's numbers, at most
VT_RAW          = 40            ; A sequence's bytes kept, to pass it on as it came
VS_PAGE         = 256           ; A window's state in vt_save
SCR_BUF         = 240           ; The screen's bytes, a write to #v/term at a time
                                ; (The rendition, F_*, and COL_DEF: cons.inc's)
META            = 127           ; A row's last cell, its meta (a window has 127 columns at most): the characters'
                                ;   plane's its blank end's first column (the cells from there on are blank, not
                                ;   written: a scroll or an erase to the row's end is a byte, not a row's cells);
                                ;   the colours' that end's colours; the rendition's the row's attributes (RA_*)
RA_CONT         = $01           ; A row's attributes: an autowrap continued the row before it into it ...
RA_DW           = $02           ;   double width (DECDWL) ...
RA_DHT          = $04           ;   double height, the top half (DECDHL 3) ...
RA_DHB          = $08           ;   and the bottom half (DECDHL 4): each half the width
RA_LINE         = RA_DW | RA_DHT | RA_DHB
VM_AWM          = $01           ; v_mode: autowrap (DECAWM) ...
VM_OM           = $02           ;   origin (DECOM) ...
VM_IRM          = $04           ;   insert (IRM) ...
VM_LNM          = $08           ;   LF as a new line (LNM) ...
VM_TCEM         = $10           ;   the cursor shown (DECTCEM) ...
VM_SCNM         = $20           ;   the screen reversed (DECSCNM: W2) ...
                                ;   (VM_CKM, VM_KPAM: cons.inc's, the keys' modes)
VM2_SCLM        = $01           ; v_mode2: smooth scrolling (DECSCLM) (VM2_VT52: cons.inc's)
S_GROUND        = 0             ; The parser's states
S_ESC           = 1
S_ESCI          = 2             ;   ESC and an intermediate
S_CSI           = 3
S_CSII          = 4             ;   CSI's intermediate
S_CSIX          = 5             ;   CSI being ignored
S_OSC           = 6             ;   (the strings from here)
S_OSCE          = 7             ;   ESC in an OSC
S_STR           = 8             ;   DCS, SOS, PM, APC
S_STRE          = 9
S_Y1            = 10            ;   VT52's ESC Y: the row next ...
S_Y2            = 11            ;   the column
G_ERROR         = $02           ; SUB's character: the DEC checkerboard

.zeropage
vrp:        .res        2                                   ; The cursor's row (its cells in a plane)
vq:         .res        2                                   ; A row
vr:         .res        2                                   ;   and another (a row's cells moved)
vch:        .res        1                                   ; The byte being parsed
vt_a:       .res        2                                   ; Scratch
vmap:       .res        2                                   ; The loaded window's screen's map (vw_maps)
vk:         .res        2                                   ; (vt_key's: a window's answers)

.bss
vs_first:                                                   ; ---- The loaded window's state (VS_N bytes)
v_state:    .res        1                                   ; The parser: its state (S_*) ...
v_priv:     .res        1                                   ;   a CSI's private marker (< = > ?), or 0 ...
v_inter:    .res        1                                   ;   the intermediate (0 none, $FF two or more) ...
v_pseen:    .res        1                                   ;   <> 0: a number's begun ...
v_npar:     .res        1                                   ;   the number being read (VT_NPAR: past the last) ...
v_parl:     .res        VT_NPAR                             ;   the numbers (16 bits; 0 for none)
v_parh:     .res        VT_NPAR
v_rawn:     .res        1                                   ; The sequence's bytes so far ($FF: too many to keep) ...
v_raw:      .res        VT_RAW                              ;   and them
v_x:        .res        1                                   ; The cursor ...
v_y:        .res        1
v_wrap:     .res        1                                   ;   <> 0: past the last column (the next character
                                                            ;   wraps: the VT100's last-column flag)
v_cols:     .res        1                                   ; The size
v_rows:     .res        1
v_sb0:      .res        1                                   ; The screen's first row in the map (POOL - rows) ...
v_sbn:      .res        1                                   ;   the scrollback's rows (before it)
v_top:      .res        1                                   ; The scrolling region: its first row, its last
v_bot:      .res        1
v_col:      .res        1                                   ; The colours and the rendition written
v_fl:       .res        1
v_mode:     .res        1                                   ; VM_*
v_mode2:    .res        1                                   ; VM2_*
v_g0:       .res        1                                   ; G0 and G1 (B, A, 0)
v_g1:       .res        1
v_gl:       .res        1                                   ;   which is in use (SO: 1, SI: 0)
v_last:     .res        1                                   ; The last character written (REP's)
v_leds:     .res        1                                   ; The VT100's four LEDs (DECLL: bits 0-3)
v_oscn:     .res        1                                   ; An OSC: its number ...
v_y52:      .res        1                                   ; VT52's ESC Y: its row
v_osci:     .res        1                                   ;   and its text's place in the label ($FF: its number
                                                            ;   being read; $FE: not a label's)
v_sx:       .res        1                                   ; The cursor saved (DECSC): its place ...
v_sy:       .res        1
v_scol:     .res        1                                   ;   rendition ...
v_sfl:      .res        1
v_sg0:      .res        1                                   ;   character sets ...
v_sg1:      .res        1
v_sgl:      .res        1
v_swrap:    .res        1                                   ;   last-column flag ...
v_som:      .res        1                                   ;   and origin mode
v_tabs:     .res        16                                  ; The tab stops (bit c & 7 of byte c >> 3)
v_alt:      .res        1                                   ; <> 0: the alternate screen in use (?47, ?1047, ?1049)
v_ssdt:     .res        1                                   ; The status line: its type (DECSSDT: 0 none, 1 the
                                                            ;   indicator, 2 the program's) ...
v_sasd:     .res        1                                   ;   <> 0: the output goes there (DECSASD 1) ...
v_stx:      .res        1                                   ;   and its cursor (its text: w_stat, cons.s's)
v_rbase:    .res        1                                   ; Its map's ring's start (the whole screen's scroll turns
                                                            ;   it: vmap, the map, is outside the state)
vs_last:
VS_N        = vs_last - vs_first
.assert     VS_N < 256 .and VS_N <= VS_PAGE, error, "A window's state is a page at most"
.assert     WIN_COLS < 128 .and POOL = 64, error, "A row's 128 cells, its meta the last; ring_resize's POOL - 1"
vb0:        .res        1                                   ; Its planes' banks: the characters, the colours, the
vb1:        .res        1                                   ;   rendition
vb2:        .res        1
vt_w:       .res        1                                   ; The window loaded ($FF: none)
vw_bank:    .res        WIN_MAX                             ; Each window's first bank (0: none) ...
vw_abank:   .res        WIN_MAX                             ;   its alternate screen's (0: none yet) ...
vw_rb:      .res        WIN_MAX * 2                         ;   each screen's ring's start and scrollback's rows, while
vw_sb:      .res        WIN_MAX * 2                         ;   the other's in use (main: the window * 2; alternate: + 1)
vw_maps:    .res        WIN_MAX * 2 * POOL                  ;   and each screen's map
vt_save:    .res        WIN_MAX * VS_PAGE                   ; Each window's state, while another's is loaded
tr_off:     .res        2                                   ; Each terminal's (0 the serial port, 1 the screen) rows
                                                            ;   above the shown window's (chr_geom's) ...
tr_bar:     .res        2                                   ;   its bar's row ($FF: none) ...
tr_head:    .res        2                                   ;   the window's header's ...
tr_foot:    .res        2                                   ;   and its footer's
ser_chr:    .res        1                                   ; <> 0: the serial port has chrome rows (chr_geom's)
sp_chr:     .res        1                                   ; The serial port's chrome drawn: the row (0 the bar, 1 the
sp_ccol:    .res        1                                   ;   header, 2 the footer, 3 done), its next cell
chr_t:      .res        1                                   ; (chr_draw's: the terminal, a row, a cell)
chr_r:      .res        1
chr_k:      .res        1
ts_ser:     .res        1                                   ; The terminals' states (cons.inc)
ts_scr:     .res        1
fw_ser:     .res        1                                   ; <> 0: the output written now goes to the serial port ...
fw_scr:     .res        1                                   ;   and to the screen
out_t:      .res        1                                   ; out's terminal: 0 the serial port, 1 the screen
sp_row:     .res        1                                   ; The serial port's paint: the row, the column ...
sp_col:     .res        1
sp_last:    .res        1                                   ;   that row's cells to paint ...
sp_c:       .res        1                                   ;   the colours and rendition the terminal has ...
sp_f:       .res        1
sp_dec:     .res        1                                   ;   <> 0: its G0 is the DEC graphics ...
sp_cy:      .res        1                                   ;   its cursor's row ...
sp_cx:      .res        1                                   ;   its cursor's column ...
ser_tcem:   .res        1                                   ; The serial port's cursor: shown (VM_TCEM) or not
sp_again:   .res        1                                   ;   <> 0: the window written to as it was painted (scroll
                                                            ;   jump): painted again after ...
sp_full:    .res        1                                   ;   <> 0: past the last column (the row before painted
                                                            ;   to its end)
scr_x:      .res        1                                   ; vid's cursor as it is ($FF: not known) ...
scr_y:      .res        1
scr_wrap:   .res        1                                   ;   <> 0: past its last column (its next character
                                                            ;   wraps: not used, a CUP comes first) ...
scr_c:      .res        1                                   ;   its colours and rendition ($FF: not known) ...
scr_f:      .res        1
scr_sync:   .res        1                                   ;   <> 0: its cursor to be put where the window's is ...
scr_dec:    .res        1                                   ;   <> 0: its G0 the DEC graphics (ESC ( 0) ...
scr_fail:   .res        1                                   ;   <> 0: a write refused (claimed): no more till the next
                                                            ;   request (painted then)
ans_n:      .res        WIN_MAX                             ; Each window's answers (DA, DSR ...): their bytes ...
ans_r:      .res        WIN_MAX                             ;   those read ...
ans_buf:    .res        WIN_MAX * ANS_SIZE                  ;   and them
lbl_buf:    .res        WIN_MAX * LBL_SIZE                  ; Each window's label (OSC 0 and 2's title), zero-ended
scr_n:      .res        1                                   ; The screen's bytes in scr_buf
scr_buf:    .res        SCR_BUF
su_t:       .res        1                                   ; A scroll: the region's top and bottom ...
su_b:       .res        1
su_n:       .res        1                                   ;   the rows ...
su_f:       .res        1                                   ;   the map's first and last entries moved ...
su_l:       .res        1
su_keep:    .res        1                                   ;   <> 0: rows off the top (a region at it) kept
bs_f:       .res        1                                   ; blank_span's columns, from and to ...
bs_t:       .res        1
bs_c:       .res        1                                   ;   and the colours (the background's: BCE)
bs_a:       .res        1                                   ; (blank_span's columns)
bs_b:       .res        1
su_i:       .res        1                                   ; (A scroll's entry moved, and the row going round)
su_r:       .res        1
cg_y:       .res        1                                   ; (cell_get's column)
rl_bf:      .res        1                                   ; (row_last's blank end)
pg_w:       .res        1                                   ; (print_glyph's: the row's width)
fp_x:       .res        1                                   ; (fc_pos's: vid's column for the cursor)
sp_dw:      .res        1                                   ; (scr_paint's: the row's a double one)
vt_i:       .res        1                                   ; Counters
vt_j:       .res        1
vt_k:       .res        1
vt_n:       .res        1                                   ; A number (a sequence's first, at least 1)
vw_k:       .res        1                                   ; vt_write's bytes so far
vd_i:       .res        1                                   ; A mode's number (SM, RM, DECSET, DECRST) ...
vd_set:     .res        1                                   ;   <> 0: set
rep_n:      .res        1                                   ; REP's count
od_n:       .res        1                                   ; (out_dec's)
vt_scr:     .res        1                                   ; <> 0: the screen scrolled with the window (print)
cell_c:     .res        1                                   ; cell_get's: the character, colours, rendition
cell_a:     .res        1
cell_f:     .res        1
tbuf:       .res        256                                 ; /text's bytes for a read
tlen:       .res        POOL + HIST_MAX                     ;   each of its rows' length (trailing blanks off) ...
tc_w:       .res        1                                   ;   for this window ($FF: none) ...
tc_n:       .res        1                                   ;   its rows
rz_o:       .res        1                                   ; A resize: the rows it had, has ...
rz_n:       .res        1
rz_y:       .res        1                                   ;   a cursor's row ...
rz_k:       .res        1                                   ;   the rows to or from the scrollback ...
rz_d:       .res        1                                   ;   and those blanked or dropped at the bottom
vw_add:     .res        WIN_MAX * 2                         ; Each window's lines dropped off its oldest end (the
                                                            ;   scrollback full, a row in): the view's
vbuf:       .res        3 * 128                             ; A row's cells (its planes'), or a line's text (vt_line)
vw_hb:      .res        WIN_MAX                             ; Each window's history (history N): its first bank ...
vw_hs:      .res        WIN_MAX                             ;   its sets of three (64 rows each; 0: none) ...
vw_hh:      .res        WIN_MAX                             ;   its next row's place (full, its oldest's) ...
vw_hn:      .res        WIN_MAX                             ;   and its rows
hk0:        .res        1                                   ; A history row's banks, its planes' (hist_ptr's) ...
hk1:        .res        1
hk2:        .res        1
hp_be:      .res        1                                   ;   a row's cells copied (hist_put's), the history's rows
hp_n:       .res        1                                   ;   (text_row's), its mask, its sets, its window
hp_m:       .res        1
hp_s:       .res        1
hp_w:       .res        1

.segment "CODE2"
; ****************************************************************************
; The calls from cons.s (FAR2)

; The driver's start: no window loaded; the serial port following (it shows the boot), the screen to be painted
vt_init:
            lda         #$FF
            sta         vt_w
            sta         tc_w
            sta         scr_y
            sta         scr_c
            stz         ts_ser
            lda         #1
            sta         ts_scr
            stz         scr_n
            stz         scr_dec
            stz         scr_fail
            lda         #VM_TCEM                            ; (The PC's terminal's cursor shown)
            sta         ser_tcem
            rts

; Window .X made: its banks and its state (a screen cleared, the cursor home).  OUT: C = 0; or C = 1, .A = E_NOMEM
vt_new:
            stx         vt_i
            lda         #3
            jsr         BANKS_ALLOC
            bcc         :+
            lda         #E_NOMEM
            rts
:
            ldx         vt_i
            sta         vw_bank,X
            stz         vw_abank,X                          ; (No alternate screen yet ...
            stz         ans_n,X                             ;   no answers, no label, no lines dropped)
            stz         ans_r,X
            txa
            asl
            tax
            stz         vw_add,X
            stz         vw_add + 1,X
            ldx         vt_i
            stz         vw_hs,X                             ; (No history)
            stz         vw_hn,X
            stz         vw_hh,X
            txa
            jsr         lbl_at
            lda         #0
            sta         (vt_a)
            ldx         vt_i
            jsr         vt_unload                           ; (Its state isn't in vt_save yet: made in place)
            lda         vt_i
            sta         vt_w
            stz         v_alt
            stz         v_sasd
            jsr         banks
            lda         lay_cols                            ; (The layout's size)
            sta         v_cols
            lda         lay_rows
            sta         v_rows
            sec
            lda         #POOL
            sbc         lay_rows
            sta         v_sb0
            stz         v_sbn
            ldy         #POOL - 1                           ; The map: each row its own
:
            tya
            sta         (vmap),Y
            dey
            bpl         :-
            stz         v_rbase
            jsr         reset                               ; (The screen cleared)
            clc
            rts

; The keys' modes of window .A (cons.s's keys vt): .A = DECCKM's and DECKPAM's bits (VM_CKM, VM_KPAM), .X = VT52
; mode's (VM2_VT52)
vt_keymodes:
            jsr         vt_load
            lda         v_mode2
            and         #VM2_VT52
            tax
            lda         v_mode
            and         #VM_CKM | VM_KPAM
            rts

; ****************************************************************************
; The scrollback's view (W6b): cons.s's window vv_w showing window vv_src's lines (its scrollback's, oldest first,
; then its screen's)

; Window .A's lines: .A = them; vv_add, those dropped off its oldest end so far
vt_lines:
            pha
            jsr         vt_load
            pla
            asl
            tax
            lda         vw_add,X
            sta         vv_add
            lda         vw_add + 1,X
            sta         vv_add + 1
            jmp         lines_n

; The view filled: its rows vv_src's lines from vv_top on (past them, blank), those from vv_ma to vv_mb (either
; order; vv_ma $FF: none) to the row's end, reversed; its cursor at the start of its row vv_cur.  If it's shown, the
; terminals painted again
vt_view:
            stz         vt_k
@row:
            lda         vv_w
            jsr         vt_load
            lda         vt_k
            cmp         v_rows
            bcc         :+
            jmp         @cursor
:
            lda         vv_src                              ; The line's cells, into vbuf
            jsr         vt_load
            clc
            lda         vv_top
            adc         vt_k
            sta         vt_j
            jsr         lines_n
            cmp         vt_j
            beq         @blank
            bcc         @blank
            jsr         text_row
            lda         vb0
            sta         $00
            ldy         #127
:
            lda         (vq),Y
            sta         vbuf,Y
            dey
            bpl         :-
            lda         vb1
            sta         $00
            ldy         #127
:
            lda         (vq),Y
            sta         vbuf + 128,Y
            dey
            bpl         :-
            lda         vb2
            sta         $00
            ldy         #127
:
            lda         (vq),Y
            sta         vbuf + 256,Y
            dey
            bpl         :-
            bra         @put
@blank:                                                     ; (Past them: a blank row)
            stz         vbuf + META
            lda         #COL_DEF
            sta         vbuf + 128 + META
            stz         vbuf + 256 + META
@put:
            lda         vv_w                                ; Into the view's row
            jsr         vt_load
            lda         vt_k
            jsr         row_ptr
            lda         vb0
            sta         $00
            ldy         #127
:
            lda         vbuf,Y
            sta         (vq),Y
            dey
            bpl         :-
            lda         vb1
            sta         $00
            ldy         #127
:
            lda         vbuf + 128,Y
            sta         (vq),Y
            dey
            bpl         :-
            lda         vb2
            sta         $00
            ldy         #127
:
            lda         vbuf + 256,Y
            sta         (vq),Y
            dey
            bpl         :-
            jsr         @sel
            bcc         :+
            jsr         @reverse
:
            inc         vt_k
            jmp         @row

@cursor:                                                    ; Its cursor, shown; painted, if it's shown
            stz         v_x
            lda         vv_cur
            sta         v_y
            stz         v_wrap
            lda         v_mode
            ora         #VM_TCEM
            sta         v_mode
            jsr         cur_row
            lda         vv_w
            cmp         w_in
            bne         :+
            lda         #1
            sta         ts_ser
            sta         ts_scr
:
            rts

@sel:                                                       ; (C = 1: line vt_j is in the selection)
            lda         vv_ma
            cmp         #$FF
            beq         @no
            cmp         vv_mb                               ; (Its first: the lesser)
            bcc         :+
            lda         vv_mb
:
            cmp         vt_j
            beq         :+
            bcs         @no
:
            lda         vv_ma                               ; (Its last: the greater)
            cmp         vv_mb
            bcs         :+
            lda         vv_mb
:
            cmp         vt_j
            bcc         @no
            sec
            rts
@no:
            clc
            rts

@reverse:                                                   ; (The row at vq to its end, its blank end's cells
            lda         vb1                                 ;   written as blanks; all of it reversed)
            sta         $00
            ldy         #META
            lda         (vq),Y
            sta         vt_n
            lda         vb0
            sta         $00
            lda         (vq),Y
            tay
@fill:
            cpy         v_cols
            bcs         @full
            lda         vb0
            sta         $00
            lda         #' '
            sta         (vq),Y
            lda         vb1
            sta         $00
            lda         vt_n
            sta         (vq),Y
            lda         vb2
            sta         $00
            lda         #0
            sta         (vq),Y
            iny
            bra         @fill
@full:
            lda         vb0
            sta         $00
            ldy         #META
            lda         v_cols
            sta         (vq),Y
            lda         vb2
            sta         $00
            ldy         v_cols
:
            dey
            bmi         :+
            lda         (vq),Y
            ora         #F_REV
            sta         (vq),Y
            bra         :-
:
            rts

; Window .A's line .X (0: its scrollback's oldest) as text, into vbuf: its trailing blanks off, DEC graphics as
; ASCII (as /text's).  OUT: .A = its length
vt_line:
            stx         vt_j
            jsr         vt_load
            jsr         text_row
            jsr         row_chars
            sta         vt_n
            lda         vb0
            sta         $00
            ldy         #0
:
            cpy         vt_n
            bcs         :++
            lda         (vq),Y
            cmp         #$20
            bcs         :+
            tax
            lda         dec_ascii,X
:
            sta         vbuf,Y
            iny
            bra         :--
:
            jsr         banks                               ; (The window's own banks again)
            lda         vt_n
            rts

; Window .X's history: .A sets of 64 rows (0-2: HIST_MAX / 64; three banks each), empty, the one it had gone.
; OUT: C = 0; or C = 1, .A = E_NOMEM (it has none)
vt_history:
            sta         hp_n
            stx         hp_w
            cpx         tc_w                                ; (/text's lengths found again)
            bne         :+
            lda         #$FF
            sta         tc_w
:
            lda         vw_hs,X                             ; The old one's banks back
            beq         :+
            sta         hp_s
            asl
            adc         hp_s
            tax
            ldy         hp_w
            lda         vw_hb,Y
            jsr         BANKS_FREE
:
            ldx         hp_w
            stz         vw_hs,X
            stz         vw_hn,X
            stz         vw_hh,X
            lda         hp_n
            beq         @done
            asl
            adc         hp_n
            jsr         BANKS_ALLOC
            bcs         @fail
            ldx         hp_w
            sta         vw_hb,X
            lda         hp_n
            sta         vw_hs,X
@done:
            clc
            rts
@fail:
            lda         #E_NOMEM
            sec
            rts

; Bracketed paste (?2004) in window .A's screen: .A <> 0, it's set
vt_paste:
            jsr         vt_load
            lda         v_mode2
            and         #VM2_BPM
            rts

; Window .X resized to the layout's size (lay_cols x lay_rows): its screen's ring turned so the cursor's row stays
; on it (ring_resize), the cells past a narrower width dropped, the cursor and the saved one in it, the margins the
; whole screen.  The alternate screen, in use, too (rows off its top dropped: it has no scrollback), and the main one
; with the saved cursor's row as its cursor's (?1049's)
vt_resize:
            txa
            jsr         vt_load
            lda         v_rows
            sta         rz_o
            lda         lay_rows
            sta         rz_n
            cmp         rz_o
            bne         :+
            lda         lay_cols
            cmp         v_cols
            bne         :+
            rts
:
            lda         v_alt
            beq         @main
            lda         v_y                                 ; The alternate screen
            jsr         ring_resize
            sta         v_y
            stz         v_sbn
            jsr         trunc_cols
            lda         #0                                  ; Then the main one
            jsr         buf_to
            lda         v_sy
            jsr         ring_resize
            sta         v_sy
            jsr         trunc_cols
            lda         #1
            jsr         buf_to
            bra         @size
@main:
            lda         v_y
            jsr         ring_resize
            sta         v_y
            jsr         trunc_cols
@size:
            lda         rz_n
            sta         v_rows
            sec
            lda         #POOL
            sbc         rz_n
            sta         v_sb0
            lda         lay_cols
            sta         v_cols
            ldx         v_x                                 ; The cursors in it
            jsr         in_cols
            stx         v_x
            ldx         v_sx
            jsr         in_cols
            stx         v_sx
            lda         v_sy
            cmp         v_rows
            bcc         :+
            ldx         v_rows
            dex
            stx         v_sy
:
            stz         v_wrap
            jsr         full_margins
            lda         tc_w                                ; (/text's rows' lengths: found again)
            cmp         vt_w
            bne         :+
            lda         #$FF
            sta         tc_w
:
            jmp         cur_row

; .X no more than the last column
in_cols:
            cpx         v_cols
            bcc         :+
            ldx         v_cols
            dex
:
            rts

; Window .X's cursor, for the line editor: .A its column, .X the columns, .Y <> 0 past the last (its last-column flag)
vt_cursor:
            txa
            jsr         vt_load
            lda         v_x
            ldx         v_cols
            ldy         v_wrap
            rts

; Window .X's size: .A its columns, .X its rows
vt_size:
            txa
            jsr         vt_load
            lda         v_cols
            ldx         v_rows
            rts

; Window .X gone: its banks back
vt_free:
            cpx         vt_w
            bne         :+
            lda         #$FF
            sta         vt_w
:
            cpx         tc_w
            bne         :+
            lda         #$FF
            sta         tc_w
:
            lda         vw_hs,X                             ; Its history's banks back
            beq         :+
            stz         vw_hs,X
            stz         vw_hn,X
            phx
            sta         hp_s
            asl
            adc         hp_s
            pha
            lda         vw_hb,X
            plx
            jsr         BANKS_FREE
            plx
:
            lda         vw_abank,X
            beq         :+
            stz         vw_abank,X
            phx
            ldx         #3
            jsr         BANKS_FREE
            plx
:
            lda         vw_bank,X
            stz         vw_bank,X
            ldx         #3
            jmp         BANKS_FREE

; The cnt bytes in iobuf, the output of window lw: as many as there's room for (the shown window, its terminal
; following on the serial port: VT_ROOM a byte in the send ring).  OUT: .A = the bytes taken
vt_write:
            ldx         lw                                  ; (Not shown, monitor on: marked, +)
            cpx         w_in
            beq         :+
            lda         w_mon,X
            beq         :+
            lda         #ACT_OUT
            jsr         act_mark
:
            lda         lw
            jsr         vt_load
            jsr         fw_setup
            jsr         jump_again
            stz         vw_k
@byte:
            ldx         vw_k
            cpx         cnt
            bcs         @done
            lda         fw_ser
            beq         :+
            jsr         tx_free
            cmp         #VT_ROOM
            bcs         :+
            ldx         vt_w                                ; (No room: scroll smooth, the rest waits; scroll jump,
            lda         w_jump,X                            ;   it's taken, the serial port painted after)
            beq         @done
            jsr         ser_dirty
:
            ldx         vw_k
            lda         iobuf,X
            jsr         vt_byte
            inc         vw_k
            bra         @byte

@done:
            lda         vw_k
            rts

; Scroll jump: the shown window written to while the serial port's painted, which is to paint it again after
jump_again:
            lda         vt_w
            cmp         w_in
            bne         @done
            lda         term
            and         #TERM_SERIAL
            beq         @done
            lda         ser_rd
            bne         @done
            lda         ts_ser
            beq         @done
            lda         #1
            sta         sp_again
@done:
            rts

; .A, a byte of window lw's output (its line editor's echo: cons.s has made sure of the room)
vt_put:
            pha
            lda         lw
            jsr         vt_load
            jsr         fw_setup
            pla
            jmp         vt_byte

; After each request: the terminals painted (the serial port's as the send ring has room), the screen's cursor
; where the window's is, and its bytes to #v/term.  (cons.s has opened #v/term if the screen's on, and leaves the
; line /ser's alone)
vt_pump:
            lda         ser_rd
            bne         @screen
            lda         term
            and         #TERM_SERIAL
            beq         @screen
            lda         chr_dirty                           ; Its chrome changed, following: drawn again (ts_ser 3)
            and         #1
            beq         :+
            lda         ts_ser
            bne         :+
            lda         #1
            trb         chr_dirty
            lda         ser_chr
            beq         :+
            lda         #3
            sta         ts_ser
            stz         sp_chr
            stz         sp_ccol
            inc         chr_pass
:
            lda         ts_ser
            beq         @screen
            stz         out_t
            jsr         ser_paint
@screen:
            lda         term
            and         #TERM_SCREEN
            beq         @done
            lda         scr_st
            cmp         #1
            bne         @done
            lda         #1
            sta         out_t
            stz         scr_fail
            lda         ts_scr
            beq         @sync
            stz         ts_scr                              ; (A write refused: to be painted again)
            jsr         scr_paint
            bra         @flush

@sync:
            lda         chr_dirty                           ; Its chrome changed: drawn again
            and         #2
            beq         :+
            trb         chr_dirty
            lda         w_in
            jsr         vt_load
            jsr         chr_geom
            ldx         #1
            jsr         chr_draw
            lda         #1
            sta         scr_sync
:
            lda         scr_sync
            beq         @flush
            stz         scr_sync
            lda         w_in
            jsr         vt_load
            jsr         fc_pos
@flush:
            jmp         scr_flush

@done:
            rts

; /text of window .X: its scrollback and screen as text, a line each row (its trailing blanks off).  A read at
; RQ_OFFSET for RQ_COUNT (255 at most) into tbuf, then to the client.  (Each row's length is found as a read starts
; at 0, and kept for the reads after it)
vt_text:
            jsr         text_go
            php
            pha
            jsr         banks                               ; (The window's own banks: the history's may be in vb0-vb2)
            pla
            plp
            rts
text_go:
            stx         vt_i
            txa
            jsr         vt_load
            stz         TASK_INBOX + RQ_DONE
            stz         TASK_INBOX + RQ_DONE + 1
            lda         TASK_INBOX + RQ_OFFSET + 2          ; (Past 64K: the end)
            ora         TASK_INBOX + RQ_OFFSET + 3
            beq         :+
            clc
            rts
:
            lda         TASK_INBOX + RQ_OFFSET
            ora         TASK_INBOX + RQ_OFFSET + 1
            beq         @lengths
            lda         vt_i
            cmp         tc_w
            beq         @find
@lengths:                                                   ; Each row's length
            lda         vt_i
            sta         tc_w
            jsr         lines_n
            sta         tc_n
            stz         vt_j
:
            lda         vt_j
            cmp         tc_n
            bcs         @find
            jsr         text_row                            ; (vq: its cells)
            jsr         row_chars
            ldx         vt_j
            sta         tlen,X
            inc         vt_j
            bra         :-

@find:                                                      ; The row the offset's in: vt_a its start
            stz         vt_a
            stz         vt_a + 1
            stz         vt_j
@row:
            lda         vt_j
            cmp         tc_n
            bcc         :+
            clc                                             ; (Past the end: none)
            rts
:
            tax                                             ; The row's end (its LF's place + 1): vr
            clc
            lda         tlen,X
            adc         #1
            adc         vt_a
            sta         vr
            lda         vt_a + 1
            adc         #0
            sta         vr + 1
            lda         TASK_INBOX + RQ_OFFSET              ; The offset before it?
            cmp         vr
            lda         TASK_INBOX + RQ_OFFSET + 1
            sbc         vr + 1
            bcc         @in
            MOVR        vt_a, vr
            inc         vt_j
            bra         @row

@in:                                                        ; From the offset's place in row vt_j: vt_k
            sec
            lda         TASK_INBOX + RQ_OFFSET
            sbc         vt_a
            sta         vt_k
            stz         vt_n                                ; (vt_n: the bytes in tbuf)
@give:
            lda         vt_n                                ; Enough?  (The count, or 255)
            cmp         #255
            bcs         @out
            ldx         TASK_INBOX + RQ_COUNT + 1
            bne         :+
            cmp         TASK_INBOX + RQ_COUNT
            bcs         @out
:
            lda         vt_j
            cmp         tc_n
            bcs         @out
            jsr         text_row
            ldx         vt_j
            lda         vt_k
            cmp         tlen,X
            bcc         @char
            lda         #LF                                 ; The row's end: its LF, then the next row
            inc         vt_j
            stz         vt_k
            bra         @put

@char:
            ldy         vt_k
            lda         vb0
            sta         $00
            lda         (vq),Y
            cmp         #$20
            bcs         :+
            tax                                             ; (A DEC graphic: as ASCII)
            lda         dec_ascii,X
:
            inc         vt_k
@put:
            ldx         vt_n
            sta         tbuf,X
            inc         vt_n
            bra         @give

@out:
            lda         vt_n
            sta         TASK_INBOX + RQ_DONE
            beq         @none
            sta         r2
            stz         r2 + 1
            LDR         r0, tbuf
            MOVR        r1, TASK_INBOX + RQ_BUF
            jsr         CLIENT_WRITE
@none:
            clc
            rts

; vq = /text's row vt_j: the history's, oldest first (vb0-vb2 then its banks: banks puts the window's back), then the
; scrollback's, then the screen's
text_row:
            jsr         hist_n
            sta         hp_n
            sec
            lda         vt_j
            sbc         hp_n
            bcs         @pool
            ldx         vt_w                                ; (The history's: back from its next place)
            clc
            adc         vw_hh,X
            pha
            jsr         hist_mask
            sta         hp_m
            pla
            and         hp_m
            jsr         hist_ptr
            lda         vr
            sta         vq
            lda         vr + 1
            sta         vq + 1
            lda         hk0
            sta         vb0
            lda         hk1
            sta         vb1
            lda         hk2
            sta         vb2
            rts
@pool:
            pha
            jsr         banks
            pla
            clc
            adc         v_sb0
            sec
            sbc         v_sbn
            jsr         map_row
            jmp         pool_ptr

; .A = the loaded window's lines: its history's, its scrollback's, its screen's
lines_n:
            jsr         hist_n
            clc
            adc         v_sbn
            clc
            adc         v_rows
            rts

; .A = the loaded window's history's rows (none while its alternate screen's in use).  Modifies .X
hist_n:
            lda         v_alt
            bne         :+
            ldx         vt_w
            lda         vw_hn,X
            rts
:
            lda         #0
            rts

; .A = window .X's history's places less one (its sets * 64 - 1: a mask)
hist_mask:
            lda         vw_hs,X
            asl
            asl
            asl
            asl
            asl
            asl
            dec         a
            rts

; vr = the loaded window's history's place .A (its row, at $8000 + 128 * its place's in its set), hk0-hk2 its
; banks.  Modifies .X
hist_ptr:
            pha
            and         #POOL - 1
            lsr
            ora         #$80
            sta         vr + 1
            lda         #0
            ror
            sta         vr
            pla
            ldx         vt_w
            and         #POOL                               ; (Its set: 0 or 1, three banks each)
            beq         :+
            lda         #3
:
            clc
            adc         vw_hb,X
            sta         hk0
            inc         a
            sta         hk1
            inc         a
            sta         hk2
            rts

; The row at vq, the loaded window's scrollback's oldest going round (it's full), into its history, if it has one
; (full, its oldest written over); a row gone is counted (vw_add, the view's)
hist_put:
            ldx         vt_w
            lda         vw_hs,X
            beq         @gone
            lda         vw_hh,X                             ; Its next place
            jsr         hist_ptr
            lda         vb0                                 ; (Each plane's cells to the row's blank end, and its
            sta         $00                                 ;   meta, through vbuf)
            ldy         #META
            lda         (vq),Y
            cmp         #META
            bcc         :+
            lda         #META
:
            sta         hp_be
            ldx         #0
@plane:
            lda         vb0,X
            sta         $00
            ldy         #META
            lda         (vq),Y
            sta         vbuf + META
            ldy         hp_be
:
            dey
            bmi         :+
            lda         (vq),Y
            sta         vbuf,Y
            bra         :-
:
            lda         hk0,X
            sta         $00
            ldy         #META
            lda         vbuf + META
            sta         (vr),Y
            ldy         hp_be
:
            dey
            bmi         :+
            lda         vbuf,Y
            sta         (vr),Y
            bra         :-
:
            inx
            cpx         #3
            bcc         @plane
            ldx         vt_w                                ; Its next place; a row more, or (full) its oldest gone
            jsr         hist_mask
            sta         hp_m
            lda         vw_hh,X
            inc         a
            and         hp_m
            sta         vw_hh,X
            lda         vw_hn,X
            cmp         hp_m
            beq         :+
            bcs         @gone
:
            inc         vw_hn,X
            rts
@gone:
            lda         vt_w
            asl
            tax
            inc         vw_add,X
            bne         :+
            inc         vw_add + 1,X
:
            rts

; The loaded window's history emptied (RIS, ED 3).  Modifies .X
hist_clear:
            ldx         vt_w
            stz         vw_hn,X
            stz         vw_hh,X
            rts

; .A = the row at vq's characters but its trailing blanks (spaces, whatever their colours; its blank end's)
row_chars:
            lda         vb0
            sta         $00
            ldy         #META
            lda         (vq),Y
            cmp         v_cols
            bcc         :+
            lda         v_cols
:
            tay
:
            dey
            bmi         :+
            lda         (vq),Y
            cmp         #' '
            beq         :-
:
            iny
            tya
            rts

; ****************************************************************************
; The windows' states

; Window .A's state loaded (vs_*), the one that was saved first
vt_load:
            cmp         vt_w
            beq         @done
            pha
            jsr         vt_unload
            pla
            sta         vt_w
            jsr         slot
            ldy         #VS_N - 1
:
            lda         (vt_a),Y
            sta         vs_first,Y
            dey
            bne         :-
            lda         (vt_a)
            sta         vs_first
            jsr         banks
            jmp         cur_row

@done:
            rts

; The loaded window's state back in its place (none loaded after)
vt_unload:
            lda         vt_w
            bmi         @done
            jsr         slot
            ldy         #VS_N - 1
:
            lda         vs_first,Y
            sta         (vt_a),Y
            dey
            bne         :-
            lda         vs_first
            sta         (vt_a)
            lda         #$FF
            sta         vt_w
@done:
            rts

; vt_a = window .A's place in vt_save
slot:
            clc
            adc         #>vt_save
            sta         vt_a + 1
            lda         #<vt_save
            sta         vt_a
            rts

; vb0-vb2: the loaded window's banks, its screen's in use (main or alternate); vmap its map
banks:
            lda         vt_w                                ; vmap: vw_maps + (the window * 2 + v_alt) * POOL
            asl
            ora         v_alt
            tax
            lsr
            lsr
            clc
            adc         #>vw_maps
            sta         vmap + 1
            txa
            asl
            asl
            asl
            asl
            asl
            asl
            clc
            adc         #<vw_maps
            sta         vmap
            bcc         :+
            inc         vmap + 1
:
            ldx         vt_w
            lda         vw_bank,X
            ldy         v_alt
            beq         :+
            lda         vw_abank,X
:
            sta         vb0
            inc         a
            sta         vb1
            inc         a
            sta         vb2
            rts

; fw_ser, fw_scr: is the loaded window's output to go to each terminal now (it's shown, the terminal's on and
; following)?
fw_setup:
            stz         fw_ser
            stz         fw_scr
            lda         vt_w
            cmp         w_in
            bne         @done
            lda         term
            and         #TERM_SERIAL
            beq         @screen
            lda         ser_rd
            bne         @screen
            lda         ts_ser
            bne         @screen
            inc         fw_ser
@screen:
            lda         term
            and         #TERM_SCREEN
            beq         @done
            lda         scr_st
            cmp         #1
            bne         @done
            lda         ts_scr
            bne         @done
            inc         fw_scr
@done:
            rts

; The terminals that can't follow what's just been done: to be painted, and nothing more to them now
ser_dirty:
            lda         fw_ser
            beq         :+
            lda         #1
            sta         ts_ser
            stz         fw_ser
:
            rts

scr_dirty:
            lda         fw_scr
            beq         :+
            lda         #1
            sta         ts_scr
            stz         fw_scr
:
            rts

; ****************************************************************************
; The parser

; .A, a byte of the loaded window's output
vt_byte:
            sta         vch
            cmp         #DEL                                ; (Nothing, anywhere)
            beq         @done
            cmp         #$20
            bcs         @state
            cmp         #ESC
            beq         @esc
            cmp         #CAN
            beq         @cancel
            cmp         #SUB
            beq         @sub
            ldx         v_state                             ; A C0 control: done at once, in a sequence too; in a
            cpx         #S_OSC                              ;   string nothing (but BEL ends an OSC)
            bcs         @string
            jmp         c0
@string:
            cmp         #BEL
            bne         @done
            cpx         #S_STR
            bcs         @done
            stz         v_state                             ; (An OSC ended)
@done:
            rts

@esc:                                                       ; ESC: a sequence starts (in a string: ST, perhaps)
            ldx         v_state
            cpx         #S_OSC
            beq         @oscesc
            cpx         #S_STR
            beq         @stresc
            jmp         esc_start

@oscesc:
            lda         #S_OSCE
            sta         v_state
            rts

@stresc:
            lda         #S_STRE
            sta         v_state
            rts

@cancel:
            stz         v_state
            rts

@sub:                                                       ; SUB: the sequence ended, and an error shown
            stz         v_state
            lda         #G_ERROR
            jmp         print_glyph

@state:
            ldx         v_state
            bne         :+
            jmp         print                               ; (Ground: a character)
:
            txa
            asl
            tax
            lda         vch
            jmp         (state_vec,X)

; ESC: a sequence starts
esc_start:
            lda         #S_ESC
            sta         v_state
            stz         v_inter
            stz         v_priv
            lda         #ESC
            sta         v_raw
            lda         #1
            sta         v_rawn
            rts

; vch into the sequence's bytes kept (too many: $FF, not kept).  Keeps .A
raw_add:
            ldx         v_rawn
            cpx         #VT_RAW
            bcs         @over
            sta         v_raw,X
            inc         v_rawn
            rts

@over:
            ldx         #$FF
            stx         v_rawn
            rts

; After ESC (in VT52 mode, its own: st_vt52)
st_esc:
            pha
            lda         v_mode2
            and         #VM2_VT52
            beq         :+
            pla
            jmp         st_vt52
:
            pla
            jsr         raw_add
            cmp         #$30
            bcs         :+
            sta         v_inter                             ; An intermediate
            lda         #S_ESCI
            sta         v_state
            rts
:
            cmp         #$80
            bcs         to_ground
            cmp         #'['
            beq         csi_start
            cmp         #']'
            beq         @osc
            cmp         #'P'
            beq         @str
            cmp         #'X'
            beq         @str
            cmp         #'^'
            beq         @str
            cmp         #'_'
            beq         @str
            stz         v_state
            jmp         esc_do

@osc:
            lda         #S_OSC
            sta         v_state
            stz         v_oscn
            lda         #$FF
            sta         v_osci
            rts

@str:
            lda         #S_STR
            sta         v_state
            rts

to_ground:
            stz         v_state
            rts

; ESC, then intermediates
st_esci:
            jsr         raw_add
            cmp         #$30
            bcs         :+
            lda         #$FF                                ; (A second: none of ours)
            sta         v_inter
            rts
:
            cmp         #$80
            bcs         to_ground
            stz         v_state
            jmp         esc_do

; CSI: its numbers start
csi_start:
            lda         #S_CSI
            sta         v_state
            stz         v_npar
            stz         v_pseen
            ldx         #VT_NPAR - 1
:
            stz         v_parl,X
            stz         v_parh,X
            dex
            bpl         :-
            rts

; In a CSI: digits, ;, a private marker, an intermediate, or the final byte
st_csi:
            jsr         raw_add
            cmp         #'0'
            bcc         @inter
            cmp         #'9' + 1
            bcc         @digit
            cmp         #';'
            beq         @semi
            cmp         #'<'
            bcc         @ignore                             ; (:, a sub-parameter's: not ours)
            cmp         #'@'
            bcs         @final
            ldx         v_pseen                             ; < = > ?: the private marker, before any number
            bne         @ignore
            ldx         v_priv
            bne         @ignore
            sta         v_priv
            rts

@inter:
            sta         v_inter
            lda         #S_CSII
            sta         v_state
            rts

@ignore:
            lda         #S_CSIX
            sta         v_state
            rts

@semi:
            sta         v_pseen                             ; (The numbers past the last: into it)
            lda         v_npar
            cmp         #VT_NPAR - 1
            bcs         :+
            inc         v_npar
:
            rts

@digit:
            ldx         v_npar
            sta         v_pseen
            and         #$0F
            sta         vt_k
            lda         v_parh,X                            ; * 10 + the digit (6400 and more: 65535)
            cmp         #$19
            bcs         @big
            lda         v_parl,X
            asl
            sta         vt_a
            lda         v_parh,X
            rol
            sta         vt_a + 1                            ; (* 2)
            lda         vt_a
            asl
            sta         vr
            lda         vt_a + 1
            rol
            sta         vr + 1
            asl         vr
            rol         vr + 1                              ; (* 8)
            clc
            lda         vr
            adc         vt_a
            sta         vr
            lda         vr + 1
            adc         vt_a + 1
            sta         vr + 1
            clc
            lda         vr
            adc         vt_k
            sta         v_parl,X
            lda         vr + 1
            adc         #0
            sta         v_parh,X
@done:
            rts

@big:
            lda         #$FF
            sta         v_parl,X
            sta         v_parh,X
            rts

@final:
            stz         v_state
            cmp         #$7F
            bcs         :+
            jmp         csi_do
:
            rts

; CSI, then intermediates
st_csii:
            jsr         raw_add
            cmp         #$30
            bcs         :+
            lda         #$FF
            sta         v_inter
            rts
:
            cmp         #$40
            bcc         :++
            stz         v_state
            cmp         #$7F
            bcs         :+
            jmp         csi_do
:
            rts
:
            lda         #S_CSIX
            sta         v_state
            rts

; A CSI being ignored: to its final byte
st_csix:
            cmp         #$40
            bcc         :+
            cmp         #$7F
            bcs         :+
            stz         v_state
:
            rts

; In a string: nothing
st_str:
            rts

; In an OSC: its number, a ;, then its text: OSC 0's and 2's the window's label (lbl_buf: LBL_SIZE - 1 at most)
st_osc:
            ldx         v_osci
            cpx         #$FE
            beq         @done
            bcc         @text
            cmp         #';'                                ; Its number
            beq         @semi
            cmp         #'0'
            bcc         @other
            cmp         #'9' + 1
            bcs         @other
            and         #$0F
            pha
            lda         v_oscn
            cmp         #25
            bcs         @big
            asl
            asl
            adc         v_oscn
            asl
            sta         v_oscn
            pla
            adc         v_oscn
            sta         v_oscn
            rts
@big:
            pla
@other:
            lda         #$FE
            sta         v_osci
@done:
            rts
@semi:
            lda         v_oscn                              ; 0 or 2: the label, from its start
            beq         :+
            cmp         #2
            bne         @other
:
            stz         v_osci
            lda         vt_w
            jsr         lbl_at
            lda         #0
            sta         (vt_a)
            jmp         chr_touch
@text:
            cpx         #LBL_SIZE - 1
            bcs         @done
            pha
            lda         vt_w
            jsr         lbl_at
            ldy         v_osci
            pla
            sta         (vt_a),Y
            iny
            lda         #0
            sta         (vt_a),Y
            sty         v_osci
            jmp         chr_touch

; vt_a = window .A's label (lbl_buf)
lbl_at:
            stz         vt_a + 1
            asl
            asl
            asl
            asl
            asl
            rol         vt_a + 1
            clc
            adc         #<lbl_buf
            sta         vt_a
            lda         vt_a + 1
            adc         #>lbl_buf
            sta         vt_a + 1
            rts

; ESC in a string: ST (ESC \) ends it; anything else ends it and starts a sequence
st_stre:
            cmp         #'\'
            bne         :+
            stz         v_state
            rts
:
            pha
            jsr         esc_start
            pla
            jmp         st_esc

; ****************************************************************************
; The C0 controls (.A), and the ESC sequences

c0:
            ldx         v_sasd                              ; (The status line's: CR, BS, BEL)
            beq         :+
            jmp         sl_c0
:
            cmp         #LF
            beq         c_lf
            cmp         #CR
            beq         c_cr
            cmp         #BS
            beq         c_bs
            cmp         #HT
            beq         c_ht
            cmp         #VT
            beq         c_vt
            cmp         #FF
            beq         c_vt
            cmp         #BEL
            beq         c_bel
            cmp         #SO
            beq         c_so
            cmp         #SI
            beq         c_si
            rts                                             ; (NUL, ENQ (W2: the answerback) and the rest: nothing)

; LF: a new line, CR LF (onlcr)
c_lf:
            stz         v_x
            jsr         m_index
            jsr         fc_scrolled
            jsr         fc_move
            lda         #CR
            jsr         fs_byte
            lda         #LF
            jmp         fs_byte

; VT and FF: as LF without its CR (LNM: with it)
c_vt:
            lda         v_mode
            and         #VM_LNM
            beq         :+
            stz         v_x
:
            jsr         m_index
            jsr         fc_scrolled
            jsr         fc_move
            lda         vch
            jmp         fs_byte

c_cr:
            stz         v_x
            stz         v_wrap
            jsr         fc_move
            lda         #CR
            jmp         fs_byte

c_bs:
            stz         v_wrap
            lda         v_x
            beq         :+
            dec         v_x
:
            jsr         fc_move
            lda         #BS
            jmp         fs_byte

c_ht:
            jsr         m_tab
            jsr         fc_move
            lda         #HT
            jmp         fs_byte

c_bel:
            ldx         vt_w                                ; The shown window's: the bell (cons.s rings it); another's
            cpx         w_in                                ;   marked in the chrome (!)
            bne         :+
            lda         #1
            sta         bell
            bra         :++
:
            lda         #ACT_BELL
            jsr         act_mark
:
            lda         #BEL
            jmp         fs_byte

c_so:
            lda         #1
            sta         v_gl
            lda         #SO
            jmp         fs_byte

c_si:
            stz         v_gl
            lda         #SI
            jmp         fs_byte

; An ESC sequence ended: .A its final byte, v_inter its intermediate
esc_do:
            ldx         v_inter
            beq         @plain
            cpx         #'#'
            beq         @hash
            cpx         #'('
            beq         @g0
            cpx         #')'
            beq         @g1
            cpx         #'*'                                ; (G2, G3: W2)
            beq         @pass
            cpx         #'+'
            beq         @pass
            rts                                             ; (Not ours: dropped)

@g0:
            jsr         set_cs
            sta         v_g0
            jmp         fs_raw

@g1:
            jsr         set_cs
            sta         v_g1
            jmp         fs_raw

@pass:
            jmp         fs_raw

@hash:
            cmp         #'8'                                ; DECALN
            bne         :+
            jsr         m_align
            jsr         scr_dirty
            jsr         sc_paint
            jmp         fs_raw
:
            cmp         #'5'                                ; DECSWL: the cursor's row single again
            bne         :+
            lda         #0
            jmp         line_attr
:
            ldx         #RA_DHT                             ; DECDHL (3 the top half, 4 the bottom), DECDWL (6)
            cmp         #'3'
            beq         :+
            ldx         #RA_DHB
            cmp         #'4'
            beq         :+
            ldx         #RA_DW
            cmp         #'6'
            beq         :+
            jmp         fs_raw
:
            txa
            jmp         line_attr

@plain:
            ldx         #ESC_N - 1
:
            cmp         esc_final,X
            beq         :+
            dex
            bpl         :-
            rts                                             ; (Not ours: dropped)
:
            txa
            asl
            tax
            jmp         (esc_vec,X)

; The cursor's row's size .A (RA_DW, RA_DHT, RA_DHB; 0 single): a double one's right half blank, and the cursor
; in its left; the screen painted (it shows a double row's characters a space apart)
line_attr:
            sta         vt_k
            beq         @set
            jsr         cur_vq
            lda         v_cols
            lsr
            tax
            ldy         v_cols
            jsr         blank_span
            lda         v_cols
            lsr
            dec         a
            cmp         v_x
            bcs         @set
            sta         v_x
@set:
            stz         v_wrap
            lda         vb2
            sta         $00
            ldy         #META
            lda         (vrp),Y
            and         #<~RA_LINE
            ora         vt_k
            sta         (vrp),Y
            jsr         scr_dirty
            jmp         fs_raw

; Z = 0 if the cursor's row is a double one (.A its RA_LINE bits)
cur_dw:
            lda         vb2
            sta         $00
            ldy         #META
            lda         (vrp),Y
            and         #RA_LINE
            rts

; .A = vid's column for the cursor: a double row's twice the window's
vid_col:
            jsr         cur_dw
            beq         :+
            lda         v_x
            asl
            rts
:
            lda         v_x
            rts

; A character set designated: .A = its final (B, A, 0; 1 and 2, the alternate ROM's, as B and 0; others B)
set_cs:
            cmp         #'1'
            bne         :+
            lda         #'B'
:
            cmp         #'2'
            bne         :+
            lda         #'0'
:
            cmp         #'A'
            beq         :+
            cmp         #'0'
            beq         :+
            lda         #'B'
:
            rts

e_decsc:
            jsr         m_save
            jsr         fc_lost
            jmp         fs_raw

e_decrc:
            jsr         m_restore
            jsr         fc_lost
            jmp         fs_raw

e_ind:
            jsr         m_index
            jsr         fc_scrolled
            jsr         fc_move
            jmp         fs_raw

e_nel:
            stz         v_x
            jsr         m_index
            jsr         fc_scrolled
            jsr         fc_move
            jmp         fs_raw

e_hts:
            ldx         v_x
            jsr         tab_bit
            ora         v_tabs,Y
            sta         v_tabs,Y
            jmp         fs_raw

e_ri:
            jsr         m_rindex
            bcc         :+
            jsr         fc_scrolled_down
:
            jsr         fc_move
            jmp         fs_raw

e_decid:                                                    ; DECID: as DA
            jmp         answer_da

e_ris:
            jsr         reset
            jsr         scr_dirty
            jsr         sc_paint                            ; (The serial port with chrome: painted)
            jmp         fs_raw

e_deckpam:
            lda         v_mode
            ora         #VM_KPAM
            sta         v_mode
            rts

e_deckpnm:
            lda         v_mode
            and         #<~VM_KPAM
            sta         v_mode
            rts

; ****************************************************************************
; The CSI sequences: .A the final byte, v_priv, v_inter, the numbers

csi_do:
            ldx         v_sasd                              ; (The status line's: its own few)
            beq         :+
            jmp         sl_csi
:
            sta         vt_k
            lda         v_parl                              ; vt_n: the first number, 1 at least (most take it
            ldx         v_parh                              ;   so: a count; 255 at most)
            beq         :+
            lda         #255
:
            cmp         #0
            bne         :+
            lda         #1
:
            sta         vt_n
            lda         vt_k
            ldx         v_inter
            beq         @noint
            cpx         #'$'
            bne         @bang
            cmp         #'p'                                ; DECRQM (CSI ? n $ p, CSI n $ p)
            bne         :+
            jmp         x_decrqm
:
            cmp         #'~'                                ; DECSSDT, DECSASD
            bne         :+
            jmp         x_decssdt
:
            cmp         #'}'
            bne         @drop
            jmp         x_decsasd
@bang:
            cpx         #'!'
            bne         @drop
            cmp         #'p'                                ; DECSTR
            bne         @drop
            ldx         v_priv
            bne         @drop
            jmp         x_decstr

@noint:
            ldx         v_priv
            beq         @plain
            cpx         #'>'
            bne         :+
            cmp         #'c'                                ; The secondary DA
            bne         @drop
            jmp         x_da2
:
            cpx         #'?'
            bne         @drop                               ; (= <: none)
            cmp         #'h'
            beq         x_decset
            cmp         #'l'
            beq         x_decrst
            cmp         #'n'                                ; DECXCPR (CSI ? 6 n)
            bne         :+
            jmp         x_decxcpr
:
            cmp         #'J'                                ; DECSED and DECSEL: as ED and EL (W2: protection)
            beq         @plain
            cmp         #'K'
            beq         @plain
@drop:
            rts

@plain:
            ldx         #CSI_N - 1
:
            cmp         csi_final,X
            beq         :+
            dex
            bpl         :-
            rts                                             ; (Not ours: dropped)
:
            txa
            asl
            tax
            jmp         (csi_vec,X)

; DECSET, DECRST: each number's mode
x_decset:
            lda         #1
            bra         x_dec
x_decrst:
            lda         #0
x_dec:
            sta         vd_set
            stz         vd_i
@mode:
            ldx         vd_i
            lda         v_parh,X
            beq         @small
            cmp         #>2004                              ; (2004: bracketed paste, the console's: its paste's)
            bne         :+
            lda         v_parl,X
            cmp         #<2004
            bne         @next
            lda         #VM2_BPM
            jsr         mode2_bit
            bra         @next
:
            cmp         #>1047                              ; (1047 and 1049: the alternate screen)
            bne         @next
            lda         v_parl,X
            cmp         #<1047
            bne         :+
            jsr         d_alt
            bra         @next
:
            cmp         #<1049
            bne         @next
            jsr         d_alt49
            bra         @next
@small:
            lda         v_parl,X
            ldx         #DECM_N - 1
:
            cmp         decm_n,X
            beq         @found
            dex
            bpl         :-
            bra         @next

@found:
            txa
            asl
            tax
            jsr         @go
@next:
            inc         vd_i
            lda         vd_i
            cmp         v_npar
            bcc         @mode
            beq         @mode
            rts

@go:
            jmp         (decm_vec,X)

; A mode's bit .A in v_mode set (vd_set <> 0) or cleared
mode_bit:
            ldx         vd_set
            beq         :+
            ora         v_mode
            sta         v_mode
            rts
:
            eor         #$FF
            and         v_mode
            sta         v_mode
            rts

; The same, in v_mode2
mode2_bit:
            ldx         vd_set
            beq         :+
            ora         v_mode2
            sta         v_mode2
            rts
:
            eor         #$FF
            and         v_mode2
            sta         v_mode2
            rts

d_ckm:                                                      ; ?1: the console's (it decodes the keys)
            lda         #VM_CKM
            jmp         mode_bit

d_colm:                                                     ; ?3: no 132 columns; the screen cleared, the margins
            jsr         full_margins                        ;   reset, the cursor home (as xterm does)
            jsr         m_home
            lda         #2
            jsr         m_ed
            jsr         ser_dirty
            jmp         scr_dirty

d_sclm:                                                     ; ?4: kept (DECRQM's), not acted on: scroll jump is the
                                                            ;   console's (consctl), as resets send ?4l
            lda         #VM2_SCLM
            jsr         mode2_bit
            jmp         fs_raw

d_scnm:                                                     ; ?5: the screen reversed (the serial port's terminal's
            lda         #VM_SCNM                            ;   own; the screen painted so)
            jsr         mode_bit
            jsr         scr_dirty
            jmp         fs_raw

d_om:                                                       ; ?6: the cursor home (the serial port with chrome: the
            lda         #VM_OM                              ;   console's alone, a CUP there)
            jsr         mode_bit
            jsr         m_home
            jsr         fc_lost
            jsr         sc_cup
            bcs         :+
            jmp         fs_raw
:
            rts

d_awm:                                                      ; ?7
            lda         #VM_AWM
            jsr         mode_bit
            stz         v_wrap
            jsr         fc_lost
            jmp         fs_raw

d_anm:                                                      ; ?2: reset, VT52 mode; set, ANSI (not to the terminals:
            lda         v_mode2                             ;   the PC's stays ANSI, VT52's sequences made ANSI's)
            ora         #VM2_VT52
            ldx         vd_set
            beq         :+
            and         #<~VM2_VT52
:
            sta         v_mode2
            rts

d_alt:                                                      ; ?47, ?1047: the alternate screen (cleared), or the main
            lda         vd_set                              ;   one; painted (not passed on: the console's)
            beq         :+
            jsr         alt_on
            bra         alt_paint
:
            jsr         alt_off
alt_paint:
            jsr         ser_dirty
            jmp         scr_dirty

d_alt49:                                                    ; ?1049: as ?1047, the cursor saved first (DECSC), and
            lda         vd_set                              ;   restored after
            beq         :+
            jsr         m_save
            jsr         alt_on
            bra         alt_paint
:
            jsr         alt_off
            jsr         m_restore
            bra         alt_paint

d_tcem:                                                     ; ?25: the cursor shown, or not (the serial port's as it
            lda         #VM_TCEM                            ;   is now, if it follows)
            jsr         mode_bit
            jsr         fc_tcem
            lda         fw_ser
            beq         :+
            lda         v_mode
            and         #VM_TCEM
            sta         ser_tcem
:
            jmp         fs_raw

; SM, RM: IRM (4) and LNM (20)
x_sm:
            lda         #1
            bra         x_ansi
x_rm:
            lda         #0
x_ansi:
            sta         vd_set
            stz         vd_i
@mode:
            ldx         vd_i
            lda         v_parh,X
            bne         @next
            lda         v_parl,X
            cmp         #4
            bne         :+
            lda         #VM_IRM
            jsr         mode_bit
            bra         @next
:
            cmp         #20
            bne         @next
            lda         #VM_LNM
            jsr         mode_bit
@next:
            inc         vd_i
            lda         vd_i
            cmp         v_npar
            bcc         @mode
            beq         @mode
            jmp         fs_raw

; The cursor moves
x_cuu:
            lda         v_y
            cmp         v_top                               ; (Within the region or below it: no higher than its
            bcs         :+                                  ;   top; above it: than the screen's)
            lda         #0
            bra         @lim
:
            lda         v_top
@lim:
            sta         vt_j
            sec
            lda         v_y
            sbc         vt_n
            bcc         @top
            cmp         vt_j
            bcs         :+
@top:
            lda         vt_j
:
            sta         v_y
            jmp         moved

x_cud:
            lda         v_y                                 ; (Within the region or above it: no lower than its
            cmp         v_bot                               ;   bottom; below it: than the screen's)
            beq         :+
            bcs         @screen
:
            lda         v_bot
            bra         @lim
@screen:
            ldx         v_rows
            dex
            txa
@lim:
            sta         vt_j
            clc
            lda         v_y
            adc         vt_n
            bcs         @bottom
            cmp         vt_j
            bcc         :+
@bottom:
            lda         vt_j
:
            sta         v_y
            jmp         moved

x_cuf:
            clc
            lda         v_x
            adc         vt_n
            bcs         @end
            cmp         v_cols
            bcc         :+
@end:
            ldx         v_cols
            dex
            txa
:
            sta         v_x
            jmp         moved

x_cub:
            sec
            lda         v_x
            sbc         vt_n
            bcs         :+
            lda         #0
:
            sta         v_x
            jmp         moved

x_cnl:
            stz         v_x
            bra         x_cud

x_cpl:
            stz         v_x
            jmp         x_cuu

x_cha:                                                      ; CHA, HPA: a column
            ldx         vt_n
            dex
            txa
            cmp         v_cols
            bcc         :+
            ldx         v_cols
            dex
            txa
:
            sta         v_x
            jmp         moved

x_vpa:                                                      ; VPA: a row (with DECOM, in the region)
            lda         vt_n
            jsr         row_of
            sta         v_y
            jmp         moved

x_vpr:                                                      ; VPR: rows down (not past the screen's bottom)
            clc
            lda         v_y
            adc         vt_n
            bcs         @end
            cmp         v_rows
            bcc         :+
@end:
            ldx         v_rows
            dex
            txa
:
            sta         v_y
            jmp         moved

x_cup:                                                      ; CUP, HVP: row ; column (from 1; DECOM: in the region)
            lda         vt_n
            jsr         row_of
            sta         v_y
            lda         v_parl + 1
            ldx         v_parh + 1
            beq         :+
            lda         #255
:
            cmp         #0
            bne         :+
            lda         #1
:
            sta         vt_n
            bra         x_cha

; .A = the row for number .A (from 1): with DECOM, in the region
row_of:
            dec         a
            sta         vt_j
            lda         v_mode
            and         #VM_OM
            beq         @screen
            clc
            lda         vt_j
            adc         v_top
            bcs         @bottom
            cmp         v_bot
            bcc         @done
            beq         @done
@bottom:
            lda         v_bot
            rts

@screen:
            lda         vt_j
            cmp         v_rows
            bcc         @done
            ldx         v_rows
            dex
            txa
@done:
            rts

; The cursor moved: its row's place, the last-column flag off; the screen's cursor to go there later; the sequence
; to the serial port
moved:
            stz         v_wrap
            jsr         cur_row
            jsr         fc_move
            jsr         sc_cup                              ; (The serial port with chrome: a CUP)
            bcs         :+
            jmp         fs_raw
:
            rts

x_cht:                                                      ; CHT: n tab stops on
:
            jsr         m_tab
            dec         vt_n
            bne         :-
            jmp         moved

x_cbt:                                                      ; CBT: n back
:
            jsr         m_backtab
            dec         vt_n
            bne         :-
            jmp         moved

; ED, EL (DECSED, DECSEL: as them, W1); ED 3, the scrollback alone
x_ed:
            lda         v_parl
            ldx         v_parh
            bne         @done
            cmp         #3
            bcc         :+
            bne         @done
            stz         v_sbn
            jsr         hist_clear
            jmp         fs_raw
:
            pha
            jsr         m_ed
            pla
            jsr         fc_erase
            lda         #'J'
            jsr         fc_erase_end
            jsr         fs_raw
            jmp         sc_erased                           ; (The serial port's chrome: drawn again)
@done:
            rts

x_el:
            lda         v_parl
            ldx         v_parh
            bne         @done
            cmp         #3
            bcs         @done
            pha
            jsr         m_el
            pla
            jsr         fc_erase
            lda         #'K'
            jsr         fc_erase_end
            jmp         fs_raw
@done:
            rts

; IL, DL: lines in at the cursor's row, or out (in the region; the cursor to the left margin)
x_il:
            jsr         in_region
            bcs         @done
            lda         v_y
            ldx         v_bot
            ldy         vt_n
            jsr         scroll_down
            stz         v_x
            stz         v_wrap
            jsr         scr_dirty
            jmp         fs_raw
@done:
            rts

x_dl:
            jsr         in_region
            bcs         @done
            stz         su_keep
            lda         v_y
            ldx         v_bot
            ldy         vt_n
            jsr         scroll_up
            stz         v_x
            stz         v_wrap
            jsr         scr_dirty
            jmp         fs_raw
@done:
            rts

; C = 0 if the cursor's row is in the region
in_region:
            lda         v_y
            cmp         v_top
            bcc         @no
            lda         v_bot
            cmp         v_y
            bcc         @no
            clc
            rts
@no:
            sec
            rts

; ICH, DCH, ECH: characters in at the cursor, out, or erased
x_ich:
            jsr         m_ich
            bra         x_cells

x_dch:
            jsr         m_dch
            bra         x_cells

x_ech:
            jsr         m_ech
x_cells:
            stz         v_wrap
            jsr         scr_dirty
            jmp         fs_raw

; SU, SD: the region scrolled n rows (SU's rows off the top not kept, as xterm's)
x_su:
            stz         su_keep
            lda         v_top
            ldx         v_bot
            ldy         vt_n
            jsr         scroll_up
            jsr         scr_dirty
            jmp         fs_raw

x_sd:
            lda         v_top
            ldx         v_bot
            ldy         vt_n
            jsr         scroll_down
            jsr         scr_dirty
            jmp         fs_raw

; REP: the last character n times more (both terminals painted)
x_rep:
            jsr         ser_dirty
            jsr         scr_dirty
            lda         vt_n
            sta         rep_n
@more:
            lda         v_last
            jsr         print_glyph
            dec         rep_n
            bne         @more
            rts

; DA: the console's answer (as a VT102's), only for 0
x_da:
            lda         v_parl
            ora         v_parh
            bne         :+
            jmp         answer_da
:
            rts

; DSR: 5, the state (all's well); 6, the cursor's place (CPR)
x_dsr:
            lda         v_parh
            bne         @done
            lda         v_parl
            cmp         #5
            beq         @ok
            cmp         #6
            bne         @done
            lda         #0
            jmp         cpr
@ok:
            ldx         #<s_dsr_ok
            ldy         #>s_dsr_ok
            jmp         ans_str
@done:
            rts

; CPR: the cursor's place, ESC [ row ; column R (with DECOM, the row from the region's top); after the [, .A if it
; isn't 0 (DECXCPR's ?)
cpr:
            pha
            lda         #ESC
            jsr         vt_key
            lda         #'['
            jsr         vt_key
            pla
            beq         :+
            jsr         vt_key
:
            lda         v_mode
            and         #VM_OM
            beq         :+
            lda         v_y
            sec
            sbc         v_top
            bra         :++
:
            lda         v_y
:
            inc         a
            jsr         key_dec
            lda         #';'
            jsr         vt_key
            lda         v_x
            inc         a
            jsr         key_dec
            lda         #'R'
            jmp         vt_key

; DECXCPR (CSI ? 6 n): the cursor's place, as CPR with a ?
x_decxcpr:
            lda         v_parh
            bne         @done
            lda         v_parl
            cmp         #6
            bne         @done
            lda         #'?'
            jmp         cpr
@done:
            rts

; The secondary DA (CSI > c): a VT220's, firmware 10
x_da2:
            lda         v_parl
            ora         v_parh
            bne         :+
            ldx         #<s_da2
            ldy         #>s_da2
            jmp         ans_str
:
            rts

; DECREQTPARM (CSI x): 0 or 1, the terminal's parameters (DECREPTPARM: 2 or 3 its reason; no parity, 8 bits, 19200
; each way, the clock 1, no flags)
x_reqtparm:
            lda         v_parh
            bne         @done
            lda         v_parl
            cmp         #2
            bcs         @done
            ora         #'2'
            pha
            lda         #ESC
            jsr         vt_key
            lda         #'['
            jsr         vt_key
            pla
            jsr         vt_key
            ldx         #<s_reptparm
            ldy         #>s_reptparm
            jmp         ans_str
@done:
            rts

; xterm's window reports (CSI t): 18 and 19, the text's size, ESC [ 8 (9) ; rows ; columns t
x_xtwin:
            lda         v_parh
            bne         @done
            lda         v_parl
            cmp         #18
            beq         :+
            cmp         #19
            bne         @done
:
            sec
            sbc         #10
            pha
            lda         #ESC
            jsr         vt_key
            lda         #'['
            jsr         vt_key
            pla
            ora         #'0'
            jsr         vt_key
            lda         #';'
            jsr         vt_key
            lda         v_rows
            jsr         key_dec
            lda         #';'
            jsr         vt_key
            lda         v_cols
            jsr         key_dec
            lda         #'t'
            jmp         vt_key
@done:
            rts

; DECLL: the LEDs (0 all off, 1-4 one on; 21-24, one off)
x_decll:
            stz         vd_i
@next:
            ldx         vd_i
            lda         v_parh,X
            bne         @skip
            lda         v_parl,X
            bne         :+
            stz         v_leds
            bra         @skip
:
            cmp         #5
            bcs         :+
            tax
            lda         bits - 1,X
            ora         v_leds
            sta         v_leds
            bra         @skip
:
            sec
            sbc         #21
            cmp         #4
            bcs         @skip
            tax
            lda         bits,X
            eor         #$FF
            and         v_leds
            sta         v_leds
@skip:
            inc         vd_i
            lda         vd_i
            cmp         v_npar
            bcc         @next
            beq         @next
            rts

; DECRQM (CSI ? n $ p, CSI n $ p): a mode's state, ESC [ (?) n ; s $ y (s: 1 set, 2 reset, 3 always set, 4 always
; reset, 0 not one known)
x_decrqm:
            jsr         rqm_state
            pha
            lda         #ESC
            jsr         vt_key
            lda         #'['
            jsr         vt_key
            lda         v_priv
            beq         :+
            jsr         vt_key
:
            lda         v_parl
            ldx         v_parh
            jsr         key_dec16
            lda         #';'
            jsr         vt_key
            pla
            ora         #'0'
            jsr         vt_key
            lda         #'$'
            jsr         vt_key
            lda         #'y'
            jmp         vt_key

; .A = DECRQM's state of the mode asked for (v_parl, v_parh; v_priv: a DEC private one)
rqm_state:
            lda         v_parh
            bne         @unknown
            lda         v_parl
            ldx         v_priv
            beq         @ansi
            ldx         #RQM_N - 1                          ; A private mode: one of the table's
:
            cmp         rqm_n,X
            beq         :+
            dex
            bpl         :-
            bra         @unknown
:
            lda         rqm_bit,X
            beq         @fixed
            cpx         #RQM_SCLM
            beq         @sclm
            and         v_mode
            bra         @set
@sclm:
            and         v_mode2
            bra         @set
@fixed:
            lda         rqm_fixed,X
            rts
@ansi:
            cmp         #4                                  ; IRM, LNM; KAM always reset
            bne         :+
            lda         #VM_IRM
            and         v_mode
            bra         @set
:
            cmp         #20
            bne         :+
            lda         #VM_LNM
            and         v_mode
            bra         @set
:
            cmp         #2
            bne         @unknown
            lda         #4
            rts
@unknown:
            lda         #0
            rts
@set:
            beq         :+
            lda         #1
            rts
:
            lda         #2
            rts

; The zero-ended string at .X/.Y into the window's keys
ans_str:
            stx         vt_a
            sty         vt_a + 1
            ldy         #0
:
            lda         (vt_a),Y
            beq         :+
            jsr         vt_key
            iny
            bra         :-
:
            rts

; TBC: 0 the stop at the cursor cleared, 3 all of them
x_tbc:
            lda         v_parh
            bne         @done
            lda         v_parl
            bne         :+
            ldx         v_x
            jsr         tab_bit
            eor         #$FF
            and         v_tabs,Y
            sta         v_tabs,Y
            jmp         fs_raw
:
            cmp         #3
            bne         @done
            ldx         #15
:
            stz         v_tabs,X
            dex
            bpl         :-
            jmp         fs_raw
@done:
            rts

; DECSTBM: the scrolling region, rows t to b (from 1; none: the whole screen), if t < b; the cursor home
x_stbm:
            lda         v_parh
            ora         v_parh + 1
            bne         @done
            lda         v_parl                              ; t (0: 1)
            bne         :+
            lda         #1
:
            sta         vt_i
            lda         v_parl + 1                          ; b (0: the last row)
            bne         :+
            lda         v_rows
:
            cmp         v_rows
            beq         :+
            bcs         @done
:
            sta         vt_j
            lda         vt_i
            cmp         vt_j
            bcs         @done
            dec         a
            sta         v_top
            ldx         vt_j
            dex
            stx         v_bot
            jsr         m_home
            jsr         fc_region
            jsr         sc_region                           ; (The serial port with chrome: offset)
            bcs         @done
            jmp         fs_raw
@done:
            rts

; DECSTR: a soft reset (not the screen, nor the cursor's place)
x_decstr:
            jsr         soft
            jsr         fc_region
            jsr         fs_raw
            jsr         sc_erased                           ; (The serial port with chrome: its margins again)
            jsr         sc_region
            rts

; SCOSC, SCORC (CSI s, u): as DECSC, DECRC
x_scosc:
            jmp         e_decsc

x_scorc:
            jmp         e_decrc

; SGR: the rendition, a number at a time
x_sgr:
            stz         vt_i
@next:
            ldx         vt_i
            cpx         v_npar
            beq         :+
            bcs         @done
:
            lda         v_parh,X
            bne         @skip
            lda         v_parl,X
            jsr         sgr1
@skip:
            inc         vt_i
            bra         @next
@done:
            jmp         fs_raw

; One SGR number, .A (38 and 48 take the numbers after them: vt_i moved on)
sgr1:
            cmp         #0
            bne         :+
            lda         #COL_DEF
            sta         v_col
            stz         v_fl
            rts
:
            cmp         #10
            bcs         @twenty
            tax
            lda         sgr_on,X
            ora         v_fl
            sta         v_fl
            rts

@twenty:
            cmp         #21
            bne         :+
            lda         #F_UL                               ; (21: double underline, as underline)
            ora         v_fl
            sta         v_fl
            rts
:
            cmp         #30
            bcs         @fg
            cmp         #22
            bcc         @done
            sec
            sbc         #22
            tax
            lda         sgr_off,X
            and         v_fl
            sta         v_fl
@done:
            rts

@fg:
            cmp         #38
            bcs         :+
            sbc         #30 - 1                             ; (C = 0)
            bra         @setfg
:
            beq         @fgx
            cmp         #39
            bne         @bg
            lda         #COL_DEF & $0F
@setfg:
            sta         vt_j
            lda         v_col
            and         #$F0
            ora         vt_j
            sta         v_col
            rts

@fgx:
            jsr         sgr_x
            bcc         @setfg
            rts

@bg:
            cmp         #48
            bcs         :+
            cmp         #40
            bcc         @done
            sbc         #40                                 ; (C = 1)
            bra         @setbg
:
            beq         @bgx
            cmp         #49
            bne         @bright
            lda         #COL_DEF >> 4
@setbg:
            asl
            asl
            asl
            asl
            sta         vt_j
            lda         v_col
            and         #$0F
            ora         vt_j
            sta         v_col
            rts

@bgx:
            jsr         sgr_x
            bcc         @setbg
            rts

@bright:
            cmp         #90
            bcc         @done
            cmp         #98
            bcs         :+
            sbc         #90 - 8 - 1                         ; (C = 0: 90-97 as 8-15)
            bra         @setfg
:
            cmp         #100
            bcc         @done
            cmp         #108
            bcs         @done
            sbc         #100 - 8                            ; (C = 1)
            bra         @setbg

; 38 or 48's colour: 5;n (xterm's 256) or 2;r;g;b, as the nearest of the 16.  OUT: C = 0, .A = it; or C = 1
sgr_x:
            inc         vt_i
            ldx         vt_i
            lda         v_parl,X
            inc         vt_i
            cmp         #5
            beq         @x256
            cmp         #2
            beq         :+
            sec                                             ; (Neither: none)
            rts
:
            ldx         vt_i                                ; r, g, b
            lda         v_parl,X
            sta         vt_a                                ; (r)
            lda         v_parl + 1,X
            sta         vt_a + 1                            ; (g)
            lda         v_parl + 2,X
            sta         vt_k                                ; (b)
            inc         vt_i
            inc         vt_i
            stz         vt_j                                ; (the colour's bits)
            stz         vt_n                                ; (the most of the three)
            lda         vt_a
            ldy         #1
            jsr         @level
            lda         vt_a + 1
            ldy         #2
            jsr         @level
            lda         vt_k
            ldy         #4
            jsr         @level
            lda         vt_n                                ; Bright, if any is past 191 (and it's a colour)
            cmp         #192
            bcc         :+
            lda         vt_j
            ora         #8
            clc
            rts
:
            lda         vt_j
            clc
            rts

@level:                                                     ; .A, a component: past 127, its bit .Y
            cmp         vt_n
            bcc         :+
            sta         vt_n
:
            cmp         #128
            bcc         :+
            tya
            ora         vt_j
            sta         vt_j
:
            rts

@x256:
            ldx         vt_i
            lda         v_parl,X
            cmp         #16
            bcc         @ok                                 ; 0-15: as they are
            cmp         #232
            bcs         @grey
            sbc         #16 - 1                             ; 16-231: r * 36 + g * 6 + b, each 0-5 (C = 0)
            ldx         #0                                  ; (r: the 36s)
:
            cmp         #36
            bcc         :+
            sbc         #36
            inx
            bra         :-
:
            stx         vt_a
            ldx         #0                                  ; (g: the 6s; b: the rest)
:
            cmp         #6
            bcc         :+
            sbc         #6
            inx
            bra         :-
:
            sta         vt_k
            stx         vt_a + 1
            stz         vt_j
            stz         vt_n
            lda         vt_a
            ldy         #1
            jsr         @level6
            lda         vt_a + 1
            ldy         #2
            jsr         @level6
            lda         vt_k
            ldy         #4
            jsr         @level6
            lda         vt_n                                ; (Bright: any at 5)
            cmp         #5
            bcc         :+
            lda         vt_j
            ora         #8
            bra         @ok
:
            lda         vt_j
@ok:
            clc
            rts

@level6:                                                    ; 0-5: past 2, its bit .Y
            cmp         vt_n
            bcc         :+
            sta         vt_n
:
            cmp         #3
            bcc         :+
            tya
            ora         vt_j
            sta         vt_j
:
            rts

@grey:                                                      ; 232-255: black, dark grey, grey, white
            sbc         #232                                ; (C = 1)
            lsr
            lsr
            tax
            lda         greys,X
            bra         @ok

@none:
            sec
            rts

; ****************************************************************************
; The window's cells: characters written, the cursor moved, scrolls, erasing

; A character (.A, as it came): its glyph through the character set in use (UK: # as £; the DEC graphics: $5F-$7E
; as $00-$1F), written
print:
            ldx         v_gl
            lda         v_g0,X
            cmp         #'0'
            beq         @dec
            cmp         #'A'
            bne         @plain
            lda         vch
            cmp         #'#'
            bne         print_glyph
            lda         #$A3
            bra         print_glyph
@dec:
            lda         vch
            cmp         #$5F
            bcc         print_glyph
            cmp         #$7F
            bcs         print_glyph
            sbc         #$5F - 1                            ; (C = 0)
            bra         print_glyph
@plain:
            lda         vch

; Glyph .A written at the cursor (it past the last column: a new line first, the last-column flag's, the row it's
; on marked continued; IRM: the rest of the row moved right), and the cursor on; then to the terminals (the serial
; port: the byte as it came).  A cell at the row's blank end moves the end on (the cells before it written blank)
print_glyph:
            ldx         v_sasd                              ; (Into the status line: its own)
            beq         :+
            jmp         sl_glyph
:
            sta         v_last
            stz         vt_scr
            lda         v_wrap
            beq         @nowrap
            stz         v_wrap
            stz         v_x
            jsr         m_index
            php
            lda         vb2                                 ; (The row it's on: continued)
            sta         $00
            ldy         #META
            lda         (vrp),Y
            ora         #RA_CONT
            sta         (vrp),Y
            plp
            bcc         @nowrap
            inc         vt_scr                              ; (Scrolled: the screen with it)
@nowrap:
            lda         v_mode
            and         #VM_IRM
            beq         @put
            lda         #1
            sta         vt_n
            jsr         m_ich
            jsr         scr_dirty
@put:
            jsr         cur_dw                              ; The row's width: a double row's half
            php
            lda         v_cols
            plp
            beq         :+
            lsr
:
            sta         pg_w
            dec         a                                   ; (The cursor in it)
            cmp         v_x
            bcs         :+
            sta         v_x
:
            lda         fw_scr                              ; To the screen first, from where the cursor is now
            beq         :+
            jsr         fc_print
:
            lda         vb0                                 ; The row's blank end: at the cursor, before it, or
            sta         $00                                 ;   after it?
            ldy         #META
            lda         (vrp),Y
            cmp         v_x
            beq         @end
            bcs         @cell
            jsr         cur_vq                              ; (Before: the cells between written blank)
            lda         v_x
            jsr         real_to
            lda         vb0
            sta         $00
@end:
            ldy         #META                               ; (It's one on)
            lda         v_x
            inc         a
            sta         (vrp),Y
@cell:
            ldy         v_x
            lda         v_last
            sta         (vrp),Y
            lda         vb1
            sta         $00
            lda         v_col
            sta         (vrp),Y
            lda         vb2
            sta         $00
            lda         v_fl
            sta         (vrp),Y
            iny                                             ; The cursor on (the last column: the flag)
            cpy         pg_w
            bcs         @last
            sty         v_x
            bra         @out
@last:
            lda         v_mode
            and         #VM_AWM
            beq         @out
            lda         #1
            sta         v_wrap
@out:
            lda         vch
            jmp         fs_byte

; IND (and LF's, the wrap's): down a row; at the region's bottom, the region scrolled.  OUT: C = 1 scrolled
m_index:
            stz         v_wrap
            lda         v_y
            cmp         v_bot
            bne         @down
            lda         v_alt                               ; (Kept: the main screen's, not the alternate's)
            eor         #1
            sta         su_keep
            lda         v_top
            ldx         v_bot
            ldy         #1
            jsr         scroll_up
            sec
            rts
@down:
            inc         a
            cmp         v_rows
            bcs         :+
            sta         v_y
            jsr         cur_row
:
            clc
            rts

; RI: up a row; at the region's top, the region scrolled down.  OUT: C = 1 scrolled
m_rindex:
            stz         v_wrap
            lda         v_y
            cmp         v_top
            bne         @up
            ldx         v_bot
            ldy         #1
            jsr         scroll_down
            sec
            rts
@up:
            cmp         #0
            beq         :+
            dec         v_y
            jsr         cur_row
:
            clc
            rts

; The cursor home (with DECOM: the region's top)
m_home:
            stz         v_x
            stz         v_wrap
            lda         v_mode
            and         #VM_OM
            beq         :+
            lda         v_top
:
            sta         v_y
            jmp         cur_row

; HT: the next tab stop (none: the last column)
m_tab:
            stz         v_wrap
@next:
            ldx         v_x
            inx
            cpx         v_cols
            bcs         @last
            stx         v_x
            jsr         tab_bit
            and         v_tabs,Y
            beq         @next
            rts
@last:
            ldx         v_cols
            dex
            stx         v_x
            rts

; CBT's: the tab stop before (none: the first column)
m_backtab:
            stz         v_wrap
@back:
            ldx         v_x
            beq         @done
            dex
            stx         v_x
            jsr         tab_bit
            and         v_tabs,Y
            beq         @back
@done:
            rts

; .A = column .X's bit in v_tabs, .Y its byte.  Keeps .X
tab_bit:
            txa
            lsr
            lsr
            lsr
            tay
            txa
            and         #7
            phx
            tax
            lda         bits,X
            plx
            rts

; ED .A: 0 from the cursor to the end, 1 from the start to the cursor, 2 all (3: all, and the scrollback)
m_ed:
            cmp         #1
            beq         @start
            bcs         @all
            jsr         m_el                                ; (.A = 0: the row's rest)
            ldx         v_y
            inx
            stx         vt_i
            ldx         v_rows
            bra         @rows
@start:
            jsr         m_el                                ; (.A = 1: the row to the cursor)
            stz         vt_i
            ldx         v_y
            bra         @rows
@all:
            cmp         #3
            bne         :+
            stz         v_sbn
            jsr         hist_clear
:
            stz         vt_i
            ldx         v_rows
@rows:                                                      ; Rows vt_i to .X - 1, blank
            stx         vt_j
@row:
            lda         vt_i
            cmp         vt_j
            bcs         @done
            jsr         row_ptr
            jsr         blank_row
            inc         vt_i
            bra         @row
@done:
            jmp         cur_row

; EL .A: 0 from the cursor to the end of its row, 1 from the start to the cursor, 2 all of it
m_el:
            pha
            jsr         cur_vq
            pla
            cmp         #1
            beq         @start
            bcs         @all
            ldx         v_x
            ldy         v_cols
            jmp         blank_span
@start:
            ldx         #0
            ldy         v_x
            iny
            jmp         blank_span
@all:
            jmp         blank_row

; ICH vt_n: blanks in at the cursor, the rest of its row right (off the end)
m_ich:
            jsr         count_rest                          ; (vt_n: no more than the cells from the cursor on)
            jsr         cur_vq
            lda         v_cols                              ; (The row's blank end written: its cells move)
            jsr         real_to
            sec                                             ; vr: the row less n (a cell's source, n before it)
            lda         vrp
            sbc         vt_n
            sta         vr
            lda         vrp + 1
            sbc         #0
            sta         vr + 1
            clc
            lda         v_x
            adc         vt_n
            sta         vt_j                                ; (The first moved: from the end down to it)
            lda         vb0
            jsr         @move
            lda         vb1
            jsr         @move
            lda         vb2
            jsr         @move
            ldx         v_x
            ldy         vt_j
            jmp         blank_span

@move:
            sta         $00
            ldy         v_cols
@cell:
            dey
            bmi         @done
            cpy         vt_j
            bcc         @done
            lda         (vr),Y
            sta         (vrp),Y
            bra         @cell
@done:
            rts

; DCH vt_n: the cells at the cursor out, the rest of its row left (blanks at the end)
m_dch:
            jsr         count_rest
            jsr         cur_vq
            lda         v_cols
            jsr         real_to
            clc                                             ; vr: the row plus n
            lda         vrp
            adc         vt_n
            sta         vr
            lda         vrp + 1
            adc         #0
            sta         vr + 1
            sec
            lda         v_cols
            sbc         vt_n
            sta         vt_j                                ; (The cells moved: up to here)
            lda         vb0
            jsr         @move
            lda         vb1
            jsr         @move
            lda         vb2
            jsr         @move
            ldx         vt_j
            ldy         v_cols
            jmp         blank_span

@move:
            sta         $00
            ldy         v_x
@cell:
            cpy         vt_j
            bcs         @done
            lda         (vr),Y
            sta         (vrp),Y
            iny
            bra         @cell
@done:
            rts

; ECH vt_n: the cells at the cursor erased
m_ech:
            jsr         count_rest
            jsr         cur_vq
            clc
            lda         v_x
            adc         vt_n
            tay
            ldx         v_x
            jmp         blank_span

; vt_n: no more than the cells from the cursor to the row's end
count_rest:
            sec
            lda         v_cols
            sbc         v_x
            cmp         vt_n
            bcs         :+
            sta         vt_n
:
            rts

; DECALN: every cell E, the margins reset, the cursor home
m_align:
            jsr         full_margins
            stz         vt_i
@row:
            lda         vt_i
            cmp         v_rows
            bcs         @done
            jsr         row_ptr
            lda         vb0
            ldx         #'E'
            jsr         @fill
            ldy         #META                               ; (No blank end)
            lda         v_cols
            sta         (vq),Y
            lda         vb1
            ldx         #COL_DEF
            jsr         @fill
            lda         vb2
            ldx         #0
            jsr         @fill
            ldy         #META
            lda         #0
            sta         (vq),Y
            inc         vt_i
            bra         @row
@done:
            jmp         m_home

@fill:
            sta         $00
            txa
            ldy         v_cols
:
            dey
            bmi         :+
            sta         (vq),Y
            bra         :-
:
            rts

; DECSC, DECRC: the cursor saved, restored
m_save:
            lda         v_x
            sta         v_sx
            lda         v_y
            sta         v_sy
            lda         v_col
            sta         v_scol
            lda         v_fl
            sta         v_sfl
            lda         v_g0
            sta         v_sg0
            lda         v_g1
            sta         v_sg1
            lda         v_gl
            sta         v_sgl
            lda         v_wrap
            sta         v_swrap
            lda         v_mode
            and         #VM_OM
            sta         v_som
            rts

m_restore:
            lda         v_sx
            sta         v_x
            lda         v_sy
            sta         v_y
            lda         v_scol
            sta         v_col
            lda         v_sfl
            sta         v_fl
            lda         v_sg0
            sta         v_g0
            lda         v_sg1
            sta         v_g1
            lda         v_sgl
            sta         v_gl
            lda         v_swrap
            sta         v_wrap
            lda         v_mode
            and         #<~VM_OM
            ora         v_som
            sta         v_mode
            jmp         cur_row

; The alternate screen in use, cleared: its banks the first time (three more of the task's; none to be had: the main
; screen still), its map each row its own, no scrollback
alt_on:
            lda         v_alt
            bne         @done
            ldx         vt_w
            lda         vw_abank,X
            bne         @have
            lda         #3
            jsr         BANKS_ALLOC
            bcs         @done
            ldx         vt_w
            sta         vw_abank,X
            txa                                             ; (Its ring at the start, no scrollback)
            asl
            tax
            stz         vw_rb + 1,X
            stz         vw_sb + 1,X
            lda         #1                                  ; Its map
            jsr         buf_to
            ldy         #POOL - 1
:
            tya
            sta         (vmap),Y
            dey
            bpl         :-
            bra         @clear
@have:
            lda         #1
            jsr         buf_to
@clear:
            lda         #2
            jsr         m_ed
            stz         v_sbn
@done:
            rts

; The main screen in use again
alt_off:
            lda         #0
            ; (falls into buf_to)

; Screen .A in use (0 main, 1 alternate): the one in use's ring and scrollback kept, the other's taken; its banks
; and map
buf_to:
            cmp         v_alt
            beq         @done
            pha
            lda         vt_w                                ; (The one in use's place: the window * 2 + v_alt)
            asl
            ora         v_alt
            tax
            lda         v_rbase
            sta         vw_rb,X
            lda         v_sbn
            sta         vw_sb,X
            txa
            eor         #1
            tax
            lda         vw_rb,X
            sta         v_rbase
            lda         vw_sb,X
            sta         v_sbn
            pla
            sta         v_alt
            jsr         banks
            jmp         cur_row
@done:
            rts

; The margins the whole screen
full_margins:
            stz         v_top
            ldx         v_rows
            dex
            stx         v_bot
            rts

; A soft reset (DECSTR): the modes, the margins, the rendition, the character sets, the saved cursor
soft:
            stz         v_sasd                              ; (The main display's output)
            lda         #VM_AWM | VM_TCEM
            sta         v_mode
            stz         v_mode2
            jsr         full_margins
            lda         #COL_DEF
            sta         v_col
            stz         v_fl
            lda         #'B'
            sta         v_g0
            sta         v_g1
            stz         v_gl
            stz         v_wrap
            stz         v_sx                                ; (The cursor saved: home, the rest as now)
            stz         v_sy
            jmp         m_save_rest

; The saved cursor's rendition and sets as they are now (soft's)
m_save_rest:
            lda         v_col
            sta         v_scol
            lda         v_fl
            sta         v_sfl
            lda         #'B'
            sta         v_sg0
            sta         v_sg1
            stz         v_sgl
            stz         v_swrap
            stz         v_som
            rts

; RIS (and a new window): all reset, the tab stops every 8, the screen and the scrollback cleared (as xterm's), the
; cursor home
reset:
            lda         #0                                  ; (The main screen)
            jsr         buf_to
            stz         v_ssdt                              ; (The status line: none, empty)
            stz         v_stx
            lda         vt_w
            jsr         stat_ptr
            lda         #0
            sta         (vt_a)
            jsr         chr_touch
            stz         v_state
            stz         v_rawn
            stz         v_sbn
            jsr         hist_clear
            jsr         soft
            ldx         #15                                 ; The tab stops: 8, 16 ...
:
            lda         #1
            sta         v_tabs,X
            dex
            bpl         :-
            stz         v_tabs
            stz         v_x
            stz         v_y
            stz         v_last
            lda         #2
            jmp         m_ed

; ****************************************************************************
; Scrolls: the map's entries moved (the whole map's: its ring turned), a row blanked as it comes in (its blank end
; the whole row: a byte)

; Rows .A to .X (the screen's, from 0) up .Y rows (no more than they are): the top row of them out (su_keep, and the
; region at the screen's top: into the scrollback), a blank one in at the bottom.  The cursor's row's place found
; again
scroll_up:
            sta         su_t
            stx         su_b
            jsr         scroll_n
            lda         su_t                                ; The first entry moved: the scrollback's oldest, if
            bne         :+                                  ;   the rows off the top are kept
            lda         su_keep
            beq         :+
            lda         #0
            bra         @first
:
            lda         su_t
            clc
            adc         v_sb0
@first:
            sta         su_f
            lda         su_b
            clc
            adc         v_sb0
            sta         su_l
@one:
            lda         su_f                                ; All of the map: its ring turned
            bne         @move
            lda         su_l
            cmp         #POOL - 1
            bne         @move
            lda         v_rbase
            inc         a
            and         #POOL - 1
            sta         v_rbase
            bra         @in
@move:
            lda         su_f                                ; Else its entries moved: the first round to the last
            sta         su_i
            jsr         map_row
            sta         su_r
@step:
            lda         su_i
            cmp         su_l
            beq         @last
            inc         a
            jsr         map_y
            lda         (vmap),Y
            pha
            lda         su_i
            jsr         map_y
            pla
            sta         (vmap),Y
            inc         su_i
            bra         @step
@last:
            jsr         map_y
            lda         su_r
            sta         (vmap),Y
@in:
            lda         su_l                                ; The row in at the bottom, blank
            jsr         map_row
            jsr         pool_ptr
            lda         su_f                                ; (Into the scrollback: it's a row longer; or, full,
            bne         @blank                              ;   its oldest, the row going round, into the history,
            lda         v_sbn                               ;   or gone: counted, for the view)
            cmp         v_sb0
            bcs         @full
            inc         v_sbn
            bra         @blank
@full:
            jsr         hist_put
@blank:
            jsr         blank_row
            dec         su_n
            bne         @one
            jmp         cur_row

; Rows .A to .X down .Y rows: a blank one in at the top, the bottom one out
scroll_down:
            sta         su_t
            stx         su_b
            jsr         scroll_n
            clc
            lda         su_t
            adc         v_sb0
            sta         su_f
            clc
            lda         su_b
            adc         v_sb0
            sta         su_l
@one:
            lda         su_l                                ; The last round to the first
            sta         su_i
            jsr         map_row
            sta         su_r
@step:
            lda         su_i
            cmp         su_f
            beq         @first
            dec         a
            jsr         map_y
            lda         (vmap),Y
            pha
            lda         su_i
            jsr         map_y
            pla
            sta         (vmap),Y
            dec         su_i
            bra         @step
@first:
            jsr         map_y
            lda         su_r
            sta         (vmap),Y
            jsr         pool_ptr
            jsr         blank_row
            dec         su_n
            bne         @one
            jmp         cur_row

; The screen in use from rz_o rows to rz_n, .A its cursor's row (OUT: .A, that row's place now): growing, the
; scrollback's newest rows come down onto its top (as many as it has, the cursor's row going down with them), then
; blank rows at its bottom (the ring turned on); shrinking, the rows above the cursor's go off its top into the
; scrollback (as many as must, for the cursor's to stay), then those at its bottom are dropped (the ring turned back)
ring_resize:
            sta         rz_y
            lda         rz_n
            cmp         rz_o
            bne         :+
            lda         rz_y                                ; (As it was)
            rts
:
            bcc         @shrink
            sbc         rz_o                                ; Growing: by g (C = 1)
            sta         rz_d
            lda         v_sbn                               ; k: the scrollback's, g at most
            cmp         rz_d
            bcc         :+
            lda         rz_d
:
            sta         rz_k
            sec
            lda         v_sbn
            sbc         rz_k
            sta         v_sbn
            clc
            lda         rz_y
            adc         rz_k
            sta         rz_y
            sec                                             ; g - k blank at the bottom
            lda         rz_d
            sbc         rz_k
            beq         @grown
            sta         rz_d
            clc
            adc         v_rbase
            and         #POOL - 1
            sta         v_rbase
@blank:
            sec
            lda         #POOL
            sbc         rz_d
            jsr         map_row
            jsr         pool_ptr
            lda         #COL_DEF
            jsr         blank_in
            dec         rz_d
            bne         @blank
@grown:
            lda         rz_y
            rts
@shrink:
            stz         rz_k                                ; Shrinking: k, the rows above the cursor's that go
            lda         rz_y
            cmp         rz_n
            bcc         :+
            sbc         rz_n                                ; (C = 1)
            inc         a
            sta         rz_k
:
            sec
            lda         rz_y
            sbc         rz_k
            sta         rz_y
            clc
            lda         v_sbn
            adc         rz_k
            sta         v_sbn
            sec                                             ; The rest dropped at the bottom
            lda         rz_o
            sbc         rz_n
            sec
            sbc         rz_k
            sta         rz_d
            sec
            lda         v_rbase
            sbc         rz_d
            and         #POOL - 1
            sta         v_rbase
@done:
            lda         rz_y
            rts

; The screen in use's rows (the pool's, all) no wider than lay_cols, if that's narrower: their blank ends no further
; on
trunc_cols:
            lda         lay_cols
            cmp         v_cols
            bcs         @done
            lda         vb0
            sta         $00
            ldx         #POOL - 1
@row:
            txa
            jsr         pool_ptr
            ldy         #META
            lda         lay_cols
            cmp         (vq),Y
            bcs         :+
            sta         (vq),Y
:
            dex
            bpl         @row
@done:
            rts

; su_n = .Y, no more than the rows su_t to su_b (and at least 1)
scroll_n:
            sec
            lda         su_b
            sbc         su_t
            inc         a
            sty         su_n
            cmp         su_n
            bcs         :+
            sta         su_n
:
            lda         su_n
            bne         :+
            inc         su_n
:
            rts

; ****************************************************************************
; Rows and cells (a row's meta: META)

; vrp: the cursor's row
cur_row:
            lda         v_y
            jsr         row_ptr
            lda         vq
            sta         vrp
            lda         vq + 1
            sta         vrp + 1
            rts

; vq: the cursor's row
cur_vq:
            lda         vrp
            sta         vq
            lda         vrp + 1
            sta         vq + 1
            rts

; .Y = the map's entry for its place .A (from the ring's start)
map_y:
            clc
            adc         v_rbase
            and         #POOL - 1
            tay
            rts

; .A = the pool's row at the map's place .A.  Modifies .Y
map_row:
            jsr         map_y
            lda         (vmap),Y
            rts

; vq: the screen's row .A
row_ptr:
            clc
            adc         v_sb0
            jsr         map_row

; vq: the pool's row .A ($8000 + 128 * it)
pool_ptr:
            lsr
            ora         #$80
            sta         vq + 1
            lda         #0
            ror
            sta         vq
            rts

; .A = the colours a cell is erased to: the background's (BCE), the foreground's at first
erase_col:
            lda         v_col
            and         #$F0
            ora         #COL_DEF & $0F
            rts

; The row at vq blank: its blank end all of it, in the background's colours (BCE; blank_in: .A's); no attributes
blank_row:
            jsr         erase_col
blank_in:
            ldy         #META
            ldx         vb1
            stx         $00
            sta         (vq),Y
            lda         vb0
            sta         $00
            lda         #0
            sta         (vq),Y
            lda         vb2
            sta         $00
            lda         #0
            sta         (vq),Y
            rts

; Columns .X to .Y - 1 of the row at vq erased (spaces, the background's colours, no rendition).  To the row's end:
; its blank end from .X (the cells before .X from where the end was written blank in its colours first); else the
; cells written
blank_span:
            stx         bs_a
            sty         bs_b
            cpy         v_cols
            bcc         @mid
            txa                                             ; (To the row's end)
            jsr         real_to
            ldy         #META
            lda         vb0
            sta         $00
            lda         bs_a
            sta         (vq),Y
            lda         vb1
            sta         $00
            jsr         erase_col
            sta         (vq),Y
            rts
@mid:
            tya
            jsr         real_to
            lda         bs_a
            sta         bs_f
            lda         bs_b
            sta         bs_t
            jsr         erase_col
            sta         bs_c
            jmp         fill3

; The row at vq's blank end written to column .A, if it starts before it: blanks in its colours
real_to:
            sta         bs_t
            lda         vb0
            sta         $00
            ldy         #META
            lda         (vq),Y
            cmp         bs_t
            bcs         @done
            sta         bs_f
            lda         bs_t
            sta         (vq),Y
            lda         vb1
            sta         $00
            lda         (vq),Y
            sta         bs_c
            jmp         fill3
@done:
            rts

; Columns bs_f to bs_t - 1 of the row at vq written: spaces, colours bs_c, no rendition
fill3:
            lda         vb0
            sta         $00
            lda         #' '
            jsr         @fill
            lda         vb1
            sta         $00
            lda         bs_c
            jsr         @fill
            lda         vb2
            sta         $00
            lda         #0
@fill:
            ldy         bs_f
:
            cpy         bs_t
            bcs         :+
            sta         (vq),Y
            iny
            bra         :-
:
            rts

; Cell .Y of the row at vq: cell_c, cell_a, cell_f (in its blank end: a space, the end's colours).  Keeps .Y
cell_get:
            sty         cg_y
            lda         vb0
            sta         $00
            ldy         #META
            lda         (vq),Y
            ldy         cg_y
            cmp         cg_y
            beq         @blank
            bcc         @blank
            lda         (vq),Y
            sta         cell_c
            lda         vb1
            sta         $00
            lda         (vq),Y
            sta         cell_a
            lda         vb2
            sta         $00
            lda         (vq),Y
            sta         cell_f
            rts
@blank:
            lda         #' '
            sta         cell_c
            lda         vb1
            sta         $00
            ldy         #META
            lda         (vq),Y
            sta         cell_a
            stz         cell_f
            ldy         cg_y
            rts

; .A = the cells of the row at vq to paint: to its last that isn't blank (a space, the colours at first, no
; rendition); a blank end in other colours: all of them
row_last:
            jsr         row_chars                           ; (The characters' end (the characters' plane selected),
            sta         vt_j                                ;   the blank end ...
            ldy         #META
            lda         (vq),Y
            cmp         v_cols
            bcc         :+
            lda         v_cols
:
            sta         rl_bf
            lda         vb1                                 ;   in colours: all of them)
            sta         $00
            lda         rl_bf
            cmp         v_cols
            bcs         :+
            lda         (vq),Y
            cmp         #COL_DEF
            beq         :+
            lda         v_cols
            rts
:
            ldy         rl_bf                               ; The colours' end, before the blank end
:
            dey
            bmi         :+
            cpy         vt_j
            bcc         :+
            lda         (vq),Y
            cmp         #COL_DEF
            beq         :-
            iny
            sty         vt_j
:
            lda         vb2                                 ; The rendition's
            sta         $00
            ldy         rl_bf
:
            dey
            bmi         :+
            cpy         vt_j
            bcc         :+
            lda         (vq),Y
            beq         :-
            iny
            sty         vt_j
:
            lda         vt_j
            rts

; ****************************************************************************
; The serial port: following, and painted

; .A to the serial port, if it's following
fs_byte:
            ldx         fw_ser
            beq         :+
            jmp         tx_put
:
            rts

; The sequence just ended to the serial port, as it came (if it's following; one too long to keep: painted)
fs_raw:
            lda         fw_ser
            beq         @done
            lda         v_rawn
            cmp         #$FF
            beq         @long
            ldx         #0
:
            cpx         v_rawn
            bcs         @done
            lda         v_raw,X
            jsr         tx_put
            inx
            bra         :-
@done:
            rts

@long:
            jmp         ser_dirty

; .A into the send ring (the room's been made sure of).  Keeps .A, .X
tx_put:
            ldy         tx_head
            sta         tx_buf,Y
            iny
            sty         tx_head
            rts

; .A = the room in the send ring
tx_free:
            sec
            lda         tx_tail
            sbc         tx_head
            dec         a
            rts

; The serial port painted, as there's room: the shown window's screen, then its state (ts_ser 1: from the start; 2:
; going on).  The rows go as a stream of lines, as they came (a CR and an LF a row; a row an autowrap continued
; after its full row, the terminal wrapping too), so the PC's terminal keeps them in its own scrollback as it would
; have.  The terminal's cursor is followed (sp_cy, sp_cx, sp_full), so the window's cursor is reached with the least
; (nothing, a CR and LFs, or a CUP)
ser_paint:
            lda         w_in
            jsr         vt_load
            jsr         chr_geom
            lda         ts_ser
            cmp         #1
            bne         @chr
            jsr         tx_free                             ; The terminal reset and cleared
            cmp         #VT_ROOM
            bcs         :+
            rts
:
            ldx         #0
:
            lda         s_ser_clear,X
            beq         :+
            jsr         tx_put
            inx
            bra         :-
:
            stz         sp_again
            stz         sp_row
            stz         sp_col
            stz         sp_cy
            stz         sp_cx
            stz         sp_full
            lda         #COL_DEF
            sta         sp_c
            stz         sp_f
            stz         sp_dec
            stz         sp_chr                              ; (Its chrome first: sp_chrome; drawn now, not again)
            stz         sp_ccol
            inc         chr_pass
            lda         #1
            trb         chr_dirty
            lda         #2
            sta         ts_ser
@chr:
            jsr         sp_chrome                           ; The chrome's rows, as there's room
            bcc         :+
            rts
:
            lda         ts_ser                              ; (Its chrome alone: then the state)
            cmp         #3
            bne         @rows
            jmp         @end
@rows:
            lda         sp_row
            cmp         v_rows
            bcc         :+
            jmp         @end
:
            jsr         row_ptr
            lda         sp_col
            bne         @cells
            jsr         row_last                            ; A row: its cells to paint (none: it's blank already;
            sta         sp_last                             ;   all of them if an autowrap continued it)
            ldx         sp_row
            inx
            cpx         v_rows
            bcs         :+
            txa
            jsr         row_cont
            beq         :+
            lda         v_cols
            sta         sp_last
:
            lda         sp_last
            bne         :+
            jmp         @next
:
            jsr         tx_free
            cmp         #VT_ROOM
            bcs         :+
            rts
:
            lda         sp_row                              ; Continuing the row before it, which was painted to its
            jsr         row_cont                            ;   end: the terminal wraps
            beq         @start
            lda         sp_full
            beq         @start
            ldx         sp_cy
            inx
            cpx         sp_row
            beq         @here
@start:
            lda         sp_row                              ; To the row's start: a CR, and an LF a row
            jsr         sp_down
@here:
            lda         sp_row
            sta         sp_cy
            stz         sp_full
            jsr         row_ptr
            lda         vb2                                 ; (A double row: its ESC # first)
            sta         $00
            ldy         #META
            lda         (vq),Y
            and         #RA_LINE
            beq         @cells
            ldx         #'6'
            cmp         #RA_DW
            beq         :+
            ldx         #'3'
            cmp         #RA_DHT
            beq         :+
            ldx         #'4'
:
            lda         #ESC
            jsr         tx_put
            lda         #'#'
            jsr         tx_put
            txa
            jsr         tx_put
@cells:
            ldy         sp_col
            cpy         sp_last
            bcs         @done
            jsr         tx_free
            cmp         #VT_ROOM
            bcs         :+
            rts
:
            ldy         sp_col
            jsr         cell_get
            jsr         sp_cell
            inc         sp_col
            bra         @cells
@done:
            lda         sp_last                             ; (The terminal's cursor after them: painted to the
            sta         sp_cx                               ;   row's end, past it)
            cmp         v_cols
            bcc         @next
            lda         #1
            sta         sp_full
@next:
            inc         sp_row
            stz         sp_col
            jmp         @rows

@end:
            jsr         tx_free                             ; Then its state
            cmp         #160
            bcc         @wait
            jsr         ser_state
            lda         sp_again                            ; (Written to meanwhile: painted again)
            beq         :+
            lda         #1
            sta         ts_ser
            rts
:
            stz         ts_ser
            inc         TASK_EVENT                          ; (The window's writers, waiting, look again)
@wait:
            rts

; The serial port's chrome rows (chr_geom's tr_*: the shown window's), as the send ring has room: sp_chr the row (0
; the bar, 1 the header, 2 the footer, 3 done), sp_ccol its next cell (each rendered again as it goes on).  Done in a
; paint (ts_ser 2), the terminal's cursor to the window's first row.  OUT: C = 1, not done (no room yet)
sp_chrome:
            stz         out_t
@row:
            ldx         sp_chr
            cpx         #3
            bcc         :+
            jmp         @done
:
            lda         tr_bar                              ; (Its row there: the serial port's)
            cpx         #0
            beq         :+
            lda         tr_head
            cpx         #1
            beq         :+
            lda         tr_foot
:
            cmp         #$FF
            beq         @next
            sta         chr_r
            lda         ser_cols
            tax
            lda         sp_chr
            FAR1        chr_render                          ; (cr_c, cr_a, cr_f: its cells)
            lda         sp_ccol
            bne         @cells
            jsr         tx_free
            cmp         #VT_ROOM
            bcs         :+
            jmp         @wait
:
            lda         chr_r
            ldx         #0
            jsr         out_cup_abs
            lda         #$FF                                ; (The terminal's cursor: not the window's)
            sta         sp_cy
@cells:
            ldx         sp_ccol
            cpx         cr_n
            bcs         @next
            jsr         tx_free
            cmp         #VT_ROOM
            bcc         @wait
            ldx         sp_ccol
            lda         cr_f,X
            tay
            lda         cr_a,X
            cmp         sp_c                                ; (Its rendition, if the terminal hasn't it)
            bne         :+
            cpy         sp_f
            beq         @glyph
:
            sta         sp_c
            sty         sp_f
            tya
            tax
            lda         sp_c
            jsr         out_sgr
@glyph:
            lda         sp_dec
            beq         :+
            jsr         out_g0b
            stz         sp_dec
:
            ldx         sp_ccol
            lda         cr_c,X
            jsr         tx_put
            inc         sp_ccol
            bra         @cells
@next:
            inc         sp_chr
            stz         sp_ccol
            jmp         @row
@done:
            lda         ts_ser                              ; (A paint's: the cursor to the window's first row, once)
            cmp         #2
            bne         @ok
            lda         sp_chr
            cmp         #3
            bne         @ok
            lda         ser_chr
            beq         :+
            jsr         tx_free
            cmp         #VT_ROOM
            bcc         @wait
            lda         #0
            ldx         #0
            jsr         out_cup
            stz         sp_cy
            stz         sp_cx
            stz         sp_full
:
            inc         sp_chr
@ok:
            clc
            rts
@wait:
            sec
            rts

; The serial port with chrome, following: the cursor put where the window's is (its rows below the chrome) in
; place of the sequence as it came.  OUT: C = 1 so; C = 0 not (no chrome there: the sequence as it came)
sc_cup:
            lda         fw_ser
            beq         sc_no
            lda         ser_chr
            beq         sc_no
            stz         out_t
            lda         v_y
            ldx         v_x
            jsr         out_cup
            sec
            rts
sc_no:
            clc
            rts

; ... the margins (the window's, below the chrome) and the cursor, in place of DECSTBM as it came.  OUT: as sc_cup's
sc_region:
            lda         fw_ser
            beq         sc_no
            lda         ser_chr
            beq         sc_no
            stz         out_t
            jsr         out_region_all
            bra         sc_cup

; ... its chrome erased (ED, VT52's J): drawn again after (vt_pump: chr_dirty)
sc_erased:
            lda         fw_ser
            beq         :+
            lda         ser_chr
            beq         :+
            lda         #1
            tsb         chr_dirty
:
            rts

; ... a sequence that changes the whole terminal (RIS, DECALN): painted again instead
sc_paint:
            lda         ser_chr
            beq         :+
            jmp         ser_dirty
:
            rts

; The terminal's cursor to the start of row .A, at or below it: a CR (if it isn't at a row's start), an LF a row
sp_down:
            sta         vt_k
            lda         sp_cx
            ora         sp_full
            beq         :+
            lda         #CR
            jsr         out
            stz         sp_cx
            stz         sp_full
:
            lda         sp_cy
            cmp         vt_k
            bcs         :+
            inc         sp_cy
            lda         #LF
            jsr         out
            bra         :-
:
            rts

; Z = 0 if screen row .A was continued by an autowrap from the row before it (its meta's RA_CONT).  Modifies vr
row_cont:
            clc
            adc         v_sb0
            jsr         map_row
            lsr
            ora         #$80
            sta         vr + 1
            lda         #0
            ror
            sta         vr
            lda         vb2
            sta         $00
            ldy         #META
            lda         (vr),Y
            and         #RA_CONT
            rts

; A cell to the serial port (being painted): its colours and rendition if they've changed, its character (a DEC
; graphic in ESC ( 0)
sp_cell:
            lda         cell_a
            cmp         sp_c
            bne         @sgr
            lda         cell_f
            cmp         sp_f
            beq         @char
@sgr:
            lda         cell_a
            sta         sp_c
            ldx         cell_f
            stx         sp_f
            jsr         out_sgr
@char:
            lda         cell_c
            cmp         #$20
            bcs         @plain
            ldx         sp_dec
            bne         :+
            jsr         out_g0dec
            inc         sp_dec
:
            lda         cell_c
            clc
            adc         #$5F
            jmp         out
@plain:
            ldx         sp_dec
            beq         :+
            jsr         out_g0b
            stz         sp_dec
:
            lda         cell_c
            jmp         out

; The serial port painted: the window's state after its cells, what isn't as the terminal has it now: the margins,
; origin mode (each homes the cursor), the cursor (past the last column: its cell again, so the terminal has the flag
; too), the modes, the character sets, the rendition, the cursor shown or not
ser_state:
            lda         sp_dec
            beq         :+
            jsr         out_g0b
:
            jsr         out_region                          ; (Not the whole screen: DECSTBM)
            bcc         :+
            lda         #$FF
            sta         sp_cy
:
            lda         ser_chr                             ; (With chrome: no DECOM there, the rows absolute)
            bne         :+
            lda         v_mode
            and         #VM_OM
            beq         :+
            ldx         #<s_om
            ldy         #>s_om
            jsr         out_str
            lda         #$FF
            sta         sp_cy
:
            lda         v_wrap
            beq         @move
            ldx         v_cols                              ; The last cell again, at the last column
            dex
            lda         v_y
            jsr         out_cup_om
            lda         v_y
            jsr         row_ptr
            ldy         v_cols
            dey
            jsr         cell_get
            stz         sp_dec
            jsr         sp_cell
            lda         sp_dec
            beq         @modes
            jsr         out_g0b
            bra         @modes
@move:
            jsr         sp_move
@modes:
            lda         v_mode
            and         #VM_AWM
            bne         :+
            ldx         #<s_awm_off
            ldy         #>s_awm_off
            jsr         out_str
:
            lda         v_mode
            and         #VM_IRM
            beq         :+
            ldx         #<s_irm_on
            ldy         #>s_irm_on
            jsr         out_str
:
            lda         v_mode
            and         #VM_LNM
            beq         :+
            ldx         #<s_lnm_on
            ldy         #>s_lnm_on
            jsr         out_str
:
            lda         v_mode
            and         #VM_SCNM
            beq         :+
            ldx         #<s_scnm_on
            ldy         #>s_scnm_on
            jsr         out_str
:
            lda         v_g0
            cmp         #'B'
            beq         :+
            pha
            lda         #ESC
            jsr         out
            lda         #'('
            jsr         out
            pla
            jsr         out
:
            lda         v_g1
            cmp         #'B'
            beq         :+
            pha
            lda         #ESC
            jsr         out
            lda         #')'
            jsr         out
            pla
            jsr         out
:
            lda         v_gl
            beq         :+
            lda         #SO
            jsr         out
:
            lda         v_col                               ; The rendition, if it isn't what the cells left
            cmp         sp_c
            bne         :+
            lda         v_fl
            cmp         sp_f
            beq         @tcem
:
            lda         v_col
            ldx         v_fl
            jsr         out_sgr
@tcem:
            lda         v_mode                              ; The cursor shown or hidden, if the terminal's isn't
            and         #VM_TCEM
            cmp         ser_tcem
            beq         @done
            sta         ser_tcem
            ldx         #<s_tcem_on
            ldy         #>s_tcem_on
            cmp         #0
            bne         :+
            ldx         #<s_tcem_off
            ldy         #>s_tcem_off
:
            jmp         out_str
@done:
            rts

; The terminal's cursor to the window's: nothing if it's there; on its row's start or a row below's start, a CR and
; LFs; else a CUP (with DECOM, from the region's top)
sp_move:
            lda         sp_cy
            cmp         #$FF
            beq         @cup
            lda         v_y
            cmp         sp_cy
            bcc         @cup
            bne         @down
            lda         sp_full                             ; (Its row)
            bne         @down
            lda         v_x
            cmp         sp_cx
            beq         @done
@down:
            lda         v_x
            bne         @cup
            lda         v_y
            jmp         sp_down
@cup:
            lda         v_y
            ldx         v_x
            jmp         out_cup_om
@done:
            rts

; ****************************************************************************
; The screen (vid's #v/term): following, and painted

; A character to the screen: vid's cursor to its cell (scrolled with the window first, if the wrap scrolled it),
; its colours and rendition, then its glyph (a DEC graphic as ASCII).  (print_glyph's, before the cell's written)
fc_print:
            lda         #1
            sta         out_t
            sta         scr_sync
            lda         vt_scr
            beq         :+
            jsr         fc_scroll_up
:
            jsr         fc_pos
            jsr         fc_sgr
            lda         v_last
            jsr         scr_glyph
            jsr         cur_dw                              ; (A double row: a space after it)
            beq         :+
            lda         #' '
            jsr         scr_put
            inc         scr_x
:
            ldx         scr_x                               ; vid's cursor on (past its last column: not known)
            inx
            cpx         v_cols
            bcs         :+
            stx         scr_x
            rts
:
            lda         #$FF
            sta         scr_y
            rts

; vid's cursor where the window's is (a CUP, if it isn't)
fc_pos:
            jsr         vid_col
            sta         fp_x
            lda         v_y
            cmp         scr_y
            bne         @cup
            lda         fp_x
            cmp         scr_x
            beq         @done
            cmp         #0                                  ; (The row's start: a CR)
            bne         @cup
            stz         scr_x
            lda         #CR
            jmp         scr_put
@cup:
            lda         #1
            sta         out_t
            lda         v_y
            ldx         fp_x
            jsr         out_cup
            lda         v_y
            sta         scr_y
            lda         fp_x
            sta         scr_x
@done:
            rts

; vid's colours and rendition the window's (SGR's canonical form, if they aren't)
fc_sgr:
            lda         v_col
            ldx         v_fl
fc_sgr_ax:
            pha                                             ; (DECSCNM: every cell reversed)
            lda         v_mode
            and         #VM_SCNM
            beq         :+
            txa
            eor         #F_REV
            tax
:
            pla
            cmp         scr_c
            bne         :+
            cpx         scr_f
            beq         @done
:
            sta         scr_c
            stx         scr_f
            ldy         #1
            sty         out_t
            jmp         out_sgr
@done:
            rts

; The cursor moved (not to the screen now: it goes there before what needs it)
fc_move:
            lda         fw_scr
            beq         :+
            sta         scr_sync
:
            rts

; vid's cursor not known (and its colours): a CUP and SGR before what comes next
fc_lost:
            lda         #$FF
            sta         scr_y
            sta         scr_c
            jmp         fc_move

; The window's region scrolled up a row (C = 1: m_index's): vid's too, its cursor at the region's bottom, an IND
fc_scrolled:
            bcc         fc_none
            lda         fw_scr
            beq         fc_none
fc_scroll_up:
            lda         #1
            sta         out_t
            sta         scr_sync
            lda         v_col                               ; (The row coming in: the background's colour)
            and         #$F0
            ora         #COL_DEF & $0F
            ldx         #0
            jsr         fc_sgr_ax
            lda         scr_y
            cmp         v_bot
            beq         :+
            lda         v_bot
            sta         scr_y
            stz         scr_x
            ldx         #0
            jsr         out_cup
:
            lda         #ESC
            jsr         scr_put
            lda         #'D'
            jmp         scr_put
fc_none:
            rts

; ... down a row (m_rindex's): vid's cursor at the region's top, an RI
fc_scrolled_down:
            lda         fw_scr
            beq         fc_none
            lda         #1
            sta         out_t
            sta         scr_sync
            lda         v_col
            and         #$F0
            ora         #COL_DEF & $0F
            ldx         #0
            jsr         fc_sgr_ax
            lda         scr_y
            cmp         v_top
            beq         :+
            lda         v_top
            sta         scr_y
            stz         scr_x
            ldx         #0
            jsr         out_cup
:
            lda         #ESC
            jsr         scr_put
            lda         #'M'
            jmp         scr_put

; ED or EL .A (to the screen if it's following): vid's cursor at the window's, the background's colour
fc_erase:
            sta         vt_i
            lda         fw_scr
            beq         fc_none
            lda         #1
            sta         out_t
            sta         scr_sync
            jsr         fc_pos
            lda         v_col
            and         #$F0
            ora         #COL_DEF & $0F
            ldx         #0
            jsr         fc_sgr_ax
            lda         #ESC
            jsr         scr_put
            lda         #'['
            jsr         scr_put
            lda         vt_i
            ora         #'0'
            jmp         scr_put

; ... and its letter, .A (J: vid's cursor not known after it)
fc_erase_end:
            ldx         fw_scr
            beq         fc_none
            pha
            jsr         scr_put
            pla
            cmp         #'J'
            bne         :+
            lda         #$FF
            sta         scr_y
            lda         #2                                  ; (Its chrome erased too: drawn again)
            tsb         chr_dirty
:
            rts

; The region's changed: vid's too (DECSTBM; it homes vid's cursor)
fc_region:
            lda         fw_scr
            bne         :+
            rts
:
            lda         #1
            sta         out_t
            sta         scr_sync
            jsr         out_region_all
            stz         scr_x
            stz         scr_y
            rts

; The cursor shown or not (DECTCEM): vid's too
fc_tcem:
            lda         fw_scr
            bne         :+
            rts
:
            lda         #1
            sta         out_t
            ldx         #<s_tcem_on
            ldy         #>s_tcem_on
            lda         v_mode
            and         #VM_TCEM
            bne         :+
            ldx         #<s_tcem_off
            ldy         #>s_tcem_off
:
            jmp         out_str

; The screen painted, all at once: its chrome, the shown window's screen, its region, cursor and rendition
scr_paint:
            lda         w_in
            jsr         vt_load
            ldx         #<s_scr_sgr0                        ; (The rendition plain, or reversed with DECSCNM, as the
            ldy         #>s_scr_sgr0                        ;   screen's cleared)
            stz         scr_f
            lda         v_mode
            and         #VM_SCNM
            beq         :+
            ldx         #<s_scr_sgr7
            ldy         #>s_scr_sgr7
            lda         #F_REV
            sta         scr_f
:
            jsr         out_str
            ldx         #<s_scr_clear
            ldy         #>s_scr_clear
            jsr         out_str
            lda         #COL_DEF
            sta         scr_c
            stz         scr_dec
            jsr         chr_geom                            ; Its chrome (the rows offset below it)
            ldx         #1
            jsr         chr_draw
            lda         #2
            trb         chr_dirty
            stz         vt_i
@row:
            lda         scr_fail                            ; (Refused: claimed; painted after)
            beq         :+
            rts
:
            lda         vt_i
            cmp         v_rows
            bcs         @state
            jsr         row_ptr
            jsr         row_last
            sta         vt_k
            beq         @next
            lda         vb2                                 ; (A double row's cells a space apart)
            sta         $00
            ldy         #META
            lda         (vq),Y
            and         #RA_LINE
            sta         sp_dw
            lda         vt_i
            ldx         #0
            jsr         out_cup
            stz         vt_j
@cell:
            ldy         vt_j
            cpy         vt_k
            bcs         @next
            jsr         cell_get
            lda         cell_a
            ldx         cell_f
            jsr         fc_sgr_ax
            lda         cell_c
            jsr         scr_glyph
            lda         sp_dw
            beq         :+
            lda         #' '
            jsr         scr_put
:
            inc         vt_j
            bra         @cell
@next:
            inc         vt_i
            bra         @row

@state:
            jsr         out_region_all
            lda         v_y
            ldx         v_x
            jsr         out_cup
            lda         v_y
            sta         scr_y
            lda         v_x
            sta         scr_x
            stz         scr_sync
            jsr         fc_sgr
            lda         #1                                  ; (fc_tcem's test)
            sta         fw_scr
            jsr         fc_tcem
            stz         fw_scr
@done:
            rts

; ****************************************************************************
; Chrome (W4): each terminal's rows around the shown window's, as its w_chr has them there: the bar (console-wide:
; at the top or the bottom), the window's header (above its screen) and footer (below), each rendered by cons.s
; (chr_render) from its format, at the terminal's width.  The window's rows are offset below the bar and header
; (tr_off: out_cup, out_region_all)

; tr_*: where each terminal's chrome rows are, for the shown window (loaded: v_rows)
chr_geom:
            ldx         #1
@term:
            lda         #$FF
            sta         tr_bar,X
            sta         tr_head,X
            sta         tr_foot,X
            stz         tr_off,X
            ldy         w_in                                ; (Its chrome there)
            lda         w_chr,Y
            and         #CH_LIVE
            cpx         #0
            beq         :+
            lsr
            lsr
            lsr
            lsr
:
            sta         chr_k
            and         #CH_BAR
            beq         @head
            lda         bar_pos
            beq         @head
            cmp         #BAR_TOP
            bne         @bottom
            stz         tr_bar,X
            inc         tr_off,X
            bra         @head
@bottom:
            lda         ser_rows                            ; (The terminal's last row)
            cpx         #0
            beq         :+
            lda         scr_rows
:
            dec         a
            sta         tr_bar,X
@head:
            lda         chr_k
            and         #CH_HEAD
            beq         @foot
            lda         tr_off,X
            sta         tr_head,X
            inc         tr_off,X
@foot:
            lda         chr_k
            and         #CH_FOOT
            beq         @next
            clc
            lda         tr_off,X
            adc         v_rows
            sta         tr_foot,X
@next:
            dex
            bpl         @term
            stz         ser_chr                             ; (The serial port's: any?)
            lda         tr_off
            bne         :+
            lda         tr_bar
            and         tr_foot
            cmp         #$FF
            beq         @none
:
            inc         ser_chr
@none:
            rts

; Terminal .X's chrome rows (the shown window's, loaded; tr_*: chr_geom's), each rendered (cons.s) and written: its
; cursor and rendition not known after
chr_draw:
            stx         chr_t
            stx         out_t
            inc         chr_pass                            ; (A drawing: the time read once)
            lda         tr_bar,X
            ldy         #CR_BAR
            jsr         chr_row
            ldx         chr_t
            lda         tr_head,X
            ldy         #CR_HEAD
            jsr         chr_row
            ldx         chr_t
            lda         tr_foot,X
            ldy         #CR_FOOT
            jsr         chr_row
            lda         #$FF                                ; (vid's cursor and colours: not known)
            sta         scr_y
            sta         scr_c
            rts

; Chrome row .Y at the terminal's row .A ($FF: none there)
chr_row:
            cmp         #$FF
            beq         @done
            sta         chr_r
            ldx         chr_t                               ; (Its width: the terminal's)
            lda         ser_cols
            cpx         #0
            beq         :+
            lda         scr_cols
:
            tax
            tya
            FAR1        chr_render                          ; (cr_c, cr_a, cr_f: its cells, cr_n of them)
            lda         chr_r
            ldx         #0
            jsr         out_cup_abs
            stz         chr_k
@cell:
            ldx         chr_k
            cpx         cr_n
            bcs         @done
            lda         cr_f,X
            tay
            lda         cr_a,X
            cmp         scr_c                               ; (Its rendition, if vid hasn't it: the chrome's own,
            bne         :+                                  ;   no DECSCNM)
            cpy         scr_f
            beq         @glyph
:
            sta         scr_c
            sty         scr_f
            tya
            tax
            lda         scr_c
            jsr         out_sgr
@glyph:
            ldx         chr_k
            lda         cr_c,X
            jsr         scr_glyph
            inc         chr_k
            bra         @cell
@done:
            rts

; Activity .A (ACT_*) in window .X, not shown: marked in the chrome (once)
act_mark:
            pha
            ora         w_act,X
            cmp         w_act,X
            beq         :+
            sta         w_act,X
            lda         #3
            tsb         chr_dirty
:
            pla
            rts

; The chrome drawn again (a label, a status line changed)
chr_touch:
            lda         #3
            tsb         chr_dirty
            rts

; vt_a = window .A's status line (w_stat: STAT_SIZE a window)
stat_ptr:
            lsr
            sta         vt_a + 1
            lda         #0
            ror
            clc
            adc         #<w_stat
            sta         vt_a
            lda         vt_a + 1
            adc         #>w_stat
            sta         vt_a + 1
            rts

; DECSSDT: the status line's type (0 none, 1 the indicator, 2 the program's: kept)
x_decssdt:
            lda         v_parl
            sta         v_ssdt
            rts

; DECSASD: the output to the status line (1) or the main display (0)
x_decsasd:
            stz         v_sasd
            lda         v_parh
            bne         :+
            lda         v_parl
            cmp         #1
            bne         :+
            sta         v_sasd
:
            rts

; The status line's glyph .A at its cursor (a DEC graphic as its ASCII), blanks before it if the text's shorter
sl_glyph:
            cmp         #$20
            bcs         :+
            tax
            lda         dec_ascii,X
:
            ldx         v_stx
            cpx         #STAT_SIZE - 1
            bcs         @done
            pha
            lda         vt_w
            jsr         stat_ptr
            ldy         #0
@scan:
            cpy         v_stx
            beq         @at
            lda         (vt_a),Y
            beq         @blank
            iny
            bra         @scan
@blank:                                                     ; (Shorter: blanks to the cursor)
            lda         #' '
            sta         (vt_a),Y
            iny
            lda         #0
            sta         (vt_a),Y
            bra         @scan
@at:
            lda         (vt_a),Y
            bne         @over
            iny                                             ; (At its end: a new end after it)
            lda         #0
            sta         (vt_a),Y
            dey
@over:
            pla
            sta         (vt_a),Y
            inc         v_stx
            jmp         chr_touch
@done:
            rts

; A C0 control into the status line: CR, BS; a BEL rings; the rest nothing
sl_c0:
            cmp         #CR
            bne         :+
            stz         v_stx
            rts
:
            cmp         #BS
            bne         :+
            lda         v_stx
            beq         @done
            dec         v_stx
@done:
            rts
:
            cmp         #BEL
            bne         @done
            jmp         c_bel

; A CSI sequence's end (.A) while the status line has the output: DECSASD and DECSSDT, EL; the rest dropped
sl_csi:
            ldx         v_inter
            cpx         #'$'
            bne         @el
            cmp         #'}'
            bne         :+
            jmp         x_decsasd
:
            cmp         #'~'
            bne         @done
            jmp         x_decssdt
@el:
            cpx         #0
            bne         @done
            ldx         v_priv
            bne         @done
            cmp         #'K'
            bne         @done
            lda         vt_w                                ; EL: 0 from the cursor, 1 to it, 2 all
            jsr         stat_ptr
            lda         v_parl
            beq         @rest
            cmp         #2
            bcs         @all
            ldy         #0                                  ; (1: blanks to the cursor, within the text)
:
            lda         (vt_a),Y
            beq         @dirty
            lda         #' '
            sta         (vt_a),Y
            cpy         v_stx
            bcs         @dirty
            iny
            bra         :-
@rest:
            ldy         #0                                  ; (0: the text ends at the cursor, if it went past it)
:
            cpy         v_stx
            beq         :+
            lda         (vt_a),Y
            beq         @done
            iny
            bra         :-
:
            lda         #0
            sta         (vt_a),Y
            bra         @dirty
@all:
            lda         #0
            sta         (vt_a)
@dirty:
            jmp         chr_touch
@done:
            rts

; Glyph .A to the screen: a DEC graphic ($00-$1F) in ESC ( 0, as its DEC character (vid shows the font's glyph), the
; rest in ESC ( B (vid's G0 followed: scr_dec)
scr_glyph:
            cmp         #$20
            bcs         @plain
            pha
            lda         scr_dec
            bne         :+
            jsr         out_g0dec
            lda         #1
            sta         scr_dec
:
            pla
            clc
            adc         #$5F
            jmp         scr_put
@plain:
            ldx         scr_dec
            beq         :+
            pha
            jsr         out_g0b
            stz         scr_dec
            pla
:
            jmp         scr_put

; .A into the screen's buffer (written when it's full; nothing after a write's been refused).  Keeps .X
scr_put:
            ldy         scr_fail
            bne         @done
            ldy         scr_n
            sta         scr_buf,Y
            iny
            sty         scr_n
            cpy         #SCR_BUF
            bcc         @done
            phx
            jsr         scr_flush
            plx
@done:
            rts

; The screen's buffer to #v/term.  A write refused (E_BUSY: the chip's claimed): the screen to be painted again, and
; nothing more to it in this request; another that fails: the screen gone, none from then on
scr_flush:
            lda         scr_n
            beq         @done
            sta         r1
            stz         r1 + 1
            LDR         r0, scr_buf
            lda         scr_fd
            jsr         WRITE
            bcc         :+
            cmp         #E_BUSY
            bne         @gone
            lda         #1
            sta         ts_scr
            sta         scr_fail
            sta         scr_chk                             ; (Its size looked at: vid's mode may have changed)
            stz         fw_scr
            bra         :+
@gone:
            lda         #2
            sta         scr_st
:
            stz         scr_n
@done:
            rts

; ****************************************************************************
; Output to a terminal (out_t: 0 the serial port, 1 the screen)

; .A out.  Keeps .X
out:
            ldy         out_t
            bne         :+
            jmp         tx_put
:
            jmp         scr_put

; The string at .X/.Y out
out_str:
            stx         vt_a
            sty         vt_a + 1
            ldy         #0
:
            lda         (vt_a),Y
            beq         :+
            phy
            jsr         out
            ply
            iny
            bra         :-
:
            rts

; .A out in decimal (1-3 digits).  Keeps .X
out_dec:
            phx
            ldx         #0
:
            cmp         #100
            bcc         :+
            sbc         #100
            inx
            bra         :-
:
            pha
            txa
            beq         :+
            ora         #'0'
            jsr         out
            lda         #1                                  ; (A hundreds' digit out: the tens' goes too)
:
            sta         od_n
            pla
            ldx         #0
:
            cmp         #10
            bcc         :+
            sbc         #10
            inx
            bra         :-
:
            pha
            txa
            ora         od_n
            beq         :+
            txa
            ora         #'0'
            jsr         out
:
            pla
            ora         #'0'
            jsr         out
            plx
            rts

; A CUP to the window's row .A, column .X (from 0): the terminal's row below its chrome's (tr_off).  Modifies .Y
out_cup:
            ldy         out_t
            clc
            adc         tr_off,Y
; ... the terminal's row .A
out_cup_abs:
            pha
            phx
            lda         #ESC
            jsr         out
            lda         #'['
            jsr         out
            plx
            pla
            inc         a
            jsr         out_dec
            lda         #';'
            jsr         out
            txa
            inc         a
            jsr         out_dec
            lda         #'H'
            jmp         out

; ... the row from the region's top with DECOM (the serial port's, painted)
out_cup_om:
            pha
            lda         ser_chr                             ; (With chrome: the terminal has no DECOM)
            bne         :+
            lda         v_mode
            and         #VM_OM
            beq         :+
            pla
            sec
            sbc         v_top
            pha
:
            pla
            bra         out_cup

; SGR's canonical form for colours .A, rendition .X: ESC [ 0 ; the rendition's ; the colours (the defaults left
; out) m
out_sgr:
            sta         vt_a
            stx         vt_a + 1
            lda         #ESC
            jsr         out
            lda         #'['
            jsr         out
            lda         #'0'
            jsr         out
            ldx         #0
@flag:
            lda         vt_a + 1
            and         sgr_bit,X
            beq         :+
            lda         #';'
            jsr         out
            lda         sgr_num,X
            jsr         out
:
            inx
            cpx         #SGR_N
            bcc         @flag
            lda         vt_a                                ; The foreground (not 7)
            and         #$0F
            cmp         #COL_DEF & $0F
            beq         @bg
            pha
            lda         #';'
            jsr         out
            pla
            cmp         #8
            bcs         :+
            pha
            lda         #'3'
            bra         @fgd
:
            sbc         #8                                  ; (C = 1)
            pha
            lda         #'9'
@fgd:
            jsr         out
            pla
            ora         #'0'
            jsr         out
@bg:
            lda         vt_a                                ; The background (not 0)
            lsr
            lsr
            lsr
            lsr
            beq         @end
            pha
            lda         #';'
            jsr         out
            pla
            cmp         #8
            bcs         :+
            pha
            lda         #'4'
            jsr         out
            bra         @bgd
:
            sbc         #8
            pha
            lda         #'1'
            jsr         out
            lda         #'0'
            jsr         out
@bgd:
            pla
            ora         #'0'
            jsr         out
@end:
            lda         #'m'
            jmp         out

; The region: DECSTBM if it isn't the whole screen (the serial port's), or always (the screen's: the window's rows
; are fewer than vid's)
out_region:
            lda         ser_chr                             ; (The serial port with chrome: always, its rows below it)
            bne         out_region_all
            lda         v_top
            bne         out_region_all
            ldx         v_rows
            dex
            cpx         v_bot
            bne         out_region_all
            clc                                             ; (C = 0: none sent; C = 1, sent)
            rts
out_region_all:
            lda         #ESC
            jsr         out
            lda         #'['
            jsr         out
            ldy         out_t                               ; (Its rows the terminal's, below its chrome)
            lda         v_top
            sec
            adc         tr_off,Y
            jsr         out_dec
            lda         #';'
            jsr         out
            ldy         out_t
            lda         v_bot
            sec
            adc         tr_off,Y
            jsr         out_dec
            lda         #'r'
            jsr         out
            sec
            rts

; ESC ( 0, ESC ( B: G0 the DEC graphics, or ASCII
out_g0dec:
            lda         #'0'
            bra         out_g0
out_g0b:
            lda         #'B'
out_g0:
            pha
            lda         #ESC
            jsr         out
            lda         #'('
            jsr         out
            pla
            jmp         out

; ****************************************************************************
; VT52 mode (DECANM reset: CSI ? 2 l; ESC < back to ANSI).  Each of its sequences becomes the ANSI one that does
; the same, its bytes in v_raw (so the serial port gets that: the PC's terminal stays in ANSI mode), and that one is
; done: A B C D H J K as CSI's, I as RI, F and G as ESC ( 0 and ESC ( B (the VT100's VT52 graphics are the DEC
; Special Graphics), Y row column as CUP; Z answered ESC / Z; = and > the keypad's modes

; After ESC, in VT52 mode
st_vt52:
            stz         v_state
            ldx         #V52_N - 1
:
            cmp         v52_final,X
            beq         :+
            dex
            bpl         :-
            rts                                             ; (Not one: dropped)
:
            txa
            asl
            tax
            lda         vch
            jmp         (v52_vec,X)

; A B C D H J K: CSI and the same letter, no numbers
v52_csi:
            pha
            jsr         csi_start
            stz         v_state
            stz         v_inter
            stz         v_priv
            lda         #'['
            jsr         raw_v52
            pla
            pha
            jsr         raw_add
            pla
            jmp         csi_do

; I: RI
v52_ri:
            jsr         raw_v52_esc
            lda         #'M'
            jsr         raw_add
            jmp         e_ri

; F, G: the graphics (G0 the DEC Special Graphics), or ASCII
v52_gfx:
            lda         #'0'
            bra         v52_g0
v52_ascii:
            lda         #'B'
v52_g0:
            pha
            jsr         raw_v52_esc
            lda         #'('
            sta         v_inter
            jsr         raw_add
            pla
            pha
            jsr         raw_add
            pla
            jmp         esc_do

; Y: the row next, then the column (each + 31, from 1)
v52_y:
            lda         #S_Y1
            sta         v_state
            rts

st_y1:
            sec
            sbc         #31
            bcs         :+
            lda         #1
:
            sta         v_y52
            lda         #S_Y2
            sta         v_state
            rts

st_y2:                                                      ; The column: CSI row ; column H
            sec
            sbc         #31
            bcs         :+
            lda         #1
:
            pha
            jsr         csi_start
            stz         v_state
            stz         v_inter
            stz         v_priv
            lda         v_y52
            sta         v_parl
            pla
            sta         v_parl + 1
            lda         #1
            sta         v_npar
            lda         #'['
            jsr         raw_v52
            lda         v_parl
            jsr         raw_dec
            lda         #';'
            jsr         raw_add
            lda         v_parl + 1
            jsr         raw_dec
            lda         #'H'
            jsr         raw_add
            lda         #'H'
            jmp         csi_do

; Z: identify, ESC / Z
v52_id:
            lda         #ESC
            jsr         vt_key
            lda         #'/'
            jsr         vt_key
            lda         #'Z'
            jmp         vt_key

; <: ANSI mode again
v52_ansi:
            lda         v_mode2
            and         #<~VM2_VT52
            sta         v_mode2
            rts

; v_raw: ESC, then .A (raw_v52); or ESC alone (raw_v52_esc)
raw_v52:
            pha
            jsr         raw_v52_esc
            pla
            jmp         raw_add
raw_v52_esc:
            lda         #ESC
            sta         v_raw
            lda         #1
            sta         v_rawn
            rts

; .A (1-255) into v_raw in decimal
raw_dec:
            ldx         #0
:
            cmp         #100
            bcc         :+
            sbc         #100
            inx
            bra         :-
:
            pha
            txa
            beq         :+
            ora         #'0'
            jsr         raw_add
            lda         #1                                  ; (A hundreds' digit: the tens' goes too)
:
            sta         od_n
            pla
            ldx         #0
:
            cmp         #10
            bcc         :+
            sbc         #10
            inx
            bra         :-
:
            pha
            txa
            ora         od_n
            beq         :+
            txa
            ora         #'0'
            jsr         raw_add
:
            pla
            ora         #'0'
            jmp         raw_add

; ****************************************************************************
; Answers (DA, DSR): into the loaded window's keys, as a terminal's would come

answer_da:
            ldx         #0
:
            lda         s_da,X
            beq         :+
            jsr         vt_key
            inx
            bra         :-
:
            rts

; .A in decimal into the window's keys
key_dec:
            ldx         #0

; .A (low), .X (high) in decimal into the window's keys
key_dec16:
            sta         vt_a
            stx         vt_a + 1
            stz         od_n                                ; (<> 0: a digit's gone: no more leading zeros)
            ldx         #0
@power:
            ldy         #'0'
@sub:
            sec
            lda         vt_a
            sbc         p10l,X
            sta         vt_k
            lda         vt_a + 1
            sbc         p10h,X
            bcc         @digit
            sta         vt_a + 1
            lda         vt_k
            sta         vt_a
            iny
            bra         @sub
@digit:
            cpy         #'0'
            bne         @out
            lda         od_n
            beq         @next
@out:
            tya
            jsr         vt_key
            inc         od_n
@next:
            inx
            cpx         #4
            bcc         @power
            lda         vt_a
            ora         #'0'
            jmp         vt_key

; .A into the loaded window's answers (cons.s's key_next gives them to a raw reader as they came, before its keys;
; dropped if they're full).  Keeps .X, .Y
vt_key:
            phx
            phy
            pha
            ldx         vt_w
            lda         ans_n,X
            cmp         #ANS_SIZE
            bcs         @full
            stz         vk + 1                              ; (Its place: the window * ANS_SIZE + n)
            txa
            asl
            asl
            asl
            asl
            asl
            rol         vk + 1
            clc
            adc         #<ans_buf
            sta         vk
            lda         vk + 1
            adc         #>ans_buf
            sta         vk + 1
            ldy         ans_n,X
            pla
            sta         (vk),Y
            inc         ans_n,X
            inc         TASK_EVENT
            ply
            plx
            rts
@full:
            pla
            ply
            plx
            rts

.assert     ANS_SIZE = 32 .and LBL_SIZE = 32 .and WIN_MAX <= 16, error, "vt_key and lbl_at: 32 bytes a window"

.segment "RODATA2"
; ****************************************************************************
; The tables

V52_N       = 15                                            ; VT52's sequences (after ESC)
v52_final:  .byte       "ABCDHJKIFGYZ=><"
v52_vec:    .word       v52_csi, v52_csi, v52_csi, v52_csi, v52_csi, v52_csi, v52_csi, v52_ri, v52_gfx, v52_ascii
            .word       v52_y, v52_id, e_deckpam, e_deckpnm, v52_ansi
.assert     * - v52_vec = V52_N * 2, error, "v52_final and v52_vec don't match"
state_vec:  .word       0, st_esc, st_esci, st_csi, st_csii, st_csix, st_osc, st_stre, st_str, st_stre, st_y1, st_y2
ESC_N       = 10
esc_final:  .byte       "78DEHMZc=>"
esc_vec:    .word       e_decsc, e_decrc, e_ind, e_nel, e_hts, e_ri, e_decid, e_ris, e_deckpam, e_deckpnm
.assert     * - esc_vec = ESC_N * 2, error, "esc_final and esc_vec don't match"
CSI_N       = 37
csi_final:  .byte       "@ABCDEFGHIJKLMPSTXZ`abcdefghlmnrsuqtx"
csi_vec:    .word       x_ich, x_cuu, x_cud, x_cuf, x_cub, x_cnl, x_cpl, x_cha, x_cup, x_cht, x_ed, x_el, x_il
            .word       x_dl, x_dch, x_su, x_sd, x_ech, x_cbt, x_cha, x_cuf, x_rep, x_da, x_vpa, x_vpr, x_cup
            .word       x_tbc, x_sm, x_rm, x_sgr, x_dsr, x_stbm, x_scosc, x_scorc, x_decll, x_xtwin, x_reqtparm
.assert     * - csi_vec = CSI_N * 2, error, "csi_final and csi_vec don't match"
DECM_N      = 9
decm_n:     .byte       1, 3, 4, 5, 6, 7, 25, 2, 47
decm_vec:   .word       d_ckm, d_colm, d_sclm, d_scnm, d_om, d_awm, d_tcem, d_anm, d_alt
.assert     * - decm_vec = DECM_N * 2, error, "decm_n and decm_vec don't match"
sgr_on:     .byte       0, F_BOLD, F_DIM, 0, F_UL, F_BLINK, F_BLINK, F_REV, F_INVIS, 0     ; (SGR 0-9: 6 as 5)
sgr_off:    .byte       <~(F_BOLD | F_DIM), $FF, <~F_UL, <~F_BLINK, $FF, <~F_REV, <~F_INVIS, $FF  ; (22-29)
SGR_N       = 6
sgr_bit:    .byte       F_BOLD, F_DIM, F_UL, F_BLINK, F_REV, F_INVIS
sgr_num:    .byte       "124578"
greys:      .byte       0, 0, 8, 8, 7, 15                   ; (232-255, by 4s)
bits:       .byte       1, 2, 4, 8, 16, 32, 64, 128
p10l:       .byte       <10000, <1000, <100, <10            ; (key_dec16's)
p10h:       .byte       >10000, >1000, >100, >10
RQM_N       = 9                                             ; DECRQM's private modes: those kept (a bit of v_mode;
RQM_SCLM    = 2                                             ;   v_mode2's, ?4), and those fixed (3: no 132 columns,
rqm_n:      .byte       1, 3, 4, 5, 6, 7, 8, 25, 2          ;   always reset; 8: autorepeat, always set; 2: ANSI,
rqm_bit:    .byte       VM_CKM, 0, VM2_SCLM, VM_SCNM, VM_OM, VM_AWM, 0, VM_TCEM, 0 ;   set, as VT52 mode asks
rqm_fixed:  .byte       0, 4, 0, 0, 0, 0, 3, 0, 1           ;   nothing)
; The DEC Special Graphics ($5F-$7E, the cells' $00-$1F) as ISO-8859-15, where the font has no glyph for them yet:
; blank, diamond, checkerboard, HT FF CR LF, degree, plus/minus, NL VT, the corners and crossing, the scan lines,
; the tees, the bars, less and greater or equal, pi, not equal, pound, middle dot
dec_ascii:  .byte       ' ', '*', '#', 'H', 'F', 'C', 'L', $B0, $B1, 'N', 'V', '+', '+', '+', '+', '+'
            .byte       '-', '-', '-', '-', '_', '+', '+', '+', '+', '|', '<', '>', 'p', '#', $A3, $B7
s_ser_clear: .byte      ESC, "[0m", ESC, "(B", ESC, ")B", SI, ESC, "[?6l", ESC, "[4l", ESC, "[?7h", ESC, "[20l", ESC, "[?5l"
            .byte       ESC, "[r", ESC, "[H", ESC, "[2J", 0
s_scr_sgr0: .byte       ESC, "[0m", 0
s_scr_sgr7: .byte       ESC, "[0;7m", 0
s_scr_clear: .byte      ESC, "(B", ESC, ")B", SI, ESC, "[r", ESC, "[H", ESC, "[2J", 0
s_om:       .byte       ESC, "[?6h", 0
s_awm_off:  .byte       ESC, "[?7l", 0
s_irm_on:   .byte       ESC, "[4h", 0
s_lnm_on:   .byte       ESC, "[20h", 0
s_scnm_on:  .byte       ESC, "[?5h", 0
s_tcem_on:  .byte       ESC, "[?25h", 0
s_tcem_off: .byte       ESC, "[?25l", 0
s_da:       .byte       ESC, "[?6c", 0                      ; (A VT102)
s_dsr_ok:   .byte       ESC, "[0n", 0
s_da2:      .byte       ESC, "[>1;10;0c", 0              ; (A VT220, firmware 10)
s_reptparm: .byte       ";1;1;120;120;1;0x", 0          ; (DECREPTPARM after its reason)
