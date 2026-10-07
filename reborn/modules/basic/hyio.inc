; ****************************************************************************
; hyio.inc - BASIC on the system (included by basic.s): its start; its output (fd 1, buffered; or SAVE's file, or a
; channel's for PRINT#); its input from sources, each an fd and a buffer: stdin (the console's cooked lines, or a
; file's or a pipe's), the file LOAD reads, and the files OPEN opens (channels 1 to CH_MAX: INPUT#, GET#); GET's keys
; (the console's raw mode); Ctrl-C (a note: intr, which NEWSTT's ISCNTC takes); the error messages' text (the
; system's in capitals); BYE, LOAD, SAVE, RUN "name" and scripts (basic file: the file LOADed and run, then the end);
; OPEN, CLOSE, PRINT#, INPUT#, GET# and EOF.  What was the 8-bit machines' MONCOUT, MONRDKEY, INLIN, ISCNTC, cold
; start and LOAD and SAVE.

; r = a 16-bit value (a call register)
.macro LDR reg, value
            lda         #<(value)
            sta         reg
            lda         #>(value)
            sta         reg + 1
.endmacro

OBUF_SIZE   = 128                                           ; The output's buffer
SBUF_SIZE   = 128                                           ; A source's (stdin's: a stat record fits, SR_SIZE)
NAME_MAX    = 64                                            ; A file's name, and its 0
CH_MAX      = 4                                             ; Channels: OPEN's files, 1 to CH_MAX
SRC_STDIN   = 0                                             ; The sources: stdin ...
SRC_LOAD    = 1                                             ;   the file LOAD reads ...
SRC_CH      = 1                                             ;   channel n's (SRC_CH + n)
SRC_N       = SRC_CH + CH_MAX + 1
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
outfd:      .res        1                                   ;   and its fd: 1, SAVE's file's, or a channel's ...
posx_save:  .res        1                                   ;   (the console's column, kept over PRINT#)
save_fd:    .res        1                                   ; SAVE's file ($FF: none)
src_fd:     .res        SRC_N                               ; The sources: each one's fd ($FF: none; stdin 0) ...
src_len:    .res        SRC_N                               ;   its buffer's bytes ...
src_pos:    .res        SRC_N                               ;   the next ...
src_skip:   .res        SRC_N                               ;   <> 0: its last line ended with CR (its LF dropped) ...
src_bufs:   .res        SRC_N * SBUF_SIZE                   ;   its buffer
cur_src:    .res        1                                   ; INLIN's and GET's source: stdin, LOAD's file, INPUT#'s
ldfd        = src_fd + SRC_LOAD                             ; (LOAD's file, $FF: none: program.inc's L2351 asks)
ld_run:     .res        1                                   ; <> 0: RUN after LOAD (RUN "name", a script)
nolf1:      .res        1                                   ; <> 0: LIST_LINES' first line with no new line before it
tokflag:    .res        1                                   ; <> 0: SAVE's ,B (tokenized)
os_err:     .res        1                                   ; ERR_OS's error: the system's code
pathbuf:    .res        NAME_MAX                            ; A file's name (GETNAME's)
errbuf:     .res        32                                  ; ERRSTR's text
hdr:        .res        HDR_LEN                             ; A tokenized program's header
chsrc:      .res        1                                   ; OPEN's channel's source
raw:        .res        1                                   ; <> 0: the console in raw mode (GET's)
ctlfd:      .res        1                                   ; /dev/consctl's fd, $FF: not open
keyfd:      .res        1                                   ; /dev/cons's, read without waiting (GET's), $FF: not open
keybuf:     .res        1

