; ****************************************************************************
; ser - the base's console driver (docs/design/plans/BASE.md, in reborn/docs): the serial port, as bare as a console
; can be, a boot driver in task F.  HydraOS's console (reborn/modules/cons) is the same port's driver with windows on
; top: both are built on the base's serial layer (lib/serial.inc: the rates, the send ring, timer 2's pacing), and
; ser's files are a part of cons's, so a program written for the base runs on HydraOS as it is.  Its device, #c:
;   /cons       the console.  A read gets a line, edited here (cooked): Backspace or Delete takes back a character,
;               Ctrl-U the line; Enter ends it (with an LF; CR, LF, or CR LF: one end); Ctrl-D on an empty line is
;               the end of the input (a read of 0).  Each key's echo goes back to the terminal as it's taken.  Or
;               (raw: consctl's rawon) the bytes as they come, no echo.  A write goes to the terminal, each LF as CR LF
;   /consctl    rawon, rawoff (raw lasts till the last consctl closes, as Plan 9's does); group (the console's notes
;               go to the writer's note group: init's, as it starts).  It reads as its state
;   /ser        the serial port, raw: bytes in and out as they are.  While it's open for reading, every byte in is
;               its, Ctrl-C and Ctrl-\ too (an XMODEM transfer's)
;   /serctl     the rate: b300, b600, b1200, b2400, b4800, b9600, b19200, b115200.  It reads as it
; Ctrl-C sends the console's note group an interrupt note, Ctrl-\ a kill note (the irq entry, as the key comes).
; Receiving: the ACIA's interrupt puts each byte into the receive ring (256 bytes; a byte past it is dropped) and adds
; 1 to the event count (TASK_EVENT: the clients waiting look again).  Sending: into the send ring as it's written,
; as there's room; VIA timer 2 sends the next byte each character's time (serial.inc).

.include "hydra.inc"
.include "hw.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "srvlib.inc"
.include "serial.inc"

            HYX2_DRIVER "ser", init, srv_serve, irq, 0, HF_BOOT

SRV_FLUSH       = flush                                     ; (srvlib: a reader's call ended by a note)
SRV_OPENED      = opened                                    ;   (a fid made: consctl's and /ser's counted)
SRV_CLUNKED     = clunked                                   ;   (a fid forgotten)

LINE_MAX        = 127           ; A line's length at most (and its LF)
IOBUF           = 64            ; A write's or a raw read's bytes, a part at a time
CTRL_C          = $03
CTRL_D          = $04
CTRL_U          = $15
CTRL_BSL        = $1C           ; (Ctrl-\)
BS              = $08
DEL             = $7F
ENT_CONSCTL     = 2             ; The tree's consctl ...
ENT_SER         = 3             ;   and ser

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
grp:        .res        1                                   ; The console's note group (Ctrl-C's)
raw:        .res        1                                   ; <> 0: raw
ctls:       .res        1                                   ; consctl's fids
ser_rd:     .res        1                                   ; /ser's fids for reading (with any, the line is /ser's)
ln_len:     .res        1                                   ; The line being typed: its length ...
ln_ready:   .res        1                                   ;   ended: its length with its LF (0: not yet) ...
ln_off:     .res        1                                   ;   how much of it the reads have had ...
was_cr:     .res        1                                   ;   and <> 0: the last key was CR (an LF after it: the same end)
was_lf:     .res        1                                   ; A write's: $80, each LF as CR LF (/cons's) ...
tx_last:    .res        1                                   ;   and the last byte it sent (an LF after a CR: as it is)
n:          .res        2                                   ; Scratch
cnt:        .res        1

.bss
rx_buf:     .res        256
tx_buf:     .res        256
ln_buf:     .res        LINE_MAX + 1
iobuf:      .res        IOBUF

.code

; ****************************************************************************
; The driver's init: the rings, the lines, the rate, the ACIA's receive interrupt on, the device (C = 1, .A = an
; error: it ends)
init:
            ldx         #cnt - rx_head                      ; (Its zero page: all 0)
