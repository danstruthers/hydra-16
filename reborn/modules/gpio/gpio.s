; ****************************************************************************
; gpio - the VIA's port A on header J27 (docs/design/reimplementation-from-scratch.md, §14.3 and §14.5), on srvlib: its pins
; and handshake lines as #g, and the I2C bus on two of them (PA0 SCL, PA1 SDA, pulled up on the board) as #i.  One
; driver for both, so their changes to port A never meet (a boot driver: task B).
; #g (at /dev/gpio):
;   /0 ... /7   a pin: reads as 0 or 1 (its level) and an LF; a write of 0 or 1 sets it, the pin an output
;   /port       all 8 pins, a byte (offset 0); a write of a byte sets the outputs' levels (the inputs keep theirs)
;   /ctl        in N, out N (a pin's direction), ddr N (all 8: bit n pin n, 1 out), ca1 rise, ca1 fall (CA1's active
;               edge), ca2 0, ca2 1 (CA2 an output, low or high), ca2 in.  It reads as a line a pin ("2 out 1"), then
;               "ca1 fall 3" (the edge, and the edges counted) and "ca2 in"
;   /ca1        a read waits for CA1's next active edge (one since this fid last read, or opened it), then gives the
;               edges counted so far ("3" and an LF); a non-blocking fd gets E_AGAIN instead.  CA1's interrupt is on
;               while a /ca1 is open (LINE_VIA_CA1, a line of its own: the kernel's VIA stub sends CA1's interrupts
;               there), so a floating CA1 can't flood the system; its edges are counted then.  A switch's bounce
;               counts too
; #i (at /dev/i2c): the I2C bus, its master bit-banged on PA0 and PA1 (as open drain: a line is driven low by making
; its pin an output, its level 0, and let go by making it an input).  Using it takes PA0 and PA1 from #g.
;   /NN         the device at address NN (7 bits, two hex digits: 08-77; the directory lists those that answer): a
;               write sends its bytes to it, a read reads from it (one transaction a request, 64 bytes at most).
;               With "subaddress 1" (or 2), the offset is the device's register: written first (a byte, or two,
;               high first), then the data (a read: the register written, a repeated start, then the read), as a
;               24C02's or a sensor's registers are.  A device that doesn't answer: E_IO
;   /ctl        speed N (kHz, 1-100: the bit-banging's delay; it tops out near 40 kHz at 3.58 MHz), subaddress N (0,
;               1 or 2).  It reads as them
; Each change to port A's registers is a read-modify-write in this task alone; the kernel's IRQ path reads the
; VIA's IFR and IER but never port A.

.include "hydra.inc"
.include "hw.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "srvlib.inc"

            HYX2_DRIVER "gpio", init, srv_serve, irq, 0, HF_BOOT

SRV_OPENED      = opened                                    ; (srvlib: a fid made: a /ca1's counted ...
SRV_CLUNKED     = clunked                                   ;   and forgotten)
SRV_STAT        = stat                                      ;   (/port's length: 1)

SCL             = $01           ; PA0
SDA             = $02           ; PA1
IOBUF           = 64            ; An I2C transaction's data at most
E_PIN           = 1             ; #g's entries: /0 (the pins are 1-8) ...
E_PORT          = 9             ;   /port ...
E_CA1           = 12            ;   and /ca1
PCR_CA1_RISE    = $01           ; PCR: CA1's active edge rising (0: falling)
PCR_CA2         = $0E           ; PCR: CA2's control (3 bits) ...
PCR_CA2_LOW     = $0C           ;   an output, low ...
PCR_CA2_HIGH    = $0E           ;   high
STRETCH         = 255           ; A device's clock stretching waited for at most (loops)

.zeropage
edges:      .res        2                                   ; CA1's edges counted (the irq entry's)
p:          .res        2                                   ; A pointer
t:          .res        1                                   ; Scratch
bits:       .res        1                                   ; A byte being sent or read
addr:       .res        1                                   ; The I2C device's address (7 bits)

