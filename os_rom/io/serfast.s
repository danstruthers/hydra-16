.debuginfo

; ****************************************************************************
; The serial port's fast paths (BIOS ROM page 2, included inside `.scope PAGE2`, see all.s).  At 115200 baud a
; character takes 320 CPU cycles, and the 65C51 holds only one each way, so the usual paths (an IO request,
; about 3000 cycles; the IRQ dispatcher and a TASK_CALL, about 650) are far too slow for a byte at a time.
; These move a byte in well under 100 cycles, by switching T to the serial task for a moment (a "quick
; look": its ZP and RAM, where the rings are; no stack use until T is back) instead of running code in it:
;   SER_IRQ_FAST    the ACIA's interrupt (its vector points at SER_IRQ_STUB in the COMMON block): moves the
;                   received byte into the RX ring, and the next byte from the TX ring to the ACIA, and wakes
;                   the tasks waiting to read or write.  The rest (the break and kill keys, console commands,
;                   the bell) it leaves in SER_PEND for the driver's handler, reached through the dispatcher
;                   (IRQ_FAST_SLOW; SERIAL_IRQ_HANDLER: SER_DO_PENDING), which only happens for those.
;   SER_CONS_PUTS   IO_FLUSH, for /dev/cons from the foreground task: the stdout buffer into the TX ring.
;   SER_CONS_GETC   STDIN_GET, for /dev/cons from the foreground task: a key from the RX ring, echoed.
; Each keeps IRQs off only for one byte at a time.  What they can't do (not the foreground task, a full ring,
; a key /dev/cons treats specially) is left to the IO layer, which does it the usual way and waits.

.segment "IO_P2"

SER_IRQ_LOGICAL = 1                                         ; The ACIA's logical IRQ# (IRQ_STUB_1)

; Wake the tasks in a wait mask of the serial task (as IO_WAKE does: TASK_WAITING_FLAG off, by a quick look at
; each), and clear it.  In the serial task, IRQs off, no stack use.  Modifies: .A, .Y
.macro _M_SER_WAKE_QUICK   mask                        ; (Anonymous labels: no @ label, which
            lda         mask                                ;   would end the caller's @ labels)
            ora         mask + 1
            beq         :+++
            ldy         #0
:
            lsr         mask + 1
            ror         mask
            bcc         :+
            sty         T_REGISTER                          ; Quick look at task .Y (no stack use!)
            rmb2        TASK_STATUS_REG                     ; (TASK_WAITING_FLAG)
            lda         #SERIAL_TASK_NUM
            sta         T_REGISTER
:
            iny
            lda         mask
            ora         mask + 1
            bne         :--
:
.endmacro
.assert     TASK_WAITING_FLAG = 4, error, "_M_SER_WAKE_QUICK clears TASK_WAITING_FLAG with rmb2"

; The fast handlers' entry: from VIA_IRQ_STUB or SER_IRQ_STUB (COMMON), IRQs off, on page 2: .Y = which (0 the
; VIA, 1 the ACIA), .X = the interrupted ROM page, the interrupted .A, .X and .Y on the interrupted task's
; stack.  Each returns through IRQ_EXIT (.A = the page), or goes on to the dispatcher (IRQ_FAST_SLOW, with
; the stack and registers an IRQ stub leaves), after pulling .Y.
IRQ_FAST_P2:
            cpy         #0
            bne         SER_IRQ_FAST
            jmp         VIA_IRQ_FAST

; The ACIA's interrupt
SER_IRQ_FAST:
            ldy         T_REGISTER                          ; The interrupted task
            lda         #SERIAL_TASK_NUM
            sta         T_REGISTER                          ; Quick switch to the serial task (no stack use!)
            sty         SER_IRQ_T
            stx         SER_IRQ_W
            ldx         ACIA_R_STATUS                       ; Read once: it clears the IRQ flag (.X, all through)
            bmi         :+
            jmp         SER_FAST_EXIT                       ; (Not interrupting)
