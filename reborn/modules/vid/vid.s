; ****************************************************************************
; vid - the Vera X card's driver (docs/design/plans/VIDEO.md; docs/design/reimplementation-from-scratch.md, phase 8): the VERA in
; slot 0 (I/O ports 2 and 3, $FF20-$FF3F; IRQ line 2), on srvlib: the device #v (a boot driver), at /dev/vid.  Its
; init looks for the card: the version register (DCSEL 63's "V": the X16 community's gateware, v47 on), or else
; ADDR0 read back as written (fvdhoef's v0.9).  A reset makes the FPGA configure itself again, a while with no
; answer, so it looks for DETECT_TICKS; with no card it ends (E_NODEV: no #v, and nothing at /dev/vid).
;   /ctl      mode 80x60, mode 80x30, mode 40x30 (the text's columns and rows; the screen cleared); cursor blink,
;             cursor on, cursor off; border N (its colour, 0-255); bitmap 320 D, bitmap 640 D, bitmap off (layer 0,
;             under the text, a bitmap from VRAM 0 of D bits a pixel: 1, 2, 4 or 8; 320 across shows the text
;             40x30, 640 80x60); claim, claim all, release (the chip, for direct access: below); reset (the chip set
;             up again for the console, the screen cleared).  It reads as the state, a line each: "vera 47.0.2" (the
;             gateware's version; "vera 0.9" without one), "mode 80x60", "cursor blink", "border 0", "bitmap off",
;             and "claimed", with the claimer's task (and "all") if the chip's claimed
;   /term     the screen's console: a write's bytes shown as an ANSI terminal shows them (below); a read gives the
;             screen's characters, a line a row: its columns, then an LF.  cons writes the windows' text here as
;             it sends it to the serial port (consctl's screen, serial, both)
;   /vram     the VERA's video RAM, 128K (the offset is the address), read and written through its data port 0
;   /pal      the palette (VRAM $1FA00: 256 entries, 2 bytes each: $GB, $0R): 0-15 the console's colours, the ANSI
;             terminal's (conio's 0-15), 16-255 the VERA's own
;   /sprites  the sprites' attributes (VRAM $1FC00: 128 of 8 bytes).  Sprite 0 is the console's cursor; a write here
;             stops the cursor blinking (it blinks by turning the sprites off and on)
;   /font     the console's font (VRAM $1F000: 256 characters of 8 bytes, a byte a row; ISO-8859-15 at the start):
;             cat /lib/font/cp437 >/dev/vid/font for the PC's
;   /frame    a read waits for the next frame (one since this fid last read, or opened it: the VERA's VSYNC, 59.5 a
;             second), then gives the frames counted ("1234" and an LF); a non-blocking fd gets E_AGAIN instead
;   /psg      the PSG's registers (VRAM $1F9C0: 16 voices of 4 bytes), the sound driver's (snd's channels 8-23): a
;             write's register/value pairs (0-63; others dropped, and an odd last byte) kept and written to the chip
;             (while it's claimed, kept only, and written as the claim ends); a read gives them as kept
;   /pcm      the PCM: a write's bytes into the FIFO (4K) when it's below a quarter full, as many as it has room for
;             (the rest in the kernel's next request; above a quarter: the writer waits for the next frame, 59.5 a
;             second, or a non-blocking one gets E_AGAIN), in the format pcmctl says; one task's at a time
;             (another's open: E_BUSY), till its last fid of it closes
;   /pcmctl   rate HZ (the VERA's nearest: 381 Hz a step, up to 48,828; 0 stops it), bits 8|16, mono, stereo, volume
;             N (0-15), reset (the FIFO emptied), drain (waits till the FIFO's empty); the PCM's task's, or anyone's
;             while no task has /pcm (another's: E_BUSY).  It reads as the state: "rate 22126", "bits 8", "mono",
;             "volume 15", and "claimed" with the task that has /pcm.  The samples are as the VERA takes them:
;             signed, 16 bits little-endian, stereo left first
; VRAM, shared by agreement (VIDEO.md): $00000-$1AFFF a program's; $1B000-$1EFFF the console's text map (128 x 64
; cells of 2 bytes: its 64 rows a ring, the screen's top at map row top, so a scroll is a VSCROLL write and a row
; blanked); $1F000-$1F7FF its font; $1F800 the cursor's image (8 x 8, 4 bits a pixel); $1F9C0 on the chip's
; registers.  Layer 1 is the console's text (16 colours: a cell's background 0 lets layer 0 show through).
;   Claims: a task that writes claim to ctl has the chip to itself, its registers its to write, till it writes
; release or its last file of #v closes (its end).  Meanwhile the driver leaves the chip alone (another task's
; /vram, /pal, /sprites, /font and /term reads, and the commands that change the chip: E_BUSY), and the console's
; text waits here (its last PEND_SIZE bytes); at the release the chip's set up for the console again (with claim
; all, its font and map too: a claimer may use all of VRAM) and the text that waited is shown.  The irq entry, still
; the driver's, reads and clears ISR's VSYNC, LINE and SPRCOL (a claimer's own, if it turned them on), and turns
; AFLOW off (a level: the line would stay low; /pcm's writers wait for the frames' event, not AFLOW's).
;   The terminal: printable bytes ($20-$7E, $80-$FF: the font's) at the cursor, wrapping at the last column (as a
; VT100 does: the next one goes on the next line); CR, LF (down, scrolling at the bottom), BS, TAB (every 8 columns),
; FF (cleared), BEL (nothing: cons rings the bell); ESC 7 and ESC 8 (the cursor saved, restored), ESC c (reset), ESC D
; (IND: down a row), ESC E (NEL: CR and IND), ESC M (RI: up a row); CSI t;b r (DECSTBM: the scrolling region, rows t
; to b; CSI r, the whole screen: an LF at its bottom row scrolls the region alone, RI at its top scrolls it down);
; CSI n A, B, C, D, E, F (moves), G and d (a column, a row), H and f (row;column, from 1), J and K (0: to the end, 1:
; from the start, 2: all), m (SGR: 0, 1 bold as bright, 22, 7 reverse, 27, 30-37, 39, 40-47, 49, 90-97, 100-107; 2,
; 4, 5, 24 and 25 taken and not shown), s and u, ?25h and ?25l (the cursor shown, hidden); the rest are taken and
; dropped.  The cursor is sprite 0, an underline, moved after each write.
;   Only this task touches the chip, but for a claimer.  Its code keeps CTRL at 0 (ADDR0, DCSEL 0), and sets
; another DCSEL only with the VERA's interrupt off; the irq entry writes DC_VIDEO (the blink: dcv, its copy, which
; the rest change with IRQs off), ISR and IEN.

.include "hydra.inc"
.include "hw.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "srvlib.inc"

            HYX2_DRIVER "vid", init, srv_serve, irq, 0, HF_BOOT

SRV_OPENED      = opened                                    ; (srvlib: a fid made: its task's, counted ...
SRV_CLUNKED     = clunked                                   ;   and forgotten: the claim ends with its last)
SRV_STAT        = stat                                      ;   (the files' lengths)

DETECT_TICKS    = TICK_HZ * 3 / 10 ; How long init looks for the card (its FPGA configuring itself after a reset)
MAP_M           = $B0           ; The text map, $1B000: ADDR_M of its row 0 (a row is 256 bytes: 128 cells) ...
MAP_ROWS        = 64            ;   its rows (a ring)
FONT_ADDR       = $F000         ; The font, $1F000 (these: bit 16 set)
CURSOR_ADDR     = $F800         ; The cursor's image, $1F800
SPRITE0         = $FC00         ; Sprite 0's attributes, $1FC00: the cursor
PSG_ADDR        = $F9C0         ; The PSG's registers, $1F9C0 (the chip's registers from here to the end)
H_INC1          = VERA_INC_1 | 1 ; ADDR_H: increment 1, bit 16 set (the console's part of VRAM)
H_INC2          = VERA_INC_2 | 1 ;   increment 2 (a row's characters, not their colours)
L1_TEXT         = $60           ; L1_CONFIG: a map 64 high, 128 wide; 1 bit a pixel, 16 colours (text)
L1_MAP          = $1B000 >> 9   ; L1_MAPBASE
L1_TILES        = ($1F000 >> 11) << 2 ; L1_TILEBASE: 8 x 8
BLINK           = 20            ; The cursor's blink: frames on, then off
FG              = 7             ; The text's colours as the terminal starts: light grey on black
BG              = 0
COLS_MAX        = 80
PEND_SIZE       = 1024          ; The console's text kept while the chip's claimed (a power of 2)
NPAR            = 4             ; CSI's numbers kept
CHUNK           = 255           ; A read of /term: a part, to the client
BS              = $08
FF              = $0C
ESC             = $1B
E_TERM          = 2             ; srv_tree's entries: term ...
E_VRAM          = 3             ;   the VRAM's files (vram, pal, sprites, font: their regions, SE_AUX) ...
E_FRAME         = 7             ;   frame ...
E_PSG           = 8             ;   psg ...
E_PCM           = 9             ;   pcm
PCM_16          = $20           ; AUDIO_CTRL: 16 bits a sample ...
PCM_STEREO      = $10           ;   stereo ...
PCM_RESET       = $80           ;   the FIFO emptied (a write; a read's bit 7: full, bit 6: empty)
PCM_FAST        = 3072 / 256    ; /pcm's parts of 256 that fit unchecked below AFLOW's mark (a quarter of the FIFO)
PCM_STEP        = 381           ; Hz a step of AUDIO_RATE (48,828.125 / 128)
PSG_REGS        = 64            ; The PSG's registers

.zeropage
frames:     .res        4                                   ; The frames counted (the irq entry's)
blink_n:    .res        1                                   ; Frames till the cursor's next blink (the irq entry's)
blinking:   .res        1                                   ; <> 0: the irq entry blinks the cursor
dcv:        .res        1                                   ; DC_VIDEO as written (the irq entry's too)
cx:         .res        1                                   ; The cursor: its column ...
cy:         .res        1                                   ;   and row on the screen
cols:       .res        1                                   ; The screen's columns ...
rows:       .res        1                                   ;   and rows
top:        .res        1                                   ; The map's row at the screen's top
attr:       .res        1                                   ; The colours written now: background << 4 | foreground
wrap:       .res        1                                   ; <> 0: the last column written (the next goes below)
vok:        .res        1                                   ; <> 0: ADDR0 is at the cursor's cell
st:         .res        1                                   ; A sequence coming in: 0 none, 1 ESC, 2 CSI
p:          .res        2                                   ; A pointer
t:          .res        2                                   ; Scratch (the terminal's)
n:          .res        2                                   ; A count (the terminal's)
va:         .res        3                                   ; A VRAM address
num:        .res        4                                   ; A number (in decimal)
cnt:        .res        2                                   ; A request's bytes still to go ...
done:       .res        2                                   ;   and done
owner:      .res        1                                   ; The request's task + 1 (its fid's)

.bss
version:    .res        3                                   ; The gateware's version (major $FF: none, v0.9)
fg:         .res        1                                   ; SGR's: the foreground (0-15) ...
bg:         .res        1                                   ;   the background ...
bold:       .res        1                                   ;   bold (shown bright) ...
rev:        .res        1                                   ;   reverse
saved:      .res        6                                   ; ESC 7's: cx, cy, fg, bg, bold, rev
stop:       .res        1                                   ; The scrolling region: its first row ...
sbot:       .res        1                                   ;   and its last (DECSTBM's; the whole screen: 0, rows - 1)
par:        .res        NPAR                                ; CSI's numbers ...
npar:       .res        1                                   ;   the one being read ...
priv:       .res        1                                   ;   <> 0: CSI ?
cur_on:     .res        1                                   ; <> 0: the cursor shown (?25h)
cur_mode:   .res        1                                   ; ctl's cursor: 0 off, 1 on, 2 blink
border:     .res        1
mode:       .res        1                                   ; 0 80x60, 1 80x30, 2 40x30
bitmap:     .res        1                                   ; Layer 0's bitmap: 0 off, 1 320 across, 2 640 ...
bmdepth:    .res        1                                   ;   its depth (0-3: 1, 2, 4, 8 bits)
claimer:    .res        1                                   ; The task + 1 that has the chip (0: nobody) ...
claim_all:  .res        1                                   ;   <> 0: all of VRAM
refs:       .res        16                                  ; Each task's fids on #v
fframe:     .res        SRV_FIDS                            ; Each fid's last frame seen (its low byte: /frame's)
pend_h:     .res        2                                   ; The text kept while the chip's claimed: the next in ...
pend_n:     .res        2                                   ;   the bytes there (PEND_SIZE at most) ...
pend_lost:  .res        1                                   ;   <> 0: older ones gone
pend:       .res        PEND_SIZE
psg:        .res        PSG_REGS                            ; The PSG's registers as written to /psg
pcm_ctl:    .res        1                                   ; The PCM as pcmctl has it: AUDIO_CTRL (bits 5-4 the
pcm_rate:   .res        1                                   ;   format, 3-0 the volume), AUDIO_RATE (0: stopped) ...
pcm_owner:  .res        1                                   ;   the task + 1 that has /pcm (0: nobody) ...
pcm_refs:   .res        1                                   ;   and its fids of it
iobuf:      .res        256

.code

; ****************************************************************************
; Its init: the card found (or E_NODEV), the chip set up for the console, its line owned, its device letter
init:
            stz         claimer
            stz         bitmap
            stz         mode
            stz         border
            lda         #2
            sta         cur_mode
            lda         #1
            sta         cur_on
            lda         #$0F                                ; The PCM: 8-bit mono, at full volume, stopped
            sta         pcm_ctl
            stz         pcm_rate
            stz         pcm_owner
            stz         pcm_refs
            ldx         #PSG_REGS - 1                       ; The PSG quiet (till snd writes it)
:
            stz         psg,X
            dex
            bpl         :-
            jsr         TICKS                               ; ---- The card: looked for, DETECT_TICKS at most
            sta         t
            stx         t + 1
@look:
            jsr         detect
            bcc         @found
            lda         #1
            ldx         #0
            jsr         SLEEP
            jsr         TICKS
            sec
            sbc         t
            pha
            txa
            sbc         t + 1
            tax
            pla
            cpx         #0
            bne         @none
            cmp         #DETECT_TICKS
            bcc         @look
@none:
            lda         #E_NODEV
            sec
            rts

@found:
            jsr         setup_all                           ; ---- The chip, the console's
            lda         #LINE_SLOT0A                        ; Its interrupt (VSYNC: the frames)
            jsr         IRQ_OWN
            bcs         @failed
            jsr         irq_on
            jsr         cursor_show
            lda         #'v'
            jmp         SRV_REGISTER

@failed:
            rts

; Is the card there?  Its version register (DCSEL 63), or ADDR0 as written (v0.9: version $FF).  OUT: C = 0 yes,
; the version in version; C = 1 no (or not yet: configuring).  CTRL left 0.  Modifies .A
detect:
            lda         #VERA_DCSEL_VER
            sta         VERA_CTRL
            lda         VERA_DC_VER0
            cmp         #VERA_VER_ID
            bne         @readback
            lda         VERA_DC_VER1
            sta         version
            lda         VERA_DC_VER2
            sta         version + 1
            lda         VERA_DC_VER3
            sta         version + 2
            stz         VERA_CTRL
            clc
            rts

@readback:
            stz         VERA_CTRL                           ; (No card: a floating bus, which reads $FF here)
            lda         #$5A
            sta         VERA_ADDR_L
            lda         #$A5
            sta         VERA_ADDR_M
            lda         VERA_ADDR_L
            cmp         #$5A
            bne         @no
            lda         VERA_ADDR_M
            cmp         #$A5
            bne         @no
            lda         #$FF
            sta         version
            clc
            rts

@no:
            sec
            rts

; ****************************************************************************
; The irq entry (LINE_SLOT0A: the VERA's IRQ#): the interrupts that came and are on cleared (but AFLOW, a level:
; turned off); a VSYNC counted, its readers told, and the cursor blinked.  About 50 cycles, 70 with the blink
irq:
            lda         VERA_ISR
            and         VERA_IEN
            bit         #VERA_IRQ_AFLOW
            bne         @aflow
@cleared:
            and         #VERA_IRQ_VSYNC | VERA_IRQ_LINE | VERA_IRQ_SPRCOL
            sta         VERA_ISR
            lsr                                             ; A VSYNC?
            bcc         @done
            inc         frames
            bne         :+
            inc         frames + 1
            bne         :+
            inc         frames + 2
            bne         :+
            inc         frames + 3
:
            inc         TASK_EVENT                          ; (The frame's readers look again)
            dec         blink_n
            bne         @done
            lda         #BLINK
            sta         blink_n
            lda         blinking
            beq         @done
            lda         dcv
            eor         #VERA_DC_SPRITES
            sta         dcv
            sta         VERA_DC_VIDEO
@done:
            lda         #0
            rts

@aflow:
            pha                                             ; AFLOW on, and nobody fills the FIFO: its interrupt off
            lda         #VERA_IRQ_AFLOW
            trb         VERA_IEN
            pla
            bra         @cleared

; The VERA's interrupts as the console has them: VSYNC (what came meanwhile cleared first).  Modifies .A
irq_on:
            lda         #VERA_IRQ_VSYNC | VERA_IRQ_LINE | VERA_IRQ_SPRCOL
            sta         VERA_ISR
            lda         #VERA_IRQ_VSYNC
            sta         VERA_IEN
            rts

; ****************************************************************************
; The chip set up for the console, its interrupts off meanwhile (IEN 0: irq_on after): the registers, the PSG /psg's,
; the PCM pcmctl's (its FIFO emptied),
; the palette, the sprites (all off but the cursor's), the cursor's image, the layers, the screen's size; and the
; font, and the terminal reset, the screen cleared (setup_all).  Modifies .A, .X, .Y, p, n
setup_all:
            jsr         setup
            jsr         load_font
            jmp         term_reset

setup:
            stz         VERA_IEN
            stz         VERA_CTRL
            php                                             ; (The screen off meanwhile)
            sei
            stz         blinking
            stz         dcv
            stz         VERA_DC_VIDEO
            plp
            lda         #<PSG_ADDR                          ; ---- VRAM's registers: the PSG's, /psg's ...
            ldx         #>PSG_ADDR
            jsr         vseek1
            ldx         #0
:
            lda         psg,X
            sta         VERA_DATA0
            inx
            cpx         #PSG_REGS
            bcc         :-
            lda         pcm_ctl                             ;   the PCM's FIFO emptied, its format, volume and
            ora         #PCM_RESET                          ;   rate pcmctl's ...
            sta         VERA_AUDIO_CTRL
            lda         pcm_rate
            sta         VERA_AUDIO_RATE
            ldx         #0                                  ;   the palette ...
:
            lda         palette,X
            sta         VERA_DATA0
            inx
            bne         :-
:
            lda         palette + 256,X
            sta         VERA_DATA0
            inx
            bne         :-
            ldx         #0                                  ;   the sprites off (4 x 256 bytes) ...
            jsr         vzero
            jsr         vzero
            jsr         vzero
            jsr         vzero
            lda         #<SPRITE0                           ;   sprite 0: the cursor (its place: cursor_show) ...
            ldx         #>SPRITE0
            jsr         vseek1
            ldx         #0
:
            lda         cursor_attr,X
            sta         VERA_DATA0
            inx
            cpx         #8
            bcc         :-
            lda         #<CURSOR_ADDR                       ;   and its image
            ldx         #>CURSOR_ADDR
            jsr         vseek1
            ldx         #0
:
            lda         cursor_img,X
            sta         VERA_DATA0
            inx
            cpx         #32
            bcc         :-
            lda         #L1_TEXT                            ; ---- Layer 1: the text
            sta         VERA_L1_CONFIG
            lda         #L1_MAP
            sta         VERA_L1_MAPBASE
            lda         #L1_TILES
            sta         VERA_L1_TILEBASE
            stz         VERA_L1_HSCROLL_L
            stz         VERA_L1_HSCROLL_H
            jsr         scroll_set
            lda         #1 << 1                             ; ---- The active area: all of it (DCSEL 1)
            sta         VERA_CTRL
            stz         VERA_DC_HSTART
            lda         #640 >> 2
            sta         VERA_DC_HSTOP
            stz         VERA_DC_VSTART
            lda         #480 >> 1
            sta         VERA_DC_VSTOP
            stz         VERA_CTRL
            lda         border
            sta         VERA_DC_BORDER
            jsr         layer0_set                          ; ---- Layer 0 (a bitmap, or nothing), the scales, the
            jsr         mode_set                            ;   screen's size
            lda         #VERA_DC_OUT_VGA | VERA_DC_LAYER1   ; ---- On (the cursor's sprite: cursor_show)
            ldx         bitmap
            beq         :+
            ora         #VERA_DC_LAYER0
:
            sta         dcv
            sta         VERA_DC_VIDEO
            rts

; The font into VRAM, from this module's (ISO-8859-15).  Modifies .A, .X, .Y, p
load_font:
            lda         #<FONT_ADDR
            ldx         #>FONT_ADDR
            jsr         vseek1
            LDR         p, font
            ldx         #8                                  ; (8 pages)
            ldy         #0
:
            lda         (p),Y
            sta         VERA_DATA0
            iny
            bne         :-
            inc         p + 1
            dex
            bne         :-
            rts

; ADDR0 = $1xxxx (.A/.X its low 16 bits), increment 1.  Modifies .A
vseek1:
            sta         VERA_ADDR_L
            stx         VERA_ADDR_M
            lda         #H_INC1
            sta         VERA_ADDR_H
            stz         vok
            rts

; .X zeros (0: 256) to DATA0.  OUT: .X = 0
vzero:
            stz         VERA_DATA0
            dex
            bne         vzero
            rts

; ADDR0 = va (17 bits), increment 1.  Modifies .A
vseek_va:
            lda         va
            sta         VERA_ADDR_L
            lda         va + 1
            sta         VERA_ADDR_M
            lda         va + 2
            and         #1
            ora         #VERA_INC_1
            sta         VERA_ADDR_H
            stz         vok
            rts

; L1's VSCROLL = top * 8 (the screen's top row).  Modifies .A
scroll_set:
            lda         top
            asl
            asl
            asl
            sta         VERA_L1_VSCROLL_L
            lda         top
            lsr
            lsr
            lsr
            lsr
            lsr
            sta         VERA_L1_VSCROLL_H
            rts

; The scales and the screen's size, from mode (with a bitmap 320 across: 40x30; 640: 80x60).  Modifies .A, .X
mode_set:
            ldx         mode
            lda         bitmap
            beq         :+
            ldx         #2
            cmp         #1
            beq         :+
            ldx         #0
:
            stx         mode
            lda         mode_cols,X
            sta         cols
            lda         mode_rows,X
            sta         rows
            lda         mode_hs,X
            sta         VERA_DC_HSCALE
            lda         mode_vs,X
            sta         VERA_DC_VSCALE
            lda         cx                                  ; (The cursor on the screen still)
            cmp         cols
            bcc         :+
            stz         cx
:
            lda         cy
            cmp         rows
            bcc         :+
            stz         cy
:
            jmp         region_all

; The scrolling region: the whole screen.  Modifies .A
region_all:
            stz         stop
            lda         rows
            dec         a
            sta         sbot
            rts

; Layer 0: a bitmap at VRAM 0 (bitmap, bmdepth), if there's one.  Modifies .A
layer0_set:
            lda         bitmap
            beq         @done
            lda         bmdepth
            ora         #$04                                ; (Bitmap mode)
            sta         VERA_L0_CONFIG
            stz         VERA_L0_MAPBASE
            lda         bitmap
            cmp         #2                                  ; (TILEW: 640 across; its tiles at 0)
            lda         #0
            rol
            sta         VERA_L0_TILEBASE
            stz         VERA_L0_HSCROLL_H                   ; (Its palette offset: 0)
@done:
            rts

; ****************************************************************************
; The terminal

; The terminal as it starts: the screen cleared, the cursor home, the colours plain.  Modifies .A, .X, .Y, n, t
term_reset:
            stz         st
            stz         bold
            stz         rev
            lda         #FG
            sta         fg
            lda         #BG
            sta         bg
            jsr         attr_set
            jsr         region_all
            jmp         cls

; attr from fg, bg, bold (bright) and rev.  Modifies .A, .X, t
attr_set:
            lda         fg
            ldx         bold
            beq         :+
            cmp         #8
            bcs         :+
            ora         #8
:
            ldx         rev
            bne         @rev
            sta         t
            lda         bg
            asl
            asl
            asl
            asl
            ora         t
            sta         attr
            rts

@rev:
            asl
            asl
            asl
            asl
            ora         bg
            sta         attr
            rts

; ADDR0 = the cell at column .X of the screen's row .Y, increment 1.  Modifies .A
cell_at:
            txa
            asl
            sta         VERA_ADDR_L
            tya
            clc
            adc         top
            and         #MAP_ROWS - 1
            clc
            adc         #MAP_M
            sta         VERA_ADDR_M
            lda         #H_INC1
            sta         VERA_ADDR_H
            rts

; Blank cells (a space, attr): n of them from column .X of row .Y.  Modifies .A, .X, .Y
blank:
            jsr         cell_at
            stz         vok
            ldx         n
            beq         @done
            lda         #' '
            ldy         attr
:
            sta         VERA_DATA0
            sty         VERA_DATA0
            dex
            bne         :-
@done:
            rts

; Row .Y blanked, all its columns.  Keeps .Y.  Modifies .A, .X, n
blank_row:
            lda         #COLS_MAX
            sta         n
            ldx         #0
            phy
            jsr         blank
            ply
            rts

; The screen's rows from .Y to its last blanked.  Modifies .A, .X, .Y, n
blank_rows:
            cpy         rows
            bcs         :+
            jsr         blank_row
            iny
            bra         blank_rows
:
            rts

; FF, CSI 2J's: the screen cleared, the cursor home.  Modifies .A, .X, .Y, n
cls:
            ldy         #0
            jsr         blank_rows
home:
            stz         cx
            stz         cy
moved:
            stz         wrap
            stz         vok
            rts

; A byte to the terminal.  IN: .A.  Modifies .A, .X, .Y, t, n
putc:
            ldx         st
            bne         @seq
            cmp         #' '
            bcc         @control
            cmp         #$7F
            beq         @drop
            jmp         put_char

@control:
            cmp         #CR
            bne         :+
            stz         cx
            bra         moved
:
            cmp         #LF
            bne         :+
            jmp         line_feed
:
            cmp         #BS
            bne         :+
            lda         cx
            beq         @drop
            dec         cx
            bra         moved
:
            cmp         #TAB
            bne         :+
            lda         cx                                  ; The next of every 8 columns (the last, at most)
            and         #$F8
            clc
            adc         #8
            cmp         cols
            bcc         @tab
            ldx         cols
            dex
            txa
@tab:
            sta         cx
            bra         moved
:
            cmp         #FF
            beq         cls
            cmp         #ESC
            bne         @drop
            lda         #1
            sta         st
@drop:
            rts

@seq:
            cpx         #1
            bne         @csi
            stz         st                                  ; ---- After an ESC
            cmp         #'['
            bne         :+
            lda         #2
            sta         st
            stz         npar
            stz         priv
            stz         par
            stz         par + 1
            stz         par + 2
            stz         par + 3
            rts
:
            cmp         #'7'
            bne         :+
            jmp         save_cursor
:
            cmp         #'8'
            bne         :+
            jmp         restore_cursor
:
            cmp         #'D'
            bne         :+
            jmp         line_feed
:
            cmp         #'E'
            bne         :+
            stz         cx
            jmp         line_feed
:
            cmp         #'M'
            bne         :+
            jmp         rev_index
:
            cmp         #'c'
            bne         :+
            jmp         term_reset
:
            rts

@csi:
            cmp         #'0'                                ; ---- In a CSI: a number's digit ...
            bcc         @notdigit
            cmp         #'9' + 1
            bcs         @notdigit
            and         #$0F
            sta         t
            ldx         npar
            lda         par,X                               ; (* 10 + the digit; past 255: 255)
            cmp         #26
            bcs         @big
            asl
            sta         t + 1
            asl
            asl
            adc         t + 1
            adc         t
            bcs         @big
            sta         par,X
            rts

@big:
            lda         #255
            sta         par,X
            rts

@notdigit:
            cmp         #';'                                ;   the next number ...
            bne         :+
            lda         npar
            cmp         #NPAR - 1
            bcs         @kept
            inc         npar
@kept:
            rts
:
            cmp         #'?'                                ;   a private mode ...
            bne         :+
            sta         priv
            rts
:
            cmp         #'@'                                ;   (another intermediate: dropped) ...
            bcc         @kept
            stz         st                                  ;   or the final: done
            ldx         #0
:
            ldy         csi_final,X
            beq         @kept
            cmp         csi_final,X
            beq         :+
            inx
            bra         :-
:
            txa
            asl
            tax
            jmp         (csi_go,X)

; A printable byte at the cursor.  IN: .A.  Modifies .A, .X, .Y, n
put_char:
            ldx         wrap
            beq         :+
            pha
            stz         cx
            jsr         line_feed
            pla
:
            ldx         vok
            bne         :+
            pha
            ldx         cx
            ldy         cy
            jsr         cell_at
            pla
            ldx         #1
            stx         vok
:
            sta         VERA_DATA0
            lda         attr
            sta         VERA_DATA0
            lda         cx
            inc         a
            cmp         cols
            bcs         @last
            sta         cx
            rts

@last:
            lda         #1                                  ; (The last column: the cursor stays, the next goes below)
            sta         wrap
            stz         vok
            rts

; LF: down a row; at the scrolling region's bottom, the region scrolled up a row (the whole screen: the map's next
; row blanked, then VSCROLL on a row; part of it: its rows copied up, its last blanked).  Below the region, down to
; the screen's last row and no further.  Modifies .A, .X, .Y, n, t
line_feed:
            stz         wrap
            stz         vok
            lda         cy
            cmp         sbot
            beq         @scroll
            inc         a
            cmp         rows
            bcs         :+
            sta         cy
:
            rts

@scroll:
            jsr         region_whole
            bne         region_up
            ldy         rows                                ; (The row below the screen: hidden till the scroll)
            jsr         blank_row
            lda         top
            inc         a
            and         #MAP_ROWS - 1
            sta         top
            jmp         scroll_set

; ESC M (RI): up a row; at the scrolling region's top, the region scrolled down a row (the whole screen: the map's
; row above blanked, then VSCROLL back a row; part of it: its rows copied down, its first blanked).  Above the
; region, up to the screen's top and no further.  Modifies .A, .X, .Y, n, t
rev_index:
            stz         wrap
            stz         vok
            lda         cy
            cmp         stop
            beq         @scroll
            cmp         #0
            beq         :+
            dec         cy
:
            rts

@scroll:
            jsr         region_whole
            bne         region_down
            ldy         #$FF                                ; (The row above the screen: hidden till the scroll)
            jsr         blank_row
            lda         top
            dec         a
            and         #MAP_ROWS - 1
            sta         top
            jmp         scroll_set

; Is the scrolling region the whole screen?  OUT: Z = 1 yes.  Modifies .A
region_whole:
            lda         stop
            bne         :+
            lda         sbot
            inc         a
            cmp         rows
:
            rts

; The region's rows (stop to sbot) scrolled up a row, its last blanked.  Modifies .A, .X, .Y, n, t
region_up:
            lda         stop
@row:
            sta         t + 1                               ; (To this row, from the one below)
            cmp         sbot
            bcs         @blank
            inc         a
            sta         t
            jsr         copy_row
            lda         t
            bra         @row

@blank:
            ldy         sbot
            jmp         blank_row

; The region's rows scrolled down a row, its first blanked.  Modifies .A, .X, .Y, n, t
region_down:
            lda         sbot
@row:
            sta         t + 1                               ; (To this row, from the one above)
            cmp         stop
            beq         @blank
            bcc         @blank
            dec         a
            sta         t
            jsr         copy_row
            lda         t
            bra         @row

@blank:
            ldy         stop
            jmp         blank_row

; Screen row t's cells (COLS_MAX: characters and colours) copied to row t + 1, through the data ports: 1 reads, 0
; writes (CTRL's ADDRSEL 1 a moment; DCSEL 0 all along, for the irq entry's DC_VIDEO).  Modifies .A, .X, .Y
copy_row:
            lda         #VERA_CTRL_ADDRSEL
            sta         VERA_CTRL
            ldx         #0
            ldy         t
            jsr         cell_at
            stz         VERA_CTRL
            ldx         #0
            ldy         t + 1
            jsr         cell_at
            ldx         #COLS_MAX / 2                       ; (Two cells a time)
:
            lda         VERA_DATA1
            sta         VERA_DATA0
            lda         VERA_DATA1
            sta         VERA_DATA0
            lda         VERA_DATA1
            sta         VERA_DATA0
            lda         VERA_DATA1
            sta         VERA_DATA0
            dex
            bne         :-
            rts

; ESC 7, CSI s: the cursor and the colours saved; ESC 8, CSI u: back
save_cursor:
            lda         cx
            sta         saved
            lda         cy
            sta         saved + 1
            lda         fg
            sta         saved + 2
            lda         bg
            sta         saved + 3
            lda         bold
            sta         saved + 4
            lda         rev
            sta         saved + 5
            rts

restore_cursor:
            lda         saved
            cmp         cols
            bcs         :+
            sta         cx
:
            lda         saved + 1
            cmp         rows
            bcs         :+
            sta         cy
:
            lda         saved + 2
            sta         fg
            lda         saved + 3
            sta         bg
            lda         saved + 4
            sta         bold
            lda         saved + 5
            sta         rev
            jsr         attr_set
            jmp         moved

; CSI's first number, at least 1 (a move's count).  OUT: .A
count1:
            lda         par
            bne         :+
            inc         a
:
            rts

; .A, at most the last row (or column: last_col).  OUT: .A
last_row:
            cmp         rows
            bcc         :+
            lda         rows
            dec         a
:
            rts

last_col:
            cmp         cols
            bcc         :+
            lda         cols
            dec         a
:
            rts

; CSI n A: up n rows (to the top at most); CSI n F: and to the line's start
csi_prev:
            stz         cx
csi_up:
            jsr         count1
            sta         t
            lda         cy
            sec
            sbc         t
            bcs         :+
            lda         #0
:
            sta         cy
            jmp         moved

; CSI n B: down n (to the bottom at most); CSI n E: and to the line's start
csi_next:
            stz         cx
csi_down:
            jsr         count1
            clc
            adc         cy
            bcc         :+
            lda         #$FF
:
            jsr         last_row
            sta         cy
            jmp         moved

; CSI n C: right n (to the last column at most)
csi_right:
            jsr         count1
            clc
            adc         cx
            bcc         :+
            lda         #$FF
:
            jsr         last_col
            sta         cx
            jmp         moved

; CSI n D: left n
csi_left:
            jsr         count1
            sta         t
            lda         cx
            sec
            sbc         t
            bcs         :+
            lda         #0
:
            sta         cx
            jmp         moved

; CSI n G: column n (from 1)
csi_col:
            jsr         count1
            dec         a
            jsr         last_col
            sta         cx
            jmp         moved

; CSI n d: row n (from 1)
csi_row:
            jsr         count1
            dec         a
            jsr         last_row
            sta         cy
            jmp         moved

; CSI r;c H (and f): row r, column c (from 1; none: 1)
csi_pos:
            jsr         count1
            dec         a
            jsr         last_row
            sta         cy
            lda         par + 1
            bne         :+
            inc         a
:
            dec         a
            jsr         last_col
            sta         cx
            jmp         moved

; CSI n J: 0 the screen from the cursor, 1 to it, 2 (3) all of it (the cursor where it is)
csi_ed:
            lda         par
            beq         @below
            cmp         #1
            beq         @above
            ldy         #0
            jsr         blank_rows
            jmp         moved

@below:
            jsr         erase_eol
            ldy         cy
            iny
            jsr         blank_rows
            jmp         moved

@above:
            ldy         #0
:
            cpy         cy
            bcs         :+
            jsr         blank_row
            iny
            bra         :-
:
            jsr         erase_bol
            jmp         moved

; CSI n K: 0 the line from the cursor, 1 to it, 2 all of it
csi_el:
            lda         par
            beq         @eol
            cmp         #1
            beq         @bol
            ldy         cy
            jsr         blank_row
            jmp         moved

@eol:
            jsr         erase_eol
            jmp         moved

@bol:
            jsr         erase_bol
            jmp         moved

; The cursor's line blanked from it to the end, or from the start to it.  Modifies .A, .X, .Y, n
erase_eol:
            lda         cols
            sec
            sbc         cx
            sta         n
            ldx         cx
            ldy         cy
            jmp         blank

erase_bol:
            ldx         cx
            inx
            stx         n
            ldx         #0
            ldy         cy
            jmp         blank

; CSI n;... m: SGR, each of its numbers
csi_sgr:
            ldx         #0
@one:
            phx
            lda         par,X
            jsr         sgr
            plx
            inx
            cpx         npar
            beq         @one
            bcc         @one
            jmp         attr_set

; One SGR number, .A.  Modifies .A
sgr:
            cmp         #0
            bne         :+
            lda         #FG
            sta         fg
            lda         #BG
            sta         bg
            stz         bold
            stz         rev
            rts
:
            cmp         #1
            bne         :+
            sta         bold
            rts
:
            cmp         #22
            bne         :+
            stz         bold
            rts
:
            cmp         #7
            bne         :+
            sta         rev
            rts
:
            cmp         #27
            bne         :+
            stz         rev
            rts
:
            cmp         #39
            bne         :+
            lda         #FG
            sta         fg
            rts
:
            cmp         #49
            bne         :+
            lda         #BG
            sta         bg
            rts
:
            sec                                             ; 30-37, 40-47, 90-97, 100-107 (C = 1 after each
            sbc         #30                                 ;   cmp past them)
            cmp         #8
            bcs         :+
            sta         fg
            rts
:
            sbc         #10
            cmp         #8
            bcs         :+
            sta         bg
            rts
:
            sbc         #50
            cmp         #8
            bcs         :+
            ora         #8
            sta         fg
            rts
:
            sbc         #10
            cmp         #8
            bcs         :+
            ora         #8
            sta         bg
:
            rts

; CSI t;b r (DECSTBM): the scrolling region, rows t to b (from 1; none: the screen's first and last), if t is above
; b; the cursor home.  CSI r: the whole screen
csi_region:
            lda         priv
            bne         @done
            lda         par
            beq         :+
            dec         a
:
            sta         t
            lda         par + 1
            beq         :+
            cmp         rows
            bcc         :++
:
            lda         rows
:
            dec         a
            cmp         t
            beq         @done
            bcc         @done
            sta         sbot
            lda         t
            sta         stop
            jmp         home

@done:
            rts

; CSI ?25h, ?25l: the cursor shown, hidden
csi_set:
            lda         #1
            bra         :+

csi_reset:
            lda         #0
:
            ldx         priv
            beq         @done
            ldx         par
            cpx         #25
            bne         @done
            sta         cur_on
@done:
            rts

; The cursor's sprite at the cursor, on (and blinking: ctl's cursor blink), or off; dcv changed with IRQs off.
; Nothing while the chip's claimed.  Modifies .A, .X
cursor_show:
            lda         claimer
            bne         @done
            lda         cur_mode
            beq         @off
            lda         cur_on
            beq         @off
            lda         #<(SPRITE0 + 2)                     ; Its place: the cell's, in the layer's pixels
            ldx         #>(SPRITE0 + 2)
            jsr         vseek1
            lda         cx
            asl
            asl
            asl
            sta         VERA_DATA0
            lda         cx
            lsr
            lsr
            lsr
            lsr
            lsr
            sta         VERA_DATA0
            lda         cy
            asl
            asl
            asl
            sta         VERA_DATA0
            lda         cy
            lsr
            lsr
            lsr
            lsr
            lsr
            sta         VERA_DATA0
            ldx         #0
            lda         cur_mode
            cmp         #2
            bne         :+
            inx
:
            php
            sei
            stx         blinking
            lda         #BLINK                              ; (Shown now, the blink starting again)
            sta         blink_n
            lda         dcv
            ora         #VERA_DC_SPRITES
            bra         @dcv

@off:
            php
            sei
            stz         blinking
            lda         dcv
            and         #<~VERA_DC_SPRITES
@dcv:
            sta         dcv
            sta         VERA_DC_VIDEO
            plp
@done:
            rts

; ****************************************************************************
; The files

; This part of a request: r1 = its place in the client's buffer (RQ_BUF + done), r2 = its length (cnt, 256 at
; most).  OUT: Z = 1, none left.  Modifies .A
part:
            lda         cnt
            sta         r2
            lda         cnt + 1
            beq         :+
            stz         r2
            lda         #1
:
            sta         r2 + 1
            clc
            lda         TASK_INBOX + RQ_BUF
            adc         done
            sta         r1
            lda         TASK_INBOX + RQ_BUF + 1
            adc         done + 1
            sta         r1 + 1
            lda         r2
            ora         r2 + 1
            rts

; A part done: done += r2, cnt -= r2, RQ_DONE = done.  Modifies .A
parted:
            clc
            lda         done
            adc         r2
            sta         done
            sta         TASK_INBOX + RQ_DONE
            lda         done + 1
            adc         r2 + 1
            sta         done + 1
            sta         TASK_INBOX + RQ_DONE + 1
            sec
            lda         cnt
            sbc         r2
            sta         cnt
            lda         cnt + 1
            sbc         r2 + 1
            sta         cnt + 1
            rts

; The request's count, all of it to go: cnt; done and RQ_DONE 0.  Modifies .A
counted:
            MOVR        cnt, TASK_INBOX + RQ_COUNT
            stz         done
            stz         done + 1
            stz         TASK_INBOX + RQ_DONE
            stz         TASK_INBOX + RQ_DONE + 1
            rts

; Is the chip another task's (claimed, not by the request's)?  OUT: C = 1, .A = E_BUSY yes; C = 0 no.  Sets
; owner.  Modifies .A, .X
others:
            ldx         z:srv_fid
            lda         srv_fid_aux,X
            sta         owner
            lda         claimer
            beq         :+
            cmp         owner
            beq         :+
            lda         #E_BUSY
            sec
            rts
:
            clc
            rts

; /term: a write, its bytes to the terminal (or kept, while the chip's claimed); a read, the screen's characters
h_term:
            cmp         #R_WRITE
            beq         w_term
            cmp         #R_READ
            bne         :+
            jmp         r_term
:
            clc
            rts

w_term:
            jsr         counted
@part:
            jsr         part
            beq         @end
            LDR         r0, iobuf
            jsr         CLIENT_READ
            ldx         #0
@byte:
            phx
            lda         iobuf,X
            ldx         claimer
            bne         @keep
            jsr         putc
            bra         @next

@keep:
            jsr         pend_put
@next:
            plx
            inx
            cpx         r2                                  ; (r2 0: 256)
            bne         @byte
            jsr         parted
            bra         @part

@end:
            jsr         cursor_show
            clc
            rts

; A byte of the console's kept while the chip's claimed (the last PEND_SIZE).  IN: .A.  Modifies .A, .X, p
pend_put:
            tax
            clc
            lda         pend_h
            adc         #<pend
            sta         p
            lda         pend_h + 1
            and         #>(PEND_SIZE - 1)
            adc         #>pend
            sta         p + 1
            txa
            sta         (p)
            inc         pend_h
            bne         :+
            inc         pend_h + 1
:
            lda         pend_n + 1                          ; (Full: the oldest gone)
            cmp         #>PEND_SIZE
            bcc         :+
            sta         pend_lost
            rts
:
            inc         pend_n
            bne         :+
            inc         pend_n + 1
:
            rts

; The bytes kept to the terminal (the chip back: release), the screen cleared first if some were lost.  Modifies
; .A, .X, .Y, p, t, n, va
pend_play:
            lda         pend_lost
            beq         :+
            jsr         cls
:
            sec                                             ; va: the oldest's place (pend_h - pend_n)
            lda         pend_h
            sbc         pend_n
            sta         va
            lda         pend_h + 1
            sbc         pend_n + 1
            sta         va + 1
@byte:
            lda         pend_n
            ora         pend_n + 1
            beq         @done
            clc
            lda         va
            adc         #<pend
            sta         p
            lda         va + 1
            and         #>(PEND_SIZE - 1)
            adc         #>pend
            sta         p + 1
            lda         (p)
            jsr         putc
            inc         va
            bne         :+
            inc         va + 1
:
            lda         pend_n
            bne         :+
            dec         pend_n + 1
:
            dec         pend_n
            bra         @byte

@done:
            stz         pend_h
            stz         pend_h + 1
            stz         pend_lost
            rts

; /term: a read: the screen's characters from the offset, a line a row (cols characters, then an LF); E_BUSY while
; another task has the chip.  (va: the row and column; va + 2, the row ADDR0 is in)
r_term:
            jsr         others
            bcc         :+
            rts
:
            jsr         counted
            lda         TASK_INBOX + RQ_OFFSET + 2
            ora         TASK_INBOX + RQ_OFFSET + 3
            bne         @end
            ldx         cols                                ; The offset's row and column
            inx
            stx         p
            MOVR        t, TASK_INBOX + RQ_OFFSET
            jsr         div8                                ; (t: the row; .A the column)
            sta         va + 1
            lda         t + 1
            bne         @end
            lda         t
            sta         va
            lda         #$FF
            sta         va + 2
            stz         vok
            ldy         #0                                  ; (.Y: iobuf's bytes)
@byte:
            lda         cnt                                 ; All it asked for?
            ora         cnt + 1
            beq         @flush
            lda         va
            cmp         rows
            bcs         @flush
            lda         va + 1
            cmp         cols
            bcc         @char
            stz         va + 1                              ; The row's end: an LF
            inc         va
            lda         #LF
            bra         @put

@char:
            lda         va                                  ; (ADDR0 at this row's characters: set once a row)
            cmp         va + 2
            beq         :+
            sta         va + 2
            phy
            ldx         va + 1
            ldy         va
            jsr         cell_at
            ply
            lda         #H_INC2
            sta         VERA_ADDR_H
:
            inc         va + 1
            lda         VERA_DATA0
@put:
            sta         iobuf,Y
            iny
            lda         cnt
            bne         :+
            dec         cnt + 1
:
            dec         cnt
            cpy         #CHUNK
            bcc         @byte
            jsr         give
            ldy         #0
            bra         @byte

@flush:
            jsr         give
@end:
            clc
            rts

; iobuf's first .Y bytes to the client (at RQ_BUF + done): done and RQ_DONE on.  Modifies .A, .X, r0-r2
give:
            sty         r2
            stz         r2 + 1
            tya
            beq         @done
            LDR         r0, iobuf
            clc
            lda         TASK_INBOX + RQ_BUF
            adc         done
            sta         r1
            lda         TASK_INBOX + RQ_BUF + 1
            adc         done + 1
            sta         r1 + 1
            jsr         CLIENT_WRITE
            clc
            lda         done
            adc         r2
            sta         done
            sta         TASK_INBOX + RQ_DONE
            lda         done + 1
            adc         #0
            sta         done + 1
            sta         TASK_INBOX + RQ_DONE + 1
@done:
            rts

; t / p (8 bits): t the quotient, .A the remainder.  Modifies .X
div8:
            lda         #0
            ldx         #16
@bit:
            asl         t
            rol         t + 1
            rol         a
            cmp         p
            bcc         :+
            sbc         p
            inc         t
:
            dex
            bne         @bit
            rts

; /vram, /pal, /sprites, /font: a region of VRAM each (SE_AUX: its number in regions), read or written at the
; offset through ADDR0; E_BUSY while another task has the chip
h_vram:
            cmp         #R_READ
            beq         :+
            cmp         #R_WRITE
            beq         :+
            clc
            rts
:
            jsr         others
            bcc         :+
            rts
:
            jsr         counted
            ldy         #SE_AUX                             ; The region (.X: its entry in regions)
            lda         (srv_ent),Y
            asl
            asl
            asl
            tax
            lda         TASK_INBOX + RQ_OFFSET + 3          ; Past its end: nothing
            bne         @none
            sec                                             ; What's left of it after the offset (17 bits: num,
            lda         regions + 4,X                       ;   num + 1, num + 2)
            sbc         TASK_INBOX + RQ_OFFSET
            sta         num
            lda         regions + 5,X
            sbc         TASK_INBOX + RQ_OFFSET + 1
            sta         num + 1
            lda         regions + 6,X
            sbc         TASK_INBOX + RQ_OFFSET + 2
            bcc         @none
            sta         num + 2
            ora         num
            ora         num + 1
            bne         @some
@none:
            clc
            rts

@some:
            lda         num + 2                             ; Less than the count?  Then only that
            bne         :+
            lda         num + 1
            cmp         cnt + 1
            bne         @cmp
            lda         num
            cmp         cnt
@cmp:
            bcs         :+
            MOVR        cnt, num
:
            clc                                             ; va: the region's start + the offset
            lda         regions,X
            adc         TASK_INBOX + RQ_OFFSET
            sta         va
            lda         regions + 1,X
            adc         TASK_INBOX + RQ_OFFSET + 1
            sta         va + 1
            lda         regions + 2,X
            adc         TASK_INBOX + RQ_OFFSET + 2
            sta         va + 2
            jsr         vseek_va
            lda         regions + 3,X                       ; (/sprites written: the cursor steady, as its blink
            beq         @part                               ;   turns every sprite off and on)
            lda         TASK_INBOX + RQ_TYPE
            cmp         #R_WRITE
            bne         @part
            lda         cur_mode
            cmp         #2
            bne         @part
            dec         cur_mode
@part:
            jsr         part
            beq         @done
            LDR         r0, iobuf
            lda         TASK_INBOX + RQ_TYPE
            cmp         #R_WRITE
            beq         @write
            ldy         #0                                  ; ---- A read: VRAM into iobuf, then to the client
:
            lda         VERA_DATA0
            sta         iobuf,Y
            iny
            cpy         r2                                  ; (r2 0: 256)
            bne         :-
            jsr         CLIENT_WRITE
            bra         @next

@write:
            jsr         CLIENT_READ                         ; ---- A write: the client's bytes, then into VRAM
            ldy         #0
:
            lda         iobuf,Y
            sta         VERA_DATA0
            iny
            cpy         r2
            bne         :-
@next:
            jsr         parted
            bra         @part

@done:
            jsr         cursor_show                         ; (The cursor's sprite as it was; ADDR0 the cursor's
@end:                                                       ;   no more: vok 0)
            clc
            rts

; /psg: a write: register/value pairs, each kept, and written to the chip through ADDR1 (ADDR0, the cursor's, left
; as it is) unless the chip's claimed (setup writes them as the claim ends); a register past the PSG's, and an odd
; last byte, dropped.  A read: them as kept, from the offset (64 bytes in all)
h_psg:
            cmp         #R_WRITE
            beq         @write
            cmp         #R_READ
            beq         @read
            clc
            rts

@read:
            stz         TASK_INBOX + RQ_DONE
            stz         TASK_INBOX + RQ_DONE + 1
            lda         TASK_INBOX + RQ_OFFSET + 1
            ora         TASK_INBOX + RQ_OFFSET + 2
            ora         TASK_INBOX + RQ_OFFSET + 3
            bne         @end
            sec                                             ; r2: what's left after the offset ...
            lda         #PSG_REGS
            sbc         TASK_INBOX + RQ_OFFSET
            bcc         @end
            beq         @end
            sta         r2
            stz         r2 + 1
            lda         TASK_INBOX + RQ_COUNT + 1           ;   or the count, if that's less
            bne         :+
            lda         TASK_INBOX + RQ_COUNT
            cmp         r2
            bcs         :+
            sta         r2
:
            clc
            lda         #<psg
            adc         TASK_INBOX + RQ_OFFSET
            sta         r0
            lda         #>psg
            adc         #0
            sta         r0 + 1
            jsr         srv_toclient
@end:
            clc
            rts

@write:
            jsr         counted
@part:
            jsr         part
            beq         @end
            LDR         r0, iobuf
            jsr         CLIENT_READ
            lda         r2 + 1                              ; Its pairs (256 bytes: 128)
            lsr
            lda         r2
            ror
            beq         @parted
            sta         n
            stz         t                                   ; t: $FF, to the chip too (ADDR1 at the PSG, no
            lda         claimer                             ;   increment); 0, the chip's claimed
            bne         :+
            dec         t
            lda         #VERA_CTRL_ADDRSEL
            sta         VERA_CTRL
            lda         #>PSG_ADDR
            sta         VERA_ADDR_M
            lda         #1
            sta         VERA_ADDR_H
:
            ldy         #0
@pair:
            ldx         iobuf,Y
            cpx         #PSG_REGS
            bcs         @next
            lda         iobuf + 1,Y
            sta         psg,X
            bit         t
            bpl         @next
            txa
            ora         #<PSG_ADDR
            sta         VERA_ADDR_L
            lda         psg,X
            sta         VERA_DATA1
@next:
            iny
            iny
            dec         n
            bne         @pair
            bit         t
            bpl         @parted
            stz         VERA_CTRL
@parted:
            jsr         parted
            bra         @part

; /pcm: a write: below AFLOW's mark (a quarter full), its bytes into the PCM FIFO, the first 3K unchecked, then as
; many as it has room for (FULL looked at before each): a short write, the kernel sending the rest in its next
; request.  Above the mark: E_AGAIN (none taken), the writer waiting for the next frame's event; so the requests come
; three quarters of the FIFO at a time, not a byte or two as it drains (the kernel goes on while a server takes any).
; While another task has the chip claimed: E_AGAIN, till the release
h_pcm:
            cmp         #R_WRITE
            beq         :+
            clc
            rts
:
            jsr         others
            bcc         :+
            lda         #E_AGAIN
            rts
:
            jsr         counted
            lda         VERA_ISR                            ; Above a quarter full: none yet
            and         #VERA_IRQ_AFLOW
            bne         :+
            lda         #E_AGAIN
            sec
            rts
:
            lda         #PCM_FAST                           ; t: the parts that fit unchecked
            sta         t
@part:
            jsr         part
            beq         @end
            LDR         r0, iobuf
            jsr         CLIENT_READ
            ldy         #0
            lda         t
            beq         @check
            dec         t
@fast:
            lda         iobuf,Y
            sta         VERA_AUDIO_DATA
            iny
            cpy         r2                                  ; (r2 0: 256)
            bne         @fast
            bra         @parted

@check:
            bit         VERA_AUDIO_CTRL                     ; (Bit 7: full)
            bmi         @full
            lda         iobuf,Y
            sta         VERA_AUDIO_DATA
            iny
            cpy         r2
            bne         @check
@parted:
            jsr         parted
            bra         @part

@full:                                                      ; Full: what went in, or none (E_AGAIN)
            tya
            clc
            adc         done
            sta         TASK_INBOX + RQ_DONE
            lda         done + 1
            adc         #0
            sta         TASK_INBOX + RQ_DONE + 1
            ora         TASK_INBOX + RQ_DONE
            bne         @end
            lda         #E_AGAIN
            sec
            rts

@end:
            clc
            rts

; A pcmctl command's say: the PCM's task's, or anyone's while no task has /pcm.  OUT: C = 1, .A = E_BUSY: another
; task has it.  Sets owner.  Modifies .A, .X
pcm_may:
            ldx         z:srv_fid
            lda         srv_fid_aux,X
            sta         owner
            lda         pcm_owner
            beq         :+
            cmp         owner
            beq         :+
            lda         #E_BUSY
            sec
            rts
:
            clc
            rts

; The PCM's format, volume and rate to the chip (another task's claim: as it ends).  OUT: C = 0.  Modifies .A
pcm_apply:
            lda         claimer
            beq         :+
            cmp         owner
            bne         @done
:
            lda         pcm_ctl
            sta         VERA_AUDIO_CTRL
            lda         pcm_rate
            sta         VERA_AUDIO_RATE
@done:
            clc
            rts

; The command's first word a number?  OUT: C = 0 yes; C = 1, .A = E_INVAL, no (or none)
pcm_num:
            lda         z:srv_argn
            beq         @inval
            lda         srv_argp
            sta         z:srv_p
            lda         srv_argp + 1
            sta         z:srv_p + 1
            lda         (srv_p)
            cmp         #'0'
            bcc         @inval
            cmp         #'9' + 1
            bcs         @inval
            clc
            rts

@inval:
            lda         #E_INVAL
            sec
            rts

; rate HZ: AUDIO_RATE the nearest, (HZ + 190) / 381 (128 at most; under 191 Hz, 0: stopped)
p_rate:
            jsr         pcm_may
            bcs         @done
            jsr         pcm_num
            bcs         @done
            clc
            lda         srv_arg
            adc         #<(PCM_STEP / 2)
            sta         t
            lda         srv_arg + 1
            adc         #>(PCM_STEP / 2)
            sta         t + 1
            ldx         #128
            bcs         @have                               ; (Past 65,535: the most)
            ldx         #0
@step:
            sec
            lda         t
            sbc         #<PCM_STEP
            tay
            lda         t + 1
            sbc         #>PCM_STEP
            bcc         @have
            sta         t + 1
            sty         t
            inx
            cpx         #128
            bcc         @step
@have:
            stx         pcm_rate
            jmp         pcm_apply

@done:
            rts

; bits 8 | 16
p_bits:
            jsr         pcm_may
            bcs         @done
            jsr         pcm_num
            bcs         @done
            lda         srv_arg + 1
            bne         @inval
            lda         srv_arg
            cmp         #8
            beq         @eight
            cmp         #16
            bne         @inval
            lda         pcm_ctl
            ora         #PCM_16
            bra         @set

@eight:
            lda         pcm_ctl
            and         #<~PCM_16
@set:
            sta         pcm_ctl
            jmp         pcm_apply

@inval:
            jmp         inval

@done:
            rts

; mono, stereo
p_mono:
            jsr         pcm_may
            bcs         :+
            lda         pcm_ctl
            and         #<~PCM_STEREO
            sta         pcm_ctl
            jmp         pcm_apply
:
            rts

p_stereo:
            jsr         pcm_may
            bcs         :+
            lda         pcm_ctl
            ora         #PCM_STEREO
            sta         pcm_ctl
            jmp         pcm_apply
:
            rts

; volume N (0-15)
p_volume:
            jsr         pcm_may
            bcs         @done
            jsr         pcm_num
            bcs         @done
            lda         srv_arg + 1
            bne         @inval
            lda         srv_arg
            cmp         #16
            bcs         @inval
            sta         t
            lda         pcm_ctl
            and         #PCM_16 | PCM_STEREO
            ora         t
            sta         pcm_ctl
            jmp         pcm_apply

@inval:
            jmp         inval

@done:
            rts

; reset: the FIFO emptied (what's in it not played)
p_reset:
            jsr         pcm_may
            bcs         @done
            lda         claimer
            beq         :+
            cmp         owner
            bne         @ok
:
            lda         pcm_ctl
            ora         #PCM_RESET
            sta         VERA_AUDIO_CTRL
@ok:
            clc
@done:
            rts

; drain: answered when the FIFO's empty (E_AGAIN till then: the writer waits for the frames' event; v0.9 has no
; empty flag: below a quarter full); at once if it's stopped.  Another task's claim: E_AGAIN, till it ends
p_drain:
            jsr         others
            bcs         @wait
            lda         pcm_rate
            beq         @done
            lda         version
            cmp         #$FF
            beq         @v09
            bit         VERA_AUDIO_CTRL                     ; (Bit 6: empty)
            bvs         @done
            bra         @wait

@v09:
            lda         VERA_ISR
            and         #VERA_IRQ_AFLOW
            bne         @done
@wait:
            lda         #E_AGAIN
            sec
            rts

@done:
            clc
            rts

; pcmctl's state: rate (in Hz: AUDIO_RATE * 381 + its half less its 32nd, as 48,828.125 / 128 has it), bits, mono or
; stereo, volume, claimed
gen_pcm:
            stz         t                                   ; The rate in Hz
            stz         t + 1
            ldx         pcm_rate
            beq         @hz
:
            clc
            lda         t
            adc         #<PCM_STEP
            sta         t
            lda         t + 1
            adc         #>PCM_STEP
            sta         t + 1
            dex
            bne         :-
            lda         pcm_rate
            lsr
            clc
            adc         t
            sta         t
            bcc         :+
            inc         t + 1
:
            lda         pcm_rate
            lsr
            lsr
            lsr
            lsr
            lsr
            sta         n
            sec
            lda         t
            sbc         n
            sta         t
            bcs         @hz
            dec         t + 1
@hz:
            lda         #<s_rate_sp
            ldx         #>s_rate_sp
            jsr         srv_tputs
            lda         t
            ldx         t + 1
            jsr         srv_tputdec
            lda         #<s_nl_bits
            ldx         #>s_nl_bits
            jsr         srv_tputs
            ldy         #8
            lda         pcm_ctl
            and         #PCM_16
            beq         :+
            ldy         #16
:
            tya
            ldx         #0
            jsr         srv_tputdec
            lda         #LF
            jsr         srv_tputc
            lda         pcm_ctl
            and         #PCM_STEREO
            bne         :+
            lda         #<s_mono
            ldx         #>s_mono
            bra         :++
:
            lda         #<s_stereo
            ldx         #>s_stereo
:
            jsr         srv_tputs
            lda         #<s_nl_volume
            ldx         #>s_nl_volume
            jsr         srv_tputs
            lda         pcm_ctl
            and         #$0F
            ldx         #0
            jsr         srv_tputdec
            lda         #<s_nl_claimed
            ldx         #>s_nl_claimed
            jsr         srv_tputs
            lda         pcm_owner
            beq         :+
            lda         #' '
            jsr         srv_tputc
            lda         pcm_owner
            dec         a
            ldx         #0
            jsr         srv_tputdec
:
            lda         #LF
            jsr         srv_tputc
            clc
            rts

; /frame: a read: the next frame (since this fid's last), then the frames counted; none yet: E_AGAIN (the irq
; entry's event, every frame).  IN: .X = the fid
h_frame:
            cmp         #R_READ
            beq         :+
            clc
            rts
:
            lda         frames                              ; (Its low byte: one look, as the irq entry changes it)
            cmp         fframe,X
            bne         :+
            lda         #E_AGAIN
            sec
            rts
:
            sta         fframe,X
            php                                             ; The count: its text
            sei
            ldx         #3
:
            lda         frames,X
            sta         num,X
            dex
            bpl         :-
            plp
            stz         z:srv_tlen
            jsr         tputdec32
            lda         #LF
            jsr         srv_tputc
            stz         TASK_INBOX + RQ_DONE                ; Its text whole, whatever the offset (an event)
            stz         TASK_INBOX + RQ_DONE + 1
            lda         z:srv_tlen
            sta         r2
            stz         r2 + 1
            lda         TASK_INBOX + RQ_COUNT + 1
            bne         :+
            lda         TASK_INBOX + RQ_COUNT
            cmp         r2
            bcs         :+
            sta         r2
:
            LDR         r0, srv_text
            jsr         srv_toclient
            clc
            rts

; num's 32 bits in decimal, to the text.  Modifies .A, .X, num
tputdec32:
            lda         #0                                  ; (A 0 on the stack: the digits' end)
            pha
@digit:
            ldx         #32                                 ; num / 10: the remainder in .A
            lda         #0
@bit:
            asl         num
            rol         num + 1
            rol         num + 2
            rol         num + 3
            rol         a
            cmp         #10
            bcc         :+
            sbc         #10
            inc         num
:
            dex
            bne         @bit
            ora         #'0'
            pha
            lda         num
            ora         num + 1
            ora         num + 2
            ora         num + 3
            bne         @digit
@out:
            pla
            beq         :+
            jsr         srv_tputc
            bra         @out
:
            rts

; ****************************************************************************
; ctl

; ctl's state: vera, mode, cursor, border, bitmap, claimed
gen_ctl:
            lda         #<s_vera
            ldx         #>s_vera
            jsr         srv_tputs
            lda         version
            cmp         #$FF
            bne         :+
            lda         #<s_v09
            ldx         #>s_v09
            jsr         srv_tputs
            bra         @mode
:
            ldx         #0
            jsr         srv_tputdec
            lda         #'.'
            jsr         srv_tputc
            lda         version + 1
            ldx         #0
            jsr         srv_tputdec
            lda         #'.'
            jsr         srv_tputc
            lda         version + 2
            ldx         #0
            jsr         srv_tputdec
@mode:
            lda         #<s_nl_mode
            ldx         #>s_nl_mode
            jsr         srv_tputs
            lda         mode
            ldx         #<mode_names
            ldy         #>mode_names
            jsr         tput_name
            lda         #<s_nl_cursor
            ldx         #>s_nl_cursor
            jsr         srv_tputs
            lda         cur_mode
            ldx         #<cursor_names
            ldy         #>cursor_names
            jsr         tput_name
            lda         #<s_nl_border
            ldx         #>s_nl_border
            jsr         srv_tputs
            lda         border
            ldx         #0
            jsr         srv_tputdec
            lda         #<s_nl_bitmap
            ldx         #>s_nl_bitmap
            jsr         srv_tputs
            lda         bitmap
            bne         :+
            lda         #<s_off
            ldx         #>s_off
            jsr         srv_tputs
            bra         @claimed
:
            cmp         #2
            beq         :+
            lda         #<320
            ldx         #>320
            bra         @width
:
            lda         #<640
            ldx         #>640
@width:
            jsr         srv_tputdec
            lda         #' '
            jsr         srv_tputc
            ldx         bmdepth
            lda         depth_bits,X
            ldx         #0
            jsr         srv_tputdec
@claimed:
            lda         #<s_nl_claimed
            ldx         #>s_nl_claimed
            jsr         srv_tputs
            lda         claimer
            beq         @done
            lda         #' '
            jsr         srv_tputc
            lda         claimer
            dec         a
            ldx         #0
            jsr         srv_tputdec
            lda         claim_all
            beq         @done
            lda         #<s_sp_all
            ldx         #>s_sp_all
            jsr         srv_tputs
@done:
            lda         #LF
            jsr         srv_tputc
            clc
            rts

; Name .A of a table of strings (.X/.Y: its pointers) to the text.  Modifies .A, .X, .Y, p
tput_name:
            stx         p
            sty         p + 1
            asl
            tay
            lda         (p),Y
            pha
            iny
            lda         (p),Y
            tax
            pla
            jmp         srv_tputs

; Is word .Y of the command's (0: the first after it) the string .A/.X?  OUT: Z = 1 yes.  Modifies .A, .Y, p, t
arg_is:
            sta         t
            stx         t + 1
            tya
            asl
            tay
            lda         srv_argp,Y
            sta         p
            lda         srv_argp + 1,Y
            sta         p + 1
            ldy         #0
:
            lda         (t),Y
            cmp         (p),Y
            bne         @no
            iny
            cmp         #0
            bne         :-
            rts

@no:
            lda         #1
            rts

; The command's first word, one of a table of strings (.A/.X: its pointers, a 0 after the last)?  OUT: C = 0, .A =
; its index; C = 1, .A = E_INVAL (none of them, or no word).  Modifies .X, .Y, p, t, n
arg_which:
            sta         n
            stx         n + 1
            lda         z:srv_argn
            beq         @inval
            ldx         #0
@next:
            txa
            asl
            tay
            lda         (n),Y
            iny
            ora         (n),Y
            beq         @inval
            phx
            lda         (n),Y
            tax
            dey
            lda         (n),Y
            ldy         #0
            jsr         arg_is
            plx
            cmp         #0                                  ; (arg_is's .A: 0, the same; plx changed Z)
            beq         @found
            inx
            bra         @next

@found:
            txa
            clc
            rts

@inval:
            lda         #E_INVAL
            sec
            rts

; A command that changes the chip: the chip must be the console's (claimed by nobody).  OUT: C = 1, .A = E_BUSY if
; it's claimed.  Modifies .A
console_has:
            lda         claimer
            beq         :+
            lda         #E_BUSY
            sec
            rts
:
            clc
            rts

; mode 80x60 | 80x30 | 40x30
c_mode:
            jsr         console_has
            bcs         @done
            lda         #<mode_names
            ldx         #>mode_names
            jsr         arg_which
            bcs         @done
            sta         mode
            stz         bitmap
            jsr         mode_set
            jsr         layer0_off
            jsr         cls
            jsr         cursor_show
            clc
@done:
            rts

; cursor off | on | blink
c_cursor:
            lda         #<cursor_names
            ldx         #>cursor_names
            jsr         arg_which
            bcs         @done
            sta         cur_mode
            jsr         cursor_show
            clc
@done:
            rts

; border N
c_border:
            jsr         console_has
            bcs         @done
            lda         z:srv_argn
            beq         inval
            lda         srv_arg + 1
            bne         inval
            lda         srv_arg
            sta         border
            sta         VERA_DC_BORDER
            clc
@done:
            rts

inval:
            lda         #E_INVAL
            sec
            rts

; bitmap off | 320 D | 640 D
c_bitmap:
            jsr         console_has
            bcs         @done
            lda         z:srv_argn
            beq         @inval
            ldy         #0
            lda         #<s_off
            ldx         #>s_off
            jsr         arg_is
            bne         @on
            stz         bitmap
            jsr         layer0_off
            clc
            rts

@on:
            lda         z:srv_argn
            cmp         #2
            bcc         @inval
            ldx         #1                                  ; 320 or 640
            lda         srv_arg + 1
            cmp         #>320
            bne         :+
            lda         srv_arg
            cmp         #<320
            beq         @w
:
            ldx         #2
            lda         srv_arg + 1
            cmp         #>640
            bne         @inval
            lda         srv_arg
            cmp         #<640
            bne         @inval
@w:
            lda         srv_arg + 3                         ; The depth: 1, 2, 4 or 8
            bne         @inval
            ldy         #3
            lda         srv_arg + 2
:
            cmp         depth_bits,Y
            beq         :+
            dey
            bpl         :-
            bra         @inval
:
            sty         bmdepth
            stx         bitmap
            jsr         layer0_set
            jsr         mode_set
            php
            sei
            lda         dcv
            ora         #VERA_DC_LAYER0
            sta         dcv
            sta         VERA_DC_VIDEO
            plp
            jsr         cls
            jsr         cursor_show
            clc
@done:
            rts

@inval:
            jmp         inval

; Layer 0 off.  Modifies .A
layer0_off:
            php
            sei
            lda         dcv
            and         #<~VERA_DC_LAYER0
            sta         dcv
            sta         VERA_DC_VIDEO
            plp
            rts

; claim, claim all: the chip the writer's (E_BUSY if it's another's)
c_claim:
            jsr         others
            bcs         @done
            stz         claim_all
            lda         z:srv_argn
            beq         :+
            ldy         #0
            lda         #<s_all
            ldx         #>s_all
            jsr         arg_is
            bne         @inval
            lda         #1
            sta         claim_all
:
            lda         claimer                             ; (Its own already: as it is, all if it says so now)
            bne         @ok
            php                                             ; The blink stopped, the cursor off (its sprite's z 0)
            sei
            stz         blinking
            plp
            lda         #<(SPRITE0 + 6)
            ldx         #>(SPRITE0 + 6)
            jsr         vseek1
            stz         VERA_DATA0
            stz         pend_n
            stz         pend_n + 1
            stz         pend_h
            stz         pend_h + 1
            stz         pend_lost
            lda         owner
            sta         claimer
@ok:
            clc
@done:
            rts

@inval:
            jmp         inval

; release: the chip the console's again (the claimer's; nobody's: nothing)
c_release:
            jsr         others
            bcs         :+
            jsr         release
            clc
:
            rts

; reset: the chip set up for the console again, the screen cleared (E_BUSY if it's claimed)
c_reset:
            jsr         console_has
            bcs         :+
            jsr         setup_all
            jsr         irq_on
            jsr         cursor_show
            clc
:
            rts

; The claim ended: the chip set up for the console (its font and map too after claim all: the claimer may have had
; all of VRAM), the text that waited shown.  Modifies .A, .X, .Y, p, t, n, va
release:
            lda         claimer
            beq         @done
            stz         claimer
            lda         claim_all
            beq         :+
            jsr         setup_all
            bra         @set
:
            jsr         setup
@set:
            jsr         irq_on
            jsr         pend_play
            jsr         cursor_show
@done:
            rts

; ****************************************************************************
; srvlib's hooks

; A fid made: its task's (R_OPEN: the client; R_DUP: the old fid's), counted; a /frame's last frame now's.  IN: .X =
; the fid.  Keeps .X
opened:
            lda         z:srv_rq
            cmp         #R_OPEN
            bne         :+
            lda         TASK_INBOX + RQ_CLIENT
            inc         a
            sta         srv_fid_aux,X
:
            lda         srv_fid_entry,X                     ; /pcm: one task's at a time (another's: E_BUSY, before
            cmp         #E_PCM                              ;   it's counted)
            bne         @count
            lda         pcm_owner
            beq         :+
            cmp         srv_fid_aux,X
            beq         :+
            lda         #E_BUSY
            sec
            rts
:
            lda         srv_fid_aux,X
            sta         pcm_owner
            inc         pcm_refs
@count:
            ldy         srv_fid_aux,X
            lda         refs - 1,Y
            inc         a
            sta         refs - 1,Y
            lda         frames
            sta         fframe,X
            clc
            rts

; A fid forgotten: its task's count down; with its last, its claim ended.  IN: .X = the fid
clunked:
            lda         z:srv_e                             ; (/pcm's last: nobody's)
            cmp         #E_PCM
            bne         :+
            dec         pcm_refs
            bne         :+
            stz         pcm_owner
:
            ldy         srv_fid_aux,X
            beq         @done
            lda         refs - 1,Y
            beq         @done
            dec         a
            sta         refs - 1,Y
            bne         @done
            cpy         claimer
            bne         @done
            jsr         release
@done:
            clc
            rts

; A stat record made: the files' lengths (the VRAM's regions'; term's: rows x (cols + 1); psg's, 64)
stat:
            lda         z:srv_e
            cmp         #E_TERM
            bne         @vram
            stz         t
            stz         t + 1
            ldx         rows
@add:
            sec                                             ; (+ cols + 1)
            lda         t
            adc         cols
            sta         t
            lda         t + 1
            adc         #0
            sta         t + 1
            dex
            bne         @add
            lda         t
            sta         srv_stat + SR_LENGTH
            lda         t + 1
            sta         srv_stat + SR_LENGTH + 1
            rts

@vram:
            cmp         #E_PSG
            bne         :+
            lda         #PSG_REGS
            sta         srv_stat + SR_LENGTH
            rts
:
            cmp         #E_VRAM
            bcc         @done
            cmp         #E_FRAME
            bcs         @done
            ldy         #SE_AUX
            lda         (srv_ent),Y
            asl
            asl
            asl
            tax
            lda         regions + 4,X
            sta         srv_stat + SR_LENGTH
            lda         regions + 5,X
            sta         srv_stat + SR_LENGTH + 1
            lda         regions + 6,X
            sta         srv_stat + SR_LENGTH + 2
@done:
            rts

; ****************************************************************************
.rodata

srv_tree:
            SRV_ENTRY   s_root,    $FF, SK_DIR,  0,          SM_READ,            0      ; 0
            SRV_ENTRY   s_ctl,     0,   SK_CTL,  ctl_cmds,   SM_READ | SM_WRITE, 11     ; 1 (reads as 11)
            SRV_ENTRY   s_term,    0,   SK_DATA, h_term,     SM_READ | SM_WRITE, 0      ; 2 (E_TERM)
            SRV_ENTRY   s_vram,    0,   SK_DATA, h_vram,     SM_READ | SM_WRITE, 0      ; 3 (E_VRAM: region 0)
            SRV_ENTRY   s_pal,     0,   SK_DATA, h_vram,     SM_READ | SM_WRITE, 1      ; 4
            SRV_ENTRY   s_sprites, 0,   SK_DATA, h_vram,     SM_READ | SM_WRITE, 2      ; 5
            SRV_ENTRY   s_font,    0,   SK_DATA, h_vram,     SM_READ | SM_WRITE, 3      ; 6
            SRV_ENTRY   s_frame,   0,   SK_DATA, h_frame,    SM_READ,            0      ; 7 (E_FRAME)
            SRV_ENTRY   s_psg,     0,   SK_DATA, h_psg,      SM_READ | SM_WRITE, 0      ; 8 (E_PSG)
            SRV_ENTRY   s_pcm,     0,   SK_DATA, h_pcm,      SM_WRITE,           0      ; 9 (E_PCM)
            SRV_ENTRY   s_pcmctl,  0,   SK_CTL,  pcm_cmds,   SM_READ | SM_WRITE, 12     ; 10 (reads as 12)
            SRV_ENTRY   s_ctl,     $FE, SK_TEXT, gen_ctl,    SM_READ,            0      ; 11 (ctl's state)
            SRV_ENTRY   s_pcmctl,  $FE, SK_TEXT, gen_pcm,    SM_READ,            0      ; 12 (pcmctl's)
            .word       0

ctl_cmds:
            .word       s_mode, c_mode
            .word       s_cursor, c_cursor
            .word       s_border, c_border
            .word       s_bitmap, c_bitmap
            .word       s_claim, c_claim
            .word       s_release, c_release
            .word       s_reset, c_reset
            .word       0

pcm_cmds:
            .word       s_rate, p_rate
            .word       s_bits, p_bits
            .word       s_mono, p_mono
            .word       s_stereo, p_stereo
            .word       s_volume, p_volume
            .word       s_reset, p_reset
            .word       s_drain, p_drain
            .word       0

; The VRAM's regions (vram, pal, sprites, font): start (3 bytes), <> 0 the sprites', length (3 bytes), a byte spare
regions:
            .byte       $00, $00, $00, 0, $00, $00, $02, 0  ; vram: $00000, 128K
            .byte       $00, $FA, $01, 0, $00, $02, $00, 0  ; pal: $1FA00, 512
            .byte       $00, $FC, $01, 1, $00, $04, $00, 0  ; sprites: $1FC00, 1024
            .byte       $00, $F0, $01, 0, $00, $08, $00, 0  ; font: $1F000, 2048

; CSI's finals, and what they do
csi_final:
            .byte       "ABCDEFGdHfJKmsuhlr", 0
csi_go:
            .word       csi_up, csi_down, csi_right, csi_left, csi_next, csi_prev, csi_col, csi_row, csi_pos
            .word       csi_pos, csi_ed, csi_el, csi_sgr, save_cursor, restore_cursor, csi_set, csi_reset, csi_region

; The modes: their names, columns, rows, scales
mode_names:
            .word       s_80x60, s_80x30, s_40x30, 0
mode_cols:  .byte       80, 80, 40
mode_rows:  .byte       60, 30, 30
mode_hs:    .byte       128, 128, 64
mode_vs:    .byte       128, 64, 64
cursor_names:
            .word       s_off, s_on, s_blink, 0
depth_bits: .byte       1, 2, 4, 8

; Sprite 0, the cursor: its image at $1F800 (4 bits a pixel), z 3 (in front), 8 x 8, palette offset 0
cursor_attr:
            .byte       <(CURSOR_ADDR >> 5), ((CURSOR_ADDR | $10000) >> 13) & $0F, 0, 0, 0, 0, $0C, $00
; Its image: an underline in colour 15 (its rows 6 and 7)
cursor_img:
            .res        24, 0
            .res        8, $FF

; The console's palette ($GB, $0R each): 0-15 the ANSI colours (conio's 0-15), 16-255 the VERA's own
palette:
            .byte       $00, $00, $00, $0A, $A0, $00, $50, $0A, $0A, $00, $0A, $0A, $AA, $00, $AA, $0A
            .byte       $55, $05, $55, $0F, $F5, $05, $F5, $0F, $5F, $05, $5F, $0F, $FF, $05, $FF, $0F
            .byte       $00, $00, $11, $01, $22, $02, $33, $03, $44, $04, $55, $05, $66, $06, $77, $07
            .byte       $88, $08, $99, $09, $AA, $0A, $BB, $0B, $CC, $0C, $DD, $0D, $EE, $0E, $FF, $0F
            .byte       $11, $02, $33, $04, $44, $06, $66, $08, $88, $0A, $99, $0C, $BB, $0F, $11, $02
            .byte       $22, $04, $33, $06, $44, $08, $55, $0A, $66, $0C, $77, $0F, $00, $02, $11, $04
            .byte       $11, $06, $22, $08, $22, $0A, $33, $0C, $33, $0F, $00, $02, $00, $04, $00, $06
            .byte       $00, $08, $00, $0A, $00, $0C, $00, $0F, $21, $02, $43, $04, $64, $06, $86, $08
            .byte       $A8, $0A, $C9, $0C, $EB, $0F, $11, $02, $32, $04, $53, $06, $74, $08, $95, $0A
            .byte       $B6, $0C, $D7, $0F, $10, $02, $31, $04, $51, $06, $62, $08, $82, $0A, $A3, $0C
            .byte       $C3, $0F, $10, $02, $30, $04, $40, $06, $60, $08, $80, $0A, $90, $0C, $B0, $0F
            .byte       $21, $01, $43, $03, $64, $05, $86, $07, $A8, $09, $C9, $0B, $FB, $0D, $21, $01
            .byte       $42, $03, $63, $04, $84, $06, $A5, $08, $C6, $09, $F7, $0B, $20, $01, $41, $02
            .byte       $61, $04, $82, $05, $A2, $06, $C3, $08, $F3, $09, $20, $01, $40, $02, $60, $03
            .byte       $80, $04, $A0, $05, $C0, $06, $F0, $07, $21, $01, $43, $03, $65, $04, $86, $06
            .byte       $A8, $08, $CA, $09, $FC, $0B, $21, $01, $42, $02, $64, $03, $85, $04, $A6, $05
            .byte       $C8, $06, $F9, $07, $20, $00, $41, $01, $62, $01, $83, $02, $A4, $02, $C5, $03
            .byte       $F6, $03, $20, $00, $41, $00, $61, $00, $82, $00, $A2, $00, $C3, $00, $F3, $00
            .byte       $22, $01, $44, $03, $66, $04, $88, $06, $AA, $08, $CC, $09, $FF, $0B, $22, $01
            .byte       $44, $02, $66, $03, $88, $04, $AA, $05, $CC, $06, $FF, $07, $22, $00, $44, $01
            .byte       $66, $01, $88, $02, $AA, $02, $CC, $03, $FF, $03, $22, $00, $44, $00, $66, $00
            .byte       $88, $00, $AA, $00, $CC, $00, $FF, $00, $12, $01, $34, $03, $56, $04, $68, $06
            .byte       $8A, $08, $AC, $09, $CF, $0B, $12, $01, $24, $02, $46, $03, $58, $04, $6A, $05
            .byte       $8C, $06, $9F, $07, $02, $00, $14, $01, $26, $01, $38, $02, $4A, $02, $5C, $03
            .byte       $6F, $03, $02, $00, $14, $00, $16, $00, $28, $00, $2A, $00, $3C, $00, $3F, $00
            .byte       $12, $01, $34, $03, $46, $05, $68, $07, $8A, $09, $9C, $0B, $BF, $0D, $12, $01
            .byte       $24, $03, $36, $04, $48, $06, $5A, $08, $6C, $09, $7F, $0B, $02, $01, $14, $02
            .byte       $16, $04, $28, $05, $2A, $06, $3C, $08, $3F, $09, $02, $01, $04, $02, $06, $03
            .byte       $08, $04, $0A, $05, $0C, $06, $0F, $07, $12, $02, $34, $04, $46, $06, $68, $08
            .byte       $8A, $0A, $9C, $0C, $BE, $0F, $11, $02, $23, $04, $35, $06, $47, $08, $59, $0A
            .byte       $6B, $0C, $7D, $0F, $01, $02, $13, $04, $15, $06, $26, $08, $28, $0A, $3A, $0C
            .byte       $3C, $0F, $01, $02, $03, $04, $04, $06, $06, $08, $08, $0A, $09, $0C, $0B, $0F

; The font: ISO-8859-15 (the X16 ROM's PXLfont, public domain), with the X16's graphics at $00-$1F and $80-$9F
font:
            .incbin     "iso8859-15.fnt"

s_root:     .byte       "/", 0
s_ctl:      .byte       "ctl", 0
s_term:     .byte       "term", 0
s_vram:     .byte       "vram", 0
s_pal:      .byte       "pal", 0
s_sprites:  .byte       "sprites", 0
s_font:     .byte       "font", 0
s_frame:    .byte       "frame", 0
s_psg:      .byte       "psg", 0
s_pcm:      .byte       "pcm", 0
s_pcmctl:   .byte       "pcmctl", 0
s_rate:     .byte       "rate", 0
s_bits:     .byte       "bits", 0
s_mono:     .byte       "mono", 0
s_stereo:   .byte       "stereo", 0
s_volume:   .byte       "volume", 0
s_drain:    .byte       "drain", 0
s_rate_sp:  .byte       "rate ", 0
s_nl_bits:  .byte       LF, "bits ", 0
s_nl_volume: .byte      LF, "volume ", 0
s_mode:     .byte       "mode", 0
s_cursor:   .byte       "cursor", 0
s_border:   .byte       "border", 0
s_bitmap:   .byte       "bitmap", 0
s_claim:    .byte       "claim", 0
s_release:  .byte       "release", 0
s_reset:    .byte       "reset", 0
s_all:      .byte       "all", 0
s_80x60:    .byte       "80x60", 0
s_80x30:    .byte       "80x30", 0
s_40x30:    .byte       "40x30", 0
s_off:      .byte       "off", 0
s_on:       .byte       "on", 0
s_blink:    .byte       "blink", 0
s_vera:     .byte       "vera ", 0
s_v09:      .byte       "0.9", 0
s_nl_mode:  .byte       LF, "mode ", 0
s_nl_cursor: .byte      LF, "cursor ", 0
s_nl_border: .byte      LF, "border ", 0
s_nl_bitmap: .byte      LF, "bitmap ", 0
s_nl_claimed: .byte     LF, "claimed", 0
s_sp_all:   .byte       " all", 0

.include "srvlib.s"
