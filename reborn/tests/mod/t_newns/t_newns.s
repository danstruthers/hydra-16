; ****************************************************************************
; t_newns - the default namespace's library (sdk/asm/nslib.s, phase 3.6), run as init (its fds 0-2 closed: newns's
; lines go out on the bring-up console, where tests.js looks for them).  A task's own area of the RAM disk: one left by
; a task before it, with files and a directory in it, emptied and made again with bin and lib; then a namespace file
; (written to the RAM disk) run: comments, quotes, $task, bind's and mount's flags (-a, -b, -c), a mount with a spec,
; a line that isn't a command and one with too few words said, and the rest run on.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "testlib.inc"

            HYX2_PROGRAM "t_newns", main

.zeropage
fd:         .res        1
total:      .res        2
task:       .res        1

.bss
buf:        .res        512
name:       .res        32                                  ; "#fr/T..."
rec:        .res        SR_SIZE

.code
main:
            stz         T_FAILS
            LDR         r0, s_ctlr                          ; The RAM disk
            lda         #O_WRITE
            jsr         OPEN
            sta         fd
            LDR         r0, s_start
            LDR         r1, 7
            lda         fd
            jsr         WRITE
            EXPECT_OK   "#d/r/ctl: start 4"
            lda         fd
            jsr         CLOSE
            jsr         GETPID
            sta         task
            sta         ns_task                             ; (nslib's: ns_area's task)

; ---- An area left by a task before it: a file, and a directory with a file in it
            LDR         r1, s_none
            jsr         area_name
            jsr         mkdir
            EXPECT_OK   "the area made, as a task before this one left it"
            LDR         r1, s_old
            jsr         area_name
            jsr         mkfile
            LDR         r1, s_sub
            jsr         area_name
            jsr         mkdir
            LDR         r1, s_subfile
            jsr         area_name
            jsr         mkfile
            EXPECT_OK   "with a file, and a directory with a file in it"
            jsr         ns_area
            LDR         r1, s_none                          ; What's in it now: bin and lib
            jsr         area_name
            jsr         readall
            lda         total
            EXPECT_A    2 * SR_SIZE, "ns_area: the area emptied, then bin and lib made in it"
            lda         buf + SR_NAME
            EXPECT_A    'b', "bin"
            lda         buf + SR_SIZE + SR_NAME
            EXPECT_A    'l', "lib"

; ---- A namespace file, run
            LDR         r1, s_nsfile
            jsr         area_name
            LDR         r0, name
            lda         #O_WRITE
            ldx         #0
            jsr         CREATE
            sta         fd
            LDR         r0, s_file
            LDR         r1, S_FILE_LEN
            lda         fd
            jsr         WRITE
            lda         fd
            jsr         CLOSE
            LDR         r0, name
            jsr         ns_file
            EXPECT_OK   "ns_file: its lines run (two of them said: tests.js looks)"
            LDR         r0, s_null
            lda         #O_READ
            jsr         OPEN
            sta         fd
            EXPECT_OK   "/dev/null: bind -a '#n' /dev (after a comment line, a comment after it)"
            lda         fd
            jsr         CLOSE
            LDR         r0, s_tmpbin
            LDR         r1, rec
            jsr         STAT
            EXPECT_OK   "/tmp/bin: mount -c '#f' /tmp r/$task (a spec with $task)"
            LDR         r0, s_created
            lda         #O_WRITE
            ldx         #0
            jsr         CREATE
            sta         fd
            EXPECT_OK   "CREATE /tmp/made"
            lda         fd
            jsr         CLOSE
            LDR         r1, s_made
            jsr         area_name
            LDR         r0, name
            LDR         r1, rec
            jsr         STAT
            EXPECT_OK   "it's #fr/T/made: $task was this task"
            LDR         r0, s_binbin
            LDR         r1, rec
            jsr         STAT
            EXPECT_OK   "/bin/bin: bind -b '/tmp' /bin (quoted), a union with #/'s bin"
            LDR         r0, s_hidden
            lda         #O_READ
            jsr         OPEN
            sta         fd
            EXPECT_OK   "/mnt/null: the last line ran, after the bad ones"
            lda         fd
            jsr         CLOSE
            DONE        "t_newns"

; name = "#fr/T" (T: this task, in decimal) and the text at r1 after it
area_name:
            ldx         #0
:
            lda         s_area,X
            sta         name,X
            beq         :+
            inx
            bra         :-
:
            lda         task
            cmp         #10
            bcc         :+
            sbc         #10
            pha
            lda         #'1'
            sta         name,X
            inx
            pla
:
            ora         #'0'
            sta         name,X
            inx
            ldy         #0
:
            lda         (r1),Y
            sta         name,X
            beq         :+
            inx
            iny
            bra         :-
:
            rts

; The directory name (or the file: mkfile) made.  OUT: C
mkdir:
            LDR         r0, name
            lda         #O_READ
            ldx         #DM_DIR
            bra         :+
mkfile:
            LDR         r0, name
            lda         #O_WRITE
            ldx         #0
:
            jsr         CREATE
            bcs         :+
            jsr         CLOSE
            clc
:
            rts

; The directory name read whole into buf: total = its bytes
readall:
            stz         total
            stz         total + 1
            LDR         r0, name
            lda         #O_READ
            jsr         OPEN
            bcs         @done
            sta         fd
            LDR         r0, buf
            LDR         r1, 512
            lda         fd
            jsr         READ
            bcs         :+
            sta         total
            stx         total + 1
:
            lda         fd
            jsr         CLOSE
@done:
            rts

.rodata
s_ctlr:     .byte       "#d/r/ctl", 0
s_start:    .byte       "start 4"
s_area:     .byte       "#fr/", 0
s_none:     .byte       0
s_old:      .byte       "/old.txt", 0
s_sub:      .byte       "/d", 0
s_subfile:  .byte       "/d/e.txt", 0
s_nsfile:   .byte       "/ns", 0
s_made:     .byte       "/made", 0
s_null:     .byte       "/dev/null", 0
s_tmpbin:   .byte       "/tmp/bin", 0
s_created:  .byte       "/tmp/made", 0
s_binbin:   .byte       "/bin/bin", 0
s_hidden:   .byte       "/mnt/null", 0
s_file:     .byte       "# a namespace file", LF
            .byte       "bind '#/' /", LF
            .byte       "bind -a '#n' /dev          # null zero", LF
            .byte       CR, LF
            .byte       "mount -c '#f' /tmp r/$task", LF
            .byte       "bind -b '/tmp' /bin", LF
            .byte       "frob /dev", LF
            .byte       "bind /tmp", LF
            .byte       "bind '#n' /mnt", LF
S_FILE_LEN  = * - s_file

.include "nslib.s"
