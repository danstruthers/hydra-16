; ****************************************************************************
; task.s - starting and ending tasks, and the module directory (docs/reimplementation-from-scratch.md, §10.6, §11).
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
            ldy         #ME_TYPE
            lda         (K0_PTR),Y

; The same for a module of type .A in bank K0_NEWBANK; or for a RAM program (K0_NEWBANK $FF, its name in
; K_NAMEBUF), which loads itself as it starts (K_TASK_LOAD)
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

; Set task K0_NEW up to run the module in bank K0_NEWBANK, a K0_NEWTYPE: its banks, its entries and its name (from
; the module's header), its OS zero page, its data copied, its BSS cleared, its first frame.  A RAM program
; (K0_NEWBANK $FF) has no header yet: its name is K_NAMEBUF, and the rest is its own, as it loads (K_TASK_LOAD).
; In the kernel task; keeps the I flag (the boot's are off).  It's in the new task's memory in short steps, with
; IRQs off for each and a moment between.  OUT: C = 0; or C = 1, .A = E_NOEXEC (not a module this kernel can
; run).  Modifies .A, .X, .Y
K_TASK_SETUP:
            ldx         K0_NEW
            lda         K0_NEWBANK
            php
            sei
            stx         T_REGISTER                          ; ---- The new task (not its stack: no jsr, no pha)
            sta         TA_MODBANK                          ; Its module, at $A000
            stz         RAM_BANK
            cmp         #$FF
            bne         :+
            stz         ROM_BANK                            ; (A RAM program: bank 0's at $A000, and no header)
            bra         @header
:
            sta         ROM_BANK
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
            stz         TK_NOTED                            ; (No notes, no handler)
            stz         TK_NOTES
            stz         TK_NOTES + 1
            stz         TK_NOTES + 2
            stz         TK_NOTES + 3
            stz         TK_INNOTE
            stz         TK_WOKEN
            stz         TK_EVENT
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
; copied from its module and its BSS cleared, as its header has them (its first bank is at $A000).  So the kernel
; task doesn't do it a byte at a time from outside, with IRQs off for each (rc's 4.3K of BSS: 257,000 cycles that
; way, 39,000 this).  Modifies .A, .X, .Y, r0, r1
K_TASK_DATA:
            lda         PROM_WINDOW + HX_DATA_LOAD          ; Its data: r0 from, r1 to
            sta         r0
            lda         PROM_WINDOW + HX_DATA_LOAD + 1
            sta         r0 + 1
            lda         PROM_WINDOW + HX_DATA_RUN
            sta         r1
            lda         PROM_WINDOW + HX_DATA_RUN + 1
            sta         r1 + 1
            ldy         #0
            ldx         PROM_WINDOW + HX_DATA_LEN + 1       ; Its whole pages ...
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
            ldx         PROM_WINDOW + HX_DATA_LEN           ; ... and the rest
            beq         @bss
:
            lda         (r0),Y
            sta         (r1),Y
            iny
            dex
            bne         :-
@bss:
            lda         PROM_WINDOW + HX_BSS                ; Its BSS: r1, 4 bytes at a time in whole pages
            sta         r1
            lda         PROM_WINDOW + HX_BSS + 1
            sta         r1 + 1
            lda         #0
            tay
            ldx         PROM_WINDOW + HX_BSS_LEN + 1
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
            ldx         PROM_WINDOW + HX_BSS_LEN
            beq         @done
:
            sta         (r1),Y
            iny
            dex
            bne         :-
@done:
            rts

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
            jmp         (TA_ENTRY)

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

; ****************************************************************************
.segment "KCODE_P1"

; At boot, in the kernel task (FARCALL): the boot drivers (each with HF_BOOT: K_TASK_BOOT, IRQs off), then, once
; they've started (K_BOOT_STARTED), init (the directory's MD_INIT: K_TASK_BOOT_INIT).  Each said on the console:
; "task F: cons", or "module NAME: error $xx"
K_TASK_BOOT:
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

; Init: the directory's MD_INIT, started, with an empty namespace of its own (it builds it)
K_TASK_BOOT_INIT:
            jsr         K_MD_VALID
            bcs         @noinit
            lda         MD_INIT
            cmp         MD_COUNT
            bcs         @noinit
            stz         K0_PTR + 1                          ; Its entry: MD_ENTRIES + 16 * MD_INIT (past entry 15,
            .repeat     4                                   ;   over 8 bits)
            asl
            rol         K0_PTR + 1
            .endrepeat
            clc
            adc         #<MD_ENTRIES
            sta         K0_PTR
            lda         K0_PTR + 1
            adc         #>MD_ENTRIES
            sta         K0_PTR + 1
            ldx         #INIT_TASK                          ; (Before it can run: the kernel task isn't preempted)
            FARCALL     K_NS_FRESH
            jmp         boot_start

@noinit:
            TPRINT      T_S_NOINIT
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
            FARCALL     K_FD_INHERIT                        ; Its fds: the map's (file.s)
            FARCALL     K_NS_INHERIT                        ; Its namespace: the caller's, or its own (ns.s)
            FARCALL     K_ENV_INHERIT                       ; Its environment: a copy of the caller's, or empty
            txa
            clc
@done:
            rts

@noexec:
            FAIL        E_NOEXEC

; ****************************************************************************
.segment "KCODE"

; EXITS: end this task.  IN: .A = the exit code; r0 = a message (31 characters at most), or 0.  Doesn't return
K_EXITS:
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

.segment "KCODE_P1"

; In the kernel task (KCALL): task .Y has ended with code .A, its message in its TA_SCRATCH.  Its IRQ lines,
; nobody's; its record, for its parent (none if it has none); its children and their records, init's (nobody's,
; if it's init)
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
