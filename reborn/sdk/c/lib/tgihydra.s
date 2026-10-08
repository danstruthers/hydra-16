; ****************************************************************************
; tgihydra.s - cc65's TGI (tgi.h) on the Vera X: hydra_tgi, a static driver (vera.h: tgi_install (hydra_tgi), then
; tgi_init ()).  320 x 240 in 256 colours, on the bitmap under the console's text (vid_bitmap (320, 8)), the drawing
; done by the driver, vid (/dev/vid/draw, through vera.c's vid_ calls), so the console stays on the screen over it.
; TGI's colour n is the palette's entry its palette gives (the identity as it starts); tgi_getpixel gives the
; bitmap's byte, the palette's entry.  Its text is the console's font, 8 x 8, at the size it is (vid's text); TGI's
; vector fonts are drawn as lines, as on any target.  A failure: TGI_ERR_INV_ARG (tgi_init with no Vera X:
; TGI_ERR_INV_MODE).  The interface: cc65's asminc/tgi-kernel.inc.

            .include    "zeropage.inc"
            .include    "tgi-kernel.inc"
            .include    "tgi-error.inc"

            .import     pushax, pusheax
            .import     _vid_bitmap, _vid_pen, _vid_plot, _vid_line, _vid_bar, _vid_clear, _vid_text, _vera_read

X1              := ptr1         ; The kernel's arguments
Y1              := ptr2
X2              := ptr3
Y2              := ptr4

; ****************************************************************************
; The header: the signature, the mode, the jump table

            .data
            .export     _hydra_tgi
_hydra_tgi:
            .byte       $74, $67, $69                       ; "tgi"
            .byte       TGI_API_VERSION
            .addr       $0000                               ; (The library reference)
            .word       320                                 ; X resolution
            .word       240                                 ; Y resolution
            .byte       <$0100                              ; Colours: 256
            .byte       1                                   ; Screens
            .byte       8                                   ; The system font's width ...
            .byte       8                                   ;   and height
            .word       $0100                               ; Aspect ratio
            .byte       0                                   ; Flags
            .addr       INSTALL
            .addr       UNINSTALL
            .addr       INIT
            .addr       DONE
            .addr       GETERROR
            .addr       CONTROL
            .addr       CLEAR
            .addr       SETVIEWPAGE
            .addr       SETDRAWPAGE
            .addr       SETCOLOR
            .addr       SETPALETTE
            .addr       GETPALETTE
            .addr       GETDEFPALETTE
            .addr       SETPIXEL
            .addr       GETPIXEL
            .addr       LINE
            .addr       BAR
            .addr       TEXTSTYLE
            .addr       OUTTEXT

error:      .byte       TGI_ERR_OK

            .bss
defpalette: .res        256                                 ; The identity
palette:    .res        256                                 ; TGI's colours: the palette's entries
color:      .res        1                                   ; The colour drawn in (TGI's)
pixel:      .res        1                                   ; GETPIXEL's

            .code

; INSTALL: the palettes, the identity; UNINSTALL: nothing
INSTALL:
            ldx         #0
:
            txa
            sta         defpalette,x
            sta         palette,x
            inx
            bne         :-
            stz         color
UNINSTALL:
            rts

; INIT: the bitmap, 320 across, 8 bits a pixel (no Vera X: TGI_ERR_INV_MODE); DONE: it gone
INIT:
            stz         error
            lda         #<320
            ldx         #>320
            jsr         pushax
            lda         #8
            ldx         #0
            jsr         _vid_bitmap
            cpx         #$FF
            bne         :+
            lda         #TGI_ERR_INV_MODE
            sta         error
            rts
:
            lda         color
            jmp         SETCOLOR

DONE:
            lda         #0
            tax
            jsr         pushax
            lda         #0
            tax
            jmp         _vid_bitmap

GETERROR:
            lda         error
            stz         error
            rts

CONTROL:
            lda         #TGI_ERR_INV_FUNC
            sta         error
            rts

; CLEAR: the bitmap in colour 0
CLEAR:
            jsr         _vid_clear
            jmp         check

SETVIEWPAGE:
SETDRAWPAGE:
            rts

; SETPALETTE: TGI's colours' entries (256 bytes at ptr1), then the colour again
SETPALETTE:
            stz         error
            ldy         #0
:
            lda         (ptr1),y
            sta         palette,y
            iny
            bne         :-
            lda         color

; SETCOLOR: TGI's colour .A, the pen its entry
SETCOLOR:
            sta         color
            tax
            lda         palette,x
            ldx         #0
            jsr         _vid_pen
            jmp         check

GETPALETTE:
            lda         #<palette
            ldx         #>palette
            rts

GETDEFPALETTE:
            lda         #<defpalette
            ldx         #>defpalette
            rts

; SETPIXEL: (X1, Y1)
SETPIXEL:
            lda         X1
            ldx         X1 + 1
            jsr         pushax
            lda         Y1
            ldx         Y1 + 1
            jsr         _vid_plot
            jmp         check

; GETPIXEL: (X1, Y1)'s byte, from VRAM (Y1 * 320 + X1: Y1 * 256 + Y1 * 64)
GETPIXEL:
            lda         Y1                                  ; tmp1/tmp2/sreg: Y1 * 64 ...
            sta         tmp1
            lda         Y1 + 1
            sta         tmp2
            stz         sreg
            ldx         #6
:
            asl         tmp1
            rol         tmp2
            rol         sreg
            dex
            bne         :-
            clc                                             ;   + Y1 * 256 ...
            lda         tmp2
            adc         Y1
            sta         tmp2
            lda         sreg
            adc         Y1 + 1
            sta         sreg
            clc                                             ;   + X1
            lda         tmp1
            adc         X1
            sta         tmp1
            lda         tmp2
            adc         X1 + 1
            sta         tmp2
            bcc         :+
            inc         sreg
:
            stz         sreg + 1
            lda         tmp1
            ldx         tmp2
            jsr         pusheax                             ; vera_read (addr, &pixel, 1)
            lda         #<pixel
            ldx         #>pixel
            jsr         pushax
            lda         #1
            ldx         #0
            jsr         _vera_read
            lda         pixel
            ldx         #0
            rts

; LINE: (X1, Y1) to (X2, Y2); BAR: their box, filled
LINE:
            jsr         four
            jsr         _vid_line
            jmp         check

BAR:
            jsr         four
            jsr         _vid_bar
            jmp         check

; X1, Y1 and X2 pushed, Y2 in .A/.X: four ints' call
four:
            lda         X1
            ldx         X1 + 1
            jsr         pushax
            lda         Y1
            ldx         Y1 + 1
            jsr         pushax
            lda         X2
            ldx         X2 + 1
            jsr         pushax
            lda         Y2
            ldx         Y2 + 1
            rts

; TEXTSTYLE: the font's size and direction (one size, across: as they are)
TEXTSTYLE:
            rts

; OUTTEXT: the string at ptr3, at (X1, Y1), in the console's font
OUTTEXT:
            lda         X1
            ldx         X1 + 1
            jsr         pushax
            lda         Y1
            ldx         Y1 + 1
            jsr         pushax
            lda         ptr3
            ldx         ptr3 + 1
            jmp         _vid_text

; A vid_ call's value (.A/.X): -1, TGI_ERR_INV_ARG
check:
            cpx         #$FF
            bne         :+
            cmp         #$FF
            bne         :+
            lda         #TGI_ERR_INV_ARG
            sta         error
:
            rts
