.debuginfo

; ****************************************************************************
; POST paged RAM line tests (see POST in os_main.s).  BIOS ROM page 2, included inside `.scope PAGE2`
; (see all.s); POST calls it through a gate, at boot (task 0, IRQs off, polled serial output).
; Prints the second POST line:
;       RAM U:x bb:x/dd/aaaa bb:x/dd/aaaa ...
;   Each is a hex mask of bad lines (bit set = bad; all 0 when good):
;   U:x      U lines U0-U3 (bit n = Un), tested on shared bank $F0
;   bb:x/dd/aaaa  bank bb at $8000-$9FFF: bank register lines 0-3 (bb ^ 1/2/4/8), data lines D0-D7 and
;            address lines A0-A12.  Tested: the first bank of each shared RAM chip ($F0, $F4, $F8, $FC, with
;            U = 0; a missing chip shows as bad lines) and of each installed task RAM module.
;   Destructive: run before anything is kept in paged RAM.  Leaves RAM_BANK_REG and U at 0.
;   Records the chips that failed, for the MMU to leave unused (in task 0): ZP_M_BAD_SH (shared RAM chips,
;   used by SHARED_RAM_INIT) and ZP_M_BAD_MODS (task RAM modules, used by TASK_RAM_INIT).

.segment "POST_P2"

POST_S_RAM: .byte ASCII_CR, ASCII_LF, "RAM U:", 0

POST_RAM_TEST:
            ldx                 #0
:
            lda                 POST_S_RAM,X
            beq                 :+
            jsr                 POST_PUTC
            inx
            bra                 :-
:
            LOAD_ADDR           U_REGISTER, ZP_TEMP_VEC2
            lda                 #$F0
            sta                 RAM_BANK_REG
            lda                 #0                                  ; U lines, on shared bank $F0
            jsr                 POST_LINE_TEST
            jsr                 POST_PUTHEX
            LOAD_ADDR           RAM_BANK_REG, ZP_TEMP_VEC2
            lda                 #$F0                                ; Shared RAM (U = 0): each chip's first bank

@shared:
            ldx                 #$03                                ; Bank lines 2-3 pick the chip: only lines
            stx                 ZP_M_TEMP                           ;   0-1 count against this one
            jsr                 POST_BANK_TEST                      ; Z = 0: bad
            clc
            beq                 :+
            sec
:
            ror                 ZP_M_BAD_SH                         ; Chip c -> bit 4 + c
            clc
            adc                 #4
            bcc                 @shared
            lsr                 ZP_M_BAD_SH                         ; Chip c -> bit c
            lsr                 ZP_M_BAD_SH
            lsr                 ZP_M_BAD_SH
            lsr                 ZP_M_BAD_SH
            jsr                 MMU_PROBE_MODULES                   ; Task RAM: each installed module's first bank
            lda                 #0

@module:
            lsr                 ZP_M_MODS + 1
            ror                 ZP_M_MODS
            bcc                 :+                                  ; Not installed (C = 0: not bad)
            ldx                 #$0F                                ; All 4 bank lines are the module's
            stx                 ZP_M_TEMP
            jsr                 POST_BANK_TEST                      ; Z = 0: bad
            clc
            beq                 :+
            sec
:
            ror                 ZP_M_BAD_MODS + 1                   ; Module m -> bit m (after all 16)
            ror                 ZP_M_BAD_MODS
            clc
            adc                 #$10
            bcc                 @module                             ; (Module 15 is the shared banks: never
            stz                 RAM_BANK_REG                        ;   installed, so never tested)
            stz                 U_REGISTER
            rts

; Test paged RAM bank .A ($8000-$9FFF) and print " bb:x/dd/aaaa": the bad bank lines (x, see
; POST_LINE_TEST), data lines (dd: D0-D7) and address lines (aaaa: A0-A12); bit set = bad line.
; ZP_TEMP_VEC2 must point at RAM_BANK_REG.  Leaves the bank selected.
; IN: ZP_M_TEMP = the bank lines that count against this bank.  OUT: Z = 0 if any line is bad.  Preserves .A
POST_BANK_TEST:
            pha
            pha
            lda                 #' '
            jsr                 POST_PUTC
            pla
            jsr                 POST_PUTBYTE
            lda                 #':'
            jsr                 POST_PUTC
            pla
            pha
            jsr                 POST_LINE_TEST
            pha
            and                 ZP_M_TEMP
            sta                 ZP_M_TEMP                           ; ZP_M_TEMP = all of this bank's bad lines
            pla
            jsr                 POST_PUTHEX
            lda                 #'/'
            jsr                 POST_PUTC
            stz                 ZP_TEMP                             ; Data lines: walking one at $8000
            ldx                 #1