:
            txa
            and         #ACIA_STATUS_BIT_RDRF
            beq         @tx
            lda         ACIA_R_DATA                         ; A byte in
            cmp         #SER_KEY_BREAK                      ; Break or kill: the driver acts on it now (the
            beq         @break                              ;   task may not be reading)
            cmp         #SER_KEY_KILL
            beq         @kill
            ldy         SER_PREFIX
            bne         @command                            ; The key after the prefix key
            cmp         #SER_KEY_PREFIX
            bne         @store
            sta         SER_PREFIX                          ; The prefix key (non-zero)
            jmp         @tx

@break:
            stz         SER_PREFIX
            smb2        SER_PEND                            ; (SER_PEND_BREAK)
            jmp         @tx

@kill:
            stz         SER_PREFIX
            smb3        SER_PEND                            ; (SER_PEND_KILL)
            jmp         @tx

@command:
            stz         SER_PREFIX
            cmp         #SER_KEY_PREFIX
            beq         @store                              ; Twice: the key itself
            sta         SER_PEND_KEY
            smb4        SER_PEND                            ; (SER_PEND_CMD)
            jmp         @tx

@store:
            ldy         SER_RX_HEAD                         ; Into the RX ring
            sta         SER_RX_BUF,Y
            iny
            cpy         SER_RX_TAIL
            beq         @tx                                 ; The ring is full: the byte is dropped
            sty         SER_RX_HEAD
            _M_SER_WAKE_QUICK SER_RD_WAIT                   ; Wake the tasks waiting to read

@tx:
.if ::SER_ACIA = ::SER_ACIA_ROCKWELL                        ; (The WDC 65C51's TDRE doesn't work: timer 2
            lda         SER_PACED                           ;   and SERIAL_T2_HANDLER instead.  Paced, at
            bne         SER_FAST_EXIT                       ;   115200: timer 2 and SER_T2_FAST)
            txa
            and         #ACIA_STATUS_BIT_TDRE
            beq         SER_FAST_EXIT                       ; Still sending

; The transmitter is free (TDRE; or paced, timer 2 ran out: SER_T2_FAST): the next byte from the TX ring,
; or idle, and the writers woken.  In the serial task (a quick look), IRQs off; on to SER_FAST_EXIT
SER_TX_STEP:
            lda         ZP_SER_SEND_STATUS
            beq         SER_FAST_EXIT                       ; Idle: nothing to send
            ldy         SER_TX_TAIL                         ; The next byte from the TX ring
            cpy         SER_TX_HEAD
            bne         @send
            stz         ZP_SER_SEND_STATUS                  ; SER_SEND_STATUS_READY: the ring is empty
            bra         @room

@send:
            lda         SER_TX_BUF,Y
            iny
            sty         SER_TX_TAIL
            _M_SER_TX_BYTE

@room:
            _M_SER_WAKE_QUICK SER_WR_WAIT                   ; Wake the tasks waiting to write
.endif

SER_FAST_EXIT:
            ldx         SER_IRQ_W
            ldy         SER_IRQ_T
            lda         SER_PEND
            cmp         #1                                  ; C = 1: work for the driver's handler
            sty         T_REGISTER                          ; Back to the interrupted task (and its stack)
            ply
            bcs         @slow
            txa
            jmp         IRQ_EXIT

@slow:
            lda         #SER_IRQ_LOGICAL
            jmp         IRQ_FAST_SLOW

