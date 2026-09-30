.debuginfo

SER_SEND_STATUS_READY = 0
SER_SEND_STATUS_BUSY  = 1
SER_SEND_STATUS_ERROR = $FF

.segment "BIOS"

; ****************************************************************************
; Serial driver: the file server for /dev/cons and /dev/ser (see docs/plans/IO_PLAN.md).  Runs in its own Resident
; task (SERIAL_TASK_NUM, started by DRV_START at boot), so its state (ZP_SER_SEND_STATUS, ZP_SER_CAPTURE,
; the SER_* task ZP in zero.s) and its RX and TX rings (SER_RX_BUF, SER_TX_BUF) live in that task.
;   RX: the IRQ handler puts each received byte into the RX ring, and wakes the tasks waiting to read.
;   TX: bytes go into the TX ring (or straight to the ACIA when it's idle); SER_TX_NEXT sends the next
;       one each time the ACIA's transmit register empties: the Rockwell 65C51's TDRE interrupt, or for
;       the WDC 65C51 (whose TDRE doesn't work) VIA timer 2, a character's time after each byte (SER_ACIA).
;   /dev/cons reads only for the foreground task (ZP_SER_CAPTURE: the shell to start with); others
;   wait until they're brought to the foreground.  /dev/ser is the raw port, and /dev/ser/ctl its settings
;   (baud rate, data bits, parity, stop bits: 9600 8-N-1 at boot).  The requests themselves are handled on
;   ROM page 2 (ser_srv.s, serctl.s).
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
FAR_GATE_INLINE SER_CONFIG,     PAGE2::SER_CONFIG,      2

; Driver init (runs in the serial task).  OUT: C = 0 on success, or C = 1 and .A = error (only if its IRQ
; handler can't be registered: without its files, /dev/cons and /dev/ser, the console still works)
SERIAL_INIT:
                php                                     ; Save caller's I flag
                sei
                lda             #SER_RATE_BOOT          ; The ACIA: the boot rate, 8-N-1, no echo, IRQs
                ldy             #SER_FMT_8N1            ;   (SER_CMD_BASE)
                jsr             SER_CONFIG
                lda             #SER_SEND_STATUS_READY
                sta             ZP_SER_SEND_STATUS
                stz             SER_PEND                ; Nothing for the fast handler's leftovers yet
                lda             #SHELL_TASK_NUM         ; The shell is in the foreground to start with
                sta             ZP_SER_CAPTURE
                ldx             #SER_RX_HEAD - SER_PREFIX
:
                stz             SER_PREFIX,X            ; Empty rings, nobody waiting, no prefix key
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
                LOAD_ADDR       CONS_SERVE, ZP_TC_VEC   ; The files.  (If they can't be registered, e.g.
                lda             #<CONS_NAME             ;   no shared RAM for the device table, the console
                ldy             #>CONS_NAME             ;   still works: tasks without fds use the rings
                ldx             #SERIAL_TASK_NUM        ;   directly.  It's what reports the other drivers'
                jsr             DEV_REGISTER         ;   failures, so its init doesn't fail.)
                LOAD_ADDR       SER_SERVE, ZP_TC_VEC
                lda             #<SER_NAME
                ldy             #>SER_NAME
                jsr             DEV_REGISTER
                clc

@done:
                jmp             MM_RETURN               ; Restore caller's I flag, keep C

.assert         SER_RX_HEAD - SER_PREFIX = 8, error, "SERIAL_INIT clears the serial task ZP as one block"

SERIAL_STOP:
                clc
                rts

; Set the foreground task: /dev/cons reads for it, and it and the tasks it started may write to it (runs
; in the serial task; use CONS_SET_FG, which checks the task first).  Wakes the tasks waiting to read
; or write, so the new foreground tasks go on and the others go back to waiting.
; IN: .A = task.  Modifies: .A, .X, .Y
SERIAL_SET_CAPTURE:
                sta             ZP_SER_CAPTURE
                ldx             #SER_RD_WAIT
                jsr             SER_WAKE
                ldx             #SER_WR_WAIT
                jsr             SER_WAKE
                clc
                rts

; Bring a task to the front (see SERIAL_SET_CAPTURE), from any task: the shell's fg, /dev/proc's ctl
; file, and a task ending in the foreground (CONS_RELEASE).
; IN: .A = task: 1-15, busy, not a driver.  OUT: C = 0; or C = 1, .A = ERR_BAD_TASK
; Modifies: .A, .X, .Y
CONS_SET_FG:
                tax
                jsr             CONS_FG_CHECK
                bcs             @done
                txa
                jmp             SER_CALL_SET_CAPTURE

@done:
                rts

; Can task .X be brought to the front?  OUT: C = 0 yes; or C = 1, .A = ERR_BAD_TASK.  Preserves .X
CONS_FG_CHECK:
                txa
                beq             @bad                        ; (Task 0: the system's idle task)
                cmp             #MAX_TASK_NUMBER + 1
                bcs             @bad
                php
                sei
                ldy             T_REGISTER
                stx             T_REGISTER                  ; Quick look (no stack use!)
                lda             TASK_STATUS_REG
                sty             T_REGISTER
                plp
                and             #TASK_BUSY_FLAG | TASK_RESIDENT_FLAG
                cmp             #TASK_BUSY_FLAG
                bne             @bad                        ; (Free, or a driver)
                clc
                rts

@bad:
                lda             #ERR_BAD_TASK
                sec
                rts

; The current task is ending (TASK_EXIT): if it's in the foreground, the task that started it gets the
; console, or else the shell.  Modifies: .A, .X, .Y
CONS_RELEASE:
                php
                sei
                ldx             T_REGISTER
                ldy             #SERIAL_TASK_NUM
                sty             T_REGISTER                  ; Quick look (no stack use!)
                ldy             ZP_SER_CAPTURE
                stx             T_REGISTER
                plp
                tya
                eor             T_REGISTER
                and             #$0F
                bne             @done                       ; Not in the foreground
                lda             ZP_TASK_OWNER
                jsr             CONS_SET_FG
                bcc             @done
                lda             #SHELL_TASK_NUM
                jsr             CONS_SET_FG

@done:
                rts

; Input a character, if there is one: from fd 0 (stdin), without waiting.  (A task without an fd 0
; gets nothing.)  /dev/cons echoes it.
; On return, carry flag indicates whether a key was pressed
; If a key was pressed, the key value will be in the A register
;
; Modifies: flags, A
READ_CHAR:
SERIAL_READ:
                _M_STDIN_FAST                           ; (Read ahead from a pipe: no IO call)
                phx
                lda             ZP_OUT_CNT              ; Our buffered output first (a prompt)
                beq             :+
                jsr             IO_FLUSH
:
                ldx             IO_FD_SERVER            ; fd 0 open?
                cpx             #IO_FD_CLOSED
                beq             @none
                lda             IO_FD_MODE              ; Just this once, don't wait
                pha
                ora             #IO_MODE_NONBLOCK
                sta             IO_FD_MODE
                jsr             STDIN_GET               ; C = 0: .A = byte
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
                _M_STDIN_FAST                           ; (Read ahead from a pipe: no IO call)
                phx
                lda             ZP_OUT_CNT              ; Our buffered output first (a prompt)
                beq             :+
                jsr             IO_FLUSH
:
                ldx             IO_FD_SERVER            ; fd 0 open?
                cpx             #IO_FD_CLOSED
                beq             @poll
                jsr             STDIN_GET               ; Waits for it
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

; Output a character (from the A register): to fd 1 (stdout), or straight to the serial port if the
; task has no fd 1 (system and driver tasks).  Buffered (IO_FLUSH): the console a line at a time.  If fd 1
; fails (e.g. a pipe nobody reads any more), the character is dropped.  Not with IRQs off: it may have to wait for the TX IRQ.
;
; Modifies: flags
WRITE_CHAR:
SERIAL_WRITE:
                _M_STDOUT_FAST                              ; (Buffered for a pipe: no IO call)
                phx
                phy
                ldx             IO_FD_SERVER + IO_FD_SIZE   ; fd 1 open?
                cpx             #IO_FD_CLOSED
                beq             @direct
                pha
                jsr             STDOUT_PUT                  ; (Buffered; the console a line at a time)
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
                jsr             SER_TX_BYTE
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

; Send .A to the ACIA: _M_SER_TX_BYTE, as a subroutine (page 0 has room for one copy).  Uses .Y
SER_TX_BYTE:
                _M_SER_TX_BYTE
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
                ldx             #SER_WR_WAIT            ; (Wake the writers: a settings change waits for
                jmp             SER_WAKE                ;   the ring to empty, SER_DRAIN)
:
                lda             SER_TX_BUF,Y
                iny
                sty             SER_TX_TAIL
                jsr             SER_TX_BYTE
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
                lda             SER_PEND                ; (The bell; and what the fast handler left)
                beq             :+
                jsr             SER_DO_PENDING
:
                sec
                rts

@not_mine:
                clc
                rts
.endif

; A break or kill key (the IRQ handler, in the serial task, IRQs off): the foreground task gets .A
; (TASK_BREAK_FLAG or TASK_KILL_FLAG), and the tasks it started are killed (TASK_SIGNAL).  The keys
; typed before it are dropped.  (A killed foreground task hands the console back as it ends: CONS_RELEASE.)
; Modifies: .A, .X, .Y
SER_BREAK:
                ldx             SER_RX_HEAD                 ; Drop the typed-ahead keys
                stx             SER_RX_TAIL
                ldx             ZP_SER_CAPTURE
                jmp             TASK_SIGNAL

; Wake every task in a wait mask of the serial task (SER_RD_WAIT or SER_WR_WAIT), and clear the mask.
; Runs in the serial task.  IN: .X = the mask's ZP address.  Modifies: .A, .Y
SER_WAKE        = TASK_WAKE_MASK

; Serial IRQ handler (registered with IRQ_REGISTER; runs in the serial task)
; OUT: C = 1 if the ACIA was interrupting
SERIAL_IRQ_HANDLER:
                lda             SER_PEND                ; What the fast handler (SER_IRQ_FAST, which does the
                beq             @status                 ;   bytes) left: it comes here only for that
                jsr             SER_DO_PENDING
                sec
                rts

@status:
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
                ldy             SER_PREFIX
                bne             @command                ; The key after the prefix key
                cmp             #SER_KEY_PREFIX
                beq             @prefix

@store:
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

@prefix:
                sta             SER_PREFIX              ; (Non-zero)
                bra             @int_done

@command:
                stz             SER_PREFIX
                cmp             #SER_KEY_PREFIX
                beq             @store                  ; Twice: the key itself
                jsr             SER_COMMAND
                bra             @int_done

@break:
                stz             SER_PREFIX
                lda             #TASK_BREAK_FLAG
                bra             :+
@kill:
                stz             SER_PREFIX
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

; Do what the fast ACIA handler left (SER_PEND, see io/serfast.s), in the serial task, in an IRQ handler:
; wake the tasks waiting to read or write, a break or kill key, a console command, the bell.
; OUT: .A.Y = SCHED_RESCHED_A/Y after a break or kill (switch tasks now, so it happens), else .A = 0
; Modifies: .A, .X, .Y
SER_DO_PENDING:
                lda             SER_PEND
                stz             SER_PEND
                pha
                bit             #SER_PEND_CMD
                beq             :+
                lda             SER_PEND_KEY
                jsr             SER_COMMAND
                pla
                pha
:
                bit             #SER_PEND_WAKE_RD
                beq             :+
                ldx             #SER_RD_WAIT
                jsr             SER_WAKE
                pla
                pha
:
                bit             #SER_PEND_WAKE_WR
                beq             :+
                ldx             #SER_WR_WAIT
                jsr             SER_WAKE
                pla
                pha
:
                bit             #SER_PEND_BEEP
                beq             :+
                jsr             YM_BEEP
                pla
                pha
:
                ldx             #TASK_BREAK_FLAG
                bit             #SER_PEND_BREAK
                bne             @signal
                ldx             #TASK_KILL_FLAG
                bit             #SER_PEND_KILL
                bne             @signal
                pla
                lda             #0
                rts

@signal:
                pla
                txa
                jsr             SER_BREAK
                lda             #SCHED_RESCHED_A
                ldy             #SCHED_RESCHED_Y
                rts

; A console command: the key after the prefix key (SER_KEY_PREFIX).  A hex digit brings that task to the
; front (CONS_SET_FG's checks); 'l' lists the tasks that can be, e.g. "[1* 2 5]" (* = the foreground
; one).  Anything else, or a task that can't be brought to the front, rings the bell.  Runs in the serial
; task, in the IRQ handler: the replies go into the TX ring, or are dropped if it's full.
; IN: .A = key.  Modifies: .A, .X, .Y
SER_COMMAND:
                ora             #$20                        ; (Letters in lower case; digits stay)
                cmp             #'l'
                beq             SER_LIST
                sec
                sbc             #'0'
                cmp             #10
                bcc             @task                       ; 0-9
                sbc             #'a' - '0' - 10             ; (C = 1)
                cmp             #10
                bcc             @bell
                cmp             #16
                bcs             @bell                       ; a-f: 10-15

@task:
                tax
                jsr             CONS_FG_CHECK
                bcs             @bell
                txa
                jsr             SERIAL_SET_CAPTURE
                lda             ZP_SER_CAPTURE              ; Say so: "[n]"
                pha
                lda             #'['
                jsr             SER_TX_TRY
                pla
                tax
                lda             HEX_MAP,X
                jsr             SER_TX_TRY
                lda             #']'
                jmp             SER_TX_TRY

@bell:
                lda             #ASCII_BELL
                jmp             SER_TX_TRY

; The tasks that can be brought to the front: "[1* 2 5]"
SER_LIST:
                lda             #'['
                jsr             SER_TX_TRY
                ldx             #1

@task:
                jsr             CONS_FG_CHECK
                bcs             @next
                lda             HEX_MAP,X
                jsr             SER_TX_TRY
                cpx             ZP_SER_CAPTURE
                bne             :+
                lda             #'*'
                jsr             SER_TX_TRY
:
                lda             #' '
                jsr             SER_TX_TRY

@next:
                inx
                cpx             #MAX_TASK_NUMBER + 1
                bne             @task
                lda             #']'
                jmp             SER_TX_TRY
