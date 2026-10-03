; ****************************************************************************
; cons - the console driver (docs/reimplementation-from-scratch.md, §14.2): the serial port, its rings, and the
; device #c, on srvlib (a boot driver: task F).
;
; Windows, Plan 9's way (rio's, on a text terminal), not job control: several consoles on the one terminal, each a
; window with its own cons and consctl, line editor, raw mode, note group and text (its last 2K of output).  One
; window is shown and gets the keys; the others run on, their output going into their text, their reads waiting
; for keys.  A window's files are #c with its number as the spec: #c2/cons (or mount '#c' /dev 2); #c is window 0.
;   /cons       the window's console.  A read gets a line, edited here (cooked): Backspace and Delete, Left, Right,
;               Home and End (and Ctrl-A, Ctrl-E), Ctrl-U, the history with Up and Down; Enter ends it, Ctrl-D on
;               an empty line is the end of the input.  Or (raw: consctl's rawon) each key as it comes, the
;               terminal's cursor and function keys as one code each (KEY_*; an Escape alone waits for the key
;               after it).  A write goes into the window's text, and out if the window is shown (each LF as CR LF)
;   /consctl    rawon, rawoff; group (the window's notes go to the writer's note group).  It reads as the state
;   /wctl       new (a window), current N (window N shown).  It reads as the windows, a line each (* the shown one)
;   /wnew       a read waits for the user's Ctrl-] c, then makes a window, shown, and gives its number (init's: it
;               starts a shell there)
;   /ser        the serial port, raw: bytes in and out as they are.  While it's open for reading, the keys are its,
;               not the windows'
;   /serctl     the rate: b300, b600, b1200, b2400, b4800, b9600, b19200, b115200.  It reads as it
; The keys: Ctrl-] then a digit shows that window (Ctrl-] n the next; Ctrl-] c asks for a new one, for /wnew's
; reader; Ctrl-] Ctrl-] is a Ctrl-]); Ctrl-C and Ctrl-\ are notes (interrupt, kill) to the shown window's note
; group, in either mode.  A window goes when the last of its cons fids closes (but window 0).
;
; Receiving: the ACIA's interrupt (LINE_ACIA) puts each byte into the receive ring and adds 1 to the event count
; (TASK_EVENT: the clients waiting look again); before each request the keys are handed to the windows' queues
; (Ctrl-] and the key after it acted on there).  Sending: a window's output goes into its text; after each request
; the shown window's text goes into the send ring, as there's room (a window just shown: the screen cleared, and its
; last 24 lines from their start).  VIA timer 2 (LINE_VIA_T2) runs a character's time and a margin, and its
; interrupt sends the next byte of the send ring.  Paced, at every rate, on both chips: the WDC W65C51N's TDRE
; doesn't work, and on the board the Rockwell's sending back to back at 115200 loses characters (2 idle bits then;
; 1 otherwise).  The interrupts' work is a few dozen cycles each: the IRQs-off budget (200 cycles) has the
; dispatch's 115 in it.

.include "hydra.inc"
.include "hw.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "srvlib.inc"

            HYX2_DRIVER "cons", init, srv_serve, irq, 0, HF_BOOT

