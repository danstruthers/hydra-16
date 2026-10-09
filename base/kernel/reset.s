; ****************************************************************************
; reset.s - the reset entry on every BIOS ROM page, the CPU's vectors on every page, and the boot.
;
; W isn't reset by the hardware, so a reset can start on any page: every page has the same 3 bytes at $E000,
; which set W = 0; the next instruction comes from page 0, at $E003, where the boot goes on.  T, U, V, the
; bank registers and all RAM power up random: the boot sets every one before it's used.
;
; The boot, in task 0 (the kernel task), with IRQs off:
;   1. every task's OS zero page and bank registers, and the kernel task's tables
;   2. the bring-up console, the banner, POST (page 4: post.s), which finds the RAM modules
;   3. the IRQ vectors and the lines' owners (the VIA's line is the kernel's: the tick)
;   4. the boot drivers (tasks F, E ...: task.s, K_TASK_BOOT)
;   5. the tick, IRQs on; once the drivers have started, init (task 1); and task 0 becomes the idle task

.include "kdefs.inc"

; ---- The reset stubs: $E000 on every page
.segment "RESET_P0"
RESET_ENTRY:
            stz         W_REGISTER                          ; (On page 0 from the next instruction, wherever we were)
            sei
            cld
            stz         T_REGISTER                          ; The kernel task's RAM and stack
            ldx         #$FF
            txs
            jmp         BOOT

