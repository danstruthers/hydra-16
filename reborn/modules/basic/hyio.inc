; ****************************************************************************
; hyio.inc - BASIC on the system (included by basic.s): its start, its output (fd 1, buffered, or SAVE's file), its
; lines (stdin through a buffer: the console's cooked lines, or a file's or a pipe's; or the file LOAD reads), GET's
; keys (the console's raw mode), Ctrl-C (a note: intr, which NEWSTT's ISCNTC takes), the error messages' text (the
; system's in capitals), BYE, LOAD, SAVE, RUN "name" and scripts (basic file: the file LOADed and run, then the end).
; What was the 8-bit machines' MONCOUT, MONRDKEY, INLIN, ISCNTC, cold start and LOAD and SAVE.

; r = a 16-bit value (a call register)
.macro LDR reg, value
            lda         #<(value)
            sta         reg
            lda         #>(value)
            sta         reg + 1
.endmacro

OBUF_SIZE   = 128                                           ; The output's buffer
IBUF_SIZE   = 128                                           ; stdin's (a stat record fits, SR_SIZE) and LOAD's
NAME_MAX    = 64                                            ; A file's name, and its 0
HDR_LEN     = 10                                            ; A tokenized program's header: "HYBAS", 1, the keyword
HDR_VERSION = 1                                             ;   table's sum (2: a program from another BASIC's table
                                                            ;   is BAD FILE), its length (2); then its lines as in memory

.bss
interactive: .res       1                                   ; <> 0: stdin is the console (the banner, OK, GET's raw keys)
intr:       .res        1                                   ; $80: Ctrl-C came (the note handler's), for ISCNTC
argp:       .res        2                                   ; BASIC's arguments (main's r0)
script:     .res        1                                   ; <> 0: running a script (basic file): RESTART is the end ...
exit_code:  .res        1                                   ;   with this code (1 after an error or BREAK)
obuf:       .res        OBUF_SIZE                           ; The output waiting ...
olen:       .res        1                                   ;   its bytes ...
outfd:      .res        1                                   ;   and its fd: 1, or SAVE's file's
ibuf:       .res        IBUF_SIZE                           ; stdin's bytes read ahead ...
ilen:       .res        1                                   ;   how many ...
ipos:       .res        1                                   ;   the next ...
skip_lf:    .res        1                                   ;   <> 0: a line ended with CR (its LF dropped)
ldfd:       .res        1                                   ; The file LOAD reads ($FF: none; else the lines come from
lbuf:       .res        IBUF_SIZE                           ;   it, through its own buffer) ...
llen:       .res        1
lpos:       .res        1
ld_run:     .res        1                                   ;   <> 0: RUN after it (RUN "name", a script)
nolf1:      .res        1                                   ; <> 0: LIST_LINES' first line with no new line before it
tokflag:    .res        1                                   ; <> 0: SAVE's ,B (tokenized)
os_err:     .res        1                                   ; ERR_OS's error: the system's code
pathbuf:    .res        NAME_MAX                            ; A file's name (GETNAME's)
errbuf:     .res        32                                  ; ERRSTR's text
hdr:        .res        HDR_LEN                             ; A tokenized program's header
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
; did (no questions: its memory is the task's RAM to MEM_END, the width 80), Ctrl-C a note; then a script (basic
; file: LOADed and run), or the banner at the console and the warm start, RESTART
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
            sta         ldfd
            lda         #1
            sta         outfd
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
            jsr         SCRTCH
            lda         argp                                ; A script's name?  LOADed and run
            sta         IOPTR
            ora         argp + 1
            beq         @prompt
            lda         argp + 1
            sta         IOPTR+1
            lda         (IOPTR)
            beq         @prompt
            ldy         #0
:
            lda         (IOPTR),y
            sta         pathbuf,y
            beq         :+
            iny
            cpy         #NAME_MAX
            bcc         :-
            lda         #E_NAMETOOLONG
            jmp         OSERR
:
            inc         script
            inc         ld_run
            jmp         LOAD_PATH
@prompt:
            lda         interactive
            beq         :+
            jsr         banner
:
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

; RESTART's: the files LOAD and SAVE had closed (an error, Ctrl-C, its end), the output fd 1 again, and a script's
; end (EXITS with its code)
IO_RESET:
            jsr         FLUSH
            lda         outfd
            cmp         #1
            beq         :+
            jsr         CLOSE
            lda         #1
            sta         outfd
:
            lda         ldfd
            bmi         :+
            jsr         CLOSE
            lda         #$FF
            sta         ldfd
            stz         skip_lf
:
            stz         ld_run
            stz         nolf1
            lda         script
            beq         :+
            lda         exit_code
            jmp         EXIT_CODE
:
            rts

; ---- Output

; .A out: into the output's buffer, which goes to its fd (outfd) at a LF or when it's full (MONCOUT).  Keeps .A, .X,
; .Y
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

; The output waiting, to its fd.  Keeps .A, .X, .Y
FLUSH:
            pha
            lda         olen
            beq         @done
            phx
            phy
            sta         r1
            stz         r1 + 1
            LDR         r0, obuf
            lda         outfd
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

; The system's error os_err out, its text (ERRSTR's) in capitals, as BASIC's are: ?NOT FOUND ERROR
OS_ERRTEXT:
            LDR         r0, errbuf
            lda         os_err
            jsr         ERRSTR
            ldy         #0