; The note handler, in RAM: Ctrl-C (NOTE_INTERRUPT) noted in intr, for the next statement (ISCNTC) or wait; another,
; the default
.segment "DATA"
; (First in DATA, at fixed places for PEEK and POKE: DATA is after the line buffer's page, at $0500)
SYSREGS:    .byte       0, 0, 0, 0                          ; $0500 (1280): .A, .X, .Y and the flags after SYS (RREG's)
USR:        jmp         IQERR                               ; $0504 (1284): USR(x)'s jump (its address at 1285, 1286)
.assert     SYSREGS = $0500 && USR = $0504, error, "SYSREGS and USR: their documented places"
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
            sta         save_fd
            sta         sndfd
            sta         sndctlfd
            ldx         #SRC_N-1                            ; The sources: none but stdin
:
            sta         src_fd,x
            dex
            bne         :-
            stz         src_fd + SRC_STDIN
            lda         #1
            sta         outfd
            LDR         r0, src_bufs                        ; stdin the console?  (Its stat record, in its buffer)
            lda         #0
            jsr         FSTAT
            bcs         :+
            lda         src_bufs + SR_DEV
            cmp         #'c'
            bne         :+
            inc         interactive
:
            lda         #$4C                                ; The vectors: jmp
            sta         GORESTART
            sta         GOSTROUT
            sta         JMPADRS
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

; RESTART's: SAVE's and LOAD's files closed (an error, Ctrl-C, its end), the output fd 1 again (PRINT#'s, SAVE's),
; the lines stdin's again (INPUT#'s, LOAD's), and a script's end (EXITS with its code).  OPEN's files stay open
IO_RESET:
            jsr         FLUSH
            lda         #1
            sta         outfd
            lda         save_fd
            bmi         :+
            jsr         CLOSE
            lda         #$FF
            sta         save_fd
:
            ldx         #SRC_LOAD
            jsr         SRC_CLOSE
            stz         cur_src
            stz         ld_run
            stz         nolf1
            lda         script
            beq         :+
            lda         exit_code
            jmp         EXIT_CODE
:
            rts

; Source .X closed, if it's open.  Keeps .X
SRC_CLOSE:
            lda         src_fd,x
            bmi         :+
            phx
            jsr         CLOSE
            plx
            lda         #$FF
            sta         src_fd,x
:
            rts

; Source .X's fd .A, its buffer empty
SRC_OPENED:
            sta         src_fd,x
            stz         src_len,x
            stz         src_pos,x
            stz         src_skip,x
            rts

; IOPTR = source .X's buffer.  Keeps .X
SRC_BUF:
            lda         src_buf_lo,x
            sta         IOPTR
            lda         src_buf_hi,x
            sta         IOPTR+1
            rts

src_buf_lo:
.repeat SRC_N, I
            .byte       <(src_bufs + I * SBUF_SIZE)
.endrepeat
src_buf_hi:
.repeat SRC_N, I
            .byte       >(src_bufs + I * SBUF_SIZE)
.endrepeat

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

; The next byte of the source cur_src (its buffer filled with a READ when it's empty; stdin's after the output's
; sent and the console's cooked).  OUT: C = 0, .A = it; C = 1, .A = E_EOF (its end) or the error (E_INTR: a note).
; Keeps .Y
IN_BYTE:
            ldx         cur_src
            lda         src_pos,x
            cmp         src_len,x
            bcc         @have
            jsr         SRC_FILL
            bcs         @done
@have:
            jsr         SRC_BUF
            phy
            ldy         src_pos,x
            lda         (IOPTR),y
            inc         src_pos,x
            ply
            clc
@done:
            rts

; Source .X's buffer filled (a READ).  OUT: C = 0, some there; C = 1, .A = E_EOF (none: its end) or the error.
; Keeps .X, .Y
SRC_FILL:
            phy
            cpx         #SRC_STDIN
            bne         :+
            jsr         FLUSH
            jsr         RAW_OFF
:
            phx
            jsr         SRC_BUF
            lda         IOPTR
            sta         r0
            lda         IOPTR+1
            sta         r0 + 1
            LDR         r1, SBUF_SIZE
            lda         src_fd,x
            jsr         READ
            plx
            ply
            bcs         @done
            sta         src_len,x
            stz         src_pos,x
            cmp         #1                                  ; (C = 1: some; none, its end)
            bcs         @some
            lda         #E_EOF
            sec
            rts
@some:
            clc
@done:
            rts

; A line into INPUTBUFFER, 0 after it (INBUF_MAX characters, the rest dropped; LF, CR or CR LF ends it; a tab a
; space, other control characters dropped), from the source cur_src: stdin (the console edits and echoes its lines
; itself), LOAD's file (its end is LOAD's: LOAD_DONE) or INPUT#'s (its end an error, END OF FILE).  stdin's end: in
; direct mode, BASIC's (BYE); in a program, an empty line.  Ctrl-C while it waits: in direct mode, the line again; in
; a program, an empty line (which stops INPUT, Microsoft's way).  OUT: .X/.Y = INPUTBUFFER-1, for CHRGET
INLIN:
            ldy         #0
@char:
            jsr         IN_BYTE
            bcs         @fail
            ldx         cur_src
            pha
            lda         src_skip,x
            stz         src_skip,x
            tax
            pla
            cmp         #LF
            bne         :+
            cpx         #0
            bne         @char                               ; (A CR LF's LF)
            bra         @end
:
            cmp         #CR
            bne         :+
            ldx         cur_src
            inc         src_skip,x
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
            ldx         cur_src                             ; (LOAD's file: its end; an error, an error; INPUT#'s:
            beq         @stdin                              ;   both errors)
            cpx         #SRC_LOAD
            bne         :+
            cmp         #E_EOF
            jeq         LOAD_DONE
:
            jmp         OSERR
@stdin:
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
            lda         cur_src                             ; (The console's column: 0 after its line)
            bne         :+
            stz         POSX
:
            ldx         #<(INPUTBUFFER-1)
            ldy         #>(INPUTBUFFER-1)
            rts

; GET's key: the console's, in raw mode, not waited for (0: none); if stdin isn't the console, or GET#'s, the
; source's next byte (its end: 0)
GETKEY:
            lda         cur_src
            bne         :+
            lda         interactive
            bne         @key
:
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

; The console cooked again, for a line.  Keeps .X, .Y
RAW_OFF:
            lda         raw
            beq         @done
            stz         raw
            phx
            phy
            LDR         r0, s_rawoff
            LDR         r1, 6
            lda         ctlfd
            jsr         WRITE
            ply
            plx
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
; lines as they were in memory).  A text's lines come through the main loop (L2351: INLIN reads them from the source
; SRC_LOAD), and its end (INLIN's) is LOAD_DONE
LOAD:
            jsr         GETNAME
LOAD_PATH:
            LDR         r0, pathbuf
            lda         #O_READ
            jsr         OPEN
            bcc         :+
            jmp         OSERR
:
            ldx         #SRC_LOAD
            jsr         SRC_OPENED
            jsr         SRC_FILL                            ; Its first bytes: a tokenized program's header?
            bcc         :+
            cmp         #E_EOF
            beq         @text                               ; (Empty: a text, no lines)
            jmp         OSERR
:
            lda         src_len + SRC_LOAD
            cmp         #HDR_LEN
            bcc         @text
            ldx         #HDR_MAGIC_LEN-1
:
            lda         LBUF,x
            cmp         s_magic,x
            bne         @text
            dex
            bpl         :-
            jmp         LOAD_TOKENS
@text:
            jsr         SCRTCH                              ; (NEW)
            lda         #SRC_LOAD
            sta         cur_src
            ldx         #STACK_TOP
            txs
            jmp         L2351

LBUF        = src_bufs + SRC_LOAD * SBUF_SIZE               ; (LOAD's source's buffer)

; A text's end: the program LOADed, and RUN if it's RUN "name"'s or a script's
LOAD_DONE:
            ldx         #SRC_LOAD
            jsr         SRC_CLOSE
            stz         cur_src
LOAD_END:
            lda         ld_run
            stz         ld_run
            bne         :+
            jmp         RESTART
:
            jsr         SETPTRS                             ; (RUN's: from the first line, the variables cleared)
            jmp         NEWSTT

; A tokenized program (LBUF has its first bytes): its header checked, its lines read to TXTTAB, linked again
LOAD_TOKENS:
            lda         LBUF + 5
            cmp         #HDR_VERSION
            jne         @bad
            jsr         TABSUM
            cmp         LBUF + 6
            jne         @bad
            cpy         LBUF + 7
            jne         @bad
            clc                                             ; Room for it?
            lda         TXTTAB
            adc         LBUF + 8
            sta         INDEX
            lda         TXTTAB+1
            adc         LBUF + 9
            sta         INDEX+1
            bcs         @full
            cmp         MEMSIZ+1
            bcs         @full
            jsr         SCRTCH
            lda         TXTTAB                              ; What LBUF has after the header, then the rest read
            sta         DEST
            lda         TXTTAB+1
            sta         DEST+1
            ldx         #HDR_LEN
@copy:
            cpx         src_len + SRC_LOAD
            bcs         @rest
            lda         DEST                                ; (Not past its length)
            cmp         INDEX
            lda         DEST+1
            sbc         INDEX+1
            bcs         @linked
            lda         LBUF,x
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
            ldx         #SRC_LOAD
            jsr         SRC_CLOSE
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
            lda         #'W'
            jsr         OPEN_PATH
            sta         save_fd
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
            bcs         @fail
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
            bcs         @fail
@close:
            jsr         FLUSH
            lda         save_fd
            jsr         CLOSE
            lda         #$FF
            sta         save_fd
            lda         #1
            sta         outfd
            rts
@fail:
            jmp         OSERR

; The file pathbuf names, opened for .A: 'R' reading; 'W' writing, made or emptied first; 'A' writing at its end,
; made if it isn't there.  OUT: .A = its fd (an error: ERROR's, the system's)
OPEN_PATH:
            cmp         #'R'
            bne         :+
            LDR         r0, pathbuf
            lda         #O_READ
            jsr         OPEN
            bcs         @fail
            rts
:
            pha
            LDR         r0, pathbuf
            pla
            pha
            cmp         #'A'
            beq         :+
            lda         #O_WRITE | O_TRUNC
            bra         :++
:
            lda         #O_WRITE
:
            jsr         OPEN
            bcc         @opened
            cmp         #E_NOENT
            bne         @fail
            LDR         r0, pathbuf                         ; (Not there: made)
            lda         #O_WRITE
            ldx         #0
            jsr         CREATE
            bcc         @opened
@fail:
            jmp         OSERR
@opened:
            plx                                             ; (Appending: at its end)
            cpx         #'A'
            bne         @done
            pha
            stz         r0
            stz         r0 + 1
            stz         r1
            stz         r1 + 1
            ldx         #2
            jsr         SEEK
            pla
@done:
            rts

; ---- Channels: OPEN, CLOSE, PRINT#, INPUT#, GET#, EOF

; .X = channel .X's source (1 to CH_MAX; else ILLEGAL QUANTITY)
CH_SRC:
            dex
            cpx         #CH_MAX
            bcc         :+
            jmp         IQERR
:
            inx
            txa
            clc
            adc         #SRC_CH
            tax
            rts

; "OPEN" STATEMENT: OPEN n,"name"[,"mode"]: channel n (1 to CH_MAX) a file or a device, read (R, as without a mode),
; written (W: made, or emptied first) or added to (A: written at its end).  One open already: FILE OPEN
OPEN_ST:
            jsr         GETBYT
            jsr         CH_SRC
            lda         src_fd,x
            bmi         :+
            ldx         #ERR_FILEOPEN
            jmp         ERROR
:
            stx         chsrc
            jsr         CHKCOM
            jsr         GETNAME
            lda         #'R'
            pha
            jsr         CHRGOT
            cmp         #','
            bne         @open
            jsr         CHRGET
            jsr         FRMEVL
            jsr         CHKSTR
            jsr         FREFAC
            cmp         #0
            beq         @open
            pla
            lda         (INDEX)
            and         #$DF                                ; (A letter in either case)
            pha
            cmp         #'R'
            beq         @open
            cmp         #'W'
            beq         @open
            cmp         #'A'
            beq         @open
            jmp         IQERR
@open:
            pla
            jsr         OPEN_PATH
            ldx         chsrc
            jmp         SRC_OPENED

; "CLOSE" STATEMENT: CLOSE n: channel n closed (if it's open); CLOSE alone: all of them
CLOSE_ST:
            beq         CH_CLOSE_ALL
            jsr         GETBYT
            jsr         CH_SRC
            jmp         SRC_CLOSE

; Every channel closed (CLOSE's, CLEARC's: RUN, NEW, CLEAR, a line entered).  Keeps .X, .Y
CH_CLOSE_ALL:
            phx
            ldx         #SRC_CH + CH_MAX
:
            jsr         SRC_CLOSE
            dex
            cpx         #SRC_CH
            bne         :-
            plx
            rts

; #n at TXTPTR (and a comma after it): .X = channel n's source, which must be open (else FILE NOT OPEN)
CH_NUM:
            jsr         GTBYTC
            jsr         CH_SRC
            lda         src_fd,x
            bpl         :+
            ldx         #ERR_NOTOPEN
            jmp         ERROR
:
            phx
            jsr         CHRGOT
            cmp         #','
            bne         :+
            jsr         CHRGET
:
            plx
            rts

; "PRINT" STATEMENT, and PRINT #n, ...: the list to channel n's file (the console's column kept)
PRINT_ST:
            cmp         #'#'
            jne         PRINT
            jsr         CH_NUM
            jsr         FLUSH
            lda         src_fd,x
            sta         outfd
            lda         POSX
            sta         posx_save
            stz         POSX
            jsr         CHRGOT
            jsr         PRINT
            jsr         FLUSH
            lda         #1
            sta         outfd
            lda         posx_save
            sta         POSX
            rts

; "INPUT" STATEMENT, and INPUT #n, ...: the values from channel n's next line (no prompt; its end, END OF FILE)
INPUT_ST:
            cmp         #'#'
            jne         INPUT
            jsr         CH_NUM
            stx         cur_src
            jsr         CHRGOT
            jsr         INPUT
            stz         cur_src
            rts

; "GET" STATEMENT, and GET #n, ...: channel n's next byte each (its end: "" or 0)
GET_ST:
            cmp         #'#'
            jne         GET
            jsr         CH_NUM
            stx         cur_src
            jsr         CHRGOT
            jsr         GET
            stz         cur_src
            rts

; "EOF" FUNCTION: EOF(n): true (-1) if channel n has nothing more to read (its buffer filled to know), else 0
EOF_FN:
            jsr         CONINT
            jsr         CH_SRC
            lda         src_fd,x
            bpl         :+
            ldx         #ERR_NOTOPEN
            jmp         ERROR
:
            lda         src_pos,x
            cmp         src_len,x
            bcc         @no
            jsr         SRC_FILL
            bcc         @no
            lda         #$FF
            tay
            jmp         GIVAYF
@no:
            lda         #0
            tay
            jmp         GIVAYF

; ---- Sound (the YM2151, through the sound driver: /dev/snd and /dev/sndctl, as C's snd.h has them), SLEEP

.bss
sndfd:      .res        1                                   ; /dev/snd's fd ($FF: not open yet) ...
sndctlfd:   .res        1                                   ;   and /dev/sndctl's
sndch:      .res        1                                   ; SOUND's channel ...
sndnote:    .res        1                                   ;   its note ...
sndlen:     .res        1                                   ;   sndbuf's bytes so far ...
sndbuf:     .res        12                                  ;   and the driver's pairs (3 commands, the channel first)
.code

; "SOUND" STATEMENT: SOUND ch, note [, patch [, vol]]: on channel ch (0-7) MIDI note (0-127: 60 middle C), patch
; (0-162: General MIDI's, then drums and percussion) and volume (0-127) first if they're given (snd_note, snd_patch,
; snd_vol); SOUND ch: its note off (snd_off); SOUND "word [n]": a line for /dev/sndctl ("claim 255", "release 255",
; "volume 150", "reset": snd_claim, snd_release, snd_volume, snd_reset).  One write: no other program's between
SOUND_ST:
            cmp         #'"'
            bne         @note
            jsr         GETNAME                             ; (A line for /dev/sndctl)
            lda         sndctlfd
            bpl         :+
            LDR         r0, s_sndctl
            lda         #O_WRITE
            jsr         OPEN
            jcs         @fail
            sta         sndctlfd
:
            ldy         #$FF                                ; (Its length)
:
            iny
            lda         pathbuf,y
            bne         :-
            sty         r1
            stz         r1 + 1
            LDR         r0, pathbuf
            lda         sndctlfd
            jsr         WRITE
            jcs         @fail
            rts
@fail:
            jmp         OSERR
@note:
            jsr         GETBYT
            cpx         #SND_CHANNELS
            jcs         @iq
            stx         sndch
            ldy         #0
            jsr         CHRGOT
            cmp         #','
            beq         @args
            lda         #SND_R_OFF                          ; (Off)
            ldx         #0
            jsr         SND_PAIR
            bra         @write
@args:
            jsr         COMBYTE
            cpx         #128                                ; (128 and up: no note)
            jcs         @iq
            stx         sndnote
            stz         sndlen
            jsr         CHRGOT
            cmp         #','
            bne         @on
            jsr         COMBYTE                             ; The patch
            cpx         #SND_PATCHES
            jcs         @iq
            ldy         #0
            lda         #SND_R_PATCH
            jsr         SND_PAIR
            sty         sndlen
            jsr         CHRGOT
            cmp         #','
            bne         @on
            jsr         COMBYTE                             ; The volume
            cpx         #128
            jcs         @iq
            ldy         sndlen
            lda         #SND_R_VOL
            jsr         SND_PAIR
            sty         sndlen
@on:
            ldy         sndlen
            lda         #SND_R_NOTE
            ldx         sndnote
            jsr         SND_PAIR
@write:
            sty         r1                                  ; (Its bytes)
            stz         r1 + 1
            lda         sndfd
            bpl         :+
            phy
            LDR         r0, s_snd
            lda         #O_RDWR
            jsr         OPEN
            ply
            jcs         @fail
            sta         sndfd
            sty         r1
            stz         r1 + 1
:
            LDR         r0, sndbuf
            lda         sndfd
            jsr         WRITE
            jcs         @fail
            rts
@iq:
            jmp         IQERR

; sndbuf at .Y: the channel's command, then command .A with .X (two pairs); .Y on past them
SND_PAIR:
            pha
            lda         #SND_R_CH
            sta         sndbuf,y
            lda         sndch
            sta         sndbuf+1,y
            pla
            sta         sndbuf+2,y
            txa
            sta         sndbuf+3,y
            iny
            iny
            iny
            iny
            rts

s_snd:      .byte       "/dev/snd", 0
s_sndctl:   .byte       "/dev/sndctl", 0

; "BEEP" STATEMENT: the console's bell (BEL, sent at once)
BEEP_ST:
            lda         #7
            jsr         OUTDO
            jmp         FLUSH

; "SLEEP" STATEMENT: SLEEP s: s seconds (to the tick, 5 ms: up to 163), the output sent first; Ctrl-C ends it
SLEEP_ST:
            jsr         FRMNUM
            lda         #<CON_TICK_HZ
            ldy         #>CON_TICK_HZ
            jsr         FMULT
            jsr         GETADR                              ; (LINNUM: the ticks, 0-65535; else ILLEGAL QUANTITY)
            lda         LINNUM+1
            bpl         :+
            jmp         IQERR
:
            jsr         FLUSH
            lda         LINNUM
            ldx         LINNUM+1
            jmp         SLEEP                               ; (A note ends it: NEWSTT takes Ctrl-C)

CON_TICK_HZ: .byte      $88, $48, $00, $00, $00             ; (200, TICK_HZ: 0.78125 * 2^8)
.assert TICK_HZ = 200, error, "CON_TICK_HZ is 200"

; ---- SYS, RREG and USR: machine code and the system's calls (Commodore's BASIC 7 form)

.bss
sysaddr:    .res        2                                   ; SYS's address ...
sysa:       .res        1                                   ;   and its registers in
sysx:       .res        1
sysy:       .res        1
rregi:      .res        1                                   ; RREG's register (0-3)
.code

; "SYS" STATEMENT: SYS address [, a [, x [, y]]]: the machine code at address called (jsr) with .A, .X and .Y (0 if
; they're not given), the output sent first; .A, .X, .Y and the flags after are RREG's (and at SYSREGS: PEEK(1280)
; to PEEK(1283)).  SYS "NAME" [, ...]: a system call by its name (spec/api.def's, either case: basicsys.inc), its
; registers r0-r15 the program's POKEs at 2-33, its results read there and with RREG (C, bit 0 of the flags: it
; failed, .A the error).  A name not there: NO SUCH CALL
SYS_ST:
            cmp         #'"'
            bne         @addr
            jsr         GETNAME
            jsr         SYS_LOOKUP
            bra         @regs
@addr:
            jsr         FRMNUM
            jsr         GETADR
            lda         LINNUM
            sta         sysaddr
            lda         LINNUM+1
            sta         sysaddr + 1
@regs:
            stz         sysa
            stz         sysx
            stz         sysy
            ldx         #0
:
            phx
            jsr         CHRGOT
            plx
            cmp         #','
            bne         @call
            phx
            jsr         COMBYTE
            txa
            plx
            sta         sysa,x
            inx
            cpx         #3
            bcc         :-
@call:
            ldx         #3                                  ; (The output out first; r0 and r1, which its WRITE
:                                                           ;   uses, kept: the program's POKEs)
            lda         r0,x
            pha
            dex
            bpl         :-
            jsr         FLUSH
            ldx         #0
:
            pla
            sta         r0,x
            inx
            cpx         #4
            bcc         :-
            lda         sysa
            ldx         sysx
            ldy         sysy
            jsr         @go
            php
            sta         SYSREGS
            stx         SYSREGS + 1
            sty         SYSREGS + 2
            pla
            sta         SYSREGS + 3
            rts
@go:
            jmp         (sysaddr)

; sysaddr = the address of the call pathbuf names (either case), from basicsys.inc's table; not there: NO SUCH CALL
SYS_LOOKUP:
            ldy         #0                                  ; (The name in capitals)
:
            lda         pathbuf,y
            beq         :++
            cmp         #'a'
            bcc         :+
            cmp         #'z'+1
            bcs         :+
            and         #$DF
            sta         pathbuf,y
:
            iny
            bra         :--
:
            lda         #<SYS_NAMES
            sta         IOPTR
            lda         #>SYS_NAMES
            sta         IOPTR+1
@name:
            lda         (IOPTR)
            bne         :+
            lda         #E_NOSYS
            jmp         OSERR
:
            ldy         #0
@char:
            lda         (IOPTR),y                           ; (Its last character: bit 7 set)
            and         #$7F
            cmp         pathbuf,y
            bne         @next
            lda         (IOPTR),y
            iny
            asl         a
            bcc         @char
            lda         pathbuf,y                           ; (All of it, and no more)
            bne         @next
            lda         (IOPTR),y
            sta         sysaddr
            iny
            lda         (IOPTR),y
            sta         sysaddr + 1
            rts
@next:
            ldy         #0                                  ; The next: past this one's name and address
:
            lda         (IOPTR),y
            iny
            asl         a
            bcc         :-
            iny
            iny
            tya
            clc
            adc         IOPTR
            sta         IOPTR
            bcc         @name
            inc         IOPTR+1
            bra         @name

; "RREG" STATEMENT: RREG [a] [, x] [, y] [, p]: the registers after the last SYS into numeric variables (any left
; out: RREG ,X)
RREG_ST:
            stz         rregi
@reg:
            jsr         CHRGOT
            beq         @done
            cmp         #','
            beq         @comma
            jsr         PTRGET
            sta         FORPNT
            sty         FORPNT+1
            lda         VALTYP                              ; (A string: TYPE MISMATCH)
            bpl         :+
            ldx         #ERR_BADTYPE
            jmp         ERROR
:
            lda         VALTYP+1
            pha
            ldx         rregi
            ldy         SYSREGS,x
            jsr         SNGFLT
            pla
            jsr         LET2
            jsr         CHRGOT
            beq         @done
            cmp         #','
            jne         SYNERR
@comma:
            jsr         CHRGET
            inc         rregi
            lda         rregi
            cmp         #4
            bcc         @reg
            jmp         SYNERR
@done:
            rts

.include "basicsys.inc"
