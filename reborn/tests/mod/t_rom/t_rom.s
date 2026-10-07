; ****************************************************************************
; t_rom - the ROM disk read back against its sources (phase 3.5), run as init: /rom mounted (#f with the spec x),
; every file in it found by walking its directories, and read whole.  A line for each, "rom: PATH SIZE CRC" (its size
; and a CRC-16 of its bytes, CCITT's: romimg.js's), which tests.js compares with the manifest's sources
; (romfs/romfs.txt).  Its fds 0-2 stay closed, so its lines go out on the bring-up console (the files it opens are
; moved to fds 5 and up).

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "testlib.inc"

            HYX2_PROGRAM "t_rom", main

MAX_DEPTH       = 4                                         ; Directories deep the walk goes
WALK_MAX        = 96

.zeropage
level:      .res        1                                   ; The walk's depth (0: /rom)
len:        .res        1                                   ; The path's length
fd:         .res        1
size:       .res        2                                   ; A file's size ...
crc:        .res        2                                   ;   and its CRC
got:        .res        2                                   ; (A read's count)
files:      .res        1                                   ; The files found

.bss
path:       .res        WALK_MAX
rec:        .res        SR_SIZE
buf:        .res        512
lv_fd:      .res        MAX_DEPTH                           ; Each level's directory: its fd ...
lv_k:       .res        MAX_DEPTH                           ;   its next record ...
lv_len:     .res        MAX_DEPTH                           ;   and the path's length there

.code
main:
            stz         T_FAILS
            stz         files
            LDR         r0, s_hroot                         ; bind '#/' /; mount '#f' /rom x
            LDR         r1, s_root
            lda         #MREPL
            jsr         BIND
            LDR         r0, s_x
            LDR         r1, s_rom
            ldx         #'f'
            lda         #MREPL
            jsr         MOUNT
            EXPECT_OK   "mount '#f' /rom x"
            ldx         #0                                  ; The walk, from /rom
:
            lda         s_rom,X
            sta         path,X
            beq         :+
            inx
            bra         :-
:
            stx         len
            stz         level
            jsr         walk
            lda         files
            beq         :+
            OK          "files found on the ROM disk (tests.js checks each against its source)"
            bra         :++
:
            NOTOK       "files found on the ROM disk"
:
            DONE        "t_rom"

; Walk the directory at path (len long): each entry's record read in turn (at its offset, k * SR_SIZE), a file read
; whole and its line said, a directory walked in its turn
walk:
            LDR         r0, path
            lda         #O_READ
            ldx         level                               ; (As fd 5 + its level)
            inx
            inx
            inx
            inx
            inx
            jsr         open_as
            bcc         :+
            NOTOK       "a directory on the ROM disk opened"
            rts
:
            ldx         level
            sta         lv_fd,X
            stz         lv_k,X
            lda         len
            sta         lv_len,X
@entry:
            ldx         level                               ; Its next record
            lda         lv_k,X
            stz         r0
            lsr
            ror         r0
            lsr
            ror         r0
            sta         r0 + 1                              ; (r0 = k * 64)
            stz         r1
            stz         r1 + 1
            lda         lv_fd,X
            ldx         #0
            jsr         SEEK
            LDR         r0, rec
            LDR         r1, SR_SIZE
            ldx         level
            lda         lv_fd,X
            jsr         READ
            bcs         @done
            cmp         #SR_SIZE
            bne         @done                               ; (None left)
            ldx         level                               ; The path: then / and its name
            ldy         lv_len,X
            lda         #'/'
            sta         path,Y
            iny
            ldx         #0
:
            lda         rec + SR_NAME,X
            sta         path,Y
            beq         :+
            iny
            inx
            cpy         #WALK_MAX - 1
            bcc         :-
            lda         #0
            sta         path,Y
:
            sty         len
            lda         rec + SR_QTYPE
            bmi         @dir
            jsr         file
            bra         @next

@dir:
            lda         level
            cmp         #MAX_DEPTH - 1
            bcs         @next
            inc         level
            jsr         walk
            dec         level
@next:
            ldx         level
            inc         lv_k,X
            ldy         lv_len,X                            ; (The path as it was)
            sty         len
            lda         #0
            sta         path,Y
            jmp         @entry

@done:
            ldx         level
            lda         lv_fd,X
            jmp         CLOSE

; The file at path: read whole, its size and CRC; its line said
file:
            LDR         r0, path
            lda         #O_READ
            ldx         #5 + MAX_DEPTH                      ; (The fd after the directories')
            jsr         open_as
            bcc         :+
            NOTOK       "a file on the ROM disk opened"
            rts
:
            sta         fd
            inc         files
            stz         size
            stz         size + 1
            lda         #$FF                                ; (CRC-16/CCITT: from $FFFF)
            sta         crc
            sta         crc + 1
@read:
            LDR         r0, buf
            LDR         r1, 512
            lda         fd
            jsr         READ
            bcs         @end
            sta         got
            stx         got + 1
            ora         got + 1
            beq         @end
            clc
            lda         size
            adc         got
            sta         size
            lda         size + 1
            adc         got + 1
            sta         size + 1
            LDR         r2, buf                             ; Each byte, into the CRC
@byte:
            lda         got
            ora         got + 1
            beq         @read
            lda         (r2)
            jsr         crc_byte
            inc         r2
            bne         :+
            inc         r2 + 1
:
            lda         got
            bne         :+
            dec         got + 1
:
            dec         got
            bra         @byte

@end:
            lda         fd
            jsr         CLOSE
            LDR         r0, s_line                          ; "rom: PATH SIZE CRC"
            jsr         PUTS
            LDR         r0, path
            jsr         PUTS
            lda         #' '
            jsr         PUTC
            lda         size + 1
            jsr         PUTHEX
            lda         size
            jsr         PUTHEX
            lda         #' '
            jsr         PUTC
            lda         crc + 1
            jsr         PUTHEX
            lda         crc
            jsr         PUTHEX
            LDR         r0, s_crlf
            jmp         PUTS

; OPEN r0 with .A, the fd then moved to .X (fds 0-2 stay closed: the lines go to the bring-up console).  OUT: .A =
; .X; or C = 1, .A = the error
open_as:
            phx
            jsr         OPEN
            plx
            bcs         @done
            phx
            pha
            jsr         DUP2
            pla
            jsr         CLOSE
            pla
            clc
@done:
            rts

; crc on by the byte .A (CRC-16/CCITT: polynomial $1021, MSB first).  Modifies: .A, .X
crc_byte:
            eor         crc + 1
            sta         crc + 1
            ldx         #8
@bit:
            asl         crc
            rol         crc + 1
            bcc         :+
            lda         crc + 1
            eor         #$10
            sta         crc + 1
            lda         crc
            eor         #$21
            sta         crc
:
            dex
            bne         @bit
            rts

.rodata
s_hroot:    .byte       "#/", 0
s_root:     .byte       "/", 0
s_x:        .byte       "x", 0
s_rom:      .byte       "/rom", 0
s_line:     .byte       "rom: ", 0
s_crlf:     .byte       CR, LF, 0
