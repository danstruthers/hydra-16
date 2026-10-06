; ****************************************************************************
; t_kmesg - the kernel's messages (KMESG, and kdev's #n/kmesg: /dev/kmesg), run as init (its fds 0-2 closed: its
; lines go out on the bring-up console, so into the messages too).  The boot's banner first; KMESG at offsets (the
; last byte held, past the end: nothing); 4200 bytes more printed, the ring full and only its last KMESG_SIZE held;
; #n/kmesg read through kdev in parts, the bytes KMESG holds.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "testlib.inc"

            HYX2_PROGRAM "t_kmesg", main

.zeropage
fd:         .res        1
held:       .res        2
total:      .res        2
byte0:      .res        1

.bss
buf:        .res        512

.code

; KMESG into buf: count bytes at most, from offset.  OUT: held = the bytes held; C
.macro KMESG_   count, offset
            LDR         r0, buf
            LDR         r1, count
            LDR         r2, offset
            jsr         KMESG
            sta         held
            stx         held + 1
.endmacro

main:
            stz         T_FAILS

; ---- The boot's: the banner first; the last byte held, the last line's end; past the end, nothing
            KMESG_      16, 0
            EXPECT_OK   "KMESG"
            lda         buf + 2
            EXPECT_A    'H', "the boot's banner first: CR LF Hydra-16 ..."
            lda         buf + 9
            EXPECT_A    '6', "  (its Hydra-16)"
            lda         held                                ; The last byte held
            sec
            sbc         #1
            sta         r2
            lda         held + 1
            sbc         #0
            sta         r2 + 1
            LDR         r0, buf
            LDR         r1, 1
            jsr         KMESG
            lda         buf
            EXPECT_A    LF, "the last byte held: the last line's LF"
            lda         #$55                                ; From the end (as it is now: lines printed since)
            sta         buf
            KMESG_      0, 0
            MOVR        r2, held
            LDR         r0, buf
            LDR         r1, 16
            jsr         KMESG
            lda         buf
            EXPECT_A    $55, "from the end: nothing copied"

; ---- 4200 bytes more: the ring full, its last KMESG_SIZE held (all dots)
            stz         total
            stz         total + 1
@dot:
            lda         #'.'
            jsr         PUTC
            inc         total
            bne         :+
            inc         total + 1
:
            lda         total + 1
            cmp         #>4200
            bne         @dot
            lda         total
            cmp         #<4200
            bne         @dot
            KMESG_      16, 0
            lda         held + 1                            ; (The count first, before a line is printed)
            sta         total + 1
            lda         held
            sta         total
            lda         buf
            sta         fd
            KMESG_      1, KMESG_SIZE - 1
            lda         buf
            pha
            lda         total
            EXPECT_A    <KMESG_SIZE, "4200 bytes more: KMESG_SIZE held ..."
            lda         total + 1
            EXPECT_A    >KMESG_SIZE, "  (its high byte)"
            lda         fd
            EXPECT_A    '.', "  the oldest held one of the dots"
            pla
            EXPECT_A    '.', "  and the last"

; ---- #n/kmesg (/dev/kmesg): kdev's, read in parts, as many bytes as KMESG holds
            LDR         r0, s_kmesg
            lda         #O_READ
            jsr         OPEN
            sta         fd
            EXPECT_OK   "OPEN #n/kmesg"
            stz         total
            stz         total + 1
            LDR         r0, buf
            LDR         r1, 512
            lda         fd
            jsr         READ
            ldy         buf                                 ; (Its first byte, kept)
            sty         byte0
            bra         @first

@part:
            LDR         r0, buf
            LDR         r1, 512
            lda         fd
            jsr         READ
@first:
            bcs         @end
            sta         r2
            stx         r2 + 1
            ora         r2 + 1
            beq         @end                                ; (0: its end)
            clc
            lda         total
            adc         r2
            sta         total
            lda         total + 1
            adc         r2 + 1
            sta         total + 1
            bra         @part

@end:
            lda         fd
            jsr         CLOSE
            lda         byte0
            EXPECT_A    '.', "#n/kmesg's first byte: a dot, as KMESG's"
            lda         total
            EXPECT_A    <KMESG_SIZE, "#n/kmesg, 512 at a time: KMESG_SIZE bytes ..."
            lda         total + 1
            EXPECT_A    >KMESG_SIZE, "  (its high byte)"

            DONE        "t_kmesg"

.rodata
s_kmesg:    .byte       "#n/kmesg", 0
