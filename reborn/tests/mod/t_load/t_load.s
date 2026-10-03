; ****************************************************************************
; t_load - SPAWN by path and the loader (phase 4.1), run as init (its fds 0-2 closed: its lines, and its children's,
; go out on the bring-up console; the fds it opens are moved to 5 on), with t_child, and a card (tests.js) whose bin has t_ram and t_big (RAM programs:
; tests/ram), t_short (t_ram cut short) and t_low (t_ram's header saying $0400), and hello.txt.  /bin is the card's
; bin, then #m/bin (as the namespace file has it): a module found there runs in place; a RAM program from the card is
; read into its task's RAM, given its arguments and the fds of a map (and the same with an empty namespace).  SPAWN's
; errors: a file that isn't a program, none at all, a program loading below $0800, a map too long, a driver; a file
; ending before its image does (its task ends, E_NOEXEC).  The time to load 16K from the card, and from the RAM disk;
; and SPAWN's own time for a module in place.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "testlib.inc"

            HYX2_PROGRAM "t_load", main

.zeropage
fd:         .res        1
rfd:        .res        1
wfd:        .res        1
child:      .res        1
spawned:    .res        1                                   ; (Bit 7: SPAWN's C, said once the child is done)
code:       .res        1

.bss
buf:        .res        512
map:        .res        SPAWN_FDS + 2

.code

; SPAWN path (a label) with args (a label, or 0) and flags; the map in map.  OUT: .A, C
.macro SPAWN_ path, args, flags
            LDR         r0, path
            LDR         r1, args
            LDR         r2, map
            lda         #flags
            jsr         SPAWN
.endmacro

; WAIT for the child.  OUT: .A = its exit code
.macro WAITCHILD
            stz         r0
            stz         r0 + 1
            lda         child
            jsr         WAIT
            txa
.endmacro

; SPAWN's result (C) kept, the child waited for (code: its exit code), then SPAWN's result said: a child that says
; things says them first (the bring-up console's lines don't mix)
.macro SPAWNED  text
            sta         child
            ror         spawned
            WAITCHILD
            sta         code
            asl         spawned
            lda         child
            EXPECT_OK   text
.endmacro

main:
            stz         T_FAILS
            LDR         r0, s_hroot                         ; The namespace: /bin the card's, then #m/bin
            LDR         r1, s_root
            lda         #MREPL
            jsr         BIND
            stz         r0
            stz         r0 + 1
            LDR         r1, s_sd
            ldx         #'f'
            lda         #MREPL
            jsr         MOUNT
            LDR         r0, s_sdbin
            LDR         r1, s_bin
            lda         #MREPL
            jsr         BIND
            LDR         r0, s_mbin
            LDR         r1, s_bin
            lda         #MAFTER
            jsr         BIND
            EXPECT_OK   "the namespace: /bin, the card's bin then #m/bin"

; ---- A module in place, by its path
            SPAWN_      s_bchild, s_e7, 0
            sta         child
            EXPECT_OK   "SPAWN /bin/t_child (#m/bin's: in place)"
            WAITCHILD
            EXPECT_A    '7', "t_child ran: its code"

; ---- A RAM program from the card: its arguments, its fds the map's
            jsr         pipe
            SPAWN_      s_bram, s_abc, SPAWN_FDMAP
            SPAWNED     "SPAWN /bin/t_ram (the card's: a RAM program), with a map"
            jsr         ramdone
            jsr         pipe                                ; (Again, its namespace empty)
            SPAWN_      s_bram, s_abc, SPAWN_FDMAP | SPAWN_NEWNS
            SPAWNED     "SPAWN /bin/t_ram, its namespace empty"
            jsr         ramdone

; ---- SPAWN's errors
            SPAWN_      s_hello, 0, 0
            EXPECT_ERR  E_NOEXEC, "SPAWN of a file that isn't a program: E_NOEXEC"
            SPAWN_      s_bnone, 0, 0
            EXPECT_ERR  E_NOENT, "SPAWN of /bin/nothing: E_NOENT"
            SPAWN_      s_low, 0, 0
            EXPECT_ERR  E_NOEXEC, "SPAWN of a RAM program loading below $0800: E_NOEXEC"
            SPAWN_      s_kdev, 0, 0
            EXPECT_ERR  E_NOEXEC, "SPAWN of #m/kdev (a driver): E_NOEXEC"
            lda         #SPAWN_FDS + 1
            sta         map
            SPAWN_      s_bchild, s_e7, SPAWN_FDMAP
            EXPECT_ERR  E_INVAL, "SPAWN with a map of 16 fds: E_INVAL"
            SPAWN_      s_short, 0, 0
            sta         child
            EXPECT_OK   "SPAWN of t_short (its header whole)"
            WAITCHILD
            EXPECT_A    E_NOEXEC, "t_short: its file ends before its image does: it ends, E_NOEXEC"

; ---- The time to load 16K: from the card, then from the RAM disk; SPAWN's own, for a module in place
            MARK        "<big"
            SPAWN_      s_big, 0, 0
            SPAWNED     "SPAWN of t_big (16K) from the card"
            lda         code
            EXPECT_A    $E9, "t_big: its image read whole, in order (its check)"
            jsr         tocopy
            MARK        "<rbig"
            SPAWN_      s_rbig, s_r, 0
            SPAWNED     "SPAWN of t_big from the RAM disk"
            lda         code
            EXPECT_A    $E9, "t_big: its image read whole, in order (its check)"
            MARK        "<b0"                               ; (A baseline: the marks' own time)
            MARK        "b0>"
            MARK        "<sp"
            SPAWN_      s_bchild, s_e7, 0
            sta         child
            MARK        "sp>"
            WAITCHILD
            EXPECT_A    '7', "t_child, by /bin's union, the card's bin first"
            MARK        "<msp"
            SPAWN_      s_mchild, s_e7, 0
            sta         child
            MARK        "msp>"
            WAITCHILD
            EXPECT_A    '7', "t_child, by #m/t_child"
            DONE        "t_load"

; Fd .A moved to fd .X (fd 1 stays closed: this task's lines go to the bring-up console).  OUT: .A = .X
move:
            phx
            pha
            jsr         DUP2
            pla
            jsr         CLOSE
            pla
            rts

; A pipe (rfd, wfd), and the map: fds 0-2 closed, fd 3 its write end
pipe:
            jsr         PIPE
            phx
            ldx         #5
            jsr         move
            sta         rfd
            pla
            ldx         #6
            jsr         move
            sta         wfd
            lda         #4
            sta         map
            lda         #$FF
            sta         map + 1
            sta         map + 2
            sta         map + 3
            lda         wfd
            sta         map + 4
            rts

; t_ram's code (its failures), its "hi" read from the pipe, and the pipe closed
ramdone:
            lda         code
            EXPECT_A    0, "t_ram's own checks passed (its code: its failures)"
            lda         wfd
            jsr         CLOSE
            LDR         r0, buf
            LDR         r1, 16
            lda         rfd
            jsr         READ
            EXPECT_A    2, "t_ram's hi, from the pipe: 2 bytes (then the end)"
            lda         buf
            EXPECT_A    'h', "hi"
            lda         rfd
            jmp         CLOSE

; t_big copied from the card to the RAM disk (started: 4 banks), 512 bytes at a time
tocopy:
            LDR         r0, s_ctlr
            lda         #O_WRITE
            jsr         OPEN
            sta         fd
            LDR         r0, s_start
            LDR         r1, 7
            lda         fd
            jsr         WRITE
            lda         fd
            jsr         CLOSE
            LDR         r0, s_big
            lda         #O_READ
            jsr         OPEN
            ldx         #5
            jsr         move
            sta         rfd
            LDR         r0, s_rbig
            lda         #O_WRITE
            ldx         #0
            jsr         CREATE
            php
            ldx         #6
            jsr         move
            sta         wfd
            plp
            EXPECT_OK   "t_big's copy made on the RAM disk"
@part:
            LDR         r0, buf
            LDR         r1, 512
            lda         rfd
            jsr         READ
            bcs         @done
            sta         r1
            stx         r1 + 1
            ora         r1 + 1
            beq         @done
            LDR         r0, buf
            lda         wfd
            jsr         WRITE
            bra         @part

@done:
            lda         rfd
            jsr         CLOSE
            lda         wfd
            jmp         CLOSE

.rodata
s_hroot:    .byte       "#/", 0
s_root:     .byte       "/", 0
s_sd:       .byte       "/sd", 0
s_sdbin:    .byte       "/sd/0/bin", 0
s_bin:      .byte       "/bin", 0
s_mbin:     .byte       "#m/bin", 0
s_bchild:   .byte       "/bin/t_child", 0
s_mchild:   .byte       "#m/t_child", 0
s_bram:     .byte       "/bin/t_ram", 0
s_bnone:    .byte       "/bin/nothing", 0
s_hello:    .byte       "/sd/0/hello.txt", 0
s_low:      .byte       "/sd/0/bin/t_low", 0
s_short:    .byte       "/sd/0/bin/t_short", 0
s_big:      .byte       "/sd/0/bin/t_big", 0
s_rbig:     .byte       "#fr/t_big", 0
s_kdev:     .byte       "#m/kdev", 0
s_ctlr:     .byte       "#d/r/ctl", 0
s_start:    .byte       "start 4"
s_e7:       .byte       "e7", 0
s_abc:      .byte       "a b c", 0
s_r:        .byte       "r", 0