.if ::SER_ACIA = ::SER_ACIA_ROCKWELL
; VIA timer 2 ran out: paced sending (SER_PACED, the Rockwell ACIA at 115200: a character's time and
; SER_PACE_GAP idle bits since the last byte went) can send the next.  From VIA_IRQ_FAST, entered as
; SER_IRQ_FAST is, and it leaves the same way (the tick, if it's due too, is the next interrupt)
SER_T2_FAST:
            lda         VIA_R_T2C_L                         ; Clears its flag
            ldy         T_REGISTER                          ; The interrupted task
            lda         #SERIAL_TASK_NUM
            sta         T_REGISTER                          ; Quick switch to the serial task (no stack use!)
            sty         SER_IRQ_T
            stx         SER_IRQ_W
            lda         SER_PACED
            bne         SER_TX_STEP
            jmp         SER_FAST_EXIT                       ; (Not paced any more: nothing to do)
.endif

; The VIA's interrupt: the scheduler's tick (timer 1), in about 60 cycles plus the sleepers due: counts it
; and wakes the sleepers whose time has come (in the system task, by a quick look: VIA_IRQ_HANDLER and
; SLEEP_CHECK, without the dispatcher), then asks the dispatcher for a task switch (IRQ_TICK), which lets
; IRQs in while it picks the task (SCHED_PICK).  Anything else the VIA raises (timer 2, for the WDC ACIA)
; goes to its registered handlers, the usual way.
VIA_IRQ_FAST:
            lda         VIA_R_INT_FLAGS
.if ::SER_ACIA = ::SER_ACIA_WDC
            bit         #VIA_T2_INT_BIT                     ; (Timer 2 as well: all to the handlers)
            bne         @others
.else
            bit         #VIA_T2_INT_BIT                     ; Timer 2: paced sending (SER_T2_FAST)
            bne         SER_T2_FAST
.endif
            and         #VIA_T1_INT_BIT
            beq         @others
            lda         VIA_R_T1C_L                         ; Clears it
            ldy         T_REGISTER
            stz         T_REGISTER                          ; Quick switch to the system task (no stack use!)
            sty         ZP_TICK_T
            stx         ZP_TICK_W
            inc         ZP_TICKS                            ; Count it (TICKS_GET)
            bne         :+
            inc         ZP_TICKS + 1
:
            dec         ZP_CLOCK_SUB                        ; The clock (CLOCK_GET): a second every
            bne         :+                                  ;   SCHED_TICK_HZ ticks
            lda         #SCHED_TICK_HZ
            sta         ZP_CLOCK_SUB
            inc         ZP_CLOCK
            bne         :+
            inc         ZP_CLOCK + 1
            bne         :+
            inc         ZP_CLOCK + 2
            bne         :+
            inc         ZP_CLOCK + 3
:
            lda         ZP_SLEEPERS                         ; The sleepers (usually none)
            sta         ZP_SLEEP_SCAN
            lda         ZP_SLEEPERS + 1
            sta         ZP_SLEEP_SCAN + 1
            ldx         #0                                  ; .X = task

@sleeper:
            lda         ZP_SLEEP_SCAN
            ora         ZP_SLEEP_SCAN + 1
            beq         @slept
            lsr         ZP_SLEEP_SCAN + 1
            ror         ZP_SLEEP_SCAN
            bcc         @next
            stx         T_REGISTER                          ; Quick look at the sleeper (no stack use!)
            lda         ZP_SLEEP_UNTIL
            ldy         ZP_SLEEP_UNTIL + 1
            stz         T_REGISTER                          ; (Back in the system task)
            clc
            sbc         ZP_TICKS                            ; The time - now - 1: negative once it's come
            tya
            sbc         ZP_TICKS + 1
            bpl         @next
            txa                                             ; Not sleeping any more
            and         #7
            tay
            lda         P2_BIT_MASKS,Y
            cpx         #8
            bcs         :+
            trb         ZP_SLEEPERS
            bra         :++
:
            trb         ZP_SLEEPERS + 1
:
            stx         T_REGISTER                          ; Wake it (as IO_WAKE: no stack use)
            rmb2        TASK_STATUS_REG                     ; (TASK_WAITING_FLAG)
            stz         T_REGISTER

@next:
            inx
            bra         @sleeper

@slept:
            ldx         ZP_TICK_W
            ldy         ZP_TICK_T
            sty         T_REGISTER                          ; Back to the interrupted task (and its stack)
            ply
            lda         #IRQ_TICK                           ; The dispatcher: a task switch, if it's time
            jmp         IRQ_FAST_SLOW

@others:
            ply
            lda         #0                                  ; (The VIA's logical IRQ#)
            jmp         IRQ_FAST_SLOW

.assert     SER_PEND_WAKE_RD = 1 .and SER_PEND_WAKE_WR = 2 .and SER_PEND_BREAK = 4 .and SER_PEND_KILL = 8 .and SER_PEND_CMD = $10, error, "SER_IRQ_FAST sets SER_PEND's bits with smb0-4"

; IO_FLUSH's fast path for /dev/cons (IO_FDF_CONS): from the foreground task, the stdout buffer goes straight
; into the TX ring (or the ACIA, when it's idle), a byte at a time with IRQs off, as /dev/cons would put it.
; IN: STDOUT_BUF, ZP_OUT_CNT bytes.  OUT: .Y = the bytes taken (it stops at the first it can't: not the
; foreground task, or the ring is full; the IO layer does the rest).  Modifies: .A, .X, ZP_IO_TMP
SER_CONS_PUTS:
            ldy         #0

@next:
            cpy         ZP_OUT_CNT
            beq         @done
            lda         STDOUT_BUF,Y                        ; (This task's RAM: before the switch)
            sty         ZP_IO_TMP
            php
            sei
            ldx         T_REGISTER
            ldy         #SERIAL_TASK_NUM
            sty         T_REGISTER                          ; Quick switch to the serial task (no stack use!)
            cpx         ZP_SER_CAPTURE
            bne         @stop                               ; Not the foreground task
            ldy         ZP_SER_SEND_STATUS
            bne         @queue                              ; Busy: the TX IRQ sends it
            _M_SER_TX_BYTE                                  ; Idle (so the ring is empty): send it now
            inc         ZP_SER_SEND_STATUS                  ; SER_SEND_STATUS_BUSY
            bra         @taken

@queue:
            ldy         SER_TX_HEAD
            sta         SER_TX_BUF,Y
            iny
            cpy         SER_TX_TAIL
            beq         @stop                               ; The ring is full (the byte stored isn't counted)
            sty         SER_TX_HEAD

@taken:
            stx         T_REGISTER                          ; Back to this task
            plp
            ldy         ZP_IO_TMP
            iny
            bra         @next

@stop:
            stx         T_REGISTER                          ; Back to this task
            plp
            ldy         ZP_IO_TMP

@done:
            rts

; STDIN_GET's fast path for /dev/cons (IO_FDF_CONS): from the foreground task, the next key straight from
; the RX ring, echoed into the TX ring as /dev/cons would.  Only plain keys: the end-of-input keys, BS and
; DEL (erased on the screen) are left to /dev/cons, as is an empty ring (it waits) and a full TX ring.
; OUT: C = 0, .A = the key; or C = 1 (use the IO layer).  Preserves .Y.  Modifies: .X
SER_CONS_GETC:
            phy
            php
            sei
            ldx         T_REGISTER
            ldy         #SERIAL_TASK_NUM
            sty         T_REGISTER                          ; Quick switch to the serial task (no stack use!)
            cpx         ZP_SER_CAPTURE
            bne         @no                                 ; Not the foreground task
            ldy         SER_RX_TAIL
            cpy         SER_RX_HEAD
            beq         @no                                 ; Nothing typed yet
            lda         SER_RX_BUF,Y
            cmp         #SER_KEY_EOF
            beq         @no
            cmp         #SER_KEY_EOF2
            beq         @no
            cmp         #ASCII_BACKSPACE
            beq         @no
            cmp         #ASCII_DEL
            beq         @no
            ldy         ZP_SER_SEND_STATUS                  ; The echo
            bne         @queue
            _M_SER_TX_BYTE                                  ; Idle: straight to the ACIA
            inc         ZP_SER_SEND_STATUS                  ; SER_SEND_STATUS_BUSY
            bra         @take

@queue:
            ldy         SER_TX_HEAD
            sta         SER_TX_BUF,Y
            iny
            cpy         SER_TX_TAIL
            beq         @no                                 ; The TX ring is full: /dev/cons waits for room
            sty         SER_TX_HEAD

@take:
            inc         SER_RX_TAIL                         ; Taken
            stx         T_REGISTER                          ; Back to this task
            plp
            ply
            clc
            rts

@no:
            stx         T_REGISTER                          ; Back to this task
            plp
            ply
            sec
            rts
