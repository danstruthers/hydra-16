; ****************************************************************************
; cons - the console driver (docs/design/reimplementation-from-scratch.md, §14.2): the serial port, its rings, and the
; devices #c and #P (/pc, a folder on the PC), on srvlib (a boot driver: task F).
;
; Windows, Plan 9's way (rio's, on a text terminal), not job control: several consoles on the one terminal, each a
; window with its own cons and consctl, line editor, raw mode, note group and screen (vt.s, the second bank: cells
; in the driver's RAM banks, written by a VT100).  One window is shown and gets the keys; the others run on, their
; output going to their screens, their reads waiting for keys.  A window's files are #c with its number as the spec: #c2/cons (or mount '#c' /dev 2); #c is window 0.
;   /cons       the window's console.  A read gets a line, edited here (cooked): Backspace and Delete, Left, Right,
;               Home and End (and Ctrl-A, Ctrl-E), Ctrl-U, the history with Up and Down; Enter ends it, Ctrl-D on
;               an empty line is the end of the input (a line longer than the window's row goes on to the rows
;               below, and the cursor up and down them; a resize draws it again at the new width).  Or (raw: consctl's rawon) each key as it comes, the
;               terminal's cursor and function keys as one code each (KEY_*; an Escape alone is a key once
;               ESC_TICKS have passed with nothing after it; KEY_RESIZE when the window's size changed, keys hydra's).
;               A write goes to the window's screen, and out to the
;               terminals if the window is shown (each LF as CR LF; a BEL rings the sound driver's bell too,
;               #a/bell: one of the calls from a driver to another, the screen's #v/term another)
;   /consctl    rawon, rawoff (raw lasts till the window's last consctl closes, as Plan 9's does); keys vt, keys
;               hydra (raw's keys: as a VT100 sends them, following the window's DECCKM, DECKPAM and VT52 mode, or
;               as one code each, KEY_*: as it starts, and again with its last consctl), keys mods (hydra's, and a
;               key the terminal sent modified, xterm's way, CSI 1 ; m A or CSI n ; m ~, as KEY_MOD, its modifiers
;               (m - 1: 1 Shift, 2 Alt, 4 Ctrl), then the key); scroll smooth, scroll
;               jump (the window shown on the serial port: every byte its writers write goes out, they waiting for
;               the line; or they go on, the terminal painted as it can, skipping what came between); group (the
;               window's notes go to the writer's note group); screen, serial, both (where the windows are shown:
;               every window's, the console's terminals: the Vera X's screen, the serial port, or both, as it
;               starts; screen with no screen: E_NODEV; terminal screen, serial, both too); terminal size C R (the
;               serial port's terminal's columns and rows: 80 x 24 as it starts; terminal size alone asks it, ESC [
;               18 t, and its answer sets it, as the PC tool's report does, ESC [ 8 ; R ; C t, sent as its window
;               changes).  It reads as the state, with the window's size (size C R): the smaller of the terminals
;               it's shown on, each whole (the screen's from vid's ctl, mode CxR), 127 x 64 at most
;   /wctl       new (a window), current N (window N shown); the chrome (W4): bar top, bar bottom, bar off, bar FORMAT
;               (console-wide), header FORMAT, footer FORMAT, header on|off, footer on|off, chrome screen|serial|both
;               on|off [bar] [header] [footer], status TEXT, monitor on|off (the window's), default header FORMAT,
;               default footer FORMAT, default chrome ... (new windows', and those still as the defaults were); history
;               N (the window's rows past its scrollback's, 64 at a time, 128 at most; emptied); key KEY ACTION, key
;               prefix KEY (the keys: below; console-wide).  It reads as the windows, a line each (*
;               the shown one); layout tabs|rows|columns|grid (the writer's window's group's: its windows one at a time, or tiled)
;   /label      the window's title (OSC 0 and 2 write it too), read and written whole; empty: its program's name
;   /wnew       a read waits for the user's Ctrl-] c, then makes a window, shown, and gives its number (init's: it
;               starts a shell there)
;   /ser        the serial port, raw: bytes in and out as they are.  While it's open for reading, the line is its
;               (xmodem's): every byte in is its, Ctrl-C and the rest too, and the windows' text (and /pc's frames)
;               wait, kept as a hidden window's is, till its last close repaints the window shown
;   /serctl     the rate: b300, b600, b1200, b2400, b4800, b9600, b19200, b115200.  It reads as it
;   /kbdin      a write's bytes are the window's keys, as if typed (rio's kbdin: a line sent to another window's
;               shell, forth's send); all of them, as its keys' queue has room, the writer waiting for the rest
;   /text       the window's scrollback and screen as text, a line a row (rio's)
;   /snarf      the console's cut buffer (rio's), one for all its windows: SNARF_MAX bytes at most, in a bank of
;               its own (taken at the first write).  A write at its start empties it first (a write replaces it);
;               Ctrl-] y pastes it into the window shown as its keys (an LF a CR, as a terminal's paste), between
;               CSI 200 ~ and CSI 201 ~ if its program asked for bracketed paste (?2004), which only a keys vt
;               reader gets (the decoder drops what isn't a key)
; The scrollback's view (W6b): Ctrl-] [ or Shift-PgUp (taken from the window's keys while it's bound) shows the
; window shown's lines (its scrollback's and screen's) in a window of the console's own, its program going on
; meanwhile.  The arrows, PgUp, PgDn, Home and End move it; Space marks the cursor's line, and the lines from it to
; the cursor's are the selection; Enter copies the selection (none: the cursor's line) to /snarf, a line each, and
; leaves; q, Escape twice, or its key again leaves.  Its footer has %y, its place
; The chrome (W4): a terminal shows the bar (a row, console-wide, at its top or its bottom) and the shown window's
; header and footer (a row each, above and below its screen), as that window's chrome is on there (w_chr; by default
; all of it on the screen, none on the serial port).  Each is rendered from its format (chr_render: %n its number, %l
; its label, %p its program, %s its status line (DECSASD's, or status's), %w and %G the windows, %c %r its size, %m its
; modes, %t %d the time and the date, %L the LEDs, %= the rest to the right, %[...] SGR's rendition, %% a %); vt.s
; draws them.  A window's size is the smaller of the terminals it's shown on, each less its chrome rows there.
; The windows' groups (W5): a group a shell session.  Ctrl-] c asks for a new one (for /wnew's reader: wstart starts
; a shell there); a window made by wctl's new joins the writer's window's group (new group: one of its own).  Each
; group keeps the window it last showed (its focus).  The keys: Ctrl-] then a digit shows that window; Ctrl-] Tab,
; or Ctrl-Tab (xterm's ESC [ 27 ; 5 ; 9 ~, CSI u's ESC [ 9 ; 5 u: the terminal's), the group's next window, Ctrl-]
; Shift-Tab or Ctrl-Shift-Tab its previous (a raw reader of the group's windows gets KEY_FOCUS and the window's
; number); Ctrl-] n and Ctrl-] p the next and previous group; Ctrl-] x the window shown's group a hangup note;
; Ctrl-] h holds the window shown's output, its writers waiting, till Ctrl-] h again (the VT100's No Scroll);
; Ctrl-] Ctrl-] is a Ctrl-]); Ctrl-] w lists the windows; Ctrl-C and Ctrl-\ are notes (interrupt, kill) to the shown
; window's note group, in either mode.  A window goes when the last of its cons fids closes (but window 0).  These
; are bindings (W5d), wctl's key lines change them: key prefix ^X or ctrl-X (a control: not Ctrl-C, Ctrl-\,
; Escape, CR or LF); key KEY ACTION, KEY after the prefix a character (not a digit: Ctrl-] and a digit is always that
; window), ^X or ctrl-X, tab or shift-tab; key ctrl-tab ACTION, key ctrl-shift-tab ACTION; ACTION next, previous (the group's windows),
; next-group, previous-group, new (a group: Ctrl-] c's), list, hold, close, paste (Ctrl-] y), scrollback (Ctrl-]
; [), split, vsplit (Ctrl-] s, v: a window with a shell in the group, tiled below or beside), zoom (Ctrl-] z: a
; tiled group's focus alone, or tiled again) or none; key shift-pgup ACTION too (scrollback: the view a page up).
; Ctrl-] and an arrow: the group's previous (up, left) or next window (down, right).  The list (Ctrl-] w) is a window
; of the console's own, shown till a window's key (its number in hex), or the arrows and Enter, shows that one; q,
; Escape twice, or the list's key again shows the one before.
;
; Receiving: the ACIA's interrupt (LINE_ACIA) puts each byte into the receive ring and adds 1 to the event count
; (TASK_EVENT: the clients waiting look again); before each request the keys are handed to the windows' queues
; (Ctrl-] and the key after it acted on there).  Sending: the shown window's output goes into the send ring as it's
; written (vt.s: the terminal following it), as there's room; a window just shown is painted from its screen, as
; there's room, after each request.  VIA timer 2 (LINE_VIA_T2) runs a character's time and a margin, and its
; interrupt sends the next byte of the send ring.  Paced, at every rate, on both chips: the WDC W65C51N's TDRE
; doesn't work, and on the board the Rockwell's sending back to back at 115200 loses characters (2 idle bits then;
; 1 otherwise).  The interrupts' work is a few dozen cycles each: the IRQs-off budget (200 cycles) has the
; dispatch's 115 in it.
;   The screen (the Vera X's, phase 8: docs/design/plans/VIDEO.md) is a second terminal, its driver's (vid: #v/term, an
; ANSI terminal; opened the first time, with no screen nothing from then on): the shown window's output goes there
; too, as vt.s makes it show the window's screen (written at each request's end), and a window shown is painted
; there all at once.  With the serial port off (consctl's screen), the shown window takes its writes whole, as a
; hidden one does: nothing paces it but the screen.
;
; /pc (#P, docs/design/plans/PC.md): a folder on the PC, served by the PC tool (sim/tools/hydrapc.js, which is the
; terminal too) over the serial port, in frames between the console's bytes (sim/lib/pcproto.js): PC_MARK, then the
; type, the tag, the payload's length (2) and the payload, and a CRC-16 of those, PC_MARK and PC_ESC stuffed (PC_ESC,
; then the byte ^ $20; the PC's frames stuff Ctrl-C, Ctrl-\ and Ctrl-] too, which the irq entry acts on).  Version
; 2 of what they carry: a request is its request block (RQ_SIZE bytes) and its data (a name, a write's bytes, a stat
; record); the reply its status (0, or an error), the fid, the count done, the qid type, and its data (a read's, a
; stat record).  So the PC tool does each request as HydraFS's #f would (sim/tools/pcfs.js).
;   A request goes out as a frame (the send ring's, ahead of the windows' text), and its client waits (E_AGAIN);
; the frame back is taken out of the receive ring as the keys are handed out, and the client, asking again, gets
; its answer from it.  One request at a time: another client waits for it (the event count changes as it ends).
; A frame back damaged (its CRC) or stopping part-way (PC_QUIET ticks: a byte lost, an overrun at 115200), the PC's
; PC_T_NAK, or no answer in its time: the request again (the PC tool answers a tag it's just answered with its last
; reply, not doing it twice), PC_TRIES tries in all; then the PC is taken as gone (E_IO).  The first request, or the
; first after that, attaches first (PC_T_ATTACH: the PC tool forgets the fids it had, and learns the version), one
; try in PC_WAIT_ATTACH ticks: with no PC tool, E_IO that soon.  While a request is out, timer 2 runs on when
; there's nothing to send, adding to the event count every PC_NAP rounds, so a client waiting for a reply that
; doesn't come looks at the time.  Frames carry PC_DATA bytes of data at most, so a reply fits the receive ring.

.include "hydra.inc"
.include "hw.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "srvlib.inc"
.include "cons.inc"

            HYX2_DRIVER "cons", init, srv_serve, irq, 0, HF_BOOT, 2

SRV_FLUSH       = flush                                     ; (srvlib: a reader's call ended by a note)
SRV_OPENED      = opened                                    ;   (a fid made: its window)
SRV_CLUNKED     = clunked                                   ;   (a fid forgotten: consctl's counted)
SRV_PRE         = distribute                                ;   (before each request: the keys to the windows)
SRV_POST        = pump                                      ;   (and after it: the terminals painted)

LINE_MAX        = 127           ; A line's length at most (and its LF)
HIST_N          = 4             ; Each window's history: its lines ...
HIST_SIZE       = 128           ;   each its length, then LINE_MAX characters
ST_SIZE         = 32            ; Each window's editor state, kept while another's is in use (st_first on)
KP_SIZE         = 8             ; A key's sequence kept for a raw read (keys vt): its bytes at most
ECHO_ROOM       = LINE_MAX + 40 ; The most a key's echo writes (a key waits for this much room: a line drawn again,
                                ;   its moves)
RX_PAGES        = 4             ; The receive ring's pages: the keys' its first; /ser's all of them (1023 bytes: a 1K
                                ;   XMODEM block at 115200 comes in faster than it can be taken, and waits there)
CTRL_A          = $01
CTRL_C          = $03
CTRL_D          = $04
CTRL_E          = $05
CTRL_U          = $15
CTRL_BSL        = $1C           ; (Ctrl-\)
CTRL_RB         = $1D           ; (Ctrl-]: the windows' key, as it starts: key prefix's)
KA_NONE         = 0             ; The keys' actions (key's: ka_vec, ka_names): none ...
KA_NEXT         = 1             ;   the group's next window, its previous ...
KA_PREV         = 2
KA_GNEXT        = 3             ;   the next group, the previous ...
KA_GPREV        = 4
KA_NEW          = 5             ;   a group wanted (Ctrl-] c's: /wnew's) ...
KA_LIST         = 6             ;   the windows' list ...
KA_HOLD         = 7             ;   the window shown held, or not ...
KA_CLOSE        = 8             ;   its note group a hangup ...
KA_PASTE        = 9             ;   the snarf buffer pasted ...
KA_VIEW         = 10            ;   the scrollback's view ...
KA_SPLIT        = 11            ;   a window in the group, the layout rows (if it was tabs) ...
KA_VSPLIT       = 12            ;   columns ...
KA_ZOOM         = 13            ;   the tiled group's focus alone, or tiled again
KA_N            = 14
SNARF_MAX       = 8192          ; /snarf's bytes, at most: its bank's
LS_ROW          = 3             ; The list's first window's row
RATE_BOOT       = 5             ; 9600: the kernel's bring-up console's
ENT_CONSCTL     = 2             ; srv_tree's consctl (its fids counted)
PC_MARK         = $1E           ; /pc's frames: one starts (from the PC, PC_MARK then PC_ESC is a typed $1E, Ctrl-^)
PC_ESC          = $1F           ;   the next byte ^ $20 is the byte
PC_T_ATTACH     = 'A'           ; The Hydra's: a new session (the PC forgets its fids); its payload: PC_VERSION
PC_T_REQ        = 'Q'           ;   a request: the request block, then its data
PC_T_REPLY      = 'R'           ; The PC's: status, fid, count (2), qid type (the attach's: the version), then data
PC_T_NAK        = 'N'           ;   the request came damaged: send it again
PC_VERSION      = 2             ; (The old system's: 1)
PC_REPLY_HDR    = 5
PC_DATA         = 128           ; A frame's data at most (a read's, a write's)
PC_RX_MAX       = PC_REPLY_HDR + PC_DATA ; A reply's payload at most (a longer frame isn't one of ours)
PC_TX_SIZE      = 4 + RQ_SIZE + PC_DATA + 2 ; The frames: header, payload, CRC
PC_RX_SIZE      = 4 + PC_RX_MAX + 2
PC_TRIES        = 3             ; A request's tries (damaged, or no answer in time); then the PC is gone
PC_WAIT_REQ     = TICK_HZ * 2   ; A reply's time (ticks): the frames' bytes both ways (at 9600, 0.2 s), and the PC's
PC_WAIT_ATTACH  = TICK_HZ       ;   an attach's (no PC tool: E_IO this soon)
PC_STALE        = TICK_HZ       ; A request this long past its time is given up (its client stopped asking)
PC_QUIET        = TICK_HZ / 10  ; A frame coming in that stops this long has lost a byte
PC_NAP          = 8             ; Timer 2's rounds (about 65,000 cycles) between looks at the time, a reply awaited
ESC_NAP         = 2             ;   and an Escape alone (ESC_TICKS: then it's a key, not a sequence's start)
ESC_TICKS       = TICK_HZ / 10
FMT_SIZE        = 64            ; A chrome format, its characters with their zero
CLK_NAP         = 110           ; Timer 2's rounds (about 2 s) between looks at the time, the chrome showing it
TM_LEN          = 19            ; The time's text: "2026-10-07 20:41:05"
PS_ATTACH       = 1             ; pc_step: the attach is out ...
PS_REQ          = 2             ;   the request is out

.zeropage
rx_head:    .res        1                                   ; The receive ring: the irq entry's end ...
rx_tail:    .res        1                                   ;   and the serve entry's (in their pages: /ser's)
rx_hp:      .res        2                                   ; /ser's: the head's page (rx_buf + 256 * n) ...
rx_tp:      .res        2                                   ;   and the tail's
tx_head:    .res        1                                   ; The send ring: the serve entry's end ...
tx_tail:    .res        1                                   ;   and timer 2's
tx_busy:    .res        1                                   ; <> 0: a byte is going (timer 2 runs)
t2_nap:     .res        1                                   ; <> 0: timer 2 runs on with nothing to send (its rounds)
esc_wait:   .res        1                                   ; <> 0: an Escape alone awaited (key_next), timer 2's
                                                            ;   rounds bringing its reader back to look at the time
t2_lo:      .res        1                                   ; Timer 2 for a character: a round's count ...
t2_hi:      .res        1
t2_rounds:  .res        1                                   ;   the rounds (more than 1 at slow rates) ...
t2_left:    .res        1                                   ;   and those left of this one
rate:       .res        1                                   ; The rate (its index in the tables)
pfx:        .res        1                                   ; The irq entry's: <> 0, the last key was Ctrl-] ($FF:
                                                            ;   /ser's, no key acted on) ...
win_grp:    .res        1                                   ;   and the note group of the window with the keys
d_pfx:      .res        1                                   ; Handing the keys out: <> 0, the last was Ctrl-]
w_in:       .res        1                                   ; The window shown, which gets the keys
kb_pfx:     .res        1                                   ; The prefix (Ctrl-]: key prefix's; the irq entry's too)
ser_rd:     .res        1                                   ; /ser's fids for reading (while there are any, the
                                                            ;   line is /ser's: h_ser)