@char:
            lda         errbuf,y
            beq         @done
            cmp         #'a'
            bcc         :+
            cmp         #'z'+1
            bcs         :+
            and         #$DF
:
            jsr         OUTDO
            iny
            cpy         #32
            bcc         @char
@done:
            rts

; ERROR for the system's error .A
OSERR:
            sta         os_err
            ldx         #ERR_OS
            jmp         ERROR

; ---- Input

; The next byte of the lines' source: the file LOAD reads, or stdin (its buffer filled with a READ when it's empty:
; the output sent and the console cooked first).  OUT: C = 0, .A = it; C = 1, .A = E_EOF (its end) or the error
; (E_INTR: a note).  Keeps .Y
IN_BYTE:
            bit         ldfd
            bmi         @stdin
            ldx         lpos
            cpx         llen
            bcc         @lhave
            phy
            LDR         r0, lbuf
            LDR         r1, IBUF_SIZE
            lda         ldfd
            jsr         READ
            ply
            bcs         @done
            sta         llen
            stz         lpos
            ldx         #0
            cmp         #0
            bne         @lhave
            bra         @eof
@lhave:
            lda         lbuf,x
            inx
            stx         lpos
            clc
            rts
@stdin:
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
@eof:
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

; A line into INPUTBUFFER, 0 after it (INBUF_MAX characters, the rest dropped; LF, CR or CR LF ends it; a tab a
; space, other control characters dropped): stdin's (the console edits and echoes its lines itself), or the file
; LOAD reads, whose end is LOAD's (LOAD_DONE).  stdin's end: in direct mode, BASIC's (BYE); in a program, an empty
; line.  Ctrl-C while it waits: in direct mode, the line again; in a program, an empty line (which stops INPUT,
; Microsoft's way).  OUT: .X/.Y = INPUTBUFFER-1, for CHRGET
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
            bit         ldfd                                ; (LOAD's file: its end, or an error, its end too)
            bmi         :+
            cmp         #E_EOF
            jeq         LOAD_DONE
            jmp         OSERR
:
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
            lda         #0
; BASIC's end with code .A
EXIT_CODE:
            pha
            jsr         FLUSH
            jsr         RAW_OFF
            stz         r0
            stz         r0 + 1
            pla
            jmp         EXITS

; ---- LOAD, SAVE, RUN "name"

; A file's name, the string expression at TXTPTR, into pathbuf, 0 after it (NAME_MAX - 1 characters: more, STRING TOO
; LONG)
GETNAME:
            jsr         FRMEVL
            jsr         CHKSTR
            jsr         FREFAC                              ; (.A its length, INDEX its characters)
            cmp         #NAME_MAX
            bcc         :+
            ldx         #ERR_STRLONG
            jmp         ERROR
:
            tay
            lda         #0
            sta         pathbuf,y
:
            dey
            bmi         :+
            lda         (INDEX),y
            sta         pathbuf,y
            bra         :-
:
            rts

; "RUN" "name": LOAD's, then RUN
RUN_FILE:
            jsr         GETNAME
            inc         ld_run
            bra         LOAD_PATH

; "LOAD" STATEMENT: LOAD "name": the program in the file pathbuf names in place of this one: a text (its lines with
; a number, as if typed: others, a #! line say, passed over), or a tokenized program (SAVE's ,B: its header, then its
; lines as they were in memory).  A text's lines come through the main loop (L2351: INLIN reads them from ldfd), and
; its end (INLIN's) is LOAD_DONE
LOAD:
            jsr         GETNAME
LOAD_PATH:
            LDR         r0, pathbuf
            lda         #O_READ
            jsr         OPEN
            bcc         :+
            jmp         OSERR
:
            sta         ldfd
            stz         lpos
            stz         skip_lf
            LDR         r0, lbuf                            ; Its first bytes: a tokenized program's header?
            LDR         r1, IBUF_SIZE
            lda         ldfd
            jsr         READ
            bcc         :+
            jmp         OSERR
:
            sta         llen
            cmp         #HDR_LEN
            bcc         @text
            ldx         #HDR_MAGIC_LEN-1
:
            lda         lbuf,x
            cmp         s_magic,x
            bne         @text
            dex
            bpl         :-
            jmp         LOAD_TOKENS
@text:
            jsr         SCRTCH                              ; (NEW)
            ldx         #STACK_TOP
            txs
            jmp         L2351

; A text's end: the program LOADed, and RUN if it's RUN "name"'s or a script's
LOAD_DONE:
            lda         ldfd
            jsr         CLOSE
            lda         #$FF
            sta         ldfd
            stz         skip_lf
LOAD_END:
            lda         ld_run
            stz         ld_run
            bne         :+
            jmp         RESTART
:
            jsr         SETPTRS                             ; (RUN's: from the first line, the variables cleared)
            jmp         NEWSTT

; A tokenized program (lbuf has its first llen bytes): its header checked, its lines read to TXTTAB, linked again
LOAD_TOKENS:
            lda         lbuf + 5
            cmp         #HDR_VERSION
            jne         @bad
            jsr         TABSUM
            cmp         lbuf + 6
            jne         @bad
            cpy         lbuf + 7
            jne         @bad
            clc                                             ; Room for it?
            lda         TXTTAB
            adc         lbuf + 8
            sta         INDEX
            lda         TXTTAB+1
            adc         lbuf + 9
            sta         INDEX+1
            bcs         @full
            cmp         MEMSIZ+1
            bcs         @full
            jsr         SCRTCH
            lda         TXTTAB                              ; What lbuf has after the header, then the rest read
            sta         DEST
            lda         TXTTAB+1
            sta         DEST+1
            ldx         #HDR_LEN
@copy:
            cpx         llen
            bcs         @rest
            lda         DEST                                ; (Not past its length)
            cmp         INDEX
            lda         DEST+1
            sbc         INDEX+1
            bcs         @linked
            lda         lbuf,x
            sta         (DEST)
            inc         DEST
            bne         :+
            inc         DEST+1
:
            inx
            bra         @copy
@rest:
            lda         DEST
            sta         r0
            lda         DEST+1
            sta         r0 + 1
            sec
            lda         INDEX
            sbc         DEST
            sta         r1
            lda         INDEX+1
            sbc         DEST+1
            sta         r1 + 1
            ora         r1
            beq         @linked
            lda         ldfd
            jsr         READ
            bcc         @linked
            jmp         OSERR
@linked:
            lda         ldfd
            jsr         CLOSE
            lda         #$FF
            sta         ldfd
            lda         INDEX                               ; The variables after it, the lines linked again
            ldy         INDEX+1
            sta         VARTAB
            sty         VARTAB+1
            jsr         LE33D
            jmp         LOAD_END
@full:
            jmp         MEMERR
@bad:
            ldx         #ERR_BADFILE
            jmp         ERROR

; .A/.Y = the keyword table's sum (its bytes added, to its 0): a tokenized program's tokens are this table's
TABSUM:
            jsr         KW_FIRST
            stz         INDEX
            stz         INDEX+1
            ldy         #0
@byte:
            lda         (KWPTR),y
            beq         @done
            clc
            adc         INDEX
            sta         INDEX
            bcc         :+
            inc         INDEX+1
:
            inc         KWPTR
            bne         @byte
            inc         KWPTR+1
            bra         @byte
@done:
            lda         INDEX
            ldy         INDEX+1
            rts

s_magic:    .byte       "HYBAS"
HDR_MAGIC_LEN = * - s_magic

; "SAVE" STATEMENT: SAVE "name": the program to the file pathbuf names (made, or emptied first), as text (LIST's:
; a line each, its number first); SAVE "name",B: tokenized (LOAD_TOKENS' header, and the lines as they're in memory)
SAVE:
            jsr         GETNAME
            stz         tokflag
            jsr         CHRGOT
            cmp         #','
            bne         :+
            jsr         CHRGET
            cmp         #'B'
            jne         SYNERR
            jsr         CHRGET
            inc         tokflag
:
            jsr         FLUSH                               ; (What's for the console first)
            LDR         r0, pathbuf
            lda         #O_WRITE | O_TRUNC
            jsr         OPEN
            bcc         @open
            cmp         #E_NOENT
            bne         @fail
            LDR         r0, pathbuf
            lda         #O_WRITE
            ldx         #0
            jsr         CREATE
            bcc         @open
@fail:
            jmp         OSERR
@open:
            sta         outfd
            lda         tokflag
            bne         @tokens
            stz         LINNUM                              ; Text: the lines LISTed into it
            stz         LINNUM+1
            jsr         FNDLIN
            lda         #$FF
            sta         LINNUM
            sta         LINNUM+1
            sta         nolf1
            jsr         LIST_LINES
            lda         nolf1                               ; (The last line's LF, if there was one)
            bne         @close
            jsr         CRDO
            bra         @close
@tokens:
            ldx         #HDR_MAGIC_LEN-1                    ; Tokenized: its header, and the lines
:
            lda         s_magic,x
            sta         hdr,x
            dex
            bpl         :-
            lda         #HDR_VERSION
            sta         hdr + 5
            jsr         TABSUM
            sta         hdr + 6
            sty         hdr + 7
            sec
            lda         VARTAB
            sbc         TXTTAB
            sta         hdr + 8
            lda         VARTAB+1
            sbc         TXTTAB+1
            sta         hdr + 9
            LDR         r0, hdr
            LDR         r1, HDR_LEN
            lda         outfd
            jsr         WRITE
            jcs         @fail
            lda         TXTTAB
            sta         r0
            lda         TXTTAB+1
            sta         r0 + 1
            lda         hdr + 8
            sta         r1
            lda         hdr + 9
            sta         r1 + 1
            lda         outfd
            jsr         WRITE
            jcs         @fail
@close:
            jsr         FLUSH
            lda         outfd
            jsr         CLOSE
            lda         #1
            sta         outfd
            rts
