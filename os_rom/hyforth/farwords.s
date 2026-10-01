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
    lda OUTBASE
    cmp #10
    bne DOTHEX
    bit TEMP1 + 1
    bpl DOTDEC
    PRINT_CHAR #ASCII_MINUS
    sec                     ; (- n)
    lda #0
    sbc TEMP1
    sta TEMP1
    lda #0
    sbc TEMP1 + 1
    sta TEMP1 + 1
DOTDEC:
    jsr PRINT_UDEC
    jmp next
DOTHEX:
    PRINT_BYTE TEMP1 + 1, TEMP1
    jmp next
udot:                       ; u.
    PRINT_SPACE
    jsr spull_0
    lda OUTBASE
    cmp #10
    beq DOTDEC
    bra DOTHEX
decimal:                    ; decimal
    lda #10
    bra SETBASE
hexbase:                    ; hex
    lda #16
SETBASE:
    sta OUTBASE
    jmp next
;
; TEMP1 in decimal, unsigned, with no leading zeros.  Modifies .A, .X, .Y, TEMP1, TEMP2
PRINT_UDEC:
    stz TEMP2               ; (A digit printed: the zeros after it count)
    ldx #3
UDECPOW:                    ; The digit for 10^(.X + 1): how many times it goes
    ldy #'0'
UDECSUB:
    lda TEMP1
    sec
    sbc UDECLO, x
    pha
    lda TEMP1 + 1
    sbc UDECHI, x
    bcc UDECLESS
    sta TEMP1 + 1
    pla
    sta TEMP1
    iny
    bra UDECSUB
UDECLESS:
    pla
    tya
    cmp #'0'
    bne UDECOUT
    ldy TEMP2
    beq UDECNEXT            ; (A leading zero)
UDECOUT:
    sta TEMP2
    phx
    PRINT_CHAR
    plx
UDECNEXT:
    dex
    bpl UDECPOW
    lda TEMP1               ; The units
    ora #'0'
    PRINT_CHAR_JMP
UDECLO:
    .byte <10, <100, <1000, <10000
UDECHI:
    .byte >10, >100, >1000, >10000
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
;  syscall ( jsaddr a y -- x ): call the machine code at jsaddr with .A and .Y set, and BIOS ROM page 0
;  selected, as a program's code runs: so every thunk ($F800 ...) works, and code in RAM that calls them.
;  It pushes what the code leaves in .X
syscall:
    jsr spull_2                 ; .Y
    jsr spull_1                 ; .A
    jsr spull_0                 ; The address
    lda TEMP2
    ldy TEMP3
    jsr SYS_CALL
    stx TEMP1
    stz TEMP1+1
    jsr spush_0
    jmp next