:
            stz         rx_head,X
            dex
            bpl         :-
            lda         #INIT_TASK                          ; (init's note group: the console's, as it starts)
            sta         grp
            lda         #LINE_ACIA
            jsr         IRQ_OWN
            bcs         @done
            lda         #LINE_VIA_T2
            jsr         IRQ_OWN
            bcs         @done
            SER_ON                                          ; (serial.inc's: 9600, the ACIA's receive interrupt on)
            lda         #'c'
            jmp         SRV_REGISTER                        ; (Its error is init's)

@done:
            rts

; ****************************************************************************
; The irq entry: .A = the line.  A byte in: Ctrl-C and Ctrl-\ the notes (but /ser's), the rest into the ring.  Timer
; 2: the next byte out (serial.inc's t2_next)
irq:
            cmp         #LINE_VIA_T2
            beq         t2_next
            lda         ACIA_STATUS                         ; (Reading it clears its interrupt)
            and         #ACIA_ST_RDRF
            beq         @none
            lda         ACIA_DATA
            ldx         ser_rd                              ; (/ser's: every byte as it is)
            bne         @store
            cmp         #CTRL_C
            beq         @intr
            cmp         #CTRL_BSL
            beq         @kill
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

@intr:                                                      ; The notes, to the console's group
            lda         #1 << (NOTE_INTERRUPT - 1)
            bra         @note

@kill:
            lda         #1 << (NOTE_KILL - 1)
@note:
            ldx         grp
            jsr         NOTE_QUEUE
            lda         #0
            rts

            SER_T2_NEXT

; Nothing more to send: timer 2 stops, and the writers waiting for room look again
t2_idle:
            stz         tx_busy
            inc         TASK_EVENT
            lda         VIA_T2CL                            ; (Its interrupt cleared)
            lda         #0
            rts

; ****************************************************************************
; The rings (serial.inc's): tx_start, tx_put, tx_free, rx_get

            SER_TX
            SER_RX_GET

; Not yet: the client waits for the event count to change (a key in, room to send)
again:
            lda         #E_AGAIN
            sec
            rts

; ****************************************************************************
; The fids: consctl's and /ser's readers counted; with consctl's last, raw ends.  IN: .X = the fid
opened:
            lda         z:srv_e
            cmp         #ENT_CONSCTL
            bne         :+
            inc         ctls
:
            clc
            rts

clunked:
            lda         z:srv_e
            cmp         #ENT_CONSCTL
            bne         @done
            lda         ctls
            beq         @done
            dec         ctls
            bne         @done
            stz         raw                                 ; (The last: cooked again)
@done:
            clc
            rts

; A reader's call ended by a note (R_FLUSH): the line being typed, gone
flush:
            lda         ln_ready
            bne         :+
            stz         ln_len
:
            clc
            rts

; ****************************************************************************
; /cons

h_cons:
            cmp         #R_READ
            beq         r_cons
            cmp         #R_WRITE
            bne         :+
            jmp         w_cons
:
            clc
            rts

; A read: raw, the bytes there are; cooked, the line (the keys taken, and echoed, till Enter)
r_cons:
            lda         raw
            beq         @cooked
            jmp         r_raw

@cooked:
            lda         ln_ready
            beq         @key
            jmp         @give
@key:
            jsr         tx_free                             ; (Room for a key's echo: 3 bytes at most)
            cmp         #3
            bcs         :+
            jmp         @wait
:
            jsr         rx_get
            bcc         :+
            jmp         @wait
:
            ldx         was_cr
            stz         was_cr
            cmp         #CR
            beq         @cr
            cmp         #LF
            beq         @lf
            cmp         #BS
            beq         @erase
            cmp         #DEL
            beq         @erase
            cmp         #CTRL_U
            beq         @kill
            cmp         #CTRL_D
            beq         @eof
            cmp         #' '                                ; (Another control: nothing; but Tab)
            bcs         :+
            cmp         #TAB
            bne         @key
:
            ldy         ln_len                              ; (A full line: nothing)
            cpy         #LINE_MAX
            bcs         @key
            sta         ln_buf,Y
            inc         ln_len
            jsr         tx_put                              ; Its echo
            bra         @key

@lf:
            cpx         #0                                  ; (CR LF: one end)
            bne         @key
            bra         @end
@cr:
            inc         was_cr
@end:
            ldy         ln_len                              ; The line ends: its LF; CR LF echoed
            lda         #LF
            sta         ln_buf,Y
            iny
            sty         ln_ready
            stz         ln_off
            lda         #CR
            jsr         tx_put
            lda         #LF
            jsr         tx_put
            jsr         tx_start
            bra         @give

@erase:
            lda         ln_len                              ; (Nothing to take back: nothing)
            beq         @key
            dec         ln_len
            lda         #BS
            jsr         tx_put
            lda         #' '
            jsr         tx_put
            lda         #BS
            jsr         tx_put
            bra         @key

@kill:
            lda         ln_len
            bne         :+
            jmp         @key
:
            jsr         tx_free
            cmp         #3
            bcs         :+
            jmp         @wait                               ; (Ctrl-U again, once there's room: the rest)
:
            dec         ln_len
            lda         #BS
            jsr         tx_put
            lda         #' '
            jsr         tx_put
            lda         #BS
            jsr         tx_put
            bra         @kill

@eof:
            lda         ln_len                              ; (On an empty line: the end of the input)
            beq         :+
            jmp         @key
:
            jsr         tx_start
            stz         TASK_INBOX + RQ_DONE
            stz         TASK_INBOX + RQ_DONE + 1
            clc
            rts

@wait:
            jsr         tx_start
            jmp         again

@give:                                                      ; The line, as much as is asked: the rest the next read's
            sec
            lda         ln_ready
            sbc         ln_off
            ldx         TASK_INBOX + RQ_COUNT + 1
            bne         :+
            cmp         TASK_INBOX + RQ_COUNT
            bcc         :+
            lda         TASK_INBOX + RQ_COUNT
:
            sta         r2
            stz         r2 + 1
            clc
            lda         #<ln_buf
            adc         ln_off
            sta         r0
            lda         #>ln_buf
            adc         #0
            sta         r0 + 1
            MOVR        r1, TASK_INBOX + RQ_BUF
            jsr         CLIENT_WRITE
            lda         r2
            sta         TASK_INBOX + RQ_DONE
            stz         TASK_INBOX + RQ_DONE + 1
            clc
            adc         ln_off
            sta         ln_off
            cmp         ln_ready
            bcc         :+
            stz         ln_len                              ; (All of it read: the next line)
            stz         ln_ready
:
            clc
            rts

; A raw read (/cons's, /ser's): the bytes in the ring, as many as are asked (IOBUF at most); or E_AGAIN
r_raw:
            ldx         #0
@byte:
            cpx         TASK_INBOX + RQ_COUNT
            bne         :+
            lda         TASK_INBOX + RQ_COUNT + 1
            beq         @out
:
            cpx         #IOBUF
            bcs         @out
            phx
            jsr         rx_get
            plx
            bcs         @out
            sta         iobuf,X
            inx
            bra         @byte

@out:
            txa
            bne         :+
            jmp         again
:
            sta         r2
            stz         r2 + 1
            LDR         r0, iobuf
            MOVR        r1, TASK_INBOX + RQ_BUF
            jsr         CLIENT_WRITE
            lda         r2
            sta         TASK_INBOX + RQ_DONE
            stz         TASK_INBOX + RQ_DONE + 1
            clc
            rts

; A write to /cons: each LF as CR LF (but one after a CR), as there's room; the rest waits (E_AGAIN, with nothing taken yet)
w_cons:
            lda         #$80
            bra         write

; A write to /ser: the bytes as they are
s_write:
            lda         #0
write:
            sta         was_lf
            stz         n
            stz         n + 1
@part:
            sec                                             ; The bytes left ...
            lda         TASK_INBOX + RQ_COUNT
            sbc         n
            sta         r3
            lda         TASK_INBOX + RQ_COUNT + 1
            sbc         n + 1
            sta         r3 + 1
            ora         r3
            beq         @end
            jsr         tx_free                             ;   as many as there's room for (half, LF as CR LF),
            bit         was_lf                              ;   IOBUF at most
            bpl         :+
            lsr
:
            cmp         #IOBUF
            bcc         :+
            lda         #IOBUF
:
            ldx         r3 + 1
            bne         :+
            cmp         r3
            bcc         :+
            lda         r3
:
            sta         cnt
            cmp         #0
            beq         @end
            sta         r2                                  ; From the client's buffer + n
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
            bit         was_lf
            bpl         @put
            cmp         #LF
            bne         @put
            ldy         tx_last                             ; (After a CR: the LF alone)
            cpy         #CR
            beq         @put
            lda         #CR
            jsr         tx_put
            lda         #LF
@put:
            sta         tx_last
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
            jsr         tx_start
            jmp         @part

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

; ****************************************************************************
; /ser: raw reads and writes; its fids for reading counted (with any, the line is its)
h_ser:
            cmp         #R_READ
            bne         :+
            jmp         r_raw
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

c_group:                                                    ; The writer's note group: Ctrl-C's
            lda         TASK_INBOX + RQ_GROUP
            sta         grp
            clc
            rts

; consctl's state: "rawon" or "rawoff", then "group N"
gen_consctl:
            lda         #<s_rawon
            ldx         #>s_rawon
            ldy         raw
            bne         :+
            lda         #<s_rawoff
            ldx         #>s_rawoff
:
            jsr         srv_tputs
            lda         #LF
            jsr         srv_tputc
            lda         #<s_group
            ldx         #>s_group
            jsr         srv_tputs
            lda         #' '
            jsr         srv_tputc
            lda         grp
            ldx         #0
            jsr         srv_tputdec
            lda         #LF
            jsr         srv_tputc
            clc
            rts

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
            SER_RATE_SET                                    ; (serial.inc's: the ACIA, and rate_t2)

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
; The rates (serial.inc's), and their names
            SER_RATES
rate_name_lo: .byte     <s_300, <s_600, <s_1200, <s_2400, <s_4800, <s_9600, <s_19200, <s_115200
rate_name_hi: .byte     >s_300, >s_600, >s_1200, >s_2400, >s_4800, >s_9600, >s_19200, >s_115200

; The tree: name, parent (entry), kind, handler, mode, aux (a ctl file's: the entry it reads as)
srv_tree:
            SRV_ENTRY   s_root,    $FF, SK_DIR,  0,           SM_READ,            0     ; 0
            SRV_ENTRY   s_cons,    0,   SK_DATA, h_cons,      SM_READ | SM_WRITE, 0     ; 1
            SRV_ENTRY   s_consctl, 0,   SK_CTL,  cons_cmds,   SM_READ | SM_WRITE, 5     ; 2 (reads as 5: ENT_CONSCTL)
            SRV_ENTRY   s_ser,     0,   SK_DATA, h_ser,       SM_READ | SM_WRITE, 0     ; 3
            SRV_ENTRY   s_serctl,  0,   SK_CTL,  ser_cmds,    SM_READ | SM_WRITE, 6     ; 4 (reads as 6)
            SRV_ENTRY   s_consctl, $FE, SK_TEXT, gen_consctl, SM_READ,            0     ; 5 (the ctl files' states:
            SRV_ENTRY   s_serctl,  $FE, SK_TEXT, gen_serctl,  SM_READ,            0     ; 6   in no directory)
            .word       0
cons_cmds:
            .word       s_rawon, c_rawon
            .word       s_rawoff, c_rawoff
            .word       s_group, c_group
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
s_rawon:    .byte       "rawon", 0
s_rawoff:   .byte       "rawoff", 0
s_group:    .byte       "group", 0
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
