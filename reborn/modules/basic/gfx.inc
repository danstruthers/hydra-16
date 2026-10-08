; ****************************************************************************
; gfx.inc - graphics (the eighth bank): QuickBASIC's statements on the Vera X's bitmap, through vid's /dev/vid/draw
; (its text commands: the driver draws, the console's text stays over it; docs/programming/video.md): SCREEN, PSET,
; PRESET, LINE, CIRCLE, PAINT (a scanline fill, the bitmap's rows read from /dev/vid/vram), DRAW (QuickBASIC's turtle
; language), GPRINT, PALETTE, POINT, SPRITE; COLOR's pen and CLS's clear in a SCREEN.  Coordinates are pixels,
; rounded; the last point referenced (LINE's, PSET's ...) is where STEP and LINE -(x, y) start.

SPR_VRAM        = $13000        ; Sprites' images: n * 256 on (16 x 16 at 8 bits a pixel at most), past a 320 x 240 x 8
GSEEDS          = 60            ; PAINT's seeds at most

.bss
g_on:       .res        1                                   ; <> 0: a bitmap (SCREEN 1 on)
g_win:      .res        1                                   ; WINDOW: 0 none, 1 (y up), 2 WINDOW SCREEN
g_view:     .res        1                                   ; VIEW: 0 none, 1 (its corner the origin), 2 VIEW SCREEN
gv_x1:      .res        2                                   ; The viewport (the bitmap's, or VIEW's)
gv_y1:      .res        2
gv_x2:      .res        2
gv_y2:      .res        2
g_dfd:      .res        1                                   ; /dev/vid/draw's fd + 1 (0: not open yet)
g_vfd:      .res        1                                   ; /dev/vid/vram's
g_pen:      .res        1                                   ; COLOR's: the pen's colour
g_cur:      .res        1                                   ; The driver's pen as BASIC last set it ($FF: not known)
g_w:        .res        2                                   ; The bitmap: its width, height, bits a pixel, a row's bytes
g_h:        .res        2
g_dep:      .res        1
g_row:      .res        2
g_lx:       .res        2                                   ; The last point referenced
g_ly:       .res        2
g_x:        .res        2                                   ; (A command's points, radius, colour, kind)
g_y:        .res        2
g_x2:       .res        2
g_y2:       .res        2
g_r:        .res        2
g_c:        .res        1
g_k:        .res        1
g_buf:      .res        64                                  ; (A command, being made)
g_n:        .res        1
g_bd:       .res        1                                   ; (PAINT's border, its seeds)
g_sp:       .res        1
g_seed:     .res        GSEEDS * 4
g_xl:       .res        2
g_xr:       .res        2
g_t:        .res        4                                   ; (Scratch)
dr_b:       .res        1                                   ; (DRAW's: B, N, its scale, its angle, its place in the
dr_nr:      .res        1                                   ;   string, the string's length, a number read)
dr_s:       .res        1
dr_a:       .res        2
dr_i:       .res        1
dr_len:     .res        1
dr_v:       .res        2
dr_neg:     .res        1

.segment "CODE8"

; ---- Commands to the driver

; g_buf begun with the word at .A/.X (this bank's, zero-terminated)
g_word:
            sta         t4
            stx         t4 + 1
            ldy         #0
:
            lda         (t4),y
            beq         :+
            sta         g_buf,y
            iny
            bra         :-
:
            sty         g_n
            rts

; A space, then the integer .A/.X (signed), in decimal, put after it
g_num:
            sta         g_t
            stx         g_t + 1
            ldy         g_n
            lda         #' '
            sta         g_buf,y
            iny
            lda         g_t + 1
            bpl         :+
            lda         #'-'
            sta         g_buf,y
            iny
            sec
            lda         #0
            sbc         g_t
            sta         g_t
            lda         #0
            sbc         g_t + 1
            sta         g_t + 1
:
            sty         g_n
            lda         #0                                  ; (Its digits, least first, on the stack)
            pha
@digit:
            ldx         #16
            lda         #0
:
            asl         g_t
            rol         g_t + 1
            rol         a
            cmp         #10
            bcc         :+
            sbc         #10
            inc         g_t
:
            dex
            bne         :--
            ora         #'0'
            pha
            lda         g_t
            ora         g_t + 1
            bne         @digit
            ldy         g_n
:
            pla
            beq         :+
            sta         g_buf,y
            iny
            bra         :-
:
            sty         g_n
            rts

; The command in g_buf written to /dev/vid/draw (opened the first time).  Its failure: the system's error (no bitmap:
; E_INVAL)
g_send:
            lda         g_dfd
            bne         :+
            LDR         r0, s_draw
            jsr         ram_path
            lda         #O_WRITE
            jsr         OPEN
            bcs         g_err
            inc         a
            sta         g_dfd
:
            LDR         r0, g_buf
            lda         g_n
            sta         r1
            stz         r1 + 1
            lda         g_dfd
            dec         a
            jsr         WRITE
            bcs         g_err
            rts
g_err:
            FARN        7, sys_err

; A command: the word at .A/.X and the points g_x, g_y (and g_x2, g_y2: g_k 4), written
g_cmd2:
            jsr         g_word
            lda         g_x
            ldx         g_x + 1
            jsr         g_num
            lda         g_y
            ldx         g_y + 1
            jsr         g_num
            rts
g_cmd4:
            jsr         g_cmd2
            lda         g_x2
            ldx         g_x2 + 1
            jsr         g_num
            lda         g_y2
            ldx         g_y2 + 1
            jsr         g_num
            jmp         g_send

; The driver's pen made .A (a pen command only if it isn't that already)
g_use:
            cmp         g_cur
            beq         @done
            sta         g_cur
            ldx         #0
            pha
            LDAX        s_pen
            jsr         g_word
            pla
            ldx         #0
            jsr         g_num
            jmp         g_send
@done:
            rts

; The colour taken off the stack (below 0: .A, the default) made the driver's pen
g_colour:
            pha
            jsr         pop_int16
            pla
            ldx         t0 + 1
            bmi         :+
            bne         g_bad
            lda         t0
:
            sta         g_c
            jmp         g_use

g_bad:
            lda         #ER_FUNC
            ldx         #0
            jmp         b_error

; g_x, g_y = the point taken off the stack (y, then x), in pixels: rounded to integers; VIEW's corner added; with
; a WINDOW, its coordinates on the viewport, exactly, then rounded (G_PLX, G_PLY the point as the program had it)
g_point:
            lda         g_win
            bne         @win
            jsr         pop_int16
            MOVW        g_y, t0
            jsr         pop_int16
            MOVW        g_x, t0
            lda         g_view
            cmp         #1
            bne         @done
            clc
            lda         g_x
            adc         gv_x1
            sta         g_x
            lda         g_x + 1
            adc         gv_x1 + 1
            sta         g_x + 1
            clc
            lda         g_y
            adc         gv_y1
            sta         g_y
            lda         g_y + 1
            adc         gv_y1 + 1
            sta         g_y + 1
@done:
            rts
@win:
            lda         #1                                  ; (y: (y - y1) * sy, from the viewport's bottom up, or
            ldx         #G_PLY                              ;   (SCREEN) its top down)
            jsr         g_depthslot
            lda         #G_WY1
            jsr         g_pushslot
            lda         #OP_SUB
            FARN        4, num_op
            lda         #G_SY
            jsr         g_pushslot
            lda         #OP_MUL
            FARN        4, num_op
            lda         g_win
            cmp         #2
            beq         @scr
            lda         #OP_NEG
            FARN        4, num_op
            lda         gv_y2
            ldx         gv_y2 + 1
            bra         @addy
@scr:
            lda         gv_y1
            ldx         gv_y1 + 1
@addy:
            jsr         g_pushint
            lda         #OP_ADD
            FARN        4, num_op
            jsr         pop_int16
            MOVW        g_y, t0
            lda         #1                                  ; (x: (x - x1) * sx from the viewport's left)
            ldx         #G_PLX
            jsr         g_depthslot
            lda         #G_WX1
            jsr         g_pushslot
            lda         #OP_SUB
            FARN        4, num_op
            lda         #G_SX
            jsr         g_pushslot
            lda         #OP_MUL
            FARN        4, num_op
            lda         gv_x1
            ldx         gv_x1 + 1
            jsr         g_pushint
            lda         #OP_ADD
            FARN        4, num_op
            jsr         pop_int16
            MOVW        g_x, t0
            rts

; The last point referenced = g_x, g_y (and, with a WINDOW, G_PLX, G_PLY: G_LLX, G_LLY)
g_last:
            MOVW        g_lx, g_x
            MOVW        g_ly, g_y
            lda         g_win
            beq         @done
            lda         #G_PLX
            jsr         g_slot
            MOVW        vu, vt
            lda         #G_LLX
            jsr         g_slot
            ldy         #2 * VAL_SIZE - 1                   ; (Both: PLX, PLY to LLX, LLY)
:
            lda         (vu),y
            sta         (vt),y
            dey
            bpl         :-
@done:
            rts

; vt = the reserved global .A's place (gbase + .A * 5)
g_slot:
            sta         g_k
            asl         a
            asl         a
            clc
            adc         g_k
            clc
            adc         gbase
            sta         vt
            lda         gbase + 1
            adc         #0
            sta         vt + 1
            rts

; The reserved global .A's value pushed
g_pushslot:
            jsr         g_slot
            ldy         #VAL_SIZE - 1
:
            lda         (vt),y
            sta         (vsp),y
            dey
            bpl         :-
            jmp         vsp_up

; The value .A below the top (1: the top) copied to the reserved global .X
g_depthslot:
            pha
            txa
            jsr         g_slot
            MOVW        vu, vt
            pla
            jsr         g_depth
            ldy         #VAL_SIZE - 1
:
            lda         (vt),y
            sta         (vu),y
            dey
            bpl         :-
            rts

; vt = the value .A below the top (1: the top)
g_depth:
            tax
            MOVW        vt, vsp
:
            sec
            lda         vt
            sbc         #VAL_SIZE
            sta         vt
            bcs         :+
            dec         vt + 1
:
            dex
            bne         :--
            rts

; The value .A below the top pushed (a copy)
g_pushdepth:
            jsr         g_depth
            ldy         #VAL_SIZE - 1
:
            lda         (vt),y
            sta         (vsp),y
            dey
            bpl         :-
            jmp         vsp_up

; .A/.X pushed (signed: 16 bits)
g_pushint:
            FARN        6, push_ax
            txa
            bpl         :+
            jsr         top_vt
            ldy         #3
            lda         #$FF
            sta         (vt),y
            iny
            sta         (vt),y
:
            rts

; ---- WINDOW, VIEW

; ST_WINDOW: (x1, y1)-(x2, y2), its kind (taken off first: 0 none, and no point; 1 y up, from y1 at the viewport's
; bottom; 2 SCREEN, y down): the program's coordinates on the viewport, x1 at its left, x2 its right ...
st_window:
            jsr         pop_int16
            lda         t0
            sta         g_k
            bne         :+
            stz         g_win
            rts
:
            pha
            lda         #4                                  ; (x1, y1)
            ldx         #G_WX1
            jsr         g_depthslot
            lda         #3
            ldx         #G_WY1
            jsr         g_depthslot
            sec                                             ; (sx: the viewport's width less 1 / (x2 - x1))
            lda         gv_x2
            sbc         gv_x1
            tay
            lda         gv_x2 + 1
            sbc         gv_x1 + 1
            tax
            tya
            jsr         g_pushint
            lda         #3
            jsr         g_pushdepth
            lda         #6
            jsr         g_pushdepth
            lda         #OP_SUB
            FARN        4, num_op
            lda         #OP_DIV
            FARN        4, num_op
            lda         #1
            ldx         #G_SX
            jsr         g_depthslot
            jsr         pop_vt
            sec                                             ; (sy: its height less 1 / (y2 - y1))
            lda         gv_y2
            sbc         gv_y1
            tay
            lda         gv_y2 + 1
            sbc         gv_y1 + 1
            tax
            tya
            jsr         g_pushint
            lda         #2
            jsr         g_pushdepth
            lda         #5
            jsr         g_pushdepth
            lda         #OP_SUB
            FARN        4, num_op
            lda         #OP_DIV
            FARN        4, num_op
            lda         #1
            ldx         #G_SY
            jsr         g_depthslot
            sec                                             ; (Its five values off)
            lda         vsp
            sbc         #5 * VAL_SIZE
            sta         vsp
            bcs         :+
            dec         vsp + 1
:
            pla
            sta         g_win
            rts

; ST_VIEW: (x1, y1)-(x2, y2), its kind (taken off first: 0 none, the whole bitmap; 1 its corner the origin; 2 SCREEN,
; the bitmap's coordinates): the viewport (a WINDOW's after it on it)
st_view:
            jsr         pop_int16
            lda         t0
            sta         g_view
            bne         :+
            jmp         g_vall
:
            jsr         pop_int16
            MOVW        gv_y2, t0
            jsr         pop_int16
            MOVW        gv_x2, t0
            jsr         pop_int16
            MOVW        gv_y1, t0
            jsr         pop_int16
            MOVW        gv_x1, t0
            rts

; The viewport the whole bitmap
g_vall:
            stz         g_view
            stz         gv_x1
            stz         gv_x1 + 1
            stz         gv_y1
            stz         gv_y1 + 1
            sec
            lda         g_w
            sbc         #1
            sta         gv_x2
            lda         g_w + 1
            sbc         #0
            sta         gv_x2 + 1
            sec
            lda         g_h
            sbc         #1
            sta         gv_y2
            lda         g_h + 1
            sbc         #0
            sta         gv_y2 + 1
            rts

; A bitmap, or Illegal function call
g_need:
            lda         g_on
            bne         @lb909
            jmp         g_bad
@lb909:
            rts

s_draw:     .byte       "/dev/vid/draw", 0
s_pen:      .byte       "pen", 0
s_plot:     .byte       "plot", 0
s_gline:     .byte       "line", 0
s_box:      .byte       "box", 0
s_bar:      .byte       "bar", 0
s_circle:   .byte       "circle", 0
s_disc:     .byte       "disc", 0
s_text:     .byte       "text", 0
s_clear:    .byte       "clear", 0
s_bitmap:   .byte       "bitmap", 0
s_bmoff:    .byte       "bitmap off", 0

; ---- SCREEN

; ST_SCREEN: n taken off: 0 the text alone (no bitmap); 1 320 x 240 x 2 bits; 2 640 x 480 x 1; 7 320 x 240 x 4;
; 12 640 x 480 x 2; 13 320 x 240 x 8.  The bitmap cleared, the pen its brightest (15 at most), the last point its
; middle
st_screen:
            jsr         pop_int16
            lda         t0 + 1
            beq         @lb908
            jmp         g_bad
@lb908:
            lda         t0
            ldx         #5
:
            cmp         scr_n,x
            beq         :+
            dex
            bpl         :-
            jmp         g_bad
:
            cpx         #0
            bne         @on
            stz         g_on                                ; (Text alone)
            LDAX        s_bmoff
            jsr         g_word
            jmp         g_ctl
@on:
            lda         #1
            sta         g_on
            lda         scr_w,x                             ; (320 or 640: its width, height, depth)
            beq         :+
            LDW         g_w, 640
            LDW         g_h, 480
            bra         :++
:
            LDW         g_w, 320
            LDW         g_h, 240
:
            lda         scr_d,x
            sta         g_dep
            LDAX        s_bitmap
            jsr         g_word
            lda         g_w
            ldx         g_w + 1
            jsr         g_num
            lda         g_dep
            ldx         #0
            jsr         g_num
            jsr         g_ctl
            lda         g_w                                 ; (A row's bytes: width * depth / 8)
            sta         g_row
            lda         g_w + 1
            sta         g_row + 1
            lda         g_dep
:
            lsr         a
            bcs         :+
            asl         g_row
            rol         g_row + 1
            bra         :-
:
            lsr         g_row + 1
            ror         g_row
            lsr         g_row + 1
            ror         g_row
            lsr         g_row + 1
            ror         g_row
            stz         g_win                               ; (No WINDOW, no VIEW: the viewport the bitmap)
            jsr         g_vall
            lda         #$FF                                ; (The pen: the depth's brightest, 15 at most)
            sta         g_cur
            ldx         g_dep
            lda         scr_pen - 1,x
            sta         g_pen
            jsr         g_use
            LDAX        s_clear
            jsr         g_word
            jsr         g_send
            lda         g_w + 1                             ; (The last point: the middle)
            lsr         a
            sta         g_lx + 1
            lda         g_w
            ror         a
            sta         g_lx
            lda         g_h + 1
            lsr         a
            sta         g_ly + 1
            lda         g_h
            ror         a
            sta         g_ly
            rts
scr_n:      .byte       0, 1, 2, 7, 12, 13
scr_w:      .byte       0, 0, 1, 0, 1, 0                    ; (1: 640 across)
scr_d:      .byte       0, 2, 1, 4, 2, 8
scr_pen:    .byte       1, 3, 3, 15, 15, 15, 15, 15         ; (By the depth: 1 a pixel, 2, -, 4, -, -, -, 8)

; The command in g_buf written to /dev/vid/ctl (opened and closed)
g_ctl:
            LDR         r0, s_ctl
            jsr         ram_path
            lda         #O_WRITE
            jsr         OPEN
            bcc         @lb907
            jmp         g_err
@lb907:
            pha
            LDR         r0, g_buf
            lda         g_n
            sta         r1
            stz         r1 + 1
            pla
            pha
            jsr         WRITE
            pla
            jmp         CLOSE
s_ctl:      .byte       "/dev/vid/ctl", 0

; ---- Points, lines, circles

; ST_PSET: (x, y), c (below 0: the pen's); ST_PRESET (below 0: 0, the background's)
st_pset:
            jsr         g_need
            lda         g_pen
            bra         :+
st_preset:
            jsr         g_need
            lda         #0
:
            jsr         g_colour
            jsr         g_point
            jsr         g_last
            LDAX        s_plot
            jsr         g_cmd2
            jmp         g_send

; ST_GMOVE: the last point the point taken off (LINE's first); ST_GLX, ST_GLY: its x, its y pushed (STEP's)
st_gmove:
            jsr         g_point
            jmp         g_last
st_glx:
            lda         g_win
            beq         :+
            lda         #G_LLX
            jmp         g_pushslot
:
            lda         g_view
            cmp         #1
            bne         :+
            sec
            lda         g_lx
            sbc         gv_x1
            tay
            lda         g_lx + 1
            sbc         gv_x1 + 1
            tax
            tya
            bra         @push
:
            lda         g_lx
            ldx         g_lx + 1
@push:
            jmp         g_pushint
st_gly:
            lda         g_win
            beq         :+
            lda         #G_LLY
            jmp         g_pushslot
:
            lda         g_view
            cmp         #1
            bne         :+
            sec
            lda         g_ly
            sbc         gv_y1
            tay
            lda         g_ly + 1
            sbc         gv_y1 + 1
            tax
            tya
            bra         @push
:
            lda         g_ly
            ldx         g_ly + 1
@push:
            jmp         g_pushint

; ST_LINE: (x1, y1), (x2, y2), c, its kind (0 a line, 1 B a box, 2 BF a filled one)
st_line:
            jsr         g_need
            jsr         pop_int16
            lda         t0
            sta         g_k
            lda         g_pen
            jsr         g_colour
            jsr         g_point                             ; (The second: the last point)
            jsr         g_last
            MOVW        g_x2, g_x
            MOVW        g_y2, g_y
            jsr         g_point
            LDAX        s_gline
            ldy         g_k
            beq         :+
            LDAX        s_box
            dey
            beq         :+
            LDAX        s_bar
:
            jmp         g_cmd4

; ST_CIRCLE: (x, y), r, c, fill (<> 0: a disc)
st_circle:
            jsr         g_need
            jsr         pop_int16
            lda         t0
            sta         g_k
            lda         g_pen
            jsr         g_colour
            lda         g_win                               ; (Its radius: with a WINDOW, x's scale)
            beq         :+
            lda         #G_SX
            jsr         g_pushslot
            lda         #OP_MUL
            FARN        4, num_op
            FARN        4, f_abs
:
            jsr         pop_int16
            MOVW        g_r, t0
            jsr         g_point
            jsr         g_last
            LDAX        s_circle
            ldy         g_k
            beq         :+
            LDAX        s_disc
:
            jsr         g_cmd2
            lda         g_r
            ldx         g_r + 1
            jsr         g_num
            jmp         g_send

; ST_GPRINT: (x, y), text$: the text on the bitmap in the console's font (its first 40 characters), the pen's colour
st_gprint:
            jsr         g_need
            FARN        7, str_lbuf
            cmp         #40
            bcc         :+
            lda         #40
:
            sta         g_k
            lda         g_pen
            jsr         g_use
            jsr         g_point
            jsr         g_last
            LDAX        s_text
            jsr         g_cmd2
            ldy         g_n
            lda         #' '
            sta         g_buf,y
            iny
            ldx         #0
:
            cpx         g_k
            beq         :+
            lda         lbuf,x
            sta         g_buf,y
            inx
            iny
            bra         :-
:
            sty         g_n
            jmp         g_send

; COLOR in a SCREEN (io.inc's): fg (taken off under bg) the pen's colour; bg dropped
g_color:
            jsr         pop_int16                           ; (The background: no matter)
            jsr         pop_int16
            lda         t0 + 1
            bmi         :+
            bne         g_bad2
            lda         t0
            sta         g_pen
            jmp         g_use
:
            rts
g_bad2:
            jmp         g_bad

; CLS in a SCREEN (io.inc's): the bitmap cleared too, the last point its middle
g_cls:
            LDAX        s_clear
            jsr         g_word
            jmp         g_send

; ---- PALETTE, POINT

; ST_PALETTE: n, rgb ($RGB, 4 bits each): the palette's entry n ($GB, then $0R)
st_palette:
            jsr         pop_int16
            lda         t0
            sta         g_t
            lda         t0 + 1
            and         #$0F
            sta         g_t + 1
            jsr         pop_int16
            lda         t0 + 1
            beq         :+
            jmp         g_bad
:
            lda         t0                                  ; (Its place: n * 2)
            asl         a
            sta         r0
            lda         #0
            rol         a
            sta         r0 + 1
            lda         #<s_pal
            ldx         #>s_pal
            jmp         g_put2
s_pal:      .byte       "/dev/vid/pal", 0

; The 2 bytes g_t into the file at .A/.X (this bank's name), at the place r0-r1: opened, put, closed
g_put2:
            pha
            lda         r0                                  ; (The place, kept over ram_path)
            sta         g_x
            lda         r0 + 1
            sta         g_x + 1
            pla
            sta         r0
            stx         r0 + 1
            jsr         ram_path
            lda         #O_RDWR
            jsr         OPEN
            bcc         :+
            jmp         g_err
:
            sta         g_k
            lda         g_x
            sta         r0
            lda         g_x + 1
            sta         r0 + 1
            stz         r1
            stz         r1 + 1
            lda         g_k
            ldx         #0
            jsr         SEEK
            LDR         r0, g_t
            LDR         r1, 2
            lda         g_k
            jsr         WRITE
            lda         g_k
            jmp         CLOSE

; /dev/vid/vram open (g_vfd), at the VRAM address g_t-g_t+2 (17 bits)
g_vram:
            lda         g_vfd
            bne         :+
            LDR         r0, s_vram
            jsr         ram_path
            lda         #O_RDWR
            jsr         OPEN
            bcc         @have
            jmp         g_err
@have:
            inc         a
            sta         g_vfd
:
            lda         g_t
            sta         r0
            lda         g_t + 1
            sta         r0 + 1
            lda         g_t + 2
            sta         r1
            stz         r1 + 1
            lda         g_vfd
            dec         a
            ldx         #0
            jmp         SEEK
s_vram:     .byte       "/dev/vid/vram", 0

; g_t = the VRAM address of the row y (g_y): y * the row's bytes
g_rowaddr:
            stz         g_t
            stz         g_t + 1
            stz         g_t + 2
            MOVW        g_x2, g_y                           ; (y's bits, from the lowest: the row added once each)
            MOVW        g_y2, g_row
            stz         g_k
@bit:
            lsr         g_x2 + 1
            ror         g_x2
            bcc         :+
            clc
            lda         g_t
            adc         g_y2
            sta         g_t
            lda         g_t + 1
            adc         g_y2 + 1
            sta         g_t + 1
            lda         g_t + 2
            adc         g_k
            sta         g_t + 2
:
            asl         g_y2
            rol         g_y2 + 1
            rol         g_k
            lda         g_x2
            ora         g_x2 + 1
            bne         @bit
            rts

; The row y (g_y) read into lbuf (its bytes: g_row)
g_readrow:
            jsr         g_rowaddr
            jsr         g_vram
            LDR         r0, lbuf
            MOVW        r1, g_row
            lda         g_vfd
            dec         a
            jsr         READ
            bcc         :+
            jmp         g_err
:
            rts

; .A = the pixel x (.A/.X: 0 to the width less 1) of the row in lbuf (its byte x >> log2 of the pixels a byte; in
; it the leftmost pixel highest)
g_pix:
            sta         g_t + 3
            ldy         g_dep
            cpy         #8
            bne         @packed
            clc
            adc         #<lbuf
            sta         t4
            txa
            adc         #>lbuf
            sta         t4 + 1
            lda         (t4)
            rts
@packed:
            sta         t4
            stx         t4 + 1
            ldx         gp_pl - 1,y
:
            lsr         t4 + 1
            ror         t4
            dex
            bne         :-
            clc
            lda         t4
            adc         #<lbuf
            sta         t4
            lda         t4 + 1
            adc         #>lbuf
            sta         t4 + 1
            lda         (t4)
            sta         g_t + 1
            ldy         g_dep                               ; (Its place in the byte: from the right, (ppb - 1) - its
            lda         gp_ppb - 1,y                        ;   index, depth bits each)
            and         g_t + 3
            eor         gp_ppb - 1,y
            tax
            lda         g_t + 1
@shift:
            cpx         #0
            beq         @mask
            ldy         g_dep
:
            lsr         a
            dey
            bne         :-
            dex
            bra         @shift
@mask:
            ldy         g_dep
            and         gp_mask - 1,y
            rts
gp_pl:      .byte       3, 2, 0, 1                          ; (By the depth, 1, 2, 4: log2 of the pixels a byte ...
gp_ppb:     .byte       7, 3, 0, 1                          ;   the pixels a byte less 1 ...
gp_mask:    .byte       1, 3, 0, 15                         ;   a pixel's bits)

; POINT(x, y) (sys.inc's F_POINT): the pixel's colour; off the bitmap (or none): -1
g_point_f:
            jsr         g_point
            lda         g_on
            beq         @off
            jsr         g_onmap
            bcs         @off
            jsr         g_readrow
            lda         g_x
            ldx         g_x + 1
            jsr         g_pix
            ldx         #0
            FARN        6, push_ax
            rts
@off:
            lda         #$FF
            FARN        6, push_int_a
            rts

; Is (g_x, g_y) on the bitmap?  OUT: C = 0 yes
g_onmap:
            lda         g_x + 1
            bmi         @no
            lda         g_y + 1
            bmi         @no
            lda         g_x
            cmp         g_w
            lda         g_x + 1
            sbc         g_w + 1
            bcs         @no
            lda         g_y
            cmp         g_h
            lda         g_y + 1
            sbc         g_h + 1
            bcs         @no
            clc
            rts
@no:
            sec
            rts

; ---- PAINT

; ST_PAINT: (x, y), c (below 0: the pen's), border (below 0: c): from the point, the pixels that aren't the border's
; colour (nor c's) filled with c, row by row (each row read from VRAM, a line drawn across each run)
st_paint:
            jsr         g_need
            jsr         pop_int16                           ; (The border)
            lda         t0
            ldx         t0 + 1
            stx         g_bd
            pha
            lda         g_pen
            jsr         g_colour
            pla
            ldx         g_bd
            bpl         :+
            lda         g_c
:
            sta         g_bd
            jsr         g_point
            jsr         g_last
            jsr         g_onmap
            bcc         :+
            rts
:
            stz         g_sp
            jsr         g_push
@seed:
            lda         g_sp                                ; (A seed taken)
            bne         :+
            rts
:
            sec
            sbc         #4
            sta         g_sp
            tax
            lda         g_seed,x
            sta         g_x
            lda         g_seed + 1,x
            sta         g_x + 1
            lda         g_seed + 2,x
            sta         g_y
            lda         g_seed + 3,x
            sta         g_y + 1
            jsr         g_readrow
            lda         g_x
            ldx         g_x + 1
            jsr         g_stop
            bcs         @seed
            MOVW        g_xl, g_x                           ; (Its run: left ...
@left:
            lda         g_xl
            ora         g_xl + 1
            beq         @right0
            lda         g_xl
            sec
            sbc         #1
            tay
            lda         g_xl + 1
            sbc         #0
            tax
            tya
            jsr         g_stop
            bcs         @right0
            lda         g_xl
            bne         :+
            dec         g_xl + 1
:
            dec         g_xl
            bra         @left
@right0:
            MOVW        g_xr, g_x                           ;   ... and right)
@right:
            clc
            lda         g_xr
            adc         #1
            sta         g_x2
            lda         g_xr + 1
            adc         #0
            sta         g_x2 + 1
            lda         g_x2
            cmp         g_w
            lda         g_x2 + 1
            sbc         g_w + 1
            bcs         @draw
            lda         g_x2
            ldx         g_x2 + 1
            jsr         g_stop
            bcs         @draw
            MOVW        g_xr, g_x2
            bra         @right
@draw:
            MOVW        g_x, g_xl                           ; (Its line)
            MOVW        g_x2, g_xr
            MOVW        g_y2, g_y
            LDAX        s_gline
            jsr         g_cmd4
            lda         g_y                                 ; (The rows above and below: their runs' seeds)
            ora         g_y + 1
            beq         @below
            jsr         g_ydec
            jsr         g_runs
            jsr         g_yinc
@below:
            jsr         g_yinc
            lda         g_y
            cmp         g_h
            lda         g_y + 1
            sbc         g_h + 1
            bcs         :+
            jsr         g_runs
:
            jmp         @seed

; g_y less 1, more 1
g_ydec:
            lda         g_y
            bne         :+
            dec         g_y + 1
:
            dec         g_y
            rts
g_yinc:
            inc         g_y
            bne         :+
            inc         g_y + 1
:
            rts

; The row g_y's runs of pixels to fill between g_xl and g_xr: a seed for each one's first
g_runs:
            jsr         g_readrow
            MOVW        g_x, g_xl
            stz         g_k                                 ; (In a run: g_k <> 0)
@px:
            lda         g_x
            ldx         g_x + 1
            jsr         g_stop
            bcs         @stop
            lda         g_k
            bne         @next
            inc         g_k
            jsr         g_push
            bra         @next
@stop:
            stz         g_k
@next:
            lda         g_x
            cmp         g_xr
            bne         :+
            lda         g_x + 1
            cmp         g_xr + 1
            beq         @done
:
            inc         g_x
            bne         @px
            inc         g_x + 1
            bra         @px
@done:
            rts

; A seed: (g_x, g_y) (past GSEEDS: dropped)
g_push:
            ldx         g_sp
            cpx         #GSEEDS * 4
            bcs         :+
            lda         g_x
            sta         g_seed,x
            lda         g_x + 1
            sta         g_seed + 1,x
            lda         g_y
            sta         g_seed + 2,x
            lda         g_y + 1
            sta         g_seed + 3,x
            txa
            clc
            adc         #4
            sta         g_sp
:
            rts

; Does the pixel .A/.X (of the row in lbuf) stop a fill: the border's colour, or the fill's?  OUT: C = 1 yes
g_stop:
            jsr         g_pix
            cmp         g_bd
            beq         :+
            cmp         g_c
            beq         :+
            clc
            rts
:
            sec
            rts

; ---- DRAW

; ST_DRAW: the string taken off, QuickBASIC's turtle language from the last point: U D L R E F G H [n] (up ... up and
; left; n units, 1 if none), M x,y (to a point; +x or -x: by so much), B before one (not drawn), N (back after), C n
; (the colour), S n (n / 4 pixels a unit; 4 at the start), A n (n * 90 degrees, anticlockwise), TA n (n degrees)
st_draw:
            jsr         g_need
            FARN        7, str_lbuf
            sta         dr_len
            stz         dr_i
            lda         dr_s
            bne         @cmd
            lda         #4
            sta         dr_s
@cmd:
            stz         dr_b
            stz         dr_nr
@next:
            jsr         dr_ch
            bcs         @done
            cmp         #'B'
            bne         :+
            inc         dr_b
            bra         @next
:
            cmp         #'N'
            bne         :+
            inc         dr_nr
            bra         @next
:
            ldx         #7                                  ; (A move: its direction)
:
            cmp         dr_dirs,x
            beq         @move
            dex
            bpl         :-
            cmp         #'M'
            beq         @m
            cmp         #'C'
            bne         @lb906
            jmp         @c
@lb906:
            cmp         #'S'
            bne         @lb905
            jmp         @s
@lb905:
            cmp         #'A'
            bne         @lb904
            jmp         @a
@lb904:
            cmp         #'T'
            bne         @lb903
            jmp         @t
@lb903:
            cmp         #' '
            beq         @next
            cmp         #';'
            beq         @next
            jmp         g_bad
@done:
            rts
@move:
            phx
            jsr         dr_num                              ; (Its units: 1 if none)
            bcc         :+
            LDW         dr_v, 1
:
            plx
            lda         dr_dx,x                             ; (Its step, scaled: units * S / 4)
            jsr         dr_scaled
            MOVW        g_x2, g_t
            lda         dr_dy,x
            jsr         dr_scaled
            MOVW        g_y2, g_t
            jsr         dr_turn
            jsr         dr_go
            jmp         @cmd
@m:
            jsr         dr_rel                              ; (x: +n, -n relative)
            pha
            jsr         dr_num
            bcs         @bad
            MOVW        g_x2, dr_v
            jsr         dr_ch
            cmp         #','
            bne         @bad
            jsr         dr_rel
            jsr         dr_num
            bcs         @bad
            MOVW        g_y2, dr_v
            pla
            beq         :+
            jsr         dr_go                               ; (By so much)
            jmp         @cmd
:
            sec                                             ; (To the point: by its difference)
            lda         g_x2
            sbc         g_lx
            sta         g_x2
            lda         g_x2 + 1
            sbc         g_lx + 1
            sta         g_x2 + 1
            sec
            lda         g_y2
            sbc         g_ly
            sta         g_y2
            lda         g_y2 + 1
            sbc         g_ly + 1
            sta         g_y2 + 1
            jsr         dr_go
            jmp         @cmd
@bad:
            jmp         g_bad
@c:
            jsr         dr_num
            bcs         @bad
            lda         dr_v
            sta         g_pen
            jsr         g_use
            jmp         @next
@s:
            jsr         dr_num
            bcs         @bad
            lda         dr_v
            sta         dr_s
            jmp         @next
@a:
            jsr         dr_num
            bcs         @bad
            lda         dr_v                                ; (n * 90)
            and         #3
            tax
            lda         dr_a90,x
            sta         dr_a
            lda         dr_a90h,x
            sta         dr_a + 1
            jmp         @next
@t:
            jsr         dr_ch
            cmp         #'A'
            bne         @bad
            jsr         dr_num
            bcs         @bad
            MOVW        dr_a, dr_v
            jmp         @next
dr_dirs:    .byte       "UDLREFGH"
dr_dx:      .byte       0, 0, $FF, 1, 1, 1, $FF, $FF
dr_dy:      .byte       $FF, 1, 0, 0, $FF, 1, 1, $FF
dr_a90:     .byte       <0, <90, <180, <270
dr_a90h:    .byte       >0, >90, >180, >270

; The string's next character (upper case), past spaces.  OUT: C = 1 at its end
dr_ch:
            ldx         dr_i
            cpx         dr_len
            bcs         @end
            lda         lbuf,x
            inc         dr_i
            cmp         #'a'
            bcc         :+
            cmp         #'z' + 1
            bcs         :+
            and         #$DF
:
            clc
            rts
@end:
            sec
            rts

; A + or - next (M's relative x and y): .A <> 0 if so (dr_neg its sign); Z flag from .A
dr_rel:
            stz         dr_neg
            ldx         dr_i
            lda         lbuf,x
            cmp         #'+'
            beq         :+
            cmp         #'-'
            bne         @no
            inc         dr_neg
:
            inc         dr_i
            lda         #1
            rts
@no:
            lda         #0
            rts

; dr_v = the number next (decimal, a - first allowed; dr_neg's too).  OUT: C = 1 none
dr_num:
            stz         dr_v
            stz         dr_v + 1
            ldx         dr_i
            lda         lbuf,x
            cmp         #'-'
            bne         :+
            lda         #1
            eor         dr_neg
            sta         dr_neg
            inx
            stx         dr_i
:
            ldy         #0
@dig:
            ldx         dr_i
            cpx         dr_len
            bcs         @end
            lda         lbuf,x
            sec
            sbc         #'0'
            cmp         #10
            bcs         @end
            pha
            lda         dr_v                                ; (* 10)
            ldx         dr_v + 1
            asl         dr_v
            rol         dr_v + 1
            asl         dr_v
            rol         dr_v + 1
            clc
            adc         dr_v
            sta         dr_v
            txa
            adc         dr_v + 1
            sta         dr_v + 1
            asl         dr_v
            rol         dr_v + 1
            pla
            clc
            adc         dr_v
            sta         dr_v
            bcc         :+
            inc         dr_v + 1
:
            inc         dr_i
            iny
            bra         @dig
@end:
            cpy         #0
            beq         @none
            lda         dr_neg
            beq         :+
            sec
            lda         #0
            sbc         dr_v
            sta         dr_v
            lda         #0
            sbc         dr_v + 1
            sta         dr_v + 1
:
            stz         dr_neg
            clc
            rts
@none:
            stz         dr_neg
            sec
            rts

; g_t = dr_v * .A ($FF -1, 0, 1) * S / 4.  Keeps .X
dr_scaled:
            phx
            stz         g_t
            stz         g_t + 1
            cmp         #0
            beq         @done
            pha
            ldy         dr_s                                ; (units * S)
:
            beq         :+
            clc
            lda         g_t
            adc         dr_v
            sta         g_t
            lda         g_t + 1
            adc         dr_v + 1
            sta         g_t + 1
            dey
            bra         :-
:
            lda         g_t + 1                             ; (/ 4, rounded toward 0)
            cmp         #$80
            ror         g_t + 1
            ror         g_t
            lda         g_t + 1
            cmp         #$80
            ror         g_t + 1
            ror         g_t
            pla
            bpl         @done
            sec
            lda         #0
            sbc         g_t
            sta         g_t
            lda         #0
            sbc         g_t + 1
            sta         g_t + 1
@done:
            plx
            rts

; The step (g_x2, g_y2) turned by the angle (dr_a degrees, anticlockwise): x cos + y sin, y cos - x sin
dr_turn:
            lda         dr_a
            ora         dr_a + 1
            bne         :+
            rts
:
            jsr         dr_cossin                           ; (g_xl: cos, g_xr: sin, * 10000)
            lda         g_x2                                ; (x cos + y sin)
            ldx         g_x2 + 1
            ldy         #0
            jsr         dr_mul                              ; (g_t: x * cos)
            MOVW        g_r, g_t
            lda         g_y2
            ldx         g_y2 + 1
            ldy         #1
            jsr         dr_mul                              ; (y * sin)
            clc
            lda         g_r
            adc         g_t
            pha
            lda         g_r + 1
            adc         g_t + 1
            pha
            lda         g_y2                                ; (y cos - x sin)
            ldx         g_y2 + 1
            ldy         #0
            jsr         dr_mul
            MOVW        g_r, g_t
            lda         g_x2
            ldx         g_x2 + 1
            ldy         #1
            jsr         dr_mul
            sec
            lda         g_r
            sbc         g_t
            sta         g_y2
            lda         g_r + 1
            sbc         g_t + 1
            sta         g_y2 + 1
            pla
            sta         g_x2 + 1
            pla
            sta         g_x2
            rts

; g_t = .A/.X (signed) * (Y 0: cos, 1: sin; g_xl, g_xr) / 10000, rounded
dr_mul:
            sta         g_k                                 ; (Its sign, its size)
            stx         dr_v
            lda         #0
            cpx         #$80
            bcc         :+
            sec
            lda         #0
            sbc         g_k
            sta         g_k
            lda         #0
            sbc         dr_v
            sta         dr_v
            lda         #1
:
            sta         g_n
            cpy         #0                                  ; (The factor: its sign too)
            bne         :+
            lda         g_xl
            ldx         g_xl + 1
            bra         :++
:
            lda         g_xr
            ldx         g_xr + 1
:
            cpx         #$80
            bcc         :+
            pha
            lda         g_n
            eor         #1
            sta         g_n
            pla
            eor         #$FF
            clc
            adc         #1
            pha
            txa
            eor         #$FF
            adc         #0
            tax
            pla
:
            sta         t0                                  ; (t0-t3 = |a| * |f|: 16 x 16)
            stx         t0 + 1
            stz         t0 + 2
            stz         t0 + 3
            lda         g_k
            sta         t4
            lda         dr_v
            sta         t4 + 1
            stz         g_t
            stz         g_t + 1
            stz         g_t + 2
            stz         g_t + 3
            ldx         #16
@bit:
            lsr         t4 + 1
            ror         t4
            bcc         :+
            clc
            lda         g_t
            adc         t0
            sta         g_t
            lda         g_t + 1
            adc         t0 + 1
            sta         g_t + 1
            lda         g_t + 2
            adc         t0 + 2
            sta         g_t + 2
            lda         g_t + 3
            adc         t0 + 3
            sta         g_t + 3
:
            asl         t0
            rol         t0 + 1
            rol         t0 + 2
            rol         t0 + 3
            dex
            bne         @bit
            clc                                             ; (+ 5000, / 10000)
            lda         g_t
            adc         #<5000
            sta         t0
            lda         g_t + 1
            adc         #>5000
            sta         t0 + 1
            lda         g_t + 2
            adc         #0
            sta         t0 + 2
            lda         g_t + 3
            adc         #0
            sta         t0 + 3
            lda         #<10000
            ldx         #>10000
            FARN        7, div32
            lda         t0
            sta         g_t
            lda         t0 + 1
            sta         g_t + 1
            lda         g_n
            beq         :+
            sec
            lda         #0
            sbc         g_t
            sta         g_t
            lda         #0
            sbc         g_t + 1
            sta         g_t + 1
:
            rts

; g_xl = cos, g_xr = sin of dr_a degrees (* 10000), from the table of 0-90
dr_cossin:
            MOVW        g_t, dr_a                           ; (0-359)
@mod:
            lda         g_t + 1
            bmi         @plus
            lda         g_t
            cmp         #<360
            lda         g_t + 1
            sbc         #>360
            bcc         @have
            sec
            lda         g_t
            sbc         #<360
            sta         g_t
            lda         g_t + 1
            sbc         #>360
            sta         g_t + 1
            bra         @mod
@plus:
            clc
            lda         g_t
            adc         #<360
            sta         g_t
            lda         g_t + 1
            adc         #>360
            sta         g_t + 1
            bra         @mod
@have:
            MOVW        g_r, g_t
            jsr         dr_sin
            MOVW        g_xr, g_t
            clc                                             ; (cos: sin of 90 more)
            lda         g_r
            adc         #90
            sta         g_t
            lda         g_r + 1
            adc         #0
            sta         g_t + 1
            lda         g_t
            cmp         #<360
            lda         g_t + 1
            sbc         #>360
            bcc         :+
            sec
            lda         g_t
            sbc         #<360
            sta         g_t
            lda         g_t + 1
            sbc         #>360
            sta         g_t + 1
:
            jsr         dr_sin
            MOVW        g_xl, g_t
            rts

; g_t = sin of g_t degrees (0-359) * 10000
dr_sin:
            stz         g_k                                 ; (Its sign: past 180, -)
            lda         g_t
            cmp         #<180
            lda         g_t + 1
            sbc         #>180
            bcc         :+
            inc         g_k
            sec
            lda         g_t
            sbc         #<180
            sta         g_t
            lda         g_t + 1
            sbc         #>180
            sta         g_t + 1
:
            lda         g_t                                 ; (Past 90: 180 less)
            cmp         #91
            bcc         :+
            sec
            lda         #180
            sbc         g_t
            sta         g_t
:
            lda         g_t
            asl         a
            tax
            lda         dr_sintab,x
            sta         g_t
            lda         dr_sintab + 1,x
            sta         g_t + 1
            lda         g_k
            beq         :+
            sec
            lda         #0
            sbc         g_t
            sta         g_t
            lda         #0
            sbc         g_t + 1
            sta         g_t + 1
:
            rts
dr_sintab:
            .word       0, 175, 349, 523, 698, 872, 1045, 1219, 1392, 1564, 1736, 1908, 2079, 2250, 2419, 2588, 2756
            .word       2924, 3090, 3256, 3420, 3584, 3746, 3907, 4067, 4226, 4384, 4540, 4695, 4848, 5000, 5150, 5299
            .word       5446, 5592, 5736, 5878, 6018, 6157, 6293, 6428, 6561, 6691, 6820, 6947, 7071, 7193, 7314, 7431
            .word       7547, 7660, 7771, 7880, 7986, 8090, 8192, 8290, 8387, 8480, 8572, 8660, 8746, 8829, 8910, 8988
            .word       9063, 9135, 9205, 9272, 9336, 9397, 9455, 9511, 9563, 9613, 9659, 9703, 9744, 9781, 9816, 9848
            .word       9877, 9903, 9925, 9945, 9962, 9976, 9986, 9994, 9998, 10000

; A DRAW move by (g_x2, g_y2) from the last point: a line unless B; the last point there unless N
dr_go:
            MOVW        g_x, g_lx
            MOVW        g_y, g_ly
            clc
            lda         g_lx
            adc         g_x2
            sta         g_x2
            lda         g_lx + 1
            adc         g_x2 + 1
            sta         g_x2 + 1
            clc
            lda         g_ly
            adc         g_y2
            sta         g_y2
            lda         g_ly + 1
            adc         g_y2 + 1
            sta         g_y2 + 1
            lda         dr_b
            bne         :+
            LDAX        s_gline
            jsr         g_cmd4
:
            lda         dr_nr
            bne         :+
            MOVW        g_lx, g_x2
            MOVW        g_ly, g_y2
:
            rts

; ---- SPRITE

; ST_SPRITE: n, x, y: sprite n there (its image as it was); ST_SPRITEI: n, x, y, image$ (8-bit pixels: 64 bytes, 8 x 8;
; 256, 16 x 16): its image at SPR_VRAM + n * 256, then all its attributes; ST_SPRITEOFF: n taken off: it hidden
st_sprite:
            jsr         g_point
            jsr         spr_n
            LDW         g_t, 4                              ; (Its x and y: attributes 2-5)
            lda         g_x
            sta         g_buf
            lda         g_x + 1
            and         #3
            sta         g_buf + 1
            lda         g_y
            sta         g_buf + 2
            lda         g_y + 1
            and         #3
            sta         g_buf + 3
            lda         #2
            jmp         spr_attr
st_spritei:
            jsr         top_vt                              ; (Its image: 64 or 256 bytes, to lbuf)
            FARN        5, str_get_a
            lda         sla + 1
            beq         :+
            lda         sla
            beq         @lb902
            jmp         @bad
@lb902:
            lda         sla + 1
            cmp         #1
            beq         @lb901
            jmp         @bad
@lb901:
            lda         #1                                  ; (256: 16 x 16)
            bra         @size
:
            lda         sla
            cmp         #64
            beq         @lb900
            jmp         @bad
@lb900:
            lda         #0                                  ; (64: 8 x 8)
@size:
            sta         g_k
            MOVW        cn, sla
            MOVW        ca, sfa
            lda         sfa + 2
            sta         cal
            jsr         ca_sel
            ldx         #0                                  ; (Its bytes to lbuf: 64, or 256: .Y 0)
            ldy         cn
:
            jsr         ca_rd
            sta         lbuf,x
            inx
            dey
            bne         :-
            jsr         code_sel
            jsr         pop_vt
            jsr         g_point
            jsr         spr_n
            MOVW        g_r, t0
            stz         g_t                                 ; (VRAM: SPR_VRAM + n * 256)
            lda         g_r
            clc
            adc         #>SPR_VRAM
            sta         g_t + 1
            lda         #^SPR_VRAM
            adc         #0
            sta         g_t + 2
            jsr         g_vram
            LDR         r0, lbuf
            MOVW        r1, cn
            lda         g_vfd
            dec         a
            jsr         WRITE
            bcc         :+
            jmp         g_err
:
            lda         g_t + 1                             ; (Its attributes: the address's bits 12-5, 16-13 | 8 bits
            asl         a                                   ;   a pixel; x; y; in front of the text; its size)
            asl         a
            asl         a
            sta         g_buf
            lda         g_t + 1
            lsr         a
            lsr         a
            lsr         a
            lsr         a
            lsr         a
            sta         g_buf + 1
            lda         g_t + 2
            asl         a
            asl         a
            asl         a
            ora         g_buf + 1
            ora         #$80
            sta         g_buf + 1
            lda         g_x
            sta         g_buf + 2
            lda         g_x + 1
            and         #3
            sta         g_buf + 3
            lda         g_y
            sta         g_buf + 4
            lda         g_y + 1
            and         #3
            sta         g_buf + 5
            lda         #$0C
            sta         g_buf + 6
            ldx         g_k
            lda         spr_size,x
            sta         g_buf + 7
            LDW         g_t, 8
            lda         #0
            jmp         spr_attr
@bad:
            jmp         g_bad
spr_size:   .byte       $00, $50                            ; (8 x 8; 16 x 16)
st_spriteoff:
            jsr         spr_n
            stz         g_buf
            LDW         g_t, 1
            lda         #6
            ; (fall through)

; g_t's bytes of g_buf into sprite g_r's attributes, from its byte .A
spr_attr:
            sta         g_k
            lda         g_r                                 ; (Its place: n * 8 + .A)
            stz         r0 + 1
            asl         a
            rol         r0 + 1
            asl         a
            rol         r0 + 1
            asl         a
            rol         r0 + 1
            clc
            adc         g_k
            sta         r0
            bcc         :+
            inc         r0 + 1
:
            stz         r1
            stz         r1 + 1
            lda         r0
            sta         g_x
            lda         r0 + 1
            sta         g_x + 1
            LDR         r0, s_sprites
            jsr         ram_path
            lda         #O_RDWR
            jsr         OPEN
            bcc         :+
            jmp         g_err
:
            sta         g_k
            lda         g_x
            sta         r0
            lda         g_x + 1
            sta         r0 + 1
            stz         r1
            stz         r1 + 1
            lda         g_k
            ldx         #0
            jsr         SEEK
            LDR         r0, g_buf
            MOVW        r1, g_t
            lda         g_k
            jsr         WRITE
            lda         g_k
            jmp         CLOSE
s_sprites:  .byte       "/dev/vid/sprites", 0

; g_r = the sprite's number taken off the stack (1-127)
spr_n:
            jsr         pop_int16
            lda         t0 + 1
            bne         @bad
            lda         t0
            beq         @bad
            bmi         @bad
            sta         g_r
            rts
@bad:
            jmp         g_bad
