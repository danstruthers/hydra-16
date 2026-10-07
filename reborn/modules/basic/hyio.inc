; ****************************************************************************
; hyio.inc - BASIC on the system (included by basic.s): its start, its output (fd 1, buffered), its lines (stdin,
; through a buffer: the console's cooked lines, or a file's or a pipe's), GET's keys (the console's raw mode), Ctrl-C
; (a note: intr, which NEWSTT's ISCNTC takes), the error messages' text, and BYE.  What was the 8-bit machines'
; MONCOUT, MONRDKEY, INLIN, ISCNTC and cold start.

; r = a 16-bit value (a call register)
.macro LDR reg, value
            lda         #<(value)
            sta         reg
            lda         #>(value)
            sta         reg + 1
.endmacro

OBUF_SIZE   = 128                                           ; The output's buffer
IBUF_SIZE   = 128                                           ; stdin's (a stat record fits, SR_SIZE)

.bss
interactive: .res       1                                   ; <> 0: stdin is the console (the banner, OK, GET's raw keys)
intr:       .res        1                                   ; $80: Ctrl-C came (the note handler's), for ISCNTC
argp:       .res        2                                   ; BASIC's arguments (main's r0)
obuf:       .res        OBUF_SIZE                           ; The output waiting ...
olen:       .res        1                                   ;   its bytes
ibuf:       .res        IBUF_SIZE                           ; stdin's bytes read ahead ...
ilen:       .res        1                                   ;   how many ...
ipos:       .res        1                                   ;   the next ...
skip_lf:    .res        1                                   ;   <> 0: a line ended with CR (its LF dropped)
raw:        .res        1                                   ; <> 0: the console in raw mode (GET's)
ctlfd:      .res        1                                   ; /dev/consctl's fd, $FF: not open
keyfd:      .res        1                                   ; /dev/cons's, read without waiting (GET's), $FF: not open
keybuf:     .res        1

; The note handler, in RAM: Ctrl-C (NOTE_INTERRUPT) noted in intr, for the next statement (ISCNTC) or wait; another,
; the default
.segment "DATA"
notes:
            cmp         #NOTE_INTERRUPT
            bne         :+
            lda         #$80
            sta         intr
            clc
            rts
:
            sec
            rts

.code
; ****************************************************************************
; The start: the stack BASIC's, its memory claimed (BREAK) and the vectors and tables set as Microsoft's cold start
; did (no questions: its memory is the task's RAM to MEM_END, the width 80), Ctrl-C a note; the banner at the console;
; then the warm start, RESTART
main:
            lda         r0
            sta         argp
            lda         r0 + 1
            sta         argp + 1
            ldx         #$FF
            stx         CURLIN+1                            ; (Direct mode)
            txs
            lda         #0                                  ; (The stack's bottom: 0, where the frames' scans stop)
            pha
            LDR         r0, MEM_END
            jsr         BREAK
            bcc         :+
            stz         r0                                  ; (Not so much: as it is)
            stz         r0 + 1
            jsr         BREAK
:
            lda         r0
            ldy         r0 + 1
            sta         MEMSIZ
            sty         MEMSIZ+1
            sta         FRETOP
            sty         FRETOP+1
            LDR         r0, notes
            jsr         NOTIFY
            lda         #$FF
            sta         ctlfd
            sta         keyfd
            LDR         r0, ibuf                            ; stdin the console?  (Its stat record, in ibuf: empty yet)
            lda         #0
            jsr         FSTAT
            bcs         :+
            lda         ibuf + SR_DEV
            cmp         #'c'
            bne         :+
            inc         interactive
:
            lda         #$4C                                ; The vectors: jmp
            sta         GORESTART
            sta         GOSTROUT
            sta         JMPADRS
            sta         USR
            lda         #<RESTART
            ldy         #>RESTART
            sta         GORESTART+1
            sty         GORESTART+2
            lda         #<STROUT
            ldy         #>STROUT
            sta         GOSTROUT+1
            sty         GOSTROUT+2
            lda         #<AYINT
            ldy         #>AYINT
            sta         GOAYINT
            sty         GOAYINT+1
            lda         #<GIVAYF
            ldy         #>GIVAYF
            sta         GOGIVEAYF
            sty         GOGIVEAYF+1
            lda         #<IQERR
            ldy         #>IQERR
            sta         USR+1
            sty         USR+2
            lda         #WIDTH                              ; The width, and the last comma field's start (Microsoft's
            sta         Z17                                 ;   sum: 70 for 80)
            sec
:
            sbc         #$0E
            bcs         :-
            eor         #$FF
            sbc         #$0C
            clc
            adc         Z17
            sta         Z18
            ldx         #BYTES_FP-1                         ; RND's seed
:
            lda         GENERIC_RNDSEED,x
            sta         RNDSEED,x
            dex
            bpl         :-
            lda         #$03
            sta         DSCLEN
            stz         SHIFTSIGNEXT
            stz         LASTPT+1
            stz         Z14
            ldx         #TEMPST
            stx         TEMPPT
            lda         #<BASIC_START                       ; The program from the BSS's end, a 0 before it
            ldy         #>BASIC_START
            sta         TXTTAB
            sty         TXTTAB+1
            lda         #0
            sta         BASIC_START-1
            lda         interactive
            beq         :+
            jsr         banner
:
            jsr         SCRTCH
            jmp         RESTART

BASIC_START = __BSS_RUN__ + __BSS_SIZE__ + 1

; The banner, and the bytes free
banner:
            lda         #<s_banner
            ldy         #>s_banner
            jsr         STROUT
            lda         MEMSIZ
            sec
            sbc         TXTTAB
            tax
            lda         MEMSIZ+1
            sbc         TXTTAB+1
            jsr         LINPRT
            lda         #<s_free
            ldy         #>s_free
            jmp         STROUT

s_banner:   .byte       "EHYBASIC FOR THE HYDRA-16 (MICROSOFT BASIC 2A)", LF, 0
s_free:     .byte       " BYTES FREE", LF, 0

; ---- Output

; .A out: into the output's buffer, which goes to fd 1 at a LF or when it's full (MONCOUT).  Keeps .A, .X, .Y
MONCOUT:
            phx
            ldx         olen
            sta         obuf,x
            inx
            stx         olen
            cmp         #LF
            beq         :+
            cpx         #OBUF_SIZE
            bcc         @done
:
            jsr         FLUSH
@done:
            plx
            rts

; The output waiting, to fd 1.  Keeps .A, .X, .Y
FLUSH:
            pha
            lda         olen
            beq         @done
            phx
            phy
            sta         r1
            stz         r1 + 1
            LDR         r0, obuf
            lda         #1
            jsr         WRITE
            stz         olen
            ply
            plx
@done:
            pla
            rts

; The string at .A/.Y out, its last character's bit 7 set (htasc's: the error messages)
OUTHTASC:
            sta         IOPTR
            sty         IOPTR+1
            ldy         #0
:
            lda         (IOPTR),y
            pha
            and         #$7F
            jsr         OUTDO
            pla
            bmi         :+
            iny
            bne         :-
:
            rts

; ---- Input

; stdin's next byte, its buffer filled with a READ when it's empty (the output sent and the console cooked first).
; OUT: C = 0, .A = it; C = 1, .A = E_EOF (its end) or the error (E_INTR: a note).  Keeps .Y
IN_BYTE:
            ldx         ipos
            cpx         ilen
            bcc         @have
            jsr         FLUSH
            jsr         RAW_OFF
            phy
            LDR         r0, ibuf
            LDR         r1, IBUF_SIZE
            lda         #0
            jsr         READ
            ply
            bcs         @done
            sta         ilen
            stz         ipos
            ldx         #0
            cmp         #0
            bne         @have
            lda         #E_EOF
            sec
@done:
            rts
@have:
            lda         ibuf,x
            inx
            stx         ipos
            clc
            rts

; A line of stdin into INPUTBUFFER, 0 after it (INBUF_MAX characters, the rest dropped; LF, CR or CR LF ends it; a
; tab a space, other control characters dropped).  The console edits and echoes its lines itself.  Its end: in direct
; mode, BASIC's (BYE); in a program, an empty line.  Ctrl-C while it waits: in direct mode, the line again; in a
; program, an empty line (which stops INPUT, Microsoft's way).  OUT: .X/.Y = INPUTBUFFER-1, for CHRGET
INLIN:
            ldy         #0
@char:
            jsr         IN_BYTE
            bcs         @fail
            ldx         skip_lf
            stz         skip_lf
            cmp         #LF
            bne         :+
            cpx         #0
            bne         @char                               ; (A CR LF's LF)
            bra         @end
:
            cmp         #CR
            bne         :+
            inc         skip_lf
            bra         @end
:
            cmp         #TAB
            bne         :+
            lda         #' '
:
            cmp         #' '
            bcc         @char
            cpy         #INBUF_MAX
            bcs         @char
            sta         INPUTBUFFER,y
            iny
            bra         @char
@fail:
            cmp         #E_INTR
            bne         @eof
            ldx         CURLIN+1                            ; (In a program: intr kept, for INPUT's BREAK)
            inx
            bne         @empty
            stz         intr
            jsr         CRDO
            jmp         INLIN
@eof:
            ldx         CURLIN+1
            inx
            jeq         BYE_EXIT
@empty:
            ldy         #0
@end:
            lda         #0
            sta         INPUTBUFFER,y
            stz         POSX
            ldx         #<(INPUTBUFFER-1)
            ldy         #>(INPUTBUFFER-1)
            rts

; GET's key: the console's, in raw mode, not waited for (0: none); if stdin isn't the console, its next byte (its end:
; 0)
GETKEY:
            lda         interactive
            bne         @key
            jsr         IN_BYTE
            bcc         :+
            lda         #0
:
            rts
@key:
            jsr         FLUSH
            jsr         RAW_ON
            lda         keyfd
            bpl         :+
            LDR         r0, s_cons
            lda         #O_READ | O_NONBLOCK
            jsr         OPEN
            bcs         @none
            sta         keyfd
:
            LDR         r0, keybuf
            LDR         r1, 1
            lda         keyfd
            jsr         READ
            bcs         @none
            cmp         #0
            beq         @none
            lda         keybuf
            rts
@none:
            lda         #0
            rts

; The console in raw mode (GET's), till a line's wanted (RAW_OFF)
RAW_ON:
            lda         raw
            bne         @done
            lda         ctlfd
            bpl         :+
            LDR         r0, s_consctl
            lda         #O_WRITE
            jsr         OPEN
            bcs         @done
            sta         ctlfd
:
            LDR         r0, s_rawon
            LDR         r1, 5
            lda         ctlfd
            jsr         WRITE
            inc         raw
@done:
            rts

; The console cooked again, for a line.  Keeps .Y
RAW_OFF:
            lda         raw
            beq         @done
            stz         raw
            phy
            LDR         r0, s_rawoff
            LDR         r1, 6
            lda         ctlfd
            jsr         WRITE
            ply
@done:
            rts

s_consctl:  .byte       "/dev/consctl", 0
s_cons:     .byte       "/dev/cons", 0
s_rawon:    .byte       "rawon"
s_rawoff:   .byte       "rawoff"

; ---- BYE

; "BYE" STATEMENT: BASIC's end, code 0 (stdin's end in direct mode too)
BYE:
BYE_EXIT:
            jsr         FLUSH
            jsr         RAW_OFF
            stz         r0
            stz         r0 + 1
            lda         #0
            jmp         EXITS

; ---- LOAD and SAVE: to come
LOAD:
SAVE:
            rts
