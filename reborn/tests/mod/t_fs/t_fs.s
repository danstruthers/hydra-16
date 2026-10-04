; ****************************************************************************
; t_fs - HydraFS (phase 3.4: #f, in the storage driver's second bank), run as init.  Its fds 0-2 stay closed (its
; lines go out on the bring-up console, and its marks are timed as they're made; the files it opens are moved to fds
; 5 and up).  The emulator's cards (tests.js makes them with the PC tool, and checks them afterwards): 0, a HydraFS
; (hello.txt, games/star.txt, big.bin: byte i is i * 7); 1, blank; 2 and 3, the old system's fixture cards
; (tests-v1, a version 1 volume; quick-v2, a version 2 one); 4, partitioned (a FAT partition first); 5, tests-v1 with
; a lost cluster planted; 6, a blank 1 GB card.  The cards' directory; reading files and directories; stat; create,
; write, read back; a write past the end (a hole); mkdir, a directory not empty, remove; a rename; a file open can't
; be removed; O_TRUNC; read-only; a length cut short and made longer; a full format and a label on a blank card;
; check; the old cards read and written; a partitioned card; the check finding a lost cluster, and fixing it; a big
; card's quick format; mounts with a spec (the ROM disk, read only; a RAM disk; a directory on it); and the time a
; byte takes.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "testlib.inc"

            HYX2_PROGRAM "t_fs", main

BUF_SIZE        = 1024