.bss
ca1_fids:   .res        1                                   ; /ca1's fids open
delay:      .res        1                                   ; The I2C bus's half-bit delay (loops)
khz:        .res        1                                   ;   (as speed's kHz)
subw:       .res        1                                   ; The register's bytes (subaddress: 0-2)
present:    .res        16                                  ; The addresses that answered (bit n of byte n / 8)
n:          .res        1                                   ; Bytes done
cnt:        .res        1                                   ;   and wanted
iobuf:      .res        IOBUF

.code

; Its init: CA1 falling, CA2 an input; the I2C bus at about 40 kHz, no subaddress; its two device letters
init:
            stz         edges                               ; (Its zero page isn't cleared: its BSS is)
            stz         edges + 1
            lda         VIA_PCR                             ; (CB1, CB2: the upper half, left as they are)
            and         #$F0
            sta         VIA_PCR
            lda         #40
            jsr         speed_set
            stz         subw
            ldx         #0
@letter:
            lda         SRV_TREES,X
            beq         @done
            phx
            jsr         SRV_REGISTER
            plx
            bcs         @failed
            inx
            inx
            inx
            bra         @letter

@done:
            clc
@failed:
            rts

; The irq entry: CA1's active edge (LINE_VIA_CA1): its flag cleared, the edge counted, the readers waiting told.
; Short: about 30 cycles
irq:
            lda         #VIA_IRQ_CA1
            sta         VIA_IFR
            inc         edges
            bne         :+
            inc         edges + 1
:
            inc         TASK_EVENT
            lda         #0
            rts

; ****************************************************************************
; #g

; A pin's file: its number is the entry's (E_PIN + n).  OUT: .A = its bit (1 << n)
pin_bit:
            lda         z:srv_e
            sec
            sbc         #E_PIN
            tax
            lda         bit_of,X
            rts

; /N: a read: its level and an LF (from the offset); a write: 0 or 1 its level, the pin an output
h_pin:
            cmp         #R_READ
            bne         :+
            stz         z:srv_tlen
            jsr         pin_bit
            and         VIA_PORTA_NH
            beq         @level
            lda         #1
@level:
            ora         #'0'
            jsr         srv_tputc
            lda         #LF
            jsr         srv_tputc
            jmp         give_text
:
            cmp         #R_WRITE
            bne         @done
            jsr         first_byte                          ; (.A: the first byte written)
            bcs         @done
            sta         t
            jsr         pin_bit
            ldx         t
            cpx         #'0'
            beq         @low
            cpx         #'1'
            bne         @inval
            tsb         VIA_PORTA_NH
            bra         @out
@low:
            trb         VIA_PORTA_NH
@out:
            tsb         VIA_DDRA
            MOVR        TASK_INBOX + RQ_DONE, TASK_INBOX + RQ_COUNT
@done:
            clc
            rts

@inval:
            lda         #E_INVAL
            sec
            rts

; srv_text (srv_tlen bytes) to the client, from the request's offset (past its end: nothing)
give_text:
            stz         TASK_INBOX + RQ_DONE
            stz         TASK_INBOX + RQ_DONE + 1
            lda         TASK_INBOX + RQ_OFFSET + 1
            ora         TASK_INBOX + RQ_OFFSET + 2
            ora         TASK_INBOX + RQ_OFFSET + 3
            bne         @done
            lda         TASK_INBOX + RQ_OFFSET
            cmp         z:srv_tlen
            bcs         @done
            sec                                             ; r2: what's left, or what's asked if less
            lda         z:srv_tlen
            sbc         TASK_INBOX + RQ_OFFSET
            sta         r2
            stz         r2 + 1
            lda         TASK_INBOX + RQ_COUNT + 1
            bne         :+
            lda         TASK_INBOX + RQ_COUNT
            cmp         r2
            bcs         :+
            sta         r2
:
            clc
            lda         #<srv_text
            adc         TASK_INBOX + RQ_OFFSET
            sta         r0
            lda         #>srv_text
            adc         #0
            sta         r0 + 1
            jsr         srv_toclient
@done:
            clc
            rts

; The write's first byte, into .A (none: C = 1, nothing done)
first_byte:
            stz         TASK_INBOX + RQ_DONE
            stz         TASK_INBOX + RQ_DONE + 1
            lda         TASK_INBOX + RQ_COUNT
            ora         TASK_INBOX + RQ_COUNT + 1
            sec
            beq         @done
            LDR         r0, iobuf
            MOVR        r1, TASK_INBOX + RQ_BUF
            LDR         r2, 1
            jsr         CLIENT_READ
            lda         iobuf
            clc
@done:
            rts

; /port: a read: the pins, a byte (at offset 0; past it, the end); a write: the outputs' levels
h_port:
            cmp         #R_READ
            beq         @read
            cmp         #R_WRITE
            bne         @done
            jsr         first_byte
            bcs         @done
            sta         VIA_PORTA_NH
            MOVR        TASK_INBOX + RQ_DONE, TASK_INBOX + RQ_COUNT
@done:
            clc
            rts

@read:
            stz         TASK_INBOX + RQ_DONE
            stz         TASK_INBOX + RQ_DONE + 1
            lda         TASK_INBOX + RQ_OFFSET
            ora         TASK_INBOX + RQ_OFFSET + 1
            ora         TASK_INBOX + RQ_OFFSET + 2
            ora         TASK_INBOX + RQ_OFFSET + 3
            bne         @done
            lda         VIA_PORTA_NH
            sta         iobuf
            LDR         r0, iobuf
            LDR         r2, 1
            jsr         srv_toclient
            clc
            rts

; /ca1: a read: the next edge (since this fid's last), then the count; none yet: E_AGAIN (the irq entry's event)
h_ca1:
            cmp         #R_READ
            beq         :+
            clc
            rts
:
            lda         edges                               ; (Its low byte: one look, as the irq entry may change
            cmp         srv_fid_aux,X                       ;   it between two)
            bne         :+
            lda         #E_AGAIN
            sec
            rts
:
            sta         srv_fid_aux,X
            stz         z:srv_tlen                          ; The count: its text, from the start
            php
            sei
            lda         edges
            ldx         edges + 1
            plp
            jsr         srv_tputdec
            lda         #LF
            jsr         srv_tputc
            stz         TASK_INBOX + RQ_OFFSET              ; (An event: its text whole, whatever the offset)
            stz         TASK_INBOX + RQ_OFFSET + 1
            stz         TASK_INBOX + RQ_OFFSET + 2
            stz         TASK_INBOX + RQ_OFFSET + 3
            jmp         give_text

; ctl's in N, out N: a pin's direction
c_in:
            jsr         ctl_pin
            bcs         :+
            trb         VIA_DDRA
:
            rts

c_out:
            jsr         ctl_pin
            bcs         :+
            tsb         VIA_DDRA
:
            rts

; The command's pin: its bit in .A.  OUT: C = 0; or C = 1, .A = E_INVAL (none, or past 7)
ctl_pin:
            lda         z:srv_argn
            beq         @inval
            lda         srv_arg + 1
            bne         @inval
            ldx         srv_arg
            cpx         #8
            bcs         @inval
            lda         bit_of,X
            clc
            rts

@inval:
            lda         #E_INVAL
            sec
            rts

; ctl's ddr N: all 8 directions
c_ddr:
            lda         z:srv_argn
            beq         @inval
            lda         srv_arg + 1
            bne         @inval
            lda         srv_arg
            sta         VIA_DDRA
            clc
            rts

@inval:
            lda         #E_INVAL
            sec
            rts

; ctl's ca1 rise, ca1 fall
c_ca1:
            ldx         #<s_rise
            ldy         #>s_rise
            jsr         arg_is
            bne         :+
            lda         #PCR_CA1_RISE
            tsb         VIA_PCR
            clc
            rts
:
            ldx         #<s_fall
            ldy         #>s_fall
            jsr         arg_is
            bne         ctl_inval
            lda         #PCR_CA1_RISE
            trb         VIA_PCR
            clc
            rts

ctl_inval:
            lda         #E_INVAL
            sec
            rts

; ctl's ca2 0, ca2 1, ca2 in
c_ca2:
            ldx         #<s_in
            ldy         #>s_in
            jsr         arg_is
            bne         :+
            lda         #0                                  ; (An input: its negative edge, unused)
            bra         @set
:
            lda         z:srv_argn
            beq         ctl_inval
            lda         srv_arg + 1
            bne         ctl_inval
            lda         srv_arg
            cmp         #2
            bcs         ctl_inval
            tax
            lda         #PCR_CA2_LOW
            cpx         #0
            beq         @set
            lda         #PCR_CA2_HIGH
@set:
            sta         t
            lda         VIA_PCR
            and         #<~PCR_CA2
            ora         t
            sta         VIA_PCR
            clc
            rts

; Is the command's first word after it the string at .X/.Y?  OUT: Z = 1 yes
arg_is:
            stx         p
            sty         p + 1
            lda         z:srv_argn
            beq         @no
            lda         srv_argp
            sta         r3
            lda         srv_argp + 1
            sta         r3 + 1
            ldy         #0
:
            lda         (p),Y
            cmp         (r3),Y
            bne         @no
            cmp         #0
            beq         @yes
            iny
            bra         :-
@yes:
            lda         #0                                  ; (Z = 1)
            rts

@no:
            lda         #1
            rts

; ctl's state: a line a pin ("2 out 1"), "ca1 fall 3", "ca2 in" (or "ca2 0", "ca2 1")
gen_ctl:
            ldy         #0
@pin:
            tya
            ora         #'0'
            jsr         srv_tputc
            lda         #' '
            jsr         srv_tputc
            lda         bit_of,Y
            and         VIA_DDRA
            beq         :+
            lda         #<s_out
            ldx         #>s_out
            bra         :++
:
            lda         #<s_in
            ldx         #>s_in
:
            phy
            jsr         srv_tputs
            ply
            lda         #' '
            jsr         srv_tputc
            lda         bit_of,Y
            and         VIA_PORTA_NH
            beq         :+
            lda         #1
:
            ora         #'0'
            jsr         srv_tputc
            lda         #LF
            jsr         srv_tputc
            iny
            cpy         #8
            bne         @pin
            lda         #<s_ca1_st
            ldx         #>s_ca1_st
            jsr         srv_tputs
            lda         VIA_PCR
            and         #PCR_CA1_RISE
            beq         :+
            lda         #<s_rise
            ldx         #>s_rise
            bra         :++
:
            lda         #<s_fall
            ldx         #>s_fall
:
            jsr         srv_tputs
            lda         #' '
            jsr         srv_tputc
            php
            sei
            lda         edges
            ldx         edges + 1
            plp
            jsr         srv_tputdec
            lda         #LF
            jsr         srv_tputc
            lda         #<s_ca2_st
            ldx         #>s_ca2_st
            jsr         srv_tputs
            lda         VIA_PCR
            and         #PCR_CA2
            cmp         #PCR_CA2_LOW
            bne         :+
            lda         #'0'
            jsr         srv_tputc
            bra         @nl
:
            cmp         #PCR_CA2_HIGH
            bne         :+
            lda         #'1'
            jsr         srv_tputc
            bra         @nl
:
            lda         #<s_in
            ldx         #>s_in
            jsr         srv_tputs
@nl:
            lda         #LF
            jsr         srv_tputc
            clc
            rts

; A fid made (srvlib): a /ca1's (#g's) counted, its edge seen now the count's, and CA1's line owned with the first
; (its interrupt on).  IN: .X = the fid.  Keeps .X
opened:
            jsr         is_ca1
            bne         @done
            lda         edges
            sta         srv_fid_aux,X
            lda         ca1_fids
            bne         :+
            phx
            lda         #LINE_VIA_CA1
            jsr         IRQ_OWN
            plx
            bcs         @refused                            ; (Its error: the open's)
:
            inc         ca1_fids
@done:
            clc
@refused:
            rts

; A fid forgotten (srvlib): a /ca1's count down; with the last, CA1's line given back (its interrupt off)
clunked:
            jsr         is_ca1
            bne         @done
            lda         ca1_fids
            beq         @done
            dec         ca1_fids
            bne         @done
            lda         #LINE_VIA_CA1
            jsr         IRQ_RELEASE
@done:
            clc
            rts

; Is the request's fid a /ca1 of #g?  OUT: Z = 1 yes
is_ca1:
            lda         TASK_INBOX + RQ_DEV
            cmp         #'g'
            bne         :+
            lda         z:srv_e
            cmp         #E_CA1
:
            rts

; A stat record made (srvlib): a pin's length, 2; /port's, 1
stat:
            lda         TASK_INBOX + RQ_DEV
            cmp         #'g'
            bne         @done
            lda         z:srv_e
            cmp         #E_PORT
            bne         :+
            lda         #1
            sta         srv_stat + SR_LENGTH
            rts
:
            cmp         #E_PORT
            bcs         @done
            cmp         #E_PIN
            bcc         @done
            lda         #2
            sta         srv_stat + SR_LENGTH
@done:
            rts

; ****************************************************************************
; #i: the bus, bit-banged.  A line low: its DDR bit set (ORA's bits 0 and 1 are kept 0); let go: its DDR bit clear

; The bus ready: both lines let go, ORA's two bits 0.  A device holding SDA low (a transaction cut short): clocked
; till it lets go (9 clocks at most).  OUT: C = 0; or C = 1, .A = E_IO (SDA held, or SCL)
bus_ready:
            lda         #SCL | SDA
            trb         VIA_DDRA
            trb         VIA_PORTA_NH
            ldx         #9
@check:
            lda         VIA_PORTA_NH
            and         #SDA
            bne         @ok
            jsr         scl_low
            jsr         scl_high
            bcs         @io
            dex
            bne         @check
@io:
            lda         #E_IO
            sec
            rts

@ok:
            clc
            rts

; A start: SDA low while SCL is high, then SCL low.  (A repeated start: SDA let go first, then SCL)
start:
            lda         #SDA
            trb         VIA_DDRA
            jsr         wait
            jsr         scl_high
            bcs         @done
            lda         #SDA
            tsb         VIA_DDRA
            jsr         wait
            jsr         scl_low
            clc
@done:
            rts

; A stop: SDA low, SCL high, then SDA high
stop:
            lda         #SDA
            tsb         VIA_DDRA
            jsr         wait
            jsr         scl_high
            lda         #SDA
            trb         VIA_DDRA
            jmp         wait

; SCL let go, and waited for (a device may hold it low: clock stretching).  OUT: C = 0; or C = 1: held too long
scl_high:
            lda         #SCL
            trb         VIA_DDRA
            ldy         #STRETCH
:
            lda         VIA_PORTA_NH
            and         #SCL
            bne         :+
            dey
            bne         :-
            sec
            rts
:
            jsr         wait
            clc
            rts

; SCL low
scl_low:
            lda         #SCL
            tsb         VIA_DDRA
            jmp         wait

; The half-bit delay
wait:
            ldy         delay
            beq         :++
:
            dey
            bne         :-
:
            rts

; The byte .A sent, MSB first, and the device's ack read.  OUT: C = 0 (acked); or C = 1 (not: .A = E_IO)
send:
            sta         bits
            ldx         #8
@bit:
            lda         #SDA
            asl         bits
            bcc         :+
            trb         VIA_DDRA                            ; (1: let go)
            bra         :++
:
            tsb         VIA_DDRA                            ; (0: low)
:
            jsr         wait
            jsr         scl_high
            bcs         @io
            jsr         scl_low
            dex
            bne         @bit
            lda         #SDA                                ; Its ack: SDA let go, the device pulls it low
            trb         VIA_DDRA
            jsr         wait
            jsr         scl_high
            bcs         @io
            lda         VIA_PORTA_NH
            pha
            jsr         scl_low
            pla
            and         #SDA
            bne         @io
            clc
            rts

@io:
            lda         #E_IO
            sec
            rts

; A byte read, MSB first, into .A; then an ack (C = 0: more to come) or not (C = 1: the last)
receive:
            php
            lda         #SDA                                ; (SDA the device's)
            trb         VIA_DDRA
            ldx         #8
@bit:
            jsr         wait
            jsr         scl_high
            lda         VIA_PORTA_NH
            lsr                                             ; (SDA, bit 1, into C)
            lsr
            rol         bits
            jsr         scl_low
            dex
            bne         @bit
            plp
            lda         #SDA
            bcs         :+                                  ; The last: no ack (SDA let go)
            tsb         VIA_DDRA                            ; More: an ack (SDA low)
:
            jsr         wait
            jsr         scl_high
            jsr         scl_low
            lda         #SDA
            trb         VIA_DDRA
            lda         bits
            rts

; ****************************************************************************
; #i's files

; /NN: a write: its bytes (64 at most a request: the rest, the kernel sends again) to device srv_id, after its
; register (the offset) if subaddress says; a read: as many read from it.  IN: .X = the fid
h_dev:
            ldy         srv_fid_aux,X
            sty         addr
            cmp         #R_WRITE
            beq         @write
            cmp         #R_READ
            beq         @read
            clc
            rts

@write:
            jsr         part                                ; cnt: this request's bytes
            LDR         r0, iobuf                           ; Its bytes, into iobuf
            MOVR        r1, TASK_INBOX + RQ_BUF
            lda         cnt
            sta         r2
            stz         r2 + 1
            jsr         CLIENT_READ
            jsr         bus_ready
            bcs         @io
            jsr         start
            bcs         @io
            lda         addr                                ; The device, to write
            asl
            jsr         send
            bcs         @fail
            jsr         register
            bcs         @fail
            ldy         #0
:
            cpy         cnt
            beq         :+
            lda         iobuf,Y
            phy
            jsr         send
            ply
            bcs         @fail
            iny
            bra         :-
:
            jsr         stop
            lda         cnt
            sta         TASK_INBOX + RQ_DONE
            stz         TASK_INBOX + RQ_DONE + 1
            clc
            rts

@fail:
            pha
            jsr         stop
            pla
@io:
            sec
            rts

@read:
            jsr         part
            jsr         bus_ready
            bcs         @io
            lda         subw                                ; Its register first, if it has one
            beq         @rd
            jsr         start
            bcs         @io
            lda         addr
            asl
            jsr         send
            bcs         @fail
            jsr         register
            bcs         @fail
@rd:
            jsr         start                               ; (Or a repeated start)
            bcs         @io
            lda         addr                                ; The device, to read
            asl
            ora         #1
            jsr         send
            bcs         @fail
            ldy         #0
:
            phy
            iny
            cpy         cnt                                 ; (C = 1: the last)
            jsr         receive
            ply
            sta         iobuf,Y
            iny
            cpy         cnt
            bne         :-
            jsr         stop
            LDR         r0, iobuf
            lda         cnt
            sta         r2
            stz         r2 + 1
            jsr         srv_toclient
            clc
            rts

; cnt: the request's bytes, IOBUF at most
part:
            lda         #IOBUF
            ldx         TASK_INBOX + RQ_COUNT + 1
            bne         :+
            cmp         TASK_INBOX + RQ_COUNT
            bcc         :+
            lda         TASK_INBOX + RQ_COUNT
:
            sta         cnt
            rts

; The register (the offset's low subw bytes, high first), sent.  OUT: as send
register:
            lda         subw
            beq         @done
            cmp         #2
            bcc         :+
            lda         TASK_INBOX + RQ_OFFSET + 1
            jsr         send
            bcs         @fail
:
            lda         TASK_INBOX + RQ_OFFSET
            jmp         send

@done:
            clc
@fail:
            rts

; Is a device at address .A?  (A write of nothing: does it ack its address)  OUT: C = 0 yes
probe:
            sta         addr
            jsr         bus_ready
            bcs         @done
            jsr         start
            bcs         @done
            lda         addr
            asl
            jsr         send
            php
            jsr         stop
            plp
@done:
            rts

; The directory: DYN_NAME, the srv_k-th address that answers (the bus probed at the first: present); DYN_FIND,
; the address at srv_p (two hex digits, 08-77: any, so a device that's there but didn't answer can be tried);
; DYN_IDNAME, address .X's name
h_addrs:
            cmp         #DYN_FIND
            beq         @find
            cmp         #DYN_IDNAME
            beq         @idname
            lda         z:srv_k                             ; DYN_NAME
            bne         :+
            jsr         scan
:
            ldx         #$08
            lda         z:srv_k
            sta         t
@addr:
            cpx         #$78
            bcs         @none
            jsr         is_present
            beq         @next
            lda         t
            beq         @this
            dec         t
@next:
            inx
            bra         @addr

@this:
            jsr         name_of
            txa
            clc
            rts

@none:
            sec
            rts

@idname:
            jmp         name_of

@find:
            ldy         #0
            jsr         hex_digit
            bcs         @noent
            asl
            asl
            asl
            asl
            sta         t
            iny
            jsr         hex_digit
            bcs         @noent
            ora         t
            sta         t
            iny
            lda         (srv_p),Y                           ; (Two digits, then the name's end)
            beq         :+
            cmp         #'/'
            bne         @noent
:
            lda         t
            cmp         #$08
            bcc         @noent
            cmp         #$78
            bcs         @noent
            clc
            rts

@noent:
            sec
            rts

; The hex digit at (srv_p),Y: .A.  OUT: C = 1: not one
hex_digit:
            lda         (srv_p),Y
            sec
            sbc         #'0'
            cmp         #10
            bcc         @done
            sbc         #'a' - '0'
            cmp         #6
            bcs         @no
            adc         #10
@done:
            clc
            rts

@no:
            sec
            rts

; Every address probed: present
scan:
            ldx         #15
:
            stz         present,X
            dex
            bpl         :-
            ldx         #$08
@addr:
            phx
            txa
            jsr         probe
            plx
            bcs         :+
            txa
            lsr
            lsr
            lsr
            tay
            txa
            and         #7
            phx
            tax
            lda         bit_of,X
            plx
            ora         present,Y
            sta         present,Y
:
            inx
            cpx         #$78
            bne         @addr
            rts

; Did address .X answer?  OUT: Z = 0 yes.  Keeps .X
is_present:
            txa
            lsr
            lsr
            lsr
            tay
            txa
            and         #7
            phx
            tax
            lda         bit_of,X
            plx
            and         present,Y
            rts

; Address .X's name ("50") into srv_dname.  Keeps .X
name_of:
            txa
            lsr
            lsr
            lsr
            lsr
            tay
            lda         hex,Y
            sta         srv_dname
            txa
            and         #$0F
            tay
            lda         hex,Y
            sta         srv_dname + 1
            stz         srv_dname + 2
            clc
            rts

; ctl's speed N: kHz (1-100): the half-bit delay, about 450 / N loops less the bit-banging's own time
c_speed:
            lda         z:srv_argn
            beq         @inval
            lda         srv_arg + 1
            bne         @inval
            lda         srv_arg
            beq         @inval
            cmp         #101
            bcs         @inval
            jsr         speed_set
            clc
            rts

@inval:
            lda         #E_INVAL
            sec
            rts

; The speed .A kHz: delay = 358 / .A - 6 loops (a half bit at .A kHz is 1790 / .A cycles; a loop is 5, and the
; bit-banging's own is about 30), at most 255, at least 0; twice the loops at 7.16 MHz
SPEED_K         = 358 * CPU_CLOCK_MULT
speed_set:
            sta         khz
            sta         t
            lda         #<SPEED_K                           ; (p: the dividend, counted down; .X the quotient)
            sta         p
            lda         #>SPEED_K
            sta         p + 1
            ldx         #0
@sub:
            sec
            lda         p
            sbc         t
            sta         p
            lda         p + 1
            sbc         #0
            sta         p + 1
            bcc         @done
            inx
            bne         @sub
            dex                                             ; (255 at most)
@done:
            txa
            sec
            sbc         #6
            bcs         :+
            lda         #0
:
            sta         delay
            rts

; ctl's subaddress N: the register's bytes (0-2)
c_sub:
            lda         z:srv_argn
            beq         @inval
            lda         srv_arg + 1
            bne         @inval
            lda         srv_arg
            cmp         #3
            bcs         @inval
            sta         subw
            clc
            rts

@inval:
            lda         #E_INVAL
            sec
            rts

; ctl's state: "speed 40", "subaddress 0"
gen_i2c:
            lda         #<s_speed
            ldx         #>s_speed
            jsr         srv_tputs
            lda         khz
            ldx         #0
            jsr         srv_tputdec
            lda         #LF
            jsr         srv_tputc
            lda         #<s_subaddr
            ldx         #>s_subaddr
            jsr         srv_tputs
            lda         subw
            ora         #'0'
            jsr         srv_tputc
            lda         #LF
            jsr         srv_tputc
            clc
            rts

.rodata
bit_of:     .byte       $01, $02, $04, $08, $10, $20, $40, $80
hex:        .byte       "0123456789abcdef"

SRV_TREES:
            .byte       'g'
            .word       tree_gpio
            .byte       'i'
            .word       tree_i2c
            .byte       0
tree_gpio:
            SRV_ENTRY   s_slash,   $FF, SK_DIR,  0,         SM_READ,            0      ; 0
            SRV_ENTRY   s_0,       0,   SK_DATA, h_pin,     SM_READ | SM_WRITE, 0      ; 1 (E_PIN): 0
            SRV_ENTRY   s_1,       0,   SK_DATA, h_pin,     SM_READ | SM_WRITE, 0      ; 2
            SRV_ENTRY   s_2,       0,   SK_DATA, h_pin,     SM_READ | SM_WRITE, 0      ; 3
            SRV_ENTRY   s_3,       0,   SK_DATA, h_pin,     SM_READ | SM_WRITE, 0      ; 4
            SRV_ENTRY   s_4,       0,   SK_DATA, h_pin,     SM_READ | SM_WRITE, 0      ; 5
            SRV_ENTRY   s_5,       0,   SK_DATA, h_pin,     SM_READ | SM_WRITE, 0      ; 6
            SRV_ENTRY   s_6,       0,   SK_DATA, h_pin,     SM_READ | SM_WRITE, 0      ; 7
            SRV_ENTRY   s_7,       0,   SK_DATA, h_pin,     SM_READ | SM_WRITE, 0      ; 8
            SRV_ENTRY   s_port,    0,   SK_DATA, h_port,    SM_READ | SM_WRITE, 0      ; 9 (E_PORT)
            SRV_ENTRY   s_ctl,     0,   SK_CTL,  gpio_cmds, SM_READ | SM_WRITE, 11     ; 10 (reads as 11)
            SRV_ENTRY   s_ctl,     $FE, SK_TEXT, gen_ctl,   SM_READ,            0      ; 11 (no directory's)
            SRV_ENTRY   s_ca1,     0,   SK_DATA, h_ca1,     SM_READ,            0      ; 12 (E_CA1)
            .word       0
tree_i2c:
            SRV_ENTRY   s_slash,   $FF, SK_DYN,  h_addrs,   SM_READ,            1      ; 0 (the addresses)
            SRV_ENTRY   s_slash,   SE_TEMPLATE, SK_DATA, h_dev, SM_READ | SM_WRITE, 0  ; 1 (each one)
            SRV_ENTRY   s_ctl,     0,   SK_CTL,  i2c_cmds,  SM_READ | SM_WRITE, 3      ; 2 (reads as 3)
            SRV_ENTRY   s_ctl,     $FE, SK_TEXT, gen_i2c,   SM_READ,            0      ; 3 (no directory's)
            .word       0
gpio_cmds:
            .word       s_in, c_in
            .word       s_out, c_out
            .word       s_ddr, c_ddr
            .word       s_ca1, c_ca1
            .word       s_ca2, c_ca2
            .word       0
i2c_cmds:
            .word       s_speed_w, c_speed
            .word       s_subaddr_w, c_sub
            .word       0
s_slash:    .byte       "/", 0
s_0:        .byte       "0", 0
s_1:        .byte       "1", 0
s_2:        .byte       "2", 0
s_3:        .byte       "3", 0
s_4:        .byte       "4", 0
s_5:        .byte       "5", 0
s_6:        .byte       "6", 0
s_7:        .byte       "7", 0
s_port:     .byte       "port", 0
s_ctl:      .byte       "ctl", 0
s_ca1:      .byte       "ca1", 0
s_ca2:      .byte       "ca2", 0
s_in:       .byte       "in", 0
s_out:      .byte       "out", 0
s_ddr:      .byte       "ddr", 0
s_rise:     .byte       "rise", 0
s_fall:     .byte       "fall", 0
s_speed_w:  .byte       "speed", 0
s_subaddr_w: .byte      "subaddress", 0
s_speed:    .byte       "speed ", 0
s_subaddr:  .byte       "subaddress ", 0
s_ca1_st:   .byte       "ca1 ", 0
s_ca2_st:   .byte       "ca2 ", 0

.include "srvlib.s"
