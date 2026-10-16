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

            .macpack    longbranch
            .export     _cast_row, _cast_own
            .export     _g_r, _g_m, _g_r2, _g_l2, _g_u2, _g_hx, _g_hy, _g_ez, _g_bright
            .export     _g_nl, _g_li, _g_lj, _g_lr, _g_lzl, _g_lzh
            .importzp   ptr1, ptr2

G_OPAQUE    = $01                           ; (view.c's: the eye doesn't see past it,
G_SKIP      = $02                           ;   nothing to see on it (rock, or no square),
G_LIT       = $04                           ;   a room's light reaches it,
G_INSIDE    = $08                           ;   a room's floor,
G_WALL      = $10                           ;   a wall, a door, a window,
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
_g_nl:      .res        1                   ; The lamps: how many, each one's square in the grid, its reach, the
_g_li:      .res        4                   ;   light's height (its floor's and two feet)
_g_lj:      .res        4
_g_lr:      .res        4
_g_lzl:     .res        4
_g_lzh:     .res        4
rj:         .res        1                   ; (The square: its row and column in the grid,
ri:         .res        1
dy:         .res        1                   ;   from the eyes, each way,
dx:         .res        1
ady:        .res        1                   ;   how far (its square: sqy, d2),
adx:        .res        1
sqy:        .res        2
d2:         .res        2
fy:         .res        1                   ;   ahead (the row's part: behind if less than 0))
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

            .rodata
sq:         .word       0, 1, 4, 9, 16, 25, 36, 49, 64, 81, 100, 121, 144, 169, 196, 225, 256, 289, 324, 361
            .word       400, 441, 484
wdx:        .byte       $FF, 0, 1, $FF, 1, $FF, 0, 1            ; (The eight round a square)
wdy:        .byte       $FF, $FF, $FF, 0, 0, 1, 1, 1
wol:        .byte       <-130, <-128, <-126, <-2, <2, <126, <128, <130
woh:        .byte       >-130, >-128, >-126, >-2, >2, >126, >128, >130
wall:       .byte       G_WALL

            .code

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
            rts
@lit:       ldy         #0
            lda         (ptr2),y
            ora         #G_VIS | G_SEEN
            sta         (ptr2),y
            and         #G_INSIDE
            beq         @done
            jmp         walls
@done:      rts

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
            lda         dy                  ; Its first square
            clc
            adc         _g_r
            clc
            adc         _g_m
            sta         rj
            ldx         _g_m
            stx         ri
            jsr         square
            sta         ptr2
            sty         ptr2 + 1
@sq:        ldy         #0                  ; Each square: something to see,
            lda         (ptr2),y
            and         #G_SKIP
            jne         @next
            lda         ri
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
            cmp         wlim
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
@skip:      dex
            bpl         @w
            rts

; ---- A chunk's run of squares (run_p its squares, run_q their floors) into a row of the grid's (run_g, two bytes
; a square), or the marks back from it into the chunk

            .export     _run_in, _run_out, _run_clear, _run_p, _run_q, _run_g
            .importzp   ptr3

            .bss
_run_p:     .res        2
_run_q:     .res        2
_run_g:     .res        2
cnt:        .res        1
doors:      .res        1

            .rodata
kind:       .byte       G_OPAQUE | G_SKIP, G_INSIDE, G_OPAQUE | G_WALL, G_WALL, 0, G_INSIDE, G_WALL, G_INSIDE
            .byte       G_INSIDE, G_INSIDE                      ; (world.h's T_ROCK ... T_PIT; the rest 0)

            .code

; unsigned char __fastcall__ run_in (unsigned char n): its squares' flags (what each is: kind) and floors; 1 if a
; door or a window is among them
_run_in:
            sta         cnt
            jsr         ptrs
            lda         #0
            sta         doors
@sq:        ldy         #0
            lda         (ptr1),y
            and         #$1F
            tax
            cpx         #3                  ; (T_DOOR, T_WINDOW)
            beq         @door
            cpx         #6
            bne         @kind
@door:      inc         doors
@kind:      lda         #0
            cpx         #10
            bcs         @put
            lda         kind,x
@put:       sta         (ptr3),y
            lda         (ptr2),y
            iny
            sta         (ptr3),y
            inc         ptr1
            bne         @q
            inc         ptr1 + 1
@q:         inc         ptr2
            bne         @g
            inc         ptr2 + 1
@g:         lda         ptr3
            clc
            adc         #2
            sta         ptr3
            bcc         @n
            inc         ptr3 + 1
@n:         dec         cnt
            bne         @sq
            lda         doors
            ldx         #0
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
