; ****************************************************************************
; draw.inc - vid's /draw (vid.s includes it): drawing on layer 0's bitmap (ctl's bitmap 320 D or 640 D), done in the
; driver, so it works under the console's text, from any language, as text commands: pen C, plot X Y, line X0 Y0 X1
; Y1, box X0 Y0 X1 Y1 (its outline), bar X0 Y0 X1 Y1 (filled), circle X Y R, disc X Y R (filled), clear [C]
; (the whole bitmap: colour 0, or C), text X Y STRING (the console's font, 8 x 8, its dots in the pen's colour, the
; rest left as it is: words a space apart).  Coordinates are the bitmap's pixels (320 x 240, or 640 x 480), each -4096 to 4095
; (outside that: E_INVAL); what falls off the bitmap isn't drawn.  The colour is the driver's, one for all (15 as it
; starts; the low bits the depth has), so each command needn't carry it and a shell's lines can each open /draw; it
; reads as "pen 15".  With no bitmap: E_INVAL; while the chip's claimed: E_BUSY.
;   A pixel's byte is (y * 5) << gk + (x >> gs): a row is 5 << gk bytes (40 to 320), gk 3 + the depth's log2 (+ 1,
; 640 across), gs 3 - the depth's log2; in its byte the leftmost pixel is in the high bits (pmask, pshift).  A line is
; Bresenham's, a circle the midpoint one, a bar rows of pixels (a run of bytes at 8 bits a pixel).

DRAW_FRONT      = 15            ; The colour as the driver starts

.zeropage
px:         .res        2                                   ; The pixel: x ...
py:         .res        2                                   ;   y ...
pcol:       .res        1                                   ;   and its colour (the depth's bits of it)
gk:         .res        1                                   ; The bitmap: a row's bytes 5 << gk ...
gs:         .res        1                                   ;   a pixel's byte x >> gs ...
gw:         .res        2                                   ;   its width ...
gh:         .res        2                                   ;   and height
pt:         .res        2                                   ; Scratch (the drawing's)
pa:         .res        2                                   ; A circle's point: a ...
pb:         .res        2                                   ;   and b, from its centre

.bss
dcolor:     .res        1                                   ; The colour (/draw's)
lx0:        .res        2                                   ; A line: from (lx0, ly0) ...
ly0:        .res        2
lx1:        .res        2                                   ;   to (lx1, ly1) ...
ly1:        .res        2
ladx:       .res        2                                   ;   |dx| ...
lady:       .res        2                                   ;   -|dy| ...
lsx:        .res        1                                   ;   the steps' signs (1, $FF) ...
lsy:        .res        1
lerr:       .res        2                                   ;   the error ...
le2:        .res        2                                   ;   and twice it
hx0:        .res        2                                   ; A row: from hx0 to hx1 (at py)
hx1:        .res        2
ccx:        .res        2                                   ; A circle: its centre ...
ccy:        .res        2
cxx:        .res        2                                   ;   x and y on it (from r, 0) ...
cyy:        .res        2
cdd:        .res        2                                   ;   and its decision
cfill:      .res        1                                   ;   <> 0: a disc
cnt3:       .res        3                                   ; clear's bytes
glyph:      .res        8                                   ; text's character, its rows
tx0:        .res        2                                   ; text's place: its column ...
ty0:        .res        2                                   ;   and row
tc:         .res        1                                   ; (text's character: its place in a word)
tw:         .res        1                                   ; (its word: 2 or 3)

.code

; ****************************************************************************
; The commands

; pen C: the colour
c_pen:
            lda         z:srv_argn
            beq         @inval
            lda         srv_arg
            sta         dcolor
            clc
            rts

@inval:
            jmp         inval

; draw's state: "pen 15"
gen_draw:
            lda         #<s_pen_sp
            ldx         #>s_pen_sp
            jsr         srv_tputs
            lda         dcolor
            ldx         #0
            jsr         srv_tputdec
            lda         #LF
            jsr         srv_tputc
            clc
            rts

; plot X Y
c_plot:
            lda         #2
            jsr         d_start
            bcs         @done
            ldx         #3
:
            lda         srv_arg,X
            sta         px,X                                ; (px, then py)
            dex
            bpl         :-
            jsr         d_plot
            clc
@done:
            rts

; line X0 Y0 X1 Y1
c_line:
            lda         #4
            jsr         d_start
            bcs         @done
            jsr         d_ends
            jsr         d_line
            clc
@done:
            rts

; box X0 Y0 X1 Y1: its four sides
c_box:
            lda         #4
            jsr         d_start
            bcs         @done
            jsr         d_ends
            lda         ly0                                 ; The top: (x0, y0) to (x1, y0)
            sta         ly1
            lda         ly0 + 1
            sta         ly1 + 1
            jsr         d_line
            jsr         d_ends                              ; The bottom: (x0, y1) to (x1, y1)
            lda         ly1
            sta         ly0
            lda         ly1 + 1
            sta         ly0 + 1
            jsr         d_line
            jsr         d_ends                              ; The left: (x0, y0) to (x0, y1)
            lda         lx0
            sta         lx1
            lda         lx0 + 1
            sta         lx1 + 1
            jsr         d_line
            jsr         d_ends                              ; The right: (x1, y0) to (x1, y1)
            lda         lx1
            sta         lx0
            lda         lx1 + 1
            sta         lx0 + 1
            jsr         d_line
            clc
@done:
            rts

; bar X0 Y0 X1 Y1: the box filled, a row at a time
c_bar:
            lda         #4
            jsr         d_start
            bcs         @done
            jsr         d_ends
            ldx         #0                                  ; x: hx0 the lesser, hx1 the greater
            jsr         d_order
            lda         lx0
            sta         hx0
            lda         lx0 + 1
            sta         hx0 + 1
            lda         lx1
            sta         hx1
            lda         lx1 + 1
            sta         hx1 + 1
            ldx         #ly0 - lx0                          ; y: the rows from ly0 to ly1
            jsr         d_order
            lda         ly0
            sta         py
            lda         ly0 + 1
            sta         py + 1
@row:
            jsr         d_hline
            lda         py                                  ; (The last row: done)
            cmp         ly1
            bne         :+
            lda         py + 1
            cmp         ly1 + 1
            beq         @end
:
            inc         py
            bne         @row
            inc         py + 1
            bra         @row

@end:
            clc
@done:
            rts

; circle X Y R, disc X Y R
c_circle:
            stz         cfill
            bra         circle

c_disc:
            lda         #1
            sta         cfill
circle:
            lda         #3
            jsr         d_start
            bcs         @done
            lda         srv_arg + 5                         ; (R: 0 or more)
            bmi         @inval
            ldx         #3
:
            lda         srv_arg,X
            sta         ccx,X                               ; (ccx, then ccy)
            dex
            bpl         :-
            jsr         d_circle
            clc
@done:
            rts

@inval:
            jmp         inval

; clear [C]: the whole bitmap in colour C (0)
c_clear:
            lda         #0
            jsr         d_start
            bcc         :+
            rts
:
            lda         z:srv_argn
            beq         :+
            lda         srv_arg
            ldx         bmdepth
            and         depth_mask,X
:
            ldx         bmdepth                             ; The colour in each of a byte's pixels
            beq         @one
            cpx         #3
            beq         @fill
            tay
            lda         byte_of,X                           ; (A byte of 2 or 4 bits a pixel: the colour times
            sta         pt                                  ;   $55 or $11)
            lda         #0
:
            dey
            bmi         @fill
            clc
            adc         pt
            bra         :-

@one:
            cmp         #0                                  ; (1 bit a pixel: all or none)
            beq         @fill
            lda         #$FF
@fill:
            sta         pt
            lda         gh                                  ; Its bytes: (gh * 5) << gk
            sta         cnt3
            lda         gh + 1
            sta         cnt3 + 1
            asl         cnt3
            rol         cnt3 + 1
            asl         cnt3
            rol         cnt3 + 1
            clc
            lda         cnt3
            adc         gh
            sta         cnt3
            lda         cnt3 + 1
            adc         gh + 1
            sta         cnt3 + 1
            stz         cnt3 + 2
            ldx         gk
:
            asl         cnt3
            rol         cnt3 + 1
            rol         cnt3 + 2
            dex
            bne         :-
            stz         VERA_ADDR_L                         ; From VRAM 0, a byte at a time
            stz         VERA_ADDR_M
            lda         #VERA_INC_1
            sta         VERA_ADDR_H
            stz         vok
            lda         pt
            ldx         cnt3 + 1                            ; The pages (cnt3's top 16 bits: .Y, .X) ...
            ldy         cnt3 + 2
@page:
            cpx         #0
            bne         :+
            cpy         #0
            beq         @rest
            dey
:
            dex
            phx
            ldx         #32                                 ; (256 bytes, 8 a turn)
:
            sta         VERA_DATA0
            sta         VERA_DATA0
            sta         VERA_DATA0
            sta         VERA_DATA0
            sta         VERA_DATA0
            sta         VERA_DATA0
            sta         VERA_DATA0
            sta         VERA_DATA0
            dex
            bne         :-
            plx
            bra         @page

@rest:
            ldx         cnt3                                ;   and the rest
            beq         :++
:
            sta         VERA_DATA0
            dex
            bne         :-
:
            clc
@done:
            rts

; text X Y STRING: the string (its words a space apart) in the console's font from (X, Y), a character each 8 across
c_text:
            lda         #2
            jsr         d_start
            bcs         @done
            lda         z:srv_argn
            cmp         #3
            bcc         @inval
            ldx         #3
:
            lda         srv_arg,X
            sta         tx0,X                               ; (tx0, then ty0)
            dex
            bpl         :-
            lda         #2                                  ; Its words: the third, then the fourth (the rest)
            sta         tw
@word:
            asl
            tax
            lda         srv_argp,X
            sta         pt
            lda         srv_argp + 1,X
            sta         pt + 1
            stz         tc
@char:
            ldy         tc
            lda         (pt),Y
            beq         @next
            jsr         d_char
            inc         tc
            bra         @char

@next:
            inc         tw                                  ; The next word, a space before it
            lda         tw
            cmp         z:srv_argn
            bcs         @end
            lda         #' '
            jsr         d_char
            lda         tw
            bra         @word

@end:
            clc
@done:
            rts

@inval:
            jmp         inval

; Character .A at (tx0, ty0), its dots in pcol; tx0 on 8.  Keeps pt
d_char:
            sta         glyph                               ; Its 8 rows from the font (VRAM $1F000 + 8 c)
            stz         glyph + 1
            asl         glyph
            rol         glyph + 1
            asl         glyph
            rol         glyph + 1
            asl         glyph
            rol         glyph + 1
            lda         glyph
            sta         VERA_ADDR_L
            lda         glyph + 1
            clc
            adc         #>FONT_ADDR
            sta         VERA_ADDR_M
            lda         #H_INC1
            sta         VERA_ADDR_H
            stz         vok
            ldx         #0
:
            lda         VERA_DATA0
            sta         glyph,X
            inx
            cpx         #8
            bcc         :-
            lda         pt                                  ; (pt: text's, d_plot's too)
            pha
            lda         pt + 1
            pha
            lda         ty0
            sta         py
            lda         ty0 + 1
            sta         py + 1
            ldy         #0
@row:
            phy
            lda         tx0
            sta         px
            lda         tx0 + 1
            sta         px + 1
            lda         glyph,Y
@dot:
            asl
            pha
            bcc         :+
            jsr         d_plot
:
            inc         px
            bne         :+
            inc         px + 1
:
            pla
            bne         @dot
            inc         py
            bne         :+
            inc         py + 1
:
            ply
            iny
            cpy         #8
            bcc         @row
            pla
            sta         pt + 1
            pla
            sta         pt
            clc                                             ; The next character's place
            lda         tx0
            adc         #8
            sta         tx0
            bcc         :+
            inc         tx0 + 1
:
            rts

; ****************************************************************************
; Their parts

; A command's start: its first .A words numbers from -4096 to 4095, the bitmap's geometry (gw, gh, gk, gs), and the
; colour (pcol).  OUT: C = 0; or C = 1, .A = E_INVAL (too few, out of range, no bitmap) or E_BUSY (claimed)
d_start:
            cmp         z:srv_argn                          ; (Its words)
            beq         :+
            bcs         @inval
:
            asl
            tay
@arg:
            dey
            dey
            bmi         @args
            lda         srv_arg + 1,Y                       ; (-4096 to 4095: its high byte $F0-$0F)
            clc
            adc         #$10
            cmp         #$20
            bcc         @arg
@inval:
            lda         #E_INVAL
            sec
            rts

@args:
            jsr         console_has
            bcs         @done
            lda         bitmap
            beq         @inval
            lda         bmdepth                             ; gk: 3 + the depth's log2; gs: 3 - it
            clc
            adc         #3
            sta         gk
            lda         #3
            sec
            sbc         bmdepth
            sta         gs
            LDR         gw, 320
            LDR         gh, 240
            lda         bitmap
            cmp         #2
            bne         :+
            inc         gk                                  ; (640 across: rows twice as long, twice as many)
            LDR         gw, 640
            LDR         gh, 480
:
            lda         dcolor
            ldx         bmdepth
            and         depth_mask,X
            sta         pcol
            clc
@done:
            rts

; A line's ends (lx0, ly0, lx1, ly1) from the command's four words
d_ends:
            ldx         #7
:
            lda         srv_arg,X
            sta         lx0,X
            dex
            bpl         :-
            rts

; lx0 + .X and lx1 + .X (2 bytes each) in order: the lesser first (signed)
d_order:
            sec
            lda         lx1,X
            sbc         lx0,X
            lda         lx1 + 1,X
            sbc         lx0 + 1,X
            bvc         :+
            eor         #$80
:
            bpl         @done                               ; (lx1 >= lx0: as they are)
            lda         lx0,X
            ldy         lx1,X
            sta         lx1,X
            tya
            sta         lx0,X
            lda         lx0 + 1,X
            ldy         lx1 + 1,X
            sta         lx1 + 1,X
            tya
            sta         lx0 + 1,X
@done:
            rts

; Is (px, py) on the bitmap?  OUT: C = 0 yes; C = 1 no
d_on:
            lda         px + 1
            bmi         @off
            cmp         gw + 1
            bcc         @y
            bne         @off
            lda         px
            cmp         gw
            bcs         @off
@y:
            lda         py + 1
            bmi         @off
            cmp         gh + 1
            bcc         @on
            bne         @off
            lda         py
            cmp         gh
            bcs         @off
@on:
            clc
            rts

@off:
            sec
            rts

; ADDR0 at (px, py)'s byte, .A its increment bits (ADDR_H's).  Modifies .A, .X, va, pt
d_addr:
            pha
            lda         py                                  ; va: (py * 5) << gk ...
            sta         va
            lda         py + 1
            sta         va + 1
            asl         va
            rol         va + 1
            asl         va
            rol         va + 1
            clc
            lda         va
            adc         py
            sta         va
            lda         va + 1
            adc         py + 1
            sta         va + 1
            stz         va + 2
            ldx         gk
:
            asl         va
            rol         va + 1
            rol         va + 2
            dex
            bne         :-
            lda         px                                  ;   + px >> gs
            sta         pt
            lda         px + 1
            sta         pt + 1
            ldx         gs
            beq         :++
:
            lsr         pt + 1
            ror         pt
            dex
            bne         :-
:
            clc
            lda         va
            adc         pt
            sta         va
            sta         VERA_ADDR_L
            lda         va + 1
            adc         pt + 1
            sta         va + 1
            sta         VERA_ADDR_M
            lda         va + 2
            adc         #0
            sta         va + 2
            and         #1
            sta         pt
            pla
            ora         pt
            sta         VERA_ADDR_H
            stz         vok                                 ; (ADDR0 isn't the terminal's now)
            rts

; Pixel (px, py) in pcol, if it's on the bitmap.  Modifies .A, .X, .Y, va, pt
d_plot:
            jsr         d_on
            bcs         @done
            lda         #0                                  ; (No increment: the byte read, then written)
            jsr         d_addr
            ldx         bmdepth
            cpx         #3
            bne         @bits
            lda         pcol                                ; 8 bits: the byte
            sta         VERA_DATA0
@done:
            rts

@bits:                                                      ; Fewer: its bits in the byte
            lda         px
            and         ppb_mask,X
            sta         pt
            txa
            asl
            asl
            asl
            ora         pt
            tay
            lda         pcol
            ldx         pshift,Y
            beq         :++
:
            asl
            dex
            bne         :-
:
            and         pmask,Y
            sta         pt
            lda         pmask,Y
            eor         #$FF
            and         VERA_DATA0
            ora         pt
            sta         VERA_DATA0
            rts

; A row: hx0 to hx1 (hx0 <= hx1) at py, in pcol, what's on the bitmap of it.  Modifies .A, .X, .Y, va, pt, px
d_hline:
            lda         py + 1                              ; (Its row off the bitmap: nothing)
            bmi         @out
            cmp         gh + 1
            bcc         :+
            bne         @out
            lda         py
            cmp         gh
            bcs         @out
:
            lda         hx1 + 1                             ; Its end left of it, or its start right of it: nothing
            bmi         @out
            lda         hx0 + 1
            bmi         @from0
            cmp         gw + 1
            bcc         @start
            bne         @out
            lda         hx0
            cmp         gw
            bcc         @start
@out:
            rts

@start:
            lda         hx0
            sta         px
            lda         hx0 + 1
            sta         px + 1
            bra         @end

@from0:
            stz         px
            stz         px + 1
@end:
            lda         hx1                                 ; pt: its last x, the bitmap's at most
            sta         pt
            lda         hx1 + 1
            sta         pt + 1
            sec
            lda         gw
            sbc         #1
            tax
            lda         gw + 1
            sbc         #0
            cmp         pt + 1
            bcc         :+
            bne         @last
            cpx         pt
            bcs         @last
:
            sta         pt + 1
            stx         pt
@last:
            lda         pt                                  ; (The count, less 1: last - first)
            sec
            sbc         px
            sta         lerr
            lda         pt + 1
            sbc         px + 1
            sta         lerr + 1
            lda         bmdepth
            cmp         #3
            bne         @each
            lda         #VERA_INC_1                         ; 8 bits a pixel: a run of bytes
            jsr         d_addr
            lda         pcol
            ldx         lerr
            ldy         lerr + 1
            iny
            inx
:
            sta         VERA_DATA0
            dex
            bne         :-
            dey
            bne         :-
@done:
            rts

@each:                                                      ; Fewer: a pixel at a time
            jsr         d_plot
            lda         lerr
            bne         :+
            lda         lerr + 1
            beq         @done
            dec         lerr + 1
:
            dec         lerr
            inc         px
            bne         @each
            inc         px + 1
            bra         @each

; The line from (lx0, ly0) to (lx1, ly1), Bresenham's: a pixel each step, x or y or both stepping.  Modifies .A, .X,
; .Y, va, pt, px, py and the line's state
d_line:
            sec                                             ; ldx: |x1 - x0|, lsx its sign
            lda         lx1
            sbc         lx0
            sta         ladx
            lda         lx1 + 1
            sbc         lx0 + 1
            sta         ladx + 1
            ldx         #1
            bit         ladx + 1
            bpl         :+
            ldx         #$FF
            sec
            lda         #0
            sbc         ladx
            sta         ladx
            lda         #0
            sbc         ladx + 1
            sta         ladx + 1
:
            stx         lsx
            sec                                             ; ldy: -|y1 - y0|, lsy its sign
            lda         ly1
            sbc         ly0
            sta         lady
            lda         ly1 + 1
            sbc         ly0 + 1
            sta         lady + 1
            ldx         #$FF
            bit         lady + 1
            bmi         :+
            ldx         #1
            sec
            lda         #0
            sbc         lady
            sta         lady
            lda         #0
            sbc         lady + 1
            sta         lady + 1
:
            stx         lsy
            clc                                             ; lerr: ladx + lady
            lda         ladx
            adc         lady
            sta         lerr
            lda         ladx + 1
            adc         lady + 1
            sta         lerr + 1
            ldx         #3                                  ; From (lx0, ly0) (px, py)
:
            lda         lx0,X
            sta         px,X
            dex
            bpl         :-
@step:
            jsr         d_plot
            lda         px                                  ; At (lx1, ly1): done
            cmp         lx1
            bne         @on
            lda         px + 1
            cmp         lx1 + 1
            bne         @on
            lda         py
            cmp         ly1
            bne         @on
            lda         py + 1
            cmp         ly1 + 1
            beq         @done
@on:
            lda         lerr                                ; le2: 2 lerr
            asl
            sta         le2
            lda         lerr + 1
            rol
            sta         le2 + 1
            sec                                             ; le2 >= ldy: x steps
            lda         le2
            sbc         lady
            lda         le2 + 1
            sbc         lady + 1
            bvc         :+
            eor         #$80
:
            bmi         @noX
            clc
            lda         lerr
            adc         lady
            sta         lerr
            lda         lerr + 1
            adc         lady + 1
            sta         lerr + 1
            ldx         #px
            lda         lsx
            jsr         d_step
@noX:
            sec                                             ; le2 <= ldx: y steps
            lda         ladx
            sbc         le2
            lda         ladx + 1
            sbc         le2 + 1
            bvc         :+
            eor         #$80
:
            bmi         @step
            clc
            lda         lerr
            adc         ladx
            sta         lerr
            lda         lerr + 1
            adc         ladx + 1
            sta         lerr + 1
            ldx         #py
            lda         lsy
            jsr         d_step
            jmp         @step

@done:
            rts

; The zero page's 16 bits at .X stepped by .A: 1 up, $FF down
d_step:
            cmp         #1
            bne         @down
            inc         0,X
            bne         :+
            inc         1,X
:
            rts

@down:
            lda         0,X
            bne         :+
            dec         1,X
:
            dec         0,X
            rts

; The circle at (ccx, ccy), radius srv_arg + 4, the midpoint way: its eight octants a step (cfill: rows between them)
d_circle:
            lda         srv_arg + 4                         ; x = r, y = 0, d = 1 - r
            sta         cxx
            lda         srv_arg + 5
            sta         cxx + 1
            stz         cyy
            stz         cyy + 1
            sec
            lda         #1
            sbc         cxx
            sta         cdd
            lda         #0
            sbc         cxx + 1
            sta         cdd + 1
@step:
            sec                                             ; While x >= y
            lda         cxx
            sbc         cyy
            lda         cxx + 1
            sbc         cyy + 1
            bmi         @done
            lda         cfill
            beq         :+
            jsr         d_rows
            bra         @next
:
            jsr         d_eight
@next:
            inc         cyy                                 ; y + 1
            bne         :+
            inc         cyy + 1
:
            bit         cdd + 1                             ; d < 0: d + 2y + 1
            bpl         @inward
            lda         cyy
            asl
            sta         pt
            lda         cyy + 1
            rol
            sta         pt + 1
            sec
            lda         cdd
            adc         pt
            sta         cdd
            lda         cdd + 1
            adc         pt + 1
            sta         cdd + 1
            bra         @step

@inward:                                                    ; Else x - 1, d + 2 (y - x) + 1
            lda         cxx
            bne         :+
            dec         cxx + 1
:
            dec         cxx
            sec
            lda         cyy
            sbc         cxx
            sta         pt
            lda         cyy + 1
            sbc         cxx + 1
            sta         pt + 1
            asl         pt
            rol         pt + 1
            sec
            lda         cdd
            adc         pt
            sta         cdd
            lda         cdd + 1
            adc         pt + 1
            sta         cdd + 1
            bra         @step

@done:
            rts

; The circle's eight points for (cxx, cyy): (ccx +- x, ccy +- y), (ccx +- y, ccy +- x)
d_eight:
            ldx         #0                                  ; (.X: 0, x and y as they are; 4, swapped)
@pair:
            phx
            jsr         @four
            plx
            cpx         #4
            beq         @done
            ldx         #4
            bra         @pair

@done:
            rts

@four:                                                      ; (ccx +- a, ccy +- b): a, b cxx, cyy or cyy, cxx
            jsr         d_ab
            jsr         @p
            jsr         d_negb
            jsr         @p
            jsr         d_nega
            jsr         @p
            jsr         d_negb
@p:
            clc
            lda         ccx
            adc         pa
            sta         px
            lda         ccx + 1
            adc         pa + 1
            sta         px + 1
            clc
            lda         ccy
            adc         pb
            sta         py
            lda         ccy + 1
            adc         pb + 1
            sta         py + 1
            jmp         d_plot

; A disc's rows for (cxx, cyy): ccx - a to ccx + a at ccy +- b, for (a, b) (x, y) and (y, x)
d_rows:
            ldx         #0
@pair:
            phx
            jsr         d_ab
            jsr         @row
            jsr         d_negb
            jsr         @row
            plx
            cpx         #4
            beq         @done
            ldx         #4
            bra         @pair

@done:
            rts

@row:
            sec
            lda         ccx
            sbc         pa
            sta         hx0
            lda         ccx + 1
            sbc         pa + 1
            sta         hx0 + 1
            clc
            lda         ccx
            adc         pa
            sta         hx1
            lda         ccx + 1
            adc         pa + 1
            sta         hx1 + 1
            clc
            lda         ccy
            adc         pb
            sta         py
            lda         ccy + 1
            adc         pb + 1
            sta         py + 1
            jmp         d_hline

; pa, pb: (cxx, cyy) for .X 0, (cyy, cxx) for .X 4
d_ab:
            cpx         #0
            bne         @swap
            lda         cxx
            sta         pa
            lda         cxx + 1
            sta         pa + 1
            lda         cyy
            sta         pb
            lda         cyy + 1
            sta         pb + 1
            rts

@swap:
            lda         cyy
            sta         pa
            lda         cyy + 1
            sta         pa + 1
            lda         cxx
            sta         pb
            lda         cxx + 1
            sta         pb + 1
            rts

; pb or pa negated
d_negb:
            sec
            lda         #0
            sbc         pb
            sta         pb
            lda         #0
            sbc         pb + 1
            sta         pb + 1
            rts

d_nega:
            sec
            lda         #0
            sbc         pa
            sta         pa
            lda         #0
            sbc         pa + 1
            sta         pa + 1
            rts

.rodata
s_pen_sp:   .byte       "pen ", 0
depth_mask: .byte       $01, $03, $0F, $FF                  ; A colour's bits at each depth (1, 2, 4, 8)
byte_of:    .byte       0, $55, $11                         ; A colour's byte, per unit: 2 and 4 bits a pixel
ppb_mask:   .byte       7, 3, 1                             ; A pixel's place in its byte: x's low bits
pshift:     .byte       7, 6, 5, 4, 3, 2, 1, 0              ; Its colour's shift: 1 bit a pixel ...
            .byte       6, 4, 2, 0, 0, 0, 0, 0              ;   2 ...
            .byte       4, 0, 0, 0, 0, 0, 0, 0              ;   4
pmask:      .byte       $80, $40, $20, $10, $08, $04, $02, $01
            .byte       $C0, $30, $0C, $03, 0, 0, 0, 0
            .byte       $F0, $0F, 0, 0, 0, 0, 0, 0
