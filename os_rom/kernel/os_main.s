.debuginfo
.macro ZERO_W
            lda     #0
            sta     W_REGISTER
.endmacro

.segment "BIOS_P0"
RESET_VECTOR_START:
            ZERO_W
            cld                                                     ; We don't like decimal mode
            sta                 T_REGISTER                          ; Make sure task 0 is selected
            sta                 $00                                 ; Init RAM Bank selector
            sta                 $01                                 ; Init ROM Bank selector
            ldx                 #$FF                                ; Init stack pointer
            txs

            jsr                 POST                                ; Power-on self test (polled serial, no IRQs)
            jsr                 IRQ_INIT                            ; Must be first: IRQ tables and vectors
            jsr                 TASKS_INIT                          ; Must be called before the drivers and MMU_INIT
            cli                                                     ; IRQs on: the dispatcher and tasks are ready
            jsr                 MMU_INIT
            jsr                 IO_INIT                             ; The IO layer's devices (/dev/null, /dev/zero)
            jsr                 VIA_INIT
            lda                 #<SERIAL_DRIVER                     ; Serial driver in its own (Resident) task;
            ldy                 #>SERIAL_DRIVER                     ;   first: it must be started before
            ldx                 #SERIAL_TASK_NUM                    ;   anything prints (DRV_BOOT too)
            jsr                 DRV_START
            jsr                 DO_WELCOME                          ; (It clears the screen: before DRV_BOOT's reports)
            lda                 #<SOUND_DRIVER                      ; Sound driver in its own (Resident) task
            ldy                 #>SOUND_DRIVER
            ldx                 #SOUND_TASK_NUM
            jsr                 DRV_BOOT
            lda                 #<PIPE_DRIVER                       ; Pipe server in its own (Resident) task
            ldy                 #>PIPE_DRIVER
            ldx                 #PIPE_TASK_NUM
            jsr                 DRV_BOOT
            lda                 #<STORAGE_DRIVER                    ; Storage (/dev/sd) in its own (Resident) task
            ldy                 #>STORAGE_DRIVER
            ldx                 #STORAGE_TASK_NUM
            jsr                 DRV_BOOT

; Start the boot shell in its own task (the default serial-capture task: page 7's SH_BOOT finds the
; volumes, then starts HyForth), start the scheduler's tick, and hand the CPU over
            lda                 #<BOOT_SHELL
            ldy                 #>BOOT_SHELL
            ldx                 #SHELL_TASK_NUM
            jsr                 TASK_PREPARE
            jsr                 SCHED_START
            jsr                 YIELD

; The system task is the idle task: the scheduler only runs it when no other task can run
@idle:
            wai
            bra                 @idle

FAR_GATE_INLINE     BOOT_SHELL,     ::SH_BOOT_P7,           7   ; The boot shell (page 7)

; A shell started later (HyForth's shell word): HyForth, then WOZMON when Forth exits (bye).  It inherits
; its parent's namespace (/sd mounted) and current directory
SHELL_MAIN:
            jsr                 IO_STD_OPEN                         ; fds 0-2 on /dev/cons (inherited by the tasks the shell starts)
            jsr                 COPYTORAM
            jsr                 forth_main                          ; (No clear screen: boot messages, e.g. a driver's FAIL, stay)
            jsr                 MON_START

; The address POST (tests/post.s, on page 4) checks on ROM page 1: page 4 comes before PAGE1 in all.s
POST_P1_PROBE   = PAGE1::forth_main

; Start a driver at boot (DRV_START), and say so if its init fails: "<NAME> FAIL ee" (ee = the error).
; The serial driver must be running already.  IN: .A.Y = DriverInfo, .X = task
DRV_BOOT:
            pha
            phy
            jsr                 DRV_START
            bcc                 @done
            ply
            sty                 ZP_TEMP_VEC + 1                     ; The DriverInfo
            ply
            sty                 ZP_TEMP_VEC
            pha
            PRINT_CRLF
            ldy                 #DriverInfo::name + 1
            lda                 (ZP_TEMP_VEC),Y
            tax
            dey
            lda                 (ZP_TEMP_VEC),Y
            phx
            ply
            jsr                 WRITE_HSTRING                       ; Its name
            PRINT_CHAR          #' ', #'F', #'A', #'I', #'L', #' '
            pla
            PRINT_BYTE
            PRINT_CRLF
            rts

@done:
            ply
            pla
            rts

NamedHString HYDRA_WELCOME, "Welcome to the HYDRA-16!"

DO_WELCOME:
            jsr                 CLEAR_SCR
            _M_WRITE_HSTRING    HYDRA_WELCOME
            PRINT_CRLF_JMP

; A: S/W interrupt number
; Preserves .X and V
SW_INT:
            php                                                     ; No task switch while V is ours (BRK runs
            sei                                                     ;   even with IRQs off)
            phx
            ldx                 V_REGISTER                          ; Save V (shared pseudo-register)
            asl                                                     ; move int# to V[4..7]
            asl
            asl
            asl
            ora                 #IRQ_NUMBER_SW                      ; S/W IRQ vector in V[0..3]
            sta                 V_REGISTER
            brk                                                     ; force an interrupt
            .byte               $00                                 ; BRK signature byte (RTI returns past it)
            stx                 V_REGISTER                          ; Restore prior V
            plx
            plp
            rts

; Start of every other BIOS page: the RESET entry at $E000 zeroes W, and execution continues on page 0
; right after RESET_VECTOR_START's ZERO_W.  The rest of each page is filled with NOPs by the linker
; (fillval), except for the COMMON block and the vectors.
.macro OTHER_PAGE_FILLER
            ZERO_W
.endmacro

.segment "BIOS_P1"                                                  ; HyForth and the disassembler follow (page1.s)
            OTHER_PAGE_FILLER
.segment "BIOS_P2"
            OTHER_PAGE_FILLER
.segment "BIOS_P3"
            OTHER_PAGE_FILLER
.segment "BIOS_P4"
            OTHER_PAGE_FILLER
.segment "BIOS_P5"
            OTHER_PAGE_FILLER
.segment "BIOS_P6"
            OTHER_PAGE_FILLER
.segment "BIOS_P7"
            OTHER_PAGE_FILLER
.segment "BIOS_P8"
            OTHER_PAGE_FILLER
.segment "BIOS_P9"
            OTHER_PAGE_FILLER
.segment "BIOS_PA"
            OTHER_PAGE_FILLER
.segment "BIOS_PB"
            OTHER_PAGE_FILLER
.segment "BIOS_PC"
            OTHER_PAGE_FILLER
.segment "BIOS_PD"
            OTHER_PAGE_FILLER
.segment "BIOS_PE"
            OTHER_PAGE_FILLER
.segment "BIOS_PF"
            OTHER_PAGE_FILLER
