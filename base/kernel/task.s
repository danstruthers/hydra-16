; ****************************************************************************
; task.s - starting and ending tasks, and the module directory (docs/design/reimplementation-from-scratch.md, Â§10.6, Â§11).
;
; A module (HYX2: layout.inc) in the paged ROM runs in place: its task's ROM bank register selects its bank, so
; its code is at $A000 in that task alone; its initialised data is copied into the task's RAM, and its BSS
; cleared, by the task itself as it starts (K_TASK_DATA).  A program starts at K_TASK_MAIN (its entry point with r0
; = its arguments; EXITS 0 if it returns); a RAM program (load.s) at K_TASK_LOAD, which loads it first.
; A driver starts at K_DRIVER_MAIN: its init, then it's ST_IDLE, running only for calls and interrupts; while
; its init runs it's busy (TK_BUSY), and calls to it wait.  Programs take the lowest free task (init is first:
; task 1); drivers the highest (task F first: the DS1747's registers are task F's top bytes, and a driver's
; RAM stops below them).
;
; An ended task's exit record (its code and message) waits in the kernel task for its parent's WAIT, and the
; task isn't used again till then (a parent that ends first leaves its children, and their records, to init).
; The task table, the parents and the exit records are the kernel task's (K_*: layout.inc): changed in KCALLs.
;
; What runs in the kernel task (the KCALLs, setting a task up, the boot) is on BIOS ROM page 1; what runs in the
; calling task (EXITS, WAIT) and the tasks' first instructions, on page 0; SPAWN's side and the loader, on page 3
; (load.s).

.include "kdefs.inc"

; Print a string of this page's on the bring-up console (t_putstr).  Keeps .A, .X, .Y
.macro TPRINT   label
            pha
            lda         #<label
            sta         r0
            lda         #>label
            sta         r0 + 1
            pla
            jsr         t_putstr
.endmacro

.segment "KCODE_P1"

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

; MODINFO: the module directory's entry .A (ME_*: its bank, type, flags and name), into r0 (ME_SIZE bytes).
; OUT: C = 0; or C = 1, .A = E_NOENT (past the last)
K_MODINFO:
            KCALL_FAR   K_MODINFO_K                         ; (Into TA_SCRATCH)
            bcs         @done
            ldy         #ME_SIZE - 1
:
            lda         TA_SCRATCH,Y
            sta         (r0),Y
            dey
            bpl         :-
            clc
@done:
            rts

; In the kernel task (KCALL): entry .A, into the caller's (.Y's) TA_SCRATCH
K_MODINFO_K:
            sty         K0_TMP
            sta         K0_TMP2
            jsr         K_MD_VALID
            bcs         @noent
            lda         K0_TMP2
            cmp         MD_COUNT
            bcs         @noent
            stz         K_PTR + 1                           ; MD_ENTRIES + ME_SIZE * .A
            .repeat     4
            asl
            rol         K_PTR + 1
            .endrepeat
            clc
            adc         #<MD_ENTRIES
            sta         K_PTR
            lda         K_PTR + 1
            adc         #>MD_ENTRIES
            sta         K_PTR + 1
            lda         #<TA_SCRATCH
            sta         K_PTR2
            lda         #>TA_SCRATCH
            sta         K_PTR2 + 1
            lda         #ME_SIZE
            sta         K_CNT
            stz         K_CNT + 1
            lda         K0_TMP
            clc
            FARCALL     K_KCOPY
            clc
            rts

@noent:
            FAIL        E_NOENT

.assert     ME_SIZE = 16, error, "K_MODINFO_K: an entry is 16 bytes"

; ****************************************************************************
; Starting a module

; Start the module at directory entry K0_PTR in a task of its own (ST_NEW: K_TASK_GO makes it run), with no
; parent.  In the kernel task (a KCALL, or the boot).  OUT: C = 0, .A = the task; or C = 1, .A = E_NOEXEC,
; E_NOTASK.  Keeps K0_PTR
K_START_MODULE:
            ldy         #ME_BANK
            lda         (K0_PTR),Y
            sta         K0_NEWBANK
            ldy         #ME_PAGE                            ; (Its header: $A000 + 256 * its page)
            lda         (K0_PTR),Y
            clc
            adc         #>PROM_WINDOW
            sta         K0_NEWBASE
            ldy         #ME_TYPE
            lda         (K0_PTR),Y

