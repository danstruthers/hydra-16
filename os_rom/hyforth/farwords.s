;
; farwords.s
;
;   BIOS ROM page A (W = $A): the code of HyForth's words that live off page 1 (the shell's, IO and files,
;   tasks, sound, memory records, the stack printers, multiply and divide ...), some of page 1's routines
;   (the error messages, MALLOC), and the disassembler.  The words' headers are on page 1, in the
;   dictionary as ever (def_far), with code that's `jsr FARWORD` and the address of the word's code here:
;   FARWORD calls it through the gate FW_CALL, and a word here runs as it would on page 1.  So page 1 has
;   room for HyForth's core, which runs from ROM there.
;
;   This is its own scope, FAR, inside PAGE1: it sees page 1's names (HyForth's zero page, its RAM, its
;   constants), but its own come first: the gates (pagea.s), and the routines below with page 1's names
;   ('next', 'this', 'errrtn', the stack's spush and spull ...).  Its RAM is HyForth's.  Page 1 reaches the
;   routines here that aren't words (INCOPEN, PIPECHK ...) through the aliases after PAGE1 in all.s.
;
;   A name for IO_OPEN can be here: the IO layer reads names on the caller's page.  Other data another page
;   reads, or a user word does through a pointer (args), has to be in RAM: in the segment FORTH_DATA, which
;   COPYTORAM copies there.
;
.scope FAR
.include "pagea.s"              ; must be first in the scope
.include "monitor/disasm.s"     ; The disassembler (DISASM, DISASM_AY, DISASM_WM)

.segment "FORTH_FAR"
;
;---------------------------------------------------------------------
; A far word starts here (FARWORD, on page 1, through the gate FW_CALL): .A.Y = its code.  The stack as it
; is here is kept (FWSP), and the routines below that end the word ('next', 'errrtn' ...) go back to it, so
; they can be used from a subroutine as well
FW_ENTRY:
    sta ZP_FAR_VEC
    sty ZP_FAR_VEC+1
    tsx
    stx FWSP
    jmp (ZP_FAR_VEC)
;
; The word is done: back to page 1, and its 'next'
next:
    ldx FWSP
    txs
    clc
    rts
;
; An error (ERRFLAG says which): back to page 1, and its 'errrtn'
errrtn:
    ldx FWSP
    txs
    sec
    rts
;
; ( -- $FFFF ) and ( -- $0000 ), and the word is done
PUSHTRUE:
    lda #$FF
    sta TEMP1
    bra keeps
PUSHFALSE:
    stz TEMP1
    stz TEMP1+1
    bra this
;
; Push TEMP1 (keeps: its high byte .A), and the word is done
keeps:
    sta TEMP1+1
this:
    jsr spush_0
    bra next
;
;---------------------------------------------------------------------
; The data stack, as page 1's routines (hyforth.s): push a cell from the zero page cell at .Y, or pull one
; into it.  A full or empty stack is an error
spush_2:
    ldy #TEMP3
    bra spush
spush_1:
    ldy #TEMP2
    bra spush
spush_0:
    ldy #TEMP1
spush:
    ldx #DSPTR
    lda DSPTR
    cmp #<DS
    beq ptrerr_s
    jsr decwx
    lda 1, y
    sta (0, x)
    jsr decwx
    lda 0, y
    sta (0, x)
    rts
;
spull_2:
    ldy #TEMP3
    bra spull
spull_1:
    ldy #TEMP2
    bra spull
spull_0:
    ldy #TEMP1
spull:
    ldx #DSPTR
    lda DSPTR
    cmp #DSEND
    beq ptrerr_s
    lda (0, x)
    sta 0, y
    jsr incwx
    lda (0, x)
    sta 1, y
incwx:
    lda #01
addwx:
    clc
    adc 0, x
    sta 0, x
    bcc addwx_end
    inc 1, x
    clc
addwx_end:
    rts
;
decwx:
    lda 0, x
    bne decwx_end
    dec 1, x
decwx_end:
    dec 0, x
    rts
;
ptrerr_s:
    lda #ERR_SPTR
    sta ERRFLAG
    bra errrtn
;
;---------------------------------------------------------------------
;                          THE WORDS
;
; Each is here as it was on page 1 (primitives.s, hywords.s), where its header and its notes are
;
;----------------------------------------------------------------------
;  .S and .R
splist:                     ; .S  (changed from %S)
    lda DSPTR
    sta TEMP1
    lda DSPTR + 1
    sta TEMP1 + 1
    WCRLF_np
    PRINT_CHAR #ASCII_S
    lda #DSEND
    jsr STKLIST
    WCRLF_np
    jmp next
rplist:                     ; .R  (changed from %R)
    lda RTPTR
    sta TEMP1
    lda RTPTR + 1
    sta TEMP1 + 1
    WCRLF_np
    PRINT_CHAR #ASCII_R
    lda #RTEND
    jsr STKLIST
    WCRLF_np
    jmp next

;----------------------------------------------------------------------
;  list a sequence of references ( for .S and .R )
STKLIST:
    sec                 ; calc diff and length of list
    sbc TEMP1
    lsr
    tax                 ; hide in X
    PRINT_BYTE TEMP1 + 1,TEMP1        ; print addr of pointer
    PRINT_SPACE
    txa
    PRINT_BYTE         ; print # of entries
    PRINT_SPACE
    txa
    beq @ends
    ldy #0
@loop:
    PRINT_SPACE
    iny
    PRINT_BYTE {(TEMP1),y}
    dey
    PRINT_BYTE {(TEMP1),y}
    iny
    iny
    dex
    bne @loop
@ends:
    rts
;
;---------------------------------------------------------------------
;  .  .C  .sz
dot:                        ; .
    PRINT_SPACE
    jsr spull_0
    PRINT_BYTE TEMP1 + 1, TEMP1
    jmp next
cdot:                       ; .C
    jsr spull_0
    PRINT_CHAR TEMP1 + 1, TEMP1
    jmp next
szdot:                      ; .sz
    jsr spull_0    ; will have MEMPTR addr
    ldy #0
    lda (TEMP1),y
    sta TEMP2      ; will have RAM stack address
    iny
    lda (TEMP1),y
    sta TEMP2+1    ; TEMP2 now points at type byte of string?
    ldy #0
    lda (TEMP2),y
    and #$7F       ; mask off temp flag
    cmp #MEM_SZ
    bne SZEND
    ldy #3
SZLOOP:
    lda (TEMP2),y
    beq SZEND
    PRINT_CHAR
    iny
    bra SZLOOP
SZEND:
    jmp next
;
;---------------------------------------------------------------------
;  The stack and memory records: dsgetn@ dsget@ dwstk! dcstk! rdcstk! decs!
dsgetn:                     ; dsgetn@
    ldy #TEMP4
    jsr spull    ; get # WORDS
    jsr spull_1  ; get ptr addr
    jsr MEMLEN   ; returns TEMP3 w/maddr, length in TEMP1
    lda TEMP4
    asl a
    sta TEMP4
    cmp TEMP1
    bcs DSGETNSK
    sta TEMP1
DSGETNSK:
    lda TEMP1
    cmp #$78
    bcc DSGETNSK2
    jmp DSGETERR
DSGETNSK2:
    clc
    adc #3
    sta TEMP1
    ldy #3
    jmp DSGLOOP
dsget:                      ; dsget@
    jsr spull_1    ; get addr on memstack
    jsr MEMLEN     ; maddr in TEMP3, length in TEMP1
    lda TEMP1
    cmp #$78
    bcs DSGETERR
    lda TEMP1+1
    bne DSGETERR    ; too much data, can't push this on
    lda TEMP1
    clc
    adc #3
    sta TEMP1
    ldy #3
DSGLOOP:
    lda (TEMP3),y
    sta TEMP2
    iny
    lda (TEMP3),y
    sta TEMP2+1
    iny
    phy
    jsr spush_1
    ply
    cpy TEMP1
    bne DSGLOOP
DSGEND:
    jmp next
DSGETERR:
    lda #ERR_SPTR   ; throw pointer error
    sta ERRFLAG
    jmp errrtn
dwstkstore:                 ; dwstk!
    jsr HSSETUP
    jsr DW_STFWD
    jsr spush_1      ; and push ptr addr back on stack
    jmp next

DW_STFWD:
    ldy #0            ; count up
DWFLOOP:
    lda DSPTR
    sec
    sbc #DSEND
    bcs DW_FWDEND
    phy
    jsr spull_0
    ply
    lda TEMP1
    sta (TEMP3),y
    iny
    lda TEMP1+1
    sta (TEMP3),y
    iny
    cpy TEMP5
    bne DWFLOOP
DW_FWDEND:
    rts
dcstkstore:                 ; dcstk!
    jsr HSSETUP
    jsr HS_STFWD
    jsr spush_1      ; and push ptr addr back on stack
    jmp next

HS_STFWD:
    ldy #0            ; count up
HSRLOOP:
    lda DSPTR
    sec
    sbc #DSEND
    bcs HS_FWDEND
    phy
    jsr spull_0
    ply
    lda TEMP1
    sta (TEMP3),y
    iny
    cpy TEMP5
    bne HSRLOOP
HS_FWDEND:
    rts
rdstkstore:                 ; rdcstk!
    jsr HSSETUP
    jsr HS_STREV
    jsr spush_1      ; and push ptr addr back on stack
    jmp next

HSSETUP:
    jsr spull_1     ; get addr from malloc run -> TEMP2
    jsr MEMLEN      ; length in TEMP1, maddr TEMP3
    lda TEMP1
    sta TEMP5
    lda TEMP3
    clc
    adc #3          ; calc offset to storage
    sta TEMP3
    bcc HSSETUPEND
    inc TEMP3+1
HSSETUPEND:
    rts

HS_STREV:
    ldy TEMP5            ; count down from length
    lda #0
    dey
    sta (TEMP3),y
HSFLOOP:
    lda DSPTR
    sec
    sbc #DSEND
    bcs HS_REVEND
    phy
    jsr spull_0
    ply
    lda TEMP1
    dey
    sta (TEMP3),y
    bne HSFLOOP
HS_REVEND:
    rts
;
;   replace leading zeros with spcs when creating decimal ascii string
;
DECSFINISH:
    ldy #0
DECSCLRLOOP:             ; replace $00 or leading $30 with spaces
    lda (TEMP3),y
    beq  DECSTSK02
    cmp #ASCII_0
    beq  DECSTSK02
    bra  DECSTDONE
DECSTSK02:
    lda #ASCII_SPACE
    sta (TEMP3),y
    iny
    bra DECSCLRLOOP
DECSTDONE:
    rts
decstore:                   ; decs!
    jsr HSSETUP
    jsr HS_STREV
    jsr DECSFINISH
    jsr spush_1      ; and push ptr addr back on stack
    jmp next
;
;---------------------------------------------------------------------
;  Bits: tbit sbit cbit
tbit:                       ; tbit  (nondestructive test bit)
    jsr spull_1     ; which bit
    lda TEMP2
    and #$0F        ; only want 0-16
    sta TEMP2
    jsr spull_0
    jsr spush_0     ; backup!
    jsr BITWIND
    lda TEMP1
    and #$01
    beq TBCLR      ; test bit 0
    lda #$FF
    sta TEMP1
    sta TEMP1+1
    bra TBITEND
TBCLR:
    stz TEMP1
    stz TEMP1+1
TBITEND:
    jmp this
sbit:                       ; sbit  (set bit)
    jsr spull_1     ; which bit
    lda TEMP2
    and #$0F        ; only want 0-16
    sta TEMP2
    jsr spull_0
    jsr BITWIND
    lda TEMP1
    ora #$01        ; set bit 0
    sta TEMP1
    jsr BITUNWIND
    jmp this
cbit:                       ; cbit
    jsr spull_1     ; which bit
    lda TEMP2
    and #$0F        ; only want 0-16
    sta TEMP2
    jsr spull_0
    jsr BITWIND
    lda TEMP1
    and #$FE       ; clear bit 0
    sta TEMP1
    jsr BITUNWIND
    jmp this

BITWIND:
    stz TEMP3
    stz TEMP3+1
    ldx TEMP2
    beq BITWSKIP
BITWLOOP:
    lsr TEMP1+1
    ror TEMP1
    ror TEMP3+1
    ror TEMP3
    dex
    bne BITWLOOP
BITWSKIP:
    rts

BITUNWIND:
    ldx TEMP2
    beq BITUWSKIP
BITUNWLOOP:
    asl TEMP3
    rol TEMP3+1
    rol TEMP1
    rol TEMP1+1
    dex
    bne BITUNWLOOP
BITUWSKIP:
    rts
;
;---------------------------------------------------------------------
;  ANSI screen: Acls Ascr Acol
.ifdef ANSIOK
Acls:                       ; Acls
    jsr CLEAR_SCR
    jmp next
Ascr:                       ; Ascr
    PRINT_ANSI_ESC_SEQ
    jsr spull_0
    ldx TEMP1
    jsr DEC2ASCII
    lda TEMP3+1
    cmp #ASCII_0
    beq ASCRNXT1
    PRINT_CHAR
ASCRNXT1:
    PRINT_CHAR TEMP3, #ASCII_SEMI
    jsr spull_0
    ldx TEMP1
    jsr DEC2ASCII
    lda TEMP3+1
    cmp #ASCII_0
    beq ASCRNXT2
    PRINT_CHAR
ASCRNXT2:
    PRINT_CHAR TEMP3, #ASCII_f
    jmp next
Acol:                       ; Acol
    PRINT_ANSI_ESC_SEQ
    jsr spull_0
    ldx TEMP1
    jsr DEC2ASCII
    lda TEMP3+1
    cmp #ASCII_0
    beq ACOLNXT2
    PRINT_CHAR
ACOLNXT2:
    PRINT_CHAR TEMP3, #ASCII_m
    jmp next
;
;-----------------------   NUMBER CONVERSIONS
;
DEC2ASCII:       ;  X is # - return as two digits in TEMP3, TEMP3+1
    lda #ASCII_0
    sta TEMP3+1
    txa
    sta TEMP3
D2ASCLOOP:
    sec
    sbc #ASCII_LF
    bcc D2ASCNEXT
    inc TEMP3+1
    bra D2ASCLOOP
D2ASCNEXT:
    clc
    adc #$3A
    sta TEMP3
    rts
.endif   ; ANSIOK
;
;---------------------------------------------------------------------
;  xdrv
xdrv:                       ; xdrv
    jsr spull_0
    lda TEMP1
    ldy TEMP1+1
    jsr HEX2DEC
    jmp next
;
H2NUM: .byte $27,$10
 .byte $03,$E8
 .byte $00,$64
 .byte $00,$0A
HEX2DEC:                            ; low/high in A,Y - use X, TEMP1, TEMP3, TEMP4, TEMP6
    sty TEMP3+1
    sta TEMP3
    ldx #0
H2DDIV10:
    lda H2NUM,x
    sta TEMP4+1
    inx
    lda H2NUM,x
    sta TEMP4
    inx
    stz TEMP6
H2DLOOP:
    lda TEMP3+1
    cmp TEMP4+1
    bcc  H2DSK1
    bne  H2DSK0
    lda TEMP3
    cmp TEMP4
    bcc  H2DSK1
H2DSK0:
    lda TEMP3
    sec
    sbc TEMP4
    sta TEMP3
    lda TEMP3+1
    sbc TEMP4+1
    sta TEMP3+1
    inc TEMP6
    bra H2DLOOP
H2DSK1:
    lda TEMP6
    clc
    adc #ASCII_0
    sta TEMP1
    stz TEMP1+1
    phx
    jsr spush_0                      ; remember!  A/X both destroyed with push and pull!
    plx
    cpx #8
    beq H2DFIN
    jmp H2DDIV10
H2DFIN:
    lda TEMP3
    clc
    adc #ASCII_0
    sta TEMP1
    stz TEMP1+1
    jsr spush_0
    rts
;
;---------------------------------------------------------------------
;  disasm
disasm:                     ; disasm
    jsr spull_1    ; # of instructions (??)
    jsr spull_0    ; addr
    ldx TEMP2
    cpx #$FE
    bcs DISEND
    lda #1           ; and ZP_D_STATE has to be =1 in order to show mnemonics etc
    sta ZP_D_STATE
    lda TEMP1
    ldy TEMP1+1
    sec              ; set the carry to make sure multiple ops returned
    jsr DISASM_AY
    lda ZP_D_XAM
    sta TEMP1
    ldy ZP_D_XAM+1
    sty TEMP1+1
    jsr spush_0     ; push last address on stack?
DISEND:
    jmp next
;
;---------------------------------------------------------------------
;  IO: files
open:                       ; open
    jsr spull_1       ; mode
    jsr spull_0       ; the string
    ldy #0
    lda (TEMP1),y
    sta TEMP3
    iny
    lda (TEMP1),y
    sta TEMP3+1       ; TEMP3 = the record
    lda (TEMP3)
    and #$7F          ; (the temp flag)
    cmp #MEM_SZ
    beq IOPENSZ
    lda #ERR_IO_NAME  ; not a string
    bra IOFAIL
IOPENSZ:
    lda TEMP3         ; its text, after the 3-byte header
    clc
    adc #3
    pha
    lda TEMP3+1
    adc #0
    tay
    pla
    ldx TEMP2
    jsr IO_OPEN       ; .A = fd
    bcs IOFAIL
IOPUSHA:
    sta TEMP1
    stz TEMP1+1
    jmp this
IOFAIL:                     ; (an IO error, .A: 'ioerr' has it, and the word ends with !IO ERR!)
    sta IOERR
    lda #ERR_IO
    sta ERRFLAG
    jmp errrtn
close:                      ; close
    jsr spull_0
    lda TEMP1
    jsr IO_CLOSE
    bcs IOFAIL
    jmp next
read:                       ; read
    jsr IOARGS
    jsr IO_READ
IODONE:
    bcs IOFAIL
    lda ZP_IO_CNT
    sta TEMP1
    lda ZP_IO_CNT+1
    jmp keeps
write:                      ; write
    jsr IOARGS
    jsr IO_WRITE
    bra IODONE
seek:                       ; seek
    jsr spull_2       ; hi
    jsr spull_1       ; lo
    jsr spull_0       ; fd
    lda TEMP2
    sta ZP_IO_OFS
    lda TEMP2+1
    sta ZP_IO_OFS+1
    lda TEMP3
    sta ZP_IO_OFS+2
    lda TEMP3+1
    sta ZP_IO_OFS+3
    lda TEMP1
    jsr IO_SEEK
    bcs SEEKFAIL
    jmp next
SEEKFAIL:
    jmp IOFAIL
ioctl:                      ; ioctl
    jsr spull_2       ; arg
    jsr spull_1       ; code
    jsr spull_0       ; fd
    lda TEMP1
    ldx TEMP2
    ldy TEMP3
    jsr IO_CTL
    bcs SEEKFAIL      ; (IOFAIL, in reach)
    jmp next
fdup2:                      ; fdup2
    jsr spull_1       ; newfd
    jsr spull_0       ; fd
    lda TEMP1
    ldx TEMP2
    jsr IO_DUP2
    bcs IOFAIL2
    jmp next
IOFAIL2:
    jmp IOFAIL
pipe:                       ; pipe
    jsr IO_PIPE       ; .A = read fd, .X = write fd
    bcs IOFAIL2
    sta TEMP1
    stz TEMP1+1
    stx TEMP2
    jsr spush_0
    lda TEMP2
    jmp IOPUSHA
create:                     ; create
    jsr spull_1       ; the mode
    jsr spull_0       ; the name
    ldx #TEMP1
    jsr SZTEXT        ; .A.Y = its text
    bcs FSFAIL
    ldx TEMP2
    stx ZP_IO_BUF     ; (IO_CREATE: the new file's mode)
    ldx #IO_MODE_RDWR
    jsr IO_CREATE
    bcs FSFAIL
    jmp IOPUSHA
FSFAIL:
    jmp IOFAIL
;
;---------------------------------------------------------------------
;  The shell: the current directory, include, run, echo
cd:                         ; cd
    jsr ARGGET          ; .A.Y = the path; C = 1: none
    bcc CDGO
    ldy #0              ; (SH_CD: .Y = 0, no path)
CDGO:
    jsr SH_CD
    bcs SHFAIL
    jmp next
pcd:                        ; (cd)
    jsr SHARG1          ; .A.Y = the q^...^ string's text
    bcc CDGO
SHFAIL:
    jmp IOFAIL
pwd:                        ; pwd
    jsr SH_PWD
    jmp next
prompt:                     ; prompt
    jsr SHARG1
    bcs SHFAIL
    sta TEMP3
    sty TEMP3+1
    ldy #0
PROMPTCP:
    lda (TEMP3),y
    sta PROMPTFMT,y
    beq PROMPTDONE
    iny
    cpy #PROMPTFMT_SIZE - 1
    bne PROMPTCP
    lda #0
    sta PROMPTFMT,y
PROMPTDONE:
    jmp next
;
; ( sz -- ) -> .A.Y = its text, C = 0; or .A = ERR_IO_NAME, C = 1 (not a string)
SHARG1:
    jsr spull_0
    ldx #TEMP1
    jmp SZTEXT
;
; A parsing word's argument: the next word on the line (up to a space, or the line's end), or "a name in
; quotes" (spaces and all), into ARGBUF, zero-terminated; the interpreter goes on after it.
; OUT: C = 0: .A.Y = ARGBUF; or C = 1: none.  Uses TEMP6
ARGGET:
    ldy CURBUF
AGSKIP:
    lda (TIB),y         ; (spaces before it)
    beq AGNONE
    cmp #ASCII_SPACE
    bne AGWORD
    iny
    bra AGSKIP
AGWORD:
    ldx #ASCII_SPACE    ; (what ends it: a space; or in quotes, the closing '"')
    cmp #ASCII_DQUOTE
    bne AGPLAIN
    iny
    ldx #ASCII_DQUOTE
AGPLAIN:
    stx TEMP6
    ldx #0
AGCOPY:
    lda (TIB),y
    beq AGEND
    cmp TEMP6
    beq AGCLOSE
    cpx #ARGBUF_SIZE - 1
    bcs AGLONG          ; (too long: the rest is left out)
    sta ARGBUF,x
    inx
AGLONG:
    iny
    bra AGCOPY
AGCLOSE:
    cmp #ASCII_DQUOTE   ; (past the closing '"')
    bne AGEND
    iny
AGEND:
    stz ARGBUF,x
    sty CURBUF
    lda #<ARGBUF
    ldy #>ARGBUF
    clc
    rts
AGNONE:
    sty CURBUF
    sec
    rts
include:                    ; include
    jsr ARGGET
    bcc INCGO
    lda #ERR_IO_NAME
    bra INCFAIL
pinclude:                   ; (include)
    jsr SHARG1
    bcs INCFAIL
INCGO:
    jsr INCOPEN
    bcs INCFAIL
    jmp next
INCFAIL:
    jmp IOFAIL
;
; Start reading the script at .A.Y (a name): stdin is kept (another fd for it), and the script becomes
; stdin until its end (INCEND, from the line reader).  OUT: C = 0; or C = 1, .A = error
INCOPEN:
    ldx INCDEPTH
    cpx #INC_MAX
    bcs INCDEEP
    ldx #IO_MODE_READ
    jsr IO_OPEN
    bcs INCODONE
INCOPENFD:              ; (.A = the script's fd, closed here: stdin has it)
    sta TEMP5           ; the script's fd
    jsr INSAVE          ; stdin, kept
    bcs INCOCLOSE
    ldx INCDEPTH
    sta INCFD,x
    txa
    asl
    tax
    stz INCLINE,x
    stz INCLINE+1,x
    inc INCDEPTH
    lda TEMP5
    ldx #0
    jsr IO_DUP2         ; stdin = the script
INCOCLOSE:
    php
    pha
    lda TEMP5
    jsr IO_CLOSE        ; (its own fd: stdin has it now)
    pla
    plp
INCODONE:
    rts
INCDEEP:
    lda #ERR_IO_NAME
    sec
    rts
;
; A script's end: stdin back to what it was
INCEND:
    dec INCDEPTH
    ldx INCDEPTH
    lda INCFD,x
    pha
    ldx #0
    jsr IO_DUP2
    pla
    jmp IO_CLOSE
;
; The next line of the script being read: count it
INCCOUNT:
    lda INCDEPTH
    asl
    tax
    inc INCLINE-2,x
    bne INCCDONE
    inc INCLINE-1,x
INCCDONE:
    rts
;
; An error while a script is read: say which line of it, and stop it and every script that includes it
INCABORT:
    lda INCDEPTH
    beq INCCDONE
    PRINT_CHAR #'l', #'i', #'n', #'e', #' '
    lda INCDEPTH
    asl
    tax
    lda INCLINE-1,x
    PRINT_BYTE
    lda INCDEPTH
    asl
    tax
    lda INCLINE-2,x
    PRINT_BYTE
INCALOOP:
    jsr INCEND
    lda INCDEPTH
    bne INCALOOP
    rts
run:                        ; run
    jsr ARGGET
    bcc RUNARGS
    lda #ERR_IO_NAME
    bra RUNFAIL
RUNARGS:                ; the rest of the line: the program's arguments
    jsr ARGREST
    lda #<ARGBUF        ; (the name: ARGGET's)
    ldy #>ARGBUF
    bra RUNGO
prun:                       ; (run)
    jsr SHARG1
    bcs RUNFAIL
    stz ARGLINE         ; (no arguments)
RUNGO:
    ldx #SHC_RUN
    jsr RUNCMD
    bcs RUNFAIL
    jmp next
RUNFAIL:
    jmp IOFAIL
;
; A program: SH_CMD .X (SHC_RUN, SHC_EXEC) on the name at .A.Y.  An executable has run when it returns;
; a script runs here: a copy of the shell (TASK_CLONE) reads it from SH_RUN_FD, and we wait for it to end.
; OUT: C = 0; or C = 1, .A = error
RUNCMD:
    jsr SH_CMD
    bcs RCDONE
    tax
    beq RCDONE          ; (an executable: done)
    lda #<PAGE1::run_start
    ldy #>PAGE1::run_start
    ldx #1              ; (HyForth's ROM page)
    jsr TASK_CLONE      ; .A = the copy
    php
    pha
    lda #SH_RUN_FD
    jsr IO_CLOSE        ; (the copy has it)
    pla
    plp
    bcs RCDONE
    ldx #SHC_WAIT
    jmp SH_CMD
RCDONE:
    rts
;
; A word HyForth doesn't know (the token at NXTTOK), when interpreting: the program of that name (SH_EXEC:
; name.hyx or name.hys, here or in the card's /bin), run as run does.
; OUT: C = 0: it ran; or C = 1, .A = error (ERR_IO_NOT_FOUND: no such program)
RUNNAME:
    ldy #0
    lda (NXTTOK),y      ; (its length, then its characters)
    cmp #ARGBUF_SIZE
    bcs RNNONE
    tax
RNCOPY:
    iny
    lda (NXTTOK),y
    sta ARGBUF-1,y
    dex
    bne RNCOPY
    lda #0
    sta ARGBUF,y
    jsr ARGREST         ; (the rest of the line: its arguments)
    lda #<ARGBUF
    ldy #>ARGBUF
    ldx #SHC_EXEC
    bra RUNCMD
RNNONE:
    lda #ERR_IO_NOT_FOUND
    sec
    rts
;
; A program's arguments: the rest of the line (from CURBUF, without the spaces around it; 63 characters at
; most) into ARGLINE, and the line ends there (the program has them, not the shell).  Modifies: .A, .X, .Y
ARGREST:
    ldy CURBUF
ARSKIP:
    lda (TIB),y         ; (spaces before them)
    cmp #ASCII_SPACE
    bne ARCOPY0
    iny
    bra ARSKIP
ARCOPY0:
    ldx #0
ARCOPY:
    lda (TIB),y
    sta ARGLINE,x
    beq ARTRIM
    iny
    inx
    cpx #ARGLINE_SIZE - 1
    bcc ARCOPY
ARTRIM:                 ; (.X = how many: spaces after them go, the line's own and blanked-out redirections)
    cpx #0
    beq AREND
    lda ARGLINE-1,x
    cmp #ASCII_SPACE
    bne AREND
    dex
    bra ARTRIM
AREND:
    stz ARGLINE,x
    ldy CURBUF          ; the line ends here
    lda #0
    sta (TIB),y
    rts
args:                       ; args
    lda #MEM_SZ         ; (ARGLINE, as a string record)
    sta ARGREC
    lda #ARGLINE_SIZE
    sta ARGREC+1
    stz ARGREC+2
    lda #<ARGREF
    sta TEMP1
    lda #>ARGREF
    sta TEMP1+1
    jmp this
.segment "FORTH_DATA"           ; (RAM: the words here read it, through the pointer args gives)
ARGREF:
    .word ARGREC
.segment "FORTH_FAR"
edit:                       ; edit
    ldx #SHC_EDIT
    jmp SHPARSE
echo:                       ; echo
    ldy CURBUF          ; .X = where the text ends (after its last character that isn't a space)
    ldx CURBUF
ECHOEND:
    iny
    lda (TIB),y
    beq ECHOSTART
    cmp #ASCII_SPACE
    beq ECHOEND
    tya
    tax
    inx
    bra ECHOEND
ECHOSTART:
    stx TEMP1
    ldy CURBUF
    iny                 ; (the space after echo)
ECHOCHAR:
    cpy TEMP1
    bcs ECHODONE
    lda (TIB),y
    cmp #ASCII_DQUOTE
    beq ECHONEXT
    PRINT_CHAR
ECHONEXT:
    iny
    bra ECHOCHAR
ECHODONE:
    PRINT_CHAR #ASCII_CR, #ASCII_LF
    lda #0              ; the line ends here: it was echo's
    ldy CURBUF
    sta (TIB),y
    jmp next
;
;---------------------------------------------------------------------
;  Shell commands: files and the cards
ls:                         ; ls
    ldx #SHC_LS
    jmp SHPARSE
pls:                        ; (ls)
    ldx #SHC_LS
    jmp SHSTACK
rm:                         ; rm
    ldx #SHC_RM
    jmp SHPARSE
prm:                        ; (rm)
    ldx #SHC_RM
    jmp SHSTACK
rmdir:                      ; rmdir
    ldx #SHC_RMDIR
    jmp SHPARSE
prmdir:                     ; (rmdir)
    ldx #SHC_RMDIR
    jmp SHSTACK
mkdir:                      ; mkdir
    ldx #SHC_MKDIR
    jmp SHPARSE
pmkdir:                     ; (mkdir)
    ldx #SHC_MKDIR
    jmp SHSTACK
cp:                         ; cp
    ldx #SHC_CP
    jmp SHPARSE2
pcp:                        ; (cp)
    ldx #SHC_CP
    jmp SHSTACK2
mv:                         ; mv
    ldx #SHC_MV
    jmp SHPARSE2
pmv:                        ; (mv)
    ldx #SHC_MV
    jmp SHSTACK2
vols:                       ; vols
    ldx #SHC_VOLS
    bra SHDO
mkfs:                       ; mkfs
    stz SHOPT
MKFSWHOLE:
    stz SHSIZE          ; (the whole card)
    stz SHSIZE+1
MKFSGO:
    ldx #SHC_MKFS
    bra SHCARDSZ
mkfsfull:                   ; mkfs-full
    lda #1
    sta SHOPT
    bra MKFSWHOLE
mkfssize:                   ; mkfs-size
    jsr spull_0
    lda TEMP1
    sta SHSIZE
    lda TEMP1+1
    sta SHSIZE+1
    stz SHOPT
    bra MKFSGO
relabel:                    ; relabel
    ldx #SHC_RELABEL
    bra SHCARDSZ
fsck:                       ; fsck
    ldx #SHC_FSCK
    bra SHCARD
fsfix:                      ; fsfix
    ldx #SHC_FSFIX
SHCARD:                 ; ( n -- ): .A = the card
    phx
    jsr spull_0
    lda TEMP1
    plx
SHDO:                   ; the command .X (SH_CMD, page 7)
    jsr SH_CMD
    bcs SHDOFAIL
    jmp next
SHDOFAIL:
    jmp IOFAIL
SHCARDSZ:               ; ( n sz -- ): .A = the card, SHBUF2 = the text
    phx
    jsr spull_1
    jsr spull_0
    ldx #TEMP2
    jsr SZTEXT
    bcs SHFAILX
    jsr SHCOPY2
    lda TEMP1
    plx
    bra SHDO
SHPARSE:                ; the line's next word (none: .Y = 0)
    phx
    jsr ARGGET
    bcc SHPGO
    ldy #0
SHPGO:
    plx
    bra SHDO
SHSTACK:                ; ( sz -- )
    phx
    jsr SHARG1
    bcs SHFAILX
    plx
    bra SHDO
SHPARSE2:               ; the line's next two words: the first in SHBUF2
    phx
    jsr ARGGET
    bcs SHP2NONE
    jsr SHCOPY2
    jsr ARGGET
    bcs SHP2NONE
    plx
    bra SHDO
SHP2NONE:
    lda #ERR_IO_NAME
SHFAILX:
    plx
    bra SHDOFAIL
SHSTACK2:               ; ( sz1 sz2 -- ): the first in SHBUF2
    phx
    jsr NSARGS          ; .A.Y = the first's text, ZP_IO_BUF = the second's
    bcs SHFAILX
    jsr SHCOPY2
    lda ZP_IO_BUF
    ldy ZP_IO_BUF+1
    plx
    bra SHDO
;
; SHBUF2 = the text at .A.Y (63 characters at most).  Uses TEMP3
SHCOPY2:
    sta TEMP3
    sty TEMP3+1
    ldy #0
SHC2LP:
    lda (TEMP3),y
    sta SHBUF2,y
    beq SHC2DONE
    iny
    cpy #63
    bne SHC2LP
    lda #0
    sta SHBUF2,y
SHC2DONE:
    rts
;
;---------------------------------------------------------------------
;  Namespaces, the serial port, ctl, cat, wc; tasks; pipelines
mount:                      ; mount
    jsr NSARGS
    bcs NSFAIL
    jsr IO_MOUNT
    bra NSDONE
bind:                       ; bind
    jsr NSARGS
    bcs NSFAIL
    jsr IO_BIND
NSDONE:
    bcs NSFAIL
    jmp next
NSFAIL:
    jmp IOFAIL
unmount:                    ; unmount
    jsr spull_0
    ldx #TEMP1
    jsr SZTEXT
    bcs NSFAIL
    jsr IO_UNMOUNT
    bra NSDONE
ns:                         ; ns
    jsr IO_NS_LIST
    jmp next
stty:                       ; stty
    jsr spull_0
    ldx #TEMP1
    jsr SZTEXT          ; .A.Y = the commands
    bcs STTYFAIL
    sta TEMP2
    sty TEMP2+1
    lda #<STTY_CTL
    ldy #>STTY_CTL
    ldx #IO_MODE_WRITE
    jsr IO_OPEN
STTYOPEN:               ; (ctl: .A = the fd or the error, TEMP2 = the text)
    bcs STTYFAIL
    sta TEMP3           ; the fd
    lda TEMP2
    sta ZP_IO_BUF
    lda TEMP2+1
    sta ZP_IO_BUF+1
    ldy #0              ; the length
STTYLEN:
    lda (TEMP2),y
    beq STTYWRITE
    iny
    bne STTYLEN
STTYWRITE:
    sty ZP_IO_CNT
    stz ZP_IO_CNT+1
    lda TEMP3
    jsr IO_WRITE
    php
    pha
    lda TEMP3
    jsr IO_CLOSE
    pla
    plp
    bcs STTYFAIL
    jmp next
STTYFAIL:
    jmp IOFAIL
sttyq:                      ; stty?
    lda #<STTY_CTL
    ldy #>STTY_CTL
    ldx #IO_MODE_READ
    jsr IO_OPEN
    bcs STTYFAIL
    sta TEMP3
STTYSHOW:
    ldx TEMP3
    jsr IO_GETC
    bcs STTYSHOWN       ; (the end)
    PRINT_CHAR
    bra STTYSHOW
STTYSHOWN:
    lda TEMP3
    jsr IO_CLOSE
    jmp next
STTY_CTL:
    .byte "/dev/ser/ctl", 0
ctl:                        ; ctl
    jsr NSARGS          ; .A.Y = the file's name, ZP_IO_BUF = the text
    bcs CTLFAIL
    ldx ZP_IO_BUF
    stx TEMP2
    ldx ZP_IO_BUF+1
    stx TEMP2+1
    ldx #IO_MODE_WRITE
    jsr IO_OPEN
    jmp STTYOPEN        ; (stty's: it writes TEMP2's text, and closes the file)
CTLFAIL:
    jmp IOFAIL
;
; ( sz-path sz-2 -- ) -> .A.Y = the path's text, ZP_IO_BUF = the second's (C = 1: not strings)
NSARGS:
    jsr spull_1
    jsr spull_0
    ldx #TEMP2
    jsr SZTEXT
    bcs NSARGEND
    sta ZP_IO_BUF
    sty ZP_IO_BUF+1
    ldx #TEMP1
    jmp SZTEXT
NSARGEND:
    rts
;
; The text of a q^...^ string.  IN: .X = the ZP address of its slot (TEMP1, TEMP2)
; OUT: .A.Y = its text, C = 0; or .A = ERR_IO_NAME, C = 1.  Uses TEMP3
SZTEXT:
    lda 0,x
    sta TEMP3
    lda 1,x
    sta TEMP3+1
    ldy #0
    lda (TEMP3),y         ; the record
    pha
    iny
    lda (TEMP3),y
    sta TEMP3+1
    pla
    sta TEMP3
    lda (TEMP3)
    and #$7F              ; (the temp flag)
    cmp #MEM_SZ
    bne SZBAD
    lda TEMP3             ; its text, after the 3-byte header
    clc
    adc #3
    pha
    lda TEMP3+1
    adc #0
    tay
    pla
    clc
    rts
SZBAD:
    lda #ERR_IO_NAME
    sec
    rts
ioerr:                      ; ioerr
    lda IOERR
    jmp IOPUSHA
cat:                        ; cat
    jsr ARGGET          ; cat file: the file
    bcs CATLOOP
    ldx #SHC_CAT
    jmp SHDO
CATLOOP:
    jsr GET_CHAR
    bcc CATEND        ; end of file (or an error)
    PRINT_CHAR
    bra CATLOOP
CATEND:
    jmp next
wc:                         ; wc
    ldx #5
WCCLR:
    stz TEMP1,x       ; TEMP1 = characters, TEMP2 = words, TEMP3 = lines
    dex
    bpl WCCLR
    stz TEMP4         ; TEMP4 <> 0: in a word
    stz TEMP4+1       ; TEMP4+1 <> 0: the last character was a CR
WCLOOP:
    jsr GET_CHAR
    bcc WCEND
    inc TEMP1         ; a character
    bne WCSK0
    inc TEMP1+1
WCSK0:
    ldx TEMP4+1       ; (the last one a CR?)
    stz TEMP4+1
    cmp #ASCII_CR
    bne WCNOTCR
    inc TEMP4+1
    bra WCLINE
WCNOTCR:
    cmp #ASCII_LF
    bne WCSK1
    cpx #0
    bne WCSK1         ; CR LF: one line
WCLINE:
    inc TEMP3         ; a line
    bne WCSK1
    inc TEMP3+1
WCSK1:
    cmp #ASCII_SPACE
    beq WCGAP
    cmp #ASCII_TAB
    beq WCGAP
    cmp #ASCII_CR
    beq WCGAP
    cmp #ASCII_LF
    beq WCGAP
    lda TEMP4
    bne WCLOOP        ; (still in the word)
    inc TEMP4         ; a new word
    inc TEMP2
    bne WCLOOP
    inc TEMP2+1
    bra WCLOOP
WCGAP:
    stz TEMP4
    bra WCLOOP
WCEND:
    lda TEMP1         ; ( -- lines words chars )
    pha
    lda TEMP1+1
    pha
    lda TEMP2
    pha
    lda TEMP2+1
    pha
    lda TEMP3
    sta TEMP1
    lda TEMP3+1
    sta TEMP1+1
    jsr spush_0       ; lines
    pla
    sta TEMP1+1
    pla
    sta TEMP1
    jsr spush_0       ; words
    pla
    sta TEMP1+1
    pla
    sta TEMP1
    jmp this          ; characters
shell:                      ; shell
    lda #<::SHELL_MAIN
    ldy #>::SHELL_MAIN
    ldx #0            ; (ROM page 0)
    jsr TASK_RUN
    bcc TKPUSH
    jmp IOFAIL
TKPUSH:
    jmp IOPUSHA
fg:                         ; fg
    jsr spull_0
    lda TEMP1
    jsr CONS_SET_FG
    bcc TKOK
    jmp IOFAIL
TKOK:
    jmp next
kill:                       ; kill
    jsr spull_0
    ldx TEMP1
    lda #TASK_KILL_FLAG
    jsr TASK_SIGNAL
    bcc TKOK
    jmp IOFAIL
sleep:                      ; sleep
    jsr spull_0
    lda TEMP1
    ldy TEMP1 + 1
    jsr TASK_SLEEP
    jmp next
ps:                         ; ps
    lda #<PSNAME
    ldy #>PSNAME
    ldx #IO_MODE_READ
    jsr IO_OPEN
    bcc PSOPEN
    jmp IOFAIL
PSOPEN:
    sta TEMP1         ; the fd
PSLOOP:
    ldx TEMP1
    jsr IO_GETC
    bcs PSEND         ; end of file
    PRINT_CHAR
    bra PSLOOP
PSEND:
    lda TEMP1
    jsr IO_CLOSE
    jmp next
PSNAME:
    .byte "/dev/proc", 0
;
;-------- Pipelines: a line  left | right  runs 'left' in a copy of this task (TASK_CLONE), with its
;         stdout into a pipe, and 'right' here, with stdin from the pipe.  Called by getline.
;
; If the line (TIB) has a '|' with spaces around it (outside q^...^ and "..." strings) and we're interpreting,
; start the left side, and go on after the '|'; and again for each '|' after it (a | b | c: 'b' runs in a
; copy too, between two pipes).  (No '|': returns with the line untouched.)  Called by getline (page 1,
; through its gate).  OUT: C = 0; or C = 1, .A = the error
PIPECHK:
    lda STATUS
    bne PCNONE        ; compiling
    ldy CURBUF        ; (What's left of the line)
    iny
    ldx #0            ; inside a string: 1 (q^...^) or 2 ("..."); 0: not
PCSCAN:
    lda (TIB),y
    beq PCNONE
    cmp #ASCII_CARET
    bne PCQUOTE
    cpx #2
    beq PCNEXT        ; (a '^' in a "..." string)
    txa
    eor #1
    tax
    bra PCNEXT
PCQUOTE:
    cmp #ASCII_DQUOTE
    bne PCBAR
    cpx #1
    beq PCNEXT        ; (a '"' in a q^...^ string)
    txa
    eor #2
    tax
    bra PCNEXT
PCBAR:
    cmp #ASCII_PIPE
    bne PCNEXT
    cpx #0
    bne PCNEXT        ; inside a string
    dey
    lda (TIB),y
    iny
    cmp #ASCII_SPACE
    bne PCNEXT
    iny
    lda (TIB),y
    dey
    cmp #ASCII_SPACE
    beq PCFOUND
PCNEXT:
    iny
    bne PCSCAN
PCNONE:
    clc
    rts
PCFOUND:                   ; .Y = the '|'
    phy
    jsr IO_PIPE            ; .A = read fd, .X = write fd
    bcs PCFAIL
    sta PIPER
    stx PIPEW
    ply
    phy
    lda #0
    sta (TIB),y            ; the copy's line ends at the '|'...
    lda #<PAGE1::child_start
    ldy #>PAGE1::child_start
    ldx #1                 ; (HyForth's ROM page)
    jsr TASK_CLONE
    ply
    pha
    lda #ASCII_SPACE
    sta (TIB),y            ; ...and this one goes on after it
    sty CURBUF
    pla
    bcs PCNOTASK
    lda PIPEW              ; the copy has the write end now
    jsr IO_CLOSE
    lda PIPEIN
    bpl PCSAVED            ; (a later '|': stdin is the previous pipe; the new one replaces it)
    jsr INSAVE             ; save stdin (with its read-ahead given back: a pipeline in a script)
    bcs PCFAIL2
    sta PIPEIN
PCSAVED:
    lda PIPER
    ldx #0
    jsr IO_DUP2            ; stdin from the pipe
    bcs PCFAIL2
    lda PIPER
    jsr IO_CLOSE
    jmp PIPECHK            ; another '|'?
PCNOTASK:                  ; no task for the copy: no pipeline
    pha
    lda PIPEW
    jsr IO_CLOSE
    lda PIPER
    jsr IO_CLOSE
    pla
    bra PCFAIL2
PCFAIL:
    ply
PCFAIL2:                   ; (getline drops its returns, and fails with the error)
    sec
    rts
;
; The end of a pipeline's line (getline): stdin back
PIPEEND:
    lda PIPEIN
    bmi PEDONE             ; ($FF: not redirected)
    ldx #0
    jsr IO_DUP2
    lda PIPEIN
    jsr IO_CLOSE
    lda #$FF
    sta PIPEIN
PEDONE:
    rts
;
; ( fd addr n -- ) -> ZP_IO_BUF = addr, ZP_IO_CNT = n, .A = fd
IOARGS:
    jsr spull_2       ; n
    jsr spull_1       ; addr
    jsr spull_0       ; fd
    lda TEMP2
    sta ZP_IO_BUF
    lda TEMP2+1
    sta ZP_IO_BUF+1
    lda TEMP3
    sta ZP_IO_CNT
    lda TEMP3+1
    sta ZP_IO_CNT+1
    lda TEMP1
    rts
;
;---------------------------------------------------------------------
;  Sound (the YM2151, through /dev/snd)
.ifdef YSOUND
sndinit:                    ; sndinit
    lda #SND_CTL_INIT
    jmp SNDCTL
sndtest:                    ; sndtest
    lda #SND_CTL_TEST
    jmp SNDCTL
sndstop:                    ; sndstop
    lda #SND_CTL_STOP
SNDCTL:               ; IO_CTL code .A on /dev/snd
    sta TEMP2
    jsr SNDOPEN
    sta TEMP1         ; the fd
    ldx TEMP2
    jsr IO_CTL
SNDCLOSE:            ; close fd TEMP1, keeping .A and C
    php
    pha
    lda TEMP1
    jsr IO_CLOSE
    pla
    plp
    bcs SNDFAIL
    jmp next
SNDFAIL:
    jmp IOFAIL
;
; Open /dev/snd for writing: .A = the fd (a failure: IOFAIL)
SNDOPEN:
    lda #<SNDNAME
    ldy #>SNDNAME
    ldx #IO_MODE_WRITE
    jsr IO_OPEN
    bcs SNDOPENF
    rts
SNDOPENF:
    ply               ; (drop the return: fail from the word)
    ply
    jmp IOFAIL
SNDNAME:
    .byte "/dev/snd", 0
ywrite:                     ; ywrite
    jsr spull_0
    lda TEMP1+1       ; the register, then the value: a pair for /dev/snd
    sta TEMP3
    lda TEMP1
    sta TEMP3+1
    jsr SNDOPEN
    sta TEMP1
    lda #<TEMP3
    sta ZP_IO_BUF
    stz ZP_IO_BUF+1
    lda #2
    sta ZP_IO_CNT
    stz ZP_IO_CNT+1
    lda TEMP1
    jsr IO_WRITE
    php
    lda TEMP1
    jsr IO_CLOSE
    plp
    bcs YMBAD
    jmp PUSHTRUE
YMBAD:
    jmp PUSHFALSE
.endif
;
;---------------------------------------------------------------------
;  Random numbers: rseed rand rand32
rseed:                      ; rseed
    jsr spull_0
    lda TEMP1
    sta RSEED
    lda TEMP1+1
    sta RSEED+1
    jsr spull_0
    lda TEMP1
    sta RSEED+2
    lda TEMP1+1
    sta RSEED+3
    jmp next
rand:                       ; rand
    jsr galois32o
    jmp RAND32IN
rand32:                     ; rand32
    jsr galois32o
    lda RSEED+2
    sta TEMP1
    lda RSEED+3
    sta TEMP1+1
    jsr spush_0
RAND32IN:
    lda RSEED
    sta TEMP1
    lda RSEED+1
    sta TEMP1+1
    jsr spush_0
    jmp next
;
;---------------------------------------------------------------------
;  Multiply and divide: * /
mult16:                     ; *
    jsr spull_1
    jsr spull_0
    lda TEMP1            ; handle zeros  3/22 1320
    ora TEMP1+1
    beq m16zero          ; if either is 0-0, can skip to end
    lda TEMP2
    ora TEMP2+1
    beq m16zero          ; TEMP2 is zero?  same deal.
    jsr MULT16
    bra m16push
m16zero:
    stz TEMP1
    stz TEMP1+1
    stz TEMP3
    stz TEMP3+1
m16push:
    jsr spush_2           ; MSbyte
    jsr spush_0           ; push LSbyte in TEMP1 on top
    jmp next
div16:                      ; /
    jsr spull_1
    jsr spull_0
    lda TEMP1            ; handle zeros  3/22 1320
    ora TEMP1+1
    beq d16zero         ; shortcut!
    lda TEMP2
    bne d16skip0
    ora TEMP2+1
    bne d16skip1
    jmp div0err                     ; divide by zero error
d16skip0:
    cmp #1
    bne d16skip1
    lda TEMP2+1
    bne d16skip1
    bra d16rzero                   ; dividing by 1, just return TEMP1
d16skip1:
    jsr DIV16                      ; results TEMP2, remainder TEMP3
    jmp d16done
d16zero:
    stz TEMP1
    stz TEMP1+1
d16rzero:
    stz TEMP3
    stz TEMP3+1
d16done:
    jsr spush_2        ; remainder first
    jsr spush_0        ; result on top
    jmp next
;
;                      divide by zero error
div0err:                      ; pop jsr off stack, throw error
    lda #ERR_DIV0
    sta ERRFLAG
    jmp errrtn
;-------------------------------------------------------------
;                MATH routines
;         with MULT16 / DIV16, signs handled by calling word.
;         we just do the math here.
;
;
MULT16:                             ; 16 x 16 multiply; TEMP1 and TEMP2 are #'s, TEMP1 will be result
    stz TEMP3                       ; with TEMP3 as high bytes
    stz TEMP3+1
    ldx #17
    clc
MULTLOOP:
    ror TEMP3+1                     ; RIGHT.  if you need to go backwards, go backwards stupid fuck.
    ror TEMP3
    ror TEMP1+1
    ror TEMP1
    bcc MULTDECCNT
    clc
    lda TEMP2
    adc TEMP3
    sta TEMP3
    lda TEMP2+1
    adc TEMP3+1
    sta TEMP3+1
MULTDECCNT:
    dex
    bne MULTLOOP
    rts

;
;
       ; 16 x 16 divide; TEMP1 and TEMP2 #'s - TEMP3 is 'overflow'
       ;  TEMP2 divisor, TEMP1 dividend, TEMP1 + 3 = result + remainder
DIV16:
    stz TEMP3
    stz TEMP3+1
    ldx #16
UDIVLP:
    rol TEMP1
    rol TEMP1+1
    rol TEMP3
    rol TEMP3+1
UDIVCHK:
    sec
    lda TEMP3
    sbc TEMP2
    tay
    lda TEMP3+1
    sbc TEMP2+1
    bcc UDIVCNT
    sty TEMP3
    sta TEMP3+1
UDIVCNT:
    dex
    bne UDIVLP
    rol TEMP1
    rol TEMP1+1
    rts
;
;     galois32o - LSFR psuedo-random # generator
;
;  -- boilerplate --
; 6502 LFSR PRNG - 32-bit
; Brad Smith, 2019
; http://rainwarrior.ca
;
;
galois32o:
    ; rotate the middle bytes left
    ldy RSEED+2                     ; will move to RSEED+3 at the end
    lda RSEED+1
    sta RSEED+2
    ; compute RSEED+1 ($C5>>1 = %1100010)
    lda RSEED+3                     ; original high byte
    lsr
    sta RSEED+1                     ; reverse: 100011
    lsr
    lsr
    lsr
    lsr
    eor RSEED+1
    lsr
    eor RSEED+1
    eor RSEED+0                     ; combine with original low byte
    sta RSEED+1
    ; compute RSEED+0 ($C5 = %11000101)
    lda RSEED+3                     ; original high byte
    asl
    eor RSEED+3
    asl
    asl
    asl
    asl
    eor RSEED+3
    asl
    asl
    eor RSEED+3
    sty RSEED+3                     ; finish rotating byte 2 into 3
    sta RSEED+0
    rts
;
;---------------------------------------------------------------------
;  Memory records and MMU handles: mktemp purge0 malloc mlen halloc hfree hlock hunlock
mktemp:                     ; mktemp
    jsr spull_1       ; get mptr in TEMP2
    jsr MEMLEN        ; len in TEMP1, maddr in TEMP3
    ldy #0
    lda (TEMP3),y
    ora #$80          ; set high bit on type byte
    sta (TEMP3),y
    jmp next
purge0:                     ; purge0
    lda MEMPTR
    sta TEMP2
    lda MEMPTR+1
    sta TEMP2+1
    cmp #>(MEMSTK+MEMEND)
    bcc PURGECONT
    bne PURGEEND
    lda TEMP2
    cmp #<(MEMSTK+MEMEND)
    bcs PURGEEND
PURGECONT:
    lda TEMP2
    clc
    adc #2
    sta TEMP2         ; TEMP2 = the last record's slot
    bcc  PURGESK00
    inc TEMP2+1
PURGESK00:
    jsr MEMLEN        ; len in TEMP1, maddr in TEMP3
    ldy #0
    lda (TEMP3),y
    and #$80          ; mask off all but temp bit
    beq  PURGEEND
    lda TEMP2         ; change pointers if temp
    sta MEMPTR
    lda TEMP2+1
    sta MEMPTR+1
    lda (TEMP3)       ; large record (its own MMU block)?
    and #MEM_MMU
    beq PURGEARENA
    jsr MMUFREE       ; free its block; the arena (MEMLAST) is unchanged
    bra PURGEEND
PURGEARENA:
    lda TEMP3
;    clc
;    adc #3
;    bcc  PURGESK01
;    inc TEMP3+1
;PURGESK01:
;    clc
;    adc TEMP1
    sta MEMLAST
    lda TEMP3+1
;    adc TEMP1+1
    sta MEMLAST+1
PURGEEND:
    jmp next
malloc:                     ; malloc
    jsr spull_1       ; type ( word ($00), char ($01), words ($02), bytes ($03), sz ($04) ..)
    jsr spull_0       ; # bytes
    jsr MALLOC        ; will return address in TEMP1
    bcs MALLOCOK
    lda #ERR_MEM      ; out of memory
    sta ERRFLAG
    jmp errrtn
MALLOCOK:
    jsr spush_0       ; push ptr address to new record on stack
    jmp next
mlen:                       ; mlen
    jsr spull_1
    jsr MEMLEN   ; returns length in TEMP1, maddr in TEMP3
    jsr spush_0
    jmp next
halloc:                     ; halloc
    jsr spull_1       ; flags
    jsr spull_0       ; bytes
    lda TEMP1
    ldy TEMP1+1
    ldx TEMP2
    jsr MM_ALLOC      ; .A = handle
    bcs HMERR
    sta TEMP1
    stz TEMP1+1
    jsr spush_0
    jmp next
HMERR:
    lda #ERR_MEM      ; out of memory (or a bad handle)
    sta ERRFLAG
    jmp errrtn
hfree:                      ; hfree
    jsr spull_0
    lda TEMP1
    jsr MM_FREE
    bcs HMERR
    jmp next
hlock:                      ; hlock
    jsr spull_0
    lda TEMP1
    jsr MM_LOCK       ; .A.Y = address, .X = previous RAM bank
    bcs HMERR
    stx HLBANK
    sta TEMP1
    sty TEMP1+1
    jsr spush_0
    jmp next
hunlock:                    ; hunlock
    jsr spull_0
    lda TEMP1
    ldx HLBANK
    jsr MM_UNLOCK
    bcs HMERR
    jmp next
;
;=====================================================================
;  Page 1's routines that live here (upper.s), called through page 1's gates
;
;---------------------------------------------------------------------
;  error messaging
;
wrterror:
    lda #>err_jumptable
    sta ERRPTR+1
    lda ERRFLAG
    beq ERREND
    asl a
    clc
    adc #<err_jumptable
    sta ERRPTR
    bcc ERRSKIP
    inc ERRPTR+1
ERRSKIP:
    WERR ERRPTR
ERREND:
    lda #0
    sta ERRFLAG
    sta ERRPTR
    sta ERRPTR+1
    rts
;
;
emcount .set 0                      ; (ERR_entry counts the messages, in this scope)
err_jumptable:
    .res 2
    ERR_entry RPTR_ERR              ; RT stack full/empty  - error $01
    ERR_entry SPTR_ERR              ; DS stack full/empty  - error $02
    ERR_entry DIV_ERR               ; divide by zero - error $03
    ERR_entry OOM_ERR               ; out of memory  - error $04
    ERR_entry UKW_ERR               ; no existing word - error $05
    ERR_entry SEC_ERR               ; writing to dangerous RAM areas - error $06
    ERR_entry SYS_ERR               ; error on return from SYSCALL - error $07
    ERR_entry IO_ERR                ; IO error (see ioerr) - error $08
    ERR_entry BRK_ERR               ; break from the console - error $09
LASTERR = 9
;
;  error messages
RPTR_ERR:
    .byte " !RT PTR ERROR!"
    .byte 0
SPTR_ERR:
    .byte " !DS PTR ERROR!"
    .byte 0
DIV_ERR:
    .byte " !DIV ZERO!"
    .byte 0
OOM_ERR:
    .byte " !LOW MEM!"
    .byte 0
UKW_ERR:
    .byte " !UNK WORD!"
    .byte 0
SEC_ERR:
    .byte " !SECURITY!"
    .byte 0
SYS_ERR:
    .byte " !SYS ERR!"
    .byte 0
IO_ERR:
    .byte " !IO ERR!"
    .byte 0
BRK_ERR:
    .byte " !BREAK!"
    .byte 0
;
;-------- malloc and mlen

MALFAIL:
    clc                   ; out of memory
    rts
MALLOC:
    ;  TEMP1 and TEMP2 should have bytes / record type if 'jsr MALLOC'
    ;  OUT: C = 1 and TEMP1 = memory stack slot (holds the record address); C = 0 if out of memory
    ;  Records smaller than FORTH_LARGE_MIN go in the arena (MEMLAST grows down, not below MEMBOT);
    ;  bigger ones get their own MMU block, marked with MEM_MMU in the type byte.
    ;  uses TEMP3, TEMP4, y, x, a
    lda MEMPTR           ; memory stack full?
    cmp #<MEMSTK
    lda MEMPTR+1
    sbc #>MEMSTK
    bcc MALFAIL
    lda TEMP1+1
    bne MALLARGE
                        ; arena: new record at MEMLAST - 3 - bytes, into TEMP4
    lda MEMLAST
    sec
    sbc #3
    sta TEMP4
    lda MEMLAST+1
    sbc #0
    sta TEMP4+1
    lda TEMP4
    sec
    sbc TEMP1
    sta TEMP4
    lda TEMP4+1
    sbc TEMP1+1
    sta TEMP4+1
    bcc MALFAIL         ; wrapped
    lda TEMP4
    cmp MEMBOT
    lda TEMP4+1
    sbc MEMBOT+1
    bcc MALFAIL         ; below the arena
    lda TEMP4
    sta MEMLAST
    lda TEMP4+1
    sta MEMLAST+1        ;MEMLAST updated to start of new record
    bra MALHDR
MALLARGE:               ; its own MMU block: bytes + 3 for the header
    lda TEMP1
    clc
    adc #3
    pha
    lda TEMP1+1
    adc #0
    tay
    pla
    bcs MALFAIL          ; more than $FFFF
    ldx #0
    jsr MM_ALLOC         ; .A = handle
    bcs MALFAIL
    pha
    jsr MM_LOCK          ; .A.Y = address (page blocks don't move), .X = RAM bank
    sta TEMP4
    sty TEMP4+1
    pla
    jsr MM_UNLOCK
    lda TEMP2
    ora #MEM_MMU
    sta TEMP2
MALHDR:
    lda TEMP4
    sta TEMP3
    lda TEMP4+1
    sta TEMP3+1         ; use TEMP3 to walk through clearing of memory
    ldy #0
    lda TEMP2            ; write type first
    sta (TEMP3),y
    iny
    lda TEMP1            ; LSB length
    sta (TEMP3),y
    iny
    lda TEMP1+1          ; MSB length
    sta (TEMP3),y
    ldx #TEMP3
    lda #3
    jsr addwx            ; increment TEMP3 by 3
MALLOOP:                 ; clear TEMP1 bytes at TEMP3
    lda TEMP1
    ora TEMP1+1
    beq MALCONT
    lda #0
    sta (TEMP3)
    inc TEMP3
    bne MALSK01
    inc TEMP3+1
MALSK01:
    lda TEMP1
    bne MALSK02
    dec TEMP1+1
MALSK02:
    dec TEMP1
    bra MALLOOP
MALCONT:                   ; now store the record address at MEMPTR
    ldy #0
    lda TEMP4
    sta (MEMPTR),y
    iny
    lda TEMP4+1
    sta (MEMPTR),y
    lda MEMPTR+1
    sta TEMP1+1
    lda MEMPTR
    sta TEMP1             ; copy to TEMP1 before incrementing
    sec                   ; MEMPTR + 2
    sbc #2
    sta MEMPTR
    bcs MALLOCEND
    dec MEMPTR+1
MALLOCEND:
    sec                   ; OK
    rts
;
;  Free a large (MEM_MMU) record's MMU block.  IN: TEMP3 = record address.  uses a, x, y
MMUFREE:
    lda TEMP3
    ldy TEMP3+1
    jsr MM_FIND                  ; .A = handle
    bcs MMUFREEND
    jsr MM_FREE
MMUFREEND:
    rts
; 
MEMLEN:              ; address in TEMP2
     ldy #0
     lda (TEMP2),y
     sta TEMP3
     iny
     lda (TEMP2),y   ; and deref once
     sta TEMP3+1
     ldy #1
     lda (TEMP3),y   ; skip over type, get length
     sta TEMP1
     iny
     lda (TEMP3),y
     sta TEMP1+1
     rts
;
.endscope
;
;  end farwords.s
