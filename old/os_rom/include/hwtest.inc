; ****************************************************************************
; The hardware test (os_rom/hwtest/): a program of its own in paged ROM bank HWT_BANK, which takes the
; machine over (no OS: IRQs off, its own polled serial I/O) and tests as much of the hardware as it can.
; It overwrites memory, and ends by resetting the machine.  Entered from BIOS ROM code with
; _M_HWT_ENTER: HyForth's hwtest (farwords.s), and POST when a T is typed during it (post.s).

HWT_BANK            = $01       ; Its paged ROM bank ($A000-$DFFF)
HWT_SUMS            = $DF00     ; The ROMs' checksums, at the end of its bank (tools/romsum.js fills them in: 256 bytes)

; Go to the hardware test, from BIOS ROM code (which T doesn't change): every task gets the test's paged
; ROM bank, so the test can switch T while it runs from it, and shared bank $F0 at $8000.  It never returns.
.macro _M_HWT_ENTER
            sei
            cld
            ldx         #15
:
            stx         T_REGISTER                          ; (No stack or ZP until T is 0 again)
            lda         #HWT_BANK
            sta         ROM_BANK_REG
            lda         #$F0
            sta         RAM_BANK_REG
            dex
            bpl         :-                                  ; (T = 0 at the end)
            jmp         ::HWT_ENTRY
.endmacro