; The same for a module of type .A in bank K0_NEWBANK (its header at page K0_NEWBASE); or for a RAM program
; (K0_NEWBANK $FF, its name in K_NAMEBUF), which loads itself as it starts (K_TASK_LOAD)
K_START_TASK:
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
            jsr         K_TASK_SETUP
            bcs         @bad
            ldx         K0_NEW
            FARCALL     K_ENV_CLEAR                         ; (An empty environment: SPAWN copies its parent's)
            lda         K0_NEWBANK
            sta         K_TASK_BANK,X
            lda         K0_NEWTYPE
            sta         K_TASK_TYPE,X
            lda         #$FF
            sta         K_PARENT,X
            stz         K_CPU_LO,X                          ; (No CPU time yet)
            stz         K_CPU_MID,X
            stz         K_CPU_HI,X
            txa
            sta         K_NGROUP,X                          ; (A note group of its own: SPAWN may change it)
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

; Set task K0_NEW up to run the module in bank K0_NEWBANK, its header at page K0_NEWBASE of it, a K0_NEWTYPE: its
; banks, its entries and its name (from the module's header), its OS zero page, its data copied, its BSS cleared, its
; first frame.  A RAM program (K0_NEWBANK $FF) has no header yet: its name is K_NAMEBUF, and the rest is its own, as
; it loads (K_TASK_LOAD).  In the kernel task; keeps the I flag (the boot's are off).  It's in the new task's memory
; in short steps, with IRQs off for each and a moment between; the header is read there through the new task's r0
; (the kernel's zero page isn't there), its own till it runs.  OUT: C = 0; or C = 1, .A = E_NOEXEC (not a module
; this kernel can run).  Modifies .A, .X, .Y
K_TASK_SETUP:
            ldx         K0_NEW
            ldy         K0_NEWBASE
            lda         K0_NEWBANK
            php
            sei
            stx         T_REGISTER                          ; ---- The new task (not its stack: no jsr, no pha)
            sta         TA_MODBANK                          ; Its module, its header at TA_MODBASE's page
            sty         TA_MODBASE
            stz         r0
            sty         r0 + 1
            stz         RAM_BANK
            cmp         #$FF
            bne         :+
            stz         ROM_BANK                            ; (A RAM program: bank 0's at $A000, and no header)
            bra         @header
:
            sta         ROM_BANK
            ldy         #3
:
            lda         (r0),Y                              ; (HX_MAGIC)
            cmp         K_STR_HYX2,Y
            bne         @bad
            dey
            bpl         :-
            ldy         #HX_ABI
            lda         (r0),Y
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
            stz         TK_NOTED                            ; (No notes, no handler)
            stz         TK_NOTES
            stz         TK_NOTES + 1
            stz         TK_NOTES + 2
            stz         TK_NOTES + 3
            stz         TK_INNOTE
            stz         TK_WOKEN
            stz         TK_EVENT
            stz         TK_STEPHIT                          ; (No step out: debug.s)
            stz         TK_RFRAME + 1                       ; (Nothing running in place: run.s)
            stz         TK_RNOTE
            stz         TA_NOTIFY + 1
            stz         T_REGISTER                          ; ---- Back (a moment)
            plp
            lda         #$FF                                ; Its fds: closed (SPAWN gives a program those its map
            php                                             ;   names); its directory: /
            sei
            stx         T_REGISTER                          ; ---- The new task
            .repeat     FD_MAX / 2, I
            sta         TA_FD + I
            .endrepeat
            stz         T_REGISTER                          ; ---- Back (a moment)
            plp
            php
            sei
            stx         T_REGISTER                          ; ---- The new task
            .repeat     FD_MAX / 2, I
            sta         TA_FD + FD_MAX / 2 + I
            .endrepeat
            lda         #'/'
            sta         TA_CWD
            stz         TA_CWD + 1
            stz         TA_ARGS                             ; (No arguments: SPAWN's come after)
            stz         T_REGISTER                          ; ---- Back (a moment)
            plp
            lda         K0_NEWBANK
            cmp         #$FF
            bne         :+
            jmp         @ram
:
            php
            sei
            stx         T_REGISTER                          ; ---- The new task
            ldy         #HX_MAIN                            ; Its entries (main or init, serve, irq: in a row)
            lda         (r0),Y
            sta         TA_ENTRY
            iny
            lda         (r0),Y
            sta         TA_ENTRY + 1
            iny
            lda         (r0),Y
            sta         TA_SERVEVEC
            iny
            lda         (r0),Y
            sta         TA_SERVEVEC + 1
            iny
            lda         (r0),Y
            sta         TA_IRQVEC
            iny
            lda         (r0),Y
            sta         TA_IRQVEC + 1
            lda         TA_SERVEVEC + 1                     ; No serve entry: it serves no calls
            bne         :+
            lda         #BUSY_NOSERVE
            sta         TK_BUSY
:
            stz         T_REGISTER                          ; ---- Back (a moment)
            plp
            ldy         #HX_NAME                            ; Its name, 4 bytes at a time
@name:
            php
            sei
            stx         T_REGISTER                          ; ---- The new task
            .repeat     4
            lda         (r0),Y
            sta         TA_NAME - HX_NAME,Y
            iny
            .endrepeat
            stz         T_REGISTER                          ; ---- Back (a moment)
            plp
            cpy         #HX_NAME + HX_NAME_MAX + 1
            bne         @name                               ; (Its data and BSS: its own, as it starts)
@first:
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
            lda         K0_NEWBANK
            cmp         #$FF
            beq         @load
            lda         #<K_TASK_MAIN
            QL_PUT      $0100 + FRAME_SP + FR_PCL
            lda         #>K_TASK_MAIN
            QL_PUT      $0100 + FRAME_SP + FR_PCH
            bra         @frame

@load:                                                      ; (A RAM program: it loads itself first)
            lda         #<K_TASK_LOAD
            QL_PUT      $0100 + FRAME_SP + FR_PCL
            lda         #>K_TASK_LOAD
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

@ram:                                                       ; A RAM program: no entries yet, no calls served ...
            php
            sei
            stx         T_REGISTER                          ; ---- The new task
            stz         TA_ENTRY
            stz         TA_ENTRY + 1
            stz         TA_SERVEVEC
            stz         TA_SERVEVEC + 1
            stz         TA_IRQVEC
            stz         TA_IRQVEC + 1
            lda         #BUSY_NOSERVE
            sta         TK_BUSY
            stz         T_REGISTER                          ; ---- Back
            plp
            lda         #<K_NAMEBUF                         ; ... its name (its header's, SPAWN's: there as it
            sta         K_PTR                               ;   loads)
            lda         #>K_NAMEBUF
            sta         K_PTR + 1
            lda         #<TA_NAME
            sta         K_PTR2
            lda         #>TA_NAME
            sta         K_PTR2 + 1
            lda         #HX_NAME_MAX + 1
            sta         K_CNT
            stz         K_CNT + 1
            lda         K0_NEW
            clc
            FARCALL     K_KCOPY
            jmp         @first

; A module's task, as it starts (from K_TASK_MAIN and K_DRIVER_MAIN, in the task itself, IRQs on): its data
; copied from its module and its BSS cleared, as its header has them (its first bank is at $A000, the header at its
; page there: TA_MODBASE).  So the kernel task doesn't do it a byte at a time from outside, with IRQs off for each
; (rc's 4.3K of BSS: 257,000 cycles that way, 39,000 this).  Modifies .A, .X, .Y, r0, r1, r2, r3 (the header)
K_TASK_DATA:
            stz         r3                                  ; r3: its header
            lda         TA_MODBASE
            sta         r3 + 1
            ldy         #HX_DATA_LOAD                       ; Its data: r0 from, r1 to, r2 its length (in a row)
            ldx         #0
:
            lda         (r3),Y
            sta         r0,X
            iny
            inx
            cpx         #6
            bne         :-
            ldy         #0
            ldx         r2 + 1                              ; Its whole pages ...
            beq         @part
@page:
            lda         (r0),Y
            sta         (r1),Y
            iny
            bne         @page
            inc         r0 + 1
            inc         r1 + 1
            dex
            bne         @page
@part:
            ldx         r2                                  ; ... and the rest
            beq         @bss
:
            lda         (r0),Y
            sta         (r1),Y
            iny
            dex
            bne         :-
@bss:
            ldy         #HX_BSS                             ; Its BSS: r1, r2 its length (in a row), 4 bytes at a
            ldx         #r1 - r0                            ;   time in whole pages
:
            lda         (r3),Y
            sta         r0,X
            iny
            inx
            cpx         #r1 - r0 + 4
            bne         :-
            lda         #0
            tay
            ldx         r2 + 1
            beq         @bpart
@bpage:
            .repeat     4
            sta         (r1),Y
            iny
            .endrepeat
            bne         @bpage
            inc         r1 + 1
            dex
            bne         @bpage
@bpart:
            ldx         r2
            beq         @done
:
            sta         (r1),Y
            iny
            dex
            bne         :-
@done:
            rts

.assert     HX_DATA_RUN = HX_DATA_LOAD + 2 .and HX_DATA_LEN = HX_DATA_LOAD + 4 .and HX_BSS_LEN = HX_BSS + 2, error, "K_TASK_DATA: the header's fields in a row"
.assert     r1 = r0 + 2 .and r2 = r0 + 4 .and HX_SERVE = HX_MAIN + 2 .and HX_IRQ = HX_MAIN + 4, error, "K_TASK_DATA, K_TASK_SETUP: in a row"

.segment "KCODE"

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

; Every module program's first instructions (its first frame's PC): on page 0, IRQs on.  Its data and BSS
; (K_TASK_DATA); then (K_TASK_START: a RAM program's, once it's loaded) its entry point with r0 = its arguments; if
; it returns, EXITS with code 0
K_TASK_MAIN:
            FARCALL     K_TASK_DATA
K_TASK_START:
            FARCALL     K_MEM_START                         ; (Its break, its maps: mem.s)
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
            lda         #TF_HOLD                            ; SPAWN_STOPPED's?
            bit         TK_FLAGS
            bne         @hold
            jmp         (TA_ENTRY)

@hold:                                                      ; Stopped, its frame's PC its entry point (as YIELD's
            lda         TA_ENTRY + 1                        ;   frame: PC, P, A, X, W, Y, U), for the debugger's
            pha                                             ;   first step
            lda         TA_ENTRY
            pha
            php
            sei
            pha
            phx
            lda         W_REGISTER
            pha
            phy
            lda         U_REGISTER
            pha
            lda         TK_FLAGS
            and         #$FF ^ TF_HOLD
            ora         #TF_STOPPED
            sta         TK_FLAGS
            stz         TK_DUE
            jmp         K_SCHED_SWITCH

; A RAM program's first instructions (its first frame's PC): it loads itself (load.s: K_LOAD), then starts as every
; program does.  One that can't be loaded ends at once, with the error as its code
K_TASK_LOAD:
            FARCALL     K_LOAD
            bcc         K_TASK_START
            stz         r0
            stz         r0 + 1
            jmp         K_EXITS

; Every driver's: its data and BSS (K_TASK_DATA), its init (C = 0, or C = 1 and .A = an error), then idle: it
; runs for calls and interrupts only
K_DRIVER_MAIN:
            FARCALL     K_TASK_DATA
            FARCALL     K_MEM_START                         ; (Its break, its maps: mem.s)
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

; SRV_NAP: a driver's init that doesn't end (a driver with work of its own to do, between its calls: a device to
; look at now and then) waits here .A/.X ticks; or, with 0 ticks, till its own event count (TASK_EVENT: its irq
; entry's, its calls') isn't .Y, the count it last saw.  Meanwhile it serves its calls, as a driver that's started
; does; back, it's busy again (a call to it waits), till its next nap, or its init's end.  OUT: C = 0
K_SRV_NAP:
            pha
            php
            sei
            lda         TK_BUSY                             ; (Its calls served: the callers blocked on it can run)
            and         #BUSY_NOSERVE
            sta         TK_BUSY
            plp
            pla
            bne         @ticks
            cpx         #0
            bne         @ticks
            tya                                             ; Till its own event count changes
            tax
            lda         T_REGISTER
            and         #TASKS - 1
            jsr         K_EVWAIT
            bra         @back
@ticks:
            jsr         K_SLEEP
@back:
            php
            sei
            inc         TK_BUSY                             ; (No call's in it: it runs.  Busy, as it was)
            plp
            clc
            rts

; INIT_RUN: init started again in a task of its own, .A (0-15) its argument: asked of the kernel task, which starts
; it as it next runs (K_INIT_AGAIN).  From a task or an irq entry.  OUT: C = 0
K_INIT_RUN:
            and         #TASKS - 1
            tax
            php
            sei
            ldy         T_REGISTER
            stz         T_REGISTER                          ; ---- The kernel task's
            lda         #1
            sta         K_INIT_WANT,X
            sta         K0_NQ_ANY                           ; (The scheduler runs it: its idle loop looks)
            sty         T_REGISTER                          ; ---- Back
            plp
            clc
            rts

; ****************************************************************************
.segment "KCODE_P1"

; At boot, in the kernel task (FARCALL): the boot drivers (each with HF_BOOT: K_TASK_BOOT, IRQs off), then, once
; they've started (K_BOOT_STARTED), init (the directory's MD_INIT: K_TASK_BOOT_INIT).  Each said on the console:
; "task F: cons", or "module NAME: error $xx"
K_TASK_BOOT:
            ldx         #K0_SCHED_END - K0_FORES - 1        ; (The scheduler's bytes, POST's till now: no
:           stz         K0_FORES,X                          ;   foreground yet, its place task 0's)
            dex
            bpl         :-
            jsr         K_MD_VALID
            bcc         :+
            TPRINT      T_S_NOMODS
            rts
:
            lda         MD_COUNT
            beq         @done
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
            jsr         boot_start
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
@done:
            rts

; Wait (yielding: the kernel task, IRQs on) till each driver has started: none still in its init, so its device
; letters are registered before init needs them.  2 seconds at most (a driver whose init waits for good).  (The
; kernel task can't SLEEP: the scheduler runs it whenever nothing else can run)
K_BOOT_STARTED:
            FARCALL     K_TICKS
            stx         K0_TMP3                             ; (The tick count's high byte: 256 ticks a step)
@look:
            ldx         #TASKS - 1                          ; A driver neither idle nor gone?
@task:
            ldy         T_REGISTER
            php
            sei
            QL_GET      TK_FLAGS
            and         #TF_DRIVER
            beq         :+
            QL_GET      TK_STATE
            cmp         #ST_IDLE
            beq         :+
            cmp         #ST_FREE
            beq         :+
            QL_GET      TK_BUSY                             ; (Or one whose init goes on, between its turns: SRV_NAP)
            bne         @starting
:
            plp
            dex
            bne         @task
            rts

@starting:
            plp
            FARCALL     K_YIELD                             ; (The drivers run meanwhile)
            FARCALL     K_TICKS                             ; 2 seconds gone?  (256-511 ticks)
            txa
            sec
            sbc         K0_TMP3
            cmp         #2
            bcc         @look
            rts

; Init: the directory's MD_INIT, started, with an empty namespace of its own (it builds it) and no arguments.  It's
; the task init's 0 is in (K_INIT_TASK): if it EXECs (a shell in its place) and that ends, init is started again
; (K_EXEC_K, K_EXIT_K, K_INIT_AGAIN); an init that ends itself isn't
K_TASK_BOOT_INIT:
            ldx         #TASKS - 1                          ; (None wanted again yet)
:
            stz         K_INIT_WANT,X
            dex
            bpl         :-
            lda         #$FF
            sta         K_INIT_TASK
            stz         K_INIT_EXEC
            jsr         init_entry
            bcs         @noinit
            ldx         #INIT_TASK                          ; (Before it can run: the kernel task isn't preempted)
            stx         K_INIT_TASK
            FARCALL     K_NS_FRESH
            ldy         T_REGISTER                          ; Its arguments: none
            php
            sei
            lda         #0
            QL_PUT      TA_ARGS
            QL_PUT      TA_ARGS + 1
            plp
            jmp         boot_start

@noinit:
            TPRINT      T_S_NOINIT
            rts

; K0_PTR = init's entry in the module directory: MD_ENTRIES + 16 * MD_INIT.  OUT: C = 0; or C = 1: there's none
init_entry:
            jsr         K_MD_VALID
            bcs         @done
            lda         MD_INIT
            cmp         MD_COUNT
            bcs         @done
            stz         K0_PTR + 1                          ; (Past entry 15, over 8 bits)
            .repeat     4
            asl
            rol         K0_PTR + 1
            .endrepeat
            clc
            adc         #<MD_ENTRIES
            sta         K0_PTR
            lda         K0_PTR + 1
            adc         #>MD_ENTRIES
            sta         K0_PTR + 1
            clc
@done:
            rts

; Init again (FARCALL from the kernel task's idle loop, IRQs on): for each n wanted (K_INIT_WANT: INIT_RUN's, or 0's
; as the task it was in ends), init started in a task of its own, as the boot starts it (no parent, no fds, an
; empty namespace and environment of its own, a note group of its own), its argument n in decimal.  With no task
; free, 0's stays wanted (tried again as a task ends); another's is dropped
K_INIT_AGAIN:
            ldx         #TASKS - 1
@want:
            lda         K_INIT_WANT,X
            beq         @next
            phx
            jsr         init_one
            plx
@next:
            dex
            bpl         @want
            rts

; Init started with .X as its argument (K_INIT_AGAIN's)
init_one:
            phx
            jsr         init_entry
            bcs         @none
            jsr         K_START_MODULE
            bcc         @started
@none:
            plx
            beq         :+                                  ; (0's: still wanted)
            stz         K_INIT_WANT,X
:
            rts

@started:
            plx
            stz         K_INIT_WANT,X
            cpx         #0
            bne         :+
            sta         K_INIT_TASK
            stz         K_INIT_EXEC
:
            txa                                             ; Its argument: n, a digit or two, and the empty one after
            ldx         K0_NEW
            ldy         T_REGISTER
            php
            sei
            cmp         #10
            bcc         @one
            sbc         #10
            pha
            lda         #'1'
            QL_PUT      TA_ARGS
            pla
            ora         #'0'
            QL_PUT      TA_ARGS + 1
            lda         #0
            QL_PUT      TA_ARGS + 2
            QL_PUT      TA_ARGS + 3
            bra         @go
@one:
            ora         #'0'
            QL_PUT      TA_ARGS
            lda         #0
            QL_PUT      TA_ARGS + 1
            QL_PUT      TA_ARGS + 2
@go:
            plp
            FARCALL     K_NS_FRESH                          ; (Keeps .X)
            txa
            FARCALL     K_TASK_GO
            rts

; Start the module at K0_PTR, and say so
boot_start:
            jsr         K_START_MODULE
            bcs         @failed
            FARCALL     K_TASK_GO
            TPRINT      T_S_TASK
            FARCALL     K_PUTNIB
            TPRINT      T_S_COLON
            jsr         @name
            TPRINT      T_S_CRLF
            rts

@failed:
            TPRINT      T_S_MODULE
            jsr         @name
            TPRINT      T_S_ERROR
            FARCALL     K_PUTHEX
            TPRINT      T_S_CRLF
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
            jmp         t_putstr

; The string at r0 (this page's, or the module directory's) on the bring-up console, by far calls to K_PUTC (page
; 0's K_PUTSTR would read page 0 for this page's strings).  Keeps .A, .X, .Y
t_putstr:
            pha
            phy
            ldy         #0
@next:
            lda         (r0),Y
            beq         @done
            FARCALL     K_PUTC
            iny
            bne         @next
@done:
            ply
            pla
            rts

; ****************************************************************************
.segment "KCODE_P1"

; In the kernel task (KCALL from SPAWN, load.s): the program, started; the caller its parent.  IN: .Y = the caller;
; .A = SPAWN's flags (SPAWN_LOAD: a RAM program); the caller's TA_SCRATCH: the program's name (SP_NAME: a module's
; in the directory, or a RAM program's from its header) and the fd map (SP_MAP)
K_SPAWN_K:
            sta         K0_SPAWNF
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
            FARCALL     K_KCOPY
            lda         K0_SPAWNF
            bmi         @ram
            jsr         K_MD_FIND                           ; In place: the directory's module of its name
            bcs         @done
            ldy         #ME_TYPE                            ; (Programs only: drivers start at boot)
            lda         (K0_PTR),Y
            cmp         #HT_PROGRAM
            bne         @noexec
            jsr         K_START_MODULE
            bra         @started

@ram:
            lda         #$FF                                ; A RAM program
            sta         K0_NEWBANK
            lda         #HT_PROGRAM
            jsr         K_START_TASK
@started:
            bcs         @done
            tax
            ldy         K0_TMP3
            tya
            sta         K_PARENT,X
            lda         K0_SPAWNF                           ; Its note group: the caller's, or its own
            and         #SPAWN_NEWGROUP
            bne         :+
            lda         K_NGROUP,Y
            sta         K_NGROUP,X
:
            lda         K0_SPAWNF                           ; SPAWN_STOPPED: it stops at its entry point (TF_HOLD:
            and         #SPAWN_STOPPED                      ;   K_TASK_MAIN)
            beq         :+
            lda         #TF_HOLD
            php
            sei
            stx         T_REGISTER                          ; ---- The child
            sta         TK_FLAGS
            stz         T_REGISTER                          ; ---- Back
            plp
:
            FARCALL     K_FD_INHERIT                        ; Its fds: the map's (file.s)
            FARCALL     K_NS_INHERIT                        ; Its namespace: the caller's, or its own (ns.s)
            FARCALL     K_ENV_INHERIT                       ; Its environment: a copy of the caller's, or empty
            txa
            clc
@done:
            rts

@noexec:
            FAIL        E_NOEXEC

; In the kernel task (KCALL from EXEC, load.s: K_EXEC_PREP).  IN: .Y = the caller.  .A = 0: find the module its
; TA_SCRATCH + SP_NAME names, a program (OUT: C = 0, .A = its bank; or C = 1, .A = E_NOENT, E_NOEXEC); SPAWN_LOAD: a
; RAM program (OUT: C = 0, .A = $FF).  .A = 1: the caller's old program gone, as at a task's end (K_EXIT_K) but
; for what's still the task's: its IRQ lines, shared segments, semaphores and device letters; its bank .X and
; its type, a program, the kernel's (OUT: C = 0)
K_EXEC_K:
            cmp         #1
            beq         @release
            tax
            bmi         @ram
            lda         #<K_NAMEBUF                         ; Its name, from the caller
            sta         K_PTR
            lda         #>K_NAMEBUF
            sta         K_PTR + 1
            lda         #<(TA_SCRATCH + SP_NAME)
            sta         K_PTR2
            lda         #>(TA_SCRATCH + SP_NAME)
            sta         K_PTR2 + 1
            lda         #HX_NAME_MAX + 1
            sta         K_CNT
            stz         K_CNT + 1
            tya
            sec
            FARCALL     K_KCOPY
            jsr         K_MD_FIND                           ; The directory's module of its name: a program
            bcs         @done
            ldy         #ME_TYPE
            lda         (K0_PTR),Y
            cmp         #HT_PROGRAM
            bne         @noexec
            ldy         #ME_BANK
            lda         (K0_PTR),Y
            clc
@done:
            rts

@ram:
            lda         #$FF
            clc
            rts

@noexec:
            FAIL        E_NOEXEC

@release:
            stx         K0_NEWBANK
            sty         K0_TMP
            FARCALL     IRQ_RELEASE_ALL                     ; Its lines
            ldy         K0_TMP
            FARCALL     K_SEG_EXIT                          ; Its shared segments (mem.s)
            ldy         K0_TMP
            FARCALL     K_SEM_EXIT                          ; Its semaphores, and the mutexes it holds (sem.s)
            ldy         K0_TMP
            FARCALL     K_FILE_EXIT                         ; Its device letters; the channels it served (file.s)
            ldy         K0_TMP
            cpy         K_INIT_TASK                         ; (Init's 0, a shell in its place now: K_EXIT_K)
            bne         :+
            lda         #1
            sta         K_INIT_EXEC
:
            lda         K0_NEWBANK
            sta         K_TASK_BANK,Y
            lda         #HT_PROGRAM
            sta         K_TASK_TYPE,Y
            clc
            rts

; ****************************************************************************
.segment "KCODE"

; EXEC: run the program at r0 in this task, in place of this one, with the arguments at r1 (or none: 0).  OUT:
; doesn't return; or C = 1, .A = the error (E_NOENT, E_NOEXEC, E_TOOBIG, E_PERM, OPEN's and READ's), and the
; caller goes on.  Its half on page 3 (load.s: K_EXEC_PREP) finds the program and lets the old one go; this one, on
; page 0, starts the new one: a fresh stack, its bank at $A000, what the task kept from the old one cleared, and
; then the program's first instructions, as SPAWN's are (K_TASK_MAIN, or K_TASK_LOAD for a RAM program)
K_EXEC:
            lda         TK_FLAGS                            ; (A driver serves calls: it can't be replaced)
            and         #TF_DRIVER
            beq         :+
            lda         #E_PERM
            sec
            rts
:
            FARCALL     K_EXEC_PREP
            bcc         @go
            jmp         K_NOTE_CHECK                        ; (Its error; a note pending: taken on the way out)

@go:                                                        ; (IRQs on: it owns no lines now, and a note waits
            ldx         #$FF                                ;   for the new program's own code) ---- The new
                                                            ;   program: the old one's frames gone
            txs
            sta         TA_MODBANK
            ldx         L_AT                                ; (Its header's page: K_EXEC_PREP's)
            stx         TA_MODBASE
            cmp         #$FF                                ; Its bank at $A000 (a RAM program: bank 0's)
            bne         :+
            lda         #0
:
            sta         ROM_BANK
            stz         RAM_BANK
            lda         TK_FLAGS                            ; Nothing of the old program's: its note handler, its
            and         #TF_STOPPED                         ;   hold on the CPU, its steps and breakpoints
            sta         TK_FLAGS
            stz         TK_PREEMPT
            stz         TK_DUE
            stz         TK_INNOTE
            stz         TK_STEPHIT
            stz         TK_RFRAME + 1                       ; (Its runs in place gone: run.s)
            stz         TA_NOTIFY
            stz         TA_NOTIFY + 1
            stz         TA_SERVEVEC                         ; (A program: no calls served, no interrupts)
            stz         TA_SERVEVEC + 1
            stz         TA_IRQVEC
            stz         TA_IRQVEC + 1
            lda         #BUSY_NOSERVE
            sta         TK_BUSY
            ldx         #HX_NAME_MAX                        ; Its name
:
            lda         TA_SCRATCH + SP_NAME,X
            sta         TA_NAME,X
            dex
            bpl         :-
            lda         TA_MODBANK
            cmp         #$FF
            beq         @ram
            stz         r0                                  ; A module: its entry, from its header (at its page)
            lda         TA_MODBASE
            sta         r0 + 1
            ldy         #HX_MAIN
            lda         (r0),Y
            sta         TA_ENTRY
            iny
            lda         (r0),Y
            sta         TA_ENTRY + 1
            jmp         K_TASK_MAIN

@ram:
            stz         TA_ENTRY                            ; A RAM program: it loads itself (its file, fd LOAD_FD)
            stz         TA_ENTRY + 1
            jmp         K_TASK_LOAD

; EXITS: end this task.  IN: .A = the exit code; r0 = a message (31 characters at most), or 0.  Doesn't return.  (In a
; program running in place, RUN's, its run ends: run.s)
K_EXITS:
            ldx         TK_RFRAME + 1
            beq         :+
            jmp         K_RUN_EXIT
:
            pha
            FARCALL     K_CLOSE_ALL                         ; Its fds (file.s: the last of a channel clunks it)
            pla
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
            KCALL       K_EXIT_K                            ; Its lines, its exit record; its children nobody's; its
                                                            ;   parent woken
            sei
            stz         TK_STATE                            ; (ST_FREE: never run again)
            stz         TK_FLAGS
            stz         TK_PREEMPT
            stz         TK_BUSY                             ; (Callers blocked on it look, and find it gone)
            jsr         K_YIELD
@gone:
            bra         @gone

.segment "KCODE_P1"

; In the kernel task (KCALL): task .Y has ended with code .A, its message in its TA_SCRATCH.  Its IRQ lines,
; nobody's; its record, for its parent (none if it has none); its children nobody's, and their records gone
K_EXIT_K:
            sta         K_EXIT_CODE,Y
            sty         K0_TMP
            FARCALL     IRQ_RELEASE_ALL                     ; Its lines
            FARCALL     K_SEG_EXIT                          ; Its shared segments (mem.s)
            ldy         K0_TMP
            FARCALL     K_SEM_EXIT                          ; Its semaphores, and the mutexes it holds (sem.s)
            ldy         K0_TMP
            FARCALL     K_FILE_EXIT                         ; Its device letters; the channels it served (file.s)
            FARCALL     K_NS_EXIT                           ; Its namespace (ns.s)
            stz         K0_TMP3
            lda         #$FF                                ; Its children's new parent: nobody (no task is kept
            sta         K0_TMP2                             ;   just to wait for them)
            lda         K_PARENT,Y
            sta         K_EXIT_PARENT,Y
            bmi         @orphans                            ; (No parent: no record)
            lda         #EX_HELD
            sta         K_EXIT_STATE,Y
            tya                                             ; Its message: to K_EXIT_MSG + 32 * the task
            FARCALL     K_EXIT_MSG_AT
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
            FARCALL     K_KCOPY
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
            FARCALL     K_WAKE
:
            lda         K0_TMP3                             ; And init, if it has records now
            beq         :+
            bmi         :+
            FARCALL     K_WAKE
:
            lda         #$FF
            sta         K_PARENT,Y
            sta         K_TASK_BANK,Y
            lda         #0
            sta         K_TASK_TYPE,Y
            cpy         K_INIT_TASK                         ; The task init's 0 was in: if it had EXECed (the login
            bne         :+                                  ;   shell, in init's place), init is started again
            lda         #$FF                                ;   (K_INIT_AGAIN); an init that ends itself is left ended
            sta         K_INIT_TASK
            lda         K_INIT_EXEC
            beq         :+
            lda         #1
            sta         K_INIT_WANT
:
            lda         K_INIT_WANT                         ; (Wanted: the kernel task looks, now a task is free)
            beq         :+
            sta         K0_NQ_ANY
:
            clc
            rts

.segment "KCODE"

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
            lda         TK_NOTED                            ; A note: E_INTR, and the note (notes.s)
            bne         @intr
            jsr         K_PAUSE                             ; Till a child ends (it wakes its parent), or a note
            bra         @look

@intr:
            jsr         K_PREEMPT_ON
            lda         #E_INTR
            sec
            jmp         K_NOTE_RETURN

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
            jmp         K_NOTE_CHECK                        ; (A note pending: taken on the way out)

@fail:
            jsr         K_PREEMPT_ON                        ; (Keeps .A and C)
            jmp         K_NOTE_CHECK

.segment "KCODE_P1"

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

.segment "KCODE"

; GETPID: .A = this task
K_GETPID:
            lda         T_REGISTER
            and         #TASKS - 1
            clc
            rts

; GETPPID: .A = this task's parent ($FF: none), a quick look at the kernel task's table.  Modifies .Y
K_GETPPID:
            php
            sei
            lda         T_REGISTER
            and         #TASKS - 1
            tay
            stz         T_REGISTER
            lda         K_PARENT,Y
            sty         T_REGISTER
            plp
            clc
            rts

.segment "KRODATA"
K_STR_CRLF:     .byte   CR, LF, 0

.segment "KRODATA_P1"
K_STR_HYMD:     .byte   "HYMD"
K_STR_HYX2:     .byte   "HYX2"
T_S_NOMODS:     .byte   "no modules (no directory in paged ROM bank 0)", CR, LF, 0
T_S_NOINIT:     .byte   "no init", CR, LF, 0
T_S_TASK:       .byte   "task ", 0
T_S_COLON:      .byte   ": ", 0
T_S_MODULE:     .byte   "module ", 0
T_S_ERROR:      .byte   ": error $", 0
T_S_CRLF:       .byte   CR, LF, 0
