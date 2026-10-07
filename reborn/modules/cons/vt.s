; ****************************************************************************
; vt - the console driver's second bank (its first: cons.s): each window's screen, kept as cells in the driver's
; RAM banks, a VT100 that writes them, and the terminals drawn from them (docs/plans/WINDOWS.md, W1).
;
; A window's screen: three of the task's banks (vw_bank, a run from BANKS_ALLOC), planes of 64 rows of 128 cells
; at $8000 (a row's cells at $8000 + 128 * the row): the characters (a byte each: the font's, ISO-8859-15; $00-$1F
; the DEC Special Graphics set's 32, its $5F-$7E), the colours (the background << 4 | the foreground: conio's
; 0-15) and the rendition (F_*).  The 64 rows are a pool and the screen a map into it (v_rmap: the screen's rows
; from v_sb0 on, the scrollback's before them, its last v_sbn the newest last).  So a scroll moves the map's
; entries, not the rows: a row that goes off the top of the whole screen (or of a region at its top) joins the
; scrollback, and the oldest scrollback row comes in, blanked, at the region's bottom.  A window is 80 x 24 here
; (W1: W3 sizes them), so it has 40 rows of scrollback.
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

VT_COLS         = 80            ; A window's size (W1)
VT_ROWS         = 24
POOL            = 64            ; A window's rows: a plane's (128 cells each)
VT_NPAR         = 16            ; A sequence's numbers, at most
VT_RAW          = 40            ; A sequence's bytes kept, to pass it on as it came
VS_PAGE         = 256           ; A window's state in vt_save
SCR_BUF         = 240           ; The screen's bytes, a write to #v/term at a time
F_BOLD          = $01           ; The rendition: bold ...
F_DIM           = $02           ;   faint ...
F_UL            = $04           ;   underlined ...
F_BLINK         = $08           ;   blinking ...
F_REV           = $10           ;   reversed ...
F_INVIS         = $20           ;   invisible ...
F_PROT          = $40           ;   protected (DECSCA: W2)
META            = 127           ; A row's last cell, its meta (a window has 127 columns at most): the characters'
                                ;   plane's its blank end's first column (the cells from there on are blank, not
                                ;   written: a scroll or an erase to the row's end is a byte, not a row's cells);
                                ;   the colours' that end's colours; the rendition's the row's attributes (RA_*)
RA_CONT         = $01           ; A row's attribute: an autowrap continued the row before it into it
COL_DEF         = $07           ; The colours at first: light grey on black (SGR 39, 49)
VM_AWM          = $01           ; v_mode: autowrap (DECAWM) ...
VM_OM           = $02           ;   origin (DECOM) ...
VM_IRM          = $04           ;   insert (IRM) ...
VM_LNM          = $08           ;   LF as a new line (LNM) ...
VM_TCEM         = $10           ;   the cursor shown (DECTCEM) ...
VM_SCNM         = $20           ;   the screen reversed (DECSCNM: W2) ...
VM_CKM          = $40           ;   the cursor keys' application mode (DECCKM) ...
VM_KPAM         = $80           ;   the keypad's (DECKPAM)
VM2_SCLM        = $01           ; v_mode2: smooth scrolling (DECSCLM)
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
G_ERROR         = $02           ; SUB's character: the DEC checkerboard

.zeropage
vrp:        .res        2                                   ; The cursor's row (its cells in a plane)
vq:         .res        2                                   ; A row
vr:         .res        2                                   ;   and another (a row's cells moved)
vch:        .res        1                                   ; The byte being parsed
vt_a:       .res        2                                   ; Scratch

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
v_rmap:     .res        POOL                                ; The map: the rows' places in the pool, a ring ...
v_rbase:    .res        1                                   ;   from here (the whole screen's scroll turns it)
vs_last:
VS_N        = vs_last - vs_first
.assert     VS_N < 256 .and VS_N <= VS_PAGE, error, "A window's state is a page at most"
vb0:        .res        1                                   ; Its planes' banks: the characters, the colours, the
vb1:        .res        1                                   ;   rendition
vb2:        .res        1
vt_w:       .res        1                                   ; The window loaded ($FF: none)
vw_bank:    .res        WIN_MAX                             ; Each window's first bank (0: none)
vt_save:    .res        WIN_MAX * VS_PAGE                   ; Each window's state, while another's is loaded
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
sp_full:    .res        1                                   ;   <> 0: past the last column (the row before painted
                                                            ;   to its end)
scr_x:      .res        1                                   ; vid's cursor as it is ($FF: not known) ...
scr_y:      .res        1
scr_wrap:   .res        1                                   ;   <> 0: past its last column (its next character
                                                            ;   wraps: not used, a CUP comes first) ...
scr_c:      .res        1                                   ;   its colours and rendition ($FF: not known) ...
scr_f:      .res        1
scr_sync:   .res        1                                   ;   <> 0: its cursor to be put where the window's is
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
tlen:       .res        POOL                                ;   each of its rows' length (trailing blanks off) ...
tc_w:       .res        1                                   ;   for this window ($FF: none) ...
tc_n:       .res        1                                   ;   its rows

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
            jsr         vt_unload                           ; (Its state isn't in vt_save yet: made in place)
            lda         vt_i
            sta         vt_w
            jsr         banks
            lda         #VT_COLS
            sta         v_cols
            lda         #VT_ROWS
            sta         v_rows
            lda         #POOL - VT_ROWS
            sta         v_sb0
            stz         v_sbn
            ldx         #POOL - 1                           ; The map: each row its own
:
            txa
            sta         v_rmap,X
            dex
            bpl         :-
            stz         v_rbase
            jsr         reset                               ; (The screen cleared)
            clc
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
            lda         vw_bank,X
            stz         vw_bank,X
            ldx         #3
            jmp         BANKS_FREE

; The cnt bytes in iobuf, the output of window lw: as many as there's room for (the shown window, its terminal
; following on the serial port: VT_ROOM a byte in the send ring).  OUT: .A = the bytes taken
vt_write:
            lda         lw
            jsr         vt_load
            jsr         fw_setup
            stz         vw_k
@byte:
            ldx         vw_k
            cpx         cnt
            bcs         @done
            lda         fw_ser
            beq         :+
            jsr         tx_free
            cmp         #VT_ROOM
            bcc         @done
:
            ldx         vw_k
            lda         iobuf,X
            jsr         vt_byte
            inc         vw_k
            bra         @byte

@done:
            lda         vw_k
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
            lda         ts_scr
            beq         @sync
            jsr         scr_paint
            stz         ts_scr
            bra         @flush

@sync:
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
            clc
            lda         v_sbn
            adc         v_rows
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

; vq = /text's row vt_j: the scrollback's, oldest first, then the screen's
text_row:
            sec
            lda         v_sb0
            sbc         v_sbn
            clc
            adc         vt_j
            jsr         map_row
            jmp         pool_ptr

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

; vb0-vb2: the loaded window's banks
banks:
            ldx         vt_w
            lda         vw_bank,X
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

; After ESC
st_esc:
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

; In a string: nothing (W4: a title)
st_str:
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
            lda         vt_w                                ; The shown window's: the bell (cons.s rings it)
            cmp         w_in
            bne         :+
            lda         #1
            sta         bell
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
            bne         @pass                               ; (DECDHL, DECDWL, DECSWL: W2; the serial port's)
            jsr         m_align
            jsr         scr_dirty
            jmp         fs_raw

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
            cpx         #'?'
            bne         @drop                               ; (> = <: W2)
            cmp         #'h'
            beq         x_decset
            cmp         #'l'
            beq         x_decrst
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
            bne         @next
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

d_sclm:                                                     ; ?4: kept (W2: jump scroll)
            lda         vd_set
            sta         v_mode2
            jmp         fs_raw

d_scnm:                                                     ; ?5: kept (W2: shown)
            lda         #VM_SCNM
            jsr         mode_bit
            jmp         fs_raw

d_om:                                                       ; ?6: the cursor home
            lda         #VM_OM
            jsr         mode_bit
            jsr         m_home
            jsr         fc_lost
            jmp         fs_raw

d_awm:                                                      ; ?7
            lda         #VM_AWM
            jsr         mode_bit
            stz         v_wrap
            jsr         fc_lost
            jmp         fs_raw

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
            jmp         fs_raw

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
            jmp         fs_raw
:
            pha
            jsr         m_ed
            pla
            jsr         fc_erase
            lda         #'J'
            jsr         fc_erase_end
            jmp         fs_raw
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

; DSR: 5, the state (all's well); 6, the cursor's place (CPR: with DECOM, in the region)
x_dsr:
            lda         v_parh
            bne         @done
            lda         v_parl
            cmp         #5
            beq         @ok
            cmp         #6
            bne         @done
            lda         #ESC
            jsr         vt_key
            lda         #'['
            jsr         vt_key
            lda         v_mode                              ; (The row: with DECOM, from the region's top)
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

@ok:
            ldx         #0
:
            lda         s_dsr_ok,X
            beq         @done
            jsr         vt_key
            inx
            bra         :-
@done:
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
            jmp         fs_raw
@done:
            rts

; DECSTR: a soft reset (not the screen, nor the cursor's place)
x_decstr:
            jsr         soft
            jsr         fc_region
            jmp         fs_raw

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
            cpy         v_cols
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
            lda         #1
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

; The margins the whole screen
full_margins:
            stz         v_top
            ldx         v_rows
            dex
            stx         v_bot
            rts

; A soft reset (DECSTR): the modes, the margins, the rendition, the character sets, the saved cursor
soft:
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
            stz         v_state
            stz         v_rawn
            stz         v_sbn
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
            jsr         map_x
            lda         v_rmap,X
            pha
            lda         su_i
            jsr         map_x
            pla
            sta         v_rmap,X
            inc         su_i
            bra         @step
@last:
            jsr         map_x
            lda         su_r
            sta         v_rmap,X
@in:
            lda         su_l                                ; The row in at the bottom, blank
            jsr         map_row
            jsr         pool_ptr
            jsr         blank_row
            lda         su_f                                ; (Into the scrollback: it's a row longer)
            bne         :+
            lda         v_sbn
            cmp         v_sb0
            bcs         :+
            inc         v_sbn
:
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
            jsr         map_x
            lda         v_rmap,X
            pha
            lda         su_i
            jsr         map_x
            pla
            sta         v_rmap,X
            dec         su_i
            bra         @step
@first:
            jsr         map_x
            lda         su_r
            sta         v_rmap,X
            jsr         pool_ptr
            jsr         blank_row
            dec         su_n
            bne         @one
            jmp         cur_row

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

; .X = the map's entry for its place .A (from the ring's start)
map_x:
            clc
            adc         v_rbase
            and         #POOL - 1
            tax
            rts

; .A = the pool's row at the map's place .A
map_row:
            jsr         map_x
            lda         v_rmap,X
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

; The row at vq blank: its blank end all of it, in the background's colours (BCE); no attributes
blank_row:
            jsr         erase_col
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
            lda         ts_ser
            cmp         #1
            bne         @rows
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
            stz         sp_row
            stz         sp_col
            stz         sp_cy
            stz         sp_cx
            stz         sp_full
            lda         #COL_DEF
            sta         sp_c
            stz         sp_f
            stz         sp_dec
            lda         #2
            sta         ts_ser
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
            beq         @next
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
            stz         ts_ser
            inc         TASK_EVENT                          ; (The window's writers, waiting, look again)
@wait:
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
            cmp         #$20
            bcs         :+
            tax
            lda         dec_ascii,X
:
            jsr         scr_put
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
            lda         v_y
            cmp         scr_y
            bne         @cup
            lda         v_x
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
            ldx         v_x
            jsr         out_cup
            lda         v_y
            sta         scr_y
            lda         v_x
            sta         scr_x
@done:
            rts

; vid's colours and rendition the window's (SGR's canonical form, if they aren't)
fc_sgr:
            lda         v_col
            ldx         v_fl
fc_sgr_ax:
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

; The screen painted, all at once: the shown window's screen, its region, cursor and rendition
scr_paint:
            lda         w_in
            jsr         vt_load
            ldx         #<s_scr_clear
            ldy         #>s_scr_clear
            jsr         out_str
            lda         #COL_DEF
            sta         scr_c
            stz         scr_f
            stz         vt_i
@row:
            lda         vt_i
            cmp         v_rows
            bcs         @state
            jsr         row_ptr
            jsr         row_last
            sta         vt_k
            beq         @next
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
            cmp         #$20
            bcs         :+
            tax
            lda         dec_ascii,X
:
            jsr         scr_put
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
            rts

; .A into the screen's buffer (written when it's full).  Keeps .X
scr_put:
            ldy         scr_n
            sta         scr_buf,Y
            iny
            sty         scr_n
            cpy         #SCR_BUF
            bcc         :+
            phx
            jsr         scr_flush
            plx
:
            rts

; The screen's buffer to #v/term (a write that fails: the screen gone, none from then on)
scr_flush:
            lda         scr_n
            beq         @done
            sta         r1
            stz         r1 + 1
            LDR         r0, scr_buf
            lda         scr_fd
            jsr         WRITE
            bcc         :+
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

; A CUP to row .A, column .X (from 0)
out_cup:
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
            lda         v_top
            inc         a
            jsr         out_dec
            lda         #';'
            jsr         out
            lda         v_bot
            inc         a
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
            jsr         vt_key
:
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
            beq         :+
            ora         #'0'
            jsr         vt_key
:
            pla
            ora         #'0'
            jmp         vt_key

; .A into the loaded window's keys (dropped if they're full).  Keeps .X
vt_key:
            phx
            pha
            ldx         vt_w
            lda         w_iqh,X
            inc         a
            and         #INQ_SIZE - 1
            cmp         w_iqt,X
            beq         @full
            sta         vt_a
            txa                                             ; (Its queue: 64 * the window)
            lsr
            ror
            ror
            ora         w_iqh,X
            tay
            pla
            sta         inq,Y
            lda         vt_a
            sta         w_iqh,X
            inc         TASK_EVENT
            plx
            rts
@full:
            pla
            plx
            rts

.segment "RODATA2"
; ****************************************************************************
; The tables

state_vec:  .word       0, st_esc, st_esci, st_csi, st_csii, st_csix, st_str, st_stre, st_str, st_stre
ESC_N       = 10
esc_final:  .byte       "78DEHMZc=>"
esc_vec:    .word       e_decsc, e_decrc, e_ind, e_nel, e_hts, e_ri, e_decid, e_ris, e_deckpam, e_deckpnm
.assert     * - esc_vec = ESC_N * 2, error, "esc_final and esc_vec don't match"
CSI_N       = 34
csi_final:  .byte       "@ABCDEFGHIJKLMPSTXZ`abcdefghlmnrsu"
csi_vec:    .word       x_ich, x_cuu, x_cud, x_cuf, x_cub, x_cnl, x_cpl, x_cha, x_cup, x_cht, x_ed, x_el, x_il
            .word       x_dl, x_dch, x_su, x_sd, x_ech, x_cbt, x_cha, x_cuf, x_rep, x_da, x_vpa, x_vpr, x_cup
            .word       x_tbc, x_sm, x_rm, x_sgr, x_dsr, x_stbm, x_scosc, x_scorc
.assert     * - csi_vec = CSI_N * 2, error, "csi_final and csi_vec don't match"
DECM_N      = 7
decm_n:     .byte       1, 3, 4, 5, 6, 7, 25
decm_vec:   .word       d_ckm, d_colm, d_sclm, d_scnm, d_om, d_awm, d_tcem
.assert     * - decm_vec = DECM_N * 2, error, "decm_n and decm_vec don't match"
sgr_on:     .byte       0, F_BOLD, F_DIM, 0, F_UL, F_BLINK, F_BLINK, F_REV, F_INVIS, 0     ; (SGR 0-9: 6 as 5)
sgr_off:    .byte       <~(F_BOLD | F_DIM), $FF, <~F_UL, <~F_BLINK, $FF, <~F_REV, <~F_INVIS, $FF  ; (22-29)
SGR_N       = 6
sgr_bit:    .byte       F_BOLD, F_DIM, F_UL, F_BLINK, F_REV, F_INVIS
sgr_num:    .byte       "124578"
greys:      .byte       0, 0, 8, 8, 7, 15                   ; (232-255, by 4s)
bits:       .byte       1, 2, 4, 8, 16, 32, 64, 128
; The DEC Special Graphics ($5F-$7E, the cells' $00-$1F) as ISO-8859-15, where the font has no glyph for them yet:
; blank, diamond, checkerboard, HT FF CR LF, degree, plus/minus, NL VT, the corners and crossing, the scan lines,
; the tees, the bars, less and greater or equal, pi, not equal, pound, middle dot
dec_ascii:  .byte       ' ', '*', '#', 'H', 'F', 'C', 'L', $B0, $B1, 'N', 'V', '+', '+', '+', '+', '+'
            .byte       '-', '-', '-', '-', '_', '+', '+', '+', '+', '|', '<', '>', 'p', '#', $A3, $B7
s_ser_clear: .byte      ESC, "[0m", ESC, "(B", ESC, ")B", SI, ESC, "[?6l", ESC, "[4l", ESC, "[?7h", ESC, "[20l"
            .byte       ESC, "[r", ESC, "[H", ESC, "[2J", 0
s_scr_clear: .byte      ESC, "[0m", ESC, "[r", ESC, "[H", ESC, "[2J", 0
s_om:       .byte       ESC, "[?6h", 0
s_awm_off:  .byte       ESC, "[?7l", 0
s_irm_on:   .byte       ESC, "[4h", 0
s_lnm_on:   .byte       ESC, "[20h", 0
s_tcem_on:  .byte       ESC, "[?25h", 0
s_tcem_off: .byte       ESC, "[?25l", 0
s_da:       .byte       ESC, "[?6c", 0                      ; (A VT102)
s_dsr_ok:   .byte       ESC, "[0n", 0