:
            stx                 PAGED_RAM_BASE
            txa
            eor                 PAGED_RAM_BASE                      ; Bits that read back wrong
            tsb                 ZP_TEMP
            txa
            asl
            tax
            bne                 :-
            lda                 ZP_TEMP
            tsb                 ZP_M_TEMP
            jsr                 POST_PUTBYTE
            lda                 #'/'
            jsr                 POST_PUTC

; Address lines: $8000 = 0, $8000 + 2^n = n + 1 (n = 0-12).  Line n is bad if $8000 + 2^n reads back
; wrong, or its write landed on $8000.
            stz                 ZP_TEMP_VEC                         ; Bad lines
            stz                 ZP_TEMP_VEC + 1
            stz                 PAGED_RAM_BASE
            ldx                 #0                                  ; Pass 0 writes, pass 1 checks

@pass:
            lda                 #1
            sta                 ZP_TEMP_VEC4                        ; 2^n
            stz                 ZP_TEMP_VEC4 + 1
            ldy                 #1                                  ; n + 1

@line:
            lda                 ZP_TEMP_VEC4
            sta                 ZP_TEMP_VEC3
            lda                 ZP_TEMP_VEC4 + 1
            ora                 #>PAGED_RAM_BASE
            sta                 ZP_TEMP_VEC3 + 1
            tya
            cpx                 #0
            bne                 :+
            sta                 (ZP_TEMP_VEC3)
            bra                 @next
:
            cmp                 PAGED_RAM_BASE
            beq                 @bad
            cmp                 (ZP_TEMP_VEC3)
            beq                 @next

@bad:
            lda                 ZP_TEMP_VEC4
            tsb                 ZP_TEMP_VEC
            lda                 ZP_TEMP_VEC4 + 1
            tsb                 ZP_TEMP_VEC + 1

@next:
            iny
            asl                 ZP_TEMP_VEC4
            rol                 ZP_TEMP_VEC4 + 1
            lda                 ZP_TEMP_VEC4 + 1
            cmp                 #$20                                ; Past A12
            bne                 @line
            inx
            cpx                 #2
            bne                 @pass
            lda                 ZP_TEMP_VEC + 1
            tsb                 ZP_M_TEMP
            jsr                 POST_PUTBYTE
            lda                 ZP_TEMP_VEC
            tsb                 ZP_M_TEMP
            jsr                 POST_PUTBYTE
            pla
            ldx                 ZP_M_TEMP                           ; Z = 0: bad
            rts

; Bank line test: the byte at $8000, with the register at (ZP_TEMP_VEC2) (RAM_BANK_REG or U) set to
; .A ^ 8, .A ^ 4, .A ^ 2, .A ^ 1 and .A.  OUT: .A = bad lines (bit n = register bit n, n = 0-3); the
; register is left at .A.  Modifies: .X, .Y
POST_LINE_TEST:
            sta                 ZP_TEMP_2
            stz                 ZP_TEMP
            ldy                 #0                                  ; Pass 0 writes, pass 1 checks

@pass:
            ldx                 #$10

@line:
            txa
            lsr
            tax                                                     ; 8, 4, 2, 1, 0 (the marker)
            eor                 ZP_TEMP_2
            sta                 (ZP_TEMP_VEC2)
            txa
            cpy                 #0
            bne                 :+
            sta                 PAGED_RAM_BASE
            bra                 @next
:
            cmp                 PAGED_RAM_BASE
            beq                 @next
            tsb                 ZP_TEMP

@next:
            txa
            bne                 @line
            iny
            cpy                 #2
            bne                 @pass
            lda                 ZP_TEMP
            rts

; Print .A as 2 hex digits (POST_PUTBYTE) or its low nibble as 1 (POST_PUTHEX).  Modifies: .Y
POST_PUTBYTE:
            pha
            lsr
            lsr
            lsr
            lsr
            jsr                 POST_PUTHEX
            pla

POST_PUTHEX:
            and                 #$0F
            cmp                 #10
            bcc                 :+
            adc                 #'A' - '0' - 10 - 1                 ; (C = 1)
:
            adc                 #'0'
            jmp                 POST_PUTC
