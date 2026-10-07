; ****************************************************************************
; hwtest - the hardware test (os_rom/hwtest, in paged ROM bank 1), from the shell: the system starts again into it
; (REBOOT_HWTEST), as a T typed during POST does.  Every task ends where it is.  Its keys and its end are its own:
; the reset button starts the system again.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "toollib.inc"

            HYX2_PROGRAM "hwtest", main

.code
main:
            jsr         tl_start
            lda         (tl_arg)                            ; (No names)
            beq         :+
            jmp         tl_badusage
:
            LDR         r0, s_going                         ; Said, then the restart
            jsr         PUTS
            lda         #20                                 ; (A moment for the console to send it)
            ldx         #0
            jsr         SLEEP
            lda         #REBOOT_HWTEST
            jmp         REBOOT

.rodata
s_going:    .byte       "hwtest: the system starts again, into the hardware test", $0A, 0
tl_name:    .byte       "hwtest", 0
tl_flagset: .byte       0
tl_usage:   .byte       "hwtest", 0

.include "toollib.s"
