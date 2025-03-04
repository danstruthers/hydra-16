.segment "WOZMON"

XAML            = ZP_LAST_USED + 1      ; Last "opened" location Low
XAMH            = XAML + 1              ; Last "opened" location High
STL             = XAML + 2              ; Store address Low
STH             = XAML + 3              ; Store address High
L               = XAML + 4              ; Hex value parsing Low
H               = XAML + 5              ; Hex value parsing High
MODE            = XAML + 6              ; $00=XAM, $7F=STOR, $AE=BLOCK XAM

IN              = $7E00

; WOZMON Entrypoint
MON_START:
                cld                     ; Clear decimal arithmetic mode.
                cli                     ; Enable interrupts
                SJMP    @is_escape

@not_cr:
                cmp     #ASCII_BACKSPACE
                beq     @is_backspace
                cmp     #ASCII_ESC
                beq     @is_escape
                iny                     ; Advance text index.
                bpl     @get_next_char  ; Auto ESC if line longer than 127.

@is_escape:
                lda     #ASCII_BACKSLASH
                jsr     WRITECHAR

@get_line:
                jsr     WRITE_CRLF
                ldy     #1              ; Initialize text index.

@is_backspace:
                dey                     ; Back up text index.
                bmi     @get_line       ; Beyond start of line, reinitialize.

@get_next_char:
                jsr     READCHAR
                bcc     @get_next_char
                sta     IN,Y            ; Add to text buffer.
                cmp     #ASCII_CR
                bne     @not_cr
                ldy     #$FF            ; Reset text index.  Will iny shortly...
                lda     #$00            ; For XAM mode.
                tax                     ; X=0.

@set_block:
                asl

@set_store:
                asl                     ; Leaves $7B if setting STOR mode.

@set_mode:
                sta     MODE            ; $00 = XAM, $74 = STOR, $B8 = BLOK XAM.

@skip_delim:
                iny                     ; Advance text index.

@next_item:
                lda     IN,Y            ; Get character.
                cmp     #ASCII_CR       ; CR?
                beq     @get_line       ; Yes, done this line.
                cmp     #ASCII_PERIOD
                bcc     @skip_delim     ; Skip delimiter.
                beq     @set_block      ; Set BLOCK XAM mode.
                cmp     #ASCII_COLON
                beq     @set_store      ; Yes, set STOR mode.
                cmp     #ASCII_R
                beq     @run_prog       ; Yes, run user program.
                stx     L               ; $00 -> L.
                stx     H               ;    and H.
                sty     ZP_Y_SAVE       ; Save Y for comparison

@next_hex:
                lda     IN,Y            ; Get character for hex test.
                eor     #ASCII_0        ; Map digits to $0-9.
                cmp     #10             ; Digit?
                bcc     @is_digit       ; Yes.
                adc     #$88            ; Map letter "A"-"F" to $FA-FF.
                cmp     #$FA            ; Hex letter?
                bcc     @not_hex        ; No, character not hex.

@is_digit:
                asl                     ; LSD to MSD of A.
                asl
                asl
                asl
                ldx     #4              ; Shift count.

@hex_shift:
                asl                     ; Hex digit left, MSB to carry.
                rol     L               ; Rotate into LSD.
                rol     H               ; Rotate into MSD's.
                dex                     ; Done 4 shifts?
                bne     @hex_shift      ; No, loop.
                iny                     ; Advance text index.
                bne     @next_hex       ; Always taken. Check next character for hex.

@not_hex:
                cpy     ZP_Y_SAVE       ; Check if L, H empty (no hex digits).
                beq     @is_escape      ; Yes, generate ESC sequence.
                bit     MODE            ; Test MODE byte.
                bvc     @not_store      ; B6=0 is STOR, 1 is XAM and BLOCK XAM.
                lda     L               ; LSD's of hex data.
                sta     (STL,X)         ; Store current 'store index'.
                inc     STL             ; Increment store index.
                bne     @next_item      ; Get next item (no carry).
                inc     STH             ; Add carry to 'store index' high order.

@to_next_item:
                jmp     @next_item      ; Get next command item.

@run_prog:
                lda     #>MON_START
                pha
                lda     #<MON_START
                pha
                jmp     (XAML)

@not_store:
                bmi     @examine_next   ; B7 = 0 for XAM, 1 for BLOCK XAM.
                ldx     #2              ; Byte count.

@set_addr:
                lda     L-1,X           ; Copy hex data to
                sta     STL-1,X         ;  'store index'.
                sta     XAML-1,X        ; And to 'XAM index'.
                dex                     ; Next of 2 bytes.
                bne     @set_addr       ; Loop unless X = 0.

@print_next:
                bne     @print_data     ; NE means no address to print.
                jsr     WRITE_CRLF
                lda     XAMH            ; 'Examine index' high-order byte.
                jsr     WRITE_BYTE      ; Output it in hex format.
                lda     XAML            ; Low-order 'examine index' byte.
                jsr     WRITE_BYTE      ; Output it in hex format.
                lda     #ASCII_COLON
                jsr     WRITECHAR       ; Output it.

@print_data:
                lda     #ASCII_SPACE
                jsr     WRITECHAR       ; Output it.
                lda     (XAML,X)        ; Get data byte at 'examine index'.
                jsr     WRITE_BYTE      ; Output it in hex format.

@examine_next:
                stx     MODE            ; 0 -> MODE (XAM mode).
                lda     XAML
                cmp     L               ; Compare 'examine index' to hex data.
                lda     XAMH
                sbc     H
                bcs     @to_next_item   ; Not less, so no more data to output.
                inc     XAML
                bne     @mod_8_check    ; Increment 'examine index'.
                inc     XAMH

@mod_8_check:
                lda     XAML            ; Check low-order 'examine index' byte
                and     #7              ; For MOD 8 = 0
                bpl     @print_next     ; Always taken.
