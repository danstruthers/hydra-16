.debuginfo

SER_SEND_STATUS_READY = 0
SER_SEND_STATUS_BUSY  = 1
SER_SEND_STATUS_ERROR = $FF

.segment "BIOS"

HEX_MAP: .byte "0123456789ABCDEF"
NamedHString HYDRA_WELCOME, "Welcome to the HYDRA-16!"

; ****************************************************************************
; Serial driver: the file server for /dev/cons and /dev/ser (see IO_PLAN.md).  Runs in its own Resident
; task (SERIAL_TASK_NUM, started by DRV_START at boot), so its state (ZP_SER_SEND_STATUS, ZP_SER_CAPTURE,
; the SER_* task ZP in zero.s) and its RX and TX rings (SER_RX_BUF, SER_TX_BUF) live in that task.
;   RX: the IRQ handler puts each received byte into the RX ring, and wakes the tasks waiting to read.
;   TX: bytes go into the TX ring (or straight to the ACIA when it's idle); SER_TX_NEXT sends the next
;       one each time the ACIA's transmit register empties: the Rockwell 65C51's TDRE interrupt, or for
;       the WDC 65C51 (whose TDRE doesn't work) VIA timer 2, a character's time after each byte (SER_ACIA).
;   /dev/cons reads only for the foreground task (ZP_SER_CAPTURE: the shell to start with); others
;   wait until they're brought to the foreground.  /dev/ser is the raw port.  The requests themselves
;   are handled on ROM page 2 (ser_srv.s).
;   READ_CHAR / WRITE_CHAR use the task's fd 0 / fd 1, or the rings directly if the task has none.

SERIAL_DRIVER:
                .word           SERIAL_INIT             ; DriverInfo::init
                .word           SERIAL_STOP             ; DriverInfo::stop
                .word           SERIAL_NAME             ; DriverInfo::name
NamedHString SERIAL_NAME, "SERIAL"
CONS_NAME:      .byte           "cons", 0
SER_NAME:       .byte           "ser", 0

; Gate into the serial task: set the foreground task (the one /dev/cons reads for)
; IN: .A = task
TASK_GATE       SER_CALL_SET_CAPTURE, SERIAL_SET_CAPTURE, SERIAL_TASK_NUM

; Serve routines (page 2, ser_srv.s)
FAR_GATE_INLINE CONS_SERVE,     PAGE2::CONS_SERVE,      2
FAR_GATE_INLINE SER_SERVE,      PAGE2::SER_SERVE,       2

; Driver init (runs in the serial task).  OUT: C = 0 on success, or C = 1 and .A = error
SERIAL_INIT:
                php                                     ; Save caller's I flag
                sei
                lda             #$10 | SR_SELECT    ; 8-N-1
                sta             ACIA_R_CTRL
.if SER_ACIA = SER_ACIA_ROCKWELL
                lda             #ACIA_CMD_BIT_DTRL | ACIA_CMD_BIT_TLIE  ; No parity, no echo, tx & rx interrupts.
.else
                lda             #ACIA_CMD_BIT_DTRL | ACIA_CMD_BIT_TLID  ; No parity, no echo, rx interrupts
.endif                                                                  ;   (WDC: TX paced by VIA timer 2)
                sta             ACIA_R_CMD
                lda             #SER_SEND_STATUS_READY
                sta             ZP_SER_SEND_STATUS
                lda             #SHELL_TASK_NUM         ; The shell is in the foreground to start with
                sta             ZP_SER_CAPTURE
                ldx             #SER_RX_HEAD - SER_WR_WAIT
:
                stz             SER_WR_WAIT,X           ; Empty rings, nobody waiting
                dex
                bpl             :-
                ldx             #IRQ_NUMBER_ONBOARD_SERIAL
                lda             #<SERIAL_IRQ_HANDLER
                ldy             #>SERIAL_IRQ_HANDLER
                jsr             IRQ_REGISTER            ; Handler runs in this (the serial) task
                bcs             @done
.if SER_ACIA = SER_ACIA_WDC
                lda             VIA_R_AUX_CTRL          ; VIA timer 2: one-shot (ACR bit 5 = 0)
                and             #<~VIA_T2_INT_BIT
                sta             VIA_R_AUX_CTRL
                lda             #VIA_INT_ENABLE | VIA_T2_INT_BIT
                sta             VIA_R_INT_ENABLE        ; Its IRQ on
                ldx             #IRQ_NUMBER_ONBOARD_VIA
                lda             #<SERIAL_T2_HANDLER
                ldy             #>SERIAL_T2_HANDLER
                jsr             IRQ_REGISTER            ; (After the scheduler's VIA handler: T1)
                bcs             @done
.endif
                LOAD_ADDR       CONS_SERVE, ZP_TC_VEC   ; The files
                lda             #<CONS_NAME
                ldy             #>CONS_NAME
                ldx             #SERIAL_TASK_NUM
                jsr             DEV_REGISTER
                bcs             @done
                LOAD_ADDR       SER_SERVE, ZP_TC_VEC
                lda             #<SER_NAME
                ldy             #>SER_NAME
                jsr             DEV_REGISTER

@done:
                jmp             MM_RETURN               ; Restore caller's I flag, keep C

.assert         SER_RX_HEAD - SER_WR_WAIT = 7, error, "SERIAL_INIT clears the serial task ZP as one block"

SERIAL_STOP:
                clc
                rts

; Set the foreground task: /dev/cons reads for it (runs in the serial task; use SER_CALL_SET_CAPTURE,
; or IO_CTL SER_CTL_FOREGROUND on a /dev/cons fd).  Wakes the tasks waiting to read, so the new
; foreground task gets its input and the others go back to waiting.
; IN: .A = task.  Modifies: .A, .X, .Y
SERIAL_SET_CAPTURE:
                sta             ZP_SER_CAPTURE
                ldx             #SER_RD_WAIT
                jsr             SER_WAKE
                clc
                rts

; Input a character, if there is one: from fd 0 (stdin), without waiting.  (A task without an fd 0
; gets nothing.)  /dev/cons echoes it.
; On return, carry flag indicates whether a key was pressed
; If a key was pressed, the key value will be in the A register
;
; Modifies: flags, A
READ_CHAR:
SERIAL_READ:
                phx
                ldx             IO_FD_SERVER            ; fd 0 open?
                cpx             #IO_FD_CLOSED
                beq             @none
                lda             IO_FD_MODE              ; Just this once, don't wait
                pha
                ora             #IO_MODE_NONBLOCK
                sta             IO_FD_MODE
                ldx             #0
                jsr             IO_GETC                 ; C = 0: .A = byte
                plx
                stx             IO_FD_MODE
                bcs             @none
                plx
                sec
                rts

@none:
                plx
                clc
                rts

; Input a character, waiting for it: from fd 0, so the task sleeps until one comes in (or until it's
; brought to the foreground, for /dev/cons, which echoes it).  A task without an fd 0 (or with a
; non-blocking one) polls READ_CHAR, yielding in between.
; OUT: .A = the character, C = 1; or .A = error (e.g. ERR_IO_EOF: the end of a pipe), C = 0
; Modifies: flags, A
GET_CHAR:
                phx
                ldx             IO_FD_SERVER            ; fd 0 open?
                cpx             #IO_FD_CLOSED
                beq             @poll
                ldx             #0
                jsr             IO_GETC                 ; Waits for it
                bcc             @got
                cmp             #ERR_IO_WOULD_BLOCK
                bne             @error                  ; (A non-blocking fd 0: poll)

@poll:
                jsr             READ_CHAR
                bcs             @done
                jsr             YIELD
                bra             @poll

@got:
                sec

@done:
                plx
                rts

@error:
                plx
                clc
                rts

WRITE_BYTE:
                pha                                         ; Save A for LSD.
                lsr
                lsr
                lsr
                lsr                                         ; MSD to LSD position.
                jsr             WRITE_HEX                   ; Output hex digit.
                pla                                         ; Restore A.

WRITE_HEX_MASK:
                and             #$0F                        ; Mask LSD for hex print.

WRITE_HEX:
                phx
                tax
                lda             HEX_MAP,x
                SKIPNEXT

; Output a character (from the A register): to fd 1 (stdout), or straight to the serial port if the
; task has no fd 1 (system and driver tasks).  If fd 1 fails (e.g. a pipe nobody reads any more), the
; character is dropped.  Not with IRQs off: it may have to wait for the TX IRQ.
;
; Modifies: flags
WRITE_CHAR:
SERIAL_WRITE:
                phx                                         ; Must stay 1 byte (WRITE_HEX SKIPNEXTs over it)
                phy
                ldx             IO_FD_SERVER + IO_FD_SIZE   ; fd 1 open?
                cpx             #IO_FD_CLOSED
                beq             @direct
                pha
                ldx             #1
                jsr             IO_PUTC
                pla
                bra             @done

@direct:
                jsr             SER_TX_TRY
                bcc             @done
                wai                                         ; The TX ring is full: wait for the TX IRQ
                bra             @direct

@done:
                ply
                plx
                rts

; Queue a byte for the serial port, from any task: into the TX ring, or straight to the ACIA when it's
; idle (the ring is empty then).  IN: .A = byte.  OUT: C = 0, or C = 1 if the ring is full
; Preserves .A, .X; modifies .Y
SER_TX_TRY:
                php                                         ; Save caller's I flag
                sei
                phx
                ldx             T_REGISTER
                ldy             #SERIAL_TASK_NUM
                sty             T_REGISTER                  ; Quick switch to the serial task (no stack use!)
                ldy             ZP_SER_SEND_STATUS
                bne             @queue                      ; Busy: the TX IRQ sends it
                _M_SER_TX_BYTE
                inc             ZP_SER_SEND_STATUS          ; SER_SEND_STATUS_BUSY
                bra             @ok

@queue:
                ldy             SER_TX_HEAD
                sta             SER_TX_BUF,Y
                iny
                cpy             SER_TX_TAIL
                beq             @full                       ; (The byte stored isn't counted: head stays)
                sty             SER_TX_HEAD

@ok:
                stx             T_REGISTER                  ; Back to the calling task
                plx
                plp
                clc
                rts

@full:
                stx             T_REGISTER                  ; Back to the calling task
                plx
                plp
                sec
                rts

; The transmitter is free (the Rockwell 65C51's TDRE interrupt, or the WDC 65C51's timer 2 ran out): send
; the next byte from the TX ring, or go idle.  Runs in the serial task, in an IRQ handler.
; Modifies: .A, .X, .Y
SER_TX_NEXT:
                lda             ZP_SER_SEND_STATUS
                beq             @done                   ; Idle: nothing to send
                ldy             SER_TX_TAIL             ; The next byte from the TX ring
                cpy             SER_TX_HEAD
                bne             :+
                stz             ZP_SER_SEND_STATUS      ; SER_SEND_STATUS_READY: the ring is empty
                rts
:
                lda             SER_TX_BUF,Y
                iny
                sty             SER_TX_TAIL
                _M_SER_TX_BYTE
                ldx             #SER_WR_WAIT            ; There's room: wake the waiting writers
                jmp             SER_WAKE

@done:
                rts

.if SER_ACIA = SER_ACIA_WDC
; The WDC 65C51's TX pacing: VIA timer 2 ran out (a character's time since the last byte went).  Registered
; by SERIAL_INIT on the VIA's IRQ (after the scheduler's handler); runs in the serial task.
; OUT: C = 1 if T2 was interrupting
SERIAL_T2_HANDLER:
                lda             #VIA_T2_INT_BIT
                and             VIA_R_INT_FLAGS
                beq             @not_mine
                lda             VIA_R_T2C_L             ; Clears its IRQ
                jsr             SER_TX_NEXT             ; (Starts it again for the next byte)
                sec
                rts

@not_mine:
                clc
                rts
.endif

; A break or kill key (the IRQ handler, in the serial task, IRQs off): the foreground task gets .A
; (TASK_BREAK_FLAG or TASK_KILL_FLAG), and the tasks it started (and theirs, 4 levels) are killed; they
; stop waiting, so they run and see it (SCHED_RESUME).  The keys typed before it are dropped.  A killed
; foreground task other than the shell hands the console back to the shell.
; Modifies: .A, .X, .Y, ZP_TEMP
SER_BREAK:
                pha                                         ; (The foreground task's flag)
                lda             SER_RX_HEAD                 ; Drop the typed-ahead keys
                sta             SER_RX_TAIL
                ldx             #MAX_TASK_NUMBER            ; Tasks 15-1

@task:
                cpx             ZP_SER_CAPTURE
                beq             @foreground
                lda             #4                          ; Started by the foreground task, or by one
                sta             ZP_TEMP                     ;   of those, ...?
                txa
                tay                                         ; .Y = the task, then its owner, ...

@owner:
                sty             T_REGISTER                  ; Quick look (no stack use!)
                ldy             ZP_TASK_OWNER
                lda             #SERIAL_TASK_NUM
                sta             T_REGISTER
                cpy             ZP_SER_CAPTURE
                beq             @child
                cpy             #MAX_TASK_NUMBER + 1
                bcs             @next                       ; ($FF: nobody)
                dec             ZP_TEMP
                bne             @owner
                bra             @next

@child:
                ldy             #TASK_KILL_FLAG
                bra             @flag

@foreground:
                pla
                pha
                tay

@flag:                                                      ; .Y = the flag for task .X
                stx             T_REGISTER                  ; Quick switch (no stack use!)
                lda             TASK_STATUS_REG
                and             #TASK_BUSY_FLAG | TASK_RESIDENT_FLAG
                cmp             #TASK_BUSY_FLAG
                bne             :+                          ; (Free, or a driver)
                tya
                tsb             TASK_STATUS_REG
                rmb2            TASK_STATUS_REG             ; Not waiting any more (TASK_WAITING_FLAG)
:
                lda             #SERIAL_TASK_NUM
                sta             T_REGISTER

@next:
                dex
                bne             @task
                pla
                cmp             #TASK_KILL_FLAG
                bne             @done
                lda             ZP_SER_CAPTURE              ; A killed foreground task: the shell gets the
                cmp             #SHELL_TASK_NUM             ;   console back
                beq             @done
                lda             #SHELL_TASK_NUM
                sta             ZP_SER_CAPTURE

@done:
                rts

; Wake every task in a wait mask of the serial task (SER_RD_WAIT or SER_WR_WAIT), and clear the mask.
; Runs in the serial task.  IN: .X = the mask's ZP address.  Modifies: .A, .Y
SER_WAKE:
                php
                sei
                lda             0,X
                ora             1,X
                beq             @done
                ldy             #0

@loop:
                lsr             1,X
                ror             0,X
                bcc             :+
                tya
                jsr             IO_WAKE
:
                iny
                cpy             #16
                bne             @loop

@done:
                plp
                rts

; Convenience method to write CR/LF to output stream
WRITE_CRLF:
                PRINT_CHAR_JMP  #ASCII_CR, #ASCII_LF

WRITE_PROMPT:
                PRINT_CRLF
                PRINT_CHAR      #ASCII_T
                PRINT_HEX_MASK  $FFF0
                PRINT_SPACE
                PRINT_BYTE      RAM_BANK_REG
                cmp             #$F0
                bcc             @not_shared
                PRINT_CHAR      #ASCII_LPAREN
                PRINT_HEX_MASK  $FFF1                       ; Shared RAM sub-bank
                PRINT_CHAR      #ASCII_RPAREN

@not_shared:
                PRINT_CHAR      #ASCII_COLON
                PRINT_BYTE      ROM_BANK_REG
                PRINT_CHAR_JMP  #ASCII_GT

; .A, .Y hold the addr of HString to write
; Clobbers .A, .Y; Preserves .X
WRITE_HSTRING:
                phx
                sta             ZP_HS_TEMP
                sty             ZP_HS_TEMP + 1
                lda             (ZP_HS_TEMP)                ; Length of HString
                tax
                ldy             #0
@write_loop:
                iny
                PRINT_CHAR      {(ZP_HS_TEMP),Y}
                dex
                bne             @write_loop
                plx
                rts

; Escape sequences
CLEAR_SCR:
                PRINT_ESC_SEQ #ASCII_LBRACKET, #ASCII_2, #ASCII_J
                PRINT_ESC_SEQ_JMP #ASCII_LBRACKET, #ASCII_0, #ASCII_SEMI, #ASCII_0, #ASCII_f

; Serial IRQ handler (registered with IRQ_REGISTER; runs in the serial task)
; OUT: C = 1 if the ACIA was interrupting
SERIAL_IRQ_HANDLER:
                lda             ACIA_R_STATUS           ; Read once: it clears the IRQ flag, so a second
                bpl             @not_mine 	            ;   read could lose a TDRE that came in between
                pha
.if SER_ACIA = SER_ACIA_ROCKWELL                        ; (The WDC 65C51's TDRE doesn't work: timer 2 instead)
                and             #ACIA_STATUS_BIT_TDRE
                beq             @check_recv             ; Transmit register still full
                jsr             SER_TX_NEXT
.endif

@check_recv:
                pla
                and             #ACIA_STATUS_BIT_RDRF   ; is read register full?
                beq             @int_done
                IO_PORT_READ    ACIA_R_DATA
                cmp             #SER_KEY_BREAK          ; Break or kill: act on it now (the task
                beq             @break                  ;   may not be reading)
                cmp             #SER_KEY_KILL
                beq             @kill
                ldy             SER_RX_HEAD             ; Into the RX ring
                sta             SER_RX_BUF,Y
                iny
                cpy             SER_RX_TAIL
                beq             @int_done               ; The ring is full: the byte is dropped
                sty             SER_RX_HEAD
                ldx             #SER_RD_WAIT            ; Wake the waiting readers
                jsr             SER_WAKE

@int_done:
                sec
                rts

@break:
                lda             #TASK_BREAK_FLAG
                bra             :+
@kill:
                lda             #TASK_KILL_FLAG
:
                jsr             SER_BREAK
                lda             #SCHED_RESCHED_A        ; Switch tasks now, so it happens (SCHED_RESUME)
                ldy             #SCHED_RESCHED_Y
                sec
                rts

@not_mine:
                clc
                rts

; ****************************************************************************
; VIA (the scheduler's tick is its timer 1; see SCHED_START; SPI uses port B, spi.s)

; Must be called from the system task (the VIA handler runs there)
VIA_INIT:
            PUSH_AXY
            ldx     #IRQ_NUMBER_ONBOARD_VIA
            lda     #<VIA_IRQ_HANDLER
            ldy     #>VIA_IRQ_HANDLER
            jsr     IRQ_REGISTER
            PULL_YXA
            rts

; VIA IRQ handler (registered by VIA_INIT; runs in the system task).
; OUT: C = 1 if T1 was interrupting
VIA_IRQ_HANDLER:
; check which sub-device is triggering the IRQ
            lda     #VIA_T1_INT_BIT
            and     VIA_R_INT_FLAGS
            beq     :+
            lda     VIA_R_T1C_L             ; clear the interrupt
            lda     #SCHED_RESCHED_A        ; T1 is the scheduler's tick: ask the dispatcher for a task switch
            ldy     #SCHED_RESCHED_Y
            sec
            rts

:
            clc
            rts


.segment "IO_PORTS"
IO_PORT_0:      .tag IO_Port
IO_PORT_1:      .tag IO_Port
IO_PORT_2:      .tag IO_Port
IO_PORT_3:      .tag IO_Port
IO_PORT_4:      .tag IO_Port
IO_PORT_5:      .tag IO_Port
IO_PORT_6:      .tag IO_Port
IO_PORT_7:      .tag IO_Port
IO_PORT_8:      .tag IO_Port
IO_PORT_9:      .tag IO_Port
IO_PORT_A:      .tag IO_Port
IO_PORT_B:      .tag IO_Port
IO_PORT_C:      .tag IO_Port
IO_PORT_D:      .tag IO_Port
IO_PORT_E:      .tag IO_Port
IO_PORT_F:      .tag IO_Port_10_Bytes

.macro VECTORS
                .word   NMI_ENTRY       ; NMI vector (COMMON block, same address on every page)
                .word   RESET_ENTRY     ; RESET vector
                .word   IRQ_STUB_F      ; IRQ vector (COMMON block S/W IRQ stub; unused, see below)
                ;       will actually be pulled from the Vector RAM, depending on lowest priority IRQ currently triggered,
                ;       or vector for IRQ# set in V[0..3] if none are triggered.  Can trigger S/W IRQs by setting V and then
                ;       calling BRK.  S/W IRQ# is $F, so setting V[0..3] to $F and v[4..7] to a different number, you can
                ;       have up to 16 unique S/W IRQs.  You can invoke a hardware device IRQ handler in the same way
.endmacro

.segment "RESETVEC_P0"
    VECTORS

.segment "RESETVEC_P1"
    VECTORS

.segment "RESETVEC_P2"
    VECTORS

.segment "RESETVEC_P3"
    VECTORS

.segment "RESETVEC_P4"
    VECTORS

.segment "RESETVEC_P5"
    VECTORS

.segment "RESETVEC_P6"
    VECTORS

.segment "RESETVEC_P7"
    VECTORS

.segment "RESETVEC_P8"
    VECTORS

.segment "RESETVEC_P9"
    VECTORS

.segment "RESETVEC_PA"
    VECTORS

.segment "RESETVEC_PB"
    VECTORS

.segment "RESETVEC_PC"
    VECTORS

.segment "RESETVEC_PD"
    VECTORS

.segment "RESETVEC_PE"
    VECTORS

.segment "RESETVEC_PF"
    VECTORS
