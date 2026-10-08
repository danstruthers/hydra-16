;
; _printf: Basic layer for all printf type functions.
;
; Ullrich von Bassewitz, 2000-10-21
;
; Altered for the Hydra-16 (the C SDK, sdk/c/README.md): the Hydra's numbers (num.h), written by the numbers
; library (DISPLAY).  %N, a number (a num_t*: its bytes in the stored format), in the base; and a base in braces
; right after the %, for %N and C's integers (d i u o x X) alike: %{x}N, %{#b}d, %{c}ld, %{16r}u; %{}N the base;
; %{*}N, the base a string from the arguments (before a * width's).  The width, - and 0 pad the whole text, a + or a
; space is put before a number not below 0, and a precision is passed over (a number is written whole).  Without
; braces C's own conversions are cc65's.  The text is written into printf's bank (numcall.s's: 8K at most) and
; out from there 20 bytes at a time, so a number of any length is written; its number must be below $8000.  A
; number that can't be written (a base that isn't one, bytes that aren't a number): nothing written for it.

        .include        "zeropage.inc"
        .include        "hydra.inc"
        .include        "numbers.inc"

        .export         __printf

        .import         popax, pushax, pusheax, decsp6, push1, axlong, axulong
        .import         _ltoa, _ultoa
        .import         _strlower, _strlen
        .import         __num_ready, __num_xcall, __num_entry, __num_tbank

