.debuginfo

; ****************************************************************************
; /pc: a folder on the PC, served by the PC tool (sim/tools/hydrapc.js, which is the console's terminal too) over
; the serial port (BIOS ROM page D; included inside `.scope PAGED`, see all.s; docs/plans/PC.md).  The device pc
; runs in the serial task, which has the frames' buffers (PC_TXBUF, PC_RXBUF); the fast handler (serfast.s) moves
; their bytes, between the console's.  The frames: include/io.inc (PC_*), and sim/lib/pcproto.js for the PC's side.
;   A request goes out as a frame (PC_T_REQ: the request block's first PC_REQ_HDR bytes, then its data: an open's,
; a create's or a remove's name, a write's bytes, a wstat's record), and the client waits (ERR_IO_WOULD_BLOCK, and a
; sleeper until its reply is due, PC_UNTIL); the reply's frame wakes it (serfast.s), and when the IO layer offers
; the request again, the reply (PC_T_REPLY: status, value, count, data) is its answer.  So every request the PC
; tool can answer works as HydraFS's would: the PC tool does the work (sim/tools/pcfs.js).
;   One request at a time: a client that finds another's out tries again 2 ticks on (a sleeper), or takes it over
; if that one is PC_STALE ticks past its time (its client was killed, say).
;   A reply that comes in damaged (its CRC), or the PC's PC_T_NAK (the request came damaged), or no reply by its
; time: the request again (the PC tool answers a repeated tag with its last reply, not doing it twice), PC_TRIES_MAX
; tries in all; then the PC is taken as gone (ERR_IO_DEVICE).  The first request, or the first after that, attaches
; first (PC_T_ATTACH: the PC tool forgets the files it had open, a new session): no answer in PC_WAIT_ATTACH ticks,
; and it's ERR_IO_DEVICE (no PC tool: the attach's 7 bytes show on the terminal).
; Server ZP: PC_* (zero.s: in the serial task, with the fast handler's).

PC_STEP_ATTACH  = 1                                     ; PC_STEP: the attach is out
PC_STEP_REQ     = 2                                     ;   the request is out
PC_TRIES_MAX    = 3
PC_WAIT_REQ     = SCHED_TICK_HZ * 2                     ; A reply's time (ticks): the TX ring's bytes first, then
PC_WAIT_ATTACH  = SCHED_TICK_HZ                         ;   the request's and the reply's (256 bytes: 0.27 s each
PC_STALE        = SCHED_TICK_HZ                         ;   at 9600 baud)
PC_NAP_TICKS    = 2                                     ; Another's request is out: try again this many ticks on

.segment "PC_PD"

; The serial driver's init (SERIAL_INIT, in the serial task, IRQs off): nothing going on, not attached; the device
PC_INIT:
            ldx         #SER_PEND_KEY - PC_ZP_LOW - 1
:
            stz         PC_ZP_LOW,X
            dex
            bpl         :-
            dec         PC_OWNER                            ; ($FF: no request out)
            LOAD_ADDR   ::PC_SERVE, ZP_TC_VEC               ; (Page 0's gate: serial.s)
            lda         #<PC_NAME
            ldy         #>PC_NAME
            ldx         #SERIAL_TASK_NUM
            jmp         DEV_REGISTER

PC_NAME:        .byte   "pc", 0
PC_BITS:        .byte   $01, $02, $04, $08, $10, $20, $40, $80

; A request.  IN: .A = request, .X = client, .Y = fid.  OUT: C = 0, .A = the value (an open's fid); or C = 1, .A =
; an error (ERR_IO_WOULD_BLOCK: the IO layer offers it again when the client is woken)
PC_SERVE:
            stx         PC_CLIENT
            lda         PC_OWNER
            bmi         @take                               ; No request out
            cpx         PC_OWNER
            beq         @mine
            lda         #PC_STALE                           ; Another's: wait for it, unless it's long past its
            jsr         PC_LATE                             ;   time (its client was killed, say)
            bcs         @take
            lda         #PC_NAP_TICKS
            ldy         #0
            jsr         PC_FROM_NOW
            jmp         PC_NAP

@take:
            stz         PC_STEP
            stx         PC_OWNER
            stz         PC_RXF                              ; (A frame for the last one's)

@mine:
            lda         PC_STEP
            cmp         #PC_STEP_REQ
            bne         :+
            jsr         PC_SAME                             ; Its request out, or another (it gave that one up:
            bcs         :+                                  ;   Ctrl-C, say)?
            stz         PC_STEP                             ; Another: this one, from the start
            stz         PC_RXF
:
            lda         PC_STEP
            bne         PC_WAITING
            lda         PC_TXS                              ; A frame still going out (a request given up)?
            ora         PC_TXE                              ;   Not over it: a tick on
            beq         :+
            lda         #1
            ldy         #0
            jsr         PC_FROM_NOW
            jmp         PC_NAP
:
            lda         PC_ONLINE
            bne         PC_REQUEST
            lda         #PC_STEP_ATTACH                     ; Attach first
            sta         PC_STEP
            lda         #1
            sta         PC_TRIES
            lda         #PC_T_ATTACH
            sta         PC_TXBUF
            lda         PC_TAG
            sta         PC_TXBUF + 1
            lda         #1
            sta         PC_TXBUF + 2
            stz         PC_TXBUF + 3
            lda         #PC_VERSION
            sta         PC_TXBUF + 4
            lda         #1
            ldy         #0
            jsr         PC_FRAME_SEND
            lda         #<PC_WAIT_ATTACH
            ldy         #>PC_WAIT_ATTACH
            bra         PC_SENT

; The request: out as a frame, with a new tag
PC_REQUEST:
            lda         #PC_STEP_REQ
            sta         PC_STEP
            lda         #PC_TRIES_MAX
            sta         PC_TRIES
            inc         PC_TAG
            jsr         PC_SEND_REQ

PC_RESENT:
            lda         #<PC_WAIT_REQ
            ldy         #>PC_WAIT_REQ

PC_SENT:                                                    ; Its reply is due .A.Y ticks from now
            jsr         PC_FROM_NOW
            sta         PC_UNTIL
            sty         PC_UNTIL + 1

PC_WAIT:                                                    ; The client waits for it
            lda         PC_UNTIL
            ldy         PC_UNTIL + 1
            jmp         PC_NAP

; The request is out: its reply, or its time
PC_WAITING:
            lda         PC_RXF
            bne         @reply
            lda         #0
            jsr         PC_LATE
            bcc         PC_WAIT                             ; Woken early: wait on
            bra         PC_AGAIN                            ; Its time has come: again

@reply:
            jsr         PC_CHECK
            bcc         @good
            stz         PC_RXF
            tax
            beq         PC_WAIT                             ; Not its reply (an older request's): wait on
            bra         PC_AGAIN                            ; Damaged: again

@good:
            cmp         #PC_T_REPLY
            beq         :+
            stz         PC_RXF                              ; PC_T_NAK, or another: again
            bra         PC_AGAIN
:
            lda         PC_STEP
            cmp         #PC_STEP_ATTACH
            bne         PC_ANSWER
            stz         PC_RXF                              ; Attached: now the request
            lda         #1
            sta         PC_ONLINE
            bra         PC_REQUEST

; Once more, if it has tries left; else the PC is gone
PC_AGAIN:
            dec         PC_TRIES
            beq         @gone
            jsr         PC_RESEND
            bra         PC_RESENT

@gone:
            stz         PC_ONLINE
            lda         #ERR_IO_DEVICE
            bra         PC_ERROR

; The reply: its status, its count (a read's or a write's) and data into the client's request block
PC_ANSWER:
            lda         PC_RXBUF + 4                        ; (Its status)
            bne         PC_ERROR
            ldx         PC_CLIENT
            jsr         IO_SRV_MAP
            ldy         #IO_BLK_TYPE
            lda         (ZP_IO_REQ),Y
            cmp         #H9_READ
            beq         :+
            cmp         #H9_WRITE
            bne         @data
:
            ldy         #IO_BLK_COUNT
            lda         PC_RXBUF + 6
            sta         (ZP_IO_REQ),Y
            iny
            lda         PC_RXBUF + 7
            sta         (ZP_IO_REQ),Y

@data:
            lda         PC_RXBUF + 2                        ; The data: the payload after its PC_REPLY_HDR bytes
            sec                                             ;   (0-256)
            sbc         #PC_REPLY_HDR
            sta         PC_N
            lda         PC_RXBUF + 3
            sbc         #0
            ora         PC_N
            beq         @copied
            inc         ZP_IO_REQ + 1                       ; The data area
            ldy         #0
:
            lda         PC_RXBUF + 4 + PC_REPLY_HDR,Y
            sta         (ZP_IO_REQ),Y
            iny
            cpy         PC_N
            bne         :-
            dec         ZP_IO_REQ + 1

@copied:
            jsr         IO_SRV_UNMAP
            lda         PC_RXBUF + 5                        ; (Its value)
            jsr         PC_RELEASE
            clc
            rts

; Done, with an error.  IN: .A = the error
PC_ERROR:
            jsr         PC_RELEASE
            sec
            rts

; The request is done: the next may go out.  Preserves .A
PC_RELEASE:
            pha
            stz         PC_STEP
            lda         #$FF
            sta         PC_OWNER                            ; (First: then no frame sets PC_RXF again)
            stz         PC_RXF
            jsr         PC_UNNAP
            pla
            rts

; The client sleeps until tick .A.Y (as TASK_SLEEP_UNTIL's: the tick wakes it; or a frame coming in does, for the
; client whose request is out), and the IO layer offers the request again then.  OUT: C = 1, .A = ERR_IO_WOULD_BLOCK
PC_NAP:
            php
            sei
            ldx         PC_CLIENT
            stx         T_REGISTER                          ; Quick look at the client (no stack use!)
            sta         ZP_SLEEP_UNTIL
            sty         ZP_SLEEP_UNTIL + 1
            stz         T_REGISTER                          ; ... and at the system task: a sleeper
            txa
            and         #7
            tay
            lda         PC_BITS,Y
            cpx         #8
            bcs         :+
            tsb         ZP_SLEEPERS
            bra         :++
:
            tsb         ZP_SLEEPERS + 1
:
            lda         #SERIAL_TASK_NUM
            sta         T_REGISTER
            plp
            lda         #ERR_IO_WOULD_BLOCK
            sec
            rts

; The client isn't a sleeper any more (its reply came before its time)
PC_UNNAP:
            php
            sei
            lda         PC_CLIENT
            and         #7
            tay
            lda         PC_BITS,Y
            ldx         PC_CLIENT
            stz         T_REGISTER                          ; Quick look at the system task (no stack use!)
            cpx         #8
            bcs         :+
            trb         ZP_SLEEPERS
            bra         :++
:
            trb         ZP_SLEEPERS + 1
:
            lda         #SERIAL_TASK_NUM
            sta         T_REGISTER
            plp
            rts

; The tick count .A.Y ticks from now.  OUT: .A.Y
PC_FROM_NOW:
            sta         PC_N
            sty         PC_N + 1
            jsr         TICKS_GET
            clc
            adc         PC_N
            pha
            tya
            adc         PC_N + 1
            tay
            pla
            rts

; Is it .A ticks past the request's time (PC_UNTIL)?  OUT: C = 1 yes
PC_LATE:
            clc
            adc         PC_UNTIL
            sta         PC_N
            lda         PC_UNTIL + 1
            adc         #0
            sta         PC_N + 1
            jsr         TICKS_GET                           ; Now - that: not negative once it's come
            sec
            sbc         PC_N
            tya
            sbc         PC_N + 1
            bmi         :+
            sec
            rts
:
            clc
            rts

; Is the client's request the one out (in PC_TXBUF: its block's first bytes and its data)?  Its mode's
; IO_MODE_NONBLOCK aside: a client that asked without waiting may ask again waiting (the song player).  OUT: C = 1 yes
PC_SAME:
            ldx         PC_CLIENT
            jsr         IO_SRV_MAP
            ldy         #PC_REQ_HDR - 1

@byte:
            lda         (ZP_IO_REQ),Y
            eor         PC_TXBUF + 4,Y
            cpy         #IO_BLK_MODE
            bne         :+
            and         #<~IO_MODE_NONBLOCK
:
            cmp         #0
            bne         @no
            dey
            bpl         @byte
            lda         PC_TXBUF + 2                        ; Its data: the payload after the block's bytes
            sec
            sbc         #PC_REQ_HDR
            sta         PC_N
            lda         PC_TXBUF + 3
            sbc         #0
            ora         PC_N
            beq         @yes
            inc         ZP_IO_REQ + 1                       ; The data area
            ldy         #0
:
            lda         (ZP_IO_REQ),Y
            cmp         PC_TXBUF + 4 + PC_REQ_HDR,Y
            bne         @no_data
            iny
            cpy         PC_N
            bne         :-
            dec         ZP_IO_REQ + 1

@yes:
            jsr         IO_SRV_UNMAP
            sec
            rts

@no_data:
            dec         ZP_IO_REQ + 1

@no:
            jsr         IO_SRV_UNMAP
            clc
            rts

; The request in the client's request block, as a frame
PC_SEND_REQ:
            ldx         PC_CLIENT
            jsr         IO_SRV_MAP
            ldy         #PC_REQ_HDR - 1                     ; Its block's first bytes
:
            lda         (ZP_IO_REQ),Y
            sta         PC_TXBUF + 4,Y
            dey
            bpl         :-
            lda         PC_TXBUF + 4 + IO_BLK_COUNT         ; Its data: a write's count
            sta         PC_N
            lda         PC_TXBUF + 4 + IO_BLK_COUNT + 1
            sta         PC_N + 1
            lda         PC_TXBUF + 4 + IO_BLK_TYPE
            cmp         #H9_WRITE
            beq         @data
            cmp         #H9_WSTAT                           ; A stat record
            beq         @stat
            cmp         #H9_OPEN                            ; A name
            beq         @name
            cmp         #H9_CREATE
            beq         @name
            cmp         #H9_REMOVE
            beq         @name
            stz         PC_N                                ; None
            stz         PC_N + 1
            bra         @data

@stat:
            lda         #IO_STAT_SIZE
            sta         PC_N
            stz         PC_N + 1
            bra         @data

@name:
            inc         ZP_IO_REQ + 1                       ; Its length, with its 0 (256 at most)
            ldy         #0
:
            lda         (ZP_IO_REQ),Y
            beq         :+
            iny
            bne         :-
            dey                                             ; (No 0 in 256: 256)
:
            dec         ZP_IO_REQ + 1
            iny
            sty         PC_N
            stz         PC_N + 1
            bne         @data
            inc         PC_N + 1

@data:
            lda         PC_N
            ora         PC_N + 1
            beq         @copied
            inc         ZP_IO_REQ + 1                       ; The data area
            ldy         #0
:
            lda         (ZP_IO_REQ),Y
            sta         PC_TXBUF + 4 + PC_REQ_HDR,Y
            iny
            cpy         PC_N
            bne         :-
            dec         ZP_IO_REQ + 1

@copied:
            jsr         IO_SRV_UNMAP
            lda         #PC_T_REQ
            sta         PC_TXBUF
            lda         PC_TAG
            sta         PC_TXBUF + 1
            lda         PC_N                                ; The payload: the block's bytes and the data
            clc
            adc         #PC_REQ_HDR
            sta         PC_TXBUF + 2
            lda         PC_N + 1
            adc         #0
            sta         PC_TXBUF + 3
            tay
            lda         PC_TXBUF + 2
                                                            ; (On to PC_FRAME_SEND)

; Send the frame in PC_TXBUF: its type, tag and length are there, and .A.Y = the payload's length; its CRC goes
; after it
PC_FRAME_SEND:
            clc
            adc         #4                                  ; The body, to the payload's end
            sta         PC_LEN
            sta         PC_N
            tya
            adc         #0
            sta         PC_LEN + 1
            sta         PC_N + 1
            lda         #<PC_TXBUF
            ldy         #>PC_TXBUF
            jsr         PC_CRC_OF
            lda         PC_CRC                              ; (PC_PTR: just after it)
            sta         (PC_PTR)
            ldy         #1
            lda         PC_CRC + 1
            sta         (PC_PTR),Y
            lda         PC_LEN                              ; With the CRC
            clc
            adc         #2
            sta         PC_LEN
            bcc         PC_RESEND
            inc         PC_LEN + 1

; Send the frame in PC_TXBUF (PC_LEN bytes, with its CRC) again: the fast handler sends it, PC_MARK first,
; ahead of the TX ring's bytes (serfast.s: SER_TX_STEP).  (Not while it's still going out)
PC_RESEND:
            php
            sei
            lda         PC_TXS
            ora         PC_TXE
            bne         @done
            lda         #<PC_TXBUF
            sta         PC_TXP
            lda         #>PC_TXBUF
            sta         PC_TXP + 1
            lda         PC_LEN
            sta         PC_TXL
            lda         PC_LEN + 1
            sta         PC_TXL + 1
            lda         #1
            sta         PC_TXS
            lda         #PC_MARK
            ldy         ZP_SER_SEND_STATUS
            bne         @queue
            _M_SER_TX_BYTE nobell                           ; Idle: PC_MARK now ...
            inc         ZP_SER_SEND_STATUS                  ; (SER_SEND_STATUS_BUSY)
            bra         @done

@queue:
            sta         PC_TXE                              ; ... or the TX IRQ's next

@done:
            plp
            rts

; The frame in PC_RXBUF: its CRC, and its tag.  OUT: C = 0, .A = its type; or C = 1, .A = 0: not the request's
; (its tag), or .A <> 0: damaged
PC_CHECK:
            lda         PC_RXBUF + 2                        ; The body, to the payload's end
            clc
            adc         #4
            sta         PC_N
            lda         PC_RXBUF + 3
            adc         #0
            sta         PC_N + 1
            lda         #<PC_RXBUF
            ldy         #>PC_RXBUF
            jsr         PC_CRC_OF
            lda         (PC_PTR)
            cmp         PC_CRC
            bne         @damaged
            ldy         #1
            lda         (PC_PTR),Y
            cmp         PC_CRC + 1
            bne         @damaged
            lda         PC_RXBUF + 1
            cmp         PC_TAG
            bne         @not_its
            lda         PC_RXBUF
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

; The CRC-16 (CCITT: $1021, from $FFFF) of PC_N bytes (1-$FFFF) at .A.Y.  OUT: PC_CRC; PC_PTR = just after them
; Modifies: .A, .X, .Y, PC_N
PC_CRC_OF:
            sta         PC_PTR
            sty         PC_PTR + 1
            lda         #$FF
            sta         PC_CRC
            sta         PC_CRC + 1

@byte:
            lda         (PC_PTR)                            ; (Greg Cook's, a byte at a time, no table)
            eor         PC_CRC + 1
            sta         PC_CRC + 1
            lsr
            lsr
            lsr
            lsr
            tax
            asl
            eor         PC_CRC
            sta         PC_CRC
            txa
            eor         PC_CRC + 1
            sta         PC_CRC + 1
            asl
            asl
            asl
            tax
            asl
            asl
            eor         PC_CRC + 1
            tay
            txa
            rol
            eor         PC_CRC
            sta         PC_CRC + 1
            sty         PC_CRC
            inc         PC_PTR
            bne         :+
            inc         PC_PTR + 1
:
            lda         PC_N
            bne         :+
            dec         PC_N + 1
:
            dec         PC_N
            lda         PC_N
            ora         PC_N + 1
            bne         @byte
            rts