.macro RESET_STUB
            stz         W_REGISTER                          ; (Then page 0's $E003)
.endmacro
.segment "RESET_P1"
            RESET_STUB
.segment "RESET_P2"
            RESET_STUB
.segment "RESET_P3"
            RESET_STUB
.segment "RESET_P4"
            RESET_STUB
.segment "RESET_P5"
            RESET_STUB
.segment "RESET_P6"
            RESET_STUB
.segment "RESET_P7"
            RESET_STUB
.segment "RESET_P8"
            RESET_STUB
.segment "RESET_P9"
            RESET_STUB
.segment "RESET_PA"
            RESET_STUB
.segment "RESET_PB"
            RESET_STUB
.segment "RESET_PC"
            RESET_STUB
.segment "RESET_PD"
            RESET_STUB
.segment "RESET_PE"
            RESET_STUB
.segment "RESET_PF"
            RESET_STUB

; ---- The vectors: NMI, RESET, and IRQ (unused: the vector RAM answers $FFFE/$FFFF)
.macro VECTORS
            .word       NMI_ENTRY
            .word       RESET_ENTRY
            .word       IRQ_STUB_F
.endmacro
.segment "VEC_P0"
            VECTORS
.segment "VEC_P1"
            VECTORS
.segment "VEC_P2"
            VECTORS
.segment "VEC_P3"
            VECTORS
.segment "VEC_P4"
            VECTORS
.segment "VEC_P5"
            VECTORS
.segment "VEC_P6"
            VECTORS
.segment "VEC_P7"
            VECTORS
.segment "VEC_P8"
            VECTORS
.segment "VEC_P9"
            VECTORS
.segment "VEC_PA"
            VECTORS
.segment "VEC_PB"
            VECTORS
.segment "VEC_PC"
            VECTORS
.segment "VEC_PD"
            VECTORS
.segment "VEC_PE"
            VECTORS
.segment "VEC_PF"
            VECTORS

.assert     RESET_ENTRY = BIOS_BASE, lderror, "The reset entry must be at $E000"

; ---- The boot
.segment "KCODE"

BOOT:
; 1. Every task's OS zero page and bank registers (no stack: T changes the stack page)
            ldx         #TASKS - 1
@task:
            stx         T_REGISTER
            stz         RAM_BANK
            stz         ROM_BANK
            stz         TK_STATE                            ; (ST_FREE)
            stz         TK_FLAGS
            stz         TK_PREEMPT
            stz         TK_DUE
            stz         TK_BUSY
            stz         TK_NOTED
            stz         TK_INNOTE
            stz         TK_WOKEN
            stz         TK_EVENT
            lda         #FRAME_SP
            sta         TK_SP
            stz         TA_IRQVEC + 1                       ; (No irq entry, no serve entry)
            stz         TA_SERVEVEC + 1
            ldy         #LINES - 1                          ; Its copy of the lines' owners: nobody; its fds closed
            lda         #$FF
:
            sta         TA_OWNERS,Y
            sta         TA_FD,Y
            dey
            bpl         :-
            sta         TA_OWNERS + LINE_VIA_T2
            sta         TA_OWNERS + LINE_VIA_CA1
            dex
            bpl         @task                               ; (Ends with T = 0)
            lda         #ST_READY                           ; The kernel task runs (the boot, then the idle loop),
            sta         TK_STATE                            ;   and is never preempted (it yields when it's idle)
            inc         TK_PREEMPT
            stz         U_REGISTER
            lda         #IRQ_INDEX(LINE_NONE)               ; V: BRK's vector entry (line 15's: nothing drives it)
            sta         V_REGISTER

; The kernel task's zero page and tables
            ldx         #K0_END - PROG_ZP - 1
@zp:
            stz         PROG_ZP,X
            dex
            bpl         @zp
            ldx         #TASKS - 1
@tables:
            lda         #$FF
            sta         K_PARENT,X
            sta         K_TASK_BANK,X
            sta         K_SEM_MAKER,X                       ; (No semaphores: SEM_MAX = TASKS)
            stz         K_SEM_WAITLO,X
            stz         K_SEM_WAITHI,X
            stz         K_IRQ_STRAY,X
            stz         K_EXIT_STATE,X
            stz         K_TASK_TYPE,X
            stz         K_CPU_LO,X
            stz         K_CPU_MID,X
            stz         K_CPU_HI,X
            stz         K_SEG_COUNT,X                       ; (No shared segments; nobody attached)
            stz         K_SEG_REFS,X
            stz         K_SEGATT_LO,X
            stz         K_SEGATT_HI,X
            stz         K_SHMAP,X
            txa
            sta         K_NGROUP,X                          ; (Each task a note group of its own)
            dex
            bpl         @tables
            ldx         #CH_MAX - 1                         ; No channels, no devices
:
            stz         K_CH_REFS,X
            dex
            bpl         :-
            ldx         #DEV_MAX - 1
:
            stz         K_DEV_LETTER,X
            dex
            bpl         :-
            FARCALL     K_NS_INIT                           ; No namespaces (page 3)
            lda         #<K_KDISPATCH                       ; KCALLs run here: the kernel task's serve entry
            sta         TA_SERVEVEC
            lda         #>K_KDISPATCH
            sta         TA_SERVEVEC + 1
            lda         #<K_KIRQ                            ; Its interrupts (the tick)
            sta         TA_IRQVEC
            lda         #>K_KIRQ
            sta         TA_IRQVEC + 1
            ldx         #K_STR_KERNEL_END - K_STR_KERNEL - 1
@name:
            lda         K_STR_KERNEL,X
            sta         TA_NAME,X
            dex
            bpl         @name
            stz         TA_ARGS                             ; (No arguments)

; 2. The console, the banner, POST (page 4: it finds the RAM modules, and the RAM to leave unused); what's printed
; kept in the kernel's messages, from here on
            stz         K_KMESG_HEAD
            stz         K_KMESG_HEAD + 1
            stz         K_KMESG_LEN
            stz         K_KMESG_LEN + 1
            jsr         K_CONS_INIT
            KPRINT      K_STR_BANNER
            FARCALL     K_POST
            FARCALL     K_ENV_INIT                          ; (The environments' banks: page 2)
            KPRINT      K_STR_MODULES
            lda         K0_MODCOUNT
            jsr         K_PUTHEX
            KPRINT      K_STR_CRLF

; 3. Interrupts: the vectors, the owners; the VIA is the kernel's
            jsr         IRQ_INIT

; 4. The boot drivers (they run when IRQs go on: page 1)
            FARCALL     K_TASK_BOOT

; 5. The tick (VIA timer 1, free-running: TICK_HZ a second); the drivers started; init; then the idle loop
            lda         VIA_ACR
            and         #$3F
            ora         #VIA_ACR_T1_FREE
            sta         VIA_ACR
            lda         #<TICK_LATCH
            sta         VIA_T1CL
            lda         #>TICK_LATCH
            sta         VIA_T1CH                            ; (Loads and starts it)
            lda         #VIA_IER_SET | VIA_IRQ_T1
            sta         VIA_IER
BOOT_DONE:                                                  ; (The tests' IRQs-off budget counts from here)
            cli
            FARCALL     K_BOOT_STARTED                      ; (Their inits run: their devices registered)
            FARCALL     K_TASK_BOOT_INIT

; The idle task: whatever can run runs; when nothing can, the CPU sleeps until an interrupt.  And the notes irq
; entries queued for groups, posted (the scheduler runs this then: notes.s)
@idle:
            jsr         K_YIELD
            lda         K0_NQ_ANY
            beq         :+
            jsr         K_NOTE_QUEUED
            bra         @idle
:
            wai
            bra         @idle

.segment "KRODATA"
K_STR_KERNEL:   .byte   "kernel", 0
K_STR_KERNEL_END:
K_STR_BANNER:   .byte   CR, LF, "Hydra-16: kernel 0.1, ABI 1", 0
K_STR_MODULES:  .byte   "RAM modules: ", 0
