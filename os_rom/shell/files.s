.debuginfo

; ****************************************************************************
; The shell's file and card commands (BIOS ROM page 7): SH_CMD, which HyForth's shell words call (ls, cp,
; mkfs ...).  Names are relative to the current directory, or not.  Its RAM is HyForth's: SHBUF, SHBUF2,
; SDCMD, SHOWBUF (after HyForth's image: hyforth.s), SHFD, SHFD2, SHN.

.segment "SHELL_P7"

; A shell command.  IN: .X = SHC_* (include/shell.inc); .A.Y = its argument, a zero-terminated name (.Y =
; 0: none); or for the cards' commands, .A = the card (SHC_WAIT: a task).  SHBUF2 = a first name (cp, mv),
; or a label (mkfs, relabel).  OUT: C = 0; or C = 1, .A = error
SH_CMD:
            cpx         #SHC_COUNT
            bcs         @bad
            cpx         #SHC_VOLS                           ; (The cards' commands: .A is the card)
            bcs         @go
            cpx         #SHC_LS                             ; (ls alone: the current directory)
            beq         @go
            cpx         #SHC_EDIT                           ; (edit alone: a new file, named when written)
            beq         @go
            cpy         #0
            bne         @go
            lda         #ERR_IO_NAME                        ; (A file command with no name)
            sec
            rts

@go:
            jmp         (SH_CMDS,X)

@bad:
            lda         #ERR_IO_BAD_REQ
            sec
            rts

SH_CMDS:
            .word       SH_LS, SH_SHOW, SH_RM, SH_RMDIR, SH_MKDIR, SH_CP, SH_MV, SH_RUN, SH_EXEC, SH_EDIT
            .word       SH_VOLS, SH_MKFS, SH_RELABEL, SH_FSCK, SH_FSFIX, SH_WAIT, SH_LIBOPEN
.assert     * - SH_CMDS = SHC_COUNT, error, "SH_CMDS: an entry for each SHC_*"

SH_S_DOT:   .byte   ".", 0

; ****************************************************************************
; Files

; ls: a directory's listing (or a file's text); none: the current directory's
SH_LS:
            cpy         #0
            bne         :+
            lda         #<SH_S_DOT
            ldy         #>SH_S_DOT
:
            ldx         PAGE1::SHOPT                        ; (ls -l)
            beq         SH_SHOW
            jmp         SH_LS_LONG

; Show the file (or directory) at .A.Y: its text, to its end, a block at a time
SH_SHOW:
            ldx         #IO_MODE_READ
            jsr         IO_OPEN
            bcs         @done
            sta         PAGE1::SHFD

@read:
            jsr         SH_READ                             ; SHOWBUF = the next block
            bcs         @close
            lda         ZP_IO_CNT
            ora         ZP_IO_CNT + 1
            beq         @close                              ; (The end: C = 0)
            ldx         ZP_IO_CNT                           ; (256: 0, and the dex loop goes round 256 times)
            ldy         #0

@print:
            lda         PAGE1::SHOWBUF,Y
            jsr         WRITE_CHAR
            iny
            dex
            bne         @print
            bra         @read

@close:
            php
            pha
            lda         PAGE1::SHFD
            jsr         IO_CLOSE
            pla
            plp

@done:
            rts

; ls -l: a line for each entry of the directory at .A.Y, "name size date time" ("name/ date time" for a
; directory), from its stat records; or the one line of a file.  (The date and time: the entry's stamp, as
; /dev/time shows the clock.)  OUT: C = 0; or C = 1, .A = error
SH_LS_LONG:
            ldx         #IO_MODE_READ | IO_MODE_STAT
            jsr         IO_OPEN
            bcs         @done
            sta         PAGE1::SHFD
            LOAD_ADDR   PAGE1::SHOWBUF, ZP_IO_BUF           ; A directory?  (Its own stat record)
            lda         PAGE1::SHFD
            jsr         IO_STAT
            bcs         @close
            stz         PAGE1::SHSEL
            lda         PAGE1::SHOWBUF + IO_ST_MODE
            bmi         @read
            jsr         SH_LS_LINE                          ; (A file: its line)
            clc
            bra         @close

