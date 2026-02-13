;
;   Taken mostly from Steve Wozniak's Apple 1 Monitor for the 6502, or WOZMON
;
.debuginfo
.zeropage
ZP_WM_ST:
    .res 2      ; STore address
ZP_WM_HVP:
    .res 2      ; Hex Value Parsing
ZP_WM_MODE:
    .res 1      ; $00=ZP_XAM, $7F=STOR, $AE=BLOCK ZP_XAM
ZP_WM_DASTATE:
    .res 1

.segment "BUFFERS"
IN:
                .res            $100

.segment "WOZMON"
; WOZMON Entrypoint
MON_START:
                cld                             ; Clear decimal arithmetic mode.
                cli                             ; Enable interrupts
                stz             ZP_WM_DASTATE
                bra             @is_start

@not_cr:
                cmp             #ASCII_BACKSPACE
                beq             @is_backspace
                cmp             #ASCII_ESC
                beq             @is_escape
                iny                             ; Advance text index.
                bpl             @get_next_char  ; Auto ESC if line longer than 127.

@is_escape:
                PRINT_CHAR      #ASCII_BACKSLASH
@is_start:
                PRINT_CRLF

@get_line:
                jsr             WRITE_PROMPT
                ldy             #1              ; Initialize text index.

@is_backspace:
                dey                     ; Back up text index.
                bmi             @get_line       ; Beyond start of line, reinitialize.

@get_next_char:
                jsr             READ_CHAR
                bcc             @get_next_char
                sta             IN,y            ; Add to text buffer.
                cmp             #ASCII_CR
                bne             @not_cr
                ldy             #$FF            ; Reset text index.  Will iny shortly...
                lda             #$00            ; For ZP_XAM mode.
                tax                             ; .X=0.

@set_block:
                asl

@set_store:
                asl                             ; Leaves $7B if setting STOR mode.

@set_mode:
                sta             ZP_WM_MODE      ; $00 = ZP_XAM, $74 = STOR, $B8 = BLOK ZP_XAM.

@skip_delim:
                iny                             ; Advance text index.

@next_item:
                lda             IN,y            ; Get character.
                cmp             #ASCII_CR       ; CR?
                beq             @get_line       ; Yes, done with this line.
                cmp             #ASCII_PERIOD
                bcc             @skip_delim     ; Skip delimiter.
                beq             @set_block      ; Set BLOCK ZP_XAM mode.
                cmp             #ASCII_COLON
                beq             @set_store      ; Yes, set STOR mode.
                cmp             #ASCII_L
                beq             @disassemble    ; Disassemble 1 or more instructions at current address/range
                cmp             #ASCII_R
                beq             @run_prog       ; Yes, run user program
                cmp             #ASCII_T        ; T, U, V, or W registers?
                bcc             @not_tuvw       ;
                cmp             #ASCII_X        ;
                bcs             @not_tuvw       ;
                adc             #($F0-ASCII_T)  ; T=FFF0, U=FFF1, V=FFF2, W=FFF3
                sta             ZP_WM_HVP       ;
                lda             #$FF            ;
                sta             ZP_WM_HVP + 1   ;
                iny                             ; skip the mnemonic
                bra             @not_hex_or_escape

@not_tuvw:
                sty             ZP_Y_SAVE       ; Save Y for comparison
                stx             ZP_WM_HVP       ; $00 -> Low byte of HPV
                stx             ZP_WM_HVP + 1   ; ...and High byte.

@next_hex:
                lda             IN,y            ; Get character for hex test.
                eor             #ASCII_0        ; Map digits to $0-9.
                cmp             #10             ; Digit?
                bcc             @is_digit       ; Yes.
                adc             #$88            ; Map letter "A"-"F" to $FA-FF.
                cmp             #$FA            ; Hex letter?
                bcc             @not_hex        ; No, character not hex.

@is_digit:
                asl                             ; LSD to MSD of A.
                asl
                asl
                asl
                ldx             #4              ; Shift count.

@hex_shift:
                asl                             ; Hex digit left, MSB to carry.
                rol             ZP_WM_HVP       ; Rotate into LSD.
                rol             ZP_WM_HVP + 1   ; Rotate into MSD's.
                dex                             ; Done 4 shifts?
                bne             @hex_shift      ; No, loop.
                iny                             ; Advance text index.
                bne             @next_hex       ; Always taken. Check next character for hex.

@not_hex:
                cpy             ZP_Y_SAVE       ; Check if HPV empty (no hex digits).
                beq             @is_escape      ; Yes, generate ESC sequence.

@not_hex_or_escape:
                bit             ZP_WM_MODE      ; Test ZP_WM_MODE byte.
                bvc             @not_store      ; B6=0 is STOR, 1 is ZP_XAM and BLOCK ZP_XAM.
                lda             ZP_WM_HVP       ; LSD's of hex data.
                sta             (ZP_WM_ST)      ; Store to current 'store index'.
                inc             ZP_WM_ST        ; Increment store index.
                bne             @next_item      ; Get next item (no carry).
                inc             ZP_WM_ST + 1    ; Add carry to 'store index' high order.

@to_next_item:
                bra             @next_item      ; Get next command item.

@run_prog:
                _M_JSRR         ZP_XAM, MON_START

@disassemble:
                lda             #1
                sta             ZP_WM_DASTATE

@not_store:
                bmi             @examine_next   ; B7 = 0 for ZP_XAM, 1 for BLOCK ZP_XAM.
                ldx             #2              ; Byte count.

@set_addr:
                lda             ZP_WM_HVP - 1,x ; Copy hex data to
                sta             ZP_WM_ST - 1,x  ;   'store index'.
                sta             ZP_XAM - 1,x    ; And to 'ZP_XAM index'.
                dex                             ; Next of 2 bytes.
                bne             @set_addr       ; Loop unless X = 0.

@print_next:
                bne             @print_data     ; NE means no address to print.
                PRINT_CRLF
                PRINT_BYTE      ZP_XAM + 1      ; Print 'examine index' high-order byte.
                PRINT_BYTE      ZP_XAM          ; Print 'examine index' low-order byte.
                PRINT_CHAR      #ASCII_COLON    ; Print a ':'.

@print_data:
                lda             ZP_WM_DASTATE
                beq             @print_bytes
                jsr             DISASM
                bra             @examine_next

@print_bytes:
                PRINT_SPACE                     ; Print a ' '.
                PRINT_BYTE      {(ZP_XAM,x)}    ; Print the byte at 'examine index'.

@examine_next:
                stx             ZP_WM_MODE      ; 0 -> ZP_WM_MODE (ZP_XAM mode).
                lda             ZP_XAM
                cmp             ZP_WM_HVP       ; Compare 'examine index' to hex data.
                lda             ZP_XAM + 1
                sbc             ZP_WM_HVP + 1
                bcs             @to_next_item   ; Not less, so no more data to output.
                inc             ZP_XAM
                bne             @mod_8_check    ; Increment 'examine index'.
                inc             ZP_XAM + 1

@mod_8_check:
                lda             ZP_XAM          ; Check low-order 'examine index' byte
                and             #7              ; For MOD 8 = 0
                bpl             @print_next     ; Always taken.
