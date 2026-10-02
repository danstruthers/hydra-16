; ****************************************************************************
; post.s - the power-on self test (BIOS ROM page 4: FARCALL K_POST), from the boot (reset.s): in the kernel task,
; IRQs off, the bring-up console ready, every task's OS zero page set.  It checks what the task system depends
; on, then the paged RAM's lines (os_rom/tests/post.s and post_ram.s, ported), and prints:
;       POST ZP:0 ST:0 OS:0 HI:0 SH:S W:0
;       RAM U:0 F0:0/00/0000 F4:0/00/0000 F8:0/00/0000 FC:0/00/0000 00:0/00/0000 10:0/00/0000
;       POST ok
;   ZP ST OS HI   the T lines (bit n = Tn; 0 when good) that don't keep the tasks' byte apart at $00FF, $0100,
;                 $03FF and $7FF0: tasks 8, 4, 2, 1 and 0 each write their number, then read it back
;   SH            shared bank $F0 Shared between tasks 0 and 1 (S), or not (X)
;   W             the W lines that don't select their BIOS ROM page (each page has its number at $FDFF)
;   U             the U lines, tested on shared bank $F0
;   bb:x/dd/aaaa  bank bb at $8000-$9FFF: its bank register lines (x), data lines (dd: D0-D7) and address lines
;                 (aaaa: A0-A12) that are bad.  Tested: the first bank of each shared RAM chip ($F0, $F4, $F8,
;                 $FC, with U = 0: a missing chip shows as bad lines) and of each installed task RAM module.
; Destructive for paged RAM only.  It records the RAM modules installed (K0_MODMASK, K0_MODCOUNT), those with bad
; lines (K0_BADMODS) and the shared chips with bad lines (K0_BADSHARED): the memory calls leave them unused.  And
; what it found wrong (K0_POSTFAULT).  A T typed while it runs (held down through the reset) starts the hardware
; test (paged ROM bank 1) instead of the system.

.include "kdefs.inc"

; Its scratch: the kernel task's zero page, K0_POST on (nothing else runs yet)
P_BAD           = K0_POST + 0   ; (2) Bad lines, collected
P_REG           = K0_POST + 2   ; (2) The register a line test sets: RAM_BANK or U_REGISTER
P_PTR           = K0_POST + 4   ; (2) An address in the bank
P_BIT           = K0_POST + 6   ; (2) 2^n, an address line
P_TMP           = K0_POST + 8
P_BASE          = K0_POST + 9   ; The register's value under test
P_LINES         = K0_POST + 10  ; The lines that count against this bank, then its bad lines
P_MODS          = K0_POST + 11  ; (2) The installed modules, shifted out one by one

PAGE_ID         = $FDFF         ; Each BIOS ROM page's number (common.s)

; Print a POST string.  Modifies .A, .X
.macro KPUTS    label
            ldx         #label - P_STRINGS
            jsr         P_PUTS
.endmacro