RAM_BANK        = $00                   ; The bank at $8000-$9FFF (hydra.h's hy_bank)
BASE_ROOM       = 96                    ; A base string at most, its 0 too (the library's BASE_MAX)

        .macpack        generic
        .macpack        cpu

; ----------------------------------------------------------------------------
; We will store variables into the register bank in the zeropage. Define
; equates for these variables.

ArgList         = regbank+0             ; Argument list pointer
Format          = regbank+2             ; Format string
OutData         = regbank+4             ; Function parameters

; ----------------------------------------------------------------------------
; Other zero page cells

Base            = ptr1
FSave           = ptr1
FCount          = ptr2

.code

; ----------------------------------------------------------------------------
; Get one character from the format string, and increment the pointer. Will
; return zero in .Y.

GetFormatChar:
        ldy     #0
        .if (.cpu .bitand ::CPU_ISET_65SC02)
        lda     (Format)
        .else
        lda     (Format),y
        .endif
IncFormatPtr:
        inc     Format
        bne     @L1
        inc     Format+1
@L1:    rts

; ----------------------------------------------------------------------------
; Output a pad character: outfunc (d, &padchar, 1)

OutputPadChar:
        lda     PadChar

; ----------------------------------------------------------------------------
; Call the output function with one character in .A

Output1:
        sta     CharArg
        jsr     PushOutData
        lda     #<CharArg
        ldx     #>CharArg
        jsr     pushax
        jsr     push1
        jmp     CallOutFunc     ; fout (OutData, &CharArg, 1)

; ----------------------------------------------------------------------------
; Decrement the argument list pointer by 2

DecArgList2:
        lda     ArgList
        sub     #2
        sta     ArgList
        bcs     @L1
        dec     ArgList+1
@L1:    rts

; ----------------------------------------------------------------------------
; Get an unsigned int or long argument depending on the IsLong flag.

GetUnsignedArg:
        lda     IsLong                  ; Check flag
        bne     GetLongArg              ; Long sets all
        jsr     GetIntArg               ; Get an integer argument
        jmp     axulong                 ; Convert to unsigned long

; ----------------------------------------------------------------------------
; Get an signed int or long argument depending on the IsLong flag.

GetSignedArg:
        lda     IsLong                  ; Check flag
        bne     GetLongArg              ; Long sets all
        jsr     GetIntArg               ; Get an integer argument
        jmp     axlong                  ; Convert to long

; ----------------------------------------------------------------------------
; Get a long argument from the argument list. Returns 0 in .Y.

GetLongArg:
        jsr     GetIntArg               ; Get high word
        sta     sreg
        stx     sreg+1

; Run into GetIntArg fetching the low word

; ----------------------------------------------------------------------------
; Get an integer argument from the argument list. Returns 0 in .Y.

GetIntArg:
        jsr     DecArgList2
        ldy     #1
        lda     (ArgList),y
        tax
        dey
        .if (.cpu .bitand ::CPU_ISET_65SC02)
        lda     (ArgList)
        .else
        lda     (ArgList),y
        .endif
        rts

; ----------------------------------------------------------------------------
; Read an integer from the format string. Will return zero in .Y.

ReadInt:
        ldy     #0
        sty     ptr1
        sty     ptr1+1                  ; Start with zero
@Loop:  lda     (Format),y              ; Get format string character
        sub     #'0'                    ; Make number from ascii digit
        bcc     @L9                     ; Jump if done
        cmp     #9+1
        bcs     @L9                     ; Jump if done

; Skip the digit character

        jsr     IncFormatPtr

; Add the digit to the value we have in ptr1

        pha                             ; Save digit value
        lda     ptr1
        ldx     ptr1+1
        asl     a
        rol     ptr1+1                  ; * 2
        asl     a
        rol     ptr1+1                  ; * 4, assume carry clear
        adc     ptr1
        sta     ptr1
        txa
        adc     ptr1+1
        sta     ptr1+1                  ; * 5
        asl     ptr1
        rol     ptr1+1                  ; * 10, assume carry clear
        pla
        adc     ptr1                    ; Add digit value
        sta     ptr1
        bcc     @Loop
        inc     ptr1+1
        bcs     @Loop                   ; Branch always

; We're done converting

@L9:    lda     ptr1
        ldx     ptr1+1                  ; Load result
        rts


; ----------------------------------------------------------------------------
; Put a character into the argument buffer and increment the buffer index

PutBuf: ldy     BufIdx
        inc     BufIdx
        sta     Buf,y
        rts

; ----------------------------------------------------------------------------
; Get a pointer to the current buffer end and push it onto the stack

PushBufPtr:
        lda     #<Buf
        ldx     #>Buf
        add     BufIdx
        bcc     @L1
        inx
@L1:    jmp     pushax

; ----------------------------------------------------------------------------
; Push OutData onto the software stack

PushOutData:
        lda     OutData
        ldx     OutData+1
        jmp     pushax

; ----------------------------------------------------------------------------
; Output Width pad characters
;

PadLoop:
        jsr     OutputPadChar
OutputPadding:
        inc     Width
        bne     PadLoop
        inc     Width+1
        bne     PadLoop
        rts

; ----------------------------------------------------------------------------
; Output the argument itself: outfunc (d, str, arglen);
;

OutputArg:
        jsr     PushOutData
        lda     Str
        ldx     Str+1
        jsr     pushax
        lda     ArgLen
        ldx     ArgLen+1
        jsr     pushax
        jmp     CallOutFunc

; ----------------------------------------------------------------------------
; ltoa: Wrapper for _ltoa that pushes all arguments

ltoa:   sty     Base                    ; Save base
        jsr     pusheax                 ; Push value
        jsr     PushBufPtr              ; Push the buffer pointer...
        lda     Base                    ; Restore base
        jmp     _ltoa                   ; ultoa (l, s, base);


; ----------------------------------------------------------------------------
; ultoa: Wrapper for _ultoa that pushes all arguments

ultoa:  sty     Base                    ; Save base
        jsr     pusheax                 ; Push value
        jsr     PushBufPtr              ; Push the buffer pointer...
        lda     Base                    ; Restore base
        jmp     _ultoa                  ; ultoa (l, s, base);


; ----------------------------------------------------------------------------
;

__printf:

; Save the register bank variables into the save area

        pha                             ; Save low byte of ap
        ldy     #5
Save:   lda     regbank,y
        sta     RegSave,y
        dey
        bpl     Save
        pla                             ; Restore low byte of ap

; Get the parameters from the stack

        sta     ArgList                 ; Argument list pointer
        stx     ArgList+1

        jsr     popax                   ; Format string
        sta     Format
        stx     Format+1

        jsr     popax                   ; Output descriptor
        sta     OutData
        stx     OutData+1

; Initialize the output counter in the output descriptor to zero

        lda     #0
        .if (.cpu .bitand ::CPU_ISET_65SC02)
        sta     (OutData)
        ldy     #$01
        sta     (OutData),y
        .else
        tay
        sta     (OutData),y
        iny
        sta     (OutData),y
        .endif

; Get the output function from the output descriptor and remember it

        iny
        lda     (OutData),y
        sta     CallOutFunc+1
        iny
        lda     (OutData),y
        sta     CallOutFunc+2

; Start parsing the format string

MainLoop:
        lda     Format                  ; Remember current format pointer
        sta     FSave
        lda     Format+1
        sta     FSave+1

        ldy     #0                      ; Index
@L1:    lda     (Format),y              ; Get next char
        beq     @L2                     ; Jump on end of string
        cmp     #'%'                    ; Format spec?
        beq     @L2
        iny                             ; Bump pointer
        bne     @L1
        inc     Format+1                ; Bump high byte of pointer
        bne     @L1                     ; Branch always

; Found a '%' character or end of string. Update the Format pointer so it is
; current (points to this character).

@L2:    tya                             ; Low byte of offset
        add     Format
        sta     Format
        bcc     @L3
        inc     Format+1

; Calculate, how many characters must be output. Beware: This number may
; be zero. .A still contains the low byte of the pointer.

@L3:    sub     FSave
        sta     FCount
        lda     Format+1
        sbc     FSave+1
        sta     FCount+1
        ora     FCount                  ; Is the result zero?
        beq     @L4                     ; Jump if yes

; Output the characters that we have until now. To make the call to out
; faster, build the stack frame by hand (don't use pushax)

        jsr     decsp6                  ; 3 args
        ldy     #5
        lda     OutData+1
        sta     (sp),y
        dey
        lda     OutData
        sta     (sp),y
        dey
        lda     FSave+1
        sta     (sp),y
        dey
        lda     FSave
        sta     (sp),y
        dey
        lda     FCount+1
        sta     (sp),y
        dey
        lda     FCount
        .if (.cpu .bitand ::CPU_ISET_65SC02)
        sta     (sp)
        .else
        sta     (sp),y
        .endif
        jsr     CallOutFunc             ; Call the output function

; We're back from out(), or we didn't call it. Check for end of string.

@L4:    jsr     GetFormatChar           ; Get one char, zero in .Y
        tax                             ; End of format string reached?
        bne     NotDone                 ; End not reached

; End of format string reached. Restore the zeropage registers and return.

        ldx     #5
Rest:   lda     RegSave,x
        sta     regbank,x
        dex
        bpl     Rest
        rts

; Still a valid format character. Check for '%' and a '%%' sequence. Output
; anything that is not a format specifier. On intro, .Y is zero.

NotDone:
        cmp     #'%'
        bne     @L1
        lda     (Format),y              ; Check for "%%"
        cmp     #'%'
        bne     FormatSpec              ; Jump if really a format specifier
        jsr     IncFormatPtr            ; Skip the second '%'
@L1:    jsr     Output1                 ; Output the character...
        jmp     MainLoop                ; ...and continue

; We have a real format specifier
; Format is: %[flags][width][.precision][mod]type
; .Y is zero on entry.

FormatSpec:

; Initialize the flags

        lda     #0
        ldx     #FormatVarSize-1
@L1:    sta     FormatVars,x
        dex
        bpl     @L1

; A base in braces (the Hydra's)

        lda     (Format),y
        cmp     #'{'
        bne     ReadFlags
        jsr     ReadBase                ; (.X $FF and .Y 0 again)

; Start with reading the flags if there are any. .X is $FF which is used
; for "true"

ReadFlags:
        lda     (Format),y              ; Get next char...
        cmp     #'-'
        bne     @L1
        stx     LeftJust
        beq     @L4

@L1:    cmp     #'+'
        bne     @L2
        stx     AddSign
        beq     @L4

@L2:    cmp     #' '
        bne     @L3
        stx     AddBlank
        beq     @L4

@L3:    cmp     #'#'
        bne     ReadPadding
        stx     AltForm

@L4:    jsr     IncFormatPtr
        jmp     ReadFlags               ; ...and start over

; Done with flags, read the pad char. .Y is still zero if we come here.

ReadPadding:
        ldx     #' '                    ; PadChar
        cmp     #'0'
        bne     @L1
        tax                             ; PadChar is '0'
        jsr     IncFormatPtr
        lda     (Format),y              ; Read current for later
@L1:    stx     PadChar

; Read the width. Even here, .Y is still zero. .A contains the current character
; from the format string.

ReadWidth:
        cmp     #'*'
        bne     @L1
        jsr     IncFormatPtr
        jsr     GetIntArg               ; Width is an additional argument
        jmp     @L2

@L1:    jsr     ReadInt                 ; Read integer from format string...
@L2:    sta     Width
        stx     Width+1                 ; ...and remember in Width

; Read the precision. Even here, .Y is still zero.

        sty     Prec                    ; Assume Precision is zero
        sty     Prec+1
        lda     (Format),y              ; Load next format string char
        cmp     #'.'                    ; Precision given?
        bne     ReadMod                 ; Branch if no precision given

ReadPrec:
        jsr     IncFormatPtr            ; Skip the '.'
        lda     (Format),y
        cmp     #'*'                    ; Variable precision?
        bne     @L1
        jsr     IncFormatPtr            ; Skip the '*'
        jsr     GetIntArg               ; Get integer argument
        jmp     @L2

@L1:    jsr     ReadInt                 ; Read integer from format string
@L2:    sta     Prec
        stx     Prec+1

; Read the modifiers. .Y is still zero.

ReadMod:
        lda     (Format),y
        cmp     #'z'                    ; size_t - same as unsigned
        beq     @L2
        cmp     #'h'                    ; short - same as int
        beq     @L2
        cmp     #'t'                    ; ptrdiff_t - same as int
        beq     @L2
        cmp     #'j'                    ; intmax_t/uintmax_t - same as long
        beq     @L1
        cmp     #'L'                    ; long double
        beq     @L1
        cmp     #'l'                    ; long int
        bne     DoFormat
@L1:    lda     #$FF
        sta     IsLong
@L2:    jsr     IncFormatPtr
        jmp     ReadMod

; Initialize the argument buffer pointers. We use a static buffer (ArgBuf) to
; assemble strings. A zero page index (BufIdx) is used to keep the current
; write position. A pointer to the buffer (Str) is used to point to the
; argument in case we will not use the buffer but a user-supplied string.
; .Y is zero when we come here.

DoFormat:
        sty     BufIdx                  ; Clear BufIdx
        ldx     #<Buf
        stx     Str
        ldx     #>Buf
        stx     Str+1

; Skip the current format character, then check it (current char in .A)

        jsr     IncFormatPtr

; A number (the Hydra's), or an integer in a base in braces?

        cmp     #'N'
        bne     @N1
        jmp     NumArg
@N1:    ldx     HaveBase
        beq     @N3
        cmp     #'d'
        beq     @N2
        cmp     #'i'
        bne     @N4
@N2:    jmp     IntSigned
@N4:    cmp     #'u'
        beq     @N5
        cmp     #'o'
        beq     @N5
        cmp     #'x'
        beq     @N5
        cmp     #'X'
        bne     @N3
@N5:    jmp     IntUnsigned
@N3:

; Is it a character?

        cmp     #'c'
        bne     CheckInt

; It is a character

        jsr     GetIntArg               ; Get the argument (promoted to int)
        sta     Buf                     ; Place it into the buffer
        ldx     #0
        lda     #1                      ; Buffer length is 1
        jmp     HaveArg1

; Is it an integer?

CheckInt:
        cmp     #'d'
        beq     @L1
        cmp     #'i'
        bne     CheckCount

; It is an integer

@L1:    ldx     #0
        lda     AddBlank                ; Add a blank for positives?
        beq     @L2                     ; Jump if no
        ldx     #' '
@L2:    lda     AddSign                 ; Add a plus for positives (precedence)?
        beq     @L3
        ldx     #'+'
@L3:    stx     Leader

; Integer argument

        jsr     GetSignedArg            ; Get argument as a long
        ldy     sreg+1                  ; Check sign
        bmi     @Int1
        ldy     Leader
        beq     @Int1
        sty     Buf
        inc     BufIdx

@Int1:  ldy     #10                     ; Base
        jsr     ltoa                    ; Push arguments, call _ltoa
        jmp     HaveArg

; Is it a count pseudo format?

CheckCount:
        cmp     #'n'
        bne     CheckOctal

; It is a count pseudo argument

        jsr     GetIntArg
        sta     ptr1
        stx     ptr1+1                  ; Get user supplied pointer
        .if (.cpu .bitand ::CPU_ISET_65SC02)
        lda     (OutData)             ; Low byte of OutData->ccount
        sta     (ptr1)
        ldy     #1
        .else
        ldy     #0
        lda     (OutData),y             ; Low byte of OutData->ccount
        sta     (ptr1),y
        iny
        .endif
        lda     (OutData),y             ; High byte of OutData->ccount
        sta     (ptr1),y
        jmp     MainLoop                ; Done

; Check for an octal digit

CheckOctal:
        cmp     #'o'
        bne     CheckPointer

; Integer in octal representation

        jsr     GetSignedArg            ; Get argument as a long
        ldy     AltForm                 ; Alternative form?
        beq     @Oct1                   ; Jump if no
        pha                             ; Save low byte of value
        stx     tmp1
        ora     tmp1
        ora     sreg
        ora     sreg+1
        ora     Prec
        ora     Prec+1                  ; Check if value or Prec != 0
        beq     @Oct1
        lda     #'0'
        jsr     PutBuf
        pla                             ; Restore low byte

@Oct1:  ldy     #8                      ; Load base
        jsr     ltoa                    ; Push arguments, call _ltoa
        jmp     HaveArg

; Check for a pointer specifier (%p)

CheckPointer:
        cmp     #'p'
        bne     CheckString

; It's a pointer. Use %#x conversion

        ldx     #0
        stx     IsLong                  ; IsLong = 0;
        inx
        stx     AltForm                 ; AltForm = 1;
        lda     #'x'
        bne     IsHex                   ; Branch always

; Check for a string specifier (%s)

CheckString:
        cmp     #'s'
        bne     CheckUnsigned

; It's a string

        jsr     GetIntArg               ; Get 16bit argument
        sta     Str
        stx     Str+1
        jmp     HaveArg

; Check for an unsigned integer (%u)

CheckUnsigned:
        cmp     #'u'
        bne     CheckHex

; It's an unsigned integer

        jsr     GetUnsignedArg          ; Get argument as unsigned long
        ldy     #10                     ; Load base
        jsr     ultoa                   ; Push arguments, call _ultoa
        jmp     HaveArg

; Check for a hexadecimal integer (%x)

CheckHex:
        cmp     #'x'
        beq     IsHex
        cmp     #'X'
        bne     UnknownFormat

; Hexadecimal integer

IsHex:  pha                             ; Save the format spec
        lda     AltForm
        beq     @L1
        lda     #'0'
        jsr     PutBuf
        lda     #'X'
        jsr     PutBuf

@L1:    jsr     GetUnsignedArg          ; Get argument as an unsigned long
        ldy     #16                     ; Load base
        jsr     ultoa                   ; Push arguments, call _ultoa

        pla                             ; Get the format spec
        cmp     #'x'                    ; Lower case?
        bne     @L2
        lda     Str
        ldx     Str+1
        jsr     _strlower               ; Make characters lower case
@L2:    jmp     HaveArg

; Unknown format character, skip it

UnknownFormat:
        jmp     MainLoop

; We have the argument, do argument string formatting

HaveArg:

; ArgLen = strlen (Str);

        lda     Str
        ldx     Str+1
        jsr     _strlen                 ; Get length of argument
HaveArg1:                               ; Jumped into here from %c handling
        sta     ArgLen
        stx     ArgLen+1

; if (Prec && Prec < ArgLen) ArgLen = Prec;

        lda     Prec
        ora     Prec+1
        beq     @L1
        ldx     Prec
        cpx     ArgLen
        lda     Prec+1
        tay
        sbc     ArgLen+1
        bcs     @L1
        stx     ArgLen
        sty     ArgLen+1

;  if (Width > ArgLen) {
;      Width -= ArgLen;                 /* padcount */
;  } else {
;      Width = 0;
;  }
; Since width is used as a counter below, calculate -(width+1)

@L1:    sec
        lda     Width
        sbc     ArgLen
        tax
        lda     Width+1
        sbc     ArgLen+1
        bcs     @L2
        lda     #0
        tax
@L2:    eor     #$FF
        sta     Width+1
        txa
        eor     #$FF
        sta     Width

;  /* Do padding on the left side if needed */
;  if (!leftjust) {
;      /* argument right justified */
;      while (width) {
;         fout (d, &padchar, 1);
;         --width;
;      }
;  }

        lda     LeftJust
        bne     @L3
        jsr     OutputPadding

; Output the argument itself

@L3:    jsr     OutputArg

;  /* Output right padding bytes if needed */
;  if (leftjust) {
;      /* argument left justified */
;      while (width) {
;         fout (d, &padchar, 1);
;         --width;
;      }
;  }

        lda     LeftJust
        beq     @L4
        jsr     OutputPadding

; Done, parse next chars from format string

@L4:    jmp     MainLoop

; ----------------------------------------------------------------------------
; The Hydra's numbers (num.h)

; A base in braces, Format at its {: past the }, HaveBase $FF and BaseAt its string (0: the base, {}).  {*}: the
; string's address from the arguments.  A string longer than BASE_ROOM - 1 is cut (not a base); with no }, the
; rest of the format is.  OUT: .X = $FF, .Y = 0 (ReadFlags')

ReadBase:
        jsr     IncFormatPtr            ; (Past the {)
        dec     HaveBase
        ldy     #0
        lda     (Format),y
        cmp     #'*'
        bne     @L1
        iny
        lda     (Format),y
        cmp     #'}'
        bne     @L1
        jsr     IncFormatPtr            ; {*}
        jsr     IncFormatPtr
        jsr     GetIntArg
        sta     BaseAt
        stx     BaseAt+1
        bra     @L5

@L1:    ldx     #0                      ; Its characters, to its }
@L2:    ldy     #0
        lda     (Format),y
        beq     @L4                     ; (The format's end)
        jsr     IncFormatPtr
        cmp     #'}'
        beq     @L4
        cpx     #BASE_ROOM - 1
        bcs     @L2                     ; (Too long: cut)
        sta     BaseBuf,x
        inx
        bra     @L2
@L4:    stz     BaseBuf,x
        txa
        beq     @L5                     ; ({}: the base)
        lda     #<BaseBuf
        sta     BaseAt
        lda     #>BaseBuf
        sta     BaseAt+1
@L5:    ldx     #$FF
        ldy     #0
        rts

; %N: the number at the argument

NumArg: jsr     GetIntArg
        sta     r0
        stx     r0+1
        lda     AddSign                 ; (A + or a space before it, if it's not below 0: by its
        ora     AddBlank                ;   real part's sign, KIND's)
        beq     NumShow
        lda     #<(NUM_KIND - NUM_INIT)
        sta     __num_entry
        lda     #0
        tax
        tay
        jsr     __num_xcall
        bcs     NumFail
        txa
        bmi     NumShow
        jsr     SetLeader
        bra     NumShow

NumFail:
        jmp     MainLoop                ; (Nothing written)

; %{base}d (or i), an int or a long; %{base}u (or o, x, X), unsigned: made a number (FROM_INT, in Buf)

IntSigned:
        jsr     GetSignedArg
        ldy     sreg+1                  ; (A + or a space before it, if it's not below 0)
        bmi     @L1
        pha
        phx
        jsr     SetLeader
        plx
        pla
@L1:    ldy     #1
        bra     IntNum
IntUnsigned:
        jsr     GetUnsignedArg
        ldy     #0
IntNum: sty     FromSigned
        sta     r0
        stx     r0+1
        lda     sreg
        sta     r1
        lda     sreg+1
        sta     r1+1
        lda     #<Buf
        sta     r2
        lda     #>Buf
        sta     r2+1
        lda     #20
        sta     r3
        stz     r3+1
        lda     #<(NUM_FROM_INT - NUM_INIT)
        sta     __num_entry
        lda     #0
        tax
        ldy     FromSigned
        jsr     __num_xcall
        bcs     NumFail
        lda     #<Buf
        sta     r0
        lda     #>Buf
        sta     r0+1

; The number at r0 written (DISPLAY) in BaseAt's base into printf's bank, at $8000, then out 20 bytes at a time
; through Buf, padded

NumShow:
        jsr     __num_ready
        bcs     NumFail
        lda     BaseAt
        sta     r4
        lda     BaseAt+1
        sta     r4+1
        stz     r2                      ; (Its text at $8000, 8K at most)
        lda     #$80
        sta     r2+1
        stz     r3
        lda     #$20
        sta     r3+1
        lda     #<(NUM_DISPLAY - NUM_INIT)
        sta     __num_entry
        lda     RAM_BANK                ; (printf's bank the caller's: the library writes there)
        sta     NumBank
        lda     __num_tbank
        sta     RAM_BANK
        lda     #0
        tax
        tay
        jsr     __num_xcall
        sta     NumLeft
        stx     NumLeft+1
        lda     NumBank
        sta     RAM_BANK
        bcc     @L1
        jmp     MainLoop                ; (Nothing written)

@L1:    lda     NumLeft                 ; Its length: the text's, and the + or space
        sta     ArgLen
        lda     NumLeft+1
        sta     ArgLen+1
        lda     Leader
        beq     @L2
        inc     ArgLen
        bne     @L2
        inc     ArgLen+1
@L2:    sec                             ; The padding: as HaveArg's, its count -(width - ArgLen + 1)
        lda     Width
        sbc     ArgLen
        tax
        lda     Width+1
        sbc     ArgLen+1
        bcs     @L3
        lda     #0
        tax
@L3:    eor     #$FF
        sta     Width+1
        txa
        eor     #$FF
        sta     Width
        lda     LeftJust
        bne     @L4
        jsr     OutputPadding
@L4:    lda     Leader
        beq     @L5
        jsr     Output1
@L5:    stz     NumAt                   ; The text, from $8000
        lda     #$80
        sta     NumAt+1

@Chunk: lda     NumLeft
        ora     NumLeft+1
        beq     @Done
        ldx     #20                     ; 20 bytes, or what's left
        lda     NumLeft+1
        bne     @L6
        lda     NumLeft
        cmp     #20
        bcs     @L6
        tax
@L6:    stx     ArgLen
        stz     ArgLen+1
        lda     NumAt
        sta     ptr1
        lda     NumAt+1
        sta     ptr1+1
        lda     RAM_BANK
        sta     NumBank
        lda     __num_tbank
        sta     RAM_BANK
        ldy     #0
@L7:    lda     (ptr1),y
        sta     Buf,y
        iny
        dex
        bne     @L7
        lda     NumBank
        sta     RAM_BANK
        lda     NumAt                   ; (On past them)
        clc
        adc     ArgLen
        sta     NumAt
        bcc     @L8
        inc     NumAt+1
@L8:    lda     NumLeft
        sec
        sbc     ArgLen
        sta     NumLeft
        bcs     @L9
        dec     NumLeft+1
@L9:    lda     #<Buf
        sta     Str
        lda     #>Buf
        sta     Str+1
        jsr     OutputArg
        bra     @Chunk

@Done:  lda     LeftJust
        beq     @L10
        jsr     OutputPadding
@L10:   jmp     MainLoop

; Leader = a + (AddSign) or a space (AddBlank), or 0

SetLeader:
        ldx     #0
        lda     AddBlank
        beq     @L1
        ldx     #' '
@L1:    lda     AddSign
        beq     @L2
        ldx     #'+'
@L2:    stx     Leader
        rts


; ----------------------------------------------------------------------------
; Local data (all static)

.bss

; Save area for the zero page registers
RegSave:        .res    regbanksize

; One character argument for OutFunc
CharArg:        .byte   0

; Format variables
FormatVars:
LeftJust:       .byte   0
AddSign:        .byte   0
AddBlank:       .byte   0
AltForm:        .byte   0
PadChar:        .byte   0
Width:          .word   0
Prec:           .word   0
IsLong:         .byte   0
Leader:         .byte   0
BufIdx:         .byte   0       ; Argument string pointer
HaveBase:       .byte   0       ; $FF: a base in braces (the Hydra's) ...
BaseAt:         .word   0       ;   its string (0: the base)
FormatVarSize   = * - FormatVars

; Argument buffer and pointer
Buf:            .res    20
Str:            .word   0
ArgLen:         .res    2

; The Hydra's numbers: a base in braces, its string; FROM_INT's .Y; the text in printf's bank, what's left of it and
; where; the caller's bank
BaseBuf:        .res    BASE_ROOM
FromSigned:     .res    1
NumLeft:        .res    2
NumAt:          .res    2
NumBank:        .res    1

.data

; Stuff from OutData. Is used as a vector
CallOutFunc:    jmp     $0000
