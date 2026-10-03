; ****************************************************************************
; cons - the console driver (docs/reimplementation-from-scratch.md, §14.2): the serial port, its rings, and the
; device #c, on srvlib (a boot driver: task F).
;   /cons       the console.  A read is the foreground note group's (the others' wait: consctl's fg), and gets a
;               line, edited here (cooked): Backspace and Delete, Left, Right, Home and End (and Ctrl-A, Ctrl-E),
;               Ctrl-U, the history with Up and Down; Enter ends it, Ctrl-D on an empty line is the end of the
;               input.  Or (raw: consctl's rawon) each key as it comes, the terminal's cursor and function keys as
;               one code each (KEY_*; an Escape alone waits for the key after it).  A write goes out with each LF
;               as CR LF
;   /consctl    rawon, rawoff; fg N (note group N's reads go on, the others' wait).  It reads as the state
;   /ser        the serial port, raw: bytes in and out as they are
;   /serctl     the rate: b300, b600, b1200, b2400, b4800, b9600, b19200, b115200.  It reads as it
; Ctrl-C and Ctrl-\ are the foreground group's notes (interrupt, kill), in either mode: a raw program that wants
; them as keys catches the notes.
;
; Receiving: the ACIA's interrupt (LINE_ACIA) puts each byte into the receive ring and adds 1 to the event count
; (TASK_EVENT: the readers waiting on it look again).  Sending: the serve entry puts bytes into the send ring and
; sends the first; VIA timer 2 (LINE_VIA_T2) runs a character's time and a margin, and its interrupt sends the
; next.  Paced, at every rate, on both chips: the WDC W65C51N's TDRE doesn't work, and on the board the
; Rockwell's sending back to back at 115200 loses characters (2 idle bits then; 1 otherwise).  The interrupts'
; work is a few dozen cycles each: the IRQs-off budget (200 cycles) has the dispatch's 115 in it.

.include "hydra.inc"
.include "hw.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "srvlib.inc"

            HYX2_DRIVER "cons", init, srv_serve, irq, 0, HF_BOOT

SRV_FLUSH       = flush                                     ; (srvlib: a reader's call ended by a note)

LINE_MAX        = 127           ; A line's length at most (and its LF)
HIST_N          = 8             ; Lines in the history ...
HIST_SIZE       = 128           ;   each its length, then LINE_MAX characters
ECHO_ROOM       = LINE_MAX + 13 ; The most a key's echo puts into the send ring (a key waits for this much room)
IOBUF           = 64            ; A write's bytes, a part at a time
CTRL_A          = $01
CTRL_C          = $03
CTRL_D          = $04
CTRL_E          = $05
BS              = $08
CTRL_U          = $15
ESC             = $1B
CTRL_BSL        = $1C           ; (Ctrl-\)
DEL             = $7F
RATE_BOOT       = 5             ; 9600: the kernel's bring-up console's

.zeropage
rx_head:    .res        1                                   ; The receive ring: the irq entry's end ...
rx_tail:    .res        1                                   ;   and the readers'
tx_head:    .res        1                                   ; The send ring: the writers' end ...
tx_tail:    .res        1                                   ;   and timer 2's
tx_busy:    .res        1                                   ; <> 0: a byte is going (timer 2 runs)
t2_lo:      .res        1                                   ; Timer 2 for a character: a round's count ...
t2_hi:      .res        1
t2_rounds:  .res        1                                   ;   the rounds (more than 1 at slow rates) ...
t2_left:    .res        1                                   ;   and those left of this one
fg:         .res        1                                   ; The foreground note group
raw:        .res        1                                   ; <> 0: raw
rate:       .res        1                                   ; The rate (its index in the tables)
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
xlate:      .res        1                                   ; A write: <> 0: each LF as CR LF
n:          .res        2                                   ; Scratch
p:          .res        2
cnt:        .res        1

.bss
rx_buf:     .res        256
tx_buf:     .res        256
ln_buf:     .res        LINE_MAX + 1
hist:       .res        HIST_N * HIST_SIZE
iobuf:      .res        IOBUF

.code
; ****************************************************************************
; The driver's init: its lines, the rate, the ACIA's receive interrupt on, the device
init:
            ldx         #cnt - rx_head                      ; (Its zero page: all 0)
:
            stz         rx_head,X
            dex
            bpl         :-
            lda         #INIT_TASK                          ; The foreground: init's group
            sta         fg
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
            cmp         #CTRL_C
            beq         @intr
            cmp         #CTRL_BSL
            beq         @kill
            ldy         rx_head                             ; Into the ring
            sta         rx_buf,Y
            iny
            cpy         rx_tail
            beq         @none                               ; (Full: the byte's dropped)
            sty         rx_head
            inc         TASK_EVENT                          ; (The readers look again)
@none:
            lda         #0
            rts

@intr:                                                      ; The foreground's notes
            lda         #1 << (NOTE_INTERRUPT - 1)
            bra         @note

@kill:
            lda         #1 << (NOTE_KILL - 1)
@note:
            ldx         fg
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
; The send ring

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

; .A into the ring (the caller has made sure of the room).  Keeps .A, .X
tx_put:
            ldy         tx_head
            sta         tx_buf,Y
            iny
            sty         tx_head
            rts

; .A = the room in the ring
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
; The files

h_cons:
            cmp         #R_READ
            bne         :+
            jmp         r_cons
:
            cmp         #R_WRITE
            bne         :+
            lda         #1                                  ; (Each LF as CR LF)
            jmp         write
:
            clc                                             ; (Opens and clunks: nothing to do)
            rts

h_ser:
            cmp         #R_READ
            bne         :+
            jmp         r_ser
:
            cmp         #R_WRITE
            bne         :+
            lda         #0
            jmp         write
:
            clc
            rts

; /cons: a read.  The foreground group's only; cooked, a line (or what's left of one); raw, the keys there are
r_cons:
            lda         TASK_INBOX + RQ_GROUP
            cmp         fg
            bne         again
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

; Not yet: the client waits for the event count to change (a byte in, room to send, the foreground changed)
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

; /cons, /ser: a write, as much of it as the send ring takes (the kernel sends the rest again); none: E_AGAIN.
; IN: .A <> 0: each LF as CR LF
write:
            sta         xlate
            stz         n                                   ; (n: the count done)
            stz         n + 1
@part:
            sec                                             ; p: the count left
            lda         TASK_INBOX + RQ_COUNT
            sbc         n
            sta         p
            lda         TASK_INBOX + RQ_COUNT + 1
            sbc         n + 1
            sta         p + 1
            ora         p
            beq         @end
            jsr         tx_free                             ; This part: the room (half, if an LF takes 2) ...
            ldx         xlate
            beq         :+
            lsr
:
            cmp         #IOBUF                              ;   IOBUF at most ...
            bcc         :+
            lda         #IOBUF
:
            ldx         p + 1                               ;   and what's left, if less
            bne         :+
            cmp         p
            bcc         :+
            lda         p
:
            sta         cnt
            cmp         #0
            beq         @end                                ; (No room)
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
            jsr         CLIENT_READ
            ldx         #0
@byte:
            lda         iobuf,X
            cmp         #LF
            bne         @put
            ldy         xlate
            beq         @put
            lda         #CR
            jsr         tx_put
            lda         #LF
@put:
            jsr         tx_put
            inx
            cpx         cnt
            bne         @byte
            clc
            lda         n
            adc         cnt
            sta         n
            bcc         :+
            inc         n + 1
:
            jsr         tx_start                            ; (Going while the next part comes)
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

; A reader's call ended by a note (R_FLUSH): the line it was typing, gone
flush:
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

; The next key, the terminal's sequences decoded (KEY_*).  OUT: C = 0, .A = it; or C = 1: none yet (a sequence
; part-way in waits for the next call).  Modifies .X, .Y
key_next:
            lda         key_pb
            beq         @byte
            stz         key_pb
            clc
            rts

@byte:
            jsr         rx_get
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
; The line editor (cooked)

; Keys into the line, echoed, while there are keys and the send ring has room for a key's echo.  OUT: C = 0: a
; line ended (ln_ready), or the input did (eof); C = 1: not yet
edit:
            jsr         tx_free
            cmp         #ECHO_ROOM
            bcc         @wait
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
            bcc         @next
            cmp         #DEL
            bcs         @next
            jsr         ed_insert
@next:
            jsr         tx_start
            bra         edit

@special:
            txa
            asl
            tax
            jsr         @go
            jsr         tx_start
            bcc         edit                                ; (C = 1: the line, or the input, ended)
            clc
            rts

@go:
            jmp         (edit_vec,X)

@wait:
            jsr         tx_start
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
            lda         #CR
            jsr         tx_put
            lda         #LF
            jsr         tx_put
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
            jsr         tx_put
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
            jsr         tx_put
            clc
            rts

ed_right:
            ldx         ln_pos
            cpx         ln_len
            bcs         ed_none
            lda         ln_buf,X
            jsr         tx_put
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
; The history

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

; p = the slot of the line .A back (1: the newest)
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
            rts

.assert     HIST_SIZE = 128, error, "hist_slot: a slot is 128 bytes"
.assert     HIST_SIZE > LINE_MAX, error, "A history slot holds a line"

; ****************************************************************************
; Echo: into the send ring (edit has made sure of the room)

; The line from the cursor to its end
echo_rest:
            ldx         ln_pos
:
            cpx         ln_len
            bcs         :+
            lda         ln_buf,X
            jsr         tx_put
            inx
            bra         :-
:
            rts

; The line erased from the cursor: ESC [ K
echo_erase:
            lda         #ESC
            jsr         tx_put
            lda         #'['
            jsr         tx_put
            lda         #'K'
            jmp         tx_put

; ESC [ .A .X (the cursor moved .A places: .X = 'C' right, 'D' left); nothing if .A = 0
echo_csi:
            cmp         #0
            beq         @done
            pha
            lda         #ESC
            jsr         tx_put
            lda         #'['
            jsr         tx_put
            pla
            jsr         echo_dec
            txa
            jmp         tx_put

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
            jsr         tx_put
            pla
            inc         cnt
@none:
            rts

; ****************************************************************************
; consctl and serctl

c_rawon:
            lda         #1
            sta         raw
            clc
            rts

c_rawoff:
            stz         raw
            clc
            rts

; fg N: note group N's reads go on (the others' wait)
c_fg:
            lda         z:srv_argn
            beq         @inval
            lda         srv_arg + 1
            bne         @inval
            lda         srv_arg
            cmp         #16
            bcs         @inval
            sta         fg
            inc         TASK_EVENT                          ; (The readers waiting look again)
            clc
            rts

@inval:
            lda         #E_INVAL
            sec
            rts

; consctl's state: "rawon" or "rawoff", and "fg N"
gen_consctl:
            lda         #<s_rawon
            ldx         #>s_rawon
            ldy         raw
            bne         :+
            lda         #<s_rawoff
            ldx         #>s_rawoff
:
            jsr         srv_tputs
            lda         #<s_fg
            ldx         #>s_fg
            jsr         srv_tputs
            lda         fg
            ldx         #0
            jsr         srv_tputdec
            lda         #LF
            jsr         srv_tputc
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
            SRV_ENTRY   s_consctl, 0,   SK_CTL,  cons_cmds,   SM_READ | SM_WRITE, 5     ; 2 (reads as 5)
            SRV_ENTRY   s_ser,     0,   SK_DATA, h_ser,       SM_READ | SM_WRITE, 0     ; 3
            SRV_ENTRY   s_serctl,  0,   SK_CTL,  ser_cmds,    SM_READ | SM_WRITE, 6     ; 4 (reads as 6)
            SRV_ENTRY   s_consctl, $FE, SK_TEXT, gen_consctl, SM_READ,            0     ; 5 (consctl's state: in no
            SRV_ENTRY   s_serctl,  $FE, SK_TEXT, gen_serctl,  SM_READ,            0     ; 6   directory)
            .word       0
cons_cmds:
            .word       s_rawon_w, c_rawon
            .word       s_rawoff_w, c_rawoff
            .word       s_fg_w, c_fg
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
s_ser:      .byte       "ser", 0
s_serctl:   .byte       "serctl", 0
s_rawon_w:  .byte       "rawon", 0
s_rawoff_w: .byte       "rawoff", 0
s_fg_w:     .byte       "fg", 0
s_rawon:    .byte       "rawon", LF, 0
s_rawoff:   .byte       "rawoff", LF, 0
s_fg:       .byte       "fg ", 0
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
