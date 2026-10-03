; ****************************************************************************
; mods - the paged ROM's modules (the module directory, by MODINFO), a line each under a heading: its bank (or
; banks), its type (program, driver, library; "boot": a driver the kernel starts) and its name.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "toollib.inc"

            HYX2_PROGRAM "mods", main

.bss
entry:      .res        1
me:         .res        ME_SIZE
name:       .res        13                                  ; (Its name, zero-terminated)

.code
main:
            jsr         tl_start
            LDR         r0, s_head
            jsr         tl_puts
            stz         entry
@entry:
            LDR         r0, me
            lda         entry
            jsr         MODINFO
            bcs         @end
            lda         me + ME_BANK                        ; Its bank, or banks: "8-9"
            jsr         tl_setnum
            lda         #3
            jsr         tl_dec
            ldx         #3                                  ; (The column: 6 wide)
            lda         me + ME_BANKS
            cmp         #2
            bcc         :+
            lda         #'-'
            jsr         tl_putc
            lda         me + ME_BANK
            clc
            adc         me + ME_BANKS
            dec         a
            jsr         tl_setnum
            lda         #2
            jsr         tl_dec
            ldx         #0
:
            jsr         tl_space
            dex
            bpl         :-
            lda         me + ME_TYPE                        ; Its type
            cmp         #HT_LIBRARY + 1
            bcc         :+
            lda         #0
:
            asl
            tax
            lda         types,X
            sta         r0
            lda         types + 1,X
            sta         r0 + 1
            lda         me + ME_FLAGS
            and         #HF_BOOT
            beq         :+
            LDR         r0, s_boot
:
            lda         #9
            jsr         tl_field
            ldx         #11                                 ; Its name
:
            lda         me + ME_NAME,X
            sta         name,X
            dex
            bpl         :-
            stz         name + 12
            LDR         r0, name
            jsr         tl_puts
            jsr         tl_nl
            inc         entry
            jmp         @entry

@end:
            jmp         tl_end

.rodata
types:      .word       s_other, s_program, s_driver, s_library
s_other:    .byte       "?", 0
s_program:  .byte       "program", 0
s_driver:   .byte       "driver", 0
s_library:  .byte       "library", 0
s_boot:     .byte       "boot", 0
s_head:     .byte       "bank   type     name", LF, 0
tl_name:    .byte       "mods", 0
tl_flagset: .byte       0
tl_usage:   .byte       "mods", 0

.include "toollib.s"