.zeropage
fd:         .res        1
fd2:        .res        1
ctl:        .res        1
total:      .res        2
cnt:        .res        1
cmdp:       .res        2
want:       .res        2                                   ; (has_text's)

.bss
buf:        .res        BUF_SIZE
pat:        .res        600                                 ; 0, 1, 2 ... (the low bytes)
rec:        .res        SR_SIZE                             ; A stat record

.code

; Open path (a label) with mode, as fd fdn (fds 0-2 stay closed).  OUT: .A = the fd, C
.macro OPEN_  path, mode, fdn
            LDR         r0, path
            lda         #mode
            ldx         #fdn
            jsr         open_as
.endmacro

; Create path with mode and the new file's mode (high byte: DM_DIR), as fd fdn.  OUT: .A = the fd, C
.macro CREATE_ path, mode, perm, fdn
            LDR         r0, path
            lda         #mode
            ldx         #perm
            jsr         CREATE
            ldx         #fdn
            jsr         move_fd
.endmacro

; Read count bytes from fd into buf.  OUT: .A/.X, C
.macro READ_  fdv, count
            LDR         r0, buf
            LDR         r1, count
            lda         fdv
            jsr         READ
.endmacro

; Write len bytes at label to fd.  OUT: .A/.X, C
.macro WRITE_ fdv, label, len
            LDR         r0, label
            LDR         r1, len
            lda         fdv
            jsr         WRITE
.endmacro

; fd's offset to offset (32 bits)
.macro SEEK_  fdv, offset
            LDR         r0, .loword(offset)
            LDR         r1, .hiword(offset)
            lda         fdv
            ldx         #0
            jsr         SEEK
.endmacro

; STAT path into rec.  OUT: C
.macro STAT_  path
            LDR         r0, path
            LDR         r1, rec
            jsr         STAT
.endmacro

; REMOVE path.  OUT: C
.macro REMOVE_ path
            LDR         r0, path
            jsr         REMOVE
.endmacro

; A ctl command (a label, its length) to the ctl file at path; then the file read back into buf.  OUT: .A = the
; command's write's error (or 0), total = the text's length
.macro CTL_   path, cmd, len
            LDR         r0, path
            LDR         r1, cmd
            lda         #len
            jsr         ctl_cmd
.endmacro

main:
            stz         T_FAILS
            ldx         #0                                  ; The pattern
:
            txa
            sta         pat,X
            sta         pat + 256,X
            inx
            bne         :-
            ldx         #600 - 512 - 1
:
            txa
            sta         pat + 512,X
            dex
            bpl         :-

; ---- The cards' directory: none started; a card's file opened starts it
            OPEN_       s_hf, O_READ, 5
            sta         fd
            EXPECT_OK   "OPEN #f (the cards' directory)"
            jsr         readall
            lda         total
            EXPECT_A    0, "#f: no card started yet"
            OPEN_       s_hello, O_READ, 5
            sta         fd
            EXPECT_OK   "OPEN #f/0/hello.txt: card 0 started, its HydraFS read"
            READ_       fd, 64
            EXPECT_A    15, "hello.txt: 15 bytes"
            lda         buf + 7
            EXPECT_A    'h', "hello, hydrafs"
            lda         fd
            jsr         CLOSE
            OPEN_       s_hf, O_READ, 5
            sta         fd
            jsr         readall
            lda         total
            EXPECT_A    SR_SIZE, "#f: card 0 now"
            lda         buf + SR_NAME
            EXPECT_A    '0', "0"
            lda         buf + SR_QTYPE
            EXPECT_A    QT_DIR, "a directory"

; ---- A directory's records; stat; a read across a cluster's end
            OPEN_       s_card0, O_READ, 5
            sta         fd
            jsr         readall
            lda         total
            EXPECT_A    3 * SR_SIZE, "#f/0: 3 entries"
            lda         buf + SR_SIZE + SR_NAME
            EXPECT_A    'g', "games second"
            lda         buf + SR_SIZE + SR_QTYPE
            EXPECT_A    QT_DIR, "games: a directory"
            lda         buf + 2 * SR_SIZE + SR_LENGTH + 1
            EXPECT_A    >9000, "big.bin: 9000 bytes (its record)"
            STAT_       s_big
            EXPECT_OK   "STAT #f/0/big.bin"
            lda         rec + SR_LENGTH
            EXPECT_A    <9000, "its length: 9000"
            lda         rec + SR_DEV
            EXPECT_A    'f', "its device: f"
            OPEN_       s_big, O_READ, 5
            sta         fd
            SEEK_       fd, 4090
            READ_       fd, 12
            EXPECT_A    12, "big.bin at 4090: 12 bytes"
            lda         buf
            EXPECT_A    <(4090 * 7), "its byte 4090"
            lda         buf + 11
            EXPECT_A    <(4101 * 7), "its byte 4101 (the next cluster's)"
            SEEK_       fd, 0                               ; The time it takes: 8192 bytes, in 512-byte reads
            MARK        "<b0"
            jsr         keep
            MARK        "b0>"
            MARK        "<file"
            lda         #16
            sta         cnt
:
            READ_       fd, 512
            dec         cnt
            bne         :-
            jsr         keep
            MARK        "file>"
            lda         total
            EXPECT_A    0, "8192 bytes of big.bin read (the last: 512)"
            lda         fd
            jsr         CLOSE

; ---- Create, write, read back; past the end (a hole)
            CREATE_     s_new, O_RDWR, 0, 5
            sta         fd
            EXPECT_OK   "CREATE #f/0/new.txt"
            WRITE_      fd, pat, 600
            cpx         #>600
            beq         :+
            lda         #$EE
:
            EXPECT_A    <600, "a write of 600 bytes (two blocks)"
            lda         fd
            jsr         CLOSE
            OPEN_       s_new, O_RDWR, 5
            sta         fd
            READ_       fd, 1024
            cpx         #>600
            beq         :+
            lda         #$EE
:
            EXPECT_A    <600, "read back: 600 bytes"
            lda         buf + 599
            EXPECT_A    <599, "... as written"
            SEEK_       fd, 20000                           ; Past its end: a hole between
            WRITE_      fd, s_end, 3
            EXPECT_A    3, "a write at 20000 (past the end)"
            LDR         r0, rec
            lda         fd
            jsr         FSTAT
            lda         rec + SR_LENGTH + 1
            EXPECT_A    >20003, "its length: 20003"
            SEEK_       fd, 10000
            lda         #$55
            sta         buf + 3
            READ_       fd, 4
            lda         buf + 3
            EXPECT_A    0, "at 10000: zeros (a hole)"
            SEEK_       fd, 20000
            READ_       fd, 64
            EXPECT_A    3, "at 20000: the 3 bytes"
            lda         buf
            EXPECT_A    'e', "end"
            lda         fd
            jsr         CLOSE

; ---- A directory: made, not empty, emptied, removed
            CREATE_     s_dir, O_READ, DM_DIR, 5
            sta         fd
            EXPECT_OK   "CREATE #f/0/dir, a directory"
            lda         fd
            jsr         CLOSE
            CREATE_     s_dirf, O_WRITE, 0, 5
            sta         fd
            EXPECT_OK   "CREATE #f/0/dir/f.txt"
            WRITE_      fd, s_end, 1
            lda         fd
            jsr         CLOSE
            REMOVE_     s_dir
            EXPECT_ERR  E_NOTEMPTY, "REMOVE #f/0/dir: not empty"
            REMOVE_     s_dirf
            EXPECT_OK   "REMOVE #f/0/dir/f.txt"
            REMOVE_     s_dir
            EXPECT_OK   "REMOVE #f/0/dir"
            STAT_       s_dir
            EXPECT_ERR  E_NOENT, "it's gone"

; ---- A rename; an open file can't be removed; O_TRUNC; read-only
            jsr         rec_mode                            ; A record: the new name, nothing else
            ldx         #0
:
            lda         s_renamed,X
            sta         rec + SR_NAME,X
            beq         :+
            inx
            bra         :-
:
            LDR         r0, s_new
            LDR         r1, rec
            jsr         WSTAT
            EXPECT_OK   "WSTAT: new.txt renamed renamed.txt"
            STAT_       s_new
            EXPECT_ERR  E_NOENT, "new.txt: gone"
            STAT_       s_ren
            EXPECT_OK   "renamed.txt: there"
            lda         rec + SR_LENGTH
            EXPECT_A    <20003, "with its length"
            OPEN_       s_ren, O_READ, 5
            sta         fd
            REMOVE_     s_ren
            EXPECT_ERR  E_BUSY, "REMOVE renamed.txt while it's open: E_BUSY"
            lda         fd
            jsr         CLOSE
            OPEN_       s_ren, O_WRITE | O_TRUNC, 5
            sta         fd
            EXPECT_OK   "OPEN renamed.txt, O_TRUNC"
            LDR         r0, rec
            lda         fd
            jsr         FSTAT
            lda         rec + SR_LENGTH + 1
            ora         rec + SR_LENGTH
            EXPECT_A    0, "it's empty"
            WRITE_      fd, s_trunc, 5
            lda         fd
            jsr         CLOSE
            jsr         rec_mode                            ; r--r--r--: read-only
            lda         #<$124
            sta         rec + SR_MODE
            lda         #>$124
            sta         rec + SR_MODE + 1
            LDR         r0, s_ren
            LDR         r1, rec
            jsr         WSTAT
            EXPECT_OK   "WSTAT: renamed.txt read-only"
            OPEN_       s_ren, O_WRITE, 5
            EXPECT_ERR  E_PERM, "OPEN it for writing: E_PERM"
            jsr         rec_mode                            ; rw-rw-rw- again
            lda         #<$1B6
            sta         rec + SR_MODE
            lda         #>$1B6
            sta         rec + SR_MODE + 1
            LDR         r0, s_ren
            LDR         r1, rec
            jsr         WSTAT

; ---- A length (FWSTAT): cut short (an extent in an extent block, a hole, part of an extent), made longer (zeros)
            CREATE_     s_cut, O_RDWR, 0, 5
            sta         fd
            EXPECT_OK   "CREATE #f/0/cut.bin"
            lda         #15
            sta         cnt
:
            WRITE_      fd, pat, 600
            dec         cnt
            bne         :-
            SEEK_       fd, 20000                           ; (9000 bytes: 3 clusters; then a hole of 1; then 1,
            WRITE_      fd, s_end, 3                        ;   in an extent block)
            LDR         r0, 5000
            jsr         cut_len
            EXPECT_OK   "FWSTAT: cut.bin cut to 5000 bytes"
            LDR         r0, rec
            lda         fd
            jsr         FSTAT
            lda         rec + SR_LENGTH + 1
            EXPECT_A    >5000, "its length: 5000"
            SEEK_       fd, 4990
            READ_       fd, 64
            EXPECT_A    10, "at 4990: 10 bytes"
            lda         buf
            EXPECT_A    190, "as written"
            LDR         r0, 7000
            jsr         cut_len
            EXPECT_OK   "FWSTAT: made 7000 bytes"
            SEEK_       fd, 4998
            READ_       fd, 64
            EXPECT_A    64, "at 4998: 64 bytes (of 2002)"
            lda         buf + 1
            EXPECT_A    199, "as written"
            lda         buf + 2
            EXPECT_A    0, "then zeros"
            lda         fd
            jsr         CLOSE
            REMOVE_     s_cut
            EXPECT_OK   "REMOVE cut.bin"

; ---- Format and label a blank card; check card 0
            OPEN_       s_card1, O_READ, 5
            EXPECT_ERR  E_NOTFS, "#f/1, a blank card: E_NOTFS"
            CTL_        s_ctl1, s_format, 16
            EXPECT_A    0, "#d/1/ctl: format -f SECOND (a full format)"
            jsr         has_label
            EXPECT_A    'S', "its ctl file: hydrafs label=SECOND"
            OPEN_       s_card1, O_READ, 5
            sta         fd
            EXPECT_OK   "#f/1: its root"
            jsr         readall
            lda         total
            EXPECT_A    0, "empty"
            CREATE_     s_a1, O_WRITE, 0, 5
            sta         fd
            EXPECT_OK   "CREATE #f/1/a.txt"
            WRITE_      fd, s_on1, 9
            EXPECT_A    9, "a write of 9"
            lda         fd
            jsr         CLOSE
            CTL_        s_ctl1, s_label, 14
            EXPECT_A    0, "#d/1/ctl: label NEW NAME"
            jsr         has_label
            EXPECT_A    'N', "hydrafs label=NEW NAME"
            CTL_        s_ctl0, s_check, 5
            EXPECT_A    0, "#d/0/ctl: check"
            LDR         want, s_clean
            jsr         has_text
            EXPECT_A    0, "check: lost 0, unmarked 0, twice 0"

; ---- The old system's cards: read, and written
            OPEN_       s_v1hello, O_READ, 5
            sta         fd
            EXPECT_OK   "OPEN #f/2/hello.txt (tests-v1: version 1)"
            READ_       fd, 64
            EXPECT_A    13, "13 bytes"
            lda         buf + 6
            EXPECT_A    'h', "hello hydra"
            lda         fd
            jsr         CLOSE
            OPEN_       s_v1a, O_READ, 5
            sta         fd
            SEEK_       fd, 16200
            READ_       fd, 512
            EXPECT_A    16384 - 16200, "#f/2/a: 16K in three extents, the last in an extent block (its end)"
            lda         fd
            jsr         CLOSE
            OPEN_       s_v2data, O_READ, 5
            sta         fd
            EXPECT_OK   "OPEN #f/3/data.bin (quick-v2: version 2)"
            SEEK_       fd, 8999
            READ_       fd, 64
            EXPECT_A    1, "its last byte"
            lda         buf
            EXPECT_A    <(8999 * 7), "as written"
            lda         fd
            jsr         CLOSE
            CREATE_     s_v1new, O_WRITE, 0, 5
            sta         fd
            WRITE_      fd, s_reborn, 11
            EXPECT_A    11, "#f/2/hydra.txt written"
            lda         fd
            jsr         CLOSE
            CREATE_     s_v2new, O_WRITE, 0, 5
            sta         fd
            WRITE_      fd, s_reborn, 11
            EXPECT_A    11, "#f/3/hydra.txt written"
            lda         fd
            jsr         CLOSE

; ---- A partitioned card; the check finding a lost cluster, and fixing it; a big card's quick format
            OPEN_       s_part, O_READ, 5
            sta         fd
            EXPECT_OK   "OPEN #f/4/part.txt (a partitioned card: a FAT partition first)"
            READ_       fd, 64
            EXPECT_A    15, "in a partition (15 bytes)"
            lda         fd
            jsr         CLOSE
            CTL_        s_ctl4, s_check, 0
            LDR         want, s_partat
            jsr         has_text
            EXPECT_A    0, "#d/4/ctl: partition at block ..."
            CTL_        s_ctl5, s_check, 5
            LDR         want, s_lost1
            jsr         has_text
            EXPECT_A    0, "#d/5/ctl, check: lost 1, unmarked 0, twice 0"
            CTL_        s_ctl5, s_checkfix, 9
            LDR         want, s_fixed
            jsr         has_text
            EXPECT_A    0, "check fix: ..., fixed"
            CTL_        s_ctl6, s_formatbig, 10
            EXPECT_A    0, "#d/6/ctl: format BIG (1 GB, quick)"
            CREATE_     s_bigf, O_WRITE, 0, 5
            sta         fd
            WRITE_      fd, s_onbig, 13
            EXPECT_A    13, "#f/6/big.txt written"
            lda         fd
            jsr         CLOSE

; ---- Mounts with a spec: the ROM disk (no HydraFS on it yet); a RAM disk; a directory on it
            LDR         r0, s_hroot
            LDR         r1, s_root
            lda         #MREPL
            jsr         BIND
            LDR         r0, s_x
            LDR         r1, s_rom
            ldx         #'f'
            lda         #MREPL
            jsr         MOUNT
            EXPECT_OK   "mount '#f' /rom x"
            OPEN_       s_romread, O_READ, 5
            sta         fd
            EXPECT_OK   "OPEN /rom/README: the ROM disk's HydraFS"
            READ_       fd, 16
            lda         buf + 4
            EXPECT_A    'H', "The Hydra-16's ROM disk"
            lda         fd
            jsr         CLOSE
            CREATE_     s_romnew, O_WRITE, 0, 5
            EXPECT_ERR  E_ROFS, "CREATE /rom/new: read only (E_ROFS)"
            OPEN_       s_fx, O_READ, 5
            EXPECT_ERR  E_NOENT, "#f/x: a disk in memory only through a spec"
            CTL_        s_ctlr, s_start, 7
            EXPECT_A    0, "#d/r/ctl: start 8"
            LDR         r0, s_r
            LDR         r1, s_ram
            ldx         #'f'
            lda         #MREPL
            jsr         MOUNT
            EXPECT_OK   "mount '#f' /ram r"
            CREATE_     s_ramt, O_RDWR, 0, 5
            sta         fd
            EXPECT_OK   "CREATE /ram/t.txt"
            WRITE_      fd, pat, 600
            SEEK_       fd, 300
            READ_       fd, 16
            lda         buf
            EXPECT_A    <300, "read back from the RAM disk"
            lda         fd
            jsr         CLOSE
            CREATE_     s_ramsub, O_READ, DM_DIR, 5
            EXPECT_OK   "mkdir /ram/sub"
            lda         #5
            jsr         CLOSE
            LDR         r0, s_rsub
            LDR         r1, s_tmp
            ldx         #'f'
            lda         #MREPL
            jsr         MOUNT
            EXPECT_OK   "mount '#f' /tmp r/sub"
            CREATE_     s_tmpin, O_WRITE, 0, 5
            EXPECT_OK   "CREATE /tmp/in.txt"
            lda         #5
            jsr         CLOSE
            STAT_       s_ramin
            EXPECT_OK   "it's /ram/sub/in.txt"
            DONE        "t_fs"

; A record that changes nothing, in rec: its name empty, the rest $FF, as Plan 9's (the caller puts in what it
; changes)
rec_mode:
            ldx         #SR_SIZE - 1
            lda         #$FF
