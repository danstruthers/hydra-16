; ****************************************************************************
; task.s - starting and ending tasks, and the module directory (docs/reimplementation-from-scratch.md, §10.6, §11).
;
; A module (HYX2: layout.inc) in the paged ROM runs in place: its task's ROM bank register selects its bank, so
; its code is at $A000 in that task alone; its initialised data is copied into the task's RAM, and its BSS
; cleared.  A program starts at K_TASK_MAIN (its entry point with r0 = its arguments; EXITS 0 if it returns).
; A driver starts at K_DRIVER_MAIN: its init, then it's ST_IDLE, running only for calls and interrupts; while
; its init runs it's busy (TK_BUSY), and calls to it wait.  Programs take the lowest free task (init is first:
; task 1); drivers the highest (task F first: the DS1747's registers are task F's top bytes, and a driver's
; RAM stops below them).
;
; An ended task's exit record (its code and message) waits in the kernel task for its parent's WAIT, and the
; task isn't used again till then (a parent that ends first leaves its children, and their records, to init).
; The task table, the parents and the exit records are the kernel task's (K_*: layout.inc): changed in KCALLs.

.include "kdefs.inc"

.segment "KCODE"

; ****************************************************************************
; The module directory (paged ROM bank 0: the kernel task's ROM bank is always 0)

; C = 0 if there's a module directory.  Modifies .A, .X
K_MD_VALID:
            ldx         #3
:
            lda         MD_MAGIC,X
            cmp         K_STR_HYMD,X
            bne         @no
            dex
            bpl         :-
            clc
            rts

@no:
            sec
            rts

; Find the module named K_NAMEBUF (12 bytes, zero-padded).  OUT: C = 0, K0_PTR = its entry; or C = 1, .A = E_NOENT.
; Modifies .A, .X, .Y, K0_TMP
K_MD_FIND:
            jsr         K_MD_VALID
            bcs         @none
            lda         MD_COUNT
            beq         @none
            sta         K0_TMP
            lda         #<(MD_ENTRIES + ME_NAME)
            sta         K0_PTR
            lda         #>(MD_ENTRIES + ME_NAME)
            sta         K0_PTR + 1
@entry:
            ldy         #0
@char:
            lda         (K0_PTR),Y
            cmp         K_NAMEBUF,Y
            bne         @next
            cmp         #0
            beq         @found                              ; (Both ended)
            iny
            cpy         #HX_NAME_MAX + 1
            bne         @char
@found:
            lda         K0_PTR                              ; Back to the entry's start
            sec
            sbc         #ME_NAME
            sta         K0_PTR
            bcs         :+
            dec         K0_PTR + 1
:
            clc
            rts

@next:
            lda         K0_PTR
            clc
            adc         #ME_SIZE
            sta         K0_PTR
            bcc         :+
            inc         K0_PTR + 1
:
            dec         K0_TMP
            bne         @entry
@none:
            FAIL        E_NOENT

; ****************************************************************************
; Starting a module

; Start the module at directory entry K0_PTR in a task of its own (ST_NEW: K_TASK_GO makes it run), with no
; parent.  In the kernel task (a KCALL, or the boot).  OUT: C = 0, .A = the task; or C = 1, .A = E_NOEXEC,
; E_NOTASK.  Keeps K0_PTR
K_START_MODULE:
            ldy         #ME_TYPE
            lda         (K0_PTR),Y
            sta         K0_NEWTYPE
            cmp         #HT_PROGRAM
            beq         @program
            cmp         #HT_DRIVER
            bne         @noexec
            lda         #1                                  ; (A driver: from the top)
            bra         @reserve

@program:
            lda         #0                                  ; (A program: from the bottom)
@reserve:
            jsr         K_TASK_RESERVE
            bcs         @done
            sta         K0_NEW
            ldy         #ME_BANK
            lda         (K0_PTR),Y
            sta         K0_NEWBANK
            jsr         K_TASK_SETUP
            bcs         @bad
            ldx         K0_NEW
            lda         K0_NEWBANK
            sta         K_TASK_BANK,X
            lda         K0_NEWTYPE
            sta         K_TASK_TYPE,X
            lda         #$FF
            sta         K_PARENT,X
            txa
            clc
@done:
            rts

@bad:                                                       ; Not a module: the task is free again
            pha
            ldx         K0_NEW
            ldy         T_REGISTER
            php
            sei
            lda         #ST_FREE
            QL_PUT      TK_STATE
            plp
            pla
            sec
            rts

@noexec:
            FAIL        E_NOEXEC

; Reserve a free task (ST_NEW).  IN: .A = 0: the lowest (from task 1); else the highest (from task F down to 2).
; In the kernel task.  OUT: C = 0, .A = the task; or C = 1, .A = E_NOTASK.  Modifies .X, .Y
K_TASK_RESERVE:
            ldy         T_REGISTER
            cmp         #0
            bne         @down
            ldx         #INIT_TASK
@up:
            jsr         @try
            bcc         @got
            inx
            cpx         #TASKS
            bne         @up
            FAIL        E_NOTASK

@down:
            ldx         #TASKS - 1
@dn:
            jsr         @try
            bcc         @got
            dex
            cpx         #INIT_TASK
            bne         @dn
            FAIL        E_NOTASK

@got:
            txa
            clc
            rts

@try:                                                       ; Task .X: free, its exit record taken?  Then it's ours
            lda         K_EXIT_STATE,X
            bne         @no
            php
            sei
            stx         T_REGISTER                          ; A quick look
            lda         TK_STATE
            bne         @taken
            lda         #ST_NEW
            sta         TK_STATE
            sty         T_REGISTER
            plp
            clc
            rts

@taken:
            sty         T_REGISTER
            plp
@no:
            sec
            rts

; Set task K0_NEW up to run the module in bank K0_NEWBANK, a K0_NEWTYPE: its banks, its entries and its name (from
; the module's header), its OS zero page, its data copied, its BSS cleared, its first frame.  In the kernel task;
; keeps the I flag (the boot's are off).  It's in the new task's memory in short steps, with IRQs off for each
; and a moment between.  OUT: C = 0; or C = 1, .A = E_NOEXEC (not a module this kernel can run).
; Modifies .A, .X, .Y
K_TASK_SETUP:
            ldx         K0_NEW
            lda         K0_NEWBANK
            php
            sei
            stx         T_REGISTER                          ; ---- The new task (not its stack: no jsr, no pha)
            sta         ROM_BANK                            ; Its module, at $A000
            sta         TA_MODBANK
            stz         RAM_BANK
            ldy         #3
:
            lda         PROM_WINDOW + HX_MAGIC,Y
            cmp         K_STR_HYX2,Y
            bne         @bad
            dey
            bpl         :-
            lda         PROM_WINDOW + HX_ABI
            beq         @bad
            cmp         #ABI_VERSION + 1
            bcc         @header                             ; (Not built for a later kernel)
@bad:
            stz         T_REGISTER
            plp
            FAIL        E_NOEXEC

@header:
            lda         #FRAME_SP                           ; Its OS zero page
            sta         TK_SP
            stz         TK_FLAGS
            stz         TK_PREEMPT
            stz         TK_DUE
            stz         TK_BUSY
            stz         T_REGISTER                          ; ---- Back (a moment)
            plp
            php
            sei
            stx         T_REGISTER                          ; ---- The new task
            lda         PROM_WINDOW + HX_MAIN               ; Its entries
            sta         TA_ENTRY
            lda         PROM_WINDOW + HX_MAIN + 1
            sta         TA_ENTRY + 1
            lda         PROM_WINDOW + HX_SERVE
            sta         TA_SERVEVEC
            lda         PROM_WINDOW + HX_SERVE + 1
            sta         TA_SERVEVEC + 1
            lda         PROM_WINDOW + HX_IRQ
            sta         TA_IRQVEC
            lda         PROM_WINDOW + HX_IRQ + 1
            sta         TA_IRQVEC + 1
            lda         PROM_WINDOW + HX_SERVE + 1          ; No serve entry: it serves no calls
            bne         :+
            lda         #BUSY_NOSERVE
            sta         TK_BUSY
:
            stz         T_REGISTER                          ; ---- Back (a moment)
            plp
            ldy         #0                                  ; Its name, 4 bytes at a time
@name:
            php
            sei
            stx         T_REGISTER                          ; ---- The new task
            .repeat     4
            lda         PROM_WINDOW + HX_NAME,Y
            sta         TA_NAME,Y
            iny
            .endrepeat
            stz         T_REGISTER                          ; ---- Back (a moment)
            plp
            cpy         #HX_NAME_MAX + 1
            bne         @name
            php
            sei
            stx         T_REGISTER                          ; ---- The new task
            lda         PROM_WINDOW + HX_DATA_LOAD          ; Its data: r0 from, r1 to, r2 bytes (its own
            sta         r0                                  ;   r-registers: it isn't running yet)
            lda         PROM_WINDOW + HX_DATA_LOAD + 1
            sta         r0 + 1
            lda         PROM_WINDOW + HX_DATA_RUN
            sta         r1
            lda         PROM_WINDOW + HX_DATA_RUN + 1
            sta         r1 + 1
            lda         PROM_WINDOW + HX_DATA_LEN
            sta         r2
            lda         PROM_WINDOW + HX_DATA_LEN + 1
            sta         r2 + 1
            stz         T_REGISTER                          ; ---- Back
            plp
            ldy         #0
            jsr         K_TASK_FILL
            ldx         K0_NEW
            php
            sei
            stx         T_REGISTER                          ; ---- Its BSS: r1, r2 bytes
            lda         PROM_WINDOW + HX_BSS
            sta         r1
            lda         PROM_WINDOW + HX_BSS + 1
            sta         r1 + 1
            lda         PROM_WINDOW + HX_BSS_LEN
            sta         r2
            lda         PROM_WINDOW + HX_BSS_LEN + 1
            sta         r2 + 1
            stz         T_REGISTER                          ; ---- Back
            plp
            ldy         #1
            jsr         K_TASK_FILL
            ldx         K0_NEW                              ; Its first frame: where it starts (and a driver's flags)
            ldy         T_REGISTER
            php
            sei
            lda         K0_NEWTYPE
            cmp         #HT_DRIVER
            bne         @program
            lda         #TF_DRIVER
            QL_PUT      TK_FLAGS
            QL_GET      TK_BUSY                             ; (Starting: calls to it wait, if it serves any)
            bmi         :+
            lda         #1
            QL_PUT      TK_BUSY
:
            lda         #<K_DRIVER_MAIN
            QL_PUT      $0100 + FRAME_SP + FR_PCL
            lda         #>K_DRIVER_MAIN
            QL_PUT      $0100 + FRAME_SP + FR_PCH
            bra         @frame

@program:
            lda         #<K_TASK_MAIN
            QL_PUT      $0100 + FRAME_SP + FR_PCL
            lda         #>K_TASK_MAIN
            QL_PUT      $0100 + FRAME_SP + FR_PCH
@frame:
            stx         T_REGISTER                          ; ---- The rest of the frame: U, Y, W, X, A 0; P 0 (IRQs on)
            stz         $0100 + FRAME_SP + FR_U
            stz         $0100 + FRAME_SP + FR_Y
            stz         $0100 + FRAME_SP + FR_W
            stz         $0100 + FRAME_SP + FR_X
            stz         $0100 + FRAME_SP + FR_A
            stz         $0100 + FRAME_SP + FR_P
            sty         T_REGISTER                          ; ---- Back
            plp
            clc
            rts

; In task K0_NEW (it isn't running yet: its r-registers are ours to use): r2 bytes to (r1), from (r0) (.Y = 0) or
; zeros (.Y <> 0).  A byte at a time with IRQs off, the caller's I flag between.  In the kernel task.
; Modifies .A, .X
K_TASK_FILL:
            ldx         K0_NEW
@byte:
            php
            sei
            stx         T_REGISTER                          ; ---- The task
            lda         r2
            ora         r2 + 1
            beq         @done
            tya
            bne         @zero
            lda         (r0)                                ; (Its module, at $A000)
            inc         r0
            bne         @put
            inc         r0 + 1
            bra         @put

@zero:
            lda         #0
@put:
            sta         (r1)                                ; (Its RAM)
            inc         r1
            bne         :+
            inc         r1 + 1
:
            lda         r2
            bne         :+
            dec         r2 + 1
:
            dec         r2
            stz         T_REGISTER                          ; ---- Back (a moment for interrupts)
            plp
            bra         @byte

@done:
            stz         T_REGISTER
            plp
            rts

; A task set up: it runs now.  IN: .A = the task.  From any task.  Keeps .A
K_TASK_GO:
            tax
            ldy         T_REGISTER
            php
            sei
            lda         #ST_READY
            QL_PUT      TK_STATE
            plp
            txa
            rts

; Every program's first instructions (its first frame's PC): on page 0, IRQs on.  Its entry point with r0 = its
; arguments; if it returns, EXITS with code 0
K_TASK_MAIN:
            lda         #<TA_ARGS
            sta         r0
            lda         #>TA_ARGS
            sta         r0 + 1
            jsr         @entry
            stz         r0
            stz         r0 + 1
            lda         #0
            jmp         K_EXITS

@entry:
            jmp         (TA_ENTRY)

; Every driver's: its init (C = 0, or C = 1 and .A = an error), then idle: it runs for calls and interrupts only
K_DRIVER_MAIN:
            jsr         @entry
            bcs         @failed
            sei
            lda         TK_BUSY                             ; (Started: it serves calls, and the callers blocked
            and         #BUSY_NOSERVE                       ;   on it can run)
            sta         TK_BUSY
            lda         #ST_IDLE
            sta         TK_STATE
            cli
@idle:
            jsr         K_YIELD                             ; (ST_IDLE: never picked to run this again)
            bra         @idle

@failed:
            stz         r0
            stz         r0 + 1
            jmp         K_EXITS                             ; (With init's error as its code)

@entry:
            jmp         (TA_ENTRY)

; ****************************************************************************
; At boot, in the kernel task, IRQs off: the boot drivers (each with HF_BOOT), then init (the directory's
; MD_INIT), each said on the console: "task F: cons", or "module NAME: error $xx"
K_TASK_BOOT:
            jsr         K_MD_VALID
            bcc         :+
            KPRINT      K_STR_NOMODS
            rts
:
            lda         MD_COUNT
            beq         @init
            sta         K0_TMP3
            lda         #<MD_ENTRIES
            sta         K0_PTR
            lda         #>MD_ENTRIES
            sta         K0_PTR + 1
@entry:
            ldy         #ME_TYPE
            lda         (K0_PTR),Y
            cmp         #HT_DRIVER
            bne         @next
            ldy         #ME_FLAGS
            lda         (K0_PTR),Y
            and         #HF_BOOT
            beq         @next
            jsr         @start
@next:
            lda         K0_PTR
            clc
            adc         #ME_SIZE
            sta         K0_PTR
            bcc         :+
            inc         K0_PTR + 1
:
            dec         K0_TMP3
            bne         @entry
@init:
            lda         MD_INIT
            cmp         MD_COUNT
            bcs         @noinit
            asl                                             ; Its entry: MD_ENTRIES + 16 * MD_INIT
            asl
            asl
            asl
            clc
            adc         #<MD_ENTRIES
            sta         K0_PTR
            lda         #0
            adc         #>MD_ENTRIES
            sta         K0_PTR + 1
            jmp         @start

@noinit:
            KPRINT      K_STR_NOINIT
            rts

@start:                                                     ; Start the module at K0_PTR, and say so
            jsr         K_START_MODULE
            bcs         @failed
            jsr         K_TASK_GO
            KPRINT      K_STR_TASK
            jsr         K_PUTNIB
            KPRINT      K_STR_COLON
            jsr         @name
            KPRINT      K_STR_CRLF
            rts

@failed:
            KPRINT      K_STR_MODULE
            jsr         @name
            KPRINT      K_STR_ERROR
            jsr         K_PUTHEX
            KPRINT      K_STR_CRLF
            rts

@name:                                                      ; Its name, from the directory.  Keeps .A
            pha
            lda         K0_PTR
            clc
            adc         #ME_NAME
            sta         r0
            lda         K0_PTR + 1
            adc         #0
            sta         r0 + 1
            pla
            jmp         K_PUTSTR

; ****************************************************************************
; SPAWN: start a program.  IN: r0 = "#m/NAME"; r1 = its arguments (zero-terminated, up to 255 characters), or 0;
; .A = flags (0).  OUT: C = 0, .A = the task; or C = 1, .A = E_NOENT, E_NAMETOOLONG, E_TOOBIG, E_NOEXEC, E_NOTASK
K_SPAWN:
            ldy         #2                                  ; "#m/"
:
            lda         (r0),Y
            cmp         K_STR_MODPATH,Y
            bne         @noent
            dey
            bpl         :-
            bra         @path

@noent:
            FAIL        E_NOENT

@path:
            ldy         #3                                  ; The name, into TA_SCRATCH (12 bytes, zero-padded)
            ldx         #0
@name:
            lda         (r0),Y
            sta         TA_SCRATCH,X
            beq         @pad
            iny
            inx
            cpx         #HX_NAME_MAX + 1
            bne         @name
            FAIL        E_NAMETOOLONG

@pad:
            cpx         #HX_NAME_MAX + 1
            beq         @args
            stz         TA_SCRATCH,X
            inx
            bra         @pad

@args:                                                      ; Its arguments' length with the 0 (checked first)
            stz         K_CNT
            stz         K_CNT + 1
            lda         r1
            ora         r1 + 1
            beq         @start
            ldy         #0
:
            lda         (r1),Y
            beq         :+
            iny
            bne         :-
            FAIL        E_TOOBIG

:
            iny
            sty         K_CNT
            bne         @start
            inc         K_CNT + 1                           ; (255 characters and the 0: 256)
@start:
            KCALL       K_SPAWN_K                           ; .A = the task, set up
            bcs         @done
            sta         K_Y
            lda         K_CNT                               ; Its arguments, into its TA_ARGS
            ora         K_CNT + 1
            beq         @noargs
            lda         r1
            sta         K_PTR
            lda         r1 + 1
            sta         K_PTR + 1
            lda         #<TA_ARGS
            sta         K_PTR2
            lda         #>TA_ARGS
            sta         K_PTR2 + 1
            lda         K_Y
            clc
            jsr         K_KCOPY
            bra         @go

@noargs:
            ldx         K_Y
            ldy         T_REGISTER
            php
            sei
            lda         #0
            QL_PUT      TA_ARGS
            plp
@go:
            lda         K_Y
            jsr         K_TASK_GO
            clc
@done:
            rts

; In the kernel task (KCALL): the module named in the caller's TA_SCRATCH, started; the caller its parent.
; IN: .Y = the caller
K_SPAWN_K:
            sty         K0_TMP3
            lda         #<K_NAMEBUF                         ; Its name, from the caller
            sta         K_PTR
            lda         #>K_NAMEBUF
            sta         K_PTR + 1
            lda         #<TA_SCRATCH
            sta         K_PTR2
            lda         #>TA_SCRATCH
            sta         K_PTR2 + 1
            lda         #HX_NAME_MAX + 1
            sta         K_CNT
            stz         K_CNT + 1
            tya
            sec
            jsr         K_KCOPY
            jsr         K_MD_FIND
            bcs         @done
            ldy         #ME_TYPE                            ; (Programs only: drivers start at boot)
            lda         (K0_PTR),Y
            cmp         #HT_PROGRAM
            bne         @noexec
            jsr         K_START_MODULE
            bcs         @done
            tax
            lda         K0_TMP3
            sta         K_PARENT,X
            txa
            clc
@done:
            rts

@noexec:
            FAIL        E_NOEXEC

; ****************************************************************************
; EXITS: end this task.  IN: .A = the exit code; r0 = a message (31 characters at most), or 0.  Doesn't return
K_EXITS:
            jsr         K_PREEMPT_OFF                       ; (Nothing else runs till it's done)
            sta         K_A
            ldy         #0                                  ; The message, into TA_SCRATCH (for the kernel task)
            lda         r0
            ora         r0 + 1
            beq         @end
@msg:
            lda         (r0),Y
            beq         @end
            sta         TA_SCRATCH,Y
            iny
            cpy         #EXIT_MSG_MAX
            bne         @msg
@end:
            lda         #0
            sta         TA_SCRATCH,Y
            lda         K_A
            KCALL       K_EXIT_K                            ; Its lines, its exit record; its children init's; its
                                                            ;   parent woken
            sei
            stz         TK_STATE                            ; (ST_FREE: never run again)
            stz         TK_FLAGS
            stz         TK_PREEMPT
            stz         TK_BUSY                             ; (Callers blocked on it look, and find it gone)
            jsr         K_YIELD
@gone:
            bra         @gone

; In the kernel task (KCALL): task .Y has ended with code .A, its message in its TA_SCRATCH.  Its IRQ lines,
; nobody's; its record, for its parent (none if it has none); its children and their records, init's (nobody's,
; if it's init)
K_EXIT_K:
            sta         K_EXIT_CODE,Y
            sty         K0_TMP
            jsr         IRQ_RELEASE_ALL                     ; Its lines
            stz         K0_TMP3                             ; (Records moved to init: it's woken too)
            lda         #INIT_TASK                          ; Its children's new parent
            cpy         #INIT_TASK
            bne         :+
            lda         #$FF
:
            sta         K0_TMP2
            lda         K_PARENT,Y
            sta         K_EXIT_PARENT,Y
            bmi         @orphans                            ; (No parent: no record)
            lda         #EX_HELD
            sta         K_EXIT_STATE,Y
            tya                                             ; Its message: to K_EXIT_MSG + 32 * the task
            jsr         K_EXIT_MSG_AT
            lda         K_PTR2
            sta         K_PTR
            lda         K_PTR2 + 1
            sta         K_PTR + 1
            lda         #<TA_SCRATCH
            sta         K_PTR2
            lda         #>TA_SCRATCH
            sta         K_PTR2 + 1
            lda         #EXIT_MSG_MAX + 1
            sta         K_CNT
            stz         K_CNT + 1
            lda         K0_TMP
            sec
            jsr         K_KCOPY
@orphans:
            ldx         #TASKS - 1
@kid:
            lda         K_PARENT,X
            cmp         K0_TMP
            bne         :+
            lda         K0_TMP2
            sta         K_PARENT,X
:
            lda         K_EXIT_STATE,X
            beq         @next
            lda         K_EXIT_PARENT,X
            cmp         K0_TMP
            bne         @next
            lda         K0_TMP2
            sta         K_EXIT_PARENT,X
            sta         K0_TMP3
            bpl         @next
            stz         K_EXIT_STATE,X                      ; (Nobody can wait for it: gone)
@next:
            dex
            bpl         @kid
            ldy         K0_TMP
            lda         K_PARENT,Y                          ; Its parent, woken (it may be waiting for it)
            bmi         :+
            jsr         K_WAKE
:
            lda         K0_TMP3                             ; And init, if it has records now
            beq         :+
            bmi         :+
            jsr         K_WAKE
:
            lda         #$FF
            sta         K_PARENT,Y
            sta         K_TASK_BANK,Y
            lda         #0
            sta         K_TASK_TYPE,Y
            clc
            rts

; K_PTR2 = K_EXIT_MSG + 32 * task .A (the kernel task's address of its exit message)
K_EXIT_MSG_AT:
            stz         K_PTR2 + 1
            asl
            asl
            asl
            asl
            rol         K_PTR2 + 1
            asl
            rol         K_PTR2 + 1
            clc
            adc         #<K_EXIT_MSG
            sta         K_PTR2
            lda         K_PTR2 + 1
            adc         #>K_EXIT_MSG
            sta         K_PTR2 + 1
            rts

; ****************************************************************************
; WAIT: wait for a child to end.  IN: .A = the child, or $FF for any; r0 = a buffer for its message (32 bytes), or
; 0.  OUT: C = 0, .A = the task, .X = its exit code; or C = 1, .A = E_CHILD
K_WAIT:
            jsr         K_PREEMPT_OFF                       ; (So a child can't end between a look and the wait)
            sta         K_Y                                 ; (Not K_A: a KCALL's own)
@look:
            lda         K_Y
            KCALL       K_WAIT_K
            bcc         @got
            cmp         #E_AGAIN
            bne         @fail
            jsr         K_PAUSE                             ; Till a child ends (it wakes its parent)
            bra         @look

@got:
            sta         K_A
            stx         K_X
            lda         r0                                  ; Its message, if it's wanted
            ora         r0 + 1
            beq         @done
            lda         r0
            sta         K_PTR
            lda         r0 + 1
            sta         K_PTR + 1
            lda         K_A
            jsr         K_EXIT_MSG_AT
            lda         #EXIT_MSG_MAX + 1
            sta         K_CNT
            stz         K_CNT + 1
            lda         #KERNEL_TASK
            sec
            jsr         K_KCOPY
@done:
            jsr         K_PREEMPT_ON
            lda         K_A
            ldx         K_X
            clc
            rts

@fail:
            jsr         K_PREEMPT_ON                        ; (Keeps .A and C)
            rts

; In the kernel task (KCALL): a held exit record of the caller's (task .Y's) child .A ($FF: any).  OUT: C = 0,
; .A = the child, .X = its code (the record released); or C = 1, .A = E_AGAIN (a child is still running) or E_CHILD
K_WAIT_K:
            sta         K0_TMP2
            sty         K0_TMP
            ldx         #TASKS - 1
@record:
            lda         K_EXIT_STATE,X
            beq         @next
            lda         K_EXIT_PARENT,X
            cmp         K0_TMP
            bne         @next
            lda         K0_TMP2
            bmi         @take                               ; (Any)
            cpx         K0_TMP2
            bne         @next
@take:
            stz         K_EXIT_STATE,X                      ; (EX_NONE: waited for; the task can be used again)
            lda         K_EXIT_CODE,X
            pha
            txa
            plx
            clc
            rts

@next:
            dex
            bpl         @record
            ldx         #TASKS - 1                          ; None ended: is one still running?
@alive:
            lda         K_PARENT,X
            cmp         K0_TMP
            bne         :+
            lda         K0_TMP2
            bmi         @again
            cpx         K0_TMP2
            beq         @again
:
            dex
            bpl         @alive
            FAIL        E_CHILD

@again:
            FAIL        E_AGAIN

; GETPID: .A = this task
K_GETPID:
            lda         T_REGISTER
            and         #TASKS - 1
            clc
            rts

.segment "KRODATA"
K_STR_HYMD:     .byte   "HYMD"
K_STR_HYX2:     .byte   "HYX2"
K_STR_MODPATH:  .byte   "#m/"
K_STR_NOMODS:   .byte   "no modules (no directory in paged ROM bank 0)", CR, LF, 0
K_STR_NOINIT:   .byte   "no init", CR, LF, 0
K_STR_TASK:     .byte   "task ", 0
K_STR_COLON:    .byte   ": ", 0
K_STR_MODULE:   .byte   "module ", 0
K_STR_ERROR:    .byte   ": error $", 0
K_STR_CRLF:     .byte   CR, LF, 0
