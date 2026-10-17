; ****************************************************************************
; cast.s - (dq0p) view.c's look about worked out in its grid (the hub's LOOK_BANK, in the window): the squares round
; the character, a row of 64 to 128 bytes, each two (its flags, G_*, and its floor's height), the view's squares
; (2 g_r + 1 each way) in the middle, the eyes in theirs, and g_m more each way for the lamps' lines (a lit thing
; lying about lights the squares within its reach that it has a clear line to: things.c's lamps).  Each square of a
; row of the view's that can be seen (not rock, within the range, not behind, lit: within the light's reach, a
; room's light, a lamp's) is seen if a line from the eyes reaches it with nothing in the way (no wall or rock, no
; shut door, no floor above the line): Bresenham's line and its heights (ez + (tz - ez) * k / n) as view.c's clear ()
; had them, worked out a step at a time without dividing; seen, it's marked (G_VIS, G_SEEN), and the walls, doors
; and windows round it too if it's a room's floor (G_INSIDE), those in the view.  (In C, the squares reached
; through at () thousands of times, a look took some 1.6 s of the senses' CPU.)
;
;   void __fastcall__ cast_row (unsigned char j);   the view's row j (0 to 2 g_r)
;   void cast_own (void);                           the character's own square: seen if anything lights it (else
;                                                   ;   remembered), the walls round it
;   void __fastcall__ cast_lamp (unsigned char w);  the light layer's (light.c): a lamp's squares, w each way round
;                                                   ;   it (the lamp g_li[0], g_lj[0], its reach g_lr[0]), each it
;                                                   ;   lights marked G_LAMP (its own, or a clear line to it)
;
; Each square cast_row and cast_own mark, the row's marks reach it: g_lo[j] and g_hi[j], the first and the last
; column of row j marked (the view's; g_lo 0xFF, none: view.c sets them so first), so that view.c marks the chunks'
; squares only there.

            .macpack    longbranch
            .export     _cast_row, _cast_own, _cast_lamp
            .export     _g_r, _g_m, _g_r2, _g_l2, _g_u2, _g_hx, _g_hy, _g_ez, _g_bright, _g_lmax
            .export     _g_nl, _g_li, _g_lj, _g_lr, _g_lzl, _g_lzh, _g_lo, _g_hi
            .importzp   ptr1, ptr2

G_OPAQUE    = $01                           ; (view.c's: the eye doesn't see past it,
G_SKIP      = $02                           ;   nothing to see on it (rock, or no square),
G_LIT       = $04                           ;   a room's light reaches it,
G_INSIDE    = $08                           ;   a room's floor,
G_WALL      = $10                           ;   a wall, a door, a window,
G_LAMP      = $20                           ;   lit by the lamp (cast_lamp's),
G_VIS       = $40                           ;   seen now, seen: the chunks' T_VIS, T_SEEN)
G_SEEN      = $80

            .bss
_g_r:       .res        1                   ; The range
_g_m:       .res        1                   ; The lamps' squares more, each way
_g_r2:      .res        2                   ; The range's square, the light's (night vision's already),
_g_l2:      .res        2                   ;   ultravision's
_g_u2:      .res        2
_g_hx:      .res        1                   ; The way the character faces (-1, 0, 1 each way)
_g_hy:      .res        1
_g_ez:      .res        2                   ; The eyes' height (its floor's and five feet)
_g_bright:  .res        1                   ; Its own light, or ultravision (its own square seen)
_g_lmax:    .res        1                   ; The greater of their reaches, in squares
_g_nl:      .res        1                   ; The lamps: how many, each one's square in the grid, its reach, the
_g_li:      .res        4                   ;   light's height (its floor's and two feet)
_g_lj:      .res        4
_g_lr:      .res        4
_g_lzl:     .res        4
_g_lzh:     .res        4
_g_lo:      .res        45                  ; Each row's marks: the first column, the last (the view's 2 g_r + 1 at
_g_hi:      .res        45                  ;   most, 45)
rj:         .res        1                   ; (The square: its row and column in the grid,
ri:         .res        1
dy:         .res        1                   ;   from the eyes, each way,
dx:         .res        1
ady:        .res        1                   ;   how far (its square: sqy, d2),
adx:        .res        1
sqy:        .res        2
d2:         .res        2
fy:         .res        1                   ;   ahead (the row's part: behind if less than 0))
ilo:        .res        1                   ; (cast_row's columns that may be seen: the view's, ilo to ihi,
ihi:        .res        1                   ;   iend after them; e, fy as a square next to the eyes is seen
iend:       .res        1                   ;   behind)
ef:         .res        1
lmx:        .res        1                   ; (cast_row's: |dx| under it lit by the light carried or ultravision,
lq:         .res        2                   ;   the greater of their squares lq; 0, none)
wlim:       .res        1                   ; (The view's squares each way: 2 g_r + 1)
eye:        .res        2                   ; (The eyes' square in the grid)
reye:       .res        2                   ; (A line's: from its square, its height, to the square at ptr2
rez:        .res        2                   ;   (rdx, rdy; how far, radx, rady),
rdx:        .res        1
rdy:        .res        1
radx:       .res        1
rady:       .res        1
e:          .res        1                   ;   Bresenham's error,
e2:         .res        1
n:          .res        1                   ;   its steps,
sxl:        .res        1                   ;   a step along and across, in the grid,
sxh:        .res        1
syl:        .res        1
syh:        .res        1
ad:         .res        2                   ;   the rise or fall to the square's floor,
neg:        .res        1
acc:        .res        2                   ;   what's left of it over the steps so far,
line:       .res        2                   ;   and the line's height there
el:         .res        2                   ;   (a square's floor, 16 bits))
lk:         .res        1                   ; (A lamp)

            .segment    "SW1RODATA"
sq:         .word       0, 1, 4, 9, 16, 25, 36, 49, 64, 81, 100, 121, 144, 169, 196, 225, 256, 289, 324, 361
            .word       400, 441, 484
wdx:        .byte       $FF, 0, 1, $FF, 1, $FF, 0, 1            ; (The eight round a square)
wdy:        .byte       $FF, $FF, $FF, 0, 0, 1, 1, 1
wol:        .byte       <-130, <-128, <-126, <-2, <2, <126, <128, <130
woh:        .byte       >-130, >-128, >-126, >-2, >2, >126, >128, >130
wall:       .byte       G_WALL

            .segment    "SW1CODE"
; The grid's square (column .X, row .A) in .A (low) and .Y (high): $8000 + row * 128 + column * 2
square:
            lsr         a
            ora         #$80
            tay
            lda         #0
            ror         a
            sta         el
            txa
            asl         a
            ora         el
            rts

; The eyes' square, and the view's size
setup:
            lda         _g_r
            asl         a
            clc
            adc         #1
            sta         wlim
            lda         _g_r
            clc
            adc         _g_m
            tax
            jsr         square
            sta         eye
            sty         eye + 1
            rts

_cast_own:
            jsr         setup
            lda         _g_r
            clc
            adc         _g_m
            sta         ri
            sta         rj
            lda         eye
            sta         ptr2
            lda         eye + 1
            sta         ptr2 + 1
            ldy         #0
            lda         (ptr2),y
            and         #G_LIT              ; Lit: by a room's light, its own, ultravision, a lamp
            ora         _g_bright
            bne         @lit
            jsr         lamps
            bcs         @lit
            ldy         #0
            lda         (ptr2),y
            ora         #G_SEEN
            sta         (ptr2),y
            jmp         noted
@lit:       ldy         #0
            lda         (ptr2),y
            ora         #G_VIS | G_SEEN
            sta         (ptr2),y
            jsr         noted
            ldy         #0
            lda         (ptr2),y
            and         #G_INSIDE
            beq         @done
            jmp         walls
@done:      rts

; cast_lamp (w): w in .A
_cast_lamp:
            sta         wlim
            lda         #0
            sta         rj
@row:       lda         #0
            sta         ri
@sq:        ldx         ri
            lda         rj
            jsr         square
            sta         ptr2
            sty         ptr2 + 1
            ldy         #0
            lda         (ptr2),y
            and         #G_SKIP
            bne         @next
            jsr         lamps
            bcc         @next
            ldy         #0
            lda         (ptr2),y
            ora         #G_LAMP
            sta         (ptr2),y
@next:      inc         ri
            lda         ri
            cmp         wlim
            bcc         @sq
            inc         rj
            lda         rj
            cmp         wlim
            bcc         @row
            rts

; cast_row (j): j in .A
_cast_row:
            pha
            jsr         setup
            pla
            sec                                 ; The row's part: from the eyes, its square, ahead
            sbc         _g_r
            sta         dy
            bpl         @dy
            eor         #$FF
            clc
            adc         #1
@dy:        sta         ady
            asl         a
            tax
            lda         sq,x
            sta         sqy
            lda         sq + 1,x
            sta         sqy + 1
            lda         _g_hy
            beq         @fy
            bmi         @fyn
            lda         dy
            jmp         @fy
@fyn:       lda         #0
            sec
            sbc         dy
@fy:        sta         fy
            ldx         _g_r                ; The columns that may be seen: in range (|dx| at most dxm, sq[dxm] +
@dm:        txa                             ;   sqy within r2),
            asl         a
            tay
            lda         sq,y
            clc
            adc         sqy
            sta         d2
            lda         sq + 1,y
            adc         sqy + 1
            sta         d2 + 1
            lda         _g_r2
            cmp         d2
            lda         _g_r2 + 1
            sbc         d2 + 1
            bcs         @dmok
            dex
            bpl         @dm
            rts                             ; (None)
@dmok:      stx         adx
            lda         _g_l2               ; (The columns the light carried or ultravision reaches: lmx)
            ldy         _g_l2 + 1
            cmp         _g_u2
            tya
            sbc         _g_u2 + 1
            bcs         @lq
            lda         _g_u2
            ldy         _g_u2 + 1
            jmp         @lqs
@lq:        lda         _g_l2
@lqs:       sta         lq
            sty         lq + 1
            ldx         adx                 ;   (from the light's reach, or the range's if it's less)
            cpx         _g_lmax
            bcc         @lm
            ldx         _g_lmax
@lm:        txa
            asl         a
            tay
            lda         sq,y
            clc
            adc         sqy
            sta         d2
            lda         sq + 1,y
            adc         sqy + 1
            sta         d2 + 1
            lda         lq
            cmp         d2
            lda         lq + 1
            sbc         d2 + 1
            bcs         @lmok
            dex
            bpl         @lm
@lmok:      inx
            stx         lmx
            lda         _g_r
            sec
            sbc         adx
            sta         ilo
            lda         _g_r
            clc
            adc         adx
            sta         ihi
            lda         fy                  ;   and not behind (hx dx + fy at least 0), but next to the eyes
            ldx         ady                 ;   (|dx|, |dy| 1 at most): ef, fy or (next to them) 1 if more
            cpx         #2
            bcs         @ef
            cmp         #1
            bpl         @ef
            lda         #1
@ef:        sta         ef
            lda         _g_hx
            beq         @hx0
            bmi         @hxn
            lda         _g_r                ; (hx 1: dx at least -ef)
            sec
            sbc         ef
            bmi         @in
            cmp         ilo
            bcc         @in
            sta         ilo
            jmp         @in
@hxn:       lda         _g_r                ; (hx -1: dx at most ef)
            clc
            adc         ef
            bmi         @none
            cmp         ihi
            bcs         @in
            sta         ihi
            jmp         @in
@hx0:       lda         fy                  ; (hx 0: all if fy at least 0, else those next to the eyes)
            bpl         @in
            lda         ady
            cmp         #2
            bcs         @none
            lda         _g_r
            sec
            sbc         #1
            cmp         ilo
            bcc         @hx1
            sta         ilo
@hx1:       lda         _g_r
            clc
            adc         #1
            cmp         ihi
            bcs         @in
            sta         ihi
@in:        lda         ihi
            cmp         ilo
            bcc         @none
            clc
            adc         #1
            sta         iend
            lda         dy                  ; Its first square
            clc
            adc         _g_r
            clc
            adc         _g_m
            sta         rj
            lda         ilo
            clc
            adc         _g_m
            sta         ri
            tax
            lda         rj
            jsr         square
            sta         ptr2
            sty         ptr2 + 1
            jmp         @sq
@none:      rts
@sq:        ldy         #0                  ; Each square: something to see,
            lda         (ptr2),y
            tax
            and         #G_SKIP
            jne         @next
            lda         _g_nl               ;   (quickly: not a room's light, no lamps, beyond the light
            bne         @full               ;   carried and ultravision, it's not lit)
            txa
            and         #G_LIT
            bne         @full
            lda         ri
            sec
            sbc         _g_m
            sec
            sbc         _g_r
            bpl         @qx
            eor         #$FF
            clc
            adc         #1
@qx:        cmp         lmx
            jcs         @next
@full:      lda         ri
            sec
            sbc         _g_m
            sec
            sbc         _g_r
            sta         dx
            bpl         @dx
            eor         #$FF
            clc
            adc         #1
@dx:        sta         adx
            asl         a
            tax
            lda         sq,x                ;   within the range (not the eyes' own),
            clc
            adc         sqy
            sta         d2
            lda         sq + 1,x
            adc         sqy + 1
            sta         d2 + 1
            ora         d2
            jeq         @next
            lda         _g_r2
            cmp         d2
            lda         _g_r2 + 1
            sbc         d2 + 1
            jcc         @next
            lda         _g_hx               ;   not behind (but beside),
            beq         @fx
            bmi         @fxn
            lda         dx
            jmp         @fx
@fxn:       lda         #0
            sec
            sbc         dx
@fx:        clc
            adc         fy
            bpl         @ahead
            lda         d2 + 1
            jne         @next
            lda         d2
            cmp         #3
            jcs         @next
@ahead:     lda         _g_l2               ;   lit (within the light's reach, ultravision's; a room's light, a
            cmp         d2                  ;   lamp's),
            lda         _g_l2 + 1
            sbc         d2 + 1
            bcs         @lit
            lda         _g_u2
            cmp         d2
            lda         _g_u2 + 1
            sbc         d2 + 1
            bcs         @lit
            ldy         #0
            lda         (ptr2),y
            and         #G_LIT
            bne         @lit
            jsr         lamps
            bcc         @next
@lit:       lda         eye                 ;   and a line to it from the eyes clear: seen
            sta         reye
            lda         eye + 1
            sta         reye + 1
            lda         _g_ez
            sta         rez
            lda         _g_ez + 1
            sta         rez + 1
            lda         dx
            sta         rdx
            lda         dy
            sta         rdy
            lda         adx
            sta         radx
            lda         ady
            sta         rady
            jsr         ray
            bcc         @next
            ldy         #0
            lda         (ptr2),y
            ora         #G_VIS | G_SEEN
            sta         (ptr2),y
            jsr         noted
            ldy         #0
            lda         (ptr2),y
            and         #G_INSIDE
            beq         @next
            jsr         walls
@next:      lda         ptr2
            clc
            adc         #2
            sta         ptr2
            inc         ri
            lda         ri
            sec
            sbc         _g_m
            cmp         iend
            jcc         @sq
            rts

; The square at ptr2 (ri, rj) lit by a lamp: within its reach (a square of it), and its own or a clear line to it
; from the light (carry set)
lamps:
            ldx         #0
@lamp:      cpx         _g_nl
            bcc         @one
            clc
            rts
@one:       stx         lk
            lda         ri
            sec
            sbc         _g_li,x
            sta         rdx
            bpl         @rx
            eor         #$FF
            clc
            adc         #1
@rx:        sta         radx
            cmp         _g_lr,x
            beq         @rxok
            bcs         @nope
@rxok:      lda         rj
            sec
            sbc         _g_lj,x
            sta         rdy
            bpl         @ry
            eor         #$FF
            clc
            adc         #1
@ry:        sta         rady
            cmp         _g_lr,x
            beq         @ryok
            bcs         @nope
@ryok:      ora         radx
            bne         @line
            sec                             ; (Its own square)
            rts
@line:      lda         _g_lzl,x
            sta         rez
            lda         _g_lzh,x
            sta         rez + 1
            lda         _g_lj,x             ; (Its square)
            pha
            lda         _g_li,x
            tax
            pla
            jsr         square
            sta         reye
            sty         reye + 1
            jsr         ray
            bcc         @nope
            rts
@nope:      ldx         lk
            inx
            jmp         @lamp

; The line from reye (its height rez) to the square at ptr2 (rdx, rdy; radx, rady): carry set if it's clear
ray:
            lda         rdx                 ; Its steps, along and across (back when it's none: clear ()'s)
            beq         @sxn
            bmi         @sxn
            lda         #2
            sta         sxl
            lda         #0
            sta         sxh
            beq         @sy
@sxn:       lda         #<-2
            sta         sxl
            lda         #>-2
            sta         sxh
@sy:        lda         rdy
            beq         @syn
            bmi         @syn
            lda         #$80
            sta         syl
            lda         #0
            sta         syh
            beq         @e
@syn:       lda         #$80
            sta         syl
            lda         #$FF
            sta         syh
@e:         lda         radx                ; Its error, its steps
            sec
            sbc         rady
            sta         e
            lda         radx
            cmp         rady
            bcs         @n
            lda         rady
@n:         sta         n
            ldy         #1                  ; The rise or fall to the square's floor (tz - ez), and which
            lda         (ptr2),y
            sta         el
            and         #$80
            beq         @pos
            lda         #$FF
@pos:       sta         el + 1
            lda         el
            sec
            sbc         rez
            sta         ad
            lda         el + 1
            sbc         rez + 1
            sta         ad + 1
            lda         #0
            sta         neg
            sta         acc
            sta         acc + 1
            lda         ad + 1
            bpl         @line
            inc         neg
            lda         #0
            sec
            sbc         ad
            sta         ad
            lda         #0
            sbc         ad + 1
            sta         ad + 1
@line:      lda         rez                 ; The line from the eyes' height
            sta         line
            lda         rez + 1
            sta         line + 1
            lda         reye
            sta         ptr1
            lda         reye + 1
            sta         ptr1 + 1
@step:      lda         e                   ; A step: e2 = 2e; along if e2 >= -ady, across if e2 <= adx
            asl         a
            sta         e2
            clc
            adc         rady
            bmi         @across
            lda         e
            sec
            sbc         rady
            sta         e
            lda         ptr1
            clc
            adc         sxl
            sta         ptr1
            lda         ptr1 + 1
            adc         sxh
            sta         ptr1 + 1
@across:    lda         radx
            sec
            sbc         e2
            bmi         @there
            lda         e
            clc
            adc         radx
            sta         e
            lda         ptr1
            clc
            adc         syl
            sta         ptr1
            lda         ptr1 + 1
            adc         syh
            sta         ptr1 + 1
@there:     lda         ptr1                ; The square itself: clear
            cmp         ptr2
            bne         @on
            lda         ptr1 + 1
            cmp         ptr2 + 1
            bne         @on
            sec
            rts
@on:        ldy         #0                  ; Rock, a wall, a shut door: not
            lda         (ptr1),y
            and         #G_OPAQUE
            bne         @no
            lda         acc                 ; The line's height a step on: (ad * k) / n, a step at a time
            clc
            adc         ad
            sta         acc
            lda         acc + 1
            adc         ad + 1
            sta         acc + 1
@dda:       lda         acc
            sec
            sbc         n
            tax
            lda         acc + 1
            sbc         #0
            bcc         @floor
            stx         acc
            sta         acc + 1
            lda         neg
            bne         @down
            inc         line
            bne         @dda
            inc         line + 1
            jmp         @dda
@down:      lda         line
            bne         @dec
            dec         line + 1
@dec:       dec         line
            jmp         @dda
@floor:     ldy         #1                  ; A floor above the line: not
            lda         (ptr1),y
            sta         el
            and         #$80
            beq         @up
            lda         #$FF
@up:        sta         el + 1
            lda         line
            cmp         el
            lda         line + 1
            sbc         el + 1
            bvc         @sign
            eor         #$80
@sign:      bmi         @no
            jmp         @step
@no:        clc
            rts

; The walls, doors and windows round the square at ptr2 (ri, rj) seen too, those in the view
walls:
            ldx         #7
@w:         lda         ri
            clc
            adc         wdx,x
            sec
            sbc         _g_m
            cmp         wlim
            bcs         @skip
            lda         rj
            clc
            adc         wdy,x
            sec
            sbc         _g_m
            cmp         wlim
            bcs         @skip
            lda         ptr2
            clc
            adc         wol,x
            sta         ptr1
            lda         ptr2 + 1
            adc         woh,x
            sta         ptr1 + 1
            ldy         #0
            lda         (ptr1),y
            bit         wall
            beq         @skip
            ora         #G_VIS | G_SEEN
            sta         (ptr1),y
            lda         rj                  ; (Noted: its row and column, the view's)
            clc
            adc         wdy,x
            sec
            sbc         _g_m
            tay
            lda         ri
            clc
            adc         wdx,x
            sec
            sbc         _g_m
            jsr         note
@skip:      dex
            bpl         @w
            rts

; The square at ri, rj marked: its row's marks reach it (noted: the view's column and row, its own)
noted:
            lda         rj
            sec
            sbc         _g_m
            tay
            lda         ri
            sec
            sbc         _g_m
; Column .A of row .Y marked: g_lo[.Y] and g_hi[.Y] reach it (.X kept)
note:
            cmp         _g_lo,y
            bcs         @lo
            sta         _g_lo,y
@lo:        cmp         _g_hi,y
            bcc         @hi
            sta         _g_hi,y
@hi:        rts

; ---- A chunk's run of squares (run_p its squares, run_q their floors) into piece (main RAM's: things.c's; their
; flags, then their floors from piece + 128), the light layer's light on them (light_in), and into a row of the
; grid's (grid_put: run_g, two bytes a square, the grid in the window); or the marks back from the grid's (copied
; into piece as they are) into the chunk

            .export     _run_in, _run_out, _run_clear, _run_p, _run_q, _run_g, _light_in, _run_l, _run_lo, _run_lb
            .export     _grid_put, _grid_fill, _grid_clr
            .import     _piece
            .importzp   ptr3

G_DOOR      = $20                           ; (run_in's in piece: a door or a window, view.c's to look at; G_LAMP
                                            ;   is the grid's alone)

            .bss
_run_p:     .res        2
_run_q:     .res        2
_run_g:     .res        2
_run_l:     .res        2                   ; (light_in's: the light layer's grid, the run's first square's byte,
_run_lo:    .res        1                   ;   1 if it's that byte's high half; the senses' bank)
_run_lb:    .res        1
cnt:        .res        1
doors:      .res        1

            .segment    "SW1RODATA"
kind:       .byte       G_OPAQUE | G_SKIP, G_INSIDE, G_OPAQUE | G_WALL, G_WALL | G_DOOR, 0, G_INSIDE
            .byte       G_WALL | G_DOOR, G_INSIDE, G_INSIDE, G_INSIDE           ; (world.h's T_ROCK ... T_PIT;
            .res        22, 0                                   ;   the rest 0)

            .segment    "SW1CODE"
; unsigned char __fastcall__ run_in (unsigned char n): its squares' flags (what each is: kind; G_DOOR, a door or a
; window) into piece, their floors into piece + 128; not 0 if a door or a window is among them
_run_in:
            sta         cnt
            jsr         ptrs
            stz         doors
            ldy         #0
@sq:        lda         (ptr1),y
            and         #$1F
            tax
            lda         kind,x
            sta         _piece,y
            tsb         doors
            lda         (ptr2),y
            sta         _piece + 128,y
            iny
            cpy         cnt
            bne         @sq
            lda         doors
            and         #G_DOOR
            ldx         #0
            rts

; void __fastcall__ grid_put (unsigned char n): piece's squares (their flags, their floors) into the grid's at run_g
_grid_put:
            sta         cnt
            jsr         ptrs
            ldx         #0
            ldy         #0
@sq:        lda         _piece,x
            sta         (ptr3),y
            iny
            lda         _piece + 128,x
            sta         (ptr3),y
            iny
            inx
            cpx         cnt
            bne         @sq
            rts

; void __fastcall__ grid_fill (unsigned char n): the grid's squares at run_g nothing (outside the stratum, or a chunk
; not made for the lamps' lines)
_grid_fill:
            asl         a
            sta         cnt
            jsr         ptrs
            ldy         #0
@sq:        lda         #G_OPAQUE | G_SKIP
            sta         (ptr3),y
            iny
            lda         #0
            sta         (ptr3),y
            iny
            cpy         cnt
            bne         @sq
            rts

; void __fastcall__ grid_clr (unsigned char n): the grid's squares at run_g not marked (the last view's marks out)
_grid_clr:
            asl         a
            sta         cnt
            jsr         ptrs
            ldy         #0
@sq:        lda         (ptr3),y
            and         #<~(G_VIS | G_SEEN)
            sta         (ptr3),y
            iny
            iny
            cpy         cnt
            bne         @sq
            rts

; void __fastcall__ run_out (unsigned char n): the grid's marks (G_VIS, G_SEEN) into its squares
_run_out:
            sta         cnt
            jsr         ptrs
@sq:        ldy         #0
            lda         (ptr3),y
            and         #G_VIS | G_SEEN
            beq         @g
            ora         (ptr1),y
            sta         (ptr1),y
@g:         inc         ptr1
            bne         @p
            inc         ptr1 + 1
@p:         lda         ptr3
            clc
            adc         #2
            sta         ptr3
            bcc         @n
            inc         ptr3 + 1
@n:         dec         cnt
            bne         @sq
            rts

; void __fastcall__ run_clear (unsigned char n): its squares no longer seen now (T_VIS)
_run_clear:
            tax
            jsr         ptrs
            ldy         #0
@sq:        lda         (ptr1),y
            and         #<~G_VIS
            sta         (ptr1),y
            iny
            dex
            bne         @sq
            rts

; void __fastcall__ light_in (unsigned char n): the light layer's squares (light.c's grid, at run_l: half a byte
; each, from its low half, or its high, run_lo) into the run's flags (piece): G_LIT where light reaches; the senses'
; bank (run_lb) left in the window
_light_in:
            sta         cnt
            lda         _run_lb
            sta         $00                 ; (The task's bank register)
            lda         _run_l
            sta         ptr1
            lda         _run_l + 1
            sta         ptr1 + 1
            ldy         #0
            ldx         _run_lo
            bne         @hi
@lo:        lda         (ptr1)              ; (The low half: the high half next)
            and         #$0F
            beq         @n1
            lda         _piece,y
            ora         #G_LIT
            sta         _piece,y
@n1:        iny
            cpy         cnt
            beq         @done
@hi:        lda         (ptr1)              ; (The high half: the byte after it next)
            and         #$F0
            beq         @n2
            lda         _piece,y
            ora         #G_LIT
            sta         _piece,y
@n2:        inc         ptr1
            bne         @n3
            inc         ptr1 + 1
@n3:        iny
            cpy         cnt
            bne         @lo
@done:      rts

ptrs:
            lda         _run_p
            sta         ptr1
            lda         _run_p + 1
            sta         ptr1 + 1
            lda         _run_q
            sta         ptr2
            lda         _run_q + 1
            sta         ptr2 + 1
            lda         _run_g
            sta         ptr3
            lda         _run_g + 1
            sta         ptr3 + 1
            rts

; ---- view.c's grid made, a column of chunks at a time, a chunk's rows at a time (rect), moved (grid_shift), and its
; marks put in the chunks or taken out of them (mark_rows): the stratum the view's (v_sn: view.c's stratum () sets it
; and its bounds, v_x0, v_y0, v_h, and the grid's bank, v_look; v_lmode, the light layer's lamps' lines)
;
;   void rect (void);                               the grid's rows r_j0 ... (r_wj of them), its columns from r_i0
;                                                   ;   (r_wi of them): the squares from r_x, r_y (a shut door,
;                                                   ;   but a portcullis, opaque; the light layer's light, but in
;                                                   ;   v_lmode: a chunk not made yet nothing; outside the stratum,
;                                                   ;   nothing)
;   void grid_shift (void);                         the grid moved, sh_dx, sh_dy (each less than sh_w, its size):
;                                                   ;   its square i, j the one at i + sh_dx, j + sh_dy was
;   void __fastcall__ mark_rows (unsigned char u);  the last view's rows' marks (bx0, by0, bw; blo, bhi) put in
;                                                   ;   the chunks (u 0), or taken out (u 1: no longer seen now)
;   unsigned char __fastcall__ band (unsigned x, unsigned x1);  the squares in x's column of chunks, to x1 at most

            .export     _rect, _grid_shift, _mark_rows, _band
            .export     _v_sn, _v_x0, _v_y0, _v_h, _v_look, _v_lmode, _r_x, _r_y, _r_i0, _r_j0, _r_wi, _r_wj
            .export     _lbk, _bx0, _by0, _bw, _blo, _bhi, _sh_dx, _sh_dy, _sh_w
            .import     _at, _cached, _light_grid, _portal_at, _hub_map, _give_way, _at_bank, _at_chunk
            .import     _cx_, _cy_, pusha, pushax, incsp2
            .importzp   sp

PS_OPEN     = $01                           ; (world.h's: a portal's state, open; its kind a portcullis)
PK_PORTCULLIS = 2
P_KIND      = 2                             ; (struct portal's kind, state)
P_STATE     = 3

            .bss
_v_sn:      .res        1                   ; The stratum, its x0, y0, h
_v_x0:      .res        2
_v_y0:      .res        2
_v_h:       .res        2
_v_look:    .res        1                   ; LOOK_BANK
_v_lmode:   .res        1
_r_x:       .res        2                   ; (rect's)
_r_y:       .res        2
_r_i0:      .res        1
_r_j0:      .res        1
_r_wi:      .res        1
_r_wj:      .res        1
_lbk:       .res        1                   ; (The chunk its light's grid is of: its bank, its place; the grid)
lck:        .res        2
lgp:        .res        2
_bx0:       .res        2                   ; The last view's squares: from bx0, by0, bw each way (0: none), each
_by0:       .res        2                   ;   row's marks from blo to bhi (0xFF: none)
_bw:        .res        1
_blo:       .res        45
_bhi:       .res        45
_sh_dx:     .res        1                   ; (grid_shift's)
_sh_dy:     .res        1
_sh_w:      .res        1
bq:         .res        2                   ; (A square's x, and the last's: band's)
bq1:        .res        2
bqn:        .res        1
rx1:        .res        2                   ; (rect's: the last column's x; the rows' end; the row, its y,
rj1:        .res        1                   ;   from the stratum's y0; the chunk's rows, the row in them; the
gj:         .res        1                   ;   columns, their first's byte in the row; the chunk's square)
ryy:        .res        2
roy:        .res        2
rk:         .res        1
rr:         .res        1
rn:         .res        1
rc:         .res        1
rp:         .res        2
ra:         .res        1
tl:         .res        1
th:         .res        1
mun:        .res        1                   ; (mark_rows's)
mj:         .res        1
mi:         .res        1
mn:         .res        1
mcnt:       .res        1
sax:        .res        1                   ; (grid_shift's)
say:        .res        1
sf:         .res        1
st:         .res        1
snb:        .res        1
sj:         .res        1

            .segment    "SW1CODE"
; band (x, x1): x1 in .A .X, x on the C stack
_band:
            sta         bq1
            stx         bq1 + 1
            ldy         #0
            lda         (sp),y
            sta         bq
            iny
            lda         (sp),y
            sta         bq + 1
            jsr         bandq
            ldx         #0
            jmp         incsp2

; The squares from bq in its column of chunks, to bq1 at most: CW - ((x - x0) & (CW - 1)), or x1 - x + 1, in .A
bandq:
            lda         bq
            sec
            sbc         _v_x0
            and         #31
            eor         #31
            clc
            adc         #1
            sta         bqn
            lda         bq1
            sec
            sbc         bq
            tay
            lda         bq1 + 1
            sbc         bq + 1
            bne         @n
            iny
            beq         @n
            cpy         bqn
            bcs         @n
            tya
            rts
@n:         lda         bqn
            rts

; (The stratum, bq, and ryy on the C stack, ryy in .A .X: at ()'s, cached ()'s arguments)
pushq:
            lda         _v_sn
            jsr         pusha
            lda         bq
            ldx         bq + 1
            jsr         pushax
            lda         ryy
            ldx         ryy + 1
            rts

; run_g the grid's row gj + rr, its byte rc: $8000 + row * 128 + rc
rowg:
            lda         gj
            clc
            adc         rr
            lsr         a
            tax
            lda         #0
            ror         a
            clc
            adc         rc
            sta         _run_g
            txa
            ora         #$80
            sta         _run_g + 1
            rts

; The doors and windows in piece's rn squares (run_in's G_DOOR): its mark out, and a shut one (but a portcullis)
; opaque (portal_at (), the chunk in the window: its row rr from at ()'s cy_)
rdoors:
            lda         #0
            sta         ra
@d:         ldy         ra
            lda         _piece,y
            and         #G_DOOR
            beq         @dn
            lda         _piece,y
            and         #<~G_DOOR
            sta         _piece,y
            lda         _cx_
            clc
            adc         ra
            jsr         pusha
            lda         _cy_
            clc
            adc         rr
            jsr         _portal_at
            sta         ptr1
            stx         ptr1 + 1
            ora         ptr1 + 1
            beq         @dn
            ldy         #P_STATE
            lda         (ptr1),y
            and         #PS_OPEN
            bne         @dn
            ldy         #P_KIND
            lda         (ptr1),y
            cmp         #PK_PORTCULLIS
            beq         @dn
            ldy         ra
            lda         _piece,y
            ora         #G_OPAQUE
            sta         _piece,y
@dn:        inc         ra
            lda         ra
            cmp         rn
            bcc         @d
            rts

_rect:
            lda         _r_x                ; The last column's x, the rows' end, the first column
            clc
            adc         _r_wi
            sta         rx1
            lda         _r_x + 1
            adc         #0
            sta         rx1 + 1
            lda         rx1
            bne         @x1
            dec         rx1 + 1
@x1:        dec         rx1
            lda         _r_j0
            clc
            adc         _r_wj
            sta         rj1
            lda         _r_x
            sta         bq
            lda         _r_x + 1
            sta         bq + 1
            lda         rx1
            sta         bq1
            lda         rx1 + 1
            sta         bq1 + 1
            lda         _r_i0
            asl         a
            sta         rc
@band:      jsr         _give_way           ; A column of chunks: its squares (giving way to the UI's command)
            jsr         bandq
            sta         rn
            lda         _r_j0
            sta         gj
            lda         _r_y
            sta         ryy
            lda         _r_y + 1
            sta         ryy + 1
@seg:       lda         ryy                 ; A chunk's rows: those left in it (in the stratum or out of it),
            sec
            sbc         _v_y0
            sta         roy
            lda         ryy + 1
            sbc         _v_y0 + 1
            sta         roy + 1
            lda         roy
            and         #31
            eor         #31
            clc
            adc         #1
            sta         rk
            lda         roy
            cmp         _v_h
            lda         roy + 1
            sbc         _v_h + 1
            bcs         @kj
            lda         _v_h
            sec
            sbc         roy
            tay
            lda         _v_h + 1
            sbc         roy + 1
            bne         @kj
            cpy         rk
            bcs         @kj
            sty         rk
@kj:        lda         rj1                 ;   the rect's at most
            sec
            sbc         gj
            cmp         rk
            bcs         @k
            sta         rk
@k:         lda         _v_lmode            ; Its chunk (the lamps' lines: one the cache has)
            beq         @at
            jsr         pushq
            jsr         _cached
            cmp         #0
            jeq         @fill
@at:        jsr         pushq
            jsr         _at
            sta         rp
            stx         rp + 1
            ora         rp + 1
            jeq         @fill
            lda         _v_lmode
            bne         @rows
            lda         _at_bank            ; Its light's grid: found once a chunk, each look
            cmp         _lbk
            bne         @lg
            lda         _at_chunk
            cmp         lck
            bne         @lg
            lda         _at_chunk + 1
            cmp         lck + 1
            beq         @rows
@lg:        lda         _at_bank
            sta         _lbk
            lda         _at_chunk
            sta         lck
            lda         _at_chunk + 1
            sta         lck + 1
            jsr         pushq
            jsr         pushax
            lda         #0
            jsr         _light_grid
            sta         lgp
            stx         lgp + 1
@rows:      lda         #0
            sta         rr
@row:       lda         _at_bank            ; A row: its squares, a chunk's rows CW apart (its floors CW * CW on),
            jsr         _hub_map
            lda         rp
            sta         _run_p
            sta         _run_q
            lda         rp + 1
            sta         _run_p + 1
            clc
            adc         #4
            sta         _run_q + 1
            lda         rn
            jsr         _run_in
            cmp         #0
            beq         @lit
            jsr         rdoors              ;   its doors,
@lit:       lda         _v_lmode            ;   its light (lgp + (cy_ + rr) * 16 + cx_ / 2),
            bne         @put
            lda         lgp
            ora         lgp + 1
            beq         @put
            lda         _cy_
            clc
            adc         rr
            sta         tl
            lsr         a
            lsr         a
            lsr         a
            lsr         a
            sta         th
            lda         tl
            asl         a
            asl         a
            asl         a
            asl         a
            sta         tl
            lda         _cx_
            lsr         a
            ora         tl
            clc
            adc         lgp
            sta         _run_l
            lda         th
            adc         lgp + 1
            sta         _run_l + 1
            lda         _cx_
            and         #1
            sta         _run_lo
            lda         rn
            jsr         _light_in
@put:       lda         _v_look             ;   into the grid
            jsr         _hub_map
            jsr         rowg
            lda         rn
            jsr         _grid_put
            lda         rp
            clc
            adc         #32
            sta         rp
            bcc         @rn
            inc         rp + 1
@rn:        inc         rr
            lda         rr
            cmp         rk
            jcc         @row
@segn:      lda         gj                  ; The next chunk's rows
            clc
            adc         rk
            sta         gj
            lda         ryy
            clc
            adc         rk
            sta         ryy
            bcc         @sj
            inc         ryy + 1
@sj:        lda         gj
            cmp         rj1
            jcc         @seg
            lda         bq                  ; The next column of chunks
            clc
            adc         rn
            sta         bq
            bcc         @bc
            inc         bq + 1
@bc:        lda         rn
            asl         a
            clc
            adc         rc
            sta         rc
            lda         rx1
            cmp         bq
            lda         rx1 + 1
            sbc         bq + 1
            jcs         @band
            rts
@fill:      lda         _v_look             ; (Nothing: outside the stratum, or not made for the lamps)
            jsr         _hub_map
            lda         #0
            sta         rr
@f:         jsr         rowg
            lda         rn
            jsr         _grid_fill
            inc         rr
            lda         rr
            cmp         rk
            bcc         @f
            jmp         @segn

; (ptr1 the grid's row .A, its byte sf; ptr2 the same, st)
srow:
            lsr         a
            tax
            lda         #0
            ror         a
            clc
            adc         sf
            sta         ptr1
            txa
            ora         #$80
            sta         ptr1 + 1
            rts
drow:
            lsr         a
            tax
            lda         #0
            ror         a
            clc
            adc         st
            sta         ptr2
            txa
            ora         #$80
            sta         ptr2 + 1
            rts
; (snb bytes from ptr1 to ptr2, back to front if it's moved right: a row to itself)
scopy:
            ldy         snb
            beq         @r
            lda         _sh_dx
            bmi         @back
            ldy         #0
@f:         lda         (ptr1),y
            sta         (ptr2),y
            iny
            cpy         snb
            bne         @f
@r:         rts
@back:      dey
            lda         (ptr1),y
            sta         (ptr2),y
            cpy         #0
            bne         @back
            rts

_grid_shift:
            lda         _sh_dx              ; Its rows' bytes kept, from sf to st: 2 (w - |dx|)
            bpl         @ax
            eor         #$FF
            clc
            adc         #1
@ax:        sta         sax
            asl         a
            tax
            lda         #0
            sta         sf
            sta         st
            lda         _sh_dx
            beq         @nb
            bmi         @tn
            stx         sf
            jmp         @nb
@tn:        stx         st
@nb:        lda         _sh_w
            sec
            sbc         sax
            asl         a
            sta         snb
            lda         _sh_dy
            bpl         @ay
            eor         #$FF
            clc
            adc         #1
@ay:        sta         say
            lda         _sh_dy
            bmi         @up
            lda         #0                  ; (Moved down: row j from j + |dy|, from the top)
            sta         sj
@dn:        lda         sj
            clc
            adc         say
            cmp         _sh_w
            bcs         @done
            jsr         srow
            lda         sj
            jsr         drow
            jsr         scopy
            inc         sj
            jmp         @dn
@up:        lda         _sh_w               ; (Up: row j from j - |dy|, from the bottom)
            sta         sj
@u:         dec         sj
            lda         sj
            cmp         say
            bcc         @done
            sec
            sbc         say
            jsr         srow
            lda         sj
            jsr         drow
            jsr         scopy
            jmp         @u
@done:      rts

_mark_rows:
            sta         mun
            lda         #0
            sta         mj
@r:         lda         mj                  ; Each row of the last view's, its marks
            cmp         _bw
            jcs         @done
            tax
            lda         _blo,x
            cmp         #$FF
            jeq         @nr
            jsr         _give_way
            ldx         mj
            lda         _blo,x              ; (From bx0 + blo, row by0 + j, bhi - blo + 1 squares)
            sta         mi
            clc
            adc         _bx0
            sta         bq
            lda         _bx0 + 1
            adc         #0
            sta         bq + 1
            lda         _bhi,x
            sec
            sbc         _blo,x
            clc
            adc         #1
            sta         mn
            lda         mj
            clc
            adc         _by0
            sta         ryy
            lda         _by0 + 1
            adc         #0
            sta         ryy + 1
@seg:       lda         bq                  ; A chunk's of them
            clc
            adc         mn
            sta         bq1
            lda         bq + 1
            adc         #0
            sta         bq1 + 1
            lda         bq1
            bne         @b1
            dec         bq1 + 1
@b1:        dec         bq1
            jsr         bandq
            sta         rk
            lda         mun
            bne         @at
            lda         _v_look             ; (The grid's marks into piece)
            jsr         _hub_map
            lda         mj
            lsr         a
            tax
            lda         #0
            ror         a
            sta         ptr1
            lda         mi
            asl         a
            clc
            adc         ptr1
            sta         ptr1
            txa
            ora         #$80
            sta         ptr1 + 1
            lda         rk
            asl         a
            sta         mcnt
            ldy         #0
@cp:        lda         (ptr1),y
            sta         _piece,y
            iny
            cpy         mcnt
            bne         @cp
@at:        jsr         pushq
            jsr         _at
            sta         _run_p
            stx         _run_p + 1
            ora         _run_p + 1
            beq         @nx
            lda         #<_piece
            sta         _run_g
            lda         #>_piece
            sta         _run_g + 1
            lda         rk
            ldx         mun
            bne         @clr
            jsr         _run_out
            jmp         @nx
@clr:       jsr         _run_clear
@nx:        lda         mn
            sec
            sbc         rk
            sta         mn
            beq         @nr
            lda         bq
            clc
            adc         rk
            sta         bq
            bcc         @mi
            inc         bq + 1
@mi:        lda         mi
            clc
            adc         rk
            sta         mi
            jmp         @seg
@nr:        inc         mj
            jmp         @r
@done:      rts