:
            sta         rec,X
            dex
            bpl         :-
            stz         rec + SR_NAME
            rts

; fd's file made r0 bytes long (FWSTAT).  OUT: C
cut_len:
            jsr         rec_mode
            lda         r0
            sta         rec + SR_LENGTH
            lda         r0 + 1
            sta         rec + SR_LENGTH + 1
            stz         rec + SR_LENGTH + 2
            stz         rec + SR_LENGTH + 3
            LDR         r0, rec
            lda         fd
            jmp         FWSTAT

; .A = the character after "label=" in the ctl text in buf (total long), or 0
has_label:
            ldx         #0
@look:
            cpx         total
            bcs         @none
            lda         buf,X
            cmp         #'='
            beq         @found
            inx
            bra         @look
@found:
            lda         buf + 1,X
            rts
@none:
            lda         #0
            rts

; .A = 0 if the ctl text in buf (total long) has the text at want in it
has_text:
            ldx         #0                                  ; (.X: where it might start)
@at:
            phx
            ldy         #0
@char:
            lda         (want),Y
            beq         @yes
            lda         buf,X
            cmp         (want),Y
            bne         @next
            inx
            iny
            bra         @char
@next:
            plx
            inx
            cpx         total
            bcc         @at
            lda         #$EE
            rts
