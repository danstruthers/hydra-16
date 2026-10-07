.debuginfo

; ****************************************************************************
; edit: a line editor, in the manner of Unix's ed (BIOS ROM page 8).  The shell's edit word starts it in a
; task of its own (SH_EDIT, page 7), with the file's name as its argument (on SH_ARGS_FD, as a program gets
; its arguments), and waits for it.  The text is in the task's RAM (ED_TEXT ... ED_END: 29 KB), a line
; ending in LF each; the file's CR LF, CR or LF line ends are LFs here, and CR LFs again when it's written.
;
; At its prompt (*), a command, with line numbers (n, or $: the last) before it or after it:
;   p [a[,b]]   print lines a to b, numbered (p alone: all of them)
;   a [n]       add lines after line n (a alone: after the last), typed until a line of just "."
;   i [n]       insert lines before line n (i alone: before the first), until "."
;   c a[,b]     change lines a to b: they go, and the lines typed until "." take their place
;   d a[,b]     delete lines a to b
;   w [file]    write the text to the file (or to another one: then that's the file)
;   q           quit (a second q quits without writing what's changed); Q quits at once
;   h           help
; Ctrl-C (while it prints, or waits for lines) comes back to the prompt.

.segment "EDIT_P8"

ED_TEXT         = HYX_RAM_LOW                               ; The text, from here ...
ED_END          = HYX_RAM_END                               ;   up to here (the MMU's pages above: its floor)
ED_LINE         = $0200                                     ; A line typed (zero-terminated; 255 at most)
ED_LINE_MAX     = 255
ED_OUT          = $0300                                     ; What's written to the file, a block at a time
ED_OUT_SIZE     = 512
ED_NAME         = $0500                                     ; The file's name (HYX_ARGS_SIZE bytes)
ED_TOP          = $0540                                     ; The text's end: after its last line's LF (2)
ED_A            = $0542                                     ; A command's lines: the first ... (2)
ED_B            = $0544                                     ;   and the last (2)
ED_GIVEN        = $0546                                     ;   how many numbers were typed (0-2)
ED_LINES        = $0547                                     ; The text's lines (ED_COUNT) (2)
ED_DIRTY        = $0549                                     ; <> 0: changed since it was read or written
ED_QUIT         = $054A                                     ; <> 0: q, with changes not written
ED_FD           = $054B                                     ; The file's fd
ED_OUTN         = $054C                                     ; Bytes in ED_OUT (2)
ED_CNT          = $054E                                     ; Bytes written (2)
ED_CR           = $0550                                     ; <> 0: the last line typed ended with a CR
ED_LAST         = $0551                                     ; The prompt's character (a backspace erases it)
ED_CMD          = $0552                                     ; The command's letter
ED_SP           = $0553                                     ; The stack pointer at the prompt (Ctrl-C)
.assert     ED_SP < STDOUT_BUF, error, "edit's RAM must end below the task's stdio buffers"
.assert     ED_OUT + ED_OUT_SIZE <= ED_NAME, error, "ED_OUT must end below ED_NAME"

TASK_ZP_BEGIN
TASK_ZP     ED_P, 2                                         ; A place in the text
TASK_ZP     ED_S, 2                                         ; A move's source ...
TASK_ZP     ED_D, 2                                         ;   destination ...
TASK_ZP     ED_C, 2                                         ;   and count
TASK_ZP     ED_N, 2                                         ; A number
TASK_ZP     ED_M, 2                                         ; A message
TASK_ZP_END

; The editor's task starts here (SH_EDIT): the file read, then its commands, until q
ED_MAIN:
            LOAD_ADDR   ED_NAME, ZP_IO_BUF                  ; The file's name: its argument (SH_ARGS_FD)
            lda         #HYX_ARGS_SIZE - 1
            sta         ZP_IO_CNT
            stz         ZP_IO_CNT + 1
            lda         #SH_ARGS_FD
            jsr         IO_READ
            ldx         #0
            bcs         :+
            ldx         ZP_IO_CNT
:
            stz         ED_NAME,X
            lda         #SH_ARGS_FD
            jsr         IO_CLOSE
            lda         #>ED_END                            ; Task RAM from ED_TEXT: the text's, not the MMU's
            jsr         MM_SET_FLOOR
            stz         ED_DIRTY
            stz         ED_QUIT
            stz         ED_CR
            jsr         ED_READ                             ; The file (or a new one)
            lda         #<ED_BREAK                          ; Ctrl-C: back to the prompt
            ldy         #>ED_BREAK
            ldx         #8
            jsr         TASK_SET_BREAK
            tsx                                             ; (The stack as it is here: the break handler's,
            stx         ED_SP                               ;   not the gate's, from inside it)

ED_LOOP:
            lda         #'*'
            jsr         ED_PROMPT
            jsr         ED_GETLINE
            bcs         @end                                ; (The end of the input: as Q)
            jsr         ED_DO
            bcc         ED_LOOP

@end:
            rts

ED_BREAK:                                                   ; (The stack: as it was after TASK_SET_BREAK)
            ldx         ED_SP
            txs
            jsr         ED_CRLF
            lda         #'?'
            jsr         WRITE_CHAR
            jsr         ED_CRLF
            bra         ED_LOOP

; The prompt's character .A (and it's the one a backspace at a line's start puts back)
ED_PROMPT:
            sta         ED_LAST
            jmp         WRITE_CHAR

; ****************************************************************************
; Commands

; The command in ED_LINE.  OUT: C = 1: quit
ED_DO:
            jsr         ED_COUNT                            ; (ED_LINES: $ and the defaults)
            ldy         #0
            jsr         ED_SKIP
            lda         ED_LINE,Y                           ; Numbers first (2,4p), or after (p 2,4)
            jsr         ED_RANGE
            jsr         ED_SKIP
            lda         ED_LINE,Y
            bne         :+
            clc                                             ; (Nothing: nothing to do)
            rts
:
            sta         ED_CMD
            iny
            cmp         #'w'                                ; (Numbers after it, if none before; w: a name)
            beq         :+
            lda         ED_GIVEN
            bne         :+
            jsr         ED_RANGE
:
            lda         ED_CMD
            cmp         #'q'
            beq         @quit
            stz         ED_QUIT                             ; (Anything else: q asks again)
            cmp         #'Q'
            beq         @done_quit
            cmp         #'p'
            bne         :+
            jmp         ED_P_CMD
:
            cmp         #'d'
            bne         :+
            jmp         ED_D_CMD
:
            cmp         #'c'
            bne         :+
            jmp         ED_C_CMD
:
            cmp         #'a'
            bne         :+
            jmp         ED_A_CMD
:
            cmp         #'i'
            bne         :+
            jmp         ED_I_CMD
:
            cmp         #'w'
            bne         :+
            jmp         ED_W_CMD
:
            cmp         #'h'
            bne         :+
            lda         #<ED_S_HELP
            ldy         #>ED_S_HELP
            jsr         ED_PUTS
            clc
            rts
:
            lda         #<ED_S_WHAT
            ldy         #>ED_S_WHAT
            bra         ED_FAIL

@quit:
            lda         ED_DIRTY                            ; Changes not written: the second q quits
            beq         @done_quit
            lda         ED_QUIT
            bne         @done_quit
            inc         ED_QUIT
            lda         #<ED_S_UNSAVED
            ldy         #>ED_S_UNSAVED
            bra         ED_FAIL

@done_quit:
            sec
            rts

ED_FAIL:                                                    ; Say what's wrong (.A.Y), and go on
            jsr         ED_PUTS
            clc
            rts

; p: print lines A to B (none given: all of them)
ED_P_CMD:
            lda         ED_GIVEN
            bne         :+
            lda         #1                                  ; (All)
            sta         ED_A
            stz         ED_A + 1
            lda         ED_LINES
            sta         ED_B
            lda         ED_LINES + 1
            sta         ED_B + 1
            ora         ED_LINES
            bne         :+
            clc                                             ; (No lines at all)
            rts
:
            jsr         ED_CHECK
            bcs         ED_FAIL
            lda         ED_A
            ldy         ED_A + 1
            jsr         ED_START                            ; ED_P = line A

@line:
            lda         ED_A                                ; Its number, right-aligned, a space ...
            sta         ED_N
            lda         ED_A + 1
            sta         ED_N + 1
            jsr         ED_DEC
            lda         #' '
            jsr         WRITE_CHAR

@char:                                                      ; ... and its text
            lda         (ED_P)
            jsr         ED_NEXT
            cmp         #ASCII_LF
            beq         :+
            jsr         WRITE_CHAR
            bra         @char
:
            jsr         ED_CRLF
            lda         ED_A                                ; The next, to B
            cmp         ED_B
            bne         :+
            lda         ED_A + 1
            cmp         ED_B + 1
            beq         @done
:
            inc         ED_A
            bne         @line
            inc         ED_A + 1
            bra         @line

@done:
            clc
            rts

; d: delete lines A to B
ED_D_CMD:
            jsr         ED_NEED
            bcs         ED_FAIL
            jsr         ED_DELETE
            clc
            rts

; c: change lines A to B: they go, and the lines typed take their place
ED_C_CMD:
            jsr         ED_NEED
            bcc         @far4
            jmp         ED_FAIL
@far4:
            jsr         ED_DELETE                           ; (ED_P: where they were)
            jmp         ED_INSERT

; a: add lines after line A (none: after the last)
ED_A_CMD:
            lda         ED_GIVEN
            bne         :+
            lda         ED_LINES
            sta         ED_A
            lda         ED_LINES + 1
            sta         ED_A + 1
:
            lda         ED_A                                ; (0: before the first)
            ora         ED_A + 1
            beq         @after
            lda         ED_A                                ; (A line that's there)
            sta         ED_B
            lda         ED_A + 1
            sta         ED_B + 1
            jsr         ED_CHECK
            bcc         @after
            jmp         ED_FAIL

@after:
            inc         ED_A                                ; After it: where the next one starts
            bne         @start
            inc         ED_A + 1

@start:
            lda         ED_A
            ldy         ED_A + 1
            jsr         ED_START
            jmp         ED_INSERT

; i: insert lines before line A (none: before the first)
ED_I_CMD:
            lda         ED_GIVEN
            bne         :+
            lda         #1
            sta         ED_A
            stz         ED_A + 1
:
            lda         ED_A                                ; (1 to the lines + 1: that's after the last)
            ora         ED_A + 1
            beq         @bad
            lda         ED_LINES
            clc
            adc         #1
            sta         ED_N
            lda         ED_LINES + 1
            adc         #0
            cmp         ED_A + 1
            bcc         @bad
            bne         :+
            lda         ED_N
            cmp         ED_A
            bcc         @bad
:
            lda         ED_A
            ldy         ED_A + 1
            jsr         ED_START
            jmp         ED_INSERT

@bad:
            lda         #<ED_S_NO_LINE
            ldy         #>ED_S_NO_LINE
            jmp         ED_FAIL

; w: write the text to the file (the name after w: that file, and it's the file from now on)
ED_W_CMD:
            jsr         ED_SKIP
            lda         ED_LINE,Y
            beq         @named
            ldx         #0                                  ; A name: the file's from now on
:
            lda         ED_LINE,Y
            beq         :+
            cmp         #' '
            beq         :+
            sta         ED_NAME,X
            inx
            iny
            cpx         #HYX_ARGS_SIZE - 1
            bne         :-
:
            stz         ED_NAME,X

@named:
            lda         ED_NAME
            bne         :+
            lda         #<ED_S_NO_NAME
            ldy         #>ED_S_NO_NAME
            jmp         ED_FAIL
:
            jsr         ED_WRITE
            clc
            rts

; ****************************************************************************
; Lines

; A range: numbers from .Y in ED_LINE (n, or $: the last line; a, or a,b).  OUT: ED_GIVEN = how many (0-2),
; ED_A and ED_B; .Y after them
ED_RANGE:
            stz         ED_GIVEN
            jsr         ED_NUMBER
            bcs         @done
            inc         ED_GIVEN
            lda         ED_N
            sta         ED_A
            sta         ED_B
            lda         ED_N + 1
            sta         ED_A + 1
            sta         ED_B + 1
            jsr         ED_SKIP
            lda         ED_LINE,Y
            cmp         #','
            bne         @done
            iny
            inc         ED_GIVEN
            jsr         ED_NUMBER                           ; (a, alone: a to the last)
            bcc         :+
            lda         ED_LINES
            sta         ED_N
            lda         ED_LINES + 1
            sta         ED_N + 1
:
            lda         ED_N
            sta         ED_B
            lda         ED_N + 1
            sta         ED_B + 1

@done:
            rts

; A number from .Y in ED_LINE (spaces before it skipped): digits, or $ (the last line).
; OUT: C = 0: ED_N = it, .Y after it; or C = 1: none there.  Modifies: .A
ED_NUMBER:
            jsr         ED_SKIP
            lda         ED_LINE,Y
            cmp         #'$'
            bne         :+
            iny
            lda         ED_LINES
            sta         ED_N
            lda         ED_LINES + 1
            sta         ED_N + 1
            clc
            rts
:
            jsr         @digit
            bcs         @none
            stz         ED_N
            stz         ED_N + 1

@more:
            pha                                             ; ED_N * 10 + the digit
            asl         ED_N
            rol         ED_N + 1
            lda         ED_N
            ldx         ED_N + 1
            asl         ED_N
            rol         ED_N + 1
            asl         ED_N
            rol         ED_N + 1
            clc
            adc         ED_N
            sta         ED_N
            txa
            adc         ED_N + 1
            sta         ED_N + 1
            pla
            clc
            adc         ED_N
            sta         ED_N
            bcc         :+
            inc         ED_N + 1
:
            iny
            lda         ED_LINE,Y
            jsr         @digit
            bcc         @more
            clc
            rts

@none:
            sec
            rts

@digit:                                                     ; .A = a digit's value, C = 0; or C = 1
            sec
            sbc         #'0'
            cmp         #10
            rts

; .Y on past spaces in ED_LINE.  Modifies: .A
ED_SKIP:
            lda         ED_LINE,Y
            cmp         #' '
            bne         :+
            iny
            bra         ED_SKIP
:
            rts

; Lines A to B, which must be given.  OUT: as ED_CHECK
ED_NEED:
            lda         ED_GIVEN
            bne         ED_CHECK
            lda         #<ED_S_WHICH
            ldy         #>ED_S_WHICH
            sec
            rts

; Are lines A to B there (1 <= A <= B <= the last)?  OUT: C = 0; or C = 1, .A.Y = what's wrong
ED_CHECK:
            lda         ED_A
            ora         ED_A + 1
            beq         @bad
            lda         ED_B + 1                            ; A <= B
            cmp         ED_A + 1
            bcc         @bad
            bne         :+
            lda         ED_B
            cmp         ED_A
            bcc         @bad
:
            lda         ED_LINES + 1                        ; B <= the last
            cmp         ED_B + 1
            bcc         @bad
            bne         :+
            lda         ED_LINES
            cmp         ED_B
            bcc         @bad
:
            clc
            rts

@bad:
            lda         #<ED_S_NO_LINE
            ldy         #>ED_S_NO_LINE
            sec
            rts

; ED_LINES = the text's lines (its LFs)
ED_COUNT:
            stz         ED_LINES
            stz         ED_LINES + 1
            LOAD_ADDR   ED_TEXT, ED_P

@byte:
            jsr         ED_AT_TOP
            bcs         @done
            lda         (ED_P)
            jsr         ED_NEXT
            cmp         #ASCII_LF
            bne         @byte
            inc         ED_LINES
            bne         @byte
            inc         ED_LINES + 1
            bra         @byte

@done:
            rts

; ED_P = where line .A.Y starts (1: the text's start; the lines + 1: its end)
ED_START:
            sta         ED_N
            sty         ED_N + 1
            LOAD_ADDR   ED_TEXT, ED_P

@line:
            lda         ED_N                                ; Past N - 1 LFs
            bne         :+
            dec         ED_N + 1
:
            dec         ED_N
            lda         ED_N
            ora         ED_N + 1
            beq         @done
:
            jsr         ED_AT_TOP
            bcs         @done
            lda         (ED_P)
            jsr         ED_NEXT
            cmp         #ASCII_LF
            bne         :-
            bra         @line

@done:
            rts

; ED_P on by 1.  Keeps .A (and the flags N, Z)
ED_NEXT:
            inc         ED_P
            bne         :+
            inc         ED_P + 1
:
            ora         #0
            rts

; Is ED_P at the text's end (ED_TOP)?  OUT: C = 1: it is.  Modifies: .A
ED_AT_TOP:
            lda         ED_P
            cmp         ED_TOP
            lda         ED_P + 1
            sbc         ED_TOP + 1
            rts

; Delete lines A to B (checked): ED_P = where they started
ED_DELETE:
            lda         ED_B                                ; ED_S = where the line after B starts
            ldy         ED_B + 1
            clc
            adc         #1
            bcc         :+
            iny
:
            jsr         ED_START
            lda         ED_P
            sta         ED_S
            lda         ED_P + 1
            sta         ED_S + 1
            lda         ED_A                                ; ED_P = ED_D = where A starts
            ldy         ED_A + 1
            jsr         ED_START
            lda         ED_P
            sta         ED_D
            lda         ED_P + 1
            sta         ED_D + 1
            sec                                             ; ED_C = the rest of the text, after them
            lda         ED_TOP
            sbc         ED_S
            sta         ED_C
            lda         ED_TOP + 1
            sbc         ED_S + 1
            sta         ED_C + 1
            clc                                             ; The text's end: where they started + the rest
            lda         ED_D
            adc         ED_C
            sta         ED_TOP
            lda         ED_D + 1
            adc         ED_C + 1
            sta         ED_TOP + 1
            jsr         ED_MOVE_DOWN
            lda         #1
            sta         ED_DIRTY
            rts

; Insert the lines typed next (until a line of just ".", or the end of the input) at ED_P
ED_INSERT:
            jsr         ED_GETLINE
            bcc         @far3
            jmp         @done
@far3:
            lda         ED_LINE                             ; "."?
            cmp         #'.'
            bne         :+
            lda         ED_LINE + 1
            beq         @done
:
            ldy         #$FF                                ; .Y = its length; ED_C = its bytes (with the LF)
:
            iny
            lda         ED_LINE,Y
            bne         :-
            phy
            iny
            sty         ED_C
            stz         ED_C + 1
            clc                                             ; Room?  ED_TOP + ED_C <= ED_END
            lda         ED_TOP
            adc         ED_C
            lda         ED_TOP + 1
            adc         #0
            cmp         #>ED_END
            bcc         :+
            ply
            lda         #<ED_S_FULL
            ldy         #>ED_S_FULL
            jmp         ED_PUTS
:
            lda         ED_P                                ; The text from ED_P on moves up by ED_C:
            sta         ED_S                                ;   ED_S = ED_P, ED_D = ED_P + ED_C, the count =
            clc                                             ;   ED_TOP - ED_P
            adc         ED_C
            sta         ED_D
            lda         ED_P + 1
            sta         ED_S + 1
            adc         #0
            sta         ED_D + 1
            lda         ED_C                                ; (Kept: the move counts ED_C down)
            pha
            sec
            lda         ED_TOP
            sbc         ED_P
            sta         ED_C
            lda         ED_TOP + 1
            sbc         ED_P + 1
            sta         ED_C + 1
            jsr         ED_MOVE_UP
            pla                                             ; The text's end moves up by it
            clc
            adc         ED_TOP
            sta         ED_TOP
            bcc         :+
            inc         ED_TOP + 1
:
            ply                                             ; The line, and its LF, at ED_P
            lda         #ASCII_LF
            sta         (ED_P),Y
:
            cpy         #0
            beq         :+
            dey
            lda         ED_LINE,Y
            sta         (ED_P),Y
            bra         :-
:                                                           ; ED_P past it
            lda         (ED_P)
            jsr         ED_NEXT
            cmp         #ASCII_LF
            bne         :-
            lda         #1
            sta         ED_DIRTY
            jmp         ED_INSERT

@done:
            clc                                             ; (Not quitting: the end of the input comes
            rts                                             ;   again at the prompt, and that quits)

; Copy ED_C bytes from ED_S to ED_D, upwards (ED_D <= ED_S).  Modifies: .A, ED_S, ED_D, ED_C
ED_MOVE_DOWN:
            lda         ED_C
            ora         ED_C + 1
            beq         @done
            lda         (ED_S)
            sta         (ED_D)
            inc         ED_S
            bne         :+
            inc         ED_S + 1
:
            inc         ED_D
            bne         :+
            inc         ED_D + 1
:
            lda         ED_C
            bne         :+
            dec         ED_C + 1
:
            dec         ED_C
            bra         ED_MOVE_DOWN

@done:
            rts

; Copy ED_C bytes from ED_S to ED_D, from the top down (ED_D >= ED_S).  Modifies: .A, ED_S, ED_D, ED_C
ED_MOVE_UP:
            clc                                             ; From their ends
            lda         ED_S
            adc         ED_C
            sta         ED_S
            lda         ED_S + 1
            adc         ED_C + 1
            sta         ED_S + 1
            clc
            lda         ED_D
            adc         ED_C
            sta         ED_D
            lda         ED_D + 1
            adc         ED_C + 1
            sta         ED_D + 1

@byte:
            lda         ED_C
            ora         ED_C + 1
            beq         @done
            lda         ED_S
            bne         :+
            dec         ED_S + 1
:
            dec         ED_S
            lda         ED_D
            bne         :+
            dec         ED_D + 1
:
            dec         ED_D
            lda         (ED_S)
            sta         (ED_D)
            lda         ED_C
            bne         :+
            dec         ED_C + 1
:
            dec         ED_C
            bra         @byte

@done:
            rts

; ****************************************************************************
; The file

; Read the file (ED_NAME) into the text, its line ends made LFs, and say how many lines it has; or, if it
; isn't there, that it's a new one
ED_READ:
            LOAD_ADDR   ED_TEXT, ED_TOP                     ; (Nothing yet)
            lda         ED_NAME
            bne         :+
            rts                                             ; (No name: w needs one)
:
            lda         #<ED_NAME
            ldy         #>ED_NAME
            ldx         #IO_MODE_READ
            jsr         IO_OPEN
            bcc         :+
            cmp         #ERR_IO_NOT_FOUND
            beq         @far2
            jmp         @error
@far2:
            lda         #<ED_NAME
            ldy         #>ED_NAME
            jsr         ED_PUTS_NL
            lda         #<ED_S_NEW
            ldy         #>ED_S_NEW
            jmp         ED_PUTS
:
            sta         ED_FD
            LOAD_ADDR   ED_TEXT, ZP_IO_BUF                  ; All of it (to 1 byte short of the room: a last
            lda         #<(ED_END - ED_TEXT - 1)            ;   LF may have to be added)
            sta         ZP_IO_CNT
            lda         #>(ED_END - ED_TEXT - 1)
            sta         ZP_IO_CNT + 1
            lda         ED_FD
            jsr         IO_READ
            php
            pha
            lda         ED_FD
            jsr         IO_CLOSE
            pla
            plp
            bcc         @far1
            jmp         @error
@far1:
            LOAD_ADDR   ED_TEXT, ED_S                       ; Its line ends: LFs (ED_S reads, ED_D writes)
            LOAD_ADDR   ED_TEXT, ED_D
            clc
            lda         #<ED_TEXT
            adc         ZP_IO_CNT
            sta         ED_C                                ; (ED_C = the end of what was read)
            lda         #>ED_TEXT
            adc         ZP_IO_CNT + 1
            sta         ED_C + 1

@byte:
            lda         ED_S
            cmp         ED_C
            lda         ED_S + 1
            sbc         ED_C + 1
            bcs         @read
            lda         (ED_S)
            inc         ED_S
            bne         :+
            inc         ED_S + 1
:
            cmp         #ASCII_CR
            bne         @put
            lda         ED_S                                ; A CR: an LF; and the LF of a CR LF goes
            cmp         ED_C
            lda         ED_S + 1
            sbc         ED_C + 1
            bcs         @lf
            lda         (ED_S)
            cmp         #ASCII_LF
            bne         @lf
            inc         ED_S
            bne         @lf
            inc         ED_S + 1

@lf:
            lda         #ASCII_LF

@put:
            sta         (ED_D)
            inc         ED_D
            bne         @byte
            inc         ED_D + 1
            bra         @byte

@read:
            lda         ED_D                                ; A last line with no line end: one
            cmp         #<ED_TEXT
            bne         :+
            lda         ED_D + 1
            cmp         #>ED_TEXT
            beq         @end                                ; (Empty)
:
            lda         ED_D
            bne         :+
            dec         ED_D + 1
:
            dec         ED_D
            lda         (ED_D)
            inc         ED_D
            bne         :+
            inc         ED_D + 1
:
            cmp         #ASCII_LF
            beq         @end
            lda         #ASCII_LF
            sta         (ED_D)
            inc         ED_D
            bne         @end
            inc         ED_D + 1

@end:
            lda         ED_D
            sta         ED_TOP
            lda         ED_D + 1
            sta         ED_TOP + 1
            lda         ZP_IO_CNT                           ; All of it?  (Read to the room's end: maybe not)
            cmp         #<(ED_END - ED_TEXT - 1)
            bne         :+
            lda         ZP_IO_CNT + 1
            cmp         #>(ED_END - ED_TEXT - 1)
            bne         :+
            lda         #<ED_S_BIG
            ldy         #>ED_S_BIG
            jsr         ED_PUTS
:
            jsr         ED_COUNT                            ; "name: n lines"
            lda         #<ED_NAME
            ldy         #>ED_NAME
            jsr         ED_PUTS_NL
            lda         #':'
            jsr         WRITE_CHAR
            lda         ED_LINES
            ldy         ED_LINES + 1
            ldx         #<ED_S_LINES
            jmp         ED_COUNT_SAY

@error:
            jmp         ED_ERROR

; Write the text to the file (ED_NAME), its LFs as CR LFs, and say how many bytes
ED_WRITE:
            stz         ZP_IO_BUF                           ; (A file: mode 0; one that's there is emptied)
            lda         #<ED_NAME
            ldy         #>ED_NAME
            ldx         #IO_MODE_WRITE
            jsr         IO_CREATE
            bcc         :+
            jmp         ED_ERROR
:
            sta         ED_FD
            stz         ED_OUTN
            stz         ED_OUTN + 1
            stz         ED_CNT
            stz         ED_CNT + 1
            LOAD_ADDR   ED_TEXT, ED_P

@byte:
            jsr         ED_AT_TOP
            bcs         @end
            lda         (ED_P)
            jsr         ED_NEXT
            cmp         #ASCII_LF
            bne         :+
            lda         #ASCII_CR
            jsr         ED_OUT_BYTE
            bcs         @fail
            lda         #ASCII_LF
:
            jsr         ED_OUT_BYTE
            bcc         @byte
            bra         @fail

@end:
            jsr         ED_FLUSH
            bcs         @fail
            lda         ED_FD
            jsr         IO_CLOSE
            bcs         @error
            stz         ED_DIRTY
            lda         ED_CNT
            ldy         ED_CNT + 1
            ldx         #<ED_S_BYTES
            jmp         ED_COUNT_SAY

@fail:
            pha
            lda         ED_FD
            jsr         IO_CLOSE
            pla

@error:
            jmp         ED_ERROR

; .A into ED_OUT (a full one written).  OUT: C = 0; or C = 1, .A = error
ED_OUT_BYTE:
            ldx         ED_OUTN
            pha
            lda         ED_OUTN + 1
            bne         :+
            pla
            sta         ED_OUT,X
            bra         @counted
:
            pla
            sta         ED_OUT + 256,X

@counted:
            inc         ED_CNT
            bne         :+
            inc         ED_CNT + 1
:
            inc         ED_OUTN
            bne         :+
            inc         ED_OUTN + 1
            lda         ED_OUTN + 1
            cmp         #>ED_OUT_SIZE
            beq         ED_FLUSH
:
            clc
            rts

; ED_OUT (ED_OUTN bytes) to the file, and it's empty.  OUT: C = 0; or C = 1, .A = error
ED_FLUSH:
            LOAD_ADDR   ED_OUT, ZP_IO_BUF
            lda         ED_OUTN
            sta         ZP_IO_CNT
            lda         ED_OUTN + 1
            sta         ZP_IO_CNT + 1
            stz         ED_OUTN
            stz         ED_OUTN + 1
            ora         ZP_IO_CNT
            beq         :+
            lda         ED_FD
            jmp         IO_WRITE
:
            clc
            rts

.assert     ED_OUT_SIZE = 512 .and <ED_OUT = 0, error, "ED_OUT_BYTE: two whole pages"

; An IO error .A: "? error $ee"
ED_ERROR:
            pha
            lda         #<ED_S_ERROR
            ldy         #>ED_S_ERROR
            jsr         ED_PUTS_NL
            pla
            jsr         WRITE_BYTE
            jmp         ED_CRLF

; ****************************************************************************
; Output

; " n lines" or " n bytes": the count .A.Y, then the text at ED_S_LINES / ED_S_BYTES (.X: its low byte;
; they're on the same page as ED_S_COUNTS), and a new line
ED_COUNT_SAY:
            sta         ED_N
            sty         ED_N + 1
            phx
            lda         #' '
            jsr         WRITE_CHAR
            jsr         ED_DEC_ALL
            plx
            txa
            ldy         #>ED_S_COUNTS
            jmp         ED_PUTS

; ED_N in decimal: right-aligned in 4 characters (ED_DEC; 10000 and up take 5), or with no spaces
; (ED_DEC_ALL).  Modifies: .A, .X, ED_M
ED_DEC:
            lda         #' '
            .byte       $2C                                 ; (bit abs: skips the lda)
ED_DEC_ALL:
            lda         #0
            sta         ED_M                                ; (What a leading 0 is: a space, or nothing)
            stz         ED_M + 1                            ; (Bit 7 set: a digit's been put, 0s are 0s)
            ldx         #8                                  ; From 10000 down

@power:
            lda         #'0'                                ; The digit: how many times it goes

@sub:
            pha
            lda         ED_N
            sec
            sbc         ED_TENS,X
            pha
            lda         ED_N + 1
            sbc         ED_TENS + 1,X
            bcc         @digit
            sta         ED_N + 1
            pla
            sta         ED_N
            pla
            inc
            bra         @sub

@digit:
            pla
            pla                                             ; (.A = the digit)
            cpx         #0                                  ; (The ones: always)
            beq         @put
            cmp         #'0'
            bne         @started
            bit         ED_M + 1                            ; (A 0 after a digit: a 0)
            bmi         @put
            cpx         #8                                  ; A leading 0: nothing in the 10000s; else a
            beq         @next                               ;   space, or nothing
            lda         ED_M
            beq         @next
            bra         @put

@started:
            pha
            lda         #$80
            sta         ED_M + 1
            pla

@put:
            jsr         WRITE_CHAR

@next:
            dex
            dex
            bpl         @power
            rts

ED_TENS:    .word       1, 10, 100, 1000, 10000

; CR LF.  Modifies: .A
ED_CRLF:
            lda         #ASCII_CR
            jsr         WRITE_CHAR
            lda         #ASCII_LF
            jmp         WRITE_CHAR

; The zero-terminated text at .A.Y, then CR LF (ED_PUTS), or no new line (ED_PUTS_NL).  Modifies: .A, .Y
ED_PUTS:
            jsr         ED_PUTS_NL
            jmp         ED_CRLF

ED_PUTS_NL:
            sta         ED_M
            sty         ED_M + 1
            ldy         #0
:
            lda         (ED_M),Y
            beq         :+
            jsr         WRITE_CHAR
            iny
            bne         :-
            inc         ED_M + 1                            ; (Longer than 255: the next page)
            bra         :-
:
            rts

; ****************************************************************************
; Input

; A line from stdin into ED_LINE (zero-terminated; 255 characters at most).  It ends at a CR, an LF or a
; CR LF; a backspace erases (with nothing typed yet, the console's echo erased the prompt: it goes back).
; From the console, a new line after it.  OUT: C = 0; or C = 1: the end of the input
ED_GETLINE:
            ldy         #0

@char:
            jsr         GET_CHAR                            ; (C = 1: .A = the character)
            bcc         @eof
            cmp         #ASCII_LF
            bne         @not_lf
            cpy         #0                                  ; An LF: the line's end; but not the LF of a
            bne         @lf_end                             ;   CR LF (at the line's start, after a line
            lda         ED_CR                               ;   that ended with a CR)
            beq         @lf_end
            stz         ED_CR
            bra         @char

@lf_end:
            stz         ED_CR
            bra         @end

@not_lf:
            stz         ED_CR
            cmp         #ASCII_CR
            bne         :+
            sta         ED_CR
            bra         @end
:
            cmp         #ASCII_BACKSPACE
            beq         @erase
            cmp         #$7F                                ; (DEL: as a backspace)
            beq         @erase
            cpy         #ED_LINE_MAX
            bcs         @char                               ; (Full: the rest is left out)
            sta         ED_LINE,Y
            iny
            bra         @char

@erase:
            cpy         #0
            beq         :+
            dey
            bra         @char
:
            lda         ED_LAST                             ; (Nothing to erase: the prompt goes back)
            beq         @char
            jsr         WRITE_CHAR
            bra         @char

@eof:
            cpy         #0                                  ; (A last line with no end: that line first)
            bne         @end
            sec
            rts

@end:
            lda         #0
            sta         ED_LINE,Y
            stz         ED_LAST                             ; (Lines typed after this one have no prompt)
            lda         IO_FD_FLAGS                         ; From the console: a new line (its echo is
            and         #IO_FDF_CONS                        ;   the CR alone)
            beq         :+
            jsr         ED_CRLF
:
            clc
            rts

; ****************************************************************************
; Texts

ED_S_COUNTS:
ED_S_LINES:     .byte   " lines", 0
ED_S_BYTES:     .byte   " bytes", 0
.assert     >ED_S_LINES = >ED_S_COUNTS .and >ED_S_BYTES = >ED_S_COUNTS, error, "ED_COUNT_SAY: its texts on one page"
ED_S_NEW:       .byte   ": new file", 0
ED_S_ERROR:     .byte   "? error $", 0
ED_S_NO_LINE:   .byte   "? no such line", 0
ED_S_WHICH:     .byte   "? which lines", 0
ED_S_FULL:      .byte   "? full", 0
ED_S_BIG:       .byte   "? too big: only the start was read", 0
ED_S_NO_NAME:   .byte   "? no file name (w name)", 0
ED_S_UNSAVED:   .byte   "? not written: q again to quit anyway", 0
ED_S_WHAT:      .byte   "? h: help", 0
ED_S_HELP:      .byte   "p [a[,b]]  print (all)       a [n]      add after n (the last)", ASCII_CR, ASCII_LF
                .byte   "i [n]      insert before n   c a[,b]    change", ASCII_CR, ASCII_LF
                .byte   "d a[,b]    delete            w [file]   write", ASCII_CR, ASCII_LF
                .byte   "q          quit              Q          quit, not writing", ASCII_CR, ASCII_LF
                .byte   "n: a number, or $ (the last).  Lines typed after a, i or c end with a .", 0