;
;  sys ( jsaddr a x y -- a x y p ): syscall with every register, in and out, and the flags after it (p; C is
;  bit 0: the OS's calls set it when they fail, with the error in .A)
sys:
    jsr spull_2                 ; .Y
    ldy #TEMP4
    jsr spull                   ; .X
    jsr spull_1                 ; .A
    jsr spull_0                 ; The address
    lda TEMP2
    ldx TEMP4
    ldy TEMP3
    jsr SYS_CALL
    php
    sta TEMP1
    stx TEMP2
    sty TEMP3
    pla
    sta TEMP4
    stz TEMP1+1
    stz TEMP2+1
    stz TEMP3+1
    stz TEMP4+1
    jsr spush_0                 ; a
    jsr spush_1                 ; x
    jsr spush_2                 ; y
    ldy #TEMP4
    jsr spush                   ; p
    jmp next
;
; Call the code at TEMP1 on BIOS ROM page 0, with .A, .X, .Y (and C) as they are; back with them as it left them
SYS_CALL:
    pha
    lda TEMP1
    sta ZP_FAR_VEC
    lda TEMP1+1
    sta ZP_FAR_VEC+1
    stz ZP_FAR_PAGE
    pla
    sta ZP_FAR_A
    jmp FAR_CALL_A
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
hwtest:                     ; hwtest
    _M_HWT_ENTER
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
;-------- Libraries (hyforth.s: LIBN_IO ...; their headers' chains: lib_begin): their names, in LIBN_ order,
;         and what loading each loads (its bit, and the ones it needs)
LIBNAMES:
    .byte "io", 0, "files", 0, "shell", 0, "tasks", 0, "sound", 0, "mem", 0, "tools", 0, "term", 0
    .byte "all", 0    ; (After them: every one)
LIBNEEDS:
    .byte LIB_IO
    .byte LIB_FILES | LIB_IO
    .byte LIB_SHELL | LIB_FILES | LIB_IO
    .byte LIB_TASKS | LIB_IO
    .byte LIB_SOUND | LIB_IO
    .byte LIB_MEM
    .byte LIB_TOOLS
    .byte LIB_TERM
.assert * - LIBNEEDS = LIB_COUNT, error, "LIBNEEDS: one for each library"
libs:                       ; libs
    PRINT_CHAR #'f', #'o', #'r', #'t', #'h'
    ldx #0            ; .X: the name's place in LIBNAMES
    lda #1
    sta TEMP1         ; the library's bit
LSEACH:
    PRINT_SPACE
    lda LIBSET        ; (one that isn't loaded: in parentheses)
    and TEMP1
    bne LSNAME
    PRINT_CHAR #'('
LSNAME:
    lda LIBNAMES,x
    beq LSEND
    PRINT_CHAR
    inx
    bra LSNAME
LSEND:
    inx               ; (past its 0)
    lda LIBSET
    and TEMP1
    bne LSNEXT
    PRINT_CHAR #')'
LSNEXT:
    asl TEMP1
    bne LSEACH
    ldx #0            ; Then the RAM libraries: each slot with a name (in parentheses: not searched)
LSRAM:
    ldy RLIBOFS,x
    lda RLIBNAME,y
    beq LSRNEXT
    PRINT_SPACE
    lda RLIBBIT,x
    and LIBSET2
    sta TEMP1
    bne LSRNAME
    PRINT_CHAR #'('
LSRNAME:
    lda RLIBNAME,y
    beq LSREND
    PRINT_CHAR
    iny
    bra LSRNAME
LSREND:
    lda TEMP1
    bne LSRNEXT
    PRINT_CHAR #')'
LSRNEXT:
    inx
    cpx #RLIB_MAX
    bne LSRAM
    jmp next
lib:                        ; lib
    jsr LIBARG
    bcc LBROM
    jmp RLIBLOAD      ; (Not a ROM library's: a file's)
LBROM:
    cpx #LIB_COUNT
    bcc LBONE
    lda RLIBHAVE      ; (all: the RAM libraries loaded too, and every ROM library)
    tsb LIBSET2
    lda #$FF
    bra LBSET
LBONE:
    lda LIBNEEDS,x    ; (it, and the ones it needs)
LBSET:
    tsb LIBSET
    jmp next
unlib:                      ; -lib
    jsr LIBARG
    bcs UNRAM
    cpx #LIB_COUNT
    bcc UNROM
    pha               ; (all: the RAM libraries too)
    lda #$FF ^ LIB2_BASE
    trb LIBSET2
    pla
UNROM:
    trb LIBSET        ; (.A: its bit; all: $FF)
    jmp next
UNRAM:
    jsr RLIBFIND      ; A RAM library: not searched (it stays in RAM: lib searches it again)
    bcc UNFOUND
    jmp LAUNKNOWN
UNFOUND:
    lda RLIBBIT,x
    trb LIBSET2
    jmp next
;
;-------- Libraries from files (RAM libraries: hyforth.s, RLIB_MAX).  lib name, when no ROM library has
;         that name: loaded already, it's searched again; else name.hyl (HyForth source: SH_CMD SHC_LIBOPEN
;         finds it, as a program is found, in $LIBPATH or /lib) is read as include reads a script, from the
;         next line on, into a slot of its own.  While it's read, the words it defines go on a chain of
;         their own (LASTHEAP from 0; the words defined in RAM aren't searched meanwhile); at its end
;         (INCEND: RLIBEND) that chain is the library's, and the words in RAM's chain are back.  An error
;         while it's read drops it (INCABORT).
RLIBLOAD:
    jsr RLIBFIND
    bcs RLNEW
    lda RLIBBIT,x     ; (Loaded: searched again; still loading: nothing)
    and RLIBHAVE
    tsb LIBSET2
    jmp next
RLNEW:
    ldx #0            ; A free slot
RLFREE:
    ldy RLIBOFS,x
    lda RLIBNAME,y
    beq RLSLOT
    inx
    cpx #RLIB_MAX
    bne RLFREE
    lda #ERR_MEM      ; (None: out of memory)
    sta ERRFLAG
    jmp errrtn
RLSLOT:
    stx RLIBSLOT
    ldx #0            ; The name must fit
RLLEN:
    lda ARGBUF,x
    beq RLOPEN
    inx
    cpx #RLIB_NAMELEN
    bcc RLLEN
    lda #ERR_IO_NAME
    bra RLFAIL
RLOPEN:
    lda #<ARGBUF
    ldy #>ARGBUF
    ldx #SHC_LIBOPEN
    jsr SH_CMD        ; .A = its fd
    bcs RLFAIL
    jsr INCOPENFD     ; Read as a script, from the next line on
    bcs RLFAIL
    ldx RLIBSLOT      ; Its slot: its name ...
    ldy RLIBOFS,x
    ldx #0
RLNAME:
    lda ARGBUF,x
    sta RLIBNAME,y
    beq RLNAMED
    iny
    inx
    bra RLNAME
RLNAMED:
    lda RLIBSLOT      ;   the words in RAM's chain, kept till its end ...
    asl
    tax
    lda LASTHEAP
    sta RLIBSAVE,x
    lda LASTHEAP+1
    sta RLIBSAVE+1,x
    stz LASTHEAP      ;   its own chain, from none ...
    stz LASTHEAP+1
    ldx RLIBSLOT
    lda INCDEPTH      ;   and its file's depth
    sta RLIBDEPTH,x
    jmp next
RLFAIL:
    jmp IOFAIL
;
; The RAM library slot named ARGBUF.  OUT: C = 0, .X = it; C = 1: none.  Modifies: .A, .Y
RLIBFIND:
    ldx #0
RFSLOT:
    ldy RLIBOFS,x
    stx RLIBSLOT
    ldx #0
RFCHAR:
    lda RLIBNAME,y
    cmp ARGBUF,x
    bne RFNEXT
    iny
    inx
    cmp #0
    bne RFCHAR
    ldx RLIBSLOT      ; (The same, to their 0s: a free slot's "" isn't a name)
    clc
    rts
RFNEXT:
    ldx RLIBSLOT
    inx
    cpx #RLIB_MAX
    bne RFSLOT
    sec
    rts
;
; The script at INCDEPTH ends: a RAM library it was loading is done: its chain is its own (LIB_HEADS2) and
; searched, and the words in RAM's chain are back; or, the scripts being stopped (RLIBFAIL), it's dropped
; (its slot free again).  Modifies: .A, .X, .Y
RLIBEND:
    ldx #RLIB_MAX-1
REFIND:
    lda RLIBDEPTH,x
    cmp INCDEPTH
    beq REFOUND
    dex
    bpl REFIND
    rts
REFOUND:
    lda #$FF
    sta RLIBDEPTH,x
    txa
    asl
    tay
    lda RLIBFAIL
    bne REDROP
    lda LASTHEAP
    sta LIB_HEADS2,y
    lda LASTHEAP+1
    sta LIB_HEADS2+1,y
    lda RLIBBIT,x
    tsb RLIBHAVE
    tsb LIBSET2
    bra REBACK
REDROP:
    phy
    ldy RLIBOFS,x
    lda #0
    sta RLIBNAME,y
    ply
REBACK:
    lda RLIBSAVE,y
    sta LASTHEAP
    lda RLIBSAVE+1,y
    sta LASTHEAP+1
    rts
;
RLIBOFS:    .byte 0 * RLIB_NAMELEN, 1 * RLIB_NAMELEN, 2 * RLIB_NAMELEN, 3 * RLIB_NAMELEN
RLIBBIT:    .byte $01, $02, $04, $08
.assert     RLIB_MAX = 4, error, "RLIBOFS, RLIBBIT: one for each RAM library slot"
;
; A library's name: the next word on the line (ARGGET), into ARGBUF.  OUT: C = 0, a ROM library's: .X = the
; library (LIBN_), .A = its bit; or all: .X = $FF, .A = $FF.  C = 1: not a ROM library's name (ARGBUF has
; it).  No name: the word ends with !UNK WORD!.  Uses TEMP1, TEMP2
LIBARG:
    jsr ARGGET        ; (into ARGBUF)
    bcs LAUNKNOWN
    ldy #0            ; .Y: LIBNAMES
    stz TEMP2         ; the library
    lda #1
    sta TEMP1         ;   and its bit
LANAME:
    ldx #0            ; .X: ARGBUF
LACHAR:
    lda LIBNAMES,y
    cmp ARGBUF,x
    bne LANEXT
    iny
    inx
    cmp #0
    bne LACHAR
    ldx TEMP2         ; (the same, to their 0s)
    lda TEMP1
    cpx #LIB_COUNT    ; (all)
    bcc LAFOUND
    ldx #$FF
    lda #$FF
LAFOUND:
    clc
    rts
LANEXT:
    lda LIBNAMES,y    ; (past the rest of this name, and its 0)
    beq LASKIP
    iny
    bra LANEXT
LASKIP:
    iny
    inc TEMP2
    asl TEMP1
    lda TEMP2
    cmp #LIB_COUNT + 1
    bne LANAME
    sec               ; (Not a ROM library's name)
    rts
LAUNKNOWN:
    lda #ERR_UKW
    sta ERRFLAG
    jmp errrtn
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
; A script's end: stdin back to what it was (and a RAM library it was loading, done: RLIBEND)
INCEND:
    phy               ; (.Y kept: the line reader's place in its line)
    jsr RLIBEND
    ply
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
    lda #1            ; (The RAM libraries they were loading: dropped)
    sta RLIBFAIL
INCALOOP:
    jsr INCEND
    lda INCDEPTH
    bne INCALOOP
    stz RLIBFAIL
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
    stz SHBG
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
; ( -- n )  the exit status
status:                     ; status
    lda HYSTAT
    jmp IOPUSHA
;
; ( n -- )  end this task with exit status n: the boot shell can't end (its status only)
exits:                      ; exits
    jsr spull_0
    lda TEMP1
    sta HYSTAT
    stz HYSTATMSG
    lda T_REGISTER
    and #$0F
    cmp #SHELL_TASK_NUM
    bne LINE_EXITS
    ldx #SHC_STATUS
    jsr SH_CMD
    jmp next
;
; getline: the end of stdin, with no script being read.  A file or a pipe (a command shell's command line, say)
; ends the task, with its status (LINE_EXITS); the console (an end-of-input key) doesn't.  As getline's prompt
; decides: no fd 0 (the console, read directly), or fd 0 on /dev/cons (IO_FDF_CONS)
LINE_EOF:
    lda IO_FD_SERVER
    cmp #IO_FD_CLOSED
    beq LEDONE
    lda IO_FD_FLAGS
    and #IO_FDF_CONS
    bne LEDONE
; ... and a copy's end (a script run by run, a pipeline's stage: getline), bye's (a copy, a command shell), exits':
; the task ends with the status (TASK_EXITS: HYSTAT, HYSTATMSG)
LINE_EXITS:
    lda #<HYSTATMSG
    sta ZP_IO_BUF
    lda #>HYSTATMSG
    sta ZP_IO_BUF+1
    lda HYSTAT
    jmp TASK_EXITS
LEDONE:
    rts
;
; A word HyForth doesn't know (the token at NXTTOK), when interpreting: the program of that name (SH_EXEC:
; name.hyx or name.hys, here or in the card's /bin), run as run does.
; OUT: C = 0: it ran; or C = 1, .A = error (ERR_IO_NOT_FOUND: no such program)
RUNNAME:
    lda LIBSET          ; (Only with the shell's library: else not found)
    and #LIB_SHELL
    bne RNSHELL
    lda #ERR_IO_NOT_FOUND
    sec
    rts
RNSHELL:
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
    jmp RUNCMD
RNNONE:
    lda #ERR_IO_NOT_FOUND
    sec
    rts
;
; A program's arguments: the rest of the line (from CURBUF, without the spaces around it; 63 characters at
; most) into ARGLINE, and the line ends there (the program has them, not the shell).  Modifies: .A, .X, .Y
ARGREST:
    stz SHBG
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
    bne ARAMP
    dex
    bra ARTRIM
ARAMP:                  ; an & at the end (alone, or after a space): the program runs in the background (SHBG)
    cmp #'&'
    bne AREND
    cpx #1
    beq ARBG
    lda ARGLINE-2,x
    cmp #ASCII_SPACE
    bne AREND
ARBG:
    dex                 ; (the & goes, and the spaces before it)
    inc SHBG
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
    stz SHOPT           ; (ls -l: SHOPT <> 0, a long listing: SH_LS)
    jsr ARGGET
    bcs LSNONE
    lda ARGBUF          ; -l?
    cmp #'-'
    bne LSPATH
    lda ARGBUF+1
    cmp #'l'
    bne LSPATH
    lda ARGBUF+2
    bne LSPATH
    inc SHOPT
    jsr ARGGET          ; (the path after it)
    bcc LSPATH
LSNONE:
    ldy #0
    bra LSGO
LSPATH:
    lda #<ARGBUF
    ldy #>ARGBUF
LSGO:
    ldx #SHC_LS
    jmp SHDO
pls:                        ; (ls)
    stz SHOPT
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
    lda #HFS_FMT_FULL
    sta SHOPT
    bra MKFSWHOLE
mkfspart:                   ; mkfs-part
    lda #HFS_FMT_PART
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
forth:                      ; forth
    lda #<PAGE1::forth_bare_main
    ldy #>PAGE1::forth_bare_main
    ldx #1            ; (ROM page 1)
    bra TKRUN
shell:                      ; shell
    lda #<::SHELL_MAIN
    ldy #>::SHELL_MAIN
    ldx #0            ; (ROM page 0)
TKRUN:
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
wait:                       ; wait
    jsr spull_0
    stz SHBG
    lda TEMP1
    ldx #SHC_WAIT
    jsr SH_CMD          ; (the status: the task's)
    bcc WTOK
    jmp IOFAIL
WTOK:
    jmp next
sem:                        ; sem
    jsr spull_0
    lda TEMP1
    ldy #0
SMNEW:
    jsr SEM_NEW
    bcc TKPUSH
    jmp IOFAIL
mutex:                      ; mutex
    lda #1
    ldy #SEM_MUTEX
    bra SMNEW
acquire:                    ; acquire
    jsr spull_0
    lda TEMP1
    jsr SEM_ACQUIRE
SMDONE:
    bcc TKOK
    jmp IOFAIL
acquireq:                   ; acquire?
    jsr spull_0
    lda TEMP1
    jsr SEM_TRY
    bcs :+
    jmp PUSHTRUE
:
    cmp #ERR_SEM_BUSY
    beq :+
    jmp IOFAIL
:
    jmp PUSHFALSE
release:                    ; release
    jsr spull_0
    lda TEMP1
    jsr SEM_RELEASE
    bra SMDONE
unsem:                      ; -sem
    jsr spull_0
    lda TEMP1
    jsr SEM_FREE
    bra SMDONE
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
;-------- The shell library's part of reading a line (hyforth.s: getline, through the gates LINE_START,
;         LINE_PROMPT and LINE_READ).  Without it loaded (LIB_SHELL in LIBSET), HyForth reads lines as a
;         plain Forth: a plain prompt, and no pipelines, redirection or boot.hys.
;
; A line is about to be read: the last line's redirection and pipe undone (whether or not the shell's
; library is loaded now: it was when that line began, if they're set up), and before the boot shell's first
; line, boot.hys from the selected volume's root (the current directory), if it's there; with no card, the
; ROM's, /rom/boot.hys (BOOTFLAG 2)
LINE_START:
    stz SHBG            ; (A new line: nothing started with & yet)
    jsr SH_UNREDIR
    jsr PIPEEND
    lda LIBSET
    and #LIB_SHELL
    beq LSDONE
    lda BOOTFLAG
    beq LSDONE
    stz BOOTFLAG
    cmp #2
    lda #<S_BOOTHYS
    ldy #>S_BOOTHYS
    bcc LSBOOT
    lda #<S_ROMBOOT
    ldy #>S_ROMBOOT
LSBOOT:
    jmp INCOPEN
LSDONE:
    rts
;
; The console's line is about to be read: the prompt.  The shell's, in its format (prompt: PROMPTFMT);
; without its library, a plain one
LINE_PROMPT:
    lda LIBSET
    and #LIB_SHELL
    beq LPBARE
    lda #<PROMPTFMT
    ldy #>PROMPTFMT
    jsr SH_PROMPT
    jmp LINE_EDIT
LPBARE:
    jsr WRITE_CRLF
    PRINT_CHAR #'>', #ASCII_SPACE
    ; (On into LINE_EDIT)
;
;---------------------------------------------------------------------
; The console's line editor: with fd 0 on the console, the line is typed with editing and history, and the
; console raw meanwhile (/dev/cons/ctl's rawon: this echoes).  Keys: Left and Right (Ctrl-B, Ctrl-F), Home
; and End (Ctrl-A, Ctrl-E), Backspace, Delete (Ctrl-D), Up and Down (Ctrl-P, Ctrl-N) for the lines typed
; before (HIST), Ctrl-U to erase the line, Enter.  (Ctrl-C and the console's other keys act as ever.)
; OUT: C = 0: the line is in TIB, from 1, and .Y = its length + 1 (as getline has it); C = 1: fd 0 isn't
; the console: getline reads the line itself
LINE_EDIT:
    lda IO_FD_SERVER        ; fd 0, open on the console?
    cmp #IO_FD_CLOSED
    beq LENOT
    lda IO_FD_FLAGS
    and #IO_FDF_CONS
    bne LEGO
LENOT:
    sec
    rts
LEGO:
    lda LECTL               ; Raw (still, after a break in the last line: its fd's still open)
    bpl LERAWON
    lda #<LE_CTL
    ldy #>LE_CTL
    ldx #IO_MODE_WRITE
    jsr IO_OPEN
    bcs LENOT               ; (No /dev/cons/ctl: as it was)
    sta LECTL
    lda #<LE_RAWON
    sta ZP_IO_BUF
    lda #>LE_RAWON
    sta ZP_IO_BUF + 1
    lda #5
    sta ZP_IO_CNT
    stz ZP_IO_CNT + 1
    lda LECTL
    jsr IO_WRITE
LERAWON:
    stz LELEN
    stz LEPOS
    lda HISTLEN
    sta LEHPOS
LEKEYS:
    jsr GET_CHAR
    bcc LEKEYS              ; (Nothing: the console waits on)
    cmp #ASCII_LF
    bne LENOTLF
    ldx LELEN               ; An LF: the line's end, but not the LF of a CR LF
    bne LELFEND
    ldx LASTCR
    beq LELFEND
    stz LASTCR
    bra LEKEYS
LELFEND:
    stz LASTCR
    jmp LEENTER
LENOTLF:
    cmp #ASCII_CR
    bne LENOTCR
    sta LASTCR
    jmp LEENTER
LENOTCR:
    cmp #ASCII_TAB          ; (A tab is a space)
    bne LENOTTAB
    lda #ASCII_SPACE
LENOTTAB:
    cmp #ASCII_SPACE
    bcc LECTRLKEY
    cmp #ASCII_DEL
    beq LEBSKEY
    bcs LEKEYS              ; (Not ASCII: nothing)
    jsr LEINSERT
    bra LEKEYS
LEBSKEY:
    lda #ASCII_BACKSPACE
LECTRLKEY:
    ldx #LE_KEYS_END - LE_KEYS - 1
LEFIND:
    cmp LE_KEYS, x
    beq LEACT
    dex
    bpl LEFIND
    cmp #ASCII_ESC
    beq LEESC
    cmp #ASCII_BELL         ; (Ctrl-G rings, as the console's echo had it; other control keys: nothing)
    bne LEKEYS
    jsr WRITE_CHAR
    jmp LEKEYS
LEESC:
    jsr GET_CHAR            ; ESC [ (or O) and a letter, or digits and ~
    cmp #'['
    beq LECSI
    cmp #'O'
    bne LEKEYS
LECSI:
    jsr GET_CHAR
    cmp #'A'
    bcc LEDIGITS
    ldx #LE_CSI_END - LE_CSI - 1
LEFINDCSI:
    cmp LE_CSI, x
    beq LEACTCSI
    dex
    bpl LEFINDCSI
    bra LEKEYS
LEDIGITS:
    sta LEOLD               ; (The first digit: 1 or 7 Home, 4 or 8 End, 3 Delete)
LETILDE:
    jsr GET_CHAR
    cmp #'~'
    beq LETILDED
    cmp #'0'
    bcs LETILDE
    jmp LEKEYS
LETILDED:
    lda LEOLD
    ldx #LE_TILDE_END - LE_TILDE - 1
LEFINDT:
    cmp LE_TILDE, x
    beq LEACTT
    dex
    bpl LEFINDT
    jmp LEKEYS
LEACTCSI:
    lda LE_CSI_ACT, x
    bra LEDO
LEACTT:
    lda LE_TILDE_ACT, x
    bra LEDO
LEACT:
    lda LE_KEYS_ACT, x
LEDO:                       ; .A = the action (LEA_*)
    asl
    tax
    jsr LEDISPATCH
    jmp LEKEYS
LEDISPATCH:
    jmp (LE_ACTIONS, x)
;
LEENTER:                    ; The line's done: it goes in the history
    jsr HIST_ADD
    lda LECTL               ; The console isn't raw any more (the fd's last close)
    jsr IO_CLOSE
    lda #$FF
    sta LECTL
    ldy LELEN
    iny
    clc
    rts
;
LE_CTL:
    .byte "/dev/cons/ctl", 0
; The keys: Ctrl-A .. and BS; ESC [ (or O) and a letter; ESC [ n ~
LEA_HOME = 0
LEA_LEFT = 1
LEA_RIGHT = 2
LEA_END = 3
LEA_BS = 4
LEA_DEL = 5
LEA_UP = 6
LEA_DOWN = 7
LEA_KILL = 8
LE_KEYS:
    .byte 1, 2, 6, 5, ASCII_BACKSPACE, 4, $10, $0E, $15
LE_KEYS_END:
LE_KEYS_ACT:
    .byte LEA_HOME, LEA_LEFT, LEA_RIGHT, LEA_END, LEA_BS, LEA_DEL, LEA_UP, LEA_DOWN, LEA_KILL
LE_CSI:
    .byte 'A', 'B', 'C', 'D', 'H', 'F'
LE_CSI_END:
LE_CSI_ACT:
    .byte LEA_UP, LEA_DOWN, LEA_RIGHT, LEA_LEFT, LEA_HOME, LEA_END
LE_TILDE:
    .byte '1', '7', '4', '8', '3'
LE_TILDE_END:
LE_TILDE_ACT:
    .byte LEA_HOME, LEA_HOME, LEA_END, LEA_END, LEA_DEL
LE_ACTIONS:
    .word LEHOME, LELEFT, LERIGHT, LEEND, LEBS, LEDELETE, LEUP, LEDOWN, LEKILL
;
; The cursor to the line's start, or end; one left, or right
LEHOME:
    lda LEPOS
    beq LERET
    jsr LELEFT
    bra LEHOME
LEEND:
    lda LEPOS
    cmp LELEN
    beq LERET
    jsr LERIGHT
    bra LEEND
LELEFT:
    lda LEPOS
    beq LERET
    dec LEPOS
    lda #ASCII_BACKSPACE
    jmp WRITE_CHAR
LERIGHT:
    ldy LEPOS
    cpy LELEN
    beq LERET
    iny
    sty LEPOS
    lda (TIB), y
    jmp WRITE_CHAR
LERET:
    rts
;
; Backspace: the character before the cursor goes; Delete: the one at it
LEBS:
    lda LEPOS
    beq LERET
    jsr LELEFT
LEDELETE:
    ldy LEPOS
    cpy LELEN
    beq LERET
LEDELMOVE:                  ; TIB[pos + 1 ..] = TIB[pos + 2 ..]
    iny
    cpy LELEN
    beq LEDELMOVED
    iny
    lda (TIB), y
    dey
    sta (TIB), y
    bra LEDELMOVE
LEDELMOVED:
    dec LELEN
    lda #1
    bra LESHOW
;
; Ctrl-U: the line goes
LEKILL:
    jsr LEHOME
    lda LELEN
    stz LELEN
    bra LESHOW
;
; .A typed: in at the cursor (if there's room: TIB's end less the one getline puts after the line)
LEINSERT:
    ldy LELEN
    iny
    iny
    cpy TIBEND
    bcs LERET
    pha
    ldy LELEN               ; TIB[pos + 2 ..] = TIB[pos + 1 ..], from the end
LEINSMOVE:
    cpy LEPOS
    beq LEINSMOVED
    lda (TIB), y
    iny
    sta (TIB), y
    dey
    dey
    bra LEINSMOVE
LEINSMOVED:
    iny
    pla
    sta (TIB), y
    inc LELEN
    jsr LERIGHT
    lda #0
;
; Write the line from the cursor on, then .A spaces (over what was there), and back to the cursor
LESHOW:
    sta LEPAD
    ldy LEPOS
LESHOWCH:
    cpy LELEN
    beq LESHOWPAD
    iny
    lda (TIB), y
    phy
    jsr WRITE_CHAR
    ply
    bra LESHOWCH
LESHOWPAD:
    lda LELEN               ; (The way back: what was written, and the spaces)
    sec
    sbc LEPOS
    clc
    adc LEPAD
    sta LECNT
    ldx LEPAD
    beq LESHOWBACK
LESHOWSP:
    lda #ASCII_SPACE
    jsr WRITE_CHAR
    dex
    bne LESHOWSP
LESHOWBACK:
    ldx LECNT
    beq LEDONE2
LESHOWBS:
    lda #ASCII_BACKSPACE
    jsr WRITE_CHAR
    dex
    bne LESHOWBS
LEDONE2:
    rts
;
; Up and Down: the line before (or after) the one shown, from the history; after the newest, an empty one
LEUP:
    ldx LEHPOS
    jsr HIST_PREV
    bcs LEDONE2
    bra LELOAD
LEDOWN:
    ldx LEHPOS
    cpx HISTLEN
    beq LEDONE2
    jsr HIST_NEXT
LELOAD:                     ; The line at HIST + .X (HISTLEN: an empty one) in place of the one typed
    stx LEHPOS
    phx
    jsr LEHOME
    plx
    lda LELEN
    sta LEOLD
    ldy #0
LELOADCH:
    cpx HISTLEN
    beq LELOADED
    lda HIST, x
    beq LELOADED
    iny
    sta (TIB), y
    inx
    bra LELOADCH
LELOADED:
    sty LELEN
    lda LEOLD               ; (Spaces over what's left of the old line)
    sec
    sbc LELEN
    bcs LELOADPAD
    lda #0
LELOADPAD:
    jsr LESHOW
    jmp LEEND
;
; The line typed (TIB, LELEN) into the history, unless it's empty or the newest line again; the oldest lines
; go to make room
HIST_ADD:
    lda LELEN
    beq LEDONE2
    ldx HISTLEN             ; The newest: the same?
    jsr HIST_PREV
    bcs HISTROOM
    ldy #0
HISTCMP:
    lda HIST, x
    beq HISTCMPEND
    iny
    cmp (TIB), y
    bne HISTROOM
    inx
    bra HISTCMP
HISTCMPEND:
    cpy LELEN
    beq LEDONE2             ; (The same)
HISTROOM:
    lda HISTLEN             ; Room for it and its 0?  (Up to 255 bytes: no carry)
    sec
    adc LELEN
    bcc HISTPUT
.assert HIST_SIZE = 255, error, "HIST_ADD's room check is the carry: HIST_SIZE must be 255"
HISTDROP:                   ; No: the oldest line goes
    ldx #0
    jsr HIST_NEXT           ; (.X = its length + 1)
    stx LECNT
    ldy #0
HISTSHIFT:
    cpx HISTLEN
    beq HISTSHIFTED
    lda HIST, x
    sta HIST, y
    inx
    iny
    bra HISTSHIFT
HISTSHIFTED:
    sty HISTLEN
    bra HISTROOM
HISTPUT:
    ldx HISTLEN
    ldy #0
HISTPUTCH:
    iny
    lda (TIB), y
    sta HIST, x
    inx
    cpy LELEN
    bne HISTPUTCH
    stz HIST, x
    inx
    stx HISTLEN
    rts
;
; HIST + .X is a line's start (or HISTLEN): .X = the line before's, C = 0; or C = 1, none
HIST_PREV:
    txa
    beq HISTNONE
    dex                     ; (The line before's 0)
HISTBACK:
    txa
    beq HISTFOUND
    lda HIST - 1, x
    beq HISTFOUND
    dex
    bra HISTBACK
HISTFOUND:
    clc
    rts
HISTNONE:
    sec
    rts
;
; HIST + .X is a line's start: .X = the next one's (after its 0)
HIST_NEXT:
    lda HIST, x
    inx
    cmp #0
    bne HIST_NEXT
    rts
;
; A line has been read (into TIB): with the shell's library, its pipeline's left side started (PIPECHK) and
; its redirection set up (SH_REDIR).  OUT: C = 0; or C = 1, .A = the IO error
LINE_READ:
    lda LIBSET
    and #LIB_SHELL
    beq LRNONE
    jsr PIPECHK
    bcs LRDONE
    jmp SH_REDIR
LRNONE:
    clc
LRDONE:
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
patch:                      ; patch
    lda #SND_R_PATCH
    bra SNDCMD2
note:                       ; note
    lda #SND_R_NOTE
SNDCMD2:              ; ( v ch -- ): the library's command .A for channel ch, value v
    sta TEMP4         ; (the pairs, in TEMP3-TEMP4: SND_R_CH ch, then the command v)
    jsr spull_0       ; TEMP1 = ch
    jsr spull_1       ; TEMP2 = v
    lda TEMP2
SNDCMD:               ; the command in TEMP4, its value .A, the channel TEMP1: one write to /dev/snd
    sta TEMP4+1
    lda #SND_R_CH
    sta TEMP3
    lda TEMP1
    sta TEMP3+1
    jsr SNDOPEN
    sta TEMP1         ; the fd
    lda #<TEMP3
    sta ZP_IO_BUF
    stz ZP_IO_BUF+1
    lda #4
    sta ZP_IO_CNT
    stz ZP_IO_CNT+1
    lda TEMP1
    jsr IO_WRITE
    jmp SNDCLOSE
noteoff:                    ; noteoff
    lda #SND_R_OFF
    sta TEMP4
    jsr spull_0       ; TEMP1 = ch
    lda #0
    bra SNDCMD
play:                       ; play
    jsr ARGGET
    bcc PLAYARGS
    lda #ERR_IO_NAME
    jmp IOFAIL
PLAYARGS:             ; the rest of the line: how many times to play its loop (and &)
    jsr ARGREST
    lda #<ARGBUF      ; (the song: ARGGET's)
    ldy #>ARGBUF
    ldx #SHC_PLAY
    jsr SH_CMD
    bcc :+
    jmp IOFAIL
:
    jmp next
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
    jsr ERRSTAT         ; the exit status: this error's
    lda ERRFLAG         ; An IO error: what it was (" !IO ERR! not found")
    cmp #ERR_IO
    bne ERRLINE
    lda IOERR
    jsr ERRWHY
ERRLINE:
    WCRLF_np
ERREND:
    lda #0
    sta ERRFLAG
    sta ERRPTR
    sta ERRPTR+1
    rts
;
;
; An error's exit status: its number (ERRFLAG), and its text (TEMP0: WERR's) without the " !" and "!" around it
; (" !UNK WORD!": "UNK WORD"), in HYSTAT and HYSTATMSG, and $status (SH_CMD: SHC_STATUS)
ERRSTAT:
    lda ERRFLAG
    sta HYSTAT
    ldy #0
    ldx #0
ESSKIP:
    lda (TEMP0),y
    beq ESEND
    cmp #ASCII_SPACE
    beq ESNEXT
    cmp #'!'
    bne ESCOPY
ESNEXT:
    iny
    bra ESSKIP
ESCOPY:
    lda (TEMP0),y
    beq ESEND
    cmp #'!'
    beq ESEND
    sta HYSTATMSG,x
    inx
    iny
    cpx #EXIT_MSG_MAX
    bcc ESCOPY
ESEND:
    stz HYSTATMSG,x
    ldx #SHC_STATUS
    jmp SH_CMD
;
; The OS's error .A, briefly, after a space (ERR_WHY: an error code, then its text; unknown: $ and the code)
ERRWHY:
    sta TEMP1
    PRINT_SPACE
    lda #<ERR_WHY
    sta TEMP2
    lda #>ERR_WHY
    sta TEMP2 + 1
EWFIND:
    lda (TEMP2)
    beq EWHEX                       ; (The table's end)
    cmp TEMP1
    beq EWFOUND
EWSKIP:                             ; (Past this one's text)
    jsr EWNEXT
    lda (TEMP2)
    bne EWSKIP
    jsr EWNEXT
    bra EWFIND
EWFOUND:
    jsr EWNEXT
    lda (TEMP2)
    beq EWDONE
    jsr WRITE_CHAR
    bra EWFOUND
EWNEXT:
    inc TEMP2
    bne EWDONE
    inc TEMP2 + 1
    rts
EWHEX:
    PRINT_CHAR #'$'
    lda TEMP1
    jmp WRITE_BYTE
EWDONE:
    rts
.pushseg
.segment "FORTH_ROM_DATA"           ; (Paged ROM bank 0, which HyForth's task has: there's no room on page A)
ERR_WHY:
    .byte ERR_IO_NOT_FOUND, "not found", 0
    .byte ERR_IO_BAD_FD, "bad fd", 0
    .byte ERR_IO_MODE, "not opened for that", 0
    .byte ERR_IO_WOULD_BLOCK, "would wait", 0
    .byte ERR_IO_EOF, "end of file", 0
    .byte ERR_IO_NO_FDS, "no fds left", 0
    .byte ERR_IO_NO_DEVS, "device table full", 0
    .byte ERR_IO_NAME, "bad name", 0
    .byte ERR_IO_BAD_REQ, "not supported", 0
    .byte ERR_IO_DEVICE, "no answer", 0
    .byte ERR_IO_BROKEN, "broken pipe", 0
    .byte ERR_IO_NO_PIPES, "no pipes left", 0
    .byte ERR_IO_NS_FULL, "namespace full", 0
    .byte ERR_IO_NS_LOOP, "bind loop", 0
    .byte ERR_IO_NOT_READY, "not ready", 0
    .byte ERR_IO_MEDIA, "media error", 0
    .byte ERR_IO_NOT_FS, "no HydraFS", 0
    .byte ERR_IO_FULL, "disk full", 0
    .byte ERR_IO_EXISTS, "exists", 0
    .byte ERR_IO_NOT_EMPTY, "not empty", 0
    .byte ERR_IO_BUSY, "busy", 0
    .byte ERR_IO_NOT_DIR, "not a directory", 0
    .byte ERR_IO_IS_DIR, "a directory", 0
    .byte ERR_IO_NOT_EXEC, "not executable", 0
    .byte ERR_IO_PERM, "not allowed", 0
    .byte ERR_SEM_BAD, "no such semaphore", 0
    .byte ERR_SEM_NONE, "no semaphores left", 0
    .byte ERR_SEM_BUSY, "none to take", 0
    .byte ERR_SEM_NOT_HELD, "not held", 0
    .byte ERR_SEM_FULL, "count full", 0
    .byte ERR_NO_TASKS_AVAILABLE, "no tasks left", 0
    .byte ERR_TASK_BUSY, "task busy", 0
    .byte ERR_BAD_TASK, "bad task", 0
    .byte 0
.popseg
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
