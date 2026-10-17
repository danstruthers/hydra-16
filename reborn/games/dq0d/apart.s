; ****************************************************************************
; apart.s - (dq0d) apart (): how far apart two squares are, the more of the two ways' (dq0d.c's, in assembly: the
; denizens' minds ask it of every square they weigh, a hundred thousand times a dig, cc65's ints slow at it).
;
;   unsigned __fastcall__ apart (int dx, int dy);        |dx| or |dy|, the more

            .export     _apart
            .import     incsp2
            .importzp   sp, ptr1

            .code

_apart:
            cpx         #$80                ; |dy| (in .A .X), kept in ptr1
            bcc         @dy
            jsr         neg
@dy:        sta         ptr1
            stx         ptr1 + 1
            ldy         #1                  ; |dx| (on the C stack)
            lda         (sp),y
            tax
            dey
            lda         (sp),y
            cpx         #$80
            bcc         @dx
            jsr         neg
@dx:        cpx         ptr1 + 1            ; The more (the same: dy, as the C had it)
            bne         @more
            cmp         ptr1
            beq         @dy2
@more:      bcs         @done
@dy2:       lda         ptr1
            ldx         ptr1 + 1
@done:      jmp         incsp2

; .A .X negated
neg:
            clc
            eor         #$FF
            adc         #1
            pha
            txa
            eor         #$FF
            adc         #0
            tax
            pla
            rts
