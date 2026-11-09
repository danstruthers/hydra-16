; ****************************************************************************
; line.inc - (the world's line.s, the senses' dq0p/line.s: each sets the segment the line itself goes in) one line
; between two squares (ENGINE-CRITIQUE.md's §4, step 2, 2026-11-08), line.h's: the eye's (a denizen's sight of the
; character, the lights the character sees far off), a missile's (the world's line of fire, wshoot.c's) and the air's
; (a smell's past the sound layer).  One rule for what a square lets through and one way across, so that what the
; character sees it can shoot, and a denizen sees the character just when the character would see it.  The look's own
; lines (cast.s's ray, in its grid: thousands a look) are this one worked out in the grid: the same squares and
; heights, its doors and windows by ln_portal.
;
; The squares: Bresenham's, as cast.s's ray steps them (a step along if 2e >= -|dy|, across if 2e <= |dx|), from the
; first end, those between the two ends, 255 squares each way at most (at a tie not the same squares as from the other
; end: a line between the character and another is drawn from the character's end, line.h's).  What stops it: no
; square (outside the stratum), rock, a wall; a portal by ln_portal (a door shut, a window's shutters closed; glass
; the eye's alone; bars, a portcullis's up or down or a grate's, and an arrow slit let anything through); and for the
; eye and a missile a floor above the line from the eyes (ln_eye feet above the first square's floor) to the last
; square's floor, cast.s's: its height a step on ez + (tz - ez) k / n, worked out without dividing.  In assembly (in
; C, cc65's, some 1,200 bytes).  The senses' all in the main RAM; the world's line in part 65 (its line of fire's,
; wshoot.c's lof: no other part draws one), ln_pass and ln_portal in the main RAM (a missile's flight on, part 68's;
; a smell's way, wtrace.c's).
;
;   unsigned char __fastcall__ line (unsigned char sn, unsigned x, unsigned y, unsigned tx, unsigned ty,
;       unsigned char mode);            0 clear, LN_STOP a square stops it, LN_WAY ln_way's said so (mode LN_CACHED
;                                       ;   too: a chunk not in the cache stops it, none made; LINE_CACHED's alone)
;   unsigned char __fastcall__ ln_pass (unsigned char sn, unsigned x, unsigned y, unsigned char mode);
;                                       a square: 0 it stops a line, 1 it lets it through, 2 narrowly (a slit, a window)
;   unsigned char __fastcall__ ln_portal (const struct portal* o, unsigned char mode);     a portal: the same

            .macpack    longbranch
            .export     _line, _ln_pass, _ln_portal, _ln_eye, _ln_n, _ln_first, _ln_last, _ln_x, _ln_y, _ln_way
            .import     _at, _portal_at, _cx_, _cy_, pusha, pushax, incsp2, addysp
.ifdef LINE_CACHED
            .import     _cached, _at_x0, _at_y0
.endif
            .importzp   sp, ptr1

LN_AIR      = 2                             ; (line.h's: the modes,
LN_STOP     = 1                             ;   line's answers)
LN_WAY      = 2
            .include    "dq0hub_line.inc"           ; (world.h's squares, T_*; struct portal's kind, state, parts;
                                                    ;   PK_*, PP_*, PS_*: the build's, ../dq0/hubgen.js)

; (The line itself: in the segment its includer set)

; line (sn, x, y, tx, ty, mode): mode in .A, ty, tx, y, x and sn on the C stack
_line:
            sta         lmode
            ldy         #8
            lda         (sp),y
            sta         lsn
            dey
@arg:       lda         (sp),y
            sta         xy,y
            dey
            bpl         @arg
            ldy         #9
            jsr         addysp
            lda         #0
            sta         _ln_n
            lda         #1
            sta         _ln_last
            ldx         #2                  ; How far, each way, and the way
            jsr         span
            sta         ax8
            sty         sx
            ldx         #0
            jsr         span
            sta         ay8
            sty         sy
            cmp         ax8                 ; Its steps (none: clear)
            bcs         @n
            lda         ax8
@n:         sta         n
            tax                             ; (Z: none; cmp's Z is equal's)
            jeq         @clear
            lda         ax8                 ; Its error: |dx| - |dy|
            sec
            sbc         ay8
            sta         e
            lda         #0
            sbc         #0
            sta         e + 1
            lda         lmode
            and         #3
            eor         #LN_AIR
            sta         air                 ; (0: the air's)
            ldy         #4                  ; Its first square (none: stopped), the stratum's bounds at ()'s
            jsr         args
            jsr         _at
            jsr         keep
            jeq         stop
.ifdef LINE_CACHED
            stz         kok
            bit         lmode               ; (LN_CACHED: its last square's chunk in the cache, or stopped)
            bpl         @made
            ldy         #0
            jsr         args
            jsr         _cached
            tax
            jeq         stop
@made:
.endif
            lda         air                 ; The line's height: from the eyes to the last square's floor
            beq         @step
            jsr         floor
            clc
            adc         _ln_eye
            sta         lz
            txa
            adc         #0
            sta         lz + 1
            ldy         #0
            jsr         args
            jsr         _at
            jsr         keep
            beq         @ad                 ; (None: 0, .A .X)
            jsr         floor
@ad:        sec                             ; (Its rise or fall, and which way)
            sbc         lz
            sta         ad
            txa
            sbc         lz + 1
            sta         ad + 1
            stz         neg
            stz         acc
            stz         acc + 1
            bpl         @step
            inc         neg
            lda         #0
            sec
            sbc         ad
            sta         ad
            lda         #0
            sbc         ad + 1
            sta         ad + 1

@step:      lda         e                   ; A step: e2 = 2e; along if e2 >= -|dy|, across if e2 <= |dx|
            asl         a
            sta         e2
            lda         e + 1
            rol         a
            sta         e2 + 1
            lda         e2                  ; (e2 + |dy|: not below 0, along)
            clc
            adc         ay8
            lda         e2 + 1
            adc         #0
            bmi         @across
            lda         e
            sec
            sbc         ay8
            sta         e
            bcs         @x
            dec         e + 1
@x:         ldx         #6
            lda         sx
            jsr         move
@across:    lda         ax8                 ; (|dx| - e2: not below 0, across)
            sec
            sbc         e2
            lda         #0
            sbc         e2 + 1
            bmi         @there
            lda         e
            clc
            adc         ax8
            sta         e
            bcc         @y
            inc         e + 1
@y:         ldx         #4
            lda         sy
            jsr         move
@there:     ldx         #3                  ; The last square: clear
@cmp:       lda         xy + 4,x
            cmp         xy,x
            bne         @on
            dex
            bpl         @cmp
@clear:     lda         #0
            tax
            rts
@on:        inc         _ln_n
.ifdef LINE_CACHED
            bit         lmode               ; (LN_CACHED: its chunk in the cache, asked once a chunk)
            bpl         @pass
            jsr         chunk
            lda         ok
            jeq         stop
@pass:
.endif
            jsr         pass                ; What it lets through (none: stopped)
            tax
            jeq         stop
            sta         _ln_last
            ldx         _ln_n               ; (The first's too)
            dex
            bne         @h
            sta         _ln_first
@h:         lda         air
            beq         @way
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
            inc         lz
            bne         @dda
            inc         lz + 1
            bra         @dda
@down:      lda         lz
            bne         @dec
            dec         lz + 1
@dec:       dec         lz
            bra         @dda
@floor:     jsr         floor               ; A floor above the line: stopped
            sta         e2                  ; (e2 free till the next step)
            stx         e2 + 1
            lda         lz
            cmp         e2
            lda         lz + 1
            sbc         e2 + 1
            bvc         @sign
            eor         #$80
@sign:      jmi         stop
@way:       lda         _ln_way + 1         ; Asked of the square, if anything's to be
            jeq         @step
            lda         _ln_n
            jsr         @call
            tax
            jeq         @step
            lda         #LN_WAY
            ldx         #0
            rts
@call:      jmp         (_ln_way)

stop:       lda         #LN_STOP
            ldx         #0
            rts

; How far the last square is from the square, the way .X says (0: y, 2: x): .A (.Y the way: 1, or $FF back); more
; than 255: stopped (the caller's caller's return)
span:       lda         xy,x
            sec
            sbc         xy + 4,x
            pha
            lda         xy + 1,x
            sbc         xy + 5,x
            bmi         @back
            bne         @far
            pla
            ldy         #1
            rts
@back:      inc         a
            bne         @far
            pla
            eor         #$FF
            inc         a
            beq         @far1               ; (-256)
            ldy         #$FF
            rts
@far:       pla
@far1:      pla                             ; (span's return, then _line's caller's)
            pla
            bra         stop

; The square a step on, the way .X says (4: y, 6: x), .A (1, or $FF back)
move:       dec         a
            beq         @fwd
            lda         xy,x
            bne         @lo
            dec         xy + 1,x
@lo:        dec         xy,x
            rts
@fwd:       inc         xy,x
            bne         @r
            inc         xy + 1,x
@r:         rts

; lp's square's floor (the chunk's elev, CW * CW after its tile), in .A .X
floor:      lda         lp
            sta         ptr1
            lda         lp + 1
            clc
            adc         #4
            sta         ptr1 + 1
            ldx         #0
            lda         (ptr1)
            bpl         @p
            dex
@p:         rts

.ifdef LINE_CACHED
; The square's chunk in the cache (ok), asked once a chunk (its place from the stratum's x0, y0, at ()'s: kt, the
; low bytes' top three bits and the high bytes; kx, ky the last asked)
chunk:      lda         xy + 6
            sec
            sbc         _at_x0
            and         #$E0
            sta         kt
            lda         xy + 7
            sbc         _at_x0 + 1
            sta         kt + 1
            lda         xy + 4
            sec
            sbc         _at_y0
            and         #$E0
            sta         kt + 2
            lda         xy + 5
            sbc         _at_y0 + 1
            sta         kt + 3
            ldx         #3
@k:         lda         kt,x
            cmp         kx,x
            bne         @ask
            dex
            bpl         @k
            lda         kok
            bne         @r
@ask:       ldx         #3                  ; (Asked: its place kept)
@c:         lda         kt,x
            sta         kx,x
            dex
            bpl         @c
            ldy         #4
            jsr         args
            jsr         _cached
            sta         ok
            lda         #1
            sta         kok
@r:         rts
.endif

; (The rest in the main RAM: a square's, a portal's)
            .code

; at ()'s or cached ()'s arguments: sn and a square (.Y 4: the square it's at; 0: the last), y in .A .X
args:       sty         ty_
            lda         lsn
            jsr         pusha
            ldy         ty_
            lda         xy + 2,y
            ldx         xy + 3,y
            jsr         pushax
            ldy         ty_
            lda         xy,y
            ldx         xy + 1,y
            rts

; at ()'s pointer kept (lp, ptr1): Z set if it's none
keep:       sta         lp
            sta         ptr1
            stx         lp + 1
            stx         ptr1 + 1
            ora         ptr1 + 1
            rts

; What the square it's at lets through, for a line of lmode's (no square, rock, a wall: 0; a door's or a window's,
; its portal's: ln_portal's); lp at ()'s pointer to it
pass:       ldy         #4
            jsr         args
            jsr         _at
            jsr         keep
            beq         @r
            lda         (ptr1)
            and         #$0F                ; (TILE's)
            beq         @r                  ; (T_ROCK)
            cmp         #T_WALL
            beq         @no
            cmp         #T_DOOR
            beq         @portal
            cmp         #T_WINDOW
            bne         @yes
@portal:    lda         _cx_
            jsr         pusha
            lda         _cy_
            jsr         _portal_at
            sta         ptr1
            stx         ptr1 + 1
            ora         ptr1 + 1
            bne         rule
@yes:       lda         #1
@r:         rts
@no:        lda         #0
            rts

; ln_pass (sn, x, y, mode): mode in .A, y, x and sn on the C stack
_ln_pass:
            sta         lmode
            ldy         #4
            lda         (sp),y
            sta         lsn
            dey
@arg:       lda         (sp),y
            sta         xy + 4,y
            dey
            bpl         @arg
            ldy         #5
            jsr         addysp
            jsr         pass
            ldx         #0
            rts

; ln_portal (o, mode): mode in .A, o on the C stack
_ln_portal:
            sta         lmode
            ldy         #1
            lda         (sp),y
            sta         ptr1 + 1
            dey
            lda         (sp),y
            sta         ptr1
            jsr         incsp2
            ; (fall through)

; Portal ptr1 for a line of lmode's: 0 it stops it, 1 it lets it through, 2 narrowly (in .A, .X 0)
rule:       ldx         #0
            ldy         #P_KIND
            lda         (ptr1),y
            cmp         #PK_PORTCULLIS      ; (Bars, up or down)
            beq         @one
            cmp         #PK_GRATE
            beq         @one
            cmp         #PK_SLIT
            beq         @two
            cmp         #PK_WINDOW
            bne         @door
            ldy         #P_PARTS            ; (A window: its shutters closed nothing, its glass the eye alone)
            lda         (ptr1),y
            and         #PP_SHUTTERS
            beq         @glass
            jsr         state
            beq         @zero
@glass:     ldy         #P_PARTS
            lda         (ptr1),y
            and         #PP_GLASS
            beq         @two
            lda         lmode
            and         #3
            beq         @two                ; (LN_SIGHT)
@zero:      lda         #0
            rts
@door:      jsr         state               ; (A door open, or broken down)
            beq         @zero
@one:       lda         #1
            rts
@two:       lda         #2
            rts
state:      ldy         #P_STATE
            lda         (ptr1),y
            and         #PS_OPEN | PS_WRECKED
            rts

            .data
_ln_eye:    .byte       5                   ; (LN_EYE: the eyes, feet above the floor)

            .bss
_ln_n:      .res        1                   ; The squares the line's gone through, the first one's ln_pass
_ln_first:  .res        1                   ;   and the last one's,
_ln_last:   .res        1
_ln_way:    .res        2                   ;   what's asked of each square (0: nothing)
xy:         .res        8                   ; (The last square (ty, tx), the square it's at (y, x: ln_y, ln_x), as
_ln_y       = xy + 4                        ;   the C stack has them,
_ln_x       = xy + 6
lsn:        .res        1                   ;   the stratum, the mode,
lmode:      .res        1
lp:         .res        2                   ;   at ()'s pointer to the square,
ty_:        .res        1                   ;   (args's)
ax8:        .res        1                   ;   how far, each way,
ay8:        .res        1
sx:         .res        1                   ;   the way (1, or $FF back),
sy:         .res        1
n:          .res        1                   ;   its steps,
e:          .res        2                   ;   Bresenham's error, twice it,
e2:         .res        2
air:        .res        1                   ;   the air's: no heights,
lz:         .res        2                   ;   the line's height, the rise or fall to the last square's floor,
ad:         .res        2                   ;   the way it goes (not 0: down), what's left of it so far,
neg:        .res        1
acc:        .res        2
.ifdef LINE_CACHED
kx:         .res        2                   ;   the chunk asked of the cache last, ok if it's there (kok 0, none),
ky:         .res        2
kt:         .res        4                   ;   the square's chunk)
kok:        .res        1
ok:         .res        1
.endif