@read:                                                      ; A directory: its entries' records, 5 at a time
            LOAD_ADDR   PAGE1::SHOWBUF, ZP_IO_BUF
            lda         #5 * IO_STAT_SIZE
            sta         ZP_IO_CNT
            stz         ZP_IO_CNT + 1
            lda         PAGE1::SHFD
            jsr         IO_READ
            bcs         @close
            lda         ZP_IO_CNT
            beq         @close                              ; (The end: C = 0)
            sta         PAGE1::SHN
            stz         PAGE1::SHSEL

@entry:
            jsr         SH_LS_LINE
            lda         PAGE1::SHSEL
            clc
            adc         #IO_STAT_SIZE
            sta         PAGE1::SHSEL
            cmp         PAGE1::SHN
            bcc         @entry
            bra         @read

@close:
            php
            pha
            lda         PAGE1::SHFD
            jsr         IO_CLOSE
            pla
            plp

@done:
            rts

; The line for the stat record at SHOWBUF + SHSEL: the name ("/" after a directory's), the size (a file's),
; the date and time (CLOCK_TEXT, page 9, into SHBUF2).  Modifies: .A, .X, .Y, ZP_TIME ...
SH_LS_LINE:
            ldx         PAGE1::SHSEL
:
            lda         PAGE1::SHOWBUF + IO_ST_NAME,X
            beq         :+
            jsr         WRITE_CHAR
            inx
            bra         :-
:
            ldx         PAGE1::SHSEL
            lda         PAGE1::SHOWBUF + IO_ST_MODE,X
            bpl         @file
            lda         #'/'
            jsr         WRITE_CHAR
            bra         @stamp

@file:
            lda         #' '
            jsr         WRITE_CHAR
            ldy         #IO_ST_SIZE
            jsr         SH_LS_FIELD
            lda         #$FF                                ; The size in decimal: its digits, from the
            pha                                             ;   last, on the stack ($FF: the end)
:
            lda         #10
            jsr         TIME_DIV8                           ; (ZP_TIME / 10: .A = the digit)
            pha
            lda         ZP_TIME
            ora         ZP_TIME + 1
            ora         ZP_TIME + 2
            ora         ZP_TIME + 3
            bne         :-
:
            pla
            bmi         @stamp
            ora         #'0'
            jsr         WRITE_CHAR
            bra         :-

