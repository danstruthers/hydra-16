.debuginfo

; ****************************************************************************
; The hardware test's ROM tests: a CRC-16 of each BIOS ROM page (U6, by W) and of each paged ROM bank the
; image uses (U31, by the ROM bank register), checked against the table the build puts at the end of this
; bank (HWT_SUMS: tools/romsum.js); and the lines that select them.  (Inside `.scope HWTEST`: hwtest.s.)
;
; The CRC is CRC-16/CCITT-FALSE, table-driven: its tables are made in task 0's RAM (HWT_CRC_LO, _HI), and
; the routine that reads a range runs there too (HWT_CRC_RAM): it switches the paged ROM bank away from the
; one this runs from.

.segment "HWT_C"

HWT_CRC_LO          = HWT_RAM           ; (256 bytes)
HWT_CRC_HI          = HWT_RAM + $100
HWT_CRC_RAM         = HWT_RAM + $200    ; HWT_RAMCODE, copied

; The tables (from the polynomial $1021) and the RAM routine.  Modifies: .A, .X, .Y
HWT_CRC_INIT:
            ldx         #0

@entry:
            stx         HWT_CRC + 1                         ; (Entry .X: .X << 8, shifted 8 times)
            stz         HWT_CRC
            ldy         #8
@bit:
            asl         HWT_CRC
            rol         HWT_CRC + 1
            bcc         :+
            lda         HWT_CRC
            eor         #$21
            sta         HWT_CRC
            lda         HWT_CRC + 1
            eor         #$10
            sta         HWT_CRC + 1
:
            dey
            bne         @bit
            lda         HWT_CRC
            sta         HWT_CRC_LO,X
            lda         HWT_CRC + 1
            sta         HWT_CRC_HI,X
            inx
            bne         @entry
            ldx         #HWT_RAMCODE_SIZE - 1
:
            lda         HWT_RAMCODE,X
            sta         HWT_CRC_RAM,X
            dex
            bpl         :-
            rts

; One byte (.A) into HWT_CRC.  Modifies: .A, .X
.macro HWT_CRC_BYTE
            eor         HWT_CRC + 1
            tax
            lda         HWT_CRC
            eor         HWT_CRC_HI,X
            sta         HWT_CRC + 1
            lda         HWT_CRC_LO,X
            sta         HWT_CRC
.endmacro

; The RAM routine (runs at HWT_CRC_RAM: branches only, no jumps within it): HWT_CRC over whole pages from
; HWT_P + 1 up to page HWT_T1, then HWT_T2 bytes of that one, with paged ROM bank .A selected (and this bank
; again after).  Modifies: .A, .X, .Y, HWT_P
HWT_RAMCODE:
            sta         ROM_BANK_REG
            stz         HWT_P
@page:
            lda         HWT_P + 1
            cmp         HWT_T1
            beq         @last
            ldy         #0
@byte:
            lda         (HWT_P),Y
            HWT_CRC_BYTE
            iny
            bne         @byte
            inc         HWT_P + 1
            bra         @page
@last:
            ldy         #0
@lbyte:
            cpy         HWT_T2
            beq         @done
            lda         (HWT_P),Y
            HWT_CRC_BYTE
            iny
            bra         @lbyte
@done:
            lda         #HWT_BANK
            sta         ROM_BANK_REG
            rts
HWT_RAMCODE_SIZE = * - HWT_RAMCODE
.assert     HWT_RAMCODE_SIZE <= 128, error, "HWT_RAMCODE: 128 bytes at most (its copy loop)"

; HWT_CRC of paged ROM bank .A's $A000 to page .X, and .Y bytes of that page.  Modifies: .A, .X, .Y
HWT_CRC_BANK:
            stx         HWT_T1
            sty         HWT_T2
            ldx         #$A0
            stx         HWT_P + 1
            ldx         #$FF
            stx         HWT_CRC
            stx         HWT_CRC + 1
            jmp         HWT_CRC_RAM

