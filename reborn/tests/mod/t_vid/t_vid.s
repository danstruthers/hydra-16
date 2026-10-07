; ****************************************************************************
; t_vid - the Vera X's driver (modules/vid: #v), run as init, through its files: with no card, #v isn't there
; (E_NODEV: the test 'vid-none'); with one, ctl's state; the terminal (/term): text written and read back, a CSI
; move, a line erased, a line's end wrapping, BS and TAB, the screen scrolling (70 lines), the colours (SGR: read in
; the map's cells through /vram), the cursor's sprite (/sprites); /frame's count and its rate; /vram written and read
; back, /pal, /font, the files' lengths (STAT); ctl's commands (mode, cursor, border, bitmap, and bad ones); a claim
; (a write to the terminal E_BUSY meanwhile), claim all (the font back at the release), and a claim
; another task holds (E_BUSY) ended by its end.  Its report goes out on the serial port raw (#c/ser), past the screen.  Its child ("t_vid c") claims the chip and holds it half a second.
; (It reads a register or two of the chip itself, as a check: VSCROLL, for the map's top row; DC_VIDEO, DC_BORDER
; and L0_CONFIG.)

.include "hydra.inc"
.include "hw.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "testlib.inc"

            HYX2_PROGRAM "t_vid", main

.zeropage
ctl:        .res        1
tw:         .res        1                                   ; /term, written ...
tr:         .res        1                                   ;   and read
fd:         .res        1
child:      .res        1
args:       .res        2
n:          .res        2
k:          .res        1
t0:         .res        2
v1:         .res        2
cnt:        .res        1                                   ; (same's)

.bss
buf:        .res        256
line:       .res        8
stat:       .res        SR_SIZE

.code

; READ count bytes at offset of fd fdv into buf.  OUT: .A/.X the count, C
.macro AT       fdv, offset, count
            LDR         r0, offset
            stz         r1
            stz         r1 + 1
            lda         fdv
            ldx         #0
            jsr         SEEK
            LDR         r0, buf
            LDR         r1, count
            lda         fdv
            jsr         READ
.endmacro

; WRITE len bytes at label to fd fdv.  OUT: C
.macro PUT      fdv, label, len
            LDR         r0, label
            LDR         r1, len
            lda         fdv
            jsr         WRITE
.endmacro

; A command (the text) to ctl.  OUT: C, .A
.macro CTL      text
            jsr         ctl_cmd
            .byte       text, 0
.endmacro

; buf's first .A bytes the string at label?  OUT: .A = 0 yes
.macro SAME     label
            ldx         #<label
            ldy         #>label
            jsr         same
.endmacro

main:
            MOVR        args, r0
            lda         (args)
            cmp         #'c'
            bne         :+
            jmp         holder
:
            stz         T_FAILS
            LDR         r0, s_ser                           ; Fds 0-2: the serial port, raw (the report goes out
            lda         #O_WRITE                            ;   there, past the screen: PUTC is fd 1's while it's
            jsr         OPEN                                ;   open), so the files below are 3 on
            lda         #0
            jsr         DUP
            lda         #0
            jsr         DUP

; ---- #v: there?  (No card: E_NODEV, and that's all)
            LDR         r0, s_ctl
            lda         #O_RDWR
            jsr         OPEN
            sta         ctl
            bcc         @there
            EXPECT_ERR  E_NODEV, "no card: #v isn't there (E_NODEV)"
            DONE        "t_vid"

@there:
            OK          "OPEN #v/ctl"
            AT          ctl, 0, 255
            SAME        s_state0
            EXPECT_A    0, "ctl reads as the state: vera 47.0.2, mode 80x60, cursor blink, border 0, bitmap off, claimed"

; ---- /term: text written and read back
            LDR         r0, s_term
            lda         #O_WRITE
            jsr         OPEN
            sta         tw
            EXPECT_OK   "OPEN #v/term (written)"
            LDR         r0, s_term
            lda         #O_READ
            jsr         OPEN
            sta         tr
            EXPECT_OK   "OPEN #v/term (read)"
            PUT         tw, t_hello, t_hello_n
            EXPECT_OK   "CSI 2J, CSI H, two lines written"
            AT          tr, 0, 6
            SAME        s_hello
            EXPECT_A    0, "read back: row 0, hello"
            AT          tr, 80, 1
            lda         buf
            EXPECT_A    LF, "a row's end: an LF (80 columns)"
            AT          tr, 81, 5
            SAME        s_world
            EXPECT_A    0, "row 1: world"
            PUT         tw, t_cup, t_cup_n
            AT          tr, 4 * 81 + 9, 1
            lda         buf
            EXPECT_A    'X', "CSI 5;10H: X at row 4, column 9"
            PUT         tw, t_el, t_el_n
            AT          tr, 2 * 81, 4
            SAME        s_ab
            EXPECT_A    0, "CSI K: the line erased from the cursor (AB left)"
            PUT         tw, t_wrap, t_wrap_n
            AT          tr, 3 * 81 + 78, 2
            SAME        s_xy
            EXPECT_A    0, "the line's last columns: x, y"
            AT          tr, 4 * 81, 1
            lda         buf
            EXPECT_A    'z', "and z on the next line (wrapped)"
            PUT         tw, t_bstab, t_bstab_n
            AT          tr, 5 * 81, 9
            SAME        s_bstab
            EXPECT_A    0, "BS (c over b), TAB (d at column 8)"

; ---- The scrolling region (CSI 2;5r): a title above, a status line below; LF at its bottom, RI at its top
            PUT         tw, t_region, t_region_n
            AT          tr, 3 * 81, 2
            SAME        s_aa
            EXPECT_A    0, "CSI 2;5r, then LF at the region's bottom: the region up a row (aa on row 3)"
            AT          tr, 4 * 81, 2
            SAME        s_bb
            EXPECT_A    0, "and bb on its last row"
            AT          tr, 0, 5
            SAME        s_title
            EXPECT_A    0, "the row above the region as it was"
            AT          tr, 5 * 81, 6
            SAME        s_status
            EXPECT_A    0, "and the row below it"
            PUT         tw, t_ri, t_ri_n
            AT          tr, 4 * 81, 2
            SAME        s_aa
            EXPECT_A    0, "ESC M at the region's top: the region down a row (aa on row 4)"
            AT          tr, 81, 2
            SAME        s_sp2
            EXPECT_A    0, "its first row blank"
            AT          tr, 0, 5
            SAME        s_title
            EXPECT_A    0, "the title as it was"
            AT          tr, 5 * 81, 6
            SAME        s_status
            EXPECT_A    0, "the status line as it was"
            PUT         tw, t_rreset, t_rreset_n
            AT          tr, 4 * 81, 6
            SAME        s_status
            EXPECT_A    0, "CSI r: LF at the screen's bottom scrolls all of it (the status line up a row)"

; ---- Scrolling: 70 lines, l00 to l69: the screen shows l10 to l69
            PUT         tw, t_clear, t_clear_n
            stz         k
@lines:
            lda         #'l'
            sta         line
            lda         k
            ldx         #'0'
:
            cmp         #10
            bcc         :+
            sbc         #10
            inx
            bra         :-
:
            stx         line + 1
            ora         #'0'
            sta         line + 2
            lda         #CR
            sta         line + 3
            lda         #LF
            sta         line + 4
            LDR         r1, 5
            lda         k
            cmp         #69
            bne         :+
            LDR         r1, 3                               ; (The last, with no new line)
:
            LDR         r0, line
            lda         tw
            jsr         WRITE
            inc         k
            lda         k
            cmp         #70
            bcc         @lines
            AT          tr, 0, 3
            SAME        s_l10
            EXPECT_A    0, "scrolled: l10 at the top"
            AT          tr, 59 * 81, 3
            SAME        s_l69
            EXPECT_A    0, "and l69 at the bottom (row 59)"

; ---- Colours: SGR, in the map's cells (VRAM $1B000, the top row's: L1_VSCROLL / 8)
            PUT         tw, t_sgr, t_sgr_n
            lda         VERA_L1_VSCROLL_H                   ; (The map's row at the screen's top)
            lsr
            lda         VERA_L1_VSCROLL_L
            ror
            lsr
            lsr
            clc
            adc         #$B0                                ; ($1B000 + row * 256: the offset's middle byte)
            sta         v1 + 1
            stz         v1
            LDR         r0, s_vram
            lda         #O_RDWR
            jsr         OPEN
            sta         fd
            MOVR        r0, v1
            lda         #1
            sta         r1
            stz         r1 + 1
            ldx         #0
            lda         fd
            jsr         SEEK
            LDR         r0, buf
            LDR         r1, 8
            lda         fd
            jsr         READ
            lda         buf + 1
            EXPECT_A    $41, "CSI 31;44m: red on blue ($41)"
            lda         buf + 3
            EXPECT_A    $07, "CSI 0m: plain (light grey on black)"
            lda         buf + 5
            EXPECT_A    $0A, "CSI 1;32m: bold green, bright ($0A)"
            lda         buf + 7
            EXPECT_A    $70, "CSI 0;7m: reverse ($70)"
            lda         buf + 6
            EXPECT_A    'V', "the characters as written"

; ---- The cursor: sprite 0 at the cell, after CSI 7;21H
            PUT         tw, t_cur, t_cur_n
            LDR         r0, s_sprites
            lda         #O_READ
            jsr         OPEN
            sta         k
            LDR         r0, buf
            LDR         r1, 8
            lda         k
            jsr         READ
            lda         k
            jsr         CLOSE
            lda         buf
            EXPECT_A    $C0, "sprite 0: its image at $1F800 ..."
            lda         buf + 1
            EXPECT_A    $0F, "  (4 bits a pixel) ..."
            lda         buf + 2
            EXPECT_A    20 * 8, "  x: column 20 (160) ..."
            lda         buf + 4
            EXPECT_A    6 * 8, "  y: row 6 (48) ..."
            lda         buf + 6
            EXPECT_A    $0C, "  in front (z 3)"

; ---- /frame: a frame each read, 59.5 a second
            LDR         r0, s_frame
            lda         #O_READ
            jsr         OPEN
            sta         k
            jsr         frame_read
            MOVR        v1, n
            jsr         TICKS
            sta         t0
            stx         t0 + 1
            lda         #12
            sta         line
:
            jsr         frame_read
            dec         line
            bne         :-
            jsr         TICKS
            sec
            sbc         t0
            sta         t0
            sec
            lda         n
            sbc         v1
            EXPECT_A    12, "12 frames, 12 reads (each the frame after the last)"
            lda         t0                                  ; (12 frames: 201.6 ms, 40.3 ticks)
            cmp         #38
            bcc         :+
            cmp         #44
            bcs         :+
            OK          "12 frames in 0.2 s (59.5 a second)"
            bra         :++
:
            NOTOK       "12 frames in 0.2 s (59.5 a second)"
:
            lda         k
            jsr         CLOSE

; ---- /vram, /pal, /font
            LDR         r0, $0100
            stz         r1
            stz         r1 + 1
            lda         fd
            ldx         #0
            jsr         SEEK
            PUT         fd, s_digits, 16
            AT          fd, $0100, 16
            SAME        s_digits
            EXPECT_A    0, "/vram: 16 bytes at $00100 written and read back"
            lda         fd
            jsr         CLOSE
            LDR         r0, s_pal
            lda         #O_READ
            jsr         OPEN
            sta         k
            AT          k, 0, 4
            lda         buf + 3
            EXPECT_A    $0A, "/pal: entry 1, red ($A00)"
            lda         k
            jsr         CLOSE
            LDR         r0, s_font
            lda         #O_RDWR
            jsr         OPEN
            sta         k
            AT          k, 'A' * 8, 7
            SAME        g_a
            EXPECT_A    0, "/font: A's glyph"
            lda         k
            jsr         CLOSE
            LDR         r0, s_vram                          ; The files' lengths
            jsr         len_of
            lda         stat + SR_LENGTH + 2
            EXPECT_A    2, "/vram: 128K long"
            LDR         r0, s_term
            jsr         len_of
            lda         stat + SR_LENGTH + 1
            ldx         stat + SR_LENGTH
            cmp         #>(60 * 81)
            bne         :+
            cpx         #<(60 * 81)
:
            php
            pla
            and         #2
            EXPECT_A    2, "/term: 60 rows of 81 long"

; ---- ctl's commands
            CTL         "mode 80x30"
            EXPECT_OK   "mode 80x30"
            AT          ctl, 0, 255
            SAME        s_state1
            EXPECT_A    0, "ctl: mode 80x30"
            LDR         r0, s_term
            jsr         len_of
            lda         stat + SR_LENGTH + 1
            EXPECT_A    >(30 * 81), "/term: 30 rows now"
            CTL         "mode 9x9"
            EXPECT_ERR  E_INVAL, "mode 9x9: E_INVAL"
            CTL         "cursor on"
            EXPECT_OK   "cursor on"
            lda         VERA_DC_VIDEO
            and         #VERA_DC_SPRITES
            EXPECT_A    VERA_DC_SPRITES, "the cursor's sprite shown (DC_VIDEO)"
            CTL         "cursor off"
            lda         VERA_DC_VIDEO
            and         #VERA_DC_SPRITES
            EXPECT_A    0, "cursor off: hidden"
            CTL         "border 6"
            EXPECT_OK   "border 6"
            lda         VERA_DC_BORDER
            EXPECT_A    6, "DC_BORDER 6"
            CTL         "bitmap 320 8"
            EXPECT_OK   "bitmap 320 8"
            AT          ctl, 0, 255
            SAME        s_state2
            EXPECT_A    0, "ctl: mode 40x30, bitmap 320 8, cursor off, border 6"
            lda         VERA_L0_CONFIG
            EXPECT_A    $07, "layer 0: a bitmap, 8 bits a pixel"
            CTL         "bitmap 320 3"
            EXPECT_ERR  E_INVAL, "bitmap 320 3: E_INVAL"
            CTL         "bitmap off"
            EXPECT_OK   "bitmap off"
            CTL         "mode 80x60"
            CTL         "cursor blink"
            CTL         "border 0"
            CTL         "flash"
            EXPECT_ERR  E_INVAL, "a command it doesn't have: E_INVAL"

; ---- A claim: a write to the terminal E_BUSY meanwhile (cons paints its window again after the release)
            PUT         tw, t_clear, t_clear_n
            CTL         "claim"
            EXPECT_OK   "claim"
            AT          ctl, 0, 255
            SAME        s_state3
            EXPECT_A    0, "ctl: claimed 1"
            PUT         tw, s_kept, 4
            EXPECT_ERR  E_BUSY, "a write to /term while it's claimed: E_BUSY"
            AT          tr, 0, 4
            SAME        s_none
            EXPECT_A    0, "not shown"
            CTL         "release"
            EXPECT_OK   "release"
            PUT         tw, s_kept, 4
            EXPECT_OK   "a write to /term after the release"
            AT          tr, 0, 4
            SAME        s_kept
            EXPECT_A    0, "shown"
            CTL         "claim all"
            EXPECT_OK   "claim all"
            LDR         r0, s_font                          ; A's glyph gone ...
            lda         #O_RDWR
            jsr         OPEN
            sta         k
            LDR         r0, 'A' * 8
            stz         r1
            stz         r1 + 1
            lda         k
            ldx         #0
            jsr         SEEK
            PUT         k, s_zeros, 8
            AT          k, 'A' * 8, 8
            jsr         zeros
            EXPECT_A    0, "claim all: the font's A overwritten"
            CTL         "release"
            AT          k, 'A' * 8, 7
            SAME        g_a
            EXPECT_A    0, "and back at the release"
            lda         k
            jsr         CLOSE

; ---- A claim another task holds: E_BUSY; ended by its end
            LDR         r0, s_me
            LDR         r1, s_c
            lda         #0
            jsr         SPAWN
            sta         child
            EXPECT_OK   "SPAWN t_vid c (it claims the chip, holds it, ends)"
            lda         #TICK_HZ / 10
            ldx         #0
            jsr         SLEEP
            CTL         "mode 80x30"
            EXPECT_ERR  E_BUSY, "another task's claim: a command that changes the chip, E_BUSY"
            AT          tr, 0, 4
            EXPECT_ERR  E_BUSY, "and a read of /term, E_BUSY"
            stz         r0
            stz         r0 + 1
            lda         child
            jsr         WAIT
            AT          ctl, 0, 255
            SAME        s_state0
            EXPECT_A    0, "the claim ended with its task: claimed (nobody)"
            DONE        "t_vid"

; The child: the chip claimed, half a second, then the end (its files closed: the claim with them)
holder:
            LDR         r0, s_ctl
            lda         #O_WRITE
            jsr         OPEN
            sta         ctl
            CTL         "claim"
            lda         #TICK_HZ / 2
            ldx         #0
            jsr         SLEEP
            rts

; A command, the string after the jsr, to ctl.  OUT: C, .A
ctl_cmd:
            jsr         t_grab
            ldy         #0
:
            lda         (r0),Y
            beq         :+
            iny
            bra         :-
:
            sty         r1
            stz         r1 + 1
            lda         ctl
            jmp         WRITE

; buf's first .A bytes the string at .X/.Y?  OUT: .A = 0 yes, 1 no.  Modifies .Y, n
same:
            sta         cnt
            stx         n
            sty         n + 1
            ldy         #0
:
            lda         (n),Y
            beq         @end
            cpy         cnt
            bcs         @no
            cmp         buf,Y
            bne         @no
            iny
            bra         :-
@end:
            cpy         cnt
            bne         @no
            lda         #0
            rts

@no:
            lda         #1
            rts

; buf's first 8 bytes ORed.  OUT: .A (0: all zeros).  Modifies .X
zeros:
            lda         #0
            ldx         #7
:
            ora         buf,X
            dex
            bpl         :-
            rts

; /frame read (fd k): n = the count, in decimal.  Modifies .A, .X, .Y
frame_read:
            LDR         r0, buf
            LDR         r1, 16
            lda         k
            jsr         READ
            stz         n
            stz         n + 1
            ldy         #0
@digit:
            lda         buf,Y
            cmp         #'0'
            bcc         @done
            cmp         #'9' + 1
            bcs         @done
            and         #$0F
            pha
            lda         n                                   ; n * 10
            ldx         n + 1
            asl         n
            rol         n + 1
            asl         n
            rol         n + 1
            clc
            adc         n
            sta         n
            txa
            adc         n + 1
            sta         n + 1
            asl         n
            rol         n + 1
            pla
            clc
            adc         n
            sta         n
            bcc         :+
            inc         n + 1
:
            iny
            bra         @digit

@done:
            rts

; stat = the stat record of the path at r0.  Modifies .A, .X, .Y, r0, r1
len_of:
            LDR         r1, stat
            jmp         STAT

.rodata
s_ser:      .byte       "#c/ser", 0
s_ctl:      .byte       "#v/ctl", 0
s_term:     .byte       "#v/term", 0
s_vram:     .byte       "#v/vram", 0
s_pal:      .byte       "#v/pal", 0
s_sprites:  .byte       "#v/sprites", 0
s_font:     .byte       "#v/font", 0
s_frame:    .byte       "#v/frame", 0
s_me:       .byte       "#m/t_vid", 0
s_c:        .byte       "c", 0, 0
s_state0:   .byte       "vera 47.0.2", LF, "mode 80x60", LF, "cursor blink", LF, "border 0", LF, "bitmap off", LF, "claimed", LF, 0
s_state1:   .byte       "vera 47.0.2", LF, "mode 80x30", LF, "cursor blink", LF, "border 0", LF, "bitmap off", LF, "claimed", LF, 0
s_state2:   .byte       "vera 47.0.2", LF, "mode 40x30", LF, "cursor off", LF, "border 6", LF, "bitmap 320 8", LF, "claimed", LF, 0
s_state3:   .byte       "vera 47.0.2", LF, "mode 80x60", LF, "cursor blink", LF, "border 0", LF, "bitmap off", LF, "claimed 1", LF, 0
s_hello:    .byte       "hello ", 0
s_world:    .byte       "world", 0
s_ab:       .byte       "AB  ", 0
s_xy:       .byte       "xy", 0
s_bstab:    .byte       "ac      d", 0
s_aa:       .byte       "aa", 0
s_bb:       .byte       "bb", 0
s_sp2:      .byte       "  ", 0
s_title:    .byte       "title", 0
s_status:   .byte       "status", 0
s_l10:      .byte       "l10", 0
s_l69:      .byte       "l69", 0
s_kept:     .byte       "kept", 0
s_none:     .byte       "    ", 0
s_digits:   .byte       "0123456789ABCDEF", 0
s_zeros:    .byte       0, 0, 0, 0, 0, 0, 0, 0
g_a:        .byte       $18, $3C, $24, $66, $7E, $66, $66, 0       ; (A's glyph: its first 7 rows; its last is 0)
t_hello:    .byte       $1B, "[2J", $1B, "[H", "hello", CR, LF, "world"
t_hello_n = * - t_hello
t_cup:      .byte       $1B, "[5;10HX"
t_cup_n = * - t_cup
t_el:       .byte       $1B, "[3;1HABCDEF", $1B, "[3;3H", $1B, "[K"
t_el_n = * - t_el
t_wrap:     .byte       $1B, "[4;79Hxyz"
t_wrap_n = * - t_wrap
t_bstab:    .byte       $1B, "[6;1Hab", 8, "c", 9, "d"
t_bstab_n = * - t_bstab
t_region:   .byte       $1B, "[2J", $1B, "[Htitle", $1B, "[6;1Hstatus", $1B, "[2;5r", $1B, "[5;1Haa", CR, LF, "bb"
t_region_n = * - t_region
t_ri:       .byte       $1B, "[2;1H", $1B, "M"
t_ri_n = * - t_ri
t_rreset:   .byte       $1B, "[r", $1B, "[60;1H", CR, LF
t_rreset_n = * - t_rreset
t_clear:    .byte       $1B, "[2J", $1B, "[H"
t_clear_n = * - t_clear
t_sgr:      .byte       $1B, "[2J", $1B, "[H", $1B, "[31;44mR", $1B, "[0mn", $1B, "[1;32mG", $1B, "[0;7mV", $1B, "[m"
t_sgr_n = * - t_sgr
t_cur:      .byte       $1B, "[7;21H"
t_cur_n = * - t_cur