lw:         .res        1                                   ; The window whose editor state is here ($FF: none)
st_first:                                                   ; ---- The loaded window's editor state (ST_N bytes)
ln_len:     .res        1                                   ; The line being edited: its length ...
ln_pos:     .res        1                                   ;   the cursor ...
ln_ready:   .res        1                                   ;   ended: its length with its LF (0: not yet) ...
ln_off:     .res        1                                   ;   how much of it the reads have had ...
eof:        .res        1                                   ;   <> 0: Ctrl-D on an empty line (a read of 0) ...
was_cr:     .res        1                                   ;   and the last key was CR (an LF after it: the same)
esc_st:     .res        1                                   ; A sequence coming in: 0 none, 1 ESC, 2 ESC [, 3 ESC O
esc_n:      .res        1                                   ;   its number (ESC [ n ~) ...
esc_mod:    .res        1                                   ;   its second (the modifiers, xterm's: esc_n's next) ...
esc_semi:   .res        1                                   ;   the ;s past ...
esc_at:     .res        2                                   ;   and the tick its ESC came at
key_pb:     .res        1                                   ; A key put back (the one after an ESC that started
                                                            ;   nothing), or 0
hi_n:       .res        1                                   ; The history: its lines ...
hi_top:     .res        1                                   ;   the newest's slot ...
hi_at:      .res        1                                   ;   and Up and Down's place (0: the line being typed)
raw:        .res        1                                   ; <> 0: raw
ln_geo:     .res        1                                   ; <> 0: where the line is on the screen is known (its
                                                            ;   first key's: the cursor after the prompt) ...
ln_s0:      .res        1                                   ;   its first character's column (the columns: the next
                                                            ;   row's start, the prompt filling its row) ...
ln_w:       .res        1                                   ;   the row's width it's laid out at ...
ln_at:      .res        1                                   ;   the terminal's cursor: its place in the line ...
ln_pend:    .res        1                                   ;   <> 0: past its row's last column, as a terminal is
                                                            ;   after writing there (ln_at at the next row's start) ...
ln_shown:   .res        1                                   ;   and the line's characters on the screen
st_last:                                                    ; ---- (Its end)
ST_N        = st_last - st_first
n:          .res        2                                   ; Scratch
m:          .res        2
p:          .res        2
qp:         .res        2                                   ; A window's key queue (iq_at)
t2_napr:    .res        1                                   ; Timer 2's idle rounds (nap_set's: t2_next's, at once)
cnt:        .res        1

.bss
rx_buf:     .res        RX_PAGES * 256
tx_buf:     .res        256
inq:        .res        WIN_MAX * INQ_SIZE                  ; Each window's keys
lines:      .res        WIN_MAX * (LINE_MAX + 1)            ; Each window's line, while another's is loaded
hist:       .res        WIN_MAX * HIST_N * HIST_SIZE        ; Each window's history
w_state:    .res        WIN_MAX * ST_SIZE                   ; Each window's editor state, while another's is loaded
ln_buf:     .res        LINE_MAX + 1                        ; The loaded window's line
iobuf:      .res        IOBUF
w_used:     .res        WIN_MAX                             ; Each window: <> 0, it's there ...
w_group:    .res        WIN_MAX                             ;   its note group (Ctrl-C's) ...
w_cons:     .res        WIN_MAX                             ;   its cons fids ...
w_ctl:      .res        WIN_MAX                             ;   its consctl fids (raw ends with the last) ...
kvt:        .res        WIN_MAX                             ;   <> 0: keys vt ...
w_kmod:     .res        WIN_MAX                             ;   <> 0: keys mods ...
w_jump:     .res        WIN_MAX                             ;   <> 0: scroll jump ...
w_hold:     .res        WIN_MAX                             ;   <> 0: held (Ctrl-] h) ...
w_rsz:      .res        WIN_MAX                             ;   <> 0: resized (KEY_RESIZE for its raw reader) ...
w_raw:      .res        WIN_MAX                             ;   <> 0: raw (raw's, for its chrome's %m) ...
w_grp:      .res        WIN_MAX                             ;   its group (0-15) ...
w_kf:       .res        WIN_MAX                             ;   the window its group focused, + 1: KEY_FOCUS for its
                                                            ;   raw reader (0: none) ...
w_chr:      .res        WIN_MAX                             ;   its chrome on each terminal (CH_*: the serial port's
                                                            ;   in bits 0-2, the screen's in 4-6) ...
w_rdr:      .res        WIN_MAX                             ;   the task that last read it, + 1 (0: none): %p ...
w_act:      .res        WIN_MAX                             ;   its activity while not shown (ACT_*) ...
w_mon:      .res        WIN_MAX                             ;   <> 0: monitor (its output marked, not shown) ...
w_hfmt:     .res        WIN_MAX * FMT_SIZE                  ;   its header's format, its footer's ...
w_ffmt:     .res        WIN_MAX * FMT_SIZE
w_stat:     .res        WIN_MAX * STAT_SIZE                 ;   and its status line (vt.s's DECSASD, status)
g_used:     .res        WIN_MAX                             ; Each group: <> 0, it's there ...
g_focus:    .res        WIN_MAX                             ;   its window focused (shown with the group) ...
g_lay:      .res        WIN_MAX                             ;   its layout (LAY_*: tabs, rows, columns, grid) ...
g_zoom:     .res        WIN_MAX                             ;   and <> 0: zoomed (its focus alone, as tabs)
mk_grp:     .res        1                                   ; (w_make's: the group, $FF a new one)
fr_w:       .res        1                                   ; (w_free's: the window)
cr_g:       .res        1                                   ; (A group's entry, chr_render's)
fid_new:    .res        SRV_FIDS                            ; Each wctl fid: the window its new made ($FF: none), its
                                                            ;   next read's answer
kw_st:      .res        1                                   ; The terminal's sequences the console takes: how far ...
kw_n:       .res        1                                   ;   which number ...
kw_p:       .res        3                                   ;   and them
want_new:   .res        1                                   ; <> 0: Ctrl-] c, a window wanted (for /wnew's reader)
kb_act:     .res        128                                 ; The keys (key's): each one's action after the prefix
                                                            ;   (KA_*; ESC's: Shift-Tab's, ESC [ Z) ...
kb_ct:      .res        3                                   ;   and Ctrl-Tab's, Ctrl-Shift-Tab's and Shift-PgUp's
ls_w:       .res        1                                   ; The windows' list (Ctrl-] w): its window ($FF: none) ...
ls_from:    .res        1                                   ;   the one shown before it ...
ls_n:       .res        1                                   ;   the windows listed ...
ls_ws:      .res        WIN_MAX                             ;   and them, in order ...
ls_sel:     .res        1                                   ;   the one chosen (in ls_ws: its marker, >) ...
ls_esc:     .res        1                                   ;   its keys' sequence (1: ESC, 2: ESC [ or ESC O) ...
ls_i:       .res        1                                   ;   and scratch
sn_bank:    .res        1                                   ; /snarf: its bank ($FF: none yet) ...
sn_len:     .res        2                                   ;   and its bytes
ps_w:       .res        1                                   ; A paste (Ctrl-] y): its window ($FF: none) ...
ps_i:       .res        2                                   ;   the snarf buffer's next byte ...
ps_ph:      .res        1                                   ;   its part (0 the bracket before, 1 the text, 2 the
ps_k:       .res        1                                   ;   bracket after), that bracket's next byte ...
ps_br:      .res        1                                   ;   and <> 0: bracketed (?2004)
vv_w:       .res        1                                   ; The scrollback's view: its window ($FF: none) ...
vv_src:     .res        1                                   ;   the window it shows ...
vv_top:     .res        1                                   ;   that one's line at its top (0: the oldest) ...
vv_cur:     .res        1                                   ;   its cursor's row ...
vv_ma:      .res        1                                   ;   the marked line ($FF: none), and the cursor's: the
vv_mb:      .res        1                                   ;   selection's ends ...
vv_n:       .res        1                                   ;   the lines, its rows ...
vv_r:       .res        1
vv_add:     .res        2                                   ;   the lines dropped off the oldest end (vt_lines'), as
vv_seen:    .res        2                                   ;   they are and as it last saw them ...
vv_esc:     .res        1                                   ;   its keys' sequence (1: ESC, 2: ESC [, 3: past a ;) ...
vv_num:     .res        1                                   ;   its number ...
vv_i:       .res        1                                   ;   and scratch
bar_pos:    .res        1                                   ; The bar: 0 none, BAR_TOP, BAR_BOTTOM ...
bar_fmt:    .res        FMT_SIZE                            ;   its format
def_chr:    .res        1                                   ; A new window's chrome, header and footer
def_hfmt:   .res        FMT_SIZE
def_ffmt:   .res        FMT_SIZE
chr_dirty:  .res        1                                   ; The terminals whose chrome is to be drawn again (bit 0
                                                            ;   the serial port, 1 the screen)
chr_v:      .res        1                                   ; (A chrome command's: the bits, the parts, a word's end)
chr_p:      .res        1
chr_wl:     .res        1
chr_sep:    .res        1
hf_k:       .res        1                                   ; (header's or footer's)
rl_chg:     .res        1                                   ; (relayout's: a window resized)
cr_c:       .res        CR_MAX                              ; A chrome row rendered: its characters, colours,
cr_a:       .res        CR_MAX                              ;   rendition ...
cr_f:       .res        CR_MAX
cr_n:       .res        1                                   ;   its cells so far ...
cr_w:       .res        1                                   ;   all of them (its width) ...
cr_i:       .res        1                                   ;   the format's character next ...
cr_ca:      .res        1                                   ;   the colours and rendition now ...
cr_cf:      .res        1
cr_rx:      .res        1                                   ;   the right part's first cell (%=; $FF: none) ...
cr_ga:      .res        1                                   ;   and the rendition at it
cr_gf:      .res        1
cr_k:       .res        1                                   ; (Scratch)
chr_pass:   .res        1                                   ; A drawing of the chrome (vt.s's: the time read once)
tm_pass:    .res        1                                   ; The time read (#t/time): the drawing it's for ...
tm_buf:     .res        TM_LEN + 1                          ;   its text ...
tm_fd:      .res        1
clk_on:     .res        1                                   ; <> 0: the chrome has shown the time (drawn again as the
clk_due:    .res        2                                   ;   minute changes: this tick)
ti_buf:     .res        TI_SIZE                             ; A task's TASKINFO (%p: its name)
kp_n:       .res        WIN_MAX                             ;   a key's sequence: its bytes, those read ...
kp_i:       .res        WIN_MAX
kp_buf:     .res        WIN_MAX * KP_SIZE                   ;   and them
w_iqh:      .res        WIN_MAX                             ;   and its keys: the next in, the next out
w_iqt:      .res        WIN_MAX
eg_c:       .res        1                                   ; ed_goto's: the column it goes to ...
eg_x:       .res        1                                   ;   <> 0: the character before written again
kbd_wait:   .res        1                                   ; <> 0: a /kbdin writer waits for a queue's room
bell:       .res        1                                   ; <> 0: a BEL the shown window sent (ring's) ...
bell_st:    .res        1                                   ;   #a/bell: 0 not opened yet, 1 open, 2 none ...
bell_fd:    .res        1                                   ;   and its fd
term:       .res        1                                   ; Where the windows are shown: TERM_SERIAL, TERM_SCREEN
scr_st:     .res        1                                   ; The screen, #v/term: 0 not opened yet, 1 open, 2 none ...
scr_fd:     .res        1                                   ;   and its fd ...
scr_cfd:    .res        1                                   ;   its ctl's (#v/ctl: its size), or $FF ...
scr_chk:    .res        1                                   ;   <> 0: its size to be looked at (opened, a write
                                                            ;   refused: vid's mode may have changed) ...
scr_cols:   .res        1                                   ;   and its size (0: not known yet)
scr_rows:   .res        1
ser_cols:   .res        1                                   ; The serial port's terminal's size (80 x 24, or as set
ser_rows:   .res        1                                   ;   or told)
lay_cols:   .res        1                                   ; The windows' size: the smaller of the terminals shown on
lay_rows:   .res        1
pc_txbuf:   .res        PC_TX_SIZE                          ; /pc: the frame going out ...
pc_rxbuf:   .res        PC_RX_SIZE                          ;   the frame come in ...
pc_req:     .res        RQ_NAMELEN + 1                      ;   the request out, as its client asked it ...
pc_first:                                                   ; ---- (init: 0)
pc_step:    .res        1                                   ;   0, PS_ATTACH, PS_REQ ...
pc_online:  .res        1                                   ;   <> 0: attached ...
pc_tag:     .res        1                                   ;   the tag ...
pc_tries:   .res        1                                   ;   the tries left ...
pc_until:   .res        2                                   ;   the tick its reply's due by ...
pc_len:     .res        1                                   ;   the frame out: its length (with its CRC) ...
pc_txi:     .res        1                                   ;   the next of its bytes into the send ring ...
pc_txe:     .res        1                                   ;   a byte to send as it is first (PC_MARK; an escaped
                                                            ;   one's second), or 0 ...
pc_rxs:     .res        1                                   ;   a frame coming in: 0 none, 1 in, $81 in but skipped ...
pc_rxi:     .res        1                                   ;   its bytes so far ...
pc_rxl:     .res        1                                   ;   all of them (from its header) ...
pc_rxe:     .res        1                                   ;   <> 0: the next is escaped ...
pc_rxf:     .res        1                                   ;   <> 0: it's whole, for the request out ...
pc_rxn:     .res        1                                   ;   its bytes when last looked at (pc_waiting) ...
pc_rxt:     .res        2                                   ;   and the tick then
pc_last:                                                    ; ---- (Its end)
pc_owner:   .res        1                                   ;   the client whose request it is ($FF: none) ...
pc_t:       .res        2                                   ;   scratch
pc_n:       .res        1
pc_k:       .res        1
pc_crc:     .res        2                                   ;   a CRC

.assert     ST_N <= ST_SIZE, error, "A window's editor state is bigger than ST_SIZE"
.assert     WIN_MAX <= 16 .and FMT_SIZE = 64 .and STAT_SIZE = 128 .and LBL_SIZE = 32, error, "fmt_at, stat_at, lbl_ptr"
.assert     PC_TX_SIZE <= 256 .and PC_RX_SIZE <= 256, error, "/pc's frames: 8-bit indexes"
.assert     RQ_NAMELEN < RQ_SIZE .and RQ_FLAGS < RQ_NAMELEN, error, "/pc: pc_same's fields"
.assert     INQ_SIZE = 64 .and WIN_MAX <= 16, error, "iq_at: 64 bytes a window, 4 a page"

.code
; ****************************************************************************
; The driver's init: window 0, its lines, the rate, the ACIA's receive interrupt on, /pc idle, the devices
init:
            HYX2_BANKS_INIT
            ldx         #cnt - rx_head                      ; (Its zero page: all 0)
:
            stz         rx_head,X
            dex
            bpl         :-
            jsr         rx_reset
            ldx         #WIN_MAX - 1
:
            stz         w_used,X
            stz         g_used,X
            dex
            bpl         :-
            ldx         #SRV_FIDS - 1                       ; (wctl's fids: no new's answers)
            lda         #$FF
:
            sta         fid_new,X
            dex
            bpl         :-
            ldx         #pc_last - pc_first - 1
:
            stz         pc_first,X
            dex
            bpl         :-
            lda         #$FF                                ; (No request out)
            sta         pc_owner
            lda         #$FF
            sta         lw
            lda         #TERM_SERIAL | TERM_SCREEN          ; Both terminals (the screen's, if there's one: it's
            sta         term                                ;   painted first, vt_init)
            lda         #80                                 ; The windows' size: the serial port's terminal's (the
            sta         ser_cols                            ;   screen's counted once it's opened, its size known)
            sta         lay_cols
            lda         #24
            sta         ser_rows
            sta         lay_rows
            stz         scr_cols
            stz         scr_rows
            stz         scr_chk
            stz         kw_st
            lda         #CTRL_RB                            ; The keys: the defaults (kb_def's)
            sta         kb_pfx
            ldx         #127
:
            stz         kb_act,X
            dex
            bpl         :-
            ldx         #0
:
            ldy         kb_def,X
            beq         :+
            lda         kb_def + 1,X
            sta         kb_act,Y
            inx
            inx
            bra         :-
:
            lda         #KA_NEXT
            sta         kb_ct
            lda         #KA_PREV
            sta         kb_ct + 1
            lda         #KA_VIEW
            sta         kb_ct + 2
            lda         #$FF
            sta         ls_w
            sta         vv_w
            sta         sn_bank                             ; (/snarf: empty, no bank yet; no paste)
            sta         ps_w
            stz         sn_len
            stz         sn_len + 1
            lda         #$FF
            sta         scr_cfd
            lda         #BAR_TOP                            ; The chrome: the bar at the top; a window's all of it
            sta         bar_pos                             ;   on the screen, none on the serial port
            lda         #(CH_ALL << 4)
            sta         def_chr
            stz         chr_dirty
            stz         clk_on
            stz         chr_pass
            lda         #$FF
            sta         tm_pass
            ldx         #FMT_SIZE - 1                       ; (Its formats)
:
            lda         s_bar_def,X
            sta         bar_fmt,X
            lda         s_head_def,X
            sta         def_hfmt,X
            lda         s_foot_def,X
            sta         def_ffmt,X
            dex
            bpl         :-
            FAR2        vt_init
            lda         #$FF                                ; Window 0: shown, init's note group's, a group of
            jsr         w_make                              ;   its own (0)
            bcs         @done
            lda         #INIT_TASK
            sta         win_grp
            lda         #LINE_ACIA
            jsr         IRQ_OWN
            bcs         @done
            lda         #LINE_VIA_T2
            jsr         IRQ_OWN
            bcs         @done
            ldx         #RATE_BOOT                          ; (The ACIA is at it already: the kernel set it)
            jsr         rate_t2
            sei
            lda         #ACIA_CMD_DTR | ACIA_CMD_TX_ON      ; Its receive interrupt on (its transmit one stays off)
            sta         ACIA_CMD
            lda         ACIA_STATUS
            lda         ACIA_DATA
            cli
            ldx         #0                                  ; The terminal asked its size: ESC [ 18 t (its answer,
:                                                           ;   or the PC tool's, sets the serial port's)
            lda         s_ask,X
            beq         :+
            jsr         tx_put
            inx
            bra         :-
:
            jsr         tx_start
            lda         #'c'
            jsr         SRV_REGISTER
            bcs         @done
            lda         #'P'
            jmp         SRV_REGISTER                        ; (Its error is init's)

@done:
            rts

; ****************************************************************************
; The irq entry: .A = the line.  Short: about 70 cycles at most, and no WAKE (TASK_EVENT).  (The ACIA's comes
; ahead of the VIA's when both are waiting: kernel/common.s's IRQ_VIA.)  It and timer 2's are in the module's RAM
; (its DATA, beside the trampolines), as an interrupt may come while either bank is at $A000: so a module of two
; banks owns its lines
.segment "DATA"
irq:
            cmp         #LINE_VIA_T2
            beq         t2_next
            lda         ACIA_STATUS                         ; (Reading it clears its interrupt)
            and         #ACIA_ST_RDRF
            beq         @none
            lda         ACIA_DATA
            ldx         pfx
            bne         @after
            cmp         #CTRL_C
            beq         @intr
            cmp         #CTRL_BSL
            beq         @kill
            cmp         kb_pfx
            bne         @store
            sta         pfx                                 ; (Ctrl-]: it goes into the ring too)
@store:
            ldy         rx_head                             ; Into the ring
            sta         rx_buf,Y
            iny
            cpy         rx_tail
            beq         @none                               ; (Full: the byte's dropped)
            sty         rx_head
            inc         TASK_EVENT                          ; (The clients waiting look again)
@none:
            lda         #0
            rts

@after:                                                     ; (pfx $FF: /ser's, the bytes as they are)
            bmi         @ser
            stz         pfx                                 ; The key after Ctrl-]: a digit is the window that has
            tax                                             ;   the keys now (its note group Ctrl-C's: the serve
            eor         #'0'                                ;   entry acts on the rest).  ($30-$39 alone give 0-9)
            cmp         #10
            bcs         :+
            tay
            lda         w_group,Y
            sta         win_grp
:
            txa
            bra         @store

@intr:                                                      ; The notes, to the shown window's group
            lda         #1 << (NOTE_INTERRUPT - 1)
            bra         @note

@kill:
            lda         #1 << (NOTE_KILL - 1)
@note:
            ldx         win_grp
            jsr         NOTE_QUEUE
            lda         #0
            rts

@ser:                                                       ; /ser's: into the ring, all its pages
            ldy         rx_head
            sta         (rx_hp),Y
            iny
            beq         @page
            cpy         rx_tail                             ; (Full, the tail next: the byte's dropped)
            bne         @put
            ldx         rx_hp + 1
            cpx         rx_tp + 1
            beq         @full
@put:
            sty         rx_head
            inc         TASK_EVENT
@full:
            lda         #0
            rts

@page:                                                      ; Its next page (round), at its start
            lda         rx_hp + 1
            inc         a
            cmp         #>(rx_buf + RX_PAGES * 256)
            bne         :+
            lda         #>rx_buf
:
            ldx         rx_tail                             ; (Full: the tail there)
            bne         :+
            cmp         rx_tp + 1
            beq         @full
:
            sta         rx_hp + 1
            bra         @put

; Timer 2 ran out: the next byte (a character's time since the last went), or nothing more to send (and /pc's reply
; awaited: its wait, PC_NAP rounds of about 65,000 cycles, the rounds a slow rate's are)
t2_next:
            dec         t2_left                             ; (A slow rate, or /pc's wait: another round)
            bne         @round
            ldy         tx_tail
            cpy         tx_head
            beq         @idle
            lda         tx_buf,Y
            sta         ACIA_DATA
            iny
            sty         tx_tail
            lda         t2_rounds
            sta         t2_left
@round:
            lda         t2_lo
            sta         VIA_T2CL
            lda         t2_hi
            sta         VIA_T2CH                            ; (It starts, and its interrupt's cleared)
            lda         #0
            rts

@idle:
            stz         tx_busy
            inc         TASK_EVENT                          ; (The writers waiting for room look again; /pc's
            lda         #PC_NAP                             ;   client looks at the time, and an Escape's reader)
            ldy         pc_step
            bne         @nap
            lda         t2_napr                             ; (An Escape alone, the chrome's time: nap_set's)
            beq         @stop
@nap:                                                       ; A /pc reply awaited, or an Escape alone: timer 2 runs
            sta         t2_left                             ;   on, its rounds $FFxx cycles (tx_start puts the
            lda         #$FF                                ;   rate's back)
            sta         t2_hi
            sta         VIA_T2CH
            sta         t2_nap
            lda         #0
            rts

@stop:
            stz         t2_nap
            lda         VIA_T2CL                            ; (Its interrupt cleared)
            lda         #0
            rts
.code

; ****************************************************************************
; The send and receive rings

; Timer 2 started, if nothing's going and there's something to send: it sends the first byte a character's time
; from now, and the rest after it.  (Not at once: the kernel's bring-up console may have sent a byte just now; and
; the status register isn't for reading here, which would lose a receive interrupt.)  Modifies .A, .Y
tx_start:
            php
            sei
            lda         tx_busy
            bne         @done
            ldy         tx_tail
            cpy         tx_head
            beq         @done
            inc         tx_busy
            stz         t2_nap
            ldy         rate                                ; (A character's time: /pc's wait may have had timer 2)
            lda         rate_hi,Y
            sta         t2_hi
            lda         t2_rounds
            sta         t2_left
            lda         t2_lo
            sta         VIA_T2CL
            lda         t2_hi
            sta         VIA_T2CH
@done:
            plp
            rts

; .A into the send ring (the caller has made sure of the room).  Keeps .A, .X
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

; The receive ring emptied, on its first page (the keys' ring; /ser's goes on from it to the others).  Modifies .A
rx_reset:
            php
            sei
            stz         rx_head
            stz         rx_tail
            LDR         rx_hp, rx_buf
            LDR         rx_tp, rx_buf
            plp
            rts

; A byte from the receive ring (the keys': its first page; not while the line is /ser's).  OUT: C = 0, .A = it; or
; C = 1: none.  Modifies .Y
rx_get:
            ldy         rx_tail
            cpy         rx_head
            beq         @none
            lda         rx_buf,Y
            inc         rx_tail
            clc
            rts

@none:
            sec
            rts

; ****************************************************************************
; The windows

; Before each request: the keys come in, each to the window shown (its queue), Ctrl-] and the key after it acted on;
; /pc's frames taken out (pc_rx).  None while /ser is open for reading: the bytes are its
distribute:
            lda         ser_rd
            beq         :+
            rts
:
            lda         ls_w                                ; (The list, or the view, another window shown: gone)
            bmi         :+
            cmp         w_in
            beq         :+
            jsr         ls_close
:
            lda         vv_w
            bmi         @byte
            cmp         w_in
            beq         @byte
            jsr         vv_close
@byte:
            jsr         rx_get
            bcc         :+
            jmp         paste_feed                          ; (Then a paste's next keys)
:
            ldx         pc_rxs                              ; A /pc frame's?
            bne         @frame
            cmp         #PC_MARK
            bne         @keys
@frame:
            jsr         pc_rx
            bcc         @byte
@keys:
            ldx         d_pfx
            bne         @command
            cmp         kb_pfx
            bne         @key
            inc         d_pfx
            bra         @byte

@key:
            pha                                             ; (To the window, or the list's or the view's; then the
            ldx         ls_w                                ;   console's look: the window's decoder drops the
            bmi         :+                                  ;   sequences that aren't keys)
            jsr         ls_key
            bra         @watch
:
            ldx         vv_w
            bmi         :+
            jsr         vv_key
            bra         @watch
:
            ldx         w_in
            jsr         iq_put
@watch:
            pla
            jsr         kw_watch
            jmp         @byte

@command:                                                   ; The key after the prefix (d_pfx 1), or its ESC, [ (2, 3)
            ldx         d_pfx
            cpx         #1
            bne         @seq
            stz         d_pfx
            cmp         kb_pfx                              ; (The prefix again: itself)
            beq         @key
            cmp         #ESC                                ; ESC: Shift-Tab's, ESC [ Z, an arrow's (d_pfx 2: [ next)
            bne         :+
            lda         #2
            sta         d_pfx
            jmp         @byte
:
            cmp         #'0'                                ; A digit: that window
            bcc         @act
            cmp         #'9' + 1
            bcs         @act
            and         #$0F
            tax
            lda         w_used,X
            beq         @same
            jsr         w_show
            jmp         @byte
@act:
            cmp         #$80                                ; Else its binding's action
            bcs         @same
            tax
            lda         kb_act,X
            jsr         k_do
            bra         @same

@seq:
            cpx         #2                                  ; (The prefix, ESC: [ next)
            bne         :+
            cmp         #'['
            bne         @off
            inc         d_pfx
            jmp         @byte
:
            stz         d_pfx                               ; (The prefix, ESC [: Z, Shift-Tab, ESC's binding; an
            cmp         #'Z'                                ;   arrow, the group's previous window (up, left) or
            beq         @stab                               ;   next (down, right))
            cmp         #'A'
            beq         @prev
            cmp         #'D'
            beq         @prev
            cmp         #'B'
            beq         @nextw
            cmp         #'C'
            bne         @off
@nextw:
            jsr         win_next
            bra         @same
@prev:
            jsr         win_prev
            bra         @same
@stab:
            lda         kb_act + ESC
            jsr         k_do
            bra         @same
@off:
            stz         d_pfx
            jmp         @byte

@same:                                                      ; (No window shown anew: the keys' note group the shown
            ldx         w_in                                ;   one's still)
            lda         w_group,X
            sta         win_grp
            jmp         @byte

@done:
            rts

; The keys' action .A (KA_*: a binding's)
k_do:
            cmp         #KA_N
            bcs         k_none
            asl
            tax
            jmp         (ka_vec,X)
k_none:
            rts

k_new:                                                      ; A group wanted (for /wnew's reader)
            lda         #1
            sta         want_new
            inc         TASK_EVENT
            rts

k_hold:                                                     ; The window shown held, or not
            ldx         w_in
            lda         w_hold,X
            eor         #1
            sta         w_hold,X
            lda         #3                                  ; (Held: in its chrome's %m)
            tsb         chr_dirty
            inc         TASK_EVENT                          ; (Its writers look again)
            rts

k_close:                                                    ; Its note group a hangup (the list or the view: the
            ldx         w_in                                ;   window before)
            cpx         ls_w
            bne         :+
            jmp         ls_back
:
            cpx         vv_w
            bne         :+
            jmp         vv_back
:
            lda         w_group,X
            ora         #NOTE_GROUP
            ldx         #NOTE_HANGUP
            jmp         NOTE_POST

; Ctrl-] s, Ctrl-] v: a window in the window shown's group (for /wnew's reader: wstart's shell), the group's layout rows
; or columns if it was tabs (as it is if it's tiled), not zoomed
k_vsplit:
            lda         #LAY_COLS
            bra         :+
k_split:
            lda         #LAY_ROWS
:
            ldx         w_in
            cpx         ls_w                                ; (Not the list's or the view's)
            beq         @none
            cpx         vv_w
            beq         @none
            ldy         w_grp,X
            ldx         g_lay,Y
            bne         :+
            sta         g_lay,Y
:
            lda         #0
            sta         g_zoom,Y
            lda         #2
            sta         want_new
            inc         TASK_EVENT
@none:
            rts

; Ctrl-] z: the tiled group shown, its focus alone (zoomed), or tiled again
k_zoom:
            ldx         w_in
            ldy         w_grp,X
            lda         g_lay,Y
            beq         @none
            lda         g_zoom,Y
            eor         #1
            sta         g_zoom,Y
            jmp         re_tile
@none:
            rts

; The sizes found again, and the terminals painted (a layout changed)
re_tile:
            jsr         relayout
            lda         #1
            sta         ts_ser
            sta         ts_scr
            rts

; Ctrl-] y: the snarf buffer pasted into the window shown as its keys (paste_feed's), bracketed if its program asked
; (?2004).  A paste going on is ended first; the list's window takes none
k_paste:
            lda         sn_len
            ora         sn_len + 1
            beq         @none
            lda         w_in
            cmp         ls_w
            beq         @none
            cmp         vv_w
            beq         @none
            sta         ps_w
            FAR2        vt_paste
            sta         ps_br
            stz         ps_i
            stz         ps_i + 1
            stz         ps_ph
            stz         ps_k
            jmp         paste_feed
@none:
            rts

; The paste going on: its next bytes into its window's keys, as its queue has room (a key's place kept free, as
; kbdin's); its reader looks again
paste_feed:
            ldx         ps_w
            bpl         :+
            rts
:
            lda         w_used,X                            ; (Its window gone: the paste too)
            beq         @end
@byte:
            ldx         ps_w
            sec
            lda         w_iqt,X
            sbc         w_iqh,X
            dec         a
            and         #INQ_SIZE - 1
            beq         @wait
            jsr         ps_next
            bcs         @end
            ldx         ps_w
            jsr         iq_put
            bra         @byte
@end:
            lda         #$FF
            sta         ps_w
@wait:
            inc         TASK_EVENT
            rts

; The paste's next byte: the bracket before (CSI 200 ~), the text (an LF as a CR), the bracket after (CSI 201 ~).
; OUT: C = 0, .A = it; or C = 1, none left
ps_next:
            lda         ps_ph
            bne         @text
            lda         ps_br                               ; The bracket before
            beq         @next
            ldx         ps_k
            lda         s_bp_open,X
            beq         @next
            inc         ps_k
            clc
            rts
@next:
            inc         ps_ph
            stz         ps_k
            bra         ps_next
@text:
            cmp         #1
            bne         @after
            lda         ps_i                                ; The text, from the snarf buffer's bank
            cmp         sn_len
            lda         ps_i + 1
            sbc         sn_len + 1
            bcs         @next
            lda         ps_i
            sta         p
            lda         ps_i + 1
            clc
            adc         #>BANK_WINDOW
            sta         p + 1
            lda         $00
            pha
            lda         sn_bank
            sta         $00
            lda         (p)
            tax
            pla
            sta         $00
            inc         ps_i
            bne         :+
            inc         ps_i + 1
:
            txa
            cmp         #LF
            bne         :+
            lda         #CR
:
            clc
            rts
@after:
            lda         ps_br                               ; The bracket after
            beq         @done
            ldx         ps_k
            lda         s_bp_close,X
            beq         @done
            inc         ps_k
            clc
            rts
@done:
            sec
            rts

; Ctrl-] [, Shift-PgUp: the scrollback's view of the window shown, at its end (the window's last lines, its cursor
; on the last); its key again leaves it.  A window of the console's own, its notes the window's (Ctrl-C's)
k_view:
            lda         vv_w
            bmi         :+
            jmp         vv_back
:
            lda         w_in
            cmp         ls_w
            beq         @none
            sta         vv_src
            lda         #$FF
            jsr         w_make
            bcs         @none
            stx         vv_w
            ldy         vv_src
            lda         w_group,Y
            sta         w_group,X
            txa                                             ; Its label, its footer
            jsr         lbl_ptr
            ldy         #0
:
            lda         s_vv_label,Y
            sta         (m),Y
            beq         :+
            iny
            bra         :-
:
            lda         vv_w
            ldy         #1
            jsr         fmt_at
            lda         #<s_vv_foot
            ldx         #>s_vv_foot
            jsr         fmt_copy
            lda         vv_src                              ; The lines, its rows
            FAR2        vt_lines
            sta         vv_n
            lda         vv_add
            sta         vv_seen
            lda         vv_add + 1
            sta         vv_seen + 1
            ldx         vv_w
            FAR2        vt_size
            stx         vv_r
            lda         #$FF
            sta         vv_ma
            stz         vv_esc
            jsr         vv_end
            jsr         vv_fill
            ldx         vv_w
            jmp         w_show
@none:
            rts

; The view filled again (vt.s), its footer with it; marked, the selection to the cursor's line
vv_fill:
            lda         vv_ma
            cmp         #$FF
            beq         :+
            clc
            lda         vv_top
            adc         vv_cur
            sta         vv_mb
:
            FAR2        vt_view
            lda         #3
            tsb         chr_dirty
            rts

; The view's key .A: the arrows, PgUp, PgDn, Home, End (a terminal's sequences, either form); Space, Enter, q,
; Escape twice
vv_key:
            ldx         vv_src                              ; (Its window gone: it goes too)
            ldy         w_used,X
            bne         :+
            jmp         vv_close
:
            ldx         vv_esc
            beq         @plain
            dex
            bne         @seq
            cmp         #'['                                ; ESC: [ or O starts a sequence, ESC again leaves
            beq         :+
            cmp         #'O'
            beq         :+
            stz         vv_esc
            cmp         #ESC
            bne         @plain
            jmp         vv_back
:
            lda         #2
            sta         vv_esc
            stz         vv_num
            rts
@seq:
            cmp         #'0'                                ; (Its first number; past a ;, the rest dropped)
            bcc         :+
            cmp         #'9' + 1
            bcs         :+
            dex
            beq         @first
            rts
@first:
            and         #$0F
            ldy         vv_num
            sty         p
            jsr         dec_add
            sta         vv_num
            rts
:
            cmp         #';'
            bne         :+
            lda         #3
            sta         vv_esc
            rts
:
            cmp         #$40
            bcc         @done
            stz         vv_esc
            cmp         #'~'
            beq         @tilde
            ldx         #3
:
            cmp         vk_let,X
            beq         :+
            dex
            bpl         :-
            rts
:
            lda         vk_letm,X
            tax
            jmp         vv_move
@tilde:
            lda         vv_num
            ldx         #5
:
            cmp         vk_num,X
            beq         :+
            dex
            bpl         :-
            rts
:
            lda         vk_numm,X
            tax
            jmp         vv_move
@plain:
            cmp         #ESC
            bne         :+
            lda         #1
            sta         vv_esc
            rts
:
            cmp         #' '
            beq         @mark
            cmp         #CR
            beq         @copy
            cmp         #LF
            beq         @copy
            cmp         #'q'
            bne         @done
            jmp         vv_back
@mark:                                                      ; (Space: the cursor's line marked, or none)
            jsr         vv_lines
            lda         vv_ma
            cmp         #$FF
            bne         :+
            clc
            lda         vv_top
            adc         vv_cur
            sta         vv_ma
            jmp         vv_fill
:
            lda         #$FF
            sta         vv_ma
            jmp         vv_fill
@copy:
            jmp         vv_copy
@done:
            rts

; The view moved by .X (0 up, 1 down, 2 a page up, 3 a page down, 4 to the start, 5 to the end), its lines counted
; again first
vv_move:
            phx
            jsr         vv_lines
            pla
            asl
            tax
            jsr         @go
            jmp         vv_fill
@go:
            jmp         (vv_vec,X)

vv_up:                                                      ; (Its cursor up a row, else its top up a line)
            lda         vv_cur
            beq         :+
            dec         vv_cur
            rts
:
            lda         vv_top
            beq         :+
            dec         vv_top
:
            rts

vv_down:                                                    ; (Its cursor down a row, else its top down a line)
            lda         vv_cur
            inc         a
            cmp         vv_r
            bcs         @top
            clc
            adc         vv_top
            cmp         vv_n
            bcs         @done
            inc         vv_cur
            rts
@top:
            clc
            lda         vv_top
            adc         vv_r
            cmp         vv_n
            bcs         @done
            inc         vv_top
@done:
            rts

vv_pgup:                                                    ; (Its top a page up; at the start, its cursor too)
            lda         vv_top
            bne         :+
            stz         vv_cur
            rts
:
            sec
            sbc         vv_r
            bcs         :+
            lda         #0
:
            sta         vv_top
            rts

vv_pgdn:                                                    ; (Its top a page down; at the end, its cursor too)
            jsr         vv_last
            cmp         vv_top
            beq         vv_end
            bcc         vv_end
            clc
            lda         vv_top
            adc         vv_r
            cmp         vv_i
            bcc         :+
            lda         vv_i
:
            sta         vv_top
            rts

vv_home:
            stz         vv_top
            stz         vv_cur
            rts

vv_end:                                                     ; (The last lines, its cursor on the last)
            jsr         vv_last
            sta         vv_top
            sec
            lda         vv_n
            sbc         vv_top
            cmp         vv_r
            bcc         :+
            lda         vv_r
:
            dec         a
            sta         vv_cur
            rts

vv_last:                                                    ; (.A, vv_i: its top at the end, the lines less its
            sec                                             ;   rows, 0 at least)
            lda         vv_n
            sbc         vv_r
            bcs         :+
            lda         #0
:
            sta         vv_i
            rts

; The view's lines counted again: those dropped off the oldest end since it last looked move its top and mark up
; with their lines; its top no further than the end
vv_lines:
            lda         vv_src
            FAR2        vt_lines
            sta         vv_n
            sec                                             ; (The lines dropped: 255 at most)
            lda         vv_add
            sbc         vv_seen
            tax
            lda         vv_add + 1
            sbc         vv_seen + 1
            beq         :+
            ldx         #$FF
:
            stx         vv_i
            lda         vv_add
            sta         vv_seen
            lda         vv_add + 1
            sta         vv_seen + 1
            sec
            lda         vv_top
            sbc         vv_i
            bcs         :+
            lda         #0
:
            sta         vv_top
            lda         vv_ma
            cmp         #$FF
            beq         :++
            sec
            sbc         vv_i
            bcs         :+
            lda         #0
:
            sta         vv_ma
:
            jsr         vv_last
            cmp         vv_top
            bcs         :+
            sta         vv_top
:
            rts

; Enter: the selection (none: the cursor's line) into the snarf buffer, in place of what it had: each line's text and
; an LF.  Then the view's left
vv_copy:
            jsr         vv_lines
            jsr         sn_bankget
            bcs         @out
            stz         sn_len
            stz         sn_len + 1
            clc
            lda         vv_top
            adc         vv_cur
            sta         vv_mb
            lda         vv_ma                               ; (vv_ma its first line, vv_mb its last)
            cmp         #$FF
            bne         :+
            lda         vv_mb
:
            cmp         vv_mb
            bcc         :+
            ldx         vv_mb
            sta         vv_mb
            txa
:
            sta         vv_ma
@line:
            ldx         vv_ma
            cpx         vv_n
            bcs         @out
            lda         vv_src
            FAR2        vt_line                             ; (.A: its length; its text in vbuf)
            sta         vv_i
            ldx         #0
:
            cpx         vv_i
            bcs         :+
            lda         vbuf,X
            phx
            jsr         sn_put
            plx
            inx
            bra         :-
:
            lda         #LF
            jsr         sn_put
            lda         vv_ma
            cmp         vv_mb
            bcs         @out
            inc         vv_ma
            bra         @line
@out:
            jmp         vv_back

; The window the view showed shown again (if it's still there), the view gone
vv_back:
            ldx         vv_src
            lda         w_used,X
            beq         vv_close
            jsr         w_show
vv_close:
            ldx         vv_w
            lda         #$FF
            sta         vv_w
            jmp         w_free

; /snarf's bank, taken the first time.  OUT: C = 0; or C = 1, .A = the error
sn_bankget:
            lda         sn_bank
            cmp         #$FF
            clc
            bne         :+
            lda         #1
            jsr         BANKS_ALLOC
            bcs         :+
            sta         sn_bank
:
            rts

; .A at the snarf buffer's end (SNARF_MAX bytes at most: past them, dropped).  Modifies .A, .Y, p
sn_put:
            ldy         sn_len + 1
            cpy         #>SNARF_MAX
            bcs         @done
            pha
            lda         sn_len
            sta         p
            tya
            adc         #>BANK_WINDOW
            sta         p + 1
            ldy         $00
            lda         sn_bank
            sta         $00
            pla
            sta         (p)
            sty         $00
            inc         sn_len
            bne         @done
            inc         sn_len + 1
@done:
            rts

; The windows' list (Ctrl-] w): a window of the console's own, a line a window (the one chosen marked >, then its key:
; its number in hex; its number, activity and label, as the bar's; its group), written while it isn't shown, then
; shown.  Its notes are the window's before it (Ctrl-C's).  Shown, the list's key again leaves it
ls_open:
            lda         ls_w
            bmi         :+
            jmp         ls_back
:
            lda         #$FF                                ; (None free: nothing)
            jsr         w_make
            bcc         :+
            rts
:
            stx         ls_w
            lda         w_in
            sta         ls_from
            tay
            lda         w_group,Y
            sta         w_group,X
            lda         #1                                  ; (Its writes all taken: the serial port painted after)
            sta         w_jump,X
            txa                                             ; Its label
            jsr         lbl_ptr
            ldy         #0
:
            lda         s_ls_label,Y
            sta         (m),Y
            beq         :+
            iny
            bra         :-
:
            stz         ls_n                                ; The windows, the one shown chosen
            stz         ls_sel
            stz         ls_esc
            ldx         #0
@win:
            lda         w_used,X
            beq         @next
            cpx         ls_w
            beq         @next
            ldy         ls_n
            txa
            sta         ls_ws,Y
            cpx         ls_from
            bne         :+
            sty         ls_sel
:
            inc         ls_n
@next:
            inx
            cpx         #WIN_MAX
            bcc         @win
            jsr         ls_reset                            ; Its text: the cursor off, the heading, a line each
            lda         #<s_ls_head
            ldx         #>s_ls_head
            jsr         cr_strax
            jsr         ls_flush
            ldy         #0
:
            cpy         ls_n
            bcs         :+
            phy
            jsr         ls_line
            ply
            iny
            bra         :-
:
            ldx         ls_w
            jmp         w_show

; The list's line for its window .Y (in ls_ws), written
ls_line:
            phy
            jsr         ls_reset
            ply
            lda         #' '
            cpy         ls_sel
            bne         :+
            lda         #'>'
:
            jsr         cr_put
            lda         #' '
            jsr         cr_put
            ldx         ls_ws,Y
            stx         ls_i
            lda         s_hex,X
            jsr         cr_put
            lda         #' '
            jsr         cr_put
            lda         #' '
            jsr         cr_put
            jsr         cr_entry
            lda         #<s_ls_grp
            ldx         #>s_ls_grp
            jsr         cr_strax
            ldx         ls_i
            lda         w_grp,X
            jsr         cr_dec
            lda         #<s_ls_end
            ldx         #>s_ls_end
            jsr         cr_strax
            jmp         ls_flush

; A row to build (cr_put's, as the chrome's)
ls_reset:
            stz         cr_n
            lda         #CR_MAX - 1
            sta         cr_w
            lda         #COL_DEF
            sta         cr_ca
            stz         cr_cf
            rts

; The row built (cr_n bytes in cr_c) written to the list's window, IOBUF bytes at a time
ls_flush:
            lda         ls_w
            jsr         load
            stz         ls_i
@part:
            ldx         #0
            ldy         ls_i
:
            cpy         cr_n
            bcs         :+
            cpx         #IOBUF
            bcs         :+
            lda         cr_c,Y
            sta         iobuf,X
            inx
            iny
            bra         :-
:
            stx         cnt
            sty         ls_i
            txa
            beq         @done
            FAR2        vt_write
            bra         @part
@done:
            rts

; The list's key .A: a window's key (its number in hex: 0-9, a-f), the arrows (up, down) and Enter; q, or Escape
; twice, the one before
ls_key:
            ldx         ls_esc
            beq         @plain
            dex
            bne         @seq
            cmp         #'['                                ; ESC: [ or O starts a sequence, ESC again leaves
            beq         :+
            cmp         #'O'
            beq         :+
            stz         ls_esc
            cmp         #ESC
            bne         @plain
            jmp         ls_back
:
            lda         #2
            sta         ls_esc
            rts
@seq:
            cmp         #$40                                ; (Its numbers, till its letter)
            bcc         @done
            stz         ls_esc
            ldx         #1
            cmp         #'B'
            beq         @move
            ldx         #$FF
            cmp         #'A'
            bne         @done
@move:
            txa
            jmp         ls_move
@plain:
            cmp         #ESC
            bne         :+
            lda         #1
            sta         ls_esc
            rts
:
            cmp         #CR
            beq         @enter
            cmp         #LF
            beq         @enter
            cmp         #'q'
            bne         :+
            jmp         ls_back
:
            cmp         #'0'                                ; (A letter either case, a digit as it is; no control)
            bcc         @done
            ora         #$20
            ldx         #15
:
            cmp         s_hex,X
            beq         :+
            dex
            bpl         :-
            rts
:
            txa                                             ; (Listed: shown)
            ldy         ls_n
:
            dey
            bmi         @done
            cmp         ls_ws,Y
            bne         :-
            tax
            jmp         ls_go
@enter:
            ldy         ls_sel
            cpy         ls_n
            bcs         @done
            ldx         ls_ws,Y
            jmp         ls_go
@done:
            rts

; The choice moved by .A (1: down, $FF: up), its marker with it
ls_move:
            clc
            adc         ls_sel
            cmp         ls_n
            bcs         @done
            pha
            jsr         ls_reset
            lda         ls_sel
            jsr         ls_mark
            lda         #' '
            jsr         cr_put
            pla
            sta         ls_sel
            jsr         ls_mark
            lda         #'>'
            jsr         cr_put
            jmp         ls_flush
@done:
            rts

; The cursor to the list's line .A, its start (ESC [ row ; 1 H), into the row being built
ls_mark:
            pha
            lda         #ESC
            jsr         cr_put
            lda         #'['
            jsr         cr_put
            pla
            clc
            adc         #LS_ROW
            jsr         cr_dec
            lda         #<s_ls_col
            ldx         #>s_ls_col
            jmp         cr_strax

; Window .X shown (the list's choice: its group's focus moved, its raw readers told), the list gone
ls_go:
            lda         w_used,X                            ; (Gone meanwhile: nothing)
            beq         @done
            ldy         w_grp,X
            txa
            cmp         g_focus,Y
            beq         :+
            jsr         focus_tell
:
            jsr         w_show
            jmp         ls_close
@done:
            rts

; The window shown before the list shown again (if it's still there), the list gone
ls_back:
            ldx         ls_from
            lda         w_used,X
            beq         ls_close
            jsr         w_show
ls_close:
            ldx         ls_w
            lda         #$FF
            sta         ls_w
            jmp         w_free

; The shown window's group's next window (Ctrl-Tab, Ctrl-] Tab), or its previous (win_prev): shown
win_next:
            ldx         w_in
            lda         w_grp,X
            sta         n
:
            inx
            cpx         #WIN_MAX
            bcc         :+
            ldx         #0
:
            lda         w_used,X
            beq         :--
            lda         w_grp,X
            cmp         n
            bne         :--
            bra         win_go

win_prev:
            ldx         w_in
            lda         w_grp,X
            sta         n
:
            dex
            bpl         :+
            ldx         #WIN_MAX - 1
:
            lda         w_used,X
            beq         :--
            lda         w_grp,X
            cmp         n
            bne         :--
win_go:
            cpx         w_in                                ; (Itself: the group's only one)
            beq         :+
            jmp         w_show
:
            rts

; The next group (Ctrl-] n), or the previous (grp_prev): its focused window shown
grp_next:
            ldx         w_in
            ldy         w_grp,X
:
            iny
            cpy         #WIN_MAX
            bcc         :+
            ldy         #0
:
            lda         g_used,Y
            beq         :--
            ldx         g_focus,Y
            bra         win_go

grp_prev:
            ldx         w_in
            ldy         w_grp,X
:
            dey
            bpl         :+
            ldy         #WIN_MAX - 1
:
            lda         g_used,Y
            beq         :--
            ldx         g_focus,Y
            bra         win_go

; Window .X focused in its group: each of the group's windows' raw reader told (KEY_FOCUS, then .X).  Keeps .X
focus_tell:
            ldy         #WIN_MAX - 1
@w:
            lda         w_used,Y
            beq         @next
            lda         w_grp,Y
            cmp         w_grp,X
            bne         @next
            txa
            inc         a
            sta         w_kf,Y
@next:
            dey
            bpl         @w
            inc         TASK_EVENT
            rts

; Window .X shown, with the keys: painted on both terminals (pump)
w_show:
            ldy         w_in                                ; (Within the group shown: its raw readers told)
            lda         w_grp,Y
            cmp         w_grp,X
            bne         :+
            cpx         w_in
            beq         :+
            jsr         focus_tell
:
            stx         w_in
            ldy         w_grp,X                             ; (Its group's focus)
            txa
            sta         g_focus,Y
            stz         w_act,X                             ; (Its activity seen)
            lda         w_group,X
            sta         win_grp
            lda         #1
            sta         ts_ser
            sta         ts_scr
            inc         TASK_EVENT                          ; (Its readers and writers, and the last one's, look
            rts                                             ;   again)

; A window made: the lowest free, in group .A ($FF: a group of its own, the lowest free).  OUT: C = 0, .X = it; or
; C = 1, .A = E_NOMEM
w_make:
            sta         mk_grp
            ldx         #0
:
            lda         w_used,X
            beq         @free
            inx
            cpx         #WIN_MAX
            bcc         :-
            lda         #E_NOMEM
            sec
            rts
@free:
            jsr         w_init
            bcs         @done
            lda         mk_grp
            bpl         @in
            ldy         #0                                  ; (A group of its own: groups are fewer than windows)
:
            lda         g_used,Y
            beq         :+
            iny
            bra         :-
:
            tya
            sta         w_grp,X
            lda         #1
            sta         g_used,Y
            lda         #LAY_TABS
            sta         g_lay,Y
            sta         g_zoom,Y
            txa
            sta         g_focus,Y
            clc
            rts
@in:
            sta         w_grp,X
            phx                                             ; (Its group's tiles: the sizes found again)
            jsr         relayout
            plx
            clc
@done:
            rts

; Window .X, new: its screen (vt.s: three banks), empty; init's group's.  OUT: C = 0; or C = 1, .A = E_NOMEM.
; Keeps .X
w_init:
            lda         def_chr                             ; (Its chrome the defaults; its size from them)
            sta         w_chr,X
            jsr         win_size
            phx
            FAR2        vt_new
            plx
            bcc         :+
            rts
:
            stz         w_raw,X                             ; Its chrome's state: its formats the defaults
            stz         w_kf,X
            stz         w_rdr,X
            stz         w_act,X
            stz         w_mon,X
            phx
            txa
            ldy         #0
            jsr         fmt_at
            lda         #<def_hfmt
            ldx         #>def_hfmt
            jsr         fmt_copy
            plx
            phx
            txa
            ldy         #1
            jsr         fmt_at
            lda         #<def_ffmt
            ldx         #>def_ffmt
            jsr         fmt_copy
            plx
            lda         #3
            tsb         chr_dirty
            lda         #1
            sta         w_used,X
            stz         w_iqh,X
            stz         w_iqt,X
            stz         w_cons,X
            stz         w_ctl,X
            stz         kvt,X
            stz         w_kmod,X
            stz         kp_n,X
            stz         kp_i,X
            stz         w_jump,X
            stz         w_hold,X
            stz         w_rsz,X
            lda         #INIT_TASK
            sta         w_group,X
            cpx         lw                                  ; (Its old state, if it was loaded: gone)
            bne         :+
            lda         #$FF
            sta         lw
:
            jsr         st_addr                             ; Its editor state: all 0
            ldy         #ST_SIZE - 1
            lda         #0
:
            sta         (p),Y
            dey
            bpl         :-
            clc
            rts

; Window .X gone (its last cons closed), its screen too; if it was shown, window 0 is
w_free:
            stz         w_used,X
            lda         #3                                  ; (Its chrome's lists without it)
            tsb         chr_dirty
            phx
            FAR2        vt_free
            plx
            cpx         lw
            bne         :+
            lda         #$FF
            sta         lw
:
            stx         fr_w                                ; Its group: gone with its last window, else its focus
            ldy         w_grp,X                             ;   another of them if it was this one
            sty         n
            ldx         #WIN_MAX - 1
@any:
            lda         w_used,X
            beq         @nx
            lda         w_grp,X
            cmp         n
            beq         @left
@nx:
            dex
            bpl         @any
            ldy         n
            lda         #0
            sta         g_used,Y
            bra         @shown
@left:
            ldy         n
            lda         g_focus,Y
            cmp         fr_w
            bne         @shown
            txa
            sta         g_focus,Y
@shown:
            lda         n                                   ; (Its group's tiles: the sizes found again)
            pha
            jsr         relayout
            pla
            sta         n
            lda         fr_w                                ; Shown: its group's focus now, or (gone) the next
            cmp         w_in                                ;   group's
            bne         @done
            ldy         n
            lda         g_used,Y
            beq         :+
            ldx         g_focus,Y
            jmp         w_show
:
            jmp         grp_next
@done:
            rts

; Window .A's editor state loaded (st_*, ln_buf), the one that was, back to its own place first
load:
            cmp         lw
            beq         @done
            pha
            ldx         lw
            bmi         @in
            jsr         st_addr
            ldy         #ST_N - 1
:
            lda         st_first,Y
            sta         (p),Y
            dey
            bpl         :-
            jsr         ln_addr
            ldy         #LINE_MAX
:
            lda         ln_buf,Y
            sta         (p),Y
            dey
            bpl         :-
@in:
            pla
            sta         lw
            tax
            jsr         st_addr
            ldy         #ST_N - 1
:
            lda         (p),Y
            sta         st_first,Y
            dey
            bpl         :-
            jsr         ln_addr
            ldy         #LINE_MAX
:
            lda         (p),Y
            sta         ln_buf,Y
            dey
            bpl         :-
@done:
            rts

; p = window .X's editor state (st_addr), or its line (ln_addr).  Keeps .X
st_addr:
            stz         p + 1
            txa
            asl
            asl
            asl
            asl
            asl
            rol         p + 1
            clc
            adc         #<w_state
            sta         p
            lda         p + 1
            adc         #>w_state
            sta         p + 1
            rts

ln_addr:
            txa
            lsr
            sta         p + 1
            lda         #0
            ror
            clc
            adc         #<lines
            sta         p
            lda         p + 1
            adc         #>lines
            sta         p + 1
            rts

.assert     ST_SIZE = 32 .and WIN_MAX <= 16 .and LINE_MAX + 1 = 128, error, "st_addr and ln_addr: 32 and 128 bytes a window"

; Key .A into window .X's queue (dropped if it's full).  Keeps .X
iq_put:
            pha
            lda         w_iqh,X
            inc         a
            and         #INQ_SIZE - 1
            cmp         w_iqt,X
            beq         @full
            sta         n
            jsr         iq_at
            ldy         w_iqh,X
            pla
            sta         (qp),Y
            lda         n
            sta         w_iqh,X
            rts

@full:
            pla
            rts

; A key from the loaded window's queue.  OUT: C = 0, .A = it; or C = 1: none.  Modifies .X, .Y
iq_get:
            ldx         lw
            lda         w_iqt,X
            cmp         w_iqh,X
            beq         @none
            jsr         iq_at
            ldy         w_iqt,X
            lda         w_iqt,X
            inc         a
            and         #INQ_SIZE - 1
            sta         w_iqt,X
            lda         kbd_wait                            ; (A /kbdin writer waiting for room: it looks again)
            beq         :+
            stz         kbd_wait
            inc         TASK_EVENT
:
            lda         (qp),Y
            clc
            rts

@none:
            sec
            rts

; qp = window .X's key queue (inq + INQ_SIZE * it).  Keeps .X
iq_at:
            txa
            lsr
            lsr
            sta         qp + 1
            txa
            and         #3
            asl
            asl
            asl
            asl
            asl
            asl
            clc
            adc         #<inq
            sta         qp
            lda         qp + 1
            adc         #>inq
            sta         qp + 1
            rts

; .A, a byte of the loaded window's output (its line editor's echo: edit has made sure of the room, w_room), to its
; screen (vt.s).  Keeps .A, .X, .Y
w_put:
            phx
            phy
            pha
            FAR2        vt_put
            pla
            ply
            plx
            rts

; m = the room for the loaded window's echo: if it's shown on the serial port, which follows it, the send ring's room
; less a byte's (VT_ROOM); none while the serial port's being painted; else no limit ($FFFF).  Modifies .A
w_room:
            lda         #$FF
            sta         m
            sta         m + 1
            lda         lw
            cmp         w_in
            bne         @done
            lda         term
            and         #TERM_SERIAL
            beq         @done
            lda         ser_rd
            bne         @done
            stz         m
            stz         m + 1
            lda         ts_ser
            bne         @done
            jsr         tx_free
            sec
            sbc         #VT_ROOM
            bcc         @done
            sta         m
@done:
            rts

; After each request: /pc's frame out first, all of it (and a request long past its time given up); then the
; terminals painted (vt.s: the serial port's as the send ring has room, not while a frame's going out; the screen's,
; and its bytes to #v/term).  None while the line is /ser's
pump:
            lda         ser_rd
            beq         :+
            jmp         tx_start
:
            lda         pc_step
            beq         :+
            lda         #PC_STALE
            jsr         pc_late
            bcc         :+
            jsr         pc_release
:
            jsr         pc_pump
            lda         #0
            rol                                             ; (.A <> 0: a frame's going out)
            pha
            jsr         scr_ready
            lda         clk_on                              ; (The chrome's time: drawn again each minute)
            beq         :+
            jsr         clk_check
:
            lda         scr_chk                             ; (The screen's size looked at, if it may have
            beq         :+                                  ;   changed)
            jsr         scr_size
:
            pla
            FAR2        vt_pump
            jmp         tx_start

; The screen's file, #v/term, opened if the screen's on and it isn't yet (scr_open).  Modifies .A, .X, .Y, r0
scr_ready:
            lda         term
            and         #TERM_SCREEN
            beq         :+
            lda         scr_st
            bne         :+
            jsr         scr_open
:
            rts

; The screen's file, #v/term, open (the first time: opened; no screen, scr_st 2 from then on).  OUT: C = 0 open;
; C = 1 none.  Modifies .A, .X, .Y, r0
scr_open:
            lda         scr_st
            cmp         #1
            beq         @open
            bcs         @none
            LDR         r0, s_scr
            lda         #O_WRITE
            jsr         OPEN
            ldx         #2
            bcs         @st
            sta         scr_fd
            LDR         r0, s_scr_ctl                       ; (Its ctl, for its size: looked at in the pump)
            lda         #O_READ
            jsr         OPEN
            bcc         :+
            lda         #$FF
:
            sta         scr_cfd
            lda         #1
            sta         scr_chk
            ldx         #1
@st:
            stx         scr_st
            cpx         #1
            bne         @none
@open:
            clc
            rts

@none:
            sec
            rts

; The screen's size, from its ctl's mode line (mode 80x60): a change lays the windows out again.  Modifies .A, .X,
; .Y, r0, r1, n, m, p
scr_size:
            stz         scr_chk
            lda         scr_cfd
            bpl         :+
            rts
:
            stz         r0                                  ; (Its text from the start)
            stz         r0 + 1
            stz         r1
            stz         r1 + 1
            ldx         #0
            jsr         SEEK
            bcs         @out
            LDR         r0, iobuf
            lda         #IOBUF
            sta         r1
            stz         r1 + 1
            lda         scr_cfd
            jsr         READ
            bcc         :+
@out:
            rts
:
            sta         m                                   ; (Its length)
            ldy         #0
@line:
            ldx         #0                                  ; A line: mode?
:
            lda         s_mode_w,X
            beq         @mode
            cpy         m
            bcs         @done
            cmp         iobuf,Y
            bne         @skip
            iny
            inx
            bra         :-
@skip:
            cpy         m                                   ; Else on to the next
            bcs         @done
            lda         iobuf,Y
            iny
            cmp         #LF
            bne         @skip
            bra         @line
@mode:
            jsr         @num                                ; Its columns, x, its rows
            sta         n
            cpy         m
            bcs         @done
            lda         iobuf,Y
            cmp         #'x'
            bne         @done
            iny
            jsr         @num
            sta         n + 1
            beq         @done
            lda         n
            beq         @done
            cmp         scr_cols
            bne         :+
            lda         n + 1
            cmp         scr_rows
            beq         @done
:
            lda         n
            sta         scr_cols
            lda         n + 1
            sta         scr_rows
            jmp         relayout
@done:
            rts

@num:                                                       ; .A = the number at iobuf,Y (255 at most), past it
            stz         p
:
            cpy         m
            bcs         :+
            lda         iobuf,Y
            sec
            sbc         #'0'
            cmp         #10
            bcs         :+
            jsr         dec_add
            iny
            bra         :-
:
            lda         p
            rts

; p = p * 10 + .A (a digit's value), 255 at most.  Keeps .X, .Y
dec_add:
            sta         p + 1
            lda         p
            cmp         #26
            bcs         @big
            asl                                             ; (C = 0 throughout: p * 10 is 250 at most)
            asl
            adc         p
            asl
            adc         p + 1
            bcc         :+
@big:
            lda         #255
:
            sta         p
            rts

; Each window's size (win_size: the smaller of the terminals it's shown on, each less its chrome there): a change
; resizes it (vt.s), tells its raw reader (KEY_RESIZE), and has the terminals painted again.  Modifies .A, .X, .Y, n
relayout:
            stz         rl_chg
            ldx         #WIN_MAX - 1
@win:
            lda         w_used,X
            beq         @next
            jsr         win_size
            phx
            FAR2        vt_size                             ; (.A: its columns, .X: its rows, as they are)
            cmp         lay_cols
            bne         @resize
            cpx         lay_rows
            beq         @same
@resize:
            plx
            phx
            FAR2        vt_resize
            plx
            lda         #1
            sta         w_rsz,X
            sta         rl_chg
            bra         @next
@same:
            plx
@next:
            dex
            bpl         @win
            lda         rl_chg
            beq         @done
            lda         #1
            sta         ts_ser
            sta         ts_scr
            inc         TASK_EVENT                          ; (Their raw readers look again)
@done:
            rts

; lay_cols, lay_rows: window .X's size: the smaller of the terminals it's shown on (the screen's once its size is
; known), each less its chrome rows there; WIN_COLS x WIN_ROWS at most.  Keeps .X.  Modifies .A, .Y, n
win_size:
            lda         #WIN_COLS
            sta         lay_cols
            lda         #WIN_ROWS
            sta         lay_rows
            lda         term
            and         #TERM_SERIAL
            beq         @screen
            ldy         #0
            jsr         chr_rows
            sta         n
            stz         tl_t
            lda         ser_rows
            ldy         ser_cols
            jsr         tile_dims
            jsr         @min
@screen:
            lda         term
            and         #TERM_SCREEN
            beq         @done
            lda         scr_st
            cmp         #1
            bne         @done
            lda         scr_cols
            beq         @done
            ldy         #1
            jsr         chr_rows
            sta         n
            lda         #1
            sta         tl_t
            lda         scr_rows
            ldy         scr_cols
            jsr         tile_dims
            jsr         @min
@done:
            rts

@min:                                                       ; No more than .Y columns, and .A rows less n
            cpy         lay_cols                            ;   (WIN_MIN_ROWS at least)
            bcs         :+
            sty         lay_cols
:
            sec
            sbc         n
            bcc         @few
            cmp         #WIN_MIN_ROWS
            bcs         @rows
@few:
            lda         #WIN_MIN_ROWS
@rows:
            cmp         lay_rows
            bcs         :+
            sta         lay_rows
:
            rts

; (win_size's) .A, .Y: window .X's tile's rows and columns on terminal tl_t, its group tiled (n: its header's row);
; else as they came.  Keeps .X
tile_dims:
            FAR2        vt_tile
            bcs         :+
            pha
            lda         #1
            sta         n
            pla
:
            rts

; .A = window .X's chrome rows on terminal .Y (0 the serial port, 1 the screen): its header, its footer, and the bar
; (if there's one).  Keeps .X.  Modifies n + 1
chr_rows:
            lda         w_chr,X
            and         #CH_LIVE
            cpy         #0
            beq         :+
            lsr
            lsr
            lsr
            lsr
:
            sta         n + 1
            lda         #0
            lsr         n + 1                               ; (CH_BAR)
            bcc         :+
            ldy         bar_pos
            beq         :+
            inc         a
:
            lsr         n + 1                               ; (CH_HEAD)
            adc         #0
            lsr         n + 1                               ; (CH_FOOT)
            adc         #0
            rts

; The serial port's terminal's size: .X columns, .A rows (too small: not taken).  OUT: C = 1 not taken.  Modifies .A,
; .X, .Y, n
ser_size:
            cpx         #WIN_MIN_COLS
            bcc         @no
            cmp         #WIN_MIN_ROWS
            bcc         @no
            sta         ser_rows
            stx         ser_cols
            jsr         relayout
            clc
            rts
@no:
            sec
            rts

; The serial port's terminal's sequences the console takes, watched for as the keys go to the window (whose decoder
; drops them: they aren't keys): ESC [ 8 ; R ; C t, its size (xterm's answer to ESC [ 18 t, and the PC tool's,
; unasked); Ctrl-Tab and Ctrl-Shift-Tab, the group's next and previous window: xterm's ESC [ 27 ; 5 ; 9 ~ (6:
; shifted) and CSI u's ESC [ 9 ; 5 u.  IN: .A, the key.  Modifies .A, .X, .Y, n, p
kw_watch:
            ldx         kw_st
            cmp         #ESC                                ; (An ESC starts one, always)
            bne         :+
            lda         #1
            sta         kw_st
            rts
:
            cpx         #0
            bne         :+
            rts
:
            cpx         #1
            bne         @csi
            cmp         #'['
            beq         :+
            stz         kw_st
            rts
:
            inc         kw_st
            stz         kw_n
            stz         kw_p
            stz         kw_p + 1
            stz         kw_p + 2
            rts
@csi:
            cmp         #';'                                ; Its numbers, three at most
            bne         :+
            ldx         kw_n
            cpx         #2
            bcc         @more
            stz         kw_st
            rts
@more:
            inc         kw_n
            rts
:
            cmp         #'0'
            bcc         @final
            cmp         #'9' + 1
            bcs         @final
            and         #$0F
            ldx         kw_n
            ldy         kw_p,X
            sty         p
            jsr         dec_add
            sta         kw_p,X
            rts
@final:
            stz         kw_st
            cmp         #'t'
            beq         @size
            cmp         #'u'
            beq         @csiu
            cmp         #'~'
            bne         @done
            lda         kw_p                                ; ESC [ 27 ; 5 (6) ; 9 ~
            cmp         #27
            bne         @spgup
            lda         kw_p + 2
            cmp         #9
            bne         @done
            lda         kw_p + 1
            bra         @tab
@csiu:
            lda         kw_p                                ; ESC [ 9 ; 5 (6) u
            cmp         #9
            bne         @done
            lda         kw_p + 1
@tab:
            cmp         #5                                  ; (Their bindings' actions)
            bne         :+
            lda         kb_ct
            jmp         k_do
:
            cmp         #6
            bne         @done
            lda         kb_ct + 1
            jmp         k_do
@spgup:                                                     ; ESC [ 5 ; 2 ~: Shift-PgUp, by its binding (the view's
            cmp         #5                                  ;   own while it's shown: its PgUp); the view, entered,
            bne         @done                               ;   a page up
            lda         kw_p + 1
            cmp         #2
            bne         @done
            lda         vv_w
            bpl         @done
            lda         kb_ct + 2
            cmp         #KA_VIEW
            beq         :+
            jmp         k_do
:
            jsr         k_view
            lda         vv_w
            bmi         @done
            ldx         #2
            jmp         vv_move
@size:
            lda         kw_p                                ; ESC [ 8 ; R ; C t
            cmp         #8
            bne         @done
            ldx         kw_p + 2
            lda         kw_p + 1
            jmp         ser_size
@reset:
            stz         kw_st
@done:
            rts

; A fid made (srvlib): its window, from the spec (none: window 0); a window that isn't there: E_NOENT.  (R_DUP's
; keeps its old fid's.)  A consctl's counted.  IN: .X = the fid.  Keeps .X
opened:
            lda         #$FF                                ; (Its wctl new's answer: none)
            sta         fid_new,X
            lda         z:srv_rq
            cmp         #R_OPEN
            bne         @ok
            lda         TASK_INBOX + RQ_SPEC                ; Its number (decimal, 0-15), or nothing
            beq         @zero
            jsr         spec_num
            bcs         @noent
            tay
            lda         w_used,Y
            beq         @noent
            tya
@zero:
            sta         srv_fid_aux,X
@ok:
            lda         z:srv_e
            cmp         #ENT_CONSCTL
            bne         :+
            ldy         srv_fid_aux,X
            lda         w_ctl,Y
            inc         a
            sta         w_ctl,Y
:
            clc
            rts

@noent:
            lda         #E_NOENT
            sec
            rts

; .A = the window the spec names (decimal: one digit or two, below WIN_MAX).  OUT: C = 1, none.  Modifies .Y
spec_num:
            lda         TASK_INBOX + RQ_SPEC
            sec
            sbc         #'0'
            cmp         #10
            bcs         @no
            ldy         TASK_INBOX + RQ_SPEC + 1
            beq         @one
            sta         n                                   ; (A second digit: the tens' first)
            tya
            sec
            sbc         #'0'
            cmp         #10
            bcs         @no
            ldy         TASK_INBOX + RQ_SPEC + 2
            bne         @no
            ldy         n
            beq         @no                                 ; ("05": no)
            sta         n + 1
            lda         n
            asl
            asl
            adc         n
            asl
            adc         n + 1
@one:
            cmp         #WIN_MAX
            bcs         @no
            clc
            rts
@no:
            sec
            rts

; A fid forgotten (srvlib): a consctl's count down; with its window's last, raw ends (Plan 9's: raw lasts while
; consctl is open, so a program that ends raw, or is ended, leaves its window cooked).  IN: .X = the fid
clunked:
            lda         #$FF
            sta         fid_new,X
            lda         z:srv_e
            cmp         #ENT_CONSCTL
            bne         @done
            ldy         z:srv_id
            lda         w_ctl,Y
            beq         @done
            dec         a
            sta         w_ctl,Y
            bne         @done
            lda         #0                                  ; (keys hydra again too)
            sta         kvt,Y
            sta         w_kmod,Y
            tya
            jsr         load
            stz         raw
            ldx         z:srv_id
            lda         #0
            jsr         raw_mark
@done:
            clc
            rts

; ****************************************************************************
; The files

; /cons: a read, a write; its fids counted (its window goes with its last)
h_cons:
            cmp         #R_READ
            bne         :+
            jmp         r_cons
:
            cmp         #R_WRITE
            bne         :+
            jmp         w_write
:
            ldy         srv_fid_aux,X
            cmp         #R_OPEN
            beq         @open
            cmp         #R_DUP
            beq         @open
            cmp         #R_CLUNK
            bne         @done
            lda         w_cons,Y                            ; (Its last: the window goes, but window 0)
            beq         @done
            dec         a
            sta         w_cons,Y
            bne         @done
            tya
            beq         @done
            tax
            jsr         w_free
@done:
            clc
            rts

@open:
            lda         w_cons,Y
            inc         a
            sta         w_cons,Y
            clc
            rts

; /ser: a read, a write; its fids for reading counted (with any, the line is /ser's)
h_ser:
            cmp         #R_READ
            bne         :+
            jmp         r_ser
:
            cmp         #R_WRITE
            bne         :+
            jmp         s_write
:
            tay                                             ; (.Y: the request)
            lda         srv_fid_mode,X                      ; For reading?
            and         #O_RW_MASK
            cmp         #O_WRITE
            beq         @done
            cpy         #R_OPEN
            beq         @open
            cpy         #R_DUP
            beq         @open
            cpy         #R_CLUNK
            bne         @done
            lda         ser_rd
            beq         @done
            dec         ser_rd
            bne         @done
            stz         pfx                                 ; Its last: the line the console's again, the keys
            jsr         rx_reset                            ;   acted on (what came for it and wasn't read
            lda         #1                                  ;   dropped), the shown window painted
            sta         ts_ser
            sta         ts_scr
            inc         TASK_EVENT                          ; (Its writers look again)
@done:
            clc
            rts

@open:
            inc         ser_rd
            lda         #$FF                                ; The line's /ser's: the irq entry stores every byte
            sta         pfx                                 ;   as it is (Ctrl-C, Ctrl-\, Ctrl-] too)
            clc
            rts

; /wnew: a read waits for Ctrl-] c, then makes a window: "N" and an LF
h_wnew:
            cmp         #R_READ
            beq         :+
            clc
            rts
:
            lda         want_new
            bne         :+
            jmp         again
:
            stz         want_new
            cmp         #2                                  ; (Ctrl-] s, v: in the shown window's group; Ctrl-] c, a
            lda         #$FF                                ;   group of its own: a shell session)
            bcc         :+
            ldx         w_in
            lda         w_grp,X
:
            jsr         w_make
            bcs         @done
            jsr         w_show                              ; (The user's: shown, as rio's new window is)
            txa
            jsr         num_buf                             ; ("N" and an LF)
            jmp         r_give

@done:
            rts

; iobuf = .A in decimal and an LF; .X = their bytes
num_buf:
            ldx         #0
            cmp         #10
            bcc         :+
            sbc         #10                                 ; (C = 1: 10-15)
            pha
            lda         #'1'
            sta         iobuf
            pla
            inx
:
            ora         #'0'
            sta         iobuf,X
            inx
            lda         #LF
            sta         iobuf,X
            inx
            rts

; /kbdin: a write's bytes are the window's keys, as if typed (Plan 9's rio's kbdin: forth's send writes a line, and
; its Enter, a CR, there).  As many as its queue has room for (and IOBUF at most), the bytes taken the write's count
; done: the kernel sends the rest in the next request.  No room: E_AGAIN, the writer waiting till the window's
; reader takes a key (iq_get: kbd_wait)
h_kbdin:
            cmp         #R_WRITE
            beq         :+
            clc
            rts
:
            ldy         srv_fid_aux,X                       ; Its window's queue: its room (a key's place kept
            sec                                             ;   free, as iq_put has it) ...
            lda         w_iqt,Y
            sbc         w_iqh,Y
            dec         a
            and         #INQ_SIZE - 1
            bne         :+
            lda         #1
            sta         kbd_wait
            jmp         again
:
            cmp         #IOBUF                              ;   IOBUF at most ...
            bcc         :+
            lda         #IOBUF
:
            ldx         TASK_INBOX + RQ_COUNT + 1           ;   and the count at most
            bne         :+
            cmp         TASK_INBOX + RQ_COUNT
            bcc         :+
            lda         TASK_INBOX + RQ_COUNT
:
            sta         cnt
            phy
            stz         n
            stz         n + 1
            jsr         from_client
            plx                                             ; (.X: the window)
            bcs         @done
            ldy         #0
:
            cpy         cnt
            beq         :+
            lda         iobuf,Y
            phy
            jsr         iq_put
            ply
            iny
            bra         :-
:
            inc         TASK_EVENT                          ; (Its reader looks again)
            lda         cnt
            sta         TASK_INBOX + RQ_DONE
            stz         TASK_INBOX + RQ_DONE + 1
            clc
@done:
            rts

; /snarf: the console's cut buffer (all the windows' one).  A read gives it from the offset, IOBUF bytes at a time; a
; write puts its bytes there (IOBUF at a time: the kernel sends the rest in the next request), emptying it first if
; it's at its start, its bank taken the first time.  SNARF_MAX bytes at most: past them, E_NOSPC
h_snarf:
            cmp         #R_READ
            beq         @read
            cmp         #R_WRITE
            beq         @write
            clc
            rts
@read:
            stz         TASK_INBOX + RQ_DONE
            stz         TASK_INBOX + RQ_DONE + 1
            lda         TASK_INBOX + RQ_OFFSET + 2          ; (n: its bytes from the offset; none past its end)
            ora         TASK_INBOX + RQ_OFFSET + 3
            bne         @none
            sec
            lda         sn_len
            sbc         TASK_INBOX + RQ_OFFSET
            sta         n
            lda         sn_len + 1
            sbc         TASK_INBOX + RQ_OFFSET + 1
            sta         n + 1
            bcc         @none
            ora         n
            beq         @none
            jsr         @count
            jsr         @at                                 ; Out of its bank, through iobuf
            ldy         cnt
:
            dey
            bmi         :+
            lda         (p),Y
            sta         iobuf,Y
            bra         :-
:
            pla
            sta         $00
            LDR         r0, iobuf
            MOVR        r1, TASK_INBOX + RQ_BUF
            lda         cnt
            sta         r2
            stz         r2 + 1
            jsr         CLIENT_WRITE
            lda         cnt
            sta         TASK_INBOX + RQ_DONE
@none:
            clc
            rts

@write:
            lda         TASK_INBOX + RQ_OFFSET + 2
            ora         TASK_INBOX + RQ_OFFSET + 3
            bne         @nospc
            lda         TASK_INBOX + RQ_OFFSET              ; (At its start: emptied)
            ora         TASK_INBOX + RQ_OFFSET + 1
            bne         :+
            stz         sn_len
            stz         sn_len + 1
:
            jsr         sn_bankget                          ; (Its bank, the first time)
            bcs         @done
            sec                                             ; n: the room from the offset
            lda         #<SNARF_MAX
            sbc         TASK_INBOX + RQ_OFFSET
            sta         n
            lda         #>SNARF_MAX
            sbc         TASK_INBOX + RQ_OFFSET + 1
            sta         n + 1
            bcc         @nospc
            ora         n
            beq         @nospc
            jsr         @count
            stz         n
            stz         n + 1
            jsr         from_client                         ; (iobuf)
            bcs         @done
            jsr         @at                                 ; Into its bank
            ldy         cnt
:
            dey
            bmi         :+
            lda         iobuf,Y
            sta         (p),Y
            bra         :-
:
            pla
            sta         $00
            clc                                             ; Its length: to the write's end, if that's further
            lda         TASK_INBOX + RQ_OFFSET
            adc         cnt
            tax
            lda         TASK_INBOX + RQ_OFFSET + 1
            adc         #0
            cmp         sn_len + 1
            bcc         @kept
            bne         :+
            cpx         sn_len
            bcc         @kept
:
            stx         sn_len
            sta         sn_len + 1
@kept:
            lda         cnt
            sta         TASK_INBOX + RQ_DONE
            stz         TASK_INBOX + RQ_DONE + 1
            clc
@done:
            rts
@nospc:
            lda         #E_NOSPC
            sec
            rts

@count:                                                     ; (cnt: n, IOBUF and the count, the least)
            lda         n + 1
            bne         :+
            lda         n
            cmp         #IOBUF
            bcc         :++
:
            lda         #IOBUF
:
            ldx         TASK_INBOX + RQ_COUNT + 1
            bne         :+
            cmp         TASK_INBOX + RQ_COUNT
            bcc         :+
            lda         TASK_INBOX + RQ_COUNT
:
            sta         cnt
            rts
@at:                                                        ; (p: the offset in its bank, which is selected; the
            lda         TASK_INBOX + RQ_OFFSET              ;   bank that was left on the stack, for the caller)
            sta         p
            lda         TASK_INBOX + RQ_OFFSET + 1
            clc
            adc         #>BANK_WINDOW
            sta         p + 1
            pla                                             ; (The return: under the bank)
            tax
            pla
            tay
            lda         $00
            pha
            phy
            phx
            lda         sn_bank
            sta         $00
            rts

; /text: a read, the window's scrollback and screen as text (vt.s's vt_text)
h_text:
            cmp         #R_READ
            beq         :+
            clc
            rts
:
            lda         srv_fid_aux,X
            tax
            FAR2        vt_text
            rts

; /cons: a read.  Cooked, a line (or what's left of one); raw, the keys there are.  IN: .X = the fid
r_cons:
            ldy         srv_fid_aux,X                       ; (Its reader: its chrome's %p)
            lda         TASK_INBOX + RQ_CLIENT
            inc         a
            cmp         w_rdr,Y
            beq         :+
            sta         w_rdr,Y
            lda         #3
            tsb         chr_dirty
:
            lda         srv_fid_aux,X
            jsr         load
            lda         raw
            bne         r_keys
            ldx         lw                                  ; (Cooked: the window's answers dropped)
            stz         ans_n,X
            stz         ans_r,X
@line:
            lda         ln_ready                            ; A line, ended?
            bne         @give
            lda         eof
            bne         @eof
            jsr         edit                                ; Keys into it
            bcc         @line
            bra         again

@eof:                                                       ; Ctrl-D on an empty line: a read of 0
            stz         eof
            stz         TASK_INBOX + RQ_DONE
            stz         TASK_INBOX + RQ_DONE + 1
            clc
            rts

@give:                                                      ; What's left of it, or what's asked if less
            sec
            sbc         ln_off
            ldx         TASK_INBOX + RQ_COUNT + 1
            bne         :+
            cmp         TASK_INBOX + RQ_COUNT
            bcc         :+
            lda         TASK_INBOX + RQ_COUNT
:
            sta         r2
            sta         TASK_INBOX + RQ_DONE
            stz         r2 + 1
            stz         TASK_INBOX + RQ_DONE + 1
            clc
            lda         #<ln_buf
            adc         ln_off
            sta         r0
            lda         #>ln_buf
            adc         #0
            sta         r0 + 1
            MOVR        r1, TASK_INBOX + RQ_BUF
            jsr         CLIENT_WRITE
            clc
            lda         ln_off
            adc         r2
            sta         ln_off
            cmp         ln_ready
            bne         :+
            stz         ln_ready                            ; (All of it: the next line's edited from here)
            stz         ln_off
            stz         ln_len
            stz         ln_pos
:
            clc
            rts

; Not yet: the client waits for the event count to change (a key in, room to send, a window shown)
again:
            lda         #E_AGAIN
            sec
            rts

; /cons, raw: the keys there are (decoded), as many as are asked; or E_AGAIN
r_keys:
            ldx         #0
@key:
            jsr         r_room
            bcs         @out
            phx
            jsr         key_next
            plx
            bcs         @out
            sta         iobuf,X
            inx
            bra         @key

@out:
            bra         r_give

; C = 0 if iobuf has room for another byte at .X for the read (IOBUF, or the count asked if less).  Keeps .X
r_room:
            cpx         #IOBUF
            bcs         @full
            lda         TASK_INBOX + RQ_COUNT + 1
            bne         @ok
            cpx         TASK_INBOX + RQ_COUNT
            bcs         @full
@ok:
            clc
            rts

@full:
            sec
            rts

; The .X bytes in iobuf to the client; none: E_AGAIN
r_give:
            txa
            beq         again
            sta         r2
            sta         TASK_INBOX + RQ_DONE
            stz         r2 + 1
            stz         TASK_INBOX + RQ_DONE + 1
            LDR         r0, iobuf
            MOVR        r1, TASK_INBOX + RQ_BUF
            jsr         CLIENT_WRITE
            clc
            rts

; /ser: the bytes there are, as they are (the count asked, if less), straight from the ring to the client (in two
; parts, if they go past its end); or E_AGAIN.  At 115200 the bytes come 311 cycles apart and the interrupt has 190
; of them: a 1K XMODEM block comes faster than it can be taken, and waits in the ring (its pages: the irq entry's
; @ser), the reads costing as little a byte as can be
r_ser:
            php                                             ; The bytes in the ring (its head as the irq entry left
            sei                                             ;   it, both bytes) ...
            lda         rx_head
            ldx         rx_hp + 1
            plp
            sec
            sbc         rx_tail
            sta         n
            txa
            sbc         rx_tp + 1
            and         #RX_PAGES - 1                       ;   (its pages, round)
            sta         n + 1
            lda         n                                   ;   no more than asked
            cmp         TASK_INBOX + RQ_COUNT
            lda         n + 1
            sbc         TASK_INBOX + RQ_COUNT + 1
            bcc         :+
            MOVR        n, TASK_INBOX + RQ_COUNT
:
            lda         n
            ora         n + 1
            bne         :+
            jmp         again
:
            stz         TASK_INBOX + RQ_DONE
            stz         TASK_INBOX + RQ_DONE + 1
            clc                                             ; m: the tail's address
            lda         rx_tp
            adc         rx_tail
            sta         m
            lda         rx_tp + 1
            adc         #0
            sta         m + 1
            sec                                             ; To the ring's end first, if they go past it
            lda         #<(rx_buf + RX_PAGES * 256)
            sbc         m
            sta         r2
            lda         #>(rx_buf + RX_PAGES * 256)
            sbc         m + 1
            sta         r2 + 1
            lda         r2
            cmp         n
            lda         r2 + 1
            sbc         n + 1
            bcs         @rest
            jsr         ser_part
            LDR         m, rx_buf
@rest:
            MOVR        r2, n
            jsr         ser_part
            sec                                             ; The tail moved, both bytes (the irq entry's room):
            lda         m                                   ;   m less the ring's start, its page and place in it
            sbc         #<rx_buf
            tax
            lda         m + 1
            sbc         #>rx_buf
            and         #RX_PAGES - 1                       ;   (at the ring's end: its start)
            clc
            adc         #>rx_buf
            php
            sei
            stx         rx_tail
            sta         rx_tp + 1
            plp
            clc
            rts

; r2 bytes at m (not past the ring's end) to the client, after those sent: m, RQ_DONE on, n less
ser_part:
            MOVR        r0, m
            clc
            lda         TASK_INBOX + RQ_BUF
            adc         TASK_INBOX + RQ_DONE
            sta         r1
            lda         TASK_INBOX + RQ_BUF + 1
            adc         TASK_INBOX + RQ_DONE + 1
            sta         r1 + 1
            jsr         CLIENT_WRITE
            clc
            lda         m
            adc         r2
            sta         m
            lda         m + 1
            adc         r2 + 1
            sta         m + 1
            clc
            lda         TASK_INBOX + RQ_DONE
            adc         r2
            sta         TASK_INBOX + RQ_DONE
            lda         TASK_INBOX + RQ_DONE + 1
            adc         r2 + 1
            sta         TASK_INBOX + RQ_DONE + 1
            sec
            lda         n
            sbc         r2
            sta         n
            lda         n + 1
            sbc         r2 + 1
            sta         n + 1
            rts

; /cons: a write, to the window's screen (vt.s).  A window that isn't shown takes it all, as does the shown one while
; the line is /ser's or the serial port's off; the shown one as much as the send ring has room for now (VT_ROOM a
; byte: vt_write), so all of it goes out at this request's end, and the writer, waiting for room, comes back for the
; rest (the kernel sends it again); none while the serial port's being painted.  None taken: E_AGAIN.  IN: .X = the
; fid
w_write:
            lda         srv_fid_aux,X
            jsr         load
            jsr         scr_ready
            ldx         lw                                  ; Held: none yet
            lda         w_hold,X
            beq         :+
            jmp         again
:
            lda         w_jump,X                            ; The shown window (but scroll jump's), the serial port
            bne         :+                                  ;   being painted?
            txa
            cmp         w_in
            bne         :+
            lda         term
            and         #TERM_SERIAL
            beq         :+
            lda         ser_rd
            bne         :+
            lda         ts_ser
            beq         :+
            jmp         again
:
            stz         n                                   ; (n: the count done)
            stz         n + 1
@part:
            sec                                             ; This part: IOBUF at most, the rest if less
            lda         TASK_INBOX + RQ_COUNT
            sbc         n
            sta         p
            lda         TASK_INBOX + RQ_COUNT + 1
            sbc         n + 1
            sta         p + 1
            ora         p
            beq         @end
            lda         #IOBUF
            ldx         p + 1
            bne         :+
            cmp         p
            bcc         :+
            lda         p
:
            sta         cnt
            jsr         from_client                         ; Its bytes, into iobuf
            FAR2        vt_write                            ; (.A: those taken)
            sta         p
            clc
            adc         n
            sta         n
            bcc         :+
            inc         n + 1
:
            lda         p
            cmp         cnt
            beq         @part
@end:
            jsr         ring
            lda         n
            ora         n + 1
            bne         :+
            jmp         again
:
            MOVR        TASK_INBOX + RQ_DONE, n
            clc
            rts

; The bell: a BEL the shown window sent rings the sound driver's, by a write to #a/bell (opened the first time: one
; of the console's two calls to another driver, the screen's the other).  No sound driver: no bell, from then on.
; Modifies .A, .X, .Y, r0-r2
ring:
            lda         bell
            beq         @done
            stz         bell
            lda         bell_st
            cmp         #1
            beq         @write
            bcs         @done                               ; (2: none)
            LDR         r0, s_bell
            lda         #O_WRITE
            jsr         OPEN
            ldx         #2
            bcs         :+
            sta         bell_fd
            ldx         #1
:
            stx         bell_st
            cpx         #1
            bne         @done
@write:
            LDR         r0, bell_st                         ; (A byte: anything)
            LDR         r1, 1
            lda         bell_fd
            jsr         WRITE
@done:
            rts

; /ser: a write, straight into the send ring as it is, as much as there's room for; none: E_AGAIN
s_write:
            stz         n
            stz         n + 1
@part:
            sec
            lda         TASK_INBOX + RQ_COUNT
            sbc         n
            sta         p
            lda         TASK_INBOX + RQ_COUNT + 1
            sbc         n + 1
            sta         p + 1
            ora         p
            beq         @end
            jsr         tx_free
            cmp         #IOBUF
            bcc         :+
            lda         #IOBUF
:
            ldx         p + 1
            bne         :+
            cmp         p
            bcc         :+
            lda         p
:
            sta         cnt
            cmp         #0
            beq         @end
            jsr         from_client
            ldx         #0
:
            lda         iobuf,X
            jsr         tx_put
            inx
            cpx         cnt
            bne         :-
            clc
            lda         n
            adc         cnt
            sta         n
            bcc         :+
            inc         n + 1
:
            jsr         tx_start
            bra         @part

@end:
            jsr         tx_start
            lda         n
            ora         n + 1
            bne         :+
            jmp         again
:
            MOVR        TASK_INBOX + RQ_DONE, n
            clc
            rts

; cnt bytes of the client's write, from its buffer + n, into iobuf
from_client:
            lda         cnt
            sta         r2
            stz         r2 + 1
            LDR         r0, iobuf
            clc
            lda         TASK_INBOX + RQ_BUF
            adc         n
            sta         r1
            lda         TASK_INBOX + RQ_BUF + 1
            adc         n + 1
            sta         r1 + 1
            jmp         CLIENT_READ

; A reader's call ended by a note (R_FLUSH): the line the shown window was typing, gone
flush:
            lda         w_in
            jsr         load
            lda         ln_ready
            bne         :+
            stz         ln_len
            stz         ln_pos
            stz         ln_geo
            stz         esc_st
            stz         esc_wait
            jsr         nap_set
:
            clc
            rts

; ****************************************************************************
; Keys

; The next key of the loaded window: key_raw's; raw with keys vt, a key's sequence as a VT100 sends it, a byte at a
; time (its first now, the rest from kp_buf).  OUT: C = 0, .A = it; or C = 1: none yet.  Modifies .X, .Y, n, m, p
key_next:
            ldx         lw
            lda         kp_i,X
            cmp         kp_n,X
            bcs         @fresh
            inc         kp_i,X
            sta         n                                   ; (Its place: the window * KP_SIZE + those read)
            txa
            asl
            asl
            asl
            clc
            adc         n
            tay
            lda         kp_buf,Y
            clc
            rts
@fresh:
            stz         kp_n,X
            stz         kp_i,X
            jsr         key_raw
            bcs         @done
            ldx         raw
            beq         @key
            ldx         lw
            ldy         kvt,X
            beq         @key
            cmp         #KEY_UP
            bcc         @key
            cmp         #KEY_F12 + 1
            bcs         @key
            jmp         key_vt
@key:
            clc
@done:
            rts

; Key .A (KEY_*) as a VT100 sends it, into the loaded window's kp_buf: the cursor keys ESC [ A, or ESC O A with
; DECCKM, or ESC A in VT52 mode (Home and End H and F as they are); F1-F4 ESC O P-S (VT52: ESC P-S); Insert, Delete,
; Page Up and Down and F5-F12 ESC [ n ~ (xterm's numbers).  OUT: C = 0, .A = its first byte.  Modifies .X, .Y, m, p
key_vt:
            sta         m
            lda         lw
            FAR2        vt_keymodes                         ; (.A: DECCKM's, DECKPAM's bits; .X: VT52 mode's)
            sta         m + 1
            stx         p
            lda         lw                                  ; (p + 1: the window's place in kp_buf)
            asl
            asl
            asl
            sta         p + 1
            tay
            lda         #ESC
            sta         kp_buf,Y
            iny
            lda         m
            cmp         #KEY_END + 1
            bcs         @other
            sec                                             ; The cursor keys, Home and End: a letter
            sbc         #KEY_UP
            tax
            lda         kv_letter,X
            pha
            lda         p
            bne         @letter
            lda         m + 1
            and         #VM_CKM
            beq         :+
            lda         #'O'
            bra         @second
:
            lda         #'['
@second:
            sta         kp_buf,Y
            iny
@letter:
            pla
            sta         kp_buf,Y
            iny
            bra         @end
@other:
            cmp         #KEY_F1
            bcc         @tilde
            cmp         #KEY_F5
            bcs         @tilde
            sbc         #KEY_F1 - 1                         ; F1-F4: ESC O, P-S (C = 0)
            clc
            adc         #'P'
            pha
            lda         p
            bne         @letter
            lda         #'O'
            bra         @second
@tilde:
            lda         #'['                                ; ESC [ n ~
            sta         kp_buf,Y
            iny
            lda         m
            sec
            sbc         #KEY_INS
            tax
            lda         kv_tilde,X
            cmp         #10
            bcc         @one
            ldx         #'0'
:
            cmp         #10
            bcc         :+
            sbc         #10
            inx
            bra         :-
:
            pha
            txa
            sta         kp_buf,Y
            iny
            pla
@one:
            ora         #'0'
            sta         kp_buf,Y
            iny
            lda         #'~'
            sta         kp_buf,Y
            iny
@end:
            tya                                             ; Its bytes, the first read
            sec
            sbc         p + 1
            ldx         lw
            sta         kp_n,X
            lda         #1
            sta         kp_i,X
            ldy         p + 1
            lda         kp_buf,Y
            clc
            rts

.assert     KP_SIZE = 8 .and WIN_MAX * KP_SIZE <= 256, error, "key_next and key_vt: 8 bytes a window"

; The next key of the loaded window, the terminal's sequences decoded (KEY_*); raw, the window's answers first, as
; they came.  OUT: C = 0, .A = it; or C = 1: none yet (a sequence part-way in waits for the next call).  Modifies .X,
; .Y, n
key_raw:
            lda         key_pb
            beq         @answer
            stz         key_pb
            clc
            rts

@answer:                                                    ; Raw: the window's size changed first (KEY_RESIZE, but
            lda         raw                                 ;   for keys vt), then its answers (DA, DSR's ...: vt.s's),
            beq         @byte                               ;   as they came
            ldx         lw
            lda         w_rsz,X
            beq         :+
            stz         w_rsz,X
            lda         kvt,X
            bne         :+
            lda         #KEY_RESIZE
            clc
            rts
:
            lda         w_kf,X                              ; The group's focus moved: KEY_FOCUS, then the window
            beq         :+                                  ;   (kp_buf's: key_next gives it next; not for keys vt)
            stz         w_kf,X
            ldy         kvt,X
            bne         :+
            dec         a
            pha
            txa
            asl
            asl
            asl
            tay
            pla
            sta         kp_buf,Y
            lda         #1
            sta         kp_n,X
            stz         kp_i,X
            lda         #KEY_FOCUS
            clc
            rts
:
            lda         ans_r,X
            cmp         ans_n,X
            bcs         @byte
            sta         n                                   ; (Its place: the window * ANS_SIZE + those read)
            inc         ans_r,X
            stz         m + 1
            txa
            asl
            asl
            asl
            asl
            asl
            rol         m + 1
            clc
            adc         #<ans_buf
            sta         m
            lda         m + 1
            adc         #>ans_buf
            sta         m + 1
            ldy         n
            lda         ans_r,X                             ; (All read: none again)
            cmp         ans_n,X
            bne         :+
            stz         ans_r,X
            stz         ans_n,X
:
            lda         (m),Y
            clc
            rts

@byte:
            jsr         iq_get
            bcc         :+
            jmp         @none
:
            ldx         esc_st
            bne         @seq
            cmp         #ESC
            bne         @key
            inc         esc_st                              ; (1: ESC; when, for one alone)
            jsr         TICKS
            sta         esc_at
            stx         esc_at + 1
            bra         @byte

@key:
            clc
@done:
            rts

@seq:
            stz         esc_wait
            jsr         nap_set
            dex
            bne         @csi
            cmp         #'['                                ; ESC, then [ or O starts a sequence
            beq         @start
            cmp         #'O'
            beq         @ss3
            stz         esc_st                              ; Else ESC itself, and this key after it
            sta         key_pb
            lda         #ESC
            clc
            rts

@start:
            lda         #2
            sta         esc_st
            stz         esc_n
            stz         esc_semi
            stz         esc_mod
            bra         @byte

@ss3:
            lda         #3
            sta         esc_st
            bra         @byte

@csi:
            dex
            beq         :+
            jmp         @o
:
            cmp         #'0'                                ; ESC [: a digit of its number?
            bcc         @notdigit
            cmp         #'9' + 1
            bcs         @notdigit
            and         #$0F
            ldx         esc_semi                            ; (Its first number, or past a ; the modifiers'; past
            cpx         #2                                  ;   a second, not kept)
            bcs         @byte
            ldy         esc_n,X                             ; * 10, + the digit
            sty         p
            jsr         dec_add
            sta         esc_n,X
            bra         @byte

@notdigit:
            cmp         #';'
            bne         @final
            inc         esc_semi
            bra         @byte

@final:
            cmp         #$40                                ; (A byte before its letter: not ours)
            bcc         @byte
            stz         esc_st
            ldx         #CSI_N - 1                          ; A letter: its key
:
            cmp         csi_final,X
            beq         @csikey
            dex
            bpl         :-
            cmp         #'~'
            beq         :+
            jmp         @byte                               ; (Not one of ours: dropped)
:
            lda         esc_n                               ; ESC [ n ~: by n (200, 201: bracketed paste's)
            cmp         #200
            bcc         :+
            cmp         #202
            bcc         @bracket
:
            ldx         #TILDE_N - 1
:
            cmp         tilde_n,X
            beq         @tildekey
            dex
            bpl         :-
            jmp         @byte

@tildekey:
            lda         tilde_key,X
            cmp         #KEY_PGUP                           ; (Shift-PgUp, bound: the console's, not a key)
            bne         :+
            ldy         esc_mod
            cpy         #2
            bne         :+
            ldy         kb_ct + 2
            beq         :+
            jmp         @byte
:
            bra         @mods

@bracket:                                                   ; (Raw, keys vt: as it came, its ESC now, the rest
            ldy         raw                                 ;   kp_buf's; else dropped, as it's not a key)
            beq         @drop
            ldx         lw
            ldy         kvt,X
            beq         @drop
            pha
            txa
            asl
            asl
            asl
            tay
            lda         #'['
            sta         kp_buf,Y
            lda         #'2'
            sta         kp_buf + 1,Y
            lda         #'0'
            sta         kp_buf + 2,Y
            pla
            sec
            sbc         #200 - '0'
            sta         kp_buf + 3,Y
            lda         #'~'
            sta         kp_buf + 4,Y
            lda         #5
            sta         kp_n,X
            stz         kp_i,X
            lda         #ESC
            clc
            rts
@drop:
            jmp         @byte

@csikey:
            lda         csi_key,X
@mods:                                                      ; (Raw, keys mods, modified: KEY_MOD, its modifiers,
            ldy         raw                                 ;   then it, kp_buf's: key_next gives them next)
            beq         @plain
            ldx         lw
            ldy         w_kmod,X
            beq         @plain
            ldy         esc_mod
            cpy         #2
            bcc         @plain
            cpy         #9
            bcs         @plain
            pha
            txa
            asl
            asl
            asl
            tay
            lda         esc_mod
            dec         a
            sta         kp_buf,Y
            pla
            sta         kp_buf + 1,Y
            lda         #2
            sta         kp_n,X
            stz         kp_i,X
            lda         #KEY_MOD
@plain:
            clc
            rts

@o:                                                         ; ESC O, then a letter
            stz         esc_st
            ldx         #SS3_N - 1
:
            cmp         ss3_final,X
            beq         @ss3key
            dex
            bpl         :-
            jmp         @byte

@ss3key:
            lda         ss3_key,X
            clc
            rts

@none:                                                      ; None yet: an ESC alone ESC_TICKS is a key
            lda         esc_st
            cmp         #1
            beq         :+
            sec
            rts
:
            jsr         TICKS
            sec
            sbc         esc_at
            tay
            txa
            sbc         esc_at + 1
            bne         @alone
            cpy         #ESC_TICKS
            bcs         @alone
            lda         #1                                  ; (Not yet: timer 2's rounds bring its reader back)
            sta         esc_wait
            jsr         nap_set
            jsr         esc_nap
            sec
            rts

@alone:
            stz         esc_st
            stz         esc_wait
            jsr         nap_set
            lda         #ESC
            clc
            rts

; t2_napr: the rounds timer 2 naps when it's idle (t2_next): an Escape alone awaited's (ESC_NAP), else the chrome's
; time's (CLK_NAP), else none.  Keeps .A, .X, .Y
nap_set:
            pha
            lda         esc_wait
            beq         :+
            lda         #ESC_NAP
            bra         @set
:
            lda         clk_on
            beq         @set
            lda         #CLK_NAP
@set:
            sta         t2_napr
            pla
            rts

; Timer 2 napping, if it's idle (nothing to send, no nap): its rounds add to the event count (t2_next), so an
; Escape's reader comes back to look at the time.  Modifies .A
esc_nap:
            php
            sei
            lda         tx_busy
            ora         t2_nap
            bne         @done
            lda         #ESC_NAP
            sta         t2_left
            lda         #$FF
            sta         t2_hi
            sta         t2_nap
            sta         VIA_T2CL
            sta         VIA_T2CH                            ; (It starts)
@done:
            plp
            rts

; ****************************************************************************
; The line editor (cooked), in the loaded window: its echo goes into the window's text

; Keys into the line, echoed, while there are keys and the text has room for a key's echo.  OUT: C = 0: a line
; ended (ln_ready), or the input did (eof); C = 1: not yet
edit:
            jsr         w_room
            lda         m + 1
            bne         :+
            lda         m
            cmp         #ECHO_ROOM
            bcc         @wait
:
            ldx         lw                                  ; The window resized: the line drawn again
            lda         w_rsz,X
            beq         :+
            stz         w_rsz,X
            jsr         ed_resize
:
            jsr         key_next
            bcs         @wait
            cmp         #LF                                 ; An LF just after a CR: the same Enter
            bne         :+
            ldx         was_cr
            stz         was_cr
            bne         edit
:
            stz         was_cr
            pha                                                 ; (Where the line is: found with its first key)
            jsr         ed_geo
            pla
            ldx         #EDIT_N - 1                         ; One of the keys that edit?
:
            cmp         edit_keys,X
            beq         @special
            dex
            bpl         :-
            cmp         #' '                                ; Else a character, if it's one
            bcc         edit
            cmp         #DEL
            bcs         edit
            jsr         ed_insert
            bra         edit

@special:
            txa
            asl
            tax
            jsr         @go
            bcc         edit                                ; (C = 1: the line, or the input, ended)
            clc
            rts

@go:
            jmp         (edit_vec,X)

@wait:
            sec
            rts

; A character at the cursor: the rest moves right
ed_insert:
            ldx         ln_len
            cpx         #LINE_MAX
            bcs         @full
            pha
:
            cpx         ln_pos                              ; (From the end down to the cursor)
            beq         :+
            lda         ln_buf - 1,X
            sta         ln_buf,X
            dex
            bra         :-
:
            pla
            sta         ln_buf,X
            inc         ln_len
            jsr         ed_rest                             ; It and the rest ...
            inc         ln_pos
            lda         ln_pos                              ;   and the cursor back after it
            jmp         ed_goto

@full:
            rts

; Enter: the line ends, with an LF after it (the cursor to its end first); CR: and the next key's LF is the same
; Enter
ed_cr:
            lda         #1
            sta         was_cr
ed_lf:
            lda         ln_len
            jsr         ed_goto
            stz         ln_geo
            ldx         ln_len
            lda         #LF
            sta         ln_buf,X
            jsr         hist_add
            ldx         ln_len
            inx
            stx         ln_ready
            stz         ln_off
            lda         #LF                                 ; (Out as CR LF)
            jsr         w_put
            sec
            rts

; Ctrl-D: on an empty line, the end of the input; else the line ends, as it is
ed_eof:
            stz         ln_geo
            lda         ln_len
            bne         :+
            inc         eof
            sec
            rts
:
            jsr         hist_add
            lda         ln_len
            sta         ln_ready
            stz         ln_off
            sec
            rts

; Backspace (BS or DEL): the character before the cursor
ed_bs:
            lda         ln_pos
            beq         ed_none
            dec         ln_pos
            lda         ln_pos
            jsr         ed_goto
            bra         ed_cut

; Delete: the character at the cursor
ed_del:
            lda         ln_pos
            cmp         ln_len
            bcs         ed_none
ed_cut:                                                     ; The character at the cursor out: the rest moves left
            ldx         ln_pos
:
            inx
            cpx         ln_len
            bcs         :+
            lda         ln_buf,X
            sta         ln_buf - 1,X
            bra         :-
:
            dec         ln_len
            jsr         ed_rest                             ; The rest, what was past its end erased, and the
            jsr         ed_tail                             ;   cursor back
            lda         ln_pos
            jsr         ed_goto
ed_none:
            clc
            rts

ed_left:
            lda         ln_pos
            beq         ed_none
            dec         ln_pos
            lda         ln_pos
            jsr         ed_goto
            clc
            rts

ed_right:                                                   ; (The character written again: past it)
            ldx         ln_pos
            cpx         ln_len
            bcs         ed_none
            lda         ln_buf,X
            jsr         ed_put
            inc         ln_pos
            clc
            rts

ed_home:
            stz         ln_pos
            lda         #0
            jsr         ed_goto
            clc
            rts

ed_end:
            lda         ln_len
            sta         ln_pos
            jsr         ed_goto
            clc
            rts

; Ctrl-U: the whole line
ed_kill:
            lda         #0
            jsr         ed_goto
            stz         ln_len
            stz         ln_pos
            stz         hi_at
            jsr         ed_tail
            clc
            rts

; Up and Down: the history's lines, older and newer (past the newest: an empty line)
ed_up:
            lda         hi_at
            cmp         hi_n
            bcs         ed_none
            inc         hi_at
            bra         hist_show

ed_down:
            lda         hi_at
            beq         ed_none
            dec         hi_at
hist_show:                                                  ; The line hi_at back in place of this one
            lda         #0
            jsr         ed_goto
            stz         ln_len
            lda         hi_at
            beq         @shown
            jsr         hist_slot                           ; p: its slot
            lda         (p)
            sta         ln_len
            tay
            beq         @shown
:
            lda         (p),Y
            sta         ln_buf - 1,Y
            dey
            bne         :-
@shown:
            stz         ln_pos
            jsr         ed_rest
            jsr         ed_tail
            lda         ln_len
            sta         ln_pos
            clc
            rts

; ****************************************************************************
; The history (the loaded window's)

; The line (not if it's empty) as the newest
hist_add:
            stz         hi_at
            lda         ln_len
            beq         @done
            lda         hi_top                              ; The next slot
            inc         a
            and         #HIST_N - 1
            sta         hi_top
            lda         hi_n
            cmp         #HIST_N
            bcs         :+
            inc         hi_n
:
            lda         #1
            jsr         hist_slot                           ; (1 back: the newest, now this one)
            ldy         ln_len
            tya
            sta         (p)
:
            lda         ln_buf - 1,Y
            sta         (p),Y
            dey
            bne         :-
@done:
            rts

; p = the slot of the line .A back (1: the newest), in the loaded window's history
hist_slot:
            sta         p
            lda         hi_top
            sec
            sbc         p
            inc         a
            and         #HIST_N - 1
            lsr                                             ; * HIST_SIZE (128)
            sta         p + 1
            lda         #0
            ror
            clc
            adc         #<hist
            sta         p
            lda         p + 1
            adc         #>hist
            sta         p + 1
            lda         lw                                  ; + the window's (HIST_N * HIST_SIZE: 2 pages)
            asl
            clc
            adc         p + 1
            sta         p + 1
            rts

.assert     HIST_SIZE = 128 .and HIST_N * HIST_SIZE = 512, error, "hist_slot: 4 slots of 128 bytes a window"
.assert     HIST_SIZE > LINE_MAX, error, "A history slot holds a line"

; ****************************************************************************
; Echo: into the loaded window's text (edit has made sure of the room)

; Where the line is: its first key finds it (the cursor after the prompt: vt.s's), and its characters' places follow
; from that and the row's width (ln_w): a character's offset from its first row's column 0 is ln_s0 + its place

; The line's place found, if it isn't known yet.  Modifies .A, .X, .Y
ed_geo:
            lda         ln_geo
            bne         @done
            ldx         lw
            FAR2        vt_cursor                           ; (.A: its column, .X: the columns, .Y: <> 0 past the
            sta         ln_s0                               ;   last)
            stx         ln_w
            stz         ln_pend
            tya
            beq         :+
            inc         ln_s0                               ; (Past the last: the line starts on the next row)
            inc         ln_pend
:
            stz         ln_at
            stz         ln_shown
            inc         ln_geo
@done:
            rts

; .A, an offset from the line's first row's column 0: .X its row (from that one), .A its column
ed_rc:
            ldx         #0
:
            cmp         ln_w
            bcc         :+
            sbc         ln_w
            inx
            bra         :-
:
            rts

; .A, the line's character at the cursor (ln_at), written: the cursor past it (pending, if it was its row's last)
ed_put:
            jsr         w_put
            clc
            lda         ln_s0
            adc         ln_at
            jsr         ed_rc
            inc         a
            stz         ln_pend
            cmp         ln_w
            bne         :+
            inc         ln_pend
:
            inc         ln_at
            rts

; The line from the cursor (ln_at) to its end, written
ed_rest:
            ldx         ln_at
            cpx         ln_len
            bcs         :+
            lda         ln_buf,X
            jsr         ed_put
            bra         ed_rest
:
            lda         ln_len
            cmp         ln_shown
            bcc         :+
            sta         ln_shown
:
            rts

; What was on the screen past the line's end (it's shorter now) erased, from the cursor there (ln_at = ln_len): to its
; row's end (EL), or to the screen's (ED) if it went on to the rows below
ed_tail:
            lda         ln_len
            cmp         ln_shown
            bcs         @done
            lda         ln_pend                             ; (Pending: the next row's start first, where the old
            beq         :+                                  ;   line went on)
            lda         #1
            ldx         #'B'
            jsr         echo_csi
            lda         #CR
            jsr         w_put
            stz         ln_pend
:
            clc                                             ; The end's row, and the old end's
            lda         ln_s0
            adc         ln_len
            jsr         ed_rc
            stx         n
            clc
            lda         ln_s0
            adc         ln_shown
            dec         a
            jsr         ed_rc
            ldy         #'K'
            cpx         n
            beq         :+
            ldy         #'J'
:
            phy
            lda         #ESC
            jsr         w_put
            lda         #'['
            jsr         w_put
            pla
            jsr         w_put
            lda         ln_len
            sta         ln_shown
@done:
            rts

; The terminal's cursor to the line's place .A: up or down to its row, then to its column (one back: BS; else CHA).
; The line's end at a row's start (its row before full) is left as a terminal leaves it there: past the row before's
; last column, that character written again.  Modifies .A, .X, .Y, m, n
ed_goto:
            cmp         ln_at
            bne         :+
            rts
:
            sta         m                                   ; (Where to)
            clc                                             ; Where it is: n its row, n + 1 its column
            lda         ln_s0
            adc         ln_at
            sec
            sbc         ln_pend                             ; (Pending: the row before's last column)
            jsr         ed_rc
            stx         n
            sta         n + 1
            stz         eg_x                                ; Where it's to be: eg_c its column (eg_x <> 0: the
            clc                                             ;   character before it written again)
            lda         ln_s0
            adc         m
            jsr         ed_rc
            cmp         #0
            bne         :+
            ldy         m
            beq         :+
            cpy         ln_len
            bne         :+
            dex                                             ; (The end, at a row's start: the row before's last
            lda         ln_w                                ;   column)
            dec         a
            inc         eg_x
:
            sta         eg_c
            txa                                             ; Up or down
            sec
            sbc         n
            beq         @same
            bcs         @down
            eor         #$FF
            inc         a
            ldx         #'A'
            bra         @vert
@down:
            ldx         #'B'
@vert:
            jsr         echo_csi
            stz         ln_pend
            bra         @col
@same:
            lda         ln_pend
            bne         @cha
@col:
            lda         eg_c                                ; Then to the column
            cmp         n + 1
            beq         @there
            inc         a
            cmp         n + 1
            bne         @cha
            lda         #BS
            jsr         w_put
            bra         @there
@cha:
            stz         ln_pend
            lda         eg_c
            inc         a
            ldx         #'G'
            jsr         echo_csi
@there:
            lda         m
            sta         ln_at
            lda         eg_x
            beq         :+
            dec         ln_at
            ldx         ln_at
            lda         ln_buf,X
            jmp         ed_put
:
            rts

; The window's size changed while a line's being edited: the line drawn again at the new width, from its first row
; (the cursor's row in it as it was laid out, up), or from the next row's start if its first column's past the new
; width; what was below it erased
ed_resize:
            lda         ln_geo
            beq         @done
            ldx         lw
            FAR2        vt_cursor                           ; (.X: the columns now)
            cpx         ln_w
            beq         @done
            phx
            clc                                             ; Up to its first row
            lda         ln_s0
            adc         ln_at
            sec
            sbc         ln_pend
            jsr         ed_rc
            txa
            ldx         #'A'
            jsr         echo_csi
            pla
            sta         ln_w
            lda         ln_s0
            cmp         ln_w
            bcc         @col
            lda         #LF                                 ; (From the next row's start: out as CR LF)
            jsr         w_put
            stz         ln_s0
            bra         @erase
@col:
            inc         a
            ldx         #'G'
            jsr         echo_csi
@erase:
            lda         #ESC
            jsr         w_put
            lda         #'['
            jsr         w_put
            lda         #'J'
            jsr         w_put
            stz         ln_at
            stz         ln_pend
            stz         ln_shown
            jsr         ed_rest
            lda         ln_pos
            jmp         ed_goto
@done:
            rts

; ESC [ .A .X (the cursor moved .A places: .X = 'C' right, 'D' left); nothing if .A = 0
echo_csi:
            cmp         #0
            beq         @done
            pha
            lda         #ESC
            jsr         w_put
            lda         #'['
            jsr         w_put
            pla
            jsr         echo_dec
            txa
            jmp         w_put

@done:
            rts

; .A in decimal, no leading zeros.  Keeps .X
echo_dec:
            stz         cnt                                 ; (cnt <> 0: a digit has gone out)
            ldy         #100
            jsr         @digit
            ldy         #10
            jsr         @digit
            ldy         #1
            sty         cnt                                 ; (The units' digit always goes)
@digit:                                                     ; .A's digit for .Y (100, 10 or 1) out; .A = the rest
            sty         p
            ldy         #'0'
            sec
:
            sbc         p
            bcc         :+
            iny
            bra         :-
:
            adc         p                                   ; (Back one: C = 0)
            cpy         #'0'
            bne         @out
            ldy         cnt
            beq         @none                               ; (A leading 0)
            ldy         #'0'
@out:
            pha
            tya
            jsr         w_put
            pla
            inc         cnt
@none:
            rts

; ****************************************************************************
; The chrome (W4): rendered here, drawn by vt.s

; Chrome row .A (CR_BAR, CR_HEAD, CR_FOOT) of the shown window (loaded in vt.s: v_cols ...), .X cells wide (CR_MAX - 1
; at most), rendered from its format into cr_c, cr_a and cr_f (characters, colours, rendition), cr_n of them, all of
; the row (FAR1: vt.s's).  The codes: %n and %g its number (and its group's: the same till W5), %l its label (with
; none, its program's name), %p its program (the task that last read it), %s its status line, %w its group's windows
; (itself till W5), %G the groups (each window till W5): each its number, ! (a bell) or + (written to, monitor on), a
; space and its label, the shown one reversed; %c, %r its columns, rows; %m its modes; %t, %d the time, the date; %L
; the LEDs (DECLL: 1-4, . off); %= what follows at the row's right end; %[n;n...] the rendition (SGR's numbers: 0, 1,
; 2, 4, 5, 7, 8, 22-28, 30-37, 39, 40-47, 49, 90-97, 100-107); %% a %.  Modifies .A, .X, .Y, p, m, n
chr_render:
            cpx         #CR_MAX
            bcc         :+
            ldx         #CR_MAX - 1
:
            stx         cr_w
            cmp         #CR_BAR                             ; Its format
            bne         :+
            lda         #<bar_fmt
            sta         p
            lda         #>bar_fmt
            sta         p + 1
            bra         @go
:
            dec         a                                   ; (.Y: 0 the header, 1 the footer)
            tay
            lda         w_in
            jsr         fmt_at
            lda         m
            sta         p
            lda         m + 1
            sta         p + 1
@go:
            stz         cr_n
            stz         cr_i
            lda         #COL_DEF
            sta         cr_ca
            stz         cr_cf
            lda         #$FF
            sta         cr_rx
@ch:
            ldy         cr_i
            lda         (p),Y
            beq         @end
            inc         cr_i
            cmp         #'%'
            beq         @code
            jsr         cr_put
            bra         @ch
@code:
            ldy         cr_i
            lda         (p),Y
            beq         @end
            inc         cr_i
            ldx         #CRC_N - 1
:
            cmp         cr_codes,X
            beq         :+
            dex
            bpl         :-
            bra         @ch                                 ; (Not a code: nothing)
:
            txa
            asl
            tax
            jsr         @do
            bra         @ch
@do:
            jmp         (cr_vec,X)

@end:
            lda         cr_rx                               ; A right part (from %=): moved to the row's end
            cmp         #$FF
            beq         @fill
            sec
            lda         cr_n
            sbc         cr_rx
            sta         m                                   ; (Its cells ...
            sec
            lda         cr_w
            sbc         m                                   ;   where they go: past the left part)
            bcc         @fill
            cmp         cr_rx
            bcc         @fill
            beq         @fill
            sta         m + 1
            sec
            sbc         cr_rx
            sta         n                                   ; (How far)
            ldx         cr_n
@move:
            cpx         cr_rx
            beq         @gap
            dex
            txa
            clc
            adc         n
            tay
            lda         cr_c,X
            sta         cr_c,Y
            lda         cr_a,X
            sta         cr_a,Y
            lda         cr_f,X
            sta         cr_f,Y
            bra         @move
@gap:
            ldx         cr_rx                               ; (The gap: blanks in the rendition at %=)
:
            cpx         m + 1
            bcs         :+
            lda         #' '
            sta         cr_c,X
            lda         cr_ga
            sta         cr_a,X
            lda         cr_gf
            sta         cr_f,X
            inx
            bra         :-
:
            lda         cr_w
            sta         cr_n
@fill:
            ldx         cr_n                                ; The rest blank, in the rendition at the end
            cpx         cr_w
            bcs         @done
            lda         #' '
            jsr         cr_put
            bra         @fill
@done:
            rts

; .A, the row's next cell, in the rendition now (none past its width).  Keeps .X, .Y
cr_put:
            phx
            ldx         cr_n
            cpx         cr_w
            bcs         :+
            sta         cr_c,X
            lda         cr_ca
            sta         cr_a,X
            lda         cr_cf
            sta         cr_f,X
            inc         cr_n
:
            plx
            rts

; The string at .A/.X (cr_strax) or m (cr_str), zero-ended, into the row.  Modifies .Y
cr_strax:
            sta         m
            stx         m + 1
cr_str:
            ldy         #0
:
            lda         (m),Y
            beq         :+
            jsr         cr_put
            iny
            bne         :-
:
            rts

; .A in decimal into the row (no leading zeros).  Modifies .A, .X, .Y, n + 1
cr_dec:
            stz         n + 1                               ; (A digit out: those after it go too)
            ldx         #2
@digit:
            ldy         #'0'
:
            cmp         cr_tens,X
            bcc         :+
            sbc         cr_tens,X
            iny
            bra         :-
:
            pha
            tya
            cpx         #0
            beq         @out
            cmp         #'0'
            bne         @out
            ldy         n + 1
            beq         @skip
@out:
            inc         n + 1
            jsr         cr_put
@skip:
            pla
            dex
            bpl         @digit
            rts

cr_num:                                                     ; %n: the window's number
            lda         w_in
            jmp         cr_dec

cr_grp:                                                     ; %g: its group's
            ldx         w_in
            lda         w_grp,X
            jmp         cr_dec

cr_y:                                                       ; %y: the scrollback's view's place (shown): the
            lda         w_in                                ;   lines shown, of them
            cmp         vv_w
            bne         @none
            lda         vv_top
            inc         a
            jsr         cr_dec
            lda         #'-'
            jsr         cr_put
            clc
            lda         vv_top
            adc         vv_r
            cmp         vv_n
            bcc         :+
            lda         vv_n
:
            jsr         cr_dec
            lda         #'/'
            jsr         cr_put
            lda         vv_n
            jmp         cr_dec
@none:
            rts

cr_lbl:                                                     ; %l: its label
            ldx         w_in
            jmp         cr_label

cr_prog:                                                    ; %p: its program
            ldx         w_in
            jmp         cr_progx

cr_stat:                                                    ; %s: its status line
            lda         w_in
            jsr         stat_at
            jmp         cr_str

cr_wins:                                                    ; %w: its group's windows
            stz         cr_k
            ldx         #0
@win:
            lda         w_used,X
            beq         @next
            ldy         w_in
            lda         w_grp,X
            cmp         w_grp,Y
            bne         @next
            lda         cr_k
            beq         :+
            lda         #' '
            jsr         cr_put
:
            inc         cr_k
            phx
            jsr         cr_entry
            plx
@next:
            inx
            cpx         #WIN_MAX
            bcc         @win
            rts

cr_groups:                                                  ; %G: the groups
            stz         cr_k
            ldx         #0
@grp:
            lda         g_used,X
            beq         @next
            lda         cr_k
            beq         :+
            lda         #' '
            jsr         cr_put
:
            inc         cr_k
            phx
            jsr         cr_gentry
            plx
@next:
            inx
            cpx         #WIN_MAX
            bcc         @grp
            rts

; Group .X's entry (%G): its number, ! or + (any of its windows'), a space, its focused window's label; the shown
; window's group reversed
cr_gentry:
            stx         cr_g
            lda         cr_cf
            pha
            ldy         w_in
            txa
            cmp         w_grp,Y
            bne         :+
            lda         cr_cf
            eor         #F_REV
            sta         cr_cf
:
            txa
            jsr         cr_dec
            stz         n                                   ; (Its windows' activity)
            ldx         #WIN_MAX - 1
@w:
            lda         w_used,X
            beq         @nx
            lda         w_grp,X
            cmp         cr_g
            bne         @nx
            lda         w_act,X
            ora         n
            sta         n
@nx:
            dex
            bpl         @w
            lda         n
            beq         @sp
            ldy         #'!'
            and         #ACT_BELL
            bne         :+
            ldy         #'+'
:
            tya
            jsr         cr_put
@sp:
            lda         #' '
            jsr         cr_put
            ldy         cr_g
            ldx         g_focus,Y
            jsr         cr_label
            pla
            sta         cr_cf
            rts

cr_cols:                                                    ; %c, %r: its size
            lda         v_cols
            jmp         cr_dec

cr_rows:
            lda         v_rows
            jmp         cr_dec

cr_modes:                                                   ; %m: raw or cooked, keys vt, held
            ldx         w_in
            lda         w_raw,X
            beq         :+
            lda         #<s_m_raw
            ldx         #>s_m_raw
            bra         :++
:
            lda         #<s_m_cooked
            ldx         #>s_m_cooked
:
            jsr         cr_strax
            ldx         w_in
            lda         kvt,X
            beq         :+
            lda         #<s_m_vt
            ldx         #>s_m_vt
            jsr         cr_strax
:
            ldx         w_in
            lda         w_kmod,X
            beq         :+
            lda         #<s_m_mods
            ldx         #>s_m_mods
            jsr         cr_strax
:
            ldx         w_in
            lda         w_hold,X
            beq         :+
            lda         #<s_m_held
            ldx         #>s_m_held
            jsr         cr_strax
:
            rts

cr_time:                                                    ; %t: the time (HH:MM)
            ldy         #11
            lda         #5
            bra         cr_tm

cr_date:                                                    ; %d: the date (YYYY-MM-DD)
            ldy         #0
            lda         #10
cr_tm:
            sta         n
            sty         n + 1
            jsr         tm_get
            lda         #1
            sta         clk_on
            jsr         nap_set
            ldy         n + 1
:
            lda         tm_buf,Y
            jsr         cr_put
            iny
            dec         n
            bne         :-
            rts

cr_leds:                                                    ; %L: the LEDs, 1-4 (. off)
            ldx         #0
:
            lda         v_leds
            and         cr_bit,X
            beq         @off
            txa
            clc
            adc         #'1'
            bra         @put
@off:
            lda         #'.'
@put:
            jsr         cr_put
            inx
            cpx         #4
            bcc         :-
            rts
cr_bit:     .byte       1, 2, 4, 8

cr_right:                                                   ; %=: the rest at the right
            lda         cr_n
            sta         cr_rx
            lda         cr_ca
            sta         cr_ga
            lda         cr_cf
            sta         cr_gf
            rts

cr_pct:                                                     ; %%
            lda         #'%'
            jmp         cr_put

cr_sgr:                                                     ; %[n;n...]: the rendition
            stz         n
@ch:
            ldy         cr_i
            lda         (p),Y
            beq         cr_sgr1                             ; (The format's end: the last one)
            inc         cr_i
            cmp         #']'
            beq         cr_sgr1
            cmp         #';'
            bne         :+
            jsr         cr_sgr1
            stz         n
            bra         @ch
:
            sec
            sbc         #'0'
            cmp         #10
            bcs         @ch
            pha
            lda         n                                   ; (* 10, + the digit: 255 at most)
            cmp         #26
            bcs         @big
            asl
            asl
            adc         n
            asl
            sta         n
            pla
            adc         n
            bcc         :+
            lda         #255
:
            sta         n
            bra         @ch
@big:
            pla
            lda         #255
            sta         n
            bra         @ch

cr_sgr1:                                                    ; SGR n's rendition
            lda         n
            bne         :+
            lda         #COL_DEF
            sta         cr_ca
            stz         cr_cf
            rts
:
            cmp         #10
            bcs         :+
            tax
            lda         sgr_on,X
            ora         cr_cf
            sta         cr_cf
            rts
:
            cmp         #20
            bcc         @done
            cmp         #30
            bcs         :+
            sbc         #20 - 1                             ; (C = 0: 20-29)
            tax
            lda         sgr_off,X
            and         cr_cf
            sta         cr_cf
            rts
:
            cmp         #38                                 ; 30-37: the foreground, 39 the default
            bcs         :+
            sbc         #30 - 1
            bra         @fg
:
            bne         :+
            rts                                             ; (38: not here)
:
            cmp         #39
            bne         :+
            lda         #COL_DEF & $0F
            bra         @fg
:
            cmp         #48                                 ; 40-47: the background, 49 none
            bcs         :+
            sbc         #40 - 1
            bra         @bg
:
            cmp         #49
            bne         :+
            lda         #0
            bra         @bg
:
            cmp         #90                                 ; 90-97, 100-107: bright
            bcc         @done
            cmp         #98
            bcs         :+
            sbc         #90 - 8 - 1
            bra         @fg
:
            cmp         #100
            bcc         @done
            cmp         #108
            bcs         @done
            sbc         #100 - 8 - 1
@bg:
            asl
            asl
            asl
            asl
            sta         n + 1
            lda         cr_ca
            and         #$0F
            ora         n + 1
            sta         cr_ca
@done:
            rts
@fg:
            sta         n + 1
            lda         cr_ca
            and         #$F0
            ora         n + 1
            sta         cr_ca
            rts

; Window .X's entry in a list (%w, %G): its number, ! or + (activity), a space, its label; the shown one reversed
cr_entry:
            lda         cr_cf
            pha
            cpx         w_in
            bne         :+
            eor         #F_REV
            sta         cr_cf
:
            phx
            txa
            jsr         cr_dec
            plx
            lda         w_act,X
            beq         @sp
            ldy         #'!'
            and         #ACT_BELL
            bne         :+
            ldy         #'+'
:
            tya
            jsr         cr_put
@sp:
            lda         #' '
            jsr         cr_put
            jsr         cr_label
            pla
            sta         cr_cf
            rts

; Window .X's label into the row: its title, or with none its program's name
cr_label:
            txa
            jsr         lbl_ptr
            lda         (m)
            beq         cr_progx
            jmp         cr_str
cr_progx:                                                   ; Window .X's program's name: its reader's (TASKINFO)
            lda         w_rdr,X
            beq         @none
            LDR         r0, ti_buf                          ; (LDR: .A too)
            lda         w_rdr,X
            dec         a
            jsr         TASKINFO
            bcs         @none
            lda         #<(ti_buf + TI_NAME)
            ldx         #>(ti_buf + TI_NAME)
            jmp         cr_strax
@none:
            rts

; m = window .A's label (lbl_buf: LBL_SIZE a window)
lbl_ptr:
            stz         m + 1
            asl
            asl
            asl
            asl
            asl
            rol         m + 1
            clc
            adc         #<lbl_buf
            sta         m
            lda         m + 1
            adc         #>lbl_buf
            sta         m + 1
            rts

; m = window .A's status line (w_stat: STAT_SIZE a window)
stat_at:
            lsr
            sta         m + 1
            lda         #0
            ror
            clc
            adc         #<w_stat
            sta         m
            lda         m + 1
            adc         #>w_stat
            sta         m + 1
            rts

; m = window .A's header's format (.Y = 0) or footer's (.Y = 1)
fmt_at:
            stz         m + 1                               ; (* FMT_SIZE: 64)
            asl
            asl
            asl
            asl
            asl
            rol         m + 1
            asl
            rol         m + 1
            cpy         #0
            bne         :+
            clc
            adc         #<w_hfmt
            sta         m
            lda         m + 1
            adc         #>w_hfmt
            sta         m + 1
            rts
:
            clc
            adc         #<w_ffmt
            sta         m
            lda         m + 1
            adc         #>w_ffmt
            sta         m + 1
            rts

; The format at .A/.X into the one at m (FMT_SIZE).  Modifies .A, .Y, p
fmt_copy:
            sta         p
            stx         p + 1
; ... the string at p (FMT_SIZE - 1 characters at most)
fmt_put:
            ldy         #0
:
            lda         (p),Y
            sta         (m),Y
            beq         :+
            iny
            cpy         #FMT_SIZE - 1
            bcc         :-
            lda         #0
            sta         (m),Y
:
            rts

; Z = 1 if the format at m is the string at .A/.X.  Modifies .A, .Y, p
fmt_same:
            sta         p
            stx         p + 1
            ldy         #0
:
            lda         (p),Y
            cmp         (m),Y
            bne         :+
            iny
            cmp         #0
            bne         :-
:
            rts

; tm_buf: the time ("2026-10-07 20:41:05"), read from #t/time once a drawing (chr_pass); clk_due, the next minute's
; tick.  Modifies .A, .X, .Y, r0-r2, m
tm_get:
            lda         chr_pass
            cmp         tm_pass
            bne         :+
            rts
:
            sta         tm_pass
            ldx         #TM_LEN - 1                         ; (Not read: ?s)
:
            lda         s_tm_none,X
            sta         tm_buf,X
            dex
            bpl         :-
            LDR         r0, s_time
            lda         #O_READ
            jsr         OPEN
            bcs         @due
            sta         tm_fd
            LDR         r0, tm_buf
            LDR         r1, TM_LEN
            lda         tm_fd
            jsr         READ
            lda         tm_fd
            jsr         CLOSE
@due:
            lda         tm_buf + 17                         ; The next minute: (60 - its seconds) seconds on
            and         #$0F
            asl
            sta         m
            asl
            asl
            adc         m
            sta         m
            lda         tm_buf + 18
            and         #$0F
            adc         m
            cmp         #60
            bcc         :+
            lda         #59
:
            eor         #$FF
            sec
            adc         #60                                 ; (60 - it: 1-60)
            tax
            stz         m                                   ; (* TICK_HZ)
            stz         m + 1
:
            clc
            lda         m
            adc         #<TICK_HZ
            sta         m
            lda         m + 1
            adc         #>TICK_HZ
            sta         m + 1
            dex
            bne         :-
            jsr         TICKS
            clc
            adc         m
            sta         clk_due
            txa
            adc         m + 1
            sta         clk_due + 1
@done:
            rts

; The chrome's time: drawn again once its minute's past (clk_due); timer 2's rounds bring the readers back meanwhile
; (its naps: t2_next).  Modifies .A, .X, .Y
clk_check:
            jsr         TICKS                               ; Now - the minute's tick: not negative once it's come
            sec
            sbc         clk_due
            txa
            sbc         clk_due + 1
            bmi         :+
            lda         #3
            tsb         chr_dirty
            jsr         TICKS                               ; (Looked at again in a second, if not drawn by then)
            clc
            adc         #<TICK_HZ
            sta         clk_due
            txa
            adc         #>TICK_HZ
            sta         clk_due + 1
:
            php                                             ; Timer 2 napping, if it's idle
            sei
            lda         tx_busy
            ora         t2_nap
            bne         @done
            lda         #CLK_NAP
            sta         t2_left
            lda         #$FF
            sta         t2_hi
            sta         t2_nap
            sta         VIA_T2CL
            sta         VIA_T2CH                            ; (It starts)
@done:
            plp
            rts

; ****************************************************************************
; The chrome's commands (wctl) and /label

; p = word .A of the ctl line (0: the first after the command), the line's rest after it as it was written (the words'
; ends spaces again; its end's blanks off).  Modifies .A, .Y, m
ctl_rest:
            asl
            tay
            lda         srv_argp,Y
            sta         p
            lda         srv_argp + 1,Y
            sta         p + 1
            sec                                             ; (Its bytes: to the write's end)
            lda         p
            sbc         #<srv_ctl
            sta         m
            sec
            lda         TASK_INBOX + RQ_COUNT
            sbc         m
            sta         m
            ldy         #0
@sp:
            cpy         m
            bcs         @end
            lda         (p),Y
            bne         :+
            lda         #' '
            sta         (p),Y
:
            iny
            bra         @sp
@end:
            lda         #0
            sta         (p),Y
:
            dey
            bmi         :+
            lda         (p),Y
            cmp         #' ' + 1
            bcs         :+
            lda         #0
            sta         (p),Y
            bra         :-
:
            rts

; The chrome's place changed (the bar's, a window's rows): the windows' sizes, both terminals painted
chr_changed:
            jsr         relayout
            lda         #1
            sta         ts_ser
            sta         ts_scr
            lda         #3
            tsb         chr_dirty
            inc         TASK_EVENT
            clc
            rts

chr_inval:
            lda         #E_INVAL
            sec
            rts

; bar top, bar bottom, bar off: where the bar is (on each terminal its window's chrome has it); bar FORMAT: what
; it shows
c_bar:
            lda         z:srv_argn
            beq         chr_inval
            lda         #0
            jsr         ctl_rest
            lda         #<s_top_w
            ldx         #>s_top_w
            jsr         word_is
            bne         :+
            lda         #BAR_TOP
            bra         @pos
:
            lda         #<s_bottom_w
            ldx         #>s_bottom_w
            jsr         word_is
            bne         :+
            lda         #BAR_BOTTOM
            bra         @pos
:
            lda         #<s_off_w
            ldx         #>s_off_w
            jsr         word_is
            bne         @fmt
            lda         #0
@pos:
            cmp         bar_pos
            beq         :+
            sta         bar_pos
            jmp         chr_changed
:
            clc
            rts
@fmt:
            lda         #<bar_fmt
            sta         m
            lda         #>bar_fmt
            sta         m + 1
            jsr         fmt_put
            lda         #3
            tsb         chr_dirty
            clc
            rts

; header FORMAT, footer FORMAT: the window's; header on, header off (footer ...): its row on both terminals, or none
c_header:
            ldy         #0
            bra         c_hf
c_footer:
            ldy         #1
c_hf:
            sty         hf_k
            lda         z:srv_argn
            beq         chr_inval
            lda         #0
            jsr         ctl_rest
            ldx         z:srv_id
            lda         w_chr,X
            sta         chr_v
            ldy         #(CH_HEAD << 4) | CH_HEAD           ; (Its bits: both terminals')
            lda         hf_k
            beq         :+
            ldy         #(CH_FOOT << 4) | CH_FOOT
:
            sty         chr_p
            lda         #<s_on_w
            ldx         #>s_on_w
            jsr         word_is
            bne         :+
            lda         chr_v
            ora         chr_p
            bra         @rows
:
            lda         #<s_off_w
            ldx         #>s_off_w
            jsr         word_is
            bne         @fmt
            lda         chr_p
            eor         #$FF
            and         chr_v
@rows:
            ldx         z:srv_id
            cmp         w_chr,X
            beq         :+
            sta         w_chr,X
            jmp         chr_changed
:
            clc
            rts
@fmt:
            lda         z:srv_id
            ldy         hf_k
            jsr         fmt_at
            jsr         fmt_put
            lda         #3
            tsb         chr_dirty
            clc
            rts

; chrome screen|serial|both on|off [bar] [header] [footer]: the window's chrome rows on that terminal (all three, with
; none named)
c_chrome:
            ldx         z:srv_id
            lda         w_chr,X
            sta         chr_v
            lda         #0
            jsr         chr_parse
            bcc         :+
            jmp         chr_inval
:
            ldx         z:srv_id
            lda         chr_v
            cmp         w_chr,X
            beq         :+
            sta         w_chr,X
            jmp         chr_changed
:
            clc
            rts

; chr_v changed by the words from .A on: a terminal (screen, serial, both), on or off, the parts (none: all).  OUT:
; C = 1, they're not that
chr_parse:
            sta         hf_k                                ; (The first word's number)
            clc
            adc         #2
            cmp         z:srv_argn
            beq         :+
            bcs         @bad
:
            lda         hf_k
            asl
            tay
            lda         srv_argp,Y
            sta         p
            lda         srv_argp + 1,Y
            sta         p + 1
            ldx         #2                                  ; Its terminal
:
            phx
            txa
            asl
            tay
            lda         term_names,Y
            pha
            lda         term_names + 1,Y
            tax
            pla
            jsr         word_is
            beq         @term                               ; (.X on the stack)
            plx
            dex
            bpl         :-
@bad:
            sec
            rts
@term:
            plx
            lda         term_bits,X
            sta         chr_sep                             ; (Its bits)
            lda         hf_k                                ; On or off
            inc         a
            asl
            tay
            lda         srv_argp,Y
            sta         p
            lda         srv_argp + 1,Y
            sta         p + 1
            lda         #<s_on_w
            ldx         #>s_on_w
            jsr         word_is
            php
            beq         :+
            lda         #<s_off_w
            ldx         #>s_off_w
            jsr         word_is
            beq         :+
            plp
            sec
            rts
:
            lda         hf_k                                ; The parts, if it names any
            clc
            adc         #2
            jsr         chr_parts
            bcc         :+
            plp
            sec
            rts
:
            lda         chr_p                               ; Those bits (both terminals' at first), the terminal's
            asl
            asl
            asl
            asl
            ora         chr_p
            and         chr_sep
            plp
            bne         :+
            ora         chr_v                               ; (on)
            bra         :++
:
            eor         #$FF                                ; (off)
            and         chr_v
:
            sta         chr_v
            clc
            rts

; chr_p: the parts named from word .A on (bar, header, footer), CH_* bits (none named: all three).  OUT: C = 1, a
; word that isn't one
chr_parts:
            stz         chr_p
            cmp         z:srv_argn
            bcs         @end
            jsr         ctl_rest                            ; (p: the words, spaces between)
@word:
            lda         (p)
            beq         @end
            cmp         #' '
            bne         :+
            inc         p
            bne         @word
            inc         p + 1
            bra         @word
:
            ldy         #0                                  ; The word: its end a 0, for word_is
:
            lda         (p),Y
            beq         :+
            cmp         #' '
            beq         :+
            iny
            bra         :-
:
            sty         chr_wl
            pha
            lda         #0
            sta         (p),Y
            ldx         #2
:
            phx
            txa
            asl
            tay
            lda         part_names,Y
            pha
            lda         part_names + 1,Y
            tax
            pla
            jsr         word_is
            beq         @part                               ; (.X on the stack)
            plx
            dex
            bpl         :-
            pla
            sec
            rts
@part:
            plx
            lda         part_bits,X
            ora         chr_p
            sta         chr_p
            ldy         chr_wl                              ; (Its end as it was; on past it)
            pla
            sta         (p),Y
            tya
            clc
            adc         p
            sta         p
            bcc         @word
            inc         p + 1
            bra         @word
@end:
            lda         chr_p
            bne         :+
            lda         #CH_ALL
            sta         chr_p
:
            clc
            rts
part_names: .word       s_bar_w, s_header_w, s_footer_w
part_bits:  .byte       CH_BAR, CH_HEAD, CH_FOOT

; default header FORMAT, default footer FORMAT, default chrome screen|serial|both on|off [parts]: what a new window
; takes; the windows still as the defaults were take them too
c_default:
            lda         z:srv_argn
            cmp         #2
            bcs         :+
            jmp         chr_inval
:
            lda         srv_argp
            sta         p
            lda         srv_argp + 1
            sta         p + 1
            lda         #<s_chrome_w
            ldx         #>s_chrome_w
            jsr         word_is
            beq         @chrome
            lda         #<s_header_w
            ldx         #>s_header_w
            jsr         word_is
            beq         @head
            lda         #<s_footer_w
            ldx         #>s_footer_w
            jsr         word_is
            beq         @foot
            jmp         chr_inval
@head:                                                      ; (word_is changes .Y: 0 or 1 after it)
            ldy         #0
            bra         :+
@foot:
            ldy         #1
:
            sty         hf_k
            lda         #1                                  ; (The format: kept in n)
            jsr         ctl_rest
            lda         p
            sta         n
            lda         p + 1
            sta         n + 1
            ldx         #WIN_MAX - 1                        ; The windows with the default: the new one
@win:
            lda         w_used,X
            beq         @next
            phx
            txa
            ldy         hf_k
            jsr         fmt_at
            jsr         @def
            jsr         fmt_same
            bne         :+
            jsr         @new
            jsr         fmt_put
:
            plx
@next:
            dex
            bpl         @win
            jsr         @defm                               ; Then the default
            jsr         @new
            jsr         fmt_put
            lda         #3
            tsb         chr_dirty
            clc
            rts

@def:                                                       ; (.A/.X: the default format)
            lda         hf_k
            bne         :+
            lda         #<def_hfmt
            ldx         #>def_hfmt
            rts
:
            lda         #<def_ffmt
            ldx         #>def_ffmt
            rts
@defm:                                                      ; (m: it)
            jsr         @def
            sta         m
            stx         m + 1
            rts
@new:                                                       ; (p: the new one)
            lda         n
            sta         p
            lda         n + 1
            sta         p + 1
            rts

@chrome:
            lda         def_chr
            sta         chr_v
            lda         #1
            jsr         chr_parse
            bcc         :+
            jmp         chr_inval
:
            ldx         #WIN_MAX - 1                        ; (The windows with the default: the new one)
:
            lda         w_used,X
            beq         :+
            lda         w_chr,X
            cmp         def_chr
            bne         :+
            lda         chr_v
            sta         w_chr,X
:
            dex
            bpl         :--
            lda         chr_v
            cmp         def_chr
            beq         :+
            sta         def_chr
            jmp         chr_changed
:
            clc
            rts

; status TEXT: the window's status line (the footer's %s; DECSASD writes it too); status alone, none
c_status:
            lda         z:srv_id
            jsr         stat_at
            lda         #0
            sta         (m)
            lda         z:srv_argn
            beq         @done
            lda         z:srv_id                            ; (ctl_rest uses m)
            pha
            lda         #0
            jsr         ctl_rest
            pla
            jsr         stat_at
            ldy         #0
:
            lda         (p),Y
            sta         (m),Y
            beq         @done
            iny
            cpy         #STAT_SIZE - 1
            bcc         :-
            lda         #0
            sta         (m),Y
@done:
            lda         #3
            tsb         chr_dirty
            clc
            rts

; monitor on, monitor off: the window's output marked in the chrome (+) while it isn't shown
c_monitor:
            lda         z:srv_argn
            beq         @inval
            lda         srv_argp
            sta         p
            lda         srv_argp + 1
            sta         p + 1
            lda         #<s_on_w
            ldx         #>s_on_w
            jsr         word_is
            beq         @on
            lda         #<s_off_w
            ldx         #>s_off_w
            jsr         word_is
            bne         @inval
            ldy         #0                                  ; (word_is changes .Y: 1 or 0 after it)
            bra         :+
@on:
            ldy         #1
:
            ldx         z:srv_id
            tya
            sta         w_mon,X
            clc
            rts
@inval:
            jmp         chr_inval

; /label: the window's title (OSC 0 and 2 write it too), read and written whole: LBL_SIZE - 1 characters at most, the
; write's line end off; written empty (a line end alone), it's automatic again (its program's name)
h_label:
            cmp         #R_READ
            beq         @read
            cmp         #R_WRITE
            beq         @write
            clc
            rts
@read:
            lda         srv_fid_aux,X
            jsr         lbl_ptr
            ldy         #0                                  ; (Its length)
:
            lda         (m),Y
            beq         :+
            iny
            bra         :-
:
            sty         n
            stz         TASK_INBOX + RQ_DONE
            stz         TASK_INBOX + RQ_DONE + 1
            lda         TASK_INBOX + RQ_OFFSET + 1          ; (Past its end: nothing)
            ora         TASK_INBOX + RQ_OFFSET + 2
            ora         TASK_INBOX + RQ_OFFSET + 3
            bne         @none
            lda         TASK_INBOX + RQ_OFFSET
            cmp         n
            bcs         @none
            sta         n + 1
            sec
            lda         n
            sbc         n + 1
            ldx         TASK_INBOX + RQ_COUNT + 1
            bne         :+
            cmp         TASK_INBOX + RQ_COUNT
            bcc         :+
            lda         TASK_INBOX + RQ_COUNT
:
            sta         r2
            sta         TASK_INBOX + RQ_DONE
            stz         r2 + 1
            clc
            lda         m
            adc         n + 1
            sta         r0
            lda         m + 1
            adc         #0
            sta         r0 + 1
            MOVR        r1, TASK_INBOX + RQ_BUF
            jsr         CLIENT_WRITE
@none:
            clc
            rts
@write:
            lda         srv_fid_aux,X
            pha
            lda         TASK_INBOX + RQ_COUNT               ; (LBL_SIZE - 1 at most)
            ldx         TASK_INBOX + RQ_COUNT + 1
            bne         :+
            cmp         #LBL_SIZE
            bcc         :++
:
            lda         #LBL_SIZE - 1
:
            sta         cnt
            stz         n
            stz         n + 1
            jsr         from_client                         ; (iobuf)
            pla
            bcs         @done
            jsr         lbl_ptr
            ldy         cnt                                 ; (Its line end, and blanks, off)
:
            dey
            bmi         :+
            lda         iobuf,Y
            cmp         #' ' + 1
            bcc         :-
:
            iny
            lda         #0
            sta         (m),Y
:
            dey
            bmi         :+
            lda         iobuf,Y
            sta         (m),Y
            bra         :-
:
            lda         TASK_INBOX + RQ_COUNT               ; (All of it taken)
            sta         TASK_INBOX + RQ_DONE
            lda         TASK_INBOX + RQ_COUNT + 1
            sta         TASK_INBOX + RQ_DONE + 1
            lda         #3
            tsb         chr_dirty
            clc
@done:
            rts

; ****************************************************************************
; consctl, wctl and serctl

; rawon, rawoff: the window's
c_rawon:
            lda         z:srv_id
            jsr         load
            lda         #1
            sta         raw
            ldx         z:srv_id                            ; (A resize, the focus moved, before it: not news to
            stz         w_rsz,X                             ;   its reader)
            stz         w_kf,X
            jmp         raw_mark

c_rawoff:
            lda         z:srv_id
            jsr         load
            stz         raw
            ldx         z:srv_id
            lda         #0
; w_raw: window .X's raw mode .A, for its chrome's %m (drawn again if it changed).  OUT: C = 0
raw_mark:
            cmp         w_raw,X
            beq         :+
            sta         w_raw,X
            lda         #3
            tsb         chr_dirty
:
            clc
            rts


; keys vt, keys hydra, keys mods: a raw read's keys as a VT100 sends them, or one code each (KEY_*), or one code each
; and KEY_MOD and its modifiers before a modified one
c_keys:
            lda         z:srv_argn
            beq         @inval
            lda         srv_argp
            sta         p
            lda         srv_argp + 1
            sta         p + 1
            lda         #<s_vt_w
            ldx         #>s_vt_w
            jsr         word_is
            bne         :+
            ldy         #1
            bra         @set
:
            lda         #<s_mods_w
            ldx         #>s_mods_w
            jsr         word_is
            bne         :+
            ldy         #2
            bra         @set
:
            lda         #<s_hydra_w
            ldx         #>s_hydra_w
            jsr         word_is
            bne         @inval
            ldy         #0
@set:
            ldx         z:srv_id
            lda         #0                                  ; (mods: hydra's, KEY_MOD too)
            cpy         #2
            bne         :+
            ldy         #0
            lda         #1
:
            cmp         w_kmod,X
            beq         :+
            sta         w_kmod,X
            lda         #3                                  ; (Its modes in the chrome)
            tsb         chr_dirty
:
            tya
            cmp         kvt,X
            beq         :+
            sta         kvt,X
            lda         #3                                  ; (Its modes in the chrome)
            tsb         chr_dirty
:
            stz         kp_n,X
            stz         kp_i,X
            clc
            rts
@inval:
            lda         #E_INVAL
            sec
            rts

; key KEY ACTION, key ctrl-tab ACTION, key ctrl-shift-tab ACTION, key prefix KEY (wctl's; console-wide): the keys'
; bindings.  KEY: a character (not a digit), ^X (a control), tab, shift-tab.  ACTION: ka_names's
c_key:
            lda         z:srv_argn
            cmp         #2
            beq         @two
            jmp         @inval
@two:
            lda         srv_argp                            ; The first word
            sta         p
            lda         srv_argp + 1
            sta         p + 1
            lda         #<s_prefix_w
            ldx         #>s_prefix_w
            jsr         word_is
            beq         @prefix
            lda         #<s_ctab_w
            ldx         #>s_ctab_w
            jsr         word_is
            bne         :+
            ldy         #0
            bra         @ct
:
            lda         #<s_cstab_w
            ldx         #>s_cstab_w
            jsr         word_is
            bne         :+
            ldy         #1
            bra         @ct
:
            lda         #<s_spgup_w
            ldx         #>s_spgup_w
            jsr         word_is
            bne         @key
            ldy         #2
@ct:
            phy
            jsr         @action
            ply
            bcs         @inval
            sta         kb_ct,Y
            clc
            rts
@key:
            jsr         key_spec
            bcs         @inval
            cmp         #'0'
            bcc         :+
            cmp         #'9' + 1
            bcc         @inval                              ; (A digit: its window, always)
:
            pha
            jsr         @action
            plx
            bcs         @inval
            sta         kb_act,X
            clc
            rts
@prefix:
            jsr         @second
            jsr         key_spec
            bcs         @inval
            cmp         #1                                  ; (A control, but these)
            bcc         @inval
            cmp         #' '
            bcs         @inval
            cmp         #CTRL_C
            beq         @inval
            cmp         #CTRL_BSL
            beq         @inval
            cmp         #ESC
            beq         @inval
            cmp         #CR
            beq         @inval
            cmp         #LF
            beq         @inval
            sta         kb_pfx
            clc
            rts
@inval:
            lda         #E_INVAL
            sec
            rts
@second:                                                    ; (p: the second word)
            lda         srv_argp + 2
            sta         p
            lda         srv_argp + 3
            sta         p + 1
            rts
@action:                                                    ; (.A: the second word's action; C = 1: none)
            jsr         @second
            ldx         #KA_N - 1
@name:
            phx
            txa
            asl
            tay
            lda         ka_names,Y
            pha
            lda         ka_names + 1,Y
            tax
            pla
            jsr         word_is
            beq         :+
            plx
            dex
            bpl         @name
            sec
            rts
:
            pla
            clc
            rts

; The key the word at p names: a character, ^X or ctrl-X (a control: rc's ^ joins words), tab, shift-tab (after
; the prefix, ESC [ Z: ESC's binding).  OUT: C = 0, .A = it; or C = 1
key_spec:
            lda         #<s_tab_w
            ldx         #>s_tab_w
            jsr         word_is
            bne         :+
            lda         #HT
            clc
            rts
:
            lda         #<s_stab_w
            ldx         #>s_stab_w
            jsr         word_is
            bne         :+
            lda         #ESC
            clc
            rts
:
            ldy         #1
            lda         (p),Y
            beq         @char
            lda         (p)
            cmp         #'^'
            beq         @ctrl
            ldy         #0                                  ; (ctrl-X)
:
            lda         (p),Y
            cmp         s_ctrl_w,Y
            bne         @bad
            iny
            cpy         #5
            bcc         :-
@ctrl:
            iny
            lda         (p),Y
            bne         @bad
            dey
            lda         (p),Y
            and         #$1F
            clc
            rts
@char:
            lda         (p)
            cmp         #$80
            bcs         @bad
            clc
            rts
@bad:
            sec
            rts

; layout tabs|rows|columns|grid: the writer's window's group's layout (its windows one at a time, or tiled), not zoomed
c_layout:
            lda         z:srv_argn
            cmp         #1
            bne         @inval
            lda         srv_argp
            sta         p
            lda         srv_argp + 1
            sta         p + 1
            ldx         #LAY_N - 1
@name:
            phx
            txa
            asl
            tay
            lda         lay_names,Y
            pha
            lda         lay_names + 1,Y
            tax
            pla
            jsr         word_is
            beq         :+
            plx
            dex
            bpl         @name
@inval:
            lda         #E_INVAL
            sec
            rts
:
            pla
            ldy         z:srv_id
            ldx         w_grp,Y
            sta         g_lay,X
            stz         g_zoom,X
            jsr         re_tile
            clc
            rts

; history N: the window's history, N rows past its scrollback's (64 at a time: N rounded up; 128 at most; 0 none),
; empty (vt.s's vt_history)
c_history:
            lda         z:srv_argn
            cmp         #1
            bne         @inval
            lda         srv_arg + 1
            bne         @inval
            lda         srv_arg
            cmp         #128 + 1
            bcs         @inval
            adc         #63                                 ; (Its sets of 64)
            lsr
            lsr
            lsr
            lsr
            lsr
            lsr
            ldx         z:srv_id
            FAR2        vt_history
            rts
@inval:
            lda         #E_INVAL
            sec
            rts

; Is the word at p the string at .A/.X?  OUT: Z = 1 yes.  Modifies .A, .Y, m
word_is:
            sta         m
            stx         m + 1
            ldy         #0
:
            lda         (m),Y
            cmp         (p),Y
            bne         @done
            iny
            cmp         #0
            bne         :-
@done:
            rts

; scroll smooth, scroll jump: the window shown's every byte to the serial port, its writers waiting for the line; or
; its writers going on, the serial port's terminal painted as it can (vt.s)
c_scroll:
            lda         z:srv_argn
            beq         @inval
            lda         srv_argp
            sta         p
            lda         srv_argp + 1
            sta         p + 1
            lda         #<s_jump_w
            ldx         #>s_jump_w
            jsr         word_is
            bne         :+
            ldy         #1
            bra         @set
:
            lda         #<s_smooth_w
            ldx         #>s_smooth_w
            jsr         word_is
            bne         @inval
            ldy         #0
@set:
            ldx         z:srv_id
            tya
            sta         w_jump,X
            inc         TASK_EVENT                          ; (Its writers look again)
            clc
            rts
@inval:
            lda         #E_INVAL
            sec
            rts

; group: the window's notes (Ctrl-C, Ctrl-\) go to the writer's note group
c_group:
            ldx         z:srv_id
            lda         TASK_INBOX + RQ_GROUP
            sta         w_group,X
            cpx         w_in
            bne         :+
            sta         win_grp
:
            clc
            rts

; consctl's state: "rawon" or "rawoff", "keys ...", "scroll ...", "group N", "window N", "size C R", "terminal ..."
gen_consctl:
            lda         z:srv_id
            jsr         load
            lda         #<s_rawon
            ldx         #>s_rawon
            ldy         raw
            bne         :+
            lda         #<s_rawoff
            ldx         #>s_rawoff
:
            jsr         srv_tputs
            ldy         z:srv_id                            ; keys hydra, keys vt, keys mods
            lda         kvt,Y
            beq         :+
            lda         #<s_keys_vt
            ldx         #>s_keys_vt
            bra         :+++
:
            lda         w_kmod,Y
            beq         :+
            lda         #<s_keys_mods
            ldx         #>s_keys_mods
            bra         :++
:
            lda         #<s_keys_hydra
            ldx         #>s_keys_hydra
:
            jsr         srv_tputs
            ldy         z:srv_id                            ; scroll smooth, scroll jump
            lda         w_jump,Y
            beq         :+
            lda         #<s_scroll_jump
            ldx         #>s_scroll_jump
            bra         :++
:
            lda         #<s_scroll_smooth
            ldx         #>s_scroll_smooth
:
            jsr         srv_tputs
            lda         #<s_group
            ldx         #>s_group
            jsr         srv_tputs
            ldx         z:srv_id
            lda         w_group,X
            ldx         #0
            jsr         srv_tputdec
            lda         #<s_window
            ldx         #>s_window
            jsr         srv_tputs
            lda         z:srv_id
            ldx         #0
            jsr         srv_tputdec
            lda         #<s_size                            ; Its size: columns, rows
            ldx         #>s_size
            jsr         srv_tputs
            ldx         z:srv_id
            FAR2        vt_size                             ; (.A: its columns, .X: its rows)
            phx
            ldx         #0
            jsr         srv_tputdec
            lda         #' '
            jsr         srv_tputc
            pla
            ldx         #0
            jsr         srv_tputdec
            lda         #<s_terminal                        ; The terminals the windows are shown on (the screen
            ldx         #>s_terminal                        ;   only if there's one)
            jsr         srv_tputs
            lda         term
            and         #TERM_SCREEN
            beq         :+
            jsr         scr_open
            lda         term
            bcc         :++
:
            lda         #TERM_SERIAL
:
            asl
            tax
            lda         term_names - 2,X
            pha
            lda         term_names - 1,X
            tax
            pla
            jsr         srv_tputs
            lda         #LF
            jsr         srv_tputc
            clc
            rts

; screen, serial, both: where the windows are shown (all of them); a terminal turned on is repainted.  screen with
; no screen: E_NODEV (both: the serial port alone, then)
c_screen:
            lda         #TERM_SCREEN
            bra         c_term

c_serial:
            lda         #TERM_SERIAL
            bra         c_term

c_both:
            lda         #TERM_SERIAL | TERM_SCREEN
c_term:
            sta         m
            and         #TERM_SCREEN
            beq         @set
            jsr         scr_open
            bcc         @set
            lda         m
            and         #TERM_SERIAL
            bne         @set
            lda         #E_NODEV
            sec
            rts

@set:
            lda         term                                ; Those turned on: repainted
            eor         #$FF
            and         m
            lsr
            bcc         :+
            ldy         #1
            sty         ts_ser
:
            lsr
            bcc         :+
            ldy         #1
            sty         ts_scr
:
            lda         m
            sta         term
            jsr         relayout                            ; (The windows' size: of those on)
            inc         TASK_EVENT                          ; (A writer waiting for the line's room looks again)
            clc
            rts

; terminal screen, serial, both: as screen, serial, both.  terminal size C R: the serial port's terminal's size;
; terminal size: asked of it (ESC [ 18 t: its answer, ESC [ 8 ; R ; C t, sets it)
c_terminal:
            lda         z:srv_argn
            beq         @inval
            lda         srv_argp
            sta         p
            lda         srv_argp + 1
            sta         p + 1
            lda         #<s_size_w
            ldx         #>s_size_w
            jsr         word_is
            beq         @size
            lda         #<s_screen_w
            ldx         #>s_screen_w
            jsr         word_is
            bne         :+
            jmp         c_screen
:
            lda         #<s_serial_w
            ldx         #>s_serial_w
            jsr         word_is
            bne         :+
            jmp         c_serial
:
            lda         #<s_both_w
            ldx         #>s_both_w
            jsr         word_is
            bne         @inval
            jmp         c_both
@size:
            lda         z:srv_argn
            cmp         #1
            beq         @ask
            cmp         #3
            bne         @inval
            lda         srv_arg + 3                         ; Its columns, its rows
            ora         srv_arg + 5
            bne         @inval
            ldx         srv_arg + 2
            lda         srv_arg + 4
            jsr         ser_size
            bcs         @inval
            rts
@ask:
            lda         ser_rd                              ; Asked (not while the line's /ser's)
            bne         @busy
            jsr         tx_free
            cmp         #8
            bcs         :+
            lda         #E_AGAIN
            sec
            rts
:
            ldx         #0
:
            lda         s_ask,X
            beq         :+
            jsr         tx_put
            inx
            bra         :-
:
            clc
            rts
@busy:
            lda         #E_BUSY
            sec
            rts
@inval:
            lda         #E_INVAL
            sec
            rts

; wctl: new (a window in the writer's window's group; new group: in one of its own; a read of this fid then gives
; its number), current N (window N shown)
c_new:
            lda         z:srv_argn
            beq         @join
            lda         srv_argp
            sta         p
            lda         srv_argp + 1
            sta         p + 1
            lda         #<s_group_w
            ldx         #>s_group_w
            jsr         word_is
            beq         :+
            lda         #E_INVAL
            sec
            rts
:
            lda         #$FF
            bra         @make
@join:
            ldx         z:srv_id
            lda         w_grp,X
@make:
            jsr         w_make
            bcs         @done
            txa
            ldx         TASK_INBOX + RQ_FID
            sta         fid_new,X
            lda         #3                                  ; (The chrome's lists)
            tsb         chr_dirty
            clc
@done:
            rts

c_current:
            lda         z:srv_argn
            beq         @inval
            lda         srv_arg + 1
            bne         @inval
            ldx         srv_arg
            cpx         #WIN_MAX
            bcs         @inval
            lda         w_used,X
            beq         @inval
            jsr         w_show
            clc
            rts

@inval:
            lda         #E_INVAL
            sec
            rts

; wctl's state: the windows, a line each: "N G C R" (its number, its group, its columns and rows), and " *" for the
; one shown.  After a new on this fid: the window it made ("N"), the once
gen_wctl:
            ldx         TASK_INBOX + RQ_FID
            lda         fid_new,X
            bmi         @list
            pha
            lda         #$FF
            sta         fid_new,X
            pla
            ldx         #0
            jsr         srv_tputdec
            lda         #LF
            jsr         srv_tputc
            clc
            rts
@list:
            stz         cnt
@window:
            ldx         cnt
            lda         w_used,X
            beq         @next
            txa
            ldx         #0
            jsr         srv_tputdec
            lda         #' '
            jsr         srv_tputc
            ldx         cnt
            lda         w_grp,X
            ldx         #0
            jsr         srv_tputdec
            lda         #' '
            jsr         srv_tputc
            ldx         cnt
            FAR2        vt_size                             ; (.A: its columns, .X: its rows)
            phx
            ldx         #0
            jsr         srv_tputdec
            lda         #' '
            jsr         srv_tputc
            pla
            ldx         #0
            jsr         srv_tputdec
            ldx         cnt
            cpx         w_in
            bne         :+
            lda         #<s_shown
            ldx         #>s_shown
            jsr         srv_tputs
:
            lda         #LF
            jsr         srv_tputc
@next:
            inc         cnt
            lda         cnt
            cmp         #WIN_MAX
            bcc         @window
            clc
            rts

; serctl's rates (one command each)
c_b300:     ldx         #0
            bra         c_rate
c_b600:     ldx         #1
            bra         c_rate
c_b1200:    ldx         #2
            bra         c_rate
c_b2400:    ldx         #3
            bra         c_rate
c_b4800:    ldx         #4
            bra         c_rate
c_b9600:    ldx         #5
            bra         c_rate
c_b19200:   ldx         #6
            bra         c_rate
c_b115200:  ldx         #7
c_rate:                                                     ; Rate .X, once nothing's going (E_AGAIN till then)
            lda         tx_busy
            beq         :+
            jmp         again
:
            stx         rate
            lda         #ACIA_CTRL_BRG | ACIA_CTRL_8N1
            ora         rate_code,X
            sta         ACIA_CTRL
rate_t2:                                                    ; Timer 2 for rate .X's characters
            stx         rate
            lda         rate_lo,X
            sta         t2_lo
            lda         rate_hi,X
            sta         t2_hi
            lda         rate_rounds,X
            sta         t2_rounds
            clc
            rts

; serctl's state: "bN"
gen_serctl:
            lda         #'b'
            jsr         srv_tputc
            ldx         rate
            lda         rate_name_lo,X
            pha
            lda         rate_name_hi,X
            tax
            pla
            jsr         srv_tputs
            lda         #LF
            jsr         srv_tputc
            clc
            rts

; ****************************************************************************
; /pc (#P)

; A request for #P (srvlib's: its tree is entry 0 alone, SK_RAW).  IN: .A = the request.  It goes to the PC as a
; frame, and the client waits (E_AGAIN) for the frame back; asking again, it's answered from that.  One request out
; at a time: another client's waits till it's done (or given up: pump).  R_FLUSH: the client gave its request up
; (a note)
pc_serve:
            ldx         TASK_INBOX + RQ_CLIENT
            cmp         #R_FLUSH
            bne         @ask
            cpx         pc_owner
            bne         :+
            jsr         pc_release
:
            clc
            rts

@ask:
            lda         pc_owner
            bmi         @take                               ; (None out)
            cpx         pc_owner
            beq         @mine
            jmp         again                               ; Another's: wait till it's done

@take:
            stx         pc_owner
            stz         pc_step
            stz         pc_rxf
@mine:
            lda         pc_step
            cmp         #PS_REQ
            bne         :+
            jsr         pc_same                             ; Its request out, or another (it gave that one up:
            bcs         :+                                  ;   a non-blocking one)?
            stz         pc_step                             ; Another: this one, from the start
            stz         pc_rxf
:
            lda         pc_step
            bne         pc_waiting
            lda         pc_online
            bne         pc_request
            lda         #PS_ATTACH                          ; Not attached: the attach first, one try
            sta         pc_step
            lda         #1
            sta         pc_tries
            inc         pc_tag
            lda         #PC_T_ATTACH
            sta         pc_txbuf
            lda         #PC_VERSION
            sta         pc_txbuf + 4
            lda         #1
            jsr         pc_frame
            lda         #<PC_WAIT_ATTACH
            ldx         #>PC_WAIT_ATTACH
            bra         pc_sent

; The request: out as a frame, with a new tag
pc_request:
            lda         #PS_REQ
            sta         pc_step
            lda         #PC_TRIES
            sta         pc_tries
            inc         pc_tag
            jsr         pc_build

pc_resent:
            lda         #<PC_WAIT_REQ
            ldx         #>PC_WAIT_REQ

pc_sent:                                                    ; Its reply's due .A/.X ticks from now: the client
            jsr         pc_from_now                         ;   waits for it
            jmp         again

; The frame's out: its reply, or its time; or a frame back that stops part-way (a byte lost: an overrun at 115200)
pc_waiting:
            lda         pc_rxf
            bne         @reply
            lda         pc_rxs                              ; A frame coming in?
            beq         @time
            lda         pc_rxi
            cmp         pc_rxn
            beq         @stopped
            sta         pc_rxn                              ; More of it: its time from now
            jsr         TICKS
            sta         pc_rxt
            stx         pc_rxt + 1
            jmp         again

@stopped:
            jsr         TICKS                               ; Nothing more for PC_QUIET ticks: it's dropped, and
            sec                                             ;   the request goes again
            sbc         pc_rxt
            tay
            txa
            sbc         pc_rxt + 1
            bne         :+
            cpy         #PC_QUIET
            bcc         @wait
:
            stz         pc_rxs
            bra         pc_again

@time:
            lda         #0
            jsr         pc_late
            bcs         pc_again                            ; Its time has come: again
@wait:
            jmp         again                               ; (Woken early: wait on)

@reply:
            stz         pc_rxf
            jsr         pc_check
            bcc         @good
            tax
            bne         pc_again                            ; Damaged: again
            jmp         again                               ; An older request's: wait on

@good:
            cmp         #PC_T_REPLY                         ; (PC_T_NAK: it reached the PC damaged)
            bne         pc_again
            lda         pc_rxbuf + 2                        ; (Shorter than a reply: not one)
            cmp         #PC_REPLY_HDR
            bcc         pc_again
            lda         pc_step
            cmp         #PS_ATTACH
            bne         pc_answer
            lda         pc_rxbuf + 4 + PC_REPLY_HDR - 1     ; Attached, if the PC tool speaks this version: now
            cmp         #PC_VERSION                         ;   the request
            bne         pc_gone
            lda         #1
            sta         pc_online
            jmp         pc_request

; Once more, if it has tries left; else the PC is gone
pc_again:
            dec         pc_tries
            beq         pc_gone
            jsr         pc_resend
            jmp         pc_resent

pc_gone:
            stz         pc_online
            lda         #E_IO
            bra         pc_error

; The reply: its status; its count, and an open's fid and qid type, into the request block (only an open's: the
; block goes back to the client, and the rest of a short write goes in it); its data (a read's, a stat record) to the
; client's buffer, no more than the client has room for
pc_answer:
            lda         pc_rxbuf + 4                        ; (Its status: 0, or the error)
            bne         pc_error
            lda         pc_rxbuf + 6
            sta         TASK_INBOX + RQ_DONE
            lda         pc_rxbuf + 7
            sta         TASK_INBOX + RQ_DONE + 1
            lda         TASK_INBOX + RQ_TYPE                ; A new fid: R_OPEN, R_CREATE, R_DUP
            cmp         #R_OPEN
            beq         :+
            cmp         #R_CREATE
            beq         :+
            cmp         #R_DUP
            bne         @data
:
            lda         pc_rxbuf + 5
            sta         TASK_INBOX + RQ_FID
            lda         pc_rxbuf + 8
            sta         TASK_INBOX + RQ_PERM
@data:
            sec                                             ; The data: the payload after the reply's header
            lda         pc_rxbuf + 2
            sbc         #PC_REPLY_HDR
            beq         @done
            sta         r2
            ldx         TASK_INBOX + RQ_TYPE                ; The room: a stat record, or the count asked for
            lda         #SR_SIZE
            cpx         #R_STAT
            beq         @room
            cpx         #R_READ
            bne         @done
            lda         TASK_INBOX + RQ_COUNT + 1
            bne         @copy
            lda         TASK_INBOX + RQ_COUNT
@room:
            cmp         r2
            bcs         @copy
            sta         r2
@copy:
            stz         r2 + 1
            LDR         r0, pc_rxbuf + 4 + PC_REPLY_HDR
            MOVR        r1, TASK_INBOX + RQ_BUF
            jsr         CLIENT_WRITE
@done:
            jsr         pc_release
            clc
            rts

; Done, with an error.  IN: .A = the error
pc_error:
            jsr         pc_release
            sec
            rts

; The request's done, or given up: the next may go out (the clients waiting their turn look again).  Keeps .A
pc_release:
            stz         pc_step
            ldx         #$FF
            stx         pc_owner
            stz         pc_rxf
            inc         TASK_EVENT
            rts

; Is this the request out, its client asking again, or another (it gave that one up: a non-blocking one)?  Its
; block's fields to RQ_NAMELEN, but its flags, and its name.  OUT: C = 1 the same.  Modifies .A, .X
pc_same:
            ldx         #RQ_NAMELEN
@field:
            cpx         #RQ_FLAGS
            beq         :+
            lda         TASK_INBOX,X
            cmp         pc_req,X
            bne         @no
:
            dex
            bpl         @field
            jsr         pc_named
            bcc         @yes
            jsr         pc_namelen                          ; (RQ_NAMELEN's the same: the same length)
:
            lda         TASK_PATH,X
            cmp         pc_txbuf + 4 + RQ_SIZE,X
            bne         @no
            dex
            bpl         :-
@yes:
            sec
            rts

@no:
            clc
            rts

; C = 1 if the request has a name (R_OPEN, R_CREATE, R_REMOVE: TASK_PATH).  Keeps .X
pc_named:
            lda         TASK_INBOX + RQ_TYPE
            cmp         #R_OPEN
            beq         @yes
            cmp         #R_CREATE
            beq         @yes
            cmp         #R_REMOVE
            beq         @yes
            clc
            rts

@yes:
            sec
            rts

; .X = the name's length (RQ_NAMELEN), PATH_MAX at most: its 0's place in TASK_PATH
pc_namelen:
            ldx         TASK_INBOX + RQ_NAMELEN
            cpx         #PATH_MAX
            bcc         :+
            ldx         #PATH_MAX
:
            rts

; The request's frame: PC_T_REQ, the request block, then its data: a name (with its 0), a write's bytes, or a stat
; record (R_WSTAT).  A read's or a write's count is PC_DATA at most (the frame's block says so)
pc_build:
            ldx         #RQ_SIZE - 1                        ; The block (and its client's fields: pc_same)
@block:
            lda         TASK_INBOX,X
            sta         pc_txbuf + 4,X
            cpx         #RQ_NAMELEN + 1
            bcs         :+
            sta         pc_req,X
:
            dex
            bpl         @block
            stz         pc_n                                ; (pc_n: the data's length)
            lda         TASK_INBOX + RQ_TYPE
            cmp         #R_READ
            beq         @read
            cmp         #R_WRITE
            beq         @write
            cmp         #R_WSTAT
            beq         @stat
            jsr         pc_named
            bcc         @frame
            jsr         pc_namelen                          ; The name, and its 0
            stx         pc_n
            inc         pc_n
:
            lda         TASK_PATH,X
            sta         pc_txbuf + 4 + RQ_SIZE,X
            dex
            bpl         :-
            bra         @frame

@read:
            jsr         pc_cap
            stz         pc_n                                ; (No data)
            bra         @frame

@write:
            jsr         pc_cap
            bra         @client

@stat:
            lda         #SR_SIZE
            sta         pc_n
@client:
            lda         pc_n                                ; The client's bytes
            sta         r2
            stz         r2 + 1
            LDR         r0, pc_txbuf + 4 + RQ_SIZE
            MOVR        r1, TASK_INBOX + RQ_BUF
            jsr         CLIENT_READ
@frame:
            lda         #PC_T_REQ
            sta         pc_txbuf
            lda         pc_n
            clc
            adc         #RQ_SIZE
                                                            ; (On to pc_frame)

; The frame in pc_txbuf, its type there: its tag, its payload's length (.A, after the header) and its CRC put in,
; and out it goes (pump: pc_pump)
pc_frame:
            sta         pc_txbuf + 2
            stz         pc_txbuf + 3
            ldx         pc_tag
            stx         pc_txbuf + 1
            clc
            adc         #4                                  ; The header and the payload
            sta         pc_len
            tax
            LDR         p, pc_txbuf
            jsr         pc_crc_of
            ldx         pc_len
            lda         pc_crc
            sta         pc_txbuf,X
            lda         pc_crc + 1
            sta         pc_txbuf + 1,X
            inx
            inx
            stx         pc_len

; The frame out again: from its start, PC_MARK first
pc_resend:
            stz         pc_txi
            lda         #PC_MARK
            sta         pc_txe
            rts

; A read's or a write's count, PC_DATA at most: in the frame's block, and pc_n
pc_cap:
            lda         #PC_DATA
            ldx         TASK_INBOX + RQ_COUNT + 1
            bne         :+
            cmp         TASK_INBOX + RQ_COUNT
            bcc         :+
            lda         TASK_INBOX + RQ_COUNT
:
            sta         pc_txbuf + 4 + RQ_COUNT
            stz         pc_txbuf + 4 + RQ_COUNT + 1
            sta         pc_n
            rts

; pump's: the frame going out (pc_txbuf, pc_len bytes, from pc_txi), into the send ring as it has room: PC_MARK
; first, then its bytes, PC_MARK and PC_ESC stuffed (PC_ESC, then the byte ^ $20).  OUT: C = 1: not all of it yet
; (the windows' text waits).  Modifies .A, .X, .Y
pc_pump:
            lda         pc_txe                              ; A byte as it is first (PC_MARK, or an escaped one's
            beq         @next                               ;   second)?
            jsr         tx_free
            beq         @full
            lda         pc_txe
            stz         pc_txe
            jsr         tx_put
@next:
            ldx         pc_txi                              ; The frame's next byte
            cpx         pc_len
            bcs         @done                               ; (All of it out)
            jsr         tx_free
            beq         @full
            inc         pc_txi
            lda         pc_txbuf,X
            cmp         #PC_MARK
            beq         :+
            cmp         #PC_ESC
            bne         @put
:
            eor         #$20
            sta         pc_txe
            lda         #PC_ESC
@put:
            jsr         tx_put
            bra         pc_pump

@done:
            clc
            rts

@full:
            sec
            rts

; distribute's: a byte of a /pc frame (PC_MARK starts one): its body into pc_rxbuf, unstuffed, and when it's whole,
; pc_rxf, if a request's out (else it's dropped).  One that comes while the last is still to be looked at is
; skipped.  IN: .A = the byte.  OUT: C = 0; or C = 1, .A = a key (PC_MARK then PC_ESC: a typed $1E).  Modifies .X
pc_rx:
            cmp         #PC_MARK
            bne         @body
            ldx         #1                                  ; A frame starts (one cut short is dropped)
            lda         pc_rxf
            beq         :+
            ldx         #$81                                ; (Skipped)
:
            stx         pc_rxs
            stz         pc_rxi
            stz         pc_rxe
            lda         #$FF                                ; (pc_waiting: none of it seen yet)
            sta         pc_rxn
            clc
            rts

@body:
            cmp         #PC_ESC
            bne         @plain
            ldx         pc_rxi
            bne         @escape
            stz         pc_rxs                              ; PC_MARK then PC_ESC: a typed $1E, a key
            lda         #PC_MARK
            sec
            rts

@escape:
            sta         pc_rxe                              ; (<> 0) The next byte is escaped
            clc
            rts

@plain:
            ldx         pc_rxe
            beq         :+
            stz         pc_rxe
            eor         #$20
:
            ldx         pc_rxi
            bit         pc_rxs                              ; (Skipped: not kept)
            bmi         :+
            sta         pc_rxbuf,X
:
            inc         pc_rxi
            cpx         #2
            bcc         @more                               ; (The type, the tag)
            beq         @low
            cpx         #3
            beq         @high
            inx                                             ; The payload and the CRC: all of it?
            cpx         pc_rxl
            bne         @more
            ldx         pc_rxs                              ; It's whole
            stz         pc_rxs
            bmi         @more                               ; (Skipped)
            lda         pc_step                             ; (No request out: dropped)
            sta         pc_rxf
@more:
            clc
            rts

@low:                                                       ; The payload's length: PC_RX_MAX at most, or it's not
            sta         pc_rxl                              ;   one of ours
            clc
            rts

@high:
            cmp         #0
            bne         @drop
            lda         pc_rxl
            cmp         #PC_RX_MAX + 1
            bcs         @drop
            adc         #4 + 2                              ; (C = 0) All of it: the header, payload and CRC
            sta         pc_rxl
            clc
            rts

@drop:
            stz         pc_rxs
            clc
            rts

; The frame in pc_rxbuf: its CRC, and its tag.  OUT: C = 0, .A = its type; or C = 1, .A = 0: not the request's (an
; older one's), .A <> 0: damaged
pc_check:
            lda         pc_rxbuf + 2                        ; The header and the payload
            clc
            adc         #4
            tax
            LDR         p, pc_rxbuf
            jsr         pc_crc_of
            lda         (p)                                 ; (p: just after them, the CRC)
            cmp         pc_crc
            bne         @damaged
            ldy         #1
            lda         (p),Y
            cmp         pc_crc + 1
            bne         @damaged
            lda         pc_rxbuf + 1
            cmp         pc_tag
            bne         @not_its
            lda         pc_rxbuf
            clc
            rts

@not_its:
            lda         #0
            sec
            rts

@damaged:
            lda         #1
            sec
            rts

; The CRC-16 (CCITT: $1021, from $FFFF) of .X bytes (1-255) at (p).  OUT: pc_crc; p just after them.  Modifies .A,
; .X, .Y
pc_crc_of:
            stx         pc_k
            lda         #$FF
            sta         pc_crc
            sta         pc_crc + 1
@byte:
            lda         (p)                                 ; (Greg Cook's, a byte at a time, no table)
            eor         pc_crc + 1
            sta         pc_crc + 1
            lsr
            lsr
            lsr
            lsr
            tax
            asl
            eor         pc_crc
            sta         pc_crc
            txa
            eor         pc_crc + 1
            sta         pc_crc + 1
            asl
            asl
            asl
            tax
            asl
            asl
            eor         pc_crc + 1
            tay
            txa
            rol
            eor         pc_crc
            sta         pc_crc + 1
            sty         pc_crc
            inc         p
            bne         :+
            inc         p + 1
:
            dec         pc_k
            bne         @byte
            rts

; pc_until = the tick count .A/.X ticks from now.  Modifies .A, .X, .Y
pc_from_now:
            sta         pc_t
            stx         pc_t + 1
            jsr         TICKS
            clc
            adc         pc_t
            sta         pc_until
            txa
            adc         pc_t + 1
            sta         pc_until + 1
            rts

; Is it .A ticks past the reply's time (pc_until), or more?  OUT: C = 1 yes.  Modifies .A, .X, .Y
pc_late:
            clc
            adc         pc_until
            sta         pc_t
            lda         pc_until + 1
            adc         #0
            sta         pc_t + 1
            jsr         TICKS                               ; Now - that: not negative once it's come
            sec
            sbc         pc_t
            txa
            sbc         pc_t + 1
            bmi         :+
            sec
            rts
:
            clc
            rts

.rodata
; ****************************************************************************
; The keys
CSI_N       = 10
csi_final:  .byte       "ABCDHFPQRS"                        ; (P-S: F1-F4 modified, CSI 1 ; m P)
csi_key:    .byte       KEY_UP, KEY_DOWN, KEY_RIGHT, KEY_LEFT, KEY_HOME, KEY_END, KEY_F1, KEY_F2, KEY_F3, KEY_F4
SS3_N       = 10
ss3_final:  .byte       "ABCDHFPQRS"
ss3_key:    .byte       KEY_UP, KEY_DOWN, KEY_RIGHT, KEY_LEFT, KEY_HOME, KEY_END, KEY_F1, KEY_F2, KEY_F3, KEY_F4
TILDE_N     = 20
tilde_n:    .byte       1, 2, 3, 4, 5, 6, 7, 8, 11, 12, 13, 14, 15, 17, 18, 19, 20, 21, 23, 24
tilde_key:  .byte       KEY_HOME, KEY_INS, KEY_DEL, KEY_END, KEY_PGUP, KEY_PGDN, KEY_HOME, KEY_END
            .byte       KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12
kv_letter:  .byte       "ABCDHF"                            ; keys vt's: the cursor keys', Home's and End's letters
kv_tilde:   .byte       2, 3, 5, 6, 0, 0, 0, 0, 15, 17, 18, 19, 20, 21, 23, 24  ; (KEY_INS on: ESC [ n ~)
EDIT_N      = 15
edit_keys:  .byte       CR, LF, CTRL_D, BS, DEL, KEY_DEL, KEY_LEFT, KEY_RIGHT, KEY_HOME, CTRL_A, KEY_END, CTRL_E
            .byte       CTRL_U, KEY_UP, KEY_DOWN
edit_vec:   .word       ed_cr, ed_lf, ed_eof, ed_bs, ed_bs, ed_del, ed_left, ed_right, ed_home, ed_home, ed_end, ed_end
            .word       ed_kill, ed_up, ed_down
.assert     * - edit_vec = EDIT_N * 2, error, "edit_keys and edit_vec don't match"
s_bell:     .byte       "#a/bell", 0

; ****************************************************************************
; The rates: the ACIA's code, and timer 2 for a character (10 bits, and the idle bits after it: 2 at 115200 on
; the Rockwell, 1 otherwise), in rounds of at most 65000 cycles.  A bit is 32 cycles at 115200 (the CPU's clock
; is the ACIA's * 2, times CPU_CLOCK_MULT).  RATE i, div, gap: T2C_i the cycles, T2R_i the rounds, T2P_i a round's
; count (less the 2 the timer adds).  (Symbols, not .define functions: ca65 loses nested ones' values)
.macro RATE i, div, gap
            .ident(.sprintf("T2C_%d", i)) = (10 + (gap)) * 32 * (div) * CPU_CLOCK_MULT
            .ident(.sprintf("T2R_%d", i)) = (.ident(.sprintf("T2C_%d", i)) + 64999) / 65000
            .ident(.sprintf("T2P_%d", i)) = .ident(.sprintf("T2C_%d", i)) / .ident(.sprintf("T2R_%d", i)) - 2
.endmacro
GAP         = 1
.if ACIA_CHIP = ACIA_WDC
GAP_FAST    = 1
.else
GAP_FAST    = 2
.endif
            RATE        0, 384, GAP                         ; 300
            RATE        1, 192, GAP                         ; 600
            RATE        2, 96, GAP                          ; 1200
            RATE        3, 48, GAP                          ; 2400
            RATE        4, 24, GAP                          ; 4800
            RATE        5, 12, GAP                          ; 9600
            RATE        6, 6, GAP                           ; 19200
            RATE        7, 1, GAP_FAST                      ; 115200
rate_code:  .byte       $06, $07, $08, $0A, $0C, ACIA_RATE_9600, ACIA_RATE_19200, ACIA_RATE_115200
rate_lo:    .byte       <T2P_0, <T2P_1, <T2P_2, <T2P_3, <T2P_4, <T2P_5, <T2P_6, <T2P_7
rate_hi:    .byte       >T2P_0, >T2P_1, >T2P_2, >T2P_3, >T2P_4, >T2P_5, >T2P_6, >T2P_7
rate_rounds: .byte      T2R_0, T2R_1, T2R_2, T2R_3, T2R_4, T2R_5, T2R_6, T2R_7
rate_name_lo: .byte     <s_300, <s_600, <s_1200, <s_2400, <s_4800, <s_9600, <s_19200, <s_115200
rate_name_hi: .byte     >s_300, >s_600, >s_1200, >s_2400, >s_4800, >s_9600, >s_19200, >s_115200
.assert     ACIA_RATE_9600 = $0E .and ACIA_RATE_19200 = $0F, error, "The rates' codes"

; ****************************************************************************
; The devices
SRV_TREES:
            .byte       'c'
            .word       srv_tree
            .byte       'P'
            .word       pc_tree
            .byte       0
pc_tree:
            SRV_ENTRY   s_root,    $FF, SK_RAW,  pc_serve,    SM_READ | SM_WRITE, 0     ; (#P: pc_serve does it all)
            .word       0
srv_tree:
            SRV_ENTRY   s_root,    $FF, SK_DIR,  0,           SM_READ,            0     ; 0
            SRV_ENTRY   s_cons,    0,   SK_DATA, h_cons,      SM_READ | SM_WRITE, 0     ; 1
            SRV_ENTRY   s_consctl, 0,   SK_CTL,  cons_cmds,   SM_READ | SM_WRITE, 7     ; 2 (reads as 7: ENT_CONSCTL)
            SRV_ENTRY   s_wctl,    0,   SK_CTL,  wctl_cmds,   SM_READ | SM_WRITE, 8     ; 3 (reads as 8)
            SRV_ENTRY   s_wnew,    0,   SK_DATA, h_wnew,      SM_READ,            0     ; 4
            SRV_ENTRY   s_ser,     0,   SK_DATA, h_ser,       SM_READ | SM_WRITE, 0     ; 5
            SRV_ENTRY   s_serctl,  0,   SK_CTL,  ser_cmds,    SM_READ | SM_WRITE, 9     ; 6 (reads as 9)
            SRV_ENTRY   s_consctl, $FE, SK_TEXT, gen_consctl, SM_READ,            0     ; 7 (the ctl files' states:
            SRV_ENTRY   s_wctl,    $FE, SK_TEXT, gen_wctl,    SM_READ,            0     ; 8   in no directory)
            SRV_ENTRY   s_serctl,  $FE, SK_TEXT, gen_serctl,  SM_READ,            0     ; 9
            SRV_ENTRY   s_kbdin,   0,   SK_DATA, h_kbdin,     SM_WRITE,           0     ; 10
            SRV_ENTRY   s_text,    0,   SK_DATA, h_text,      SM_READ,            0     ; 11
            SRV_ENTRY   s_label,   0,   SK_DATA, h_label,     SM_READ | SM_WRITE, 0     ; 12
            SRV_ENTRY   s_snarf,   0,   SK_DATA, h_snarf,     SM_READ | SM_WRITE, 0     ; 13
            .word       0
cons_cmds:
            .word       s_rawon_w, c_rawon
            .word       s_rawoff_w, c_rawoff
            .word       s_keys_w, c_keys
            .word       s_scroll_w, c_scroll
            .word       s_group_w, c_group
            .word       s_screen_w, c_screen
            .word       s_serial_w, c_serial
            .word       s_both_w, c_both
            .word       s_terminal_w, c_terminal
            .word       0
wctl_cmds:
            .word       s_new_w, c_new
            .word       s_current_w, c_current
            .word       s_bar_w, c_bar
            .word       s_header_w, c_header
            .word       s_footer_w, c_footer
            .word       s_default_w, c_default
            .word       s_chrome_w, c_chrome
            .word       s_status_w, c_status
            .word       s_monitor_w, c_monitor
            .word       s_key_w, c_key
            .word       s_history_w, c_history
            .word       s_layout_w, c_layout
            .word       0
ser_cmds:
            .word       s_b300, c_b300
            .word       s_b600, c_b600
            .word       s_b1200, c_b1200
            .word       s_b2400, c_b2400
            .word       s_b4800, c_b4800
            .word       s_b9600, c_b9600
            .word       s_b19200, c_b19200
            .word       s_b115200, c_b115200
            .word       0
s_root:     .byte       "/", 0
s_cons:     .byte       "cons", 0
s_consctl:  .byte       "consctl", 0
s_wctl:     .byte       "wctl", 0
s_wnew:     .byte       "wnew", 0
s_ser:      .byte       "ser", 0
s_serctl:   .byte       "serctl", 0
s_kbdin:    .byte       "kbdin", 0
s_text:     .byte       "text", 0
s_label:    .byte       "label", 0
s_snarf:    .byte       "snarf", 0
s_bar_w:    .byte       "bar", 0
s_header_w: .byte       "header", 0
s_footer_w: .byte       "footer", 0
s_default_w: .byte      "default", 0
s_chrome_w: .byte       "chrome", 0
s_status_w: .byte       "status", 0
s_monitor_w: .byte      "monitor", 0
s_key_w:    .byte       "key", 0
s_history_w: .byte      "history", 0
s_layout_w: .byte       "layout", 0
s_tabs_w:   .byte       "tabs", 0
s_rows_w:   .byte       "rows", 0
s_columns_w: .byte      "columns", 0
s_grid_w:   .byte       "grid", 0
lay_names:  .word       s_tabs_w, s_rows_w, s_columns_w, s_grid_w
.assert     * - lay_names = LAY_N * 2, error, "lay_names: LAY_N"
s_split_w:  .byte       "split", 0
s_vsplit_w: .byte       "vsplit", 0
s_zoom_w:   .byte       "zoom", 0
s_prefix_w: .byte       "prefix", 0
s_ctab_w:   .byte       "ctrl-tab", 0
s_cstab_w:  .byte       "ctrl-shift-tab", 0
s_tab_w:    .byte       "tab", 0
s_ctrl_w:   .byte       "ctrl-"
s_stab_w:   .byte       "shift-tab", 0
s_none_w:   .byte       "none", 0
s_next_w:   .byte       "next", 0
s_prev_w:   .byte       "previous", 0
s_gnext_w:  .byte       "next-group", 0
s_gprev_w:  .byte       "previous-group", 0
s_list_w:   .byte       "list", 0
s_hold_w:   .byte       "hold", 0
s_close_w:  .byte       "close", 0
s_paste_w:  .byte       "paste", 0
s_view_w:   .byte       "scrollback", 0
s_spgup_w:  .byte       "shift-pgup", 0
ka_names:   .word       s_none_w, s_next_w, s_prev_w, s_gnext_w, s_gprev_w, s_new_w, s_list_w, s_hold_w, s_close_w
            .word       s_paste_w, s_view_w, s_split_w, s_vsplit_w, s_zoom_w
ka_vec:     .word       k_none, win_next, win_prev, grp_next, grp_prev, k_new, ls_open, k_hold, k_close
            .word       k_paste, k_view, k_split, k_vsplit, k_zoom
.assert     * - ka_vec = KA_N * 2 .and ka_vec - ka_names = KA_N * 2, error, "ka_names and ka_vec: KA_N each"
kb_def:     .byte       HT, KA_NEXT, ESC, KA_PREV, 'c', KA_NEW, 'n', KA_GNEXT, 'p', KA_GPREV, 'w', KA_LIST
            .byte       'h', KA_HOLD, 'x', KA_CLOSE, 'y', KA_PASTE, '[', KA_VIEW, 's', KA_SPLIT, 'v', KA_VSPLIT
            .byte       'z', KA_ZOOM, 0                     ; (The keys after the prefix,
                                                            ;   as it starts)
s_vv_label: .byte       "scrollback", 0                     ; The view's label and footer
s_vv_foot:  .byte       "%[7] %y  Space: mark, Enter: copy, q: leave%=", 0
vk_let:     .byte       "ABHF"                              ; Its keys: the arrows, Home, End ...
vk_letm:    .byte       0, 1, 4, 5                          ;   their moves (vv_vec's) ...
vk_num:     .byte       5, 6, 1, 7, 4, 8                    ;   and ESC [ n ~'s: PgUp, PgDn, Home (1, 7), End (4, 8)
vk_numm:    .byte       2, 3, 4, 4, 5, 5
vv_vec:     .word       vv_up, vv_down, vv_pgup, vv_pgdn, vv_home, vv_end
s_bp_open:  .byte       ESC, "[200~", 0                     ; (Bracketed paste's)
s_bp_close: .byte       ESC, "[201~", 0
s_hex:      .byte       "0123456789abcdef"
s_ls_label: .byte       "windows", 0
s_ls_head:  .byte       ESC, "[?25lThe windows: a window's key, or the arrows and Enter, shows it; q the one before", CR, LF
            .byte       CR, LF, 0
s_ls_grp:   .byte       "  (group ", 0
s_ls_end:   .byte       ")", CR, LF, 0
s_ls_col:   .byte       ";1H", 0
s_top_w:    .byte       "top", 0
s_bottom_w: .byte       "bottom", 0
s_on_w:     .byte       "on", 0
s_off_w:    .byte       "off", 0
s_m_raw:    .byte       "raw", 0
s_m_cooked: .byte       "cooked", 0
s_m_vt:     .byte       " vt", 0
s_m_mods:   .byte       " mods", 0
s_m_held:   .byte       " held", 0
s_time:     .byte       "#t/time", 0
s_tm_none:  .byte       "????-??-?? ??:??:??"
s_bar_def:  .byte       "%[7] %G%=%t"                       ; The chrome's formats as the console starts (as
            .res        FMT_SIZE - (* - s_bar_def), 0       ;   /rom/lib/windows has them)
s_head_def: .byte       "%[1]%n %l%=%w"
            .res        FMT_SIZE - (* - s_head_def), 0
s_foot_def: .byte       "%s"
            .res        FMT_SIZE - (* - s_foot_def), 0
cr_codes:   .byte       "nglpswGcrmtdL=[%y"                 ; chr_render's codes, and theirs
CRC_N       = * - cr_codes
cr_vec:     .word       cr_num, cr_grp, cr_lbl, cr_prog, cr_stat, cr_wins, cr_groups, cr_cols, cr_rows, cr_modes
            .word       cr_time, cr_date, cr_leds, cr_right, cr_sgr, cr_pct, cr_y
cr_tens:    .byte       1, 10, 100
sgr_on:     .byte       0, F_BOLD, F_DIM, 0, F_UL, F_BLINK, F_BLINK, F_REV, F_INVIS, 0      ; (SGR 0-9's)
sgr_off:    .byte       $FF, $FF, <~(F_BOLD | F_DIM), $FF, <~F_UL, <~F_BLINK, $FF, <~F_REV, <~F_INVIS, $FF ; (20-29's)
s_rawon_w:  .byte       "rawon", 0
s_rawoff_w: .byte       "rawoff", 0
s_group_w:  .byte       "group", 0
s_screen_w: .byte       "screen", 0
s_serial_w: .byte       "serial", 0
s_both_w:   .byte       "both", 0
term_names: .word       s_serial_w, s_screen_w, s_both_w    ; (term 1-3)
term_bits:  .byte       CH_ALL, CH_ALL << 4, CH_ALL | (CH_ALL << 4) ; (chrome's terminals: serial, screen, both)
s_terminal: .byte       LF, "terminal ", 0
s_scr:      .byte       "#v/term", 0
s_scr_ctl:  .byte       "#v/ctl", 0
s_mode_w:   .byte       "mode ", 0
s_terminal_w: .byte     "terminal", 0
s_size_w:   .byte       "size", 0
s_size:     .byte       LF, "size ", 0
s_ask:      .byte       ESC, "[18t", 0
s_new_w:    .byte       "new", 0
s_current_w: .byte      "current", 0
s_rawon:    .byte       "rawon", LF, 0
s_rawoff:   .byte       "rawoff", LF, 0
s_keys_w:   .byte       "keys", 0
s_vt_w:     .byte       "vt", 0
s_hydra_w:  .byte       "hydra", 0
s_mods_w:   .byte       "mods", 0
s_keys_vt:  .byte       "keys vt", LF, 0
s_keys_hydra: .byte     "keys hydra", LF, 0
s_keys_mods: .byte      "keys mods", LF, 0
s_scroll_w: .byte       "scroll", 0
s_jump_w:   .byte       "jump", 0
s_smooth_w: .byte       "smooth", 0
s_scroll_jump: .byte    "scroll jump", LF, 0
s_scroll_smooth: .byte  "scroll smooth", LF, 0
s_group:    .byte       "group ", 0
s_window:   .byte       LF, "window ", 0
s_shown:    .byte       " *", 0
s_b300:     .byte       "b"
s_300:      .byte       "300", 0
s_b600:     .byte       "b"
s_600:      .byte       "600", 0
s_b1200:    .byte       "b"
s_1200:     .byte       "1200", 0
s_b2400:    .byte       "b"
s_2400:     .byte       "2400", 0
s_b4800:    .byte       "b"
s_4800:     .byte       "4800", 0
s_b9600:    .byte       "b"
s_9600:     .byte       "9600", 0
s_b19200:   .byte       "b"
s_19200:    .byte       "19200", 0
s_b115200:  .byte       "b"
s_115200:   .byte       "115200", 0

.include "srvlib.s"