@yes:
            plx
            lda         #0
            rts

; A ctl command: r0 = the ctl file's name, r1 = the command, .A = its length.  Then the file read back (total, buf).
; OUT: .A = the write's error, or 0
ctl_cmd:
            sta         cnt
            MOVR        cmdp, r1                            ; (The command, a moment)
            lda         #O_RDWR
            ldx         #6
            jsr         open_as
            bcs         @done
            sta         ctl
            MOVR        r0, cmdp
            lda         cnt
            sta         r1
            stz         r1 + 1
            lda         ctl
            jsr         WRITE
            php
            pha
            SEEK_       ctl, 0
            READ_       ctl, BUF_SIZE
            sta         total
            stx         total + 1
            lda         ctl
            jsr         CLOSE
            pla
            plp
            bcs         @done
            lda         #0
@done:
            rts

; .A/.X kept in total (across a MARK)
keep:
            sta         total
            stx         total + 1
            rts

; OPEN r0 with .A, the fd then moved to .X.  OUT: .A = .X; or C = 1, .A = the error
open_as:
            phx
            jsr         OPEN
            plx
move_fd:                                                    ; (Or a fd made otherwise: .A, C)
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

; Read fd to its end into buf (BUF_SIZE bytes at most): total = the bytes; the fd closed
readall:
            stz         total
            stz         total + 1
