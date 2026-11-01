; ****************************************************************************
; run.inc - (a part's: the senses' part 2, dq0p/run.s; the world's part 23, dq0w/run.s; RUNSEG the part's segment)
; ../dq0/run.h's run_of in assembly, for the parts' room: a passage's run in a walk's steps (run_s, headings, eighths),
; its length (run_n) and its bearing in sixteenths.  Two headings an eighth or a quarter apart, the one that comes
; twice running the main one, the other never twice running: a slant, its bearing the steps' sum, if it's three steps
; or more and the other came twice (every step, with no main one: the two by turns, its bearing halfway between them);
; else straight on its first heading, as far as it goes.  Its state the part's own (none kept from one call to the next).

            .export     _run_of, _run_s, _run_n
            .import     _head_x, _head_y

            .segment    RUNSEG

_run_s:     .res        8                   ; (RUN_MAX)
_run_n:     .res        1
rcnt:       .res        1                   ; (n)
ra:         .res        1                   ; (The first heading)
rb:         .res        1                   ; (The other: $FF none yet)
rm:         .res        1                   ; (The main one: $FF none yet)
rdx:        .res        1                   ; (The steps' sum)
rdy:        .res        1
ta:         .res        1                   ; (b16's: |dx|, |dy|, a multiple)
tb:         .res        1
tc:         .res        1

; unsigned char __fastcall__ run_of (unsigned char n): n in .A
_run_of:
            sta         rcnt
            lda         #$FF
            sta         rb
            sta         rm
            lda         _run_s
            sta         ra
            tax
            lda         _head_x,x
            sta         rdx
            lda         _head_y,x
            sta         rdy
            ldy         #1
@step:      cpy         rcnt
            bcs         @end
            lda         _run_s,y
            cmp         ra
            beq         @twice
            ldx         rb
            cpx         #$FF
            bne         @hasb
            sec                             ; (The other's first: an eighth or a quarter off, (h - a + 2) & 7 0, 1, 3, 4)
            sbc         ra
            clc
            adc         #2
            and         #7
            cmp         #2
            beq         @end
            cmp         #5
            bcs         @end
            lda         _run_s,y
            sta         rb
            bra         @twice
@hasb:      cmp         rb
            bne         @end                ; (A third heading)
@twice:     lda         _run_s,y
            cmp         _run_s - 1,y
            bne         @sum
            ldx         rm                  ; (Twice running: the main one)
            cpx         #$FF
            bne         @hasm
            sta         rm
            bra         @sum
@hasm:      cmp         rm
            bne         @end                ; (The other twice running: a bend)
@sum:       tax
            lda         _head_x,x
            clc
            adc         rdx
            sta         rdx
            lda         _head_y,x
            clc
            adc         rdy
            sta         rdy
            iny
            bra         @step
@end:       lda         rb                  ; (.Y its steps)
            cmp         #$FF
            beq         @straight
            cpy         #3
            bcc         @straight
            sty         _run_n
            ldx         #0                  ; (The other's steps: those not the main one)
            dey
@count:     lda         _run_s,y
            cmp         rm
            beq         @main
            inx
@main:      dey
            bpl         @count
            cpx         #2
            bcs         @slant
@straight:  ldy         #1                  ; (Straight on its first heading, as far as that goes)
@on:        cpy         rcnt
            bcs         @out
            lda         _run_s,y
            cmp         ra
            bne         @out
            iny
            bra         @on
@out:       sty         _run_n
            lda         ra
            asl         a
            ldx         #0
            rts

@slant:     lda         rm                  ; (A main one: the steps' sum's bearing)
            cmp         #$FF
            bne         b16
            lda         rb                  ; (None, the two by turns: between them, a + (b - a) / 2 in eighths)
            sec
            sbc         ra
            and         #7
            cmp         #4
            bcc         @half
            ora         #$F8                ; ((b - a) less 8: the other way round)
@half:      sta         tc
            lda         ra
            asl         a
            clc
            adc         tc
            and         #15
            ldx         #0
            rts

; The bearing of (rdx, rdy) in sixteenths: from north toward east, 5a < b 0, 3a < 2b 1, 2a < 3b 2, a < 5b 3, else 4
; (a, b: |dx|, |dy|; tan 11.25 degrees near 1/5, tan 33.75 near 2/3); south of east-west 8 less it, west 16 less it
b16:        lda         rdx
            bpl         @ax
            eor         #$FF
            inc         a
@ax:        sta         ta
            lda         rdy
            bpl         @ay
            eor         #$FF
            inc         a
@ay:        sta         tb
            ldx         #0
            lda         ta                  ; 5a < b
            asl         a
            asl         a
            adc         ta
            cmp         tb
            bcc         @got
            inx
            lda         tb                  ; 3a < 2b
            asl         a
            sta         tc
            lda         ta
            asl         a
            adc         ta
            cmp         tc
            bcc         @got
            inx
            lda         tb                  ; 2a < 3b
            asl         a
            adc         tb
            sta         tc
            lda         ta
            asl         a
            cmp         tc
            bcc         @got
            inx
            lda         tb                  ; a < 5b
            asl         a
            asl         a
            adc         tb
            sta         tc
            lda         ta
            cmp         tc
            bcc         @got
            inx
@got:       txa
            ldx         rdy
            beq         @north
            bmi         @north
            eor         #$FF                ; (8 - s)
            clc
            adc         #9
@north:     ldx         rdx
            bpl         @east
            eor         #$FF                ; (16 - s)
            clc
            adc         #17
            and         #15
@east:      ldx         #0
            rts