; .A = the T lines (bit n = Tn) that don't keep the tasks' byte at addr apart (Z: none): tasks 8, 4, 2, 1 and 0
; each write their number there, then read it back.  (No stack or zero page while T isn't 0: unnamed labels
; only, so the routine's own cheap locals keep their scope.)  Modifies .X
.macro T_LINES  addr
            ldx         #8
:
            stx         T_REGISTER                          ; ---- Task .X
            stx         addr
            txa
            lsr
            tax
            bne         :-
            stz         T_REGISTER                          ; ---- Task 0: its number, 0
            stz         addr
            stz         P_BAD
            ldx         #8
:
            stx         T_REGISTER                          ; ---- Task .X: its number still?
            lda         addr
            stz         T_REGISTER                          ; ---- Task 0
            stx         P_TMP
            eor         P_TMP
            beq         :+
            txa
            tsb         P_BAD
:
            txa
            lsr
            tax
            bne         :--
            lda         addr                                ; Task 0's still 0?  (Else the tasks that landed on it)
            tsb         P_BAD
            lda         P_BAD
.endmacro

.segment "KCODE_P4"

; ****************************************************************************
K_POST:
            KPUTS       P_S_POST
            T_LINES     $00FF
            jsr         @tdone
            KPUTS       P_S_ST
            T_LINES     $0100
            jsr         @tdone
            KPUTS       P_S_OS
            T_LINES     $03FF
            jsr         @tdone
            KPUTS       P_S_HI
            T_LINES     $7FF0
            jsr         @tdone

            KPUTS       P_S_SH                              ; Shared bank $F0, from task 0 and task 1
            stz         U_REGISTER
            ldx         #SHARED_BANK
            stx         RAM_BANK                            ; (Task 0's)
            lda         #$C3
            sta         BANK_WINDOW
            lda         #1
            sta         T_REGISTER                          ; ---- Task 1 (no stack or zero page)
            stx         RAM_BANK
            lda         BANK_WINDOW
            stz         RAM_BANK
            stz         T_REGISTER                          ; ---- Task 0
            stz         RAM_BANK
            ldx         #'S'
            cmp         #$C3
            beq         :+
            ldx         #'X'
            lda         #PF_SHARED
            tsb         K0_POSTFAULT
:
            txa
            jsr         P_PUTC

            KPUTS       P_S_W                               ; The W lines: pages 8, 4, 2, 1 and 0's numbers
            lda         #<PAGE_ID
            sta         KF_VEC
            lda         #>PAGE_ID
            sta         KF_VEC + 1
            stz         P_BAD
            ldx         #8
@page:
            jsr         K_PEEK_PAGE                         ; (In the COMMON block: the same on every page)
            stx         P_TMP
            eor         P_TMP                               ; (The lines whose page answered instead)
            tsb         P_BAD
            txa
            lsr
            tax
            bne         @page
            jsr         K_PEEK_PAGE                         ; (Page 0's: 0)
            tsb         P_BAD
            lda         P_BAD
            beq         :+
            lda         #PF_W
            tsb         K0_POSTFAULT
            lda         P_BAD
:
            jsr         P_PUTNIB

            jsr         P_RAM_TEST                          ; The paged RAM's lines
            KPUTS       P_S_RESULT
            lda         K0_BADMODS                          ; Bad RAM: left unused
            ora         K0_BADMODS + 1
            ora         K0_BADSHARED
            beq         :+
            lda         #PF_RAM
            tsb         K0_POSTFAULT
:
            lda         K0_POSTFAULT
            bne         @faults
            KPUTS       P_S_OK
            bra         @hwt

@faults:
            KPUTS       P_S_FAULTS
            ldx         #0                                  ; Each fault's word
@fault:
            lsr         K0_POSTFAULT                        ; (Shifted out, and back in: it's kept)
            php
            ror         P_TMP
            plp
            bcc         @next
            lda         P_S_FWORDS,X
            phx
            tax
            jsr         P_PUTS
            plx
@next:
            inx
            cpx         #4
            bne         @fault
            lda         P_TMP                               ; (Its 4 bits, back)
            lsr
            lsr
            lsr
            lsr
            sta         K0_POSTFAULT
            KPUTS       P_S_CRLF

@hwt:                                                       ; A T typed: the hardware test, if the ROM has it
            lda         ACIA_STATUS
            and         #ACIA_ST_RDRF
            beq         @done
            lda         ACIA_DATA
            and         #$DF                                ; (Either case)
            cmp         #'T'
            bne         @done
            FARCALL     K_MD_VALID
            bcs         @nohwt
            lda         MD_HWTEST
            beq         @nohwt
            cld                                             ; ---- The machine is the hardware test's from here:
            ldx         #TASKS - 1                          ;   every task with its bank, and shared bank $F0
:
            stx         T_REGISTER                          ; (No stack or zero page till it's T 0 again)
            lda         #HWT_BANK
            sta         ROM_BANK
            lda         #SHARED_BANK
            sta         RAM_BANK
            dex
            bpl         :-
            jmp         HWT_ENTRY

@nohwt:
            KPUTS       P_S_NOHWT
@done:
            rts

@tdone:                                                     ; A T line test's result (.A): shown, and noted
            beq         :+
            pha
            lda         #PF_T
            tsb         K0_POSTFAULT
            pla
:
            jmp         P_PUTNIB

; ****************************************************************************
; The paged RAM's lines (os_rom/tests/post_ram.s): the U lines, each shared chip's first bank, the modules found
; and each one's first bank.  Leaves RAM_BANK and U at 0
P_RAM_TEST:
            KPUTS       P_S_RAM
            lda         #<U_REGISTER
            sta         P_REG
            lda         #>U_REGISTER
            sta         P_REG + 1
            lda         #SHARED_BANK
            sta         RAM_BANK
            lda         #0                                  ; The U lines, on shared bank $F0
            jsr         P_LINE_TEST
            jsr         P_PUTNIB
            lda         #<RAM_BANK
            sta         P_REG
            lda         #>RAM_BANK
            sta         P_REG + 1
            stz         K0_BADSHARED
            lda         #SHARED_BANK                        ; Shared RAM (U = 0): each chip's first bank
@shared:
            ldx         #$03                                ; (Bank lines 2-3 pick the chip: only lines 0-1 count
            stx         P_LINES                             ;   against this one)
            jsr         P_BANK_TEST                         ; Z = 0: bad
            clc
            beq         :+
            sec
:
            ror         K0_BADSHARED                        ; (Chip c ends in bit 4 + c)
            clc
            adc         #4
            bcc         @shared
            lsr         K0_BADSHARED                        ; Chip c: bit c
            lsr         K0_BADSHARED
            lsr         K0_BADSHARED
            lsr         K0_BADSHARED
            jsr         P_PROBE                             ; The task RAM modules installed: each one's first bank
            lda         K0_MODMASK
            sta         P_MODS
            lda         K0_MODMASK + 1
            sta         P_MODS + 1
            stz         K0_BADMODS
            stz         K0_BADMODS + 1
            lda         #0
@module:
            lsr         P_MODS + 1
            ror         P_MODS
            bcc         :+                                  ; (Not installed: C = 0, not bad)
            ldx         #$0F                                ; (All 4 bank lines are the module's)
            stx         P_LINES
            jsr         P_BANK_TEST                         ; Z = 0: bad
            clc
            beq         :+
            sec
:
            ror         K0_BADMODS + 1                      ; (Module m ends in bit m)
            ror         K0_BADMODS
            clc
            adc         #$10
            bcc         @module                             ; (Module 15 is the shared banks' place: never
            stz         RAM_BANK                            ;   installed, so never tested)
            stz         U_REGISTER
            rts

; The RAM modules (daughter cards: module m has banks $m0-$mF in every task): which are installed.  A missing
; module's banks float: a pattern written doesn't read back.  OUT: K0_MODMASK (bit = module), K0_MODCOUNT
P_PROBE:
            stz         K0_MODMASK
            stz         K0_MODMASK + 1
            stz         K0_MODCOUNT
            ldx         #RAM_MODULES - 1
@module:
            txa
            asl
            asl
            asl
            asl
            sta         RAM_BANK                            ; Its first bank
            lda         #$55
            sta         BANK_WINDOW
            cmp         BANK_WINDOW
            bne         @missing
            asl                                             ; ($AA)
            sta         BANK_WINDOW
            cmp         BANK_WINDOW
            bne         @missing
            inc         K0_MODCOUNT
            sec
            bra         @bit

@missing:
            clc
@bit:
            rol         K0_MODMASK                          ; (Module 14 first: it ends in bit 14)
            rol         K0_MODMASK + 1
            dex
            bpl         @module
            stz         RAM_BANK
            rts

; Test bank .A ($8000-$9FFF; P_REG points at RAM_BANK) and print " bb:x/dd/aaaa": its bad bank lines (x: see
; P_LINE_TEST), data lines (dd: D0-D7) and address lines (aaaa: A0-A12), bit set = bad.  Leaves the bank selected.
; IN: P_LINES = the bank lines that count against this bank.  OUT: Z = 0 if any line is bad.  Keeps .A
P_BANK_TEST:
            pha
            pha
            lda         #' '
            jsr         P_PUTC
            pla
            jsr         P_PUTBYTE
            lda         #':'
            jsr         P_PUTC
            pla
            pha
            jsr         P_LINE_TEST
            pha
            and         P_LINES
            sta         P_LINES                             ; P_LINES: all of this bank's bad lines from here
            pla
            jsr         P_PUTNIB
            lda         #'/'
            jsr         P_PUTC
            stz         P_TMP                               ; Data lines: a walking one at $8000
            ldx         #1
:
            stx         BANK_WINDOW
            txa
            eor         BANK_WINDOW                         ; (The bits that read back wrong)
            tsb         P_TMP
            txa
            asl
            tax
            bne         :-
            lda         P_TMP
            tsb         P_LINES
            jsr         P_PUTBYTE
            lda         #'/'
            jsr         P_PUTC

; Address lines: $8000 = 0, $8000 + 2^n = n + 1 (n = 0-12).  Line n is bad if $8000 + 2^n reads back wrong, or
; its write landed on $8000
            stz         P_BAD
            stz         P_BAD + 1
            stz         BANK_WINDOW
            ldx         #0                                  ; Pass 0 writes, pass 1 checks
@pass:
            lda         #1
            sta         P_BIT                               ; 2^n
            stz         P_BIT + 1
            ldy         #1                                  ; n + 1
@line:
            lda         P_BIT
            sta         P_PTR
            lda         P_BIT + 1
            ora         #>BANK_WINDOW
            sta         P_PTR + 1
            tya
            cpx         #0
            bne         :+
            sta         (P_PTR)
            bra         @next

:
            cmp         BANK_WINDOW
            beq         @bad
            cmp         (P_PTR)
            beq         @next
@bad:
            lda         P_BIT
            tsb         P_BAD
            lda         P_BIT + 1
            tsb         P_BAD + 1
@next:
            iny
            asl         P_BIT
            rol         P_BIT + 1
            lda         P_BIT + 1
            cmp         #>BANK_SIZE                         ; (Past A12)
            bne         @line
            inx
            cpx         #2
            bne         @pass
            lda         P_BAD + 1
            tsb         P_LINES
            jsr         P_PUTBYTE
            lda         P_BAD
            tsb         P_LINES
            jsr         P_PUTBYTE
            pla
            ldx         P_LINES                             ; (Z = 0: bad)
            rts

; The register at (P_REG) (RAM_BANK or U) set to .A ^ 8, .A ^ 4, .A ^ 2, .A ^ 1 and .A in turn, the byte at
; $8000 telling them apart.  OUT: .A = the bad lines (bit n = the register's bit n, n = 0-3); the register is
; left at .A.  Modifies .X, .Y
P_LINE_TEST:
            sta         P_BASE
            stz         P_TMP
            ldy         #0                                  ; Pass 0 writes, pass 1 checks
@pass:
            ldx         #$10
@line:
            txa
            lsr
            tax                                             ; 8, 4, 2, 1, 0 (the marker)
            eor         P_BASE
            sta         (P_REG)
            txa
            cpy         #0
            bne         :+
            sta         BANK_WINDOW
            bra         @next

:
            cmp         BANK_WINDOW
            beq         @next
            tsb         P_TMP
@next:
            txa
            bne         @line
            iny
            cpy         #2
            bne         @pass
            lda         P_TMP
            rts

; ****************************************************************************
; Printing, on the bring-up console (page 0's routines, by far calls)

; The POST string at offset .X in P_STRINGS.  Modifies .A, .X
P_PUTS:
            lda         P_STRINGS,X
            beq         :+
            jsr         P_PUTC
            inx
            bra         P_PUTS
:
            rts

; .A as a byte (P_PUTBYTE: two hex digits), or its low 4 bits (P_PUTNIB: one), or as a character (P_PUTC)
P_PUTBYTE:
            FARCALL     K_PUTHEX
            rts

P_PUTNIB:
            FARCALL     K_PUTNIB
            rts

P_PUTC:
            FARCALL     K_PUTC
            rts

.segment "KRODATA_P4"
P_STRINGS:
P_S_POST:   .byte       CR, LF, "POST ZP:", 0
P_S_ST:     .byte       " ST:", 0
P_S_OS:     .byte       " OS:", 0
P_S_HI:     .byte       " HI:", 0
P_S_SH:     .byte       " SH:", 0
P_S_W:      .byte       " W:", 0
P_S_RAM:    .byte       CR, LF, "RAM U:", 0
P_S_RESULT: .byte       CR, LF, "POST ", 0
P_S_OK:     .byte       "ok", CR, LF, 0
P_S_FAULTS: .byte       "found faults:", 0
P_S_FT:     .byte       " T lines", 0
P_S_FSH:    .byte       " shared RAM", 0
P_S_FW:     .byte       " W lines", 0
P_S_FRAM:   .byte       " RAM (left unused)", 0
P_S_CRLF:   .byte       CR, LF, 0
P_S_NOHWT:  .byte       "(no hardware test in this ROM)", CR, LF, 0
P_S_FWORDS: .byte       P_S_FT - P_STRINGS, P_S_FSH - P_STRINGS, P_S_FW - P_STRINGS, P_S_FRAM - P_STRINGS