@read:
            clc                                             ; Into buf + total, the room left
            lda         #<buf
            adc         total
            sta         r0
            lda         #>buf
            adc         total + 1
            sta         r0 + 1
            sec
            lda         #<BUF_SIZE
            sbc         total
            sta         r1
            lda         #>BUF_SIZE
            sbc         total + 1
            sta         r1 + 1
            ora         r1
            beq         @done
            lda         fd
            jsr         READ
            bcs         @done
            sta         r2
            stx         r2 + 1
            ora         r2 + 1
            beq         @done
            clc
            lda         total
            adc         r2
            sta         total
            lda         total + 1
            adc         r2 + 1
            sta         total + 1
            bra         @read

@done:
            lda         fd
            jmp         CLOSE

.rodata
s_hf:       .byte       "#f", 0
s_hello:    .byte       "#f/0/hello.txt", 0
s_card0:    .byte       "#f/0", 0
s_big:      .byte       "#f/0/big.bin", 0
s_new:      .byte       "#f/0/new.txt", 0
s_ren:      .byte       "#f/0/renamed.txt", 0
s_renamed:  .byte       "renamed.txt", 0
s_cut:      .byte       "#f/0/cut.bin", 0
s_dir:      .byte       "#f/0/dir", 0
s_dirf:     .byte       "#f/0/dir/f.txt", 0
s_card1:    .byte       "#f/1", 0
s_a1:       .byte       "#f/1/a.txt", 0
s_ctl0:     .byte       "#d/0/ctl", 0
s_ctl1:     .byte       "#d/1/ctl", 0
s_ctlr:     .byte       "#d/r/ctl", 0
s_v1hello:  .byte       "#f/2/hello.txt", 0
s_v1a:      .byte       "#f/2/a", 0
s_v2data:   .byte       "#f/3/data.bin", 0
s_v1new:    .byte       "#f/2/hydra.txt", 0
s_v2new:    .byte       "#f/3/hydra.txt", 0
s_hroot:    .byte       "#/", 0
s_root:     .byte       "/", 0
s_rom:      .byte       "/rom", 0
s_romread:  .byte       "/rom/README", 0
s_romnew:   .byte       "/rom/new", 0
s_ram:      .byte       "/ram", 0
s_tmp:      .byte       "/tmp", 0
s_fx:       .byte       "#f/x", 0
s_x:        .byte       "x", 0
s_r:        .byte       "r", 0
s_rsub:     .byte       "r/sub", 0
s_ramt:     .byte       "/ram/t.txt", 0
s_ramsub:   .byte       "/ram/sub", 0
s_tmpin:    .byte       "/tmp/in.txt", 0
s_ramin:    .byte       "/ram/sub/in.txt", 0
s_end:      .byte       "end"
s_trunc:    .byte       "trunc"
s_on1:      .byte       "on card 1"
s_reborn:   .byte       "from reborn"
s_format:   .byte       "format -f SECOND"
s_label:    .byte       "label NEW NAME"
s_check:    .byte       "check"
s_start:    .byte       "start 8"
s_clean:    .byte       "lost 0, unmarked 0, twice 0", 0
s_lost1:    .byte       "lost 1, unmarked 0, twice 0", 0
s_fixed:    .byte       "twice 0, fixed", 0
s_partat:   .byte       "partition at block", 0
s_checkfix: .byte       "check fix"
s_formatbig: .byte      "format BIG"
s_onbig:    .byte       "on a big card"
s_part:     .byte       "#f/4/part.txt", 0
s_ctl4:     .byte       "#d/4/ctl", 0
s_ctl5:     .byte       "#d/5/ctl", 0
s_ctl6:     .byte       "#d/6/ctl", 0
s_bigf:     .byte       "#f/6/big.txt", 0
