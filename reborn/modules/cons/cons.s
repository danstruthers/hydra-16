; ****************************************************************************
; cons - the console driver (docs/reimplementation-from-scratch.md, §14.2): the serial port, its rings, and the
; devices #c and #P (/pc, a folder on the PC), on srvlib (a boot driver: task F).
;
; Windows, Plan 9's way (rio's, on a text terminal), not job control: several consoles on the one terminal, each a
; window with its own cons and consctl, line editor, raw mode, note group and screen (vt.s, the second bank: cells
; in the driver's RAM banks, written by a VT100).  One window is shown and gets the keys; the others run on, their
; output going to their screens, their reads waiting for keys.  A window's files are #c with its number as the spec: #c2/cons (or mount '#c' /dev 2); #c is window 0.
;   /cons       the window's console.  A read gets a line, edited here (cooked): Backspace and Delete, Left, Right,
;               Home and End (and Ctrl-A, Ctrl-E), Ctrl-U, the history with Up and Down; Enter ends it, Ctrl-D on
;               an empty line is the end of the input.  Or (raw: consctl's rawon) each key as it comes, the
;               terminal's cursor and function keys as one code each (KEY_*; an Escape alone is a key once
;               ESC_TICKS have passed with nothing after it).  A write goes to the window's screen, and out to the
;               terminals if the window is shown (each LF as CR LF; a BEL rings the sound driver's bell too,
;               #a/bell: one of the calls from a driver to another, the screen's #v/term another)
;   /consctl    rawon, rawoff (raw lasts till the window's last consctl closes, as Plan 9's does); keys vt, keys
;               hydra (raw's keys: as a VT100 sends them, following the window's DECCKM, DECKPAM and VT52 mode, or
;               as one code each, KEY_*: as it starts, and again with its last consctl); group (the
;               window's notes go to the writer's note group); screen, serial, both (where the windows are shown:
;               every window's, the console's terminals: the Vera X's screen, the serial port, or both, as it
;               starts; screen with no screen: E_NODEV).  It reads as the state
;   /wctl       new (a window), current N (window N shown).  It reads as the windows, a line each (* the shown one)
;   /wnew       a read waits for the user's Ctrl-] c, then makes a window, shown, and gives its number (init's: it
;               starts a shell there)
;   /ser        the serial port, raw: bytes in and out as they are.  While it's open for reading, the line is its
;               (xmodem's): every byte in is its, Ctrl-C and the rest too, and the windows' text (and /pc's frames)
;               wait, kept as a hidden window's is, till its last close repaints the window shown
;   /serctl     the rate: b300, b600, b1200, b2400, b4800, b9600, b19200, b115200.  It reads as it
;   /kbdin      a write's bytes are the window's keys, as if typed (rio's kbdin: a line sent to another window's
;               shell, forth's send); all of them, as its keys' queue has room, the writer waiting for the rest
;   /text       the window's scrollback and screen as text, a line a row (rio's)
; The keys: Ctrl-] then a digit shows that window (Ctrl-] n the next; Ctrl-] c asks for a new one, for /wnew's
; reader; Ctrl-] Ctrl-] is a Ctrl-]); Ctrl-C and Ctrl-\ are notes (interrupt, kill) to the shown window's note
; group, in either mode.  A window goes when the last of its cons fids closes (but window 0).
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
;   The screen (the Vera X's, phase 8: docs/plans/VIDEO.md) is a second terminal, its driver's (vid: #v/term, an
; ANSI terminal; opened the first time, with no screen nothing from then on): the shown window's output goes there
; too, as vt.s makes it show the window's screen (written at each request's end), and a window shown is painted
; there all at once.  With the serial port off (consctl's screen), the shown window takes its writes whole, as a
; hidden one does: nothing paces it but the screen.
;
; /pc (#P, docs/plans/PC.md): a folder on the PC, served by the PC tool (sim/tools/hydrapc.js, which is the
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
ST_SIZE         = 16            ; Each window's editor state, kept while another's is in use (st_first on)
KP_SIZE         = 8             ; A key's sequence kept for a raw read (keys vt): its bytes at most
ECHO_ROOM       = LINE_MAX + 13 ; The most a key's echo writes (a key waits for this much room)
RX_PAGES        = 4             ; The receive ring's pages: the keys' its first; /ser's all of them (1023 bytes: a 1K
                                ;   XMODEM block at 115200 comes in faster than it can be taken, and waits there)
CTRL_A          = $01
CTRL_C          = $03
CTRL_D          = $04
CTRL_E          = $05
CTRL_U          = $15
CTRL_BSL        = $1C           ; (Ctrl-\)
CTRL_RB         = $1D           ; (Ctrl-]: the windows' key)
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
want_new:   .res        1                                   ; <> 0: Ctrl-] c, a window wanted (for /wnew's reader)
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
esc_semi:   .res        1                                   ;   past a ; (the modifiers: not kept) ...
esc_at:     .res        2                                   ;   and the tick its ESC came at
key_pb:     .res        1                                   ; A key put back (the one after an ESC that started
                                                            ;   nothing), or 0
hi_n:       .res        1                                   ; The history: its lines ...
hi_top:     .res        1                                   ;   the newest's slot ...
hi_at:      .res        1                                   ;   and Up and Down's place (0: the line being typed)
raw:        .res        1                                   ; <> 0: raw
st_last:                                                    ; ---- (Its end)
ST_N        = st_last - st_first
n:          .res        2                                   ; Scratch
m:          .res        2
p:          .res        2
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
kp_n:       .res        WIN_MAX                             ;   a key's sequence: its bytes, those read ...
kp_i:       .res        WIN_MAX
kp_buf:     .res        WIN_MAX * KP_SIZE                   ;   and them
w_iqh:      .res        WIN_MAX                             ;   and its keys: the next in, the next out
w_iqt:      .res        WIN_MAX
kbd_wait:   .res        1                                   ; <> 0: a /kbdin writer waits for a queue's room
bell:       .res        1                                   ; <> 0: a BEL the shown window sent (ring's) ...
bell_st:    .res        1                                   ;   #a/bell: 0 not opened yet, 1 open, 2 none ...
bell_fd:    .res        1                                   ;   and its fd
term:       .res        1                                   ; Where the windows are shown: TERM_SERIAL, TERM_SCREEN
scr_st:     .res        1                                   ; The screen, #v/term: 0 not opened yet, 1 open, 2 none ...
scr_fd:     .res        1                                   ;   and its fd
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
.assert     PC_TX_SIZE <= 256 .and PC_RX_SIZE <= 256, error, "/pc's frames: 8-bit indexes"
.assert     RQ_NAMELEN < RQ_SIZE .and RQ_FLAGS < RQ_NAMELEN, error, "/pc: pc_same's fields"
.assert     WIN_MAX * INQ_SIZE = 256 .and WIN_MAX = 4, error, "iq_put and iq_get: 4 queues of 64, a page"

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
            FAR2        vt_init
            ldx         #0                                  ; Window 0: shown, init's group's
            jsr         w_init
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
            cmp         #CTRL_RB
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
            eor         #'0'                                ;   entry acts on the rest).  ($30-$33 alone give 0-3)
            cmp         #WIN_MAX
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
            lda         #ESC_NAP
            ldy         esc_wait
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
            bne         @done
@byte:
            jsr         rx_get
            bcs         @done
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
            cmp         #CTRL_RB
            bne         @key
            inc         d_pfx
            bra         @byte

@key:
            ldx         w_in
            jsr         iq_put
            bra         @byte

@command:                                                   ; The key after Ctrl-]
            stz         d_pfx
            cmp         #CTRL_RB                            ; (Ctrl-] again: a Ctrl-])
            beq         @key
            cmp         #'c'                                ; c: a window wanted (for /wnew's reader)
            beq         @new
            cmp         #'n'                                ; n: the next window
            beq         @next
            sec                                             ; A digit: that window
            sbc         #'0'
            cmp         #WIN_MAX
            bcs         @same
            tax
            lda         w_used,X
            beq         @same
            jsr         w_show
            bra         @byte

@next:
            ldx         w_in
:
            inx
            cpx         #WIN_MAX
            bcc         :+
            ldx         #0
:
            lda         w_used,X
            beq         :--
            jsr         w_show
            bra         @byte

@new:
            lda         #1
            sta         want_new
            inc         TASK_EVENT
@same:                                                      ; (No window shown anew: the keys' note group the shown
            ldx         w_in                                ;   one's still)
            lda         w_group,X
            sta         win_grp
            bra         @byte

@done:
            rts

; Window .X shown, with the keys: painted on both terminals (pump)
w_show:
            stx         w_in
            lda         w_group,X
            sta         win_grp
            lda         #1
            sta         ts_ser
            sta         ts_scr
            inc         TASK_EVENT                          ; (Its readers and writers, and the last one's, look
            rts                                             ;   again)

; A window made: the lowest free.  OUT: C = 0, .X = it; or C = 1, .A = E_NOMEM
w_make:
            ldx         #0
:
            lda         w_used,X
            beq         w_init
            inx
            cpx         #WIN_MAX
            bcc         :-
            lda         #E_NOMEM
            sec
            rts

; Window .X, new: its screen (vt.s: three banks), empty; init's group's.  OUT: C = 0; or C = 1, .A = E_NOMEM.
; Keeps .X
w_init:
            phx
            FAR2        vt_new
            plx
            bcc         :+
            rts
:
            lda         #1
            sta         w_used,X
            stz         w_iqh,X
            stz         w_iqt,X
            stz         w_cons,X
            stz         w_ctl,X
            stz         kvt,X
            stz         kp_n,X
            stz         kp_i,X
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
            phx
            FAR2        vt_free
            plx
            cpx         lw
            bne         :+
            lda         #$FF
            sta         lw
:
            cpx         w_in
            bne         :+
            ldx         #0
            jsr         w_show
:
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
            txa
            asl
            asl
            asl
            asl
            clc
            adc         #<w_state
            sta         p
            lda         #>w_state
            adc         #0
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

.assert     ST_SIZE = 16 .and LINE_MAX + 1 = 128, error, "st_addr and ln_addr: 16 and 128 bytes a window"

; Key .A into window .X's queue (dropped if it's full).  Keeps .X
iq_put:
            pha
            lda         w_iqh,X
            inc         a
            and         #INQ_SIZE - 1
            cmp         w_iqt,X
            beq         @full
            sta         n
            txa                                             ; (Its queue: 64 * the window)
            lsr
            ror
            ror
            ora         w_iqh,X
            tay
            pla
            sta         inq,Y
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
            txa
            lsr
            ror
            ror
            ora         w_iqt,X
            tay
            lda         w_iqt,X
            inc         a
            and         #INQ_SIZE - 1
            sta         w_iqt,X
            lda         kbd_wait                            ; (A /kbdin writer waiting for room: it looks again)
            beq         :+
            stz         kbd_wait
            inc         TASK_EVENT
:
            lda         inq,Y
            clc
            rts

@none:
            sec
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
            bcs         :+
            sta         scr_fd
            ldx         #1
:
            stx         scr_st
            cpx         #1
            bne         @none
@open:
            clc
            rts

@none:
            sec
            rts

; A fid made (srvlib): its window, from the spec (none: window 0); a window that isn't there: E_NOENT.  (R_DUP's
; keeps its old fid's.)  A consctl's counted.  IN: .X = the fid.  Keeps .X
opened:
            lda         z:srv_rq
            cmp         #R_OPEN
            bne         @ok
            lda         TASK_INBOX + RQ_SPEC                ; A digit, or nothing
            beq         @zero
            sec
            sbc         #'0'
            cmp         #WIN_MAX
            bcs         @noent
            ldy         TASK_INBOX + RQ_SPEC + 1
            bne         @noent
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

; A fid forgotten (srvlib): a consctl's count down; with its window's last, raw ends (Plan 9's: raw lasts while
; consctl is open, so a program that ends raw, or is ended, leaves its window cooked).  IN: .X = the fid
clunked:
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
            tya
            jsr         load
            stz         raw
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
            jsr         w_make
            bcs         @done
            jsr         w_show                              ; (The user's: shown, as rio's new window is)
            txa
            ora         #'0'
            sta         iobuf
            lda         #LF
            sta         iobuf + 1
            ldx         #2
            jmp         r_give

@done:
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
            lda         lw                                  ; The shown window, the serial port being painted?
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
            stz         esc_st
            stz         esc_wait
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

@answer:                                                    ; Raw: the window's answers first (DA, DSR's ...: vt.s's),
            lda         raw                                 ;   as they came
            beq         @byte
            ldx         lw
            lda         ans_r,X
            cmp         ans_n,X
            bcs         @byte
            sta         n                                   ; (Its place: the window * ANS_SIZE + those read)
            inc         ans_r,X
            txa
            asl
            asl
            asl
            asl
            asl
            clc
            adc         n
            tay
            lda         ans_r,X                             ; (All read: none again)
            cmp         ans_n,X
            bne         :+
            stz         ans_r,X
            stz         ans_n,X
:
            lda         ans_buf,Y
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
            bra         @byte

@ss3:
            lda         #3
            sta         esc_st
            bra         @byte

@csi:
            dex
            bne         @o
            cmp         #'0'                                ; ESC [: a digit of its number?
            bcc         @notdigit
            cmp         #'9' + 1
            bcs         @notdigit
            ldx         esc_semi
            bne         @byte                               ; (A modifier's: not kept)
            and         #$0F
            sta         p
            lda         esc_n                               ; * 10, + the digit
            asl
            asl
            clc
            adc         esc_n
            asl
            clc
            adc         p
            sta         esc_n
            bra         @byte

@notdigit:
            cmp         #';'
            bne         @final
            sta         esc_semi
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
            bne         @byte                               ; (Not one of ours: dropped)
            lda         esc_n                               ; ESC [ n ~: by n
            ldx         #TILDE_N - 1
:
            cmp         tilde_n,X
            beq         @tildekey
            dex
            bpl         :-
            jmp         @byte

@tildekey:
            lda         tilde_key,X
            clc
            rts

@csikey:
            lda         csi_key,X
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
            jsr         esc_nap
            sec
            rts

@alone:
            stz         esc_st
            stz         esc_wait
            lda         #ESC
            clc
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
            jsr         key_next
            bcs         @wait
            cmp         #LF                                 ; An LF just after a CR: the same Enter
            bne         :+
            ldx         was_cr
            stz         was_cr
            bne         edit
:
            stz         was_cr
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
            jsr         echo_rest                           ; It and the rest ...
            inc         ln_pos
            lda         ln_len                              ;   and the cursor back after it
            sec
            sbc         ln_pos
            ldx         #'D'
            jmp         echo_csi

@full:
            rts

; Enter: the line ends, with an LF (CR: and the next key's LF is the same Enter)
ed_cr:
            lda         #1
            sta         was_cr
ed_lf:
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
            lda         #BS
            jsr         w_put
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
            jsr         echo_rest                           ; The rest, the end of the old line erased, and the
            jsr         echo_erase                          ;   cursor back
            lda         ln_len
            sec
            sbc         ln_pos
            ldx         #'D'
            jsr         echo_csi
ed_none:
            clc
            rts

ed_left:
            lda         ln_pos
            beq         ed_none
            dec         ln_pos
            lda         #BS
            jsr         w_put
            clc
            rts

ed_right:
            ldx         ln_pos
            cpx         ln_len
            bcs         ed_none
            lda         ln_buf,X
            jsr         w_put
            inc         ln_pos
            clc
            rts

ed_home:
            lda         ln_pos
            ldx         #'D'
            jsr         echo_csi
            stz         ln_pos
            clc
            rts

ed_end:
            lda         ln_len
            sec
            sbc         ln_pos
            ldx         #'C'
            jsr         echo_csi
            lda         ln_len
            sta         ln_pos
            clc
            rts

; Ctrl-U: the whole line
ed_kill:
            lda         ln_pos
            ldx         #'D'
            jsr         echo_csi
            jsr         echo_erase
            stz         ln_len
            stz         ln_pos
            stz         hi_at
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
            lda         ln_pos
            ldx         #'D'
            jsr         echo_csi
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
            jsr         echo_rest
            jsr         echo_erase
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

; The line from the cursor to its end
echo_rest:
            ldx         ln_pos
:
            cpx         ln_len
            bcs         :+
            lda         ln_buf,X
            jsr         w_put
            inx
            bra         :-
:
            rts

; The line erased from the cursor: ESC [ K
echo_erase:
            lda         #ESC
            jsr         w_put
            lda         #'['
            jsr         w_put
            lda         #'K'
            jmp         w_put

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
; consctl, wctl and serctl

; rawon, rawoff: the window's
c_rawon:
            lda         z:srv_id
            jsr         load
            lda         #1
            sta         raw
            clc
            rts

c_rawoff:
            lda         z:srv_id
            jsr         load
            stz         raw
            clc
            rts

; keys vt, keys hydra: a raw read's keys as a VT100 sends them, or one code each (KEY_*)
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
            lda         #<s_hydra_w
            ldx         #>s_hydra_w
            jsr         word_is
            bne         @inval
            ldy         #0
@set:
            ldx         z:srv_id
            tya
            sta         kvt,X
            stz         kp_n,X
            stz         kp_i,X
            clc
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

; consctl's state: "rawon" or "rawoff", "group N", "window N"
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
            ldy         z:srv_id                            ; keys hydra, keys vt
            lda         kvt,Y
            beq         :+
            lda         #<s_keys_vt
            ldx         #>s_keys_vt
            bra         :++
:
            lda         #<s_keys_hydra
            ldx         #>s_keys_hydra
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
            inc         TASK_EVENT                          ; (A writer waiting for the line's room looks again)
            clc
            rts

; wctl: new (a window), current N (window N shown)
c_new:
            jsr         w_make
            bcs         :+
            clc
:
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

; wctl's state: the windows, a line each: "N", and " *" for the one shown
gen_wctl:
            stz         cnt
@window:
            ldx         cnt
            lda         w_used,X
            beq         @next
            txa
            ora         #'0'
            jsr         srv_tputc
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
CSI_N       = 6
csi_final:  .byte       "ABCDHF"
csi_key:    .byte       KEY_UP, KEY_DOWN, KEY_RIGHT, KEY_LEFT, KEY_HOME, KEY_END
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
            .word       0
cons_cmds:
            .word       s_rawon_w, c_rawon
            .word       s_rawoff_w, c_rawoff
            .word       s_keys_w, c_keys
            .word       s_group_w, c_group
            .word       s_screen_w, c_screen
            .word       s_serial_w, c_serial
            .word       s_both_w, c_both
            .word       0
wctl_cmds:
            .word       s_new_w, c_new
            .word       s_current_w, c_current
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
s_rawon_w:  .byte       "rawon", 0
s_rawoff_w: .byte       "rawoff", 0
s_group_w:  .byte       "group", 0
s_screen_w: .byte       "screen", 0
s_serial_w: .byte       "serial", 0
s_both_w:   .byte       "both", 0
term_names: .word       s_serial_w, s_screen_w, s_both_w    ; (term 1-3)
s_terminal: .byte       LF, "terminal ", 0
s_scr:      .byte       "#v/term", 0
s_new_w:    .byte       "new", 0
s_current_w: .byte      "current", 0
s_rawon:    .byte       "rawon", LF, 0
s_rawoff:   .byte       "rawoff", LF, 0
s_keys_w:   .byte       "keys", 0
s_vt_w:     .byte       "vt", 0
s_hydra_w:  .byte       "hydra", 0
s_keys_vt:  .byte       "keys vt", LF, 0
s_keys_hydra: .byte     "keys hydra", LF, 0
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