SRV_FLUSH       = flush                                     ; (srvlib: a reader's call ended by a note)
SRV_OPENED      = opened                                    ;   (a fid made: its window)
SRV_PRE         = distribute                                ;   (before each request: the keys to the windows)
SRV_POST        = pump                                      ;   (and after it: the shown window's text out)

WIN_MAX         = 4             ; Windows
TEXT_SIZE       = 2048          ; Each window's text: its last output ...
TEXT_MAX        = TEXT_SIZE - 1 ;   (of which this much is kept)
INQ_SIZE        = 64            ; Each window's keys, waiting to be read
SCREEN_ROWS     = 24            ; A window shown again: its text's last 24 lines
LINE_MAX        = 127           ; A line's length at most (and its LF)
HIST_N          = 4             ; Each window's history: its lines ...
HIST_SIZE       = 128           ;   each its length, then LINE_MAX characters
ST_SIZE         = 16            ; Each window's editor state, kept while another's is in use (st_first on)
ECHO_ROOM       = LINE_MAX + 13 ; The most a key's echo puts into the text (a key waits for this much room)
IOBUF           = 64            ; A write's bytes, a part at a time
CTRL_A          = $01
CTRL_C          = $03
CTRL_D          = $04
CTRL_E          = $05
BS              = $08
CTRL_U          = $15
ESC             = $1B
CTRL_BSL        = $1C           ; (Ctrl-\)
CTRL_RB         = $1D           ; (Ctrl-]: the windows' key)
DEL             = $7F
RATE_BOOT       = 5             ; 9600: the kernel's bring-up console's

.zeropage
rx_head:    .res        1                                   ; The receive ring: the irq entry's end ...
rx_tail:    .res        1                                   ;   and the serve entry's
tx_head:    .res        1                                   ; The send ring: the serve entry's end ...
tx_tail:    .res        1                                   ;   and timer 2's
tx_busy:    .res        1                                   ; <> 0: a byte is going (timer 2 runs)
t2_lo:      .res        1                                   ; Timer 2 for a character: a round's count ...
t2_hi:      .res        1
t2_rounds:  .res        1                                   ;   the rounds (more than 1 at slow rates) ...
t2_left:    .res        1                                   ;   and those left of this one
rate:       .res        1                                   ; The rate (its index in the tables)
pfx:        .res        1                                   ; The irq entry's: <> 0, the last key was Ctrl-] ...
win_grp:    .res        1                                   ;   and the note group of the window with the keys
d_pfx:      .res        1                                   ; Handing the keys out: <> 0, the last was Ctrl-]
w_in:       .res        1                                   ; The window shown, which gets the keys
repaint:    .res        1                                   ; <> 0: it's just been shown (its screen to repaint)
want_new:   .res        1                                   ; <> 0: Ctrl-] c, a window wanted (for /wnew's reader)
ser_rd:     .res        1                                   ; /ser's fids for reading (while there are any, the
                                                            ;   keys are /ser's)
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
esc_semi:   .res        1                                   ;   past a ; (the modifiers: not kept)
key_pb:     .res        1                                   ; A key put back (the one after an ESC that started
                                                            ;   nothing), or 0
hi_n:       .res        1                                   ; The history: its lines ...
hi_top:     .res        1                                   ;   the newest's slot ...
hi_at:      .res        1                                   ;   and Up and Down's place (0: the line being typed)
raw:        .res        1                                   ; <> 0: raw
st_last:                                                    ; ---- (Its end)
ST_N        = st_last - st_first
tp:         .res        2                                   ; A window's text: a byte's address ...
tq:         .res        2                                   ;   and its place (t_at)
n:          .res        2                                   ; Scratch
m:          .res        2
p:          .res        2
cnt:        .res        1
budget:     .res        1                                   ; A write to the shown window: the send ring's room

.bss
rx_buf:     .res        256
tx_buf:     .res        256
text:       .res        WIN_MAX * TEXT_SIZE                 ; Each window's text
inq:        .res        WIN_MAX * INQ_SIZE                  ; Each window's keys
lines:      .res        WIN_MAX * (LINE_MAX + 1)            ; Each window's line, while another's is loaded
hist:       .res        WIN_MAX * HIST_N * HIST_SIZE        ; Each window's history
w_state:    .res        WIN_MAX * ST_SIZE                   ; Each window's editor state, while another's is loaded
ln_buf:     .res        LINE_MAX + 1                        ; The loaded window's line
iobuf:      .res        IOBUF
w_used:     .res        WIN_MAX                             ; Each window: <> 0, it's there ...
w_group:    .res        WIN_MAX                             ;   its note group (Ctrl-C's) ...
w_cons:     .res        WIN_MAX                             ;   its cons fids ...
w_hl:       .res        WIN_MAX                             ;   its text's place: where the next byte goes ...
w_hh:       .res        WIN_MAX
w_sl:       .res        WIN_MAX                             ;   the next byte out (the shown one's) ...
w_sh:       .res        WIN_MAX
w_cl:       .res        WIN_MAX                             ;   the bytes there are (TEXT_MAX at most) ...
w_ch:       .res        WIN_MAX
w_iqh:      .res        WIN_MAX                             ;   and its keys: the next in, the next out
w_iqt:      .res        WIN_MAX

.assert     ST_N <= ST_SIZE, error, "A window's editor state is bigger than ST_SIZE"
.assert     WIN_MAX * INQ_SIZE = 256 .and WIN_MAX = 4, error, "iq_put and iq_get: 4 queues of 64, a page"
.assert     TEXT_SIZE = 2048, error, "t_at: a window's text is 8 pages"

.code
; ****************************************************************************
; The driver's init: window 0, its lines, the rate, the ACIA's receive interrupt on, the device
init:
            ldx         #cnt - rx_head                      ; (Its zero page: all 0)
:
            stz         rx_head,X
            dex
            bpl         :-
            ldx         #WIN_MAX - 1
:
            stz         w_used,X
            dex
            bpl         :-
            lda         #$FF
            sta         lw
            ldx         #0                                  ; Window 0: shown, init's group's
            jsr         w_init
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
            jmp         SRV_REGISTER                        ; (Its error is init's)

@done:
            rts

; ****************************************************************************
; The irq entry: .A = the line.  Short: about 70 cycles at most, and no WAKE (TASK_EVENT)
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

@after:                                                     ; The key after Ctrl-]: a digit is the window that has
            stz         pfx                                 ;   the keys now (its note group Ctrl-C's: the serve
            tax                                             ;   entry acts on the rest)
            sec
            sbc         #'0'
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

; Timer 2 ran out: the next byte (a character's time since the last went), or nothing more to send
t2_next:
            dec         t2_left                             ; (A slow rate: another round)
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
            lda         VIA_T2CL                            ; (Its interrupt cleared)
            stz         tx_busy
            inc         TASK_EVENT                          ; (The writers waiting for room look again)
            lda         #0
            rts

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

; A byte from the receive ring.  OUT: C = 0, .A = it; or C = 1: none.  Modifies .Y
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

; Before each request: the keys come in, each to the window shown (its queue), Ctrl-] and the key after it acted on.
; None while /ser is open for reading: the keys are its
distribute:
            lda         ser_rd
            bne         @done
@byte:
            jsr         rx_get
            bcs         @done
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

; Window .X shown, with the keys: repainted (pump)
w_show:
            stx         w_in
            lda         w_group,X
            sta         win_grp
            lda         #1
            sta         repaint
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

; Window .X, new: empty, init's group's.  OUT: C = 0.  Keeps .X
w_init:
            lda         #1
            sta         w_used,X
            stz         w_hl,X
            stz         w_hh,X
            stz         w_sl,X
            stz         w_sh,X
            stz         w_cl,X
            stz         w_ch,X
            stz         w_iqh,X
            stz         w_iqt,X
            stz         w_cons,X
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

; Window .X gone (its last cons closed); if it was shown, window 0 is
w_free:
            stz         w_used,X
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
            lda         inq,Y
            clc
            rts

@none:
            sec
            rts

; tp = window .X's text at place tq (its low 11 bits).  Keeps .X, .Y
t_at:
            txa
            asl
            asl
            asl
            sta         tp + 1
            lda         tq + 1
            and         #>TEXT_MAX
            ora         tp + 1
            sta         tp + 1
            clc
            lda         tq
            adc         #<text
            sta         tp
            lda         tp + 1
            adc         #>text
            sta         tp + 1
            rts

; .A into the loaded window's text (the writers have made sure of the room: w_room).  Keeps .A, .X, .Y
w_put:
            phx
            phy
            pha
            ldx         lw
            lda         w_hl,X
            sta         tq
            lda         w_hh,X
            sta         tq + 1
            jsr         t_at
            pla
            pha
            sta         (tp)
            inc         w_hl,X                              ; The place on ...
            bne         :+
            inc         w_hh,X
:
            lda         w_ch,X                              ;   and the bytes there are, TEXT_MAX at most
            cmp         #>TEXT_MAX
            bcc         @more
            lda         w_cl,X
            cmp         #<TEXT_MAX
            bcs         @kept
@more:
            inc         w_cl,X
            bne         @kept
            inc         w_ch,X
@kept:
            pla
            ply
            plx
            rts

; m = the room in the loaded window's text: all of it if it isn't shown (its oldest bytes go), else as much as
; doesn't overtake what's still to go out.  Modifies .A, .X
w_room:
            lda         #<TEXT_MAX
            sta         m
            lda         #>TEXT_MAX
            sta         m + 1
            ldx         lw
            cpx         w_in
            bne         @done
            sec                                             ; Less what's still to go out
            lda         w_hl,X
            sbc         w_sl,X
            sta         n
            lda         w_hh,X
            sbc         w_sh,X
            sta         n + 1
            sec
            lda         m
            sbc         n
            sta         m
            lda         m + 1
            sbc         n + 1
            sta         m + 1
            bcs         @done
            stz         m
            stz         m + 1
@done:
            rts

; After each request: the shown window's text out, as the send ring has room (each LF as CR LF); a window just shown
; first: the screen cleared, and its text from the start of its last SCREEN_ROWS lines
pump:
            lda         repaint
            beq         @text
            jsr         tx_free
            cmp         #8
            bcc         @done
            ldx         #0
:
            lda         s_clear,X
            beq         :+
            jsr         tx_put
            inx
            bra         :-
:
            jsr         replay
            stz         repaint
@text:
            ldx         w_in                                ; All of it out?
            lda         w_sl,X
            cmp         w_hl,X
            bne         @byte
            lda         w_sh,X
            cmp         w_hh,X
            beq         @done
@byte:
            jsr         tx_free                             ; (Room for a CR LF)
            cmp         #2
            bcc         @done
            ldx         w_in
            lda         w_sl,X
            sta         tq
            lda         w_sh,X
            sta         tq + 1
            jsr         t_at
            inc         w_sl,X
            bne         :+
            inc         w_sh,X
:
            lda         (tp)
            cmp         #LF
            bne         :+
            lda         #CR
            jsr         tx_put
            lda         #LF
:
            jsr         tx_put
            bra         @text

@done:
            jmp         tx_start

; The shown window's next byte out: the start of its text's last SCREEN_ROWS lines (or its oldest byte)
replay:
            ldx         w_in
            lda         w_hl,X                              ; tq: back from the end ...
            sta         tq
            lda         w_hh,X
            sta         tq + 1
            lda         w_cl,X                              ;   m: no further than this
            sta         m
            lda         w_ch,X
            sta         m + 1
            stz         cnt                                 ; (The LFs passed)
@back:
            lda         m
            ora         m + 1
            beq         @start
            lda         tq
            bne         :+
            dec         tq + 1
:
            dec         tq
            lda         m
            bne         :+
            dec         m + 1
:
            dec         m
            jsr         t_at
            lda         (tp)
            cmp         #LF
            bne         @back
            inc         cnt
            lda         cnt
            cmp         #SCREEN_ROWS
            bcc         @back
            inc         tq                                  ; (From the byte after that LF)
            bne         @start
            inc         tq + 1
@start:
            lda         tq
            sta         w_sl,X
            lda         tq + 1
            sta         w_sh,X
            rts

; A fid made (srvlib): its window, from the spec (none: window 0); a window that isn't there: E_NOENT.  (R_DUP's
; keeps its old fid's.)  IN: .X = the fid.  Keeps .X
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
            clc
            rts

@noent:
            lda         #E_NOENT
            sec
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

; /ser: a read, a write; its fids for reading counted
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
@done:
            clc
            rts

@open:
            inc         ser_rd
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

; /cons: a read.  Cooked, a line (or what's left of one); raw, the keys there are.  IN: .X = the fid
r_cons:
            lda         srv_fid_aux,X
            jsr         load
            lda         raw
            bne         r_keys
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

; /ser: the bytes there are, as they are; or E_AGAIN
r_ser:
            ldx         #0
@byte:
            jsr         r_room
            bcs         r_give
            jsr         rx_get
            bcs         r_give
            sta         iobuf,X
            inx
            bra         @byte

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

; /cons: a write, into the window's text.  A window that isn't shown takes it all (its oldest text goes); the shown
; one as much as the send ring has room for now (each LF as CR LF; none while its text has more to go out), so all
; of it goes out at this request's end, and the writer, waiting for room, comes back for the rest (the kernel sends
; it again).  None taken: E_AGAIN.  IN: .X = the fid
w_write:
            lda         srv_fid_aux,X
            jsr         load
            lda         #$FF                                ; budget: the shown window's room
            sta         budget
            ldx         lw
            cpx         w_in
            bne         :+
            stz         budget
            lda         w_hl,X                              ; (Its text all out?)
            cmp         w_sl,X
            bne         :+
            lda         w_hh,X
            cmp         w_sh,X
            bne         :+
            jsr         tx_free
            sta         budget
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
            ldx         #0
@byte:
            lda         lw                                  ; The shown window's: room in the send ring?
            cmp         w_in
            bne         @put
            ldy         #1                                  ; (It takes 1, or an LF 2: CR LF)
            lda         iobuf,X
            cmp         #LF
            bne         :+
            iny
:
            sty         m
            lda         budget
            cmp         m
            bcc         @end
            sbc         m                                   ; (C = 1)
            sta         budget
@put:
            lda         iobuf,X
            jsr         w_put
            inc         n
            bne         :+
            inc         n + 1
:
            inx
            cpx         cnt
            bne         @byte
            bra         @part

@end:
            lda         n
            ora         n + 1
            bne         :+
            jmp         again
:
            MOVR        TASK_INBOX + RQ_DONE, n
            clc
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
:
            clc
            rts

; ****************************************************************************
; Keys

; The next key of the loaded window, the terminal's sequences decoded (KEY_*).  OUT: C = 0, .A = it; or C = 1: none
; yet (a sequence part-way in waits for the next call).  Modifies .X, .Y
key_next:
            lda         key_pb
            beq         @byte
            stz         key_pb
            clc
            rts

@byte:
            jsr         iq_get
            bcs         @done
            ldx         esc_st
            bne         @seq
            cmp         #ESC
            bne         @key
            inc         esc_st                              ; (1: ESC)
            bra         @byte

@key:
            clc
@done:
            rts

@seq:
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
            lda         #LF
            jsr         srv_tputc
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
EDIT_N      = 15
edit_keys:  .byte       CR, LF, CTRL_D, BS, DEL, KEY_DEL, KEY_LEFT, KEY_RIGHT, KEY_HOME, CTRL_A, KEY_END, CTRL_E
            .byte       CTRL_U, KEY_UP, KEY_DOWN
edit_vec:   .word       ed_cr, ed_lf, ed_eof, ed_bs, ed_bs, ed_del, ed_left, ed_right, ed_home, ed_home, ed_end, ed_end
            .word       ed_kill, ed_up, ed_down
.assert     * - edit_vec = EDIT_N * 2, error, "edit_keys and edit_vec don't match"
s_clear:    .byte       ESC, "[H", ESC, "[2J", 0            ; (The terminal's screen cleared, the cursor home)

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
; The device
srv_tree:
            SRV_ENTRY   s_root,    $FF, SK_DIR,  0,           SM_READ,            0     ; 0
            SRV_ENTRY   s_cons,    0,   SK_DATA, h_cons,      SM_READ | SM_WRITE, 0     ; 1
            SRV_ENTRY   s_consctl, 0,   SK_CTL,  cons_cmds,   SM_READ | SM_WRITE, 7     ; 2 (reads as 7)
            SRV_ENTRY   s_wctl,    0,   SK_CTL,  wctl_cmds,   SM_READ | SM_WRITE, 8     ; 3 (reads as 8)
            SRV_ENTRY   s_wnew,    0,   SK_DATA, h_wnew,      SM_READ,            0     ; 4
            SRV_ENTRY   s_ser,     0,   SK_DATA, h_ser,       SM_READ | SM_WRITE, 0     ; 5
            SRV_ENTRY   s_serctl,  0,   SK_CTL,  ser_cmds,    SM_READ | SM_WRITE, 9     ; 6 (reads as 9)
            SRV_ENTRY   s_consctl, $FE, SK_TEXT, gen_consctl, SM_READ,            0     ; 7 (the ctl files' states:
            SRV_ENTRY   s_wctl,    $FE, SK_TEXT, gen_wctl,    SM_READ,            0     ; 8   in no directory)
            SRV_ENTRY   s_serctl,  $FE, SK_TEXT, gen_serctl,  SM_READ,            0     ; 9
            .word       0
cons_cmds:
            .word       s_rawon_w, c_rawon
            .word       s_rawoff_w, c_rawoff
            .word       s_group_w, c_group
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
s_rawon_w:  .byte       "rawon", 0
s_rawoff_w: .byte       "rawoff", 0
s_group_w:  .byte       "group", 0
s_new_w:    .byte       "new", 0
s_current_w: .byte      "current", 0
s_rawon:    .byte       "rawon", LF, 0
s_rawoff:   .byte       "rawoff", LF, 0
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