; The expected checksum for entry .X of HWT_SUMS's CRCs (the BIOS pages', then the banks') in .A.Y; and
; HWT_CRC compared with it: Z = 1 if they're the same.  Modifies: .A, .Y
HWT_CRC_IS:
            txa
            asl
            tay
            lda         HWT_SUMS + 2,Y
            pha
            lda         HWT_SUMS + 3,Y
            tay
            pla
            cmp         HWT_CRC
            bne         :+
            cpy         HWT_CRC + 1
:
            rts

; ****************************************************************************
; The BIOS ROM: each page's CRC.  A bad page: FAIL and its number; and if it reads as the page one W line
; away from it should, that line.
HWT_T_BIOS:
            jsr         HWT_CRC_INIT
            stz         HWT_T3                              ; (The page)

@page:
            lda         HWT_T3
            sta         W_REGISTER                          ; (Not this code's: it's in the paged ROM)
            ldx         #$FF
            ldy         #0
            lda         #$E0
            sta         HWT_P + 1
            lda         #$FF
            sta         HWT_CRC
            sta         HWT_CRC + 1
            stx         HWT_T1
            sty         HWT_T2
            lda         #HWT_BANK
            jsr         HWT_CRC_RAM
            stz         W_REGISTER
            ldx         HWT_T3
            jsr         HWT_CRC_IS
            beq         @next
            jsr         HWT_FAIL
            .byte       "page ", 0
            lda         HWT_T3
            jsr         HWT_HEX1
            stz         HWT_T6                              ; (Is it another's, a W line away?)
@line:
            ldx         HWT_T6
            lda         HWT_T3
            eor         HWT_BITS,X
            cmp         HWT_SUMS                            ; (Pages in the image)
            bcs         @no_line
            tax
            jsr         HWT_CRC_IS
            bne         @no_line
            jsr         HWT_PRINT
            .byte       " (W line ", 0
            lda         HWT_T6
            jsr         HWT_HEX1
            lda         #')'
            jsr         HWT_PUTC
            bra         @next
@no_line:
            inc         HWT_T6
            ldx         HWT_T6
            cpx         #6
            bne         @line

@next:
            inc         HWT_T3
            lda         HWT_T3
            cmp         HWT_SUMS
            bne         @page
            rts

; ****************************************************************************
; The paged ROM: each bank the image uses, its CRC; then the bank lines: bank 2^n (n = 1-7) mustn't read as
; bank 0 does (its first page), as it would with that line stuck.  A fault: FAIL, and the bank or line.
HWT_T_PAGED:
            jsr         HWT_CRC_INIT
            stz         HWT_T3                              ; (The bank)

@bank:
            lda         HWT_T3
            ldx         #$DF
            ldy         #<(HWT_SUMS - $DF00)
            jsr         HWT_CRC_BANK
            lda         HWT_T3
            clc
            adc         HWT_SUMS                            ; (Its CRC's entry: after the pages')
            tax
            jsr         HWT_CRC_IS
            beq         @next
            jsr         HWT_FAIL
            .byte       "bank ", 0
            lda         HWT_T3
            jsr         HWT_HEX2

@next:
            inc         HWT_T3
            lda         HWT_T3
            cmp         HWT_SUMS + 1
            bne         @bank
            lda         #0                                  ; Bank 0's first page ...
            ldx         #$A1
            ldy         #0
            jsr         HWT_CRC_BANK
            lda         HWT_CRC
            sta         HWT_T4
            lda         HWT_CRC + 1
            sta         HWT_T5
            ldy         #1                                  ;   and bank 2^n's
            sty         HWT_T3

@line:
            lda         HWT_BITS,Y
            ldx         #$A1
            ldy         #0
            jsr         HWT_CRC_BANK
            lda         HWT_CRC
            cmp         HWT_T4
            bne         :+
            lda         HWT_CRC + 1
            cmp         HWT_T5
            bne         :+
            jsr         HWT_FAIL
            .byte       "bank line ", 0
            lda         HWT_T3
            jsr         HWT_HEX1
:
            inc         HWT_T3
            ldy         HWT_T3
            cpy         #8
            bne         @line
            rts

.segment "HWT_SUMS"
            .res        256                                 ; (tools/romsum.js fills them in)