@stamp:
            lda         #' '
            jsr         WRITE_CHAR
            ldy         #IO_ST_STAMP
            jsr         SH_LS_FIELD
            LOAD_ADDR   PAGE1::SHBUF2, ZP_IO_REQ            ; (CLOCK_TEXT's text: at (ZP_IO_REQ) + ZP_PROC_IDX)
            stz         ZP_PROC_IDX
            jsr         CLOCK_TEXT
            ldx         #0
:
            lda         PAGE1::SHBUF2,X                     ; (It ends with CR LF)
            jsr         WRITE_CHAR
            inx
            cpx         ZP_PROC_IDX
            bne         :-
            rts

; ZP_TIME = the 4 bytes at offset .Y in the stat record at SHOWBUF + SHSEL.  Modifies: .A, .X, .Y
SH_LS_FIELD:
            tya
            clc
            adc         PAGE1::SHSEL
            tax
            ldy         #0
:
            lda         PAGE1::SHOWBUF,X
            sta         ZP_TIME,Y
            inx
            iny
            cpy         #4
            bne         :-
            rts

; Read up to SHOWBUF_SIZE bytes from fd SHFD into SHOWBUF: ZP_IO_CNT = how many (0: the end).
; OUT: C = 0; or C = 1, .A = error
SH_READ:
            LOAD_ADDR   PAGE1::SHOWBUF, ZP_IO_BUF
            lda         #<PAGE1::SHOWBUF_SIZE
            sta         ZP_IO_CNT
            lda         #>PAGE1::SHOWBUF_SIZE
            sta         ZP_IO_CNT + 1
            lda         PAGE1::SHFD
            jmp         IO_READ

.assert     PAGE1::SHOWBUF_SIZE <= 256, error, "SH_SHOW prints a block with an 8-bit count"

; Is the name at .A.Y a directory?  OUT: C = 0: .A = its mode (HFS_M_DIR: bit 7); or C = 1, .A = error
; Modifies: .X, .Y
SH_MODE:
            ldx         #IO_MODE_READ | IO_MODE_STAT
            jsr         IO_OPEN
            bcs         @done
            sta         PAGE1::SHSEL                        ; (The fd)
            LOAD_ADDR   PAGE1::SHOWBUF, ZP_IO_BUF           ; (Its stat record: 48 bytes)
            lda         PAGE1::SHSEL
            jsr         IO_STAT
            php
            pha
            lda         PAGE1::SHSEL
            jsr         IO_CLOSE
            pla
            plp
            bcs         @done
            lda         PAGE1::SHOWBUF + IO_ST_MODE
            clc

@done:
            rts

; rm: a file (a directory: rmdir)
SH_RM:
            jsr         SH_KEEP                             ; (SH_PTR = the name)
            jsr         SH_MODE
            bcs         @done
            ora         #0
            bpl         SH_REMOVE
            lda         #ERR_IO_IS_DIR
            sec

@done:
            rts

; rmdir: an empty directory
SH_RMDIR:
            jsr         SH_KEEP
            jsr         SH_MODE
            bcs         @done
            ora         #0
            bmi         SH_REMOVE
            lda         #ERR_IO_NOT_DIR
            sec

@done:
            rts

SH_REMOVE:
            lda         SH_PTR
            ldy         SH_PTR + 1
            jmp         IO_REMOVE

; mkdir: a new directory
SH_MKDIR:
            ldx         #HFS_M_DIR
            stx         ZP_IO_BUF
            ldx         #IO_MODE_READ
            jsr         IO_CREATE
            bcs         :+
            jmp         IO_CLOSE
:
            rts

; SH_PTR = .A.Y (a name to keep while other names come and go)
SH_KEEP:
            sta         SH_PTR
            sty         SH_PTR + 1
            rts

; cp: SHBUF2 copied to the name at .A.Y (a file, made or emptied; or a directory: the copy goes in it,
; with the same name)
SH_CP:
            jsr         SH_TARGET                           ; SH_PTR = where it goes
            bcs         SH_CP_DONE
            lda         #<PAGE1::SHBUF2
            ldy         #>PAGE1::SHBUF2
            jsr         SH_MODE                             ; (Not a directory)
            bcs         SH_CP_DONE
            ora         #0
            bpl         :+
            lda         #ERR_IO_IS_DIR
            sec
            rts
:
            lda         #<PAGE1::SHBUF2
            ldy         #>PAGE1::SHBUF2
            ldx         #IO_MODE_READ
            jsr         IO_OPEN
            bcs         SH_CP_DONE
            sta         PAGE1::SHFD
            stz         ZP_IO_BUF                           ; (A file: mode 0)
            lda         SH_PTR
            ldy         SH_PTR + 1
            ldx         #IO_MODE_WRITE
            jsr         IO_CREATE
            bcs         @close_from
            sta         PAGE1::SHFD2

@block:
            jsr         SH_READ
            bcs         @close
            lda         ZP_IO_CNT
            ora         ZP_IO_CNT + 1
            beq         @close                              ; (All of it: C = 0)
            LOAD_ADDR   PAGE1::SHOWBUF, ZP_IO_BUF           ; (IO_READ moved it on; ZP_IO_CNT: as read)
            lda         PAGE1::SHFD2
            jsr         IO_WRITE
            bcc         @block

@close:
            php
            pha
            lda         PAGE1::SHFD2
            jsr         IO_CLOSE
            pla
            plp

@close_from:
            php
            pha
            lda         PAGE1::SHFD
            jsr         IO_CLOSE
            pla
            plp

SH_CP_DONE:
            rts

; Where cp and mv put SHBUF2: the name at .A.Y; or if that's a directory, SHBUF2's last name in it (SHBUF).
; OUT: C = 0: SH_PTR = it; or C = 1, .A = error
SH_TARGET:
            jsr         SH_KEEP
            jsr         SH_MODE
            bcs         @name                               ; (Not there: a new name)
            ora         #0
            bpl         @name
            ldy         #0                                  ; A directory: SHBUF = it, '/', SHBUF2's last name
            ldx         #0
:
            lda         (SH_PTR),Y
            beq         :+
            sta         PAGE1::SHBUF,X
            inx
            iny
            bra         :-
:
            lda         #'/'
            sta         PAGE1::SHBUF,X
            inx
            jsr         SH_LAST                             ; .Y = where SHBUF2's last name starts
:
            lda         PAGE1::SHBUF2,Y
            sta         PAGE1::SHBUF,X
            beq         :+
            inx
            iny
            cpx         #63
            bne         :-
            lda         #ERR_IO_NAME
            sec
            rts
:
            lda         #<PAGE1::SHBUF
            ldy         #>PAGE1::SHBUF
            jsr         SH_KEEP

@name:
            clc
            rts

; .Y = where SHBUF2's last name starts (after its last '/').  Modifies: .A
SH_LAST:
            ldy         #0
            phx
            ldx         #0
:
            lda         PAGE1::SHBUF2,Y
            beq         :++
            iny
            cmp         #'/'
            bne         :+
            tya
            tax                                             ; (.X = after the '/')
:
            bra         :--
:
            txa
            tay
            plx
            rts

; mv: SHBUF2 renamed to the name at .A.Y, when that's a plain name (no '/') and not a directory there;
; else it moves: copied (SH_CP), then removed
SH_MV:
            jsr         SH_KEEP
            ldy         #0                                  ; A '/' in it?
:
            lda         (SH_PTR),Y
            beq         @plain
            cmp         #'/'
            beq         @move
            iny
            bra         :-

@plain:
            lda         SH_PTR
            ldy         SH_PTR + 1
            jsr         SH_MODE                             ; A directory there: it goes in it
            bcs         @rename
            ora         #0
            bmi         @move

@rename:                                                    ; The stat record: the new name, the mode as it is
            ldy         #0
:
            lda         (SH_PTR),Y
            sta         PAGE1::SHOWBUF,Y
            beq         :+
            iny
            cpy         #HFS_NAME_MAX + 1
            bne         :-
            lda         #ERR_IO_NAME
            sec
            rts
:
            lda         #$FF
            sta         PAGE1::SHOWBUF + IO_ST_MODE
            lda         #<PAGE1::SHBUF2
            ldy         #>PAGE1::SHBUF2
            ldx         #IO_MODE_READ
            jsr         IO_OPEN
            bcs         @done
            sta         PAGE1::SHFD
            LOAD_ADDR   PAGE1::SHOWBUF, ZP_IO_BUF
            lda         PAGE1::SHFD
            jsr         IO_WSTAT
            php
            pha
            lda         PAGE1::SHFD
            jsr         IO_CLOSE
            pla
            plp

@done:
            rts

@move:
            lda         SH_PTR
            ldy         SH_PTR + 1
            jsr         SH_CP
            bcs         @done
            lda         #<PAGE1::SHBUF2
            ldy         #>PAGE1::SHBUF2
            jmp         IO_REMOVE

; ****************************************************************************
; The cards (their ctl files, /dev/sd/N/ctl)

; The cards: for each of 0-7, "N: " and its ctl file's text (what it is, or none; its HydraFS)
SH_VOLS:
            stz         PAGE1::SHN

@card:
            lda         PAGE1::SHN
            ora         #'0'
            jsr         WRITE_CHAR
            lda         #':'
            jsr         WRITE_CHAR
            lda         #' '
            jsr         WRITE_CHAR
            lda         PAGE1::SHN
            jsr         SH_CTL_PATH
            jsr         SH_CTL_SHOW
            bcs         @done
            inc         PAGE1::SHN
            lda         PAGE1::SHN
            cmp         #SD_MAX_CARDS
            bne         @card
            clc

@done:
            rts

; mkfs: "format", its options, and the label (SHBUF2), to card .A's ctl file; then it's shown.  The options:
; PAGE1::SHOPT: HFS_FMT_FULL " -f" (a full format: the whole free map written), HFS_FMT_PART " -p" (in a
; partition); PAGE1::SHSIZE (megabytes; 0: the whole card): " -s N"
SH_MKFS:
            ldy         #$FF                                ; (The prompt's %l: read the label again)
            sty         PAGE1::LBLCARD
            ldx         #SH_S_FORMAT - SH_S_CTL
            jsr         SH_CTL_START                        ; SHBUF = the path, SDCMD = the word
            bcc         @far1
            jmp         SH_CTL_DONE
@far1:
            lda         PAGE1::SHOPT                        ; (HFS_FMT_* bits)
            and         #HFS_FMT_FULL
            beq         :+
            ldx         #SH_S_OPT_F - SH_S_CTL
            jsr         SH_CTL_ADD
:
            lda         PAGE1::SHOPT
            and         #HFS_FMT_PART
            beq         :+
            ldx         #SH_S_OPT_P - SH_S_CTL
            jsr         SH_CTL_ADD
:
            lda         PAGE1::SHSIZE
            ora         PAGE1::SHSIZE + 1
            beq         SH_CTL_ADD_LABEL
            ldx         #SH_S_OPT_S - SH_S_CTL
            jsr         SH_CTL_ADD
            jsr         SH_CTL_SIZE
            bra         SH_CTL_ADD_LABEL

; relabel: "label", and the label (SHBUF2), to card .A's ctl file; then it's shown
SH_RELABEL:
            ldy         #$FF                                ; (The prompt's %l: read the label again)
            sty         PAGE1::LBLCARD
            ldx         #SH_S_LABEL - SH_S_CTL
            jsr         SH_CTL_START                        ; SHBUF = the path, SDCMD = the word
            bcs         SH_CTL_DONE

SH_CTL_ADD_LABEL:                                           ; SDCMD from .Y on: a space, the label; then send it
            lda         #' '                                ; ... a space, the label
            sta         PAGE1::SDCMD,Y
            iny
            ldx         #0
:
            lda         PAGE1::SHBUF2,X
            sta         PAGE1::SDCMD,Y
            beq         SH_CTL_SEND
            inx
            iny
            cpy         #PAGE1::SDCMD_SIZE - 1
            bne         :-
            lda         #0
            sta         PAGE1::SDCMD,Y
            bra         SH_CTL_SEND

; fsck, fsfix: "check" or "check fix" to card .A's ctl file; then it's shown
SH_FSCK:
            ldx         #SH_S_CHECK - SH_S_CTL
            bra         :+

SH_FSFIX:
            ldx         #SH_S_FIX - SH_S_CTL
:
            jsr         SH_CTL_START
            bcs         SH_CTL_DONE

; Send SDCMD to the ctl file (SHBUF), then show it
SH_CTL_SEND:
            lda         #<PAGE1::SHBUF
            ldy         #>PAGE1::SHBUF
            ldx         #IO_MODE_WRITE
            jsr         IO_OPEN
            bcs         SH_CTL_DONE
            sta         PAGE1::SHFD
            LOAD_ADDR   PAGE1::SDCMD, ZP_IO_BUF
            ldy         #0                                  ; (Its length)
:
            lda         PAGE1::SDCMD,Y
            beq         :+
            iny
            bra         :-
:
            sty         ZP_IO_CNT
            stz         ZP_IO_CNT + 1
            lda         PAGE1::SHFD
            jsr         IO_WRITE
            php
            pha
            lda         PAGE1::SHFD
            jsr         IO_CLOSE
            pla
            plp
            bcs         SH_CTL_DONE

; Show the ctl file (SHBUF)
SH_CTL_SHOW:
            lda         #<PAGE1::SHBUF
            ldy         #>PAGE1::SHBUF
            jmp         SH_SHOW

SH_CTL_DONE:
            rts

; Card .A's ctl file into SHBUF, and the command word (SH_S_CTL,X) into SDCMD.
; OUT: C = 0: .Y = the word's length; or C = 1, .A = ERR_IO_NOT_FOUND (not a card 0-7)
SH_CTL_START:
            jsr         SH_CTL_PATH
            bcc         :+
            rts
:
            ldy         #0

; SH_S_CTL,X (zero-terminated) into SDCMD from .Y on.  OUT: .Y = where its 0 is, C = 0.  Modifies: .A, .X
SH_CTL_ADD:
            lda         SH_S_CTL,X
            sta         PAGE1::SDCMD,Y
            beq         :+
            inx
            iny
            bra         SH_CTL_ADD
:
            clc
            rts

; PAGE1::SHSIZE (16 bits) in decimal, into SDCMD from .Y on (zero-terminated).  OUT: .Y = where its 0 is
; Modifies: .A, .X, SH_PTR2
SH_CTL_SIZE:
            lda         PAGE1::SHSIZE
            sta         SH_PTR2
            lda         PAGE1::SHSIZE + 1
            sta         SH_PTR2 + 1
            ldx         #SH_TENS_END - SH_TENS - 2          ; From 10000 down
            stz         PAGE1::SHN                          ; (<> 0: a digit put already)

@power:
            lda         #'0'                                ; The digit: how many times it goes

@sub:
            pha
            lda         SH_PTR2
            sec
            sbc         SH_TENS,X
            pha
            lda         SH_PTR2 + 1
            sbc         SH_TENS + 1,X
            bcc         @digit                              ; (Less than it: that's the digit)
            sta         SH_PTR2 + 1
            pla
            sta         SH_PTR2
            pla
            inc
            bra         @sub

@digit:
            pla
            pla
            cpx         #0                                  ; (The ones: always)
            beq         @put
            cmp         #'0'
            bne         @put
            bit         PAGE1::SHN                          ; (A leading 0: left out.  '0' & a digit put <> 0)
            beq         @next

@put:
            sta         PAGE1::SDCMD,Y
            iny
            sta         PAGE1::SHN

@next:
            dex
            dex
            bpl         @power
            lda         #0
            sta         PAGE1::SDCMD,Y
            rts

SH_TENS:    .word       1, 10, 100, 1000, 10000
SH_TENS_END:

; SHBUF = card .A's ctl file, "/dev/sd/N/ctl".  OUT: C = 0; or C = 1, .A = ERR_IO_NOT_FOUND
; Modifies: .A (not .X)
SH_CTL_PATH:
            cmp         #SD_MAX_CARDS
            bcs         @bad
            phx
            ora         #'0'
            ldx         #SH_S_CTLPATH_END - SH_S_CTLPATH - 1
:
            pha
            lda         SH_S_CTLPATH,X
            sta         PAGE1::SHBUF,X
            pla
            dex
            bpl         :-
            sta         PAGE1::SHBUF + 8                    ; (The card)
            plx
            clc
            rts

@bad:
            lda         #ERR_IO_NOT_FOUND
            sec
            rts

SH_S_CTLPATH:   .byte   "/dev/sd/0/ctl", 0
SH_S_CTLPATH_END:
SH_S_CTL:
SH_S_FORMAT:    .byte   "format", 0
SH_S_LABEL:     .byte   "label", 0
SH_S_CHECK:     .byte   "check", 0
SH_S_FIX:       .byte   "check fix", 0
SH_S_OPT_F:     .byte   " -f", 0
SH_S_OPT_P:     .byte   " -p", 0
SH_S_OPT_S:     .byte   " -s ", 0
