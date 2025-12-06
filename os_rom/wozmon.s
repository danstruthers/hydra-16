.debuginfo
.segment "BUFFERS"
IN:
                .res 256

.segment "WOZMON"

; WOZMON Entrypoint
MON_START:
                cld                     ; Clear decimal arithmetic mode.
                cli                     ; Enable interrupts
                bra    @is_start

@not_cr:
                cmp     #ASCII_BACKSPACE
                beq     @is_backspace
                cmp     #ASCII_ESC
                beq     @is_escape
                iny                     ; Advance text index.
                bpl     @get_next_char  ; Auto ESC if line longer than 127.

@is_escape:
                PRINT_CHAR      #ASCII_BACKSLASH
@is_start:
                PRINT_CRLF

@get_line:
                jsr     WRITE_PROMPT
                ldy     #1              ; Initialize text index.

@is_backspace:
                dey                     ; Back up text index.
                bmi     @get_line       ; Beyond start of line, reinitialize.

@get_next_char:
                jsr     READ_CHAR
                bcc     @get_next_char
                sta     IN,y            ; Add to text buffer.
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
                lda     IN,y            ; Get character.
                cmp     #ASCII_CR       ; CR?
                beq     @get_line       ; Yes, done this line.
                cmp     #ASCII_PERIOD
                bcc     @skip_delim     ; Skip delimiter.
                beq     @set_block      ; Set BLOCK XAM mode.
                cmp     #ASCII_COLON
                beq     @set_store      ; Yes, set STOR mode.
                cmp     #ASCII_R
                beq     @run_prog       ; Yes, run user program
                cmp     #ASCII_T        ; T, U, V, or W registers?
                bcc     @not_tuvw       ;
                cmp     #ASCII_X        ;
                bcs     @not_tuvw       ;
                adc     #($F0-ASCII_T)  ; T=FFF0, U=FFF1, V=FFF2, W=FFF3
                sta     HVP             ;
                lda     #$FF            ;
                sta     HVP + 1         ;
                iny                     ; skip the mnemonic
                bra     @not_hex_or_escape

@not_tuvw:
                sty     ZP_Y_SAVE       ; Save Y for comparison
                stx     HVP             ; $00 -> L
                stx     HVP + 1         ; ...and H.

@next_hex:
                lda     IN,y            ; Get character for hex test.
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
                rol     HVP             ; Rotate into LSD.
                rol     HVP + 1         ; Rotate into MSD's.
                dex                     ; Done 4 shifts?
                bne     @hex_shift      ; No, loop.
                iny                     ; Advance text index.
                bne     @next_hex       ; Always taken. Check next character for hex.

@not_hex:
                cpy     ZP_Y_SAVE       ; Check if L, H empty (no hex digits).
                beq     @is_escape      ; Yes, generate ESC sequence.

@not_hex_or_escape:
                bit     MODE            ; Test MODE byte.
                bvc     @not_store      ; B6=0 is STOR, 1 is XAM and BLOCK XAM.
                lda     HVP             ; LSD's of hex data.
                sta     (ST)           ; Store current 'store index'.
                inc     ST              ; Increment store index.
                bne     @next_item      ; Get next item (no carry).
                inc     ST + 1          ; Add carry to 'store index' high order.

@to_next_item:
                jmp     @next_item      ; Get next command item.

@run_prog:
                JSRR    XAM, MON_START

@not_store:
                bmi     @examine_next   ; B7 = 0 for XAM, 1 for BLOCK XAM.
                ldx     #2              ; Byte count.

@set_addr:
                lda     HVP - 1,x       ; Copy hex data to
                sta     ST - 1,x        ;  'store index'.
                sta     XAM - 1,x       ; And to 'XAM index'.
                dex                     ; Next of 2 bytes.
                bne     @set_addr       ; Loop unless X = 0.

@print_next:
                bne     @print_data     ; NE means no address to print.
                PRINT_CRLF
                PRINT_BYTE  XAM + 1     ; Print 'Examine index' high-order byte.
                PRINT_BYTE  XAM         ; Print 'Examine index' low-order byte.
                PRINT_CHAR  #ASCII_COLON; Print a ':'.

@print_data:
                PRINT_CHAR  #ASCII_SPACE; Print a ' '.
                PRINT_BYTE  {(XAM,x)}  ; Print the byte at 'examine index'.

@examine_next:
                stx     MODE            ; 0 -> MODE (XAM mode).
                lda     XAM
                cmp     HVP             ; Compare 'examine index' to hex data.
                lda     XAM + 1
                sbc     HVP + 1
                bcs     @to_next_item   ; Not less, so no more data to output.
                inc     XAM
                bne     @mod_8_check    ; Increment 'examine index'.
                inc     XAM + 1

@mod_8_check:
                lda     XAM             ; Check low-order 'examine index' byte
                and     #7              ; For MOD 8 = 0
                bpl     @print_next     ; Always taken.
