; ****************************************************************************
; init - the first program (task 1): its fds 0-2 on the console (#c/cons, the console driver's window 0; or, without
; it, none, and the bring-up console's); the RAM disks started (r: 256K, s: 512K, each halved till it fits); its
; namespace from the namespace file (nslib.s's ns_default: its own area of the RAM disk, /rom/lib/namespace, a card's
; /lib/namespace; with no /rom/lib/namespace, the one built in here: the devices at their places); the windows' chrome
; (/lib/windows's lines written to #c/wctl, a card's before the ROM's); the tasks listed;
; hello run and waited for; then window 0's shell (rc -l, a namespace of its own: it builds it, newns, and its
; profile puts its window at /dev) and the windows' starter (wstart: the shell in the next window the user asks for,
; Ctrl-] c), each started again when it ends; and the input controller's driver (input: the Vera X's keyboard and
; mouse, a note group of its own), once (with no controller it ends at once).  The shell is /lib/shell's line, if there is one (its program and
; arguments: /bin/forth -l, HyForth as a shell; a card's /lib/shell, or the shared RAM disk's, /sram/lib/shell), read
; each time one's started, else rc -l; wstart is given it as its arguments.  It waits for every task left to it (the
; windows' shells are).  Its note handler keeps it going.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"

            HYX2_PROGRAM "init", main

.zeropage
child:      .res        1
sh0:        .res        1                                   ; Window 0's shell ...
sw:         .res        1                                   ;   and the windows' starter
fd:         .res        1
banks:      .res        1                                   ; A RAM disk's size, in 8K banks
wfd:        .res        1                                   ; /lib/windows: #c/wctl's fd ...
wn:         .res        1                                   ;   wbuf's bytes ...
wi:         .res        1                                   ;   a line's start, its end
wj:         .res        1

SH_MAX      = 64                                            ; /lib/shell's bytes read, at most

.bss
msg:        .res        32                                  ; An exit message, an error's text
shraw:      .res        SH_MAX                              ; /lib/shell, as read ...
shline:     .res        SH_MAX + 2                          ;   its line's words: the shell's program, then its
shargs:     .res        2                                   ;   arguments (shargs: where), SPAWN's way (each one
                                                            ;   zero-terminated, an empty one after the last)
wbuf:       .res        256                                 ; /lib/windows, a part at a time

.code
main:
            LDR         r0, notes                           ; (Notes don't end it)
            jsr         NOTIFY
            LDR         r0, s_cons                          ; Fds 0-2: the console
            lda         #O_RDWR
            jsr         OPEN
            bcs         :+                                  ; (No console driver: the bring-up console's)
            lda         #0
            jsr         DUP
            lda         #0
            jsr         DUP
:
            jsr         ramdisks
            jsr         ns_default                          ; Its namespace: the file's
            bcc         :+
            PRINT       s_builtin                           ; (None: the one built in)
            jsr         namespace
:
            jsr         windows                             ; The windows' chrome: /lib/windows
            PRINT       s_up
            jsr         GETPID
            jsr         PUTHEX
            PRINT       s_crlf
            jsr         DBG_PS
            LDR         r0, s_hello                         ; hello, with arguments, and its end
            LDR         r1, s_args
            lda         #0
            jsr         SPAWN
            bcs         @failed
            sta         child
            LDR         r0, msg
            lda         child
            jsr         WAIT
            bcs         @failed
            phx
            PRINT       s_ended                             ; hello ended: code $07 ("bye")
            pla
            jsr         PUTHEX
            PRINT       s_open
            PRINT       msg
            PRINT       s_close
            bra         shells

@failed:
            jsr         error

; ****************************************************************************
; The shells: window 0's and the windows' starter, started again when they end; the rest just waited for
shells:
            jsr         shell0
            jsr         starter
            LDR         r0, s_input                         ; The keyboard's and mouse's driver, once
            stz         r1
            stz         r1 + 1
            lda         #SPAWN_NEWGROUP
            jsr         SPAWN
@wait:
            stz         r0
            stz         r0 + 1
            lda         #$FF
            jsr         WAIT
            bcc         @ended
            cmp         #E_CHILD                            ; (None: a moment, then try again)
            bne         @wait
            lda         #TICK_HZ
            ldx         #0
            jsr         SLEEP
            jsr         shell0
            jsr         starter
            bra         @wait

@ended:
            cmp         sh0
            bne         :+
            jsr         shell0
            bra         @wait
:
            cmp         sw
            bne         @wait
            jsr         starter
            bra         @wait

; Window 0's shell (a note group of its own: its window's notes are its), or the windows' starter (in init's
; namespace, so the shell's program is found as here; the shell given it as its arguments), started
shell0:
            jsr         shell_line
            LDR         r0, shline
            lda         shargs
            sta         r1
            lda         shargs + 1
            sta         r1 + 1
            lda         #SPAWN_NEWGROUP | SPAWN_NEWNS
            jsr         SPAWN
            sta         sh0
            bcc         :+
            lda         #$FF
            sta         sh0
:
            rts

starter:
            jsr         shell_line
            LDR         r0, s_wstart
            LDR         r1, shline
            lda         #0
            jsr         SPAWN
            sta         sw
            bcc         :+
            lda         #$FF
            sta         sw
:
            rts

; The shell (shline, shargs): /lib/shell's first line's words, or, with no /lib/shell or none in it, rc -l
shell_line:
            LDR         r0, s_lshell
            lda         #O_READ
            jsr         OPEN
            bcs         @default
            sta         fd
            LDR         r0, shraw
            LDR         r1, SH_MAX
            lda         fd
            jsr         READ
            php
            pha
            lda         fd
            jsr         CLOSE
            pla
            plp
            bcs         @default
            sta         r2                                  ; (Its bytes)
            ldy         #0                                  ; Its words, to its line's end (.Y in, .X out)
            ldx         #0
            stz         shargs
@skip:
            jsr         @at
            bcs         @end
            cmp         #' ' + 1
            bcs         @word
            iny
            bra         @skip
@word:
            jsr         @at
            bcs         @ended
            cmp         #' ' + 1
            bcc         @ended
            sta         shline,X
            inx
            iny
            bra         @word
@ended:
            stz         shline,X                            ; (A word's 0; after the first, its arguments)
            inx
            lda         shargs
            bne         @skip
            stx         shargs
            bra         @skip
@end:
            lda         shargs
            beq         @default
            stz         shline,X                            ; (The empty one after the last)
            clc
            lda         shargs
            adc         #<shline
            sta         shargs
            lda         #>shline
            adc         #0
            sta         shargs + 1
            rts
@default:
            ldx         #S_RCL_LEN - 1
:
            lda         s_rcl,X
            sta         shline,X
            dex
            bpl         :-
            LDR         shargs, shline + S_RC_LEN
            rts
@at:                                                        ; (Byte .Y, if it's on the first line and there's room
            cpy         r2                                  ;   for it: C = 0; else C = 1)
            bcs         :+
            cpx         #SH_MAX - 1
            bcs         :+
            lda         shraw,Y
            cmp         #LF
            beq         :+
            cmp         #CR
            beq         :+
            clc
            rts
:
            sec
            rts

; The RAM disks started: the RAM disk (r: 256K of the storage driver's banks) and the shared one (s: 512K of shared
; RAM), each halved till it fits (a machine with less); each gets an empty HydraFS, and the shared one bin and lib
; (the shared caches).  One that can't start is said
ramdisks:
            LDR         r0, s_ctlr
            lda         #32
            jsr         ramdisk
            LDR         r0, s_ctls
            lda         #64
            jsr         ramdisk
            LDR         r0, s_sbin                          ; The shared caches: its bin and lib
            jsr         mkdir
            LDR         r0, s_slib

; Make directory r0 (there already, or no disk: as it is)
mkdir:
            lda         #O_READ
            ldx         #DM_DIR
            jsr         CREATE
            bcs         :+
            jmp         CLOSE
:
            rts

; RAM disk ctl file r0 started, .A banks, or half that ... (one started already: as it is)
ramdisk:
            sta         banks
            lda         #O_WRITE
            jsr         OPEN
            bcs         @error
            sta         fd
@try:
            ldx         #0                                  ; "start N"
:
            lda         s_start,X
            sta         msg,X
            inx
            cpx         #6
            bne         :-
            lda         banks
            cmp         #10
            bcc         @one
            ldy         #'0' - 1                            ; (Its tens)
:
            iny
            sec
            sbc         #10
            bcs         :-
            adc         #10
            pha
            tya
            sta         msg,X
            inx
            pla
@one:
            ora         #'0'
            sta         msg,X
            inx
            stx         r1
            stz         r1 + 1
            LDR         r0, msg
            lda         fd
            jsr         WRITE
            bcc         @done
            cmp         #E_BUSY                             ; (Started already)
            beq         @done
            cmp         #E_NOMEM
            bne         @failed
            lsr         banks                               ; (Too big: half)
            bne         @try
            lda         #E_NOMEM
@failed:
            pha
            lda         fd
            jsr         CLOSE
            pla
@error:
            jmp         error

@done:
            lda         fd
            jmp         CLOSE

; The windows' chrome: /lib/windows's lines (a card's before the ROM's: /lib's union) written to #c/wctl, a write
; each; an empty line, or one starting #, skipped; one longer than wbuf dropped.  No file: the console's own (the
; ROM's file says the same)
windows:
            LDR         r0, s_windows
            lda         #O_READ
            jsr         OPEN
            bcc         :+
            rts
:
            sta         fd
            LDR         r0, s_wctl
            lda         #O_WRITE
            jsr         OPEN
            bcs         @close
            sta         wfd
            stz         wn
@read:
            clc                                             ; As much more as wbuf has room for
            lda         #<wbuf
            adc         wn
            sta         r0
            lda         #>wbuf
            adc         #0
            sta         r0 + 1
            sec
            lda         #255
            sbc         wn
            sta         r1
            stz         r1 + 1
            lda         fd
            jsr         READ
            bcs         @last
            cmp         #0
            beq         @last
            clc
            adc         wn
            sta         wn
            stz         wi
@line:
            ldy         wi                                  ; A line: to its LF
:
            cpy         wn
            bcs         @part
            lda         wbuf,Y
            cmp         #LF
            beq         :+
            iny
            bra         :-
:
            sty         wj
            jsr         wline
            ldy         wj
            iny
            sty         wi
            bra         @line
@part:
            ldx         #0                                  ; (Its start, not its end: to wbuf's start)
            ldy         wi
            bne         :+
            cpy         wn                                  ; (None of it: the next)
            beq         @read
            lda         wn                                  ; (wbuf full of one line: dropped)
            cmp         #255
            bcc         @read
            stz         wn
            bra         @read
:
            cpy         wn
            bcs         :+
            lda         wbuf,Y
            sta         wbuf,X
            inx
            iny
            bra         :-
:
            stx         wn
            bra         @read
@last:
            stz         wi                                  ; (The last, with no LF)
            lda         wn
            sta         wj
            jsr         wline
            lda         wfd
            jsr         CLOSE
@close:
            lda         fd
            jsr         CLOSE
@done:
            rts

; wbuf's line wi to wj: to #c/wctl, unless it's empty or a comment (#)
wline:
            ldy         wi
            cpy         wj
            bcs         @done
            lda         wbuf,Y
            cmp         #'#'
            beq         @done
            cmp         #CR
            beq         @done
            clc
            tya
            adc         #<wbuf
            sta         r0
            lda         #>wbuf
            adc         #0
            sta         r0 + 1
            sec
            lda         wj
            sbc         wi
            sta         r1
            stz         r1 + 1
            lda         wfd
            jsr         WRITE
            bcc         @done
            jmp         error
@done:
            rts

; The namespace, built in (a bind each: its flags, new, old): what can't be bound is said, and the rest goes on
namespace:
            ldx         #0
@entry:
            lda         ns_table,X
            cmp         #$FF
            beq         @done
            pha
            lda         ns_table + 1,X
            sta         r0
            lda         ns_table + 2,X
            sta         r0 + 1
            lda         ns_table + 3,X
            sta         r1
            lda         ns_table + 4,X
            sta         r1 + 1
            pla
            phx
            jsr         BIND
            bcc         :+
            jsr         error
:
            pla
            clc
            adc         #5
            tax
            bra         @entry

@done:
            rts

; The error .A, said: "init: its text"
error:
            pha
            PRINT       s_error
            LDR         r0, msg
            pla
            jsr         ERRSTR
            PRINT       msg
            PRINT       s_crlf
            rts

; The note handler: init goes on, whatever the note (but a kill, which isn't caught)
notes:
            clc
            rts

.rodata
s_cons:     .byte       "#c/cons", 0
s_up:       .byte       "HydraOS 1.0 for the Hydra-16", CR, LF, "init: up in task ", 0   ; (The kernel's banner is the base's)
s_hello:    .byte       "#m/hello", 0
s_args:     .byte       "from init", 0, 0
s_ended:    .byte       "init: hello ended: code $", 0
s_open:     .byte       " (", 0
s_close:    .byte       ")"
s_crlf:     .byte       CR, LF, 0
s_error:    .byte       "init: ", 0
s_rcl:      .byte       "#m/rc", 0                          ; The shell with no /lib/shell: rc -l
S_RC_LEN    = * - s_rcl
            .byte       "-l", 0, 0
S_RCL_LEN   = * - s_rcl
s_lshell:   .byte       "/lib/shell", 0
s_windows:  .byte       "/lib/windows", 0
s_wctl:     .byte       "#c/wctl", 0
s_wstart:   .byte       "#m/wstart", 0
s_input:    .byte       "#m/input", 0
s_builtin:  .byte       "init: no /rom/lib/namespace: the one built in", CR, LF, 0
s_ctlr:     .byte       "#d/r/ctl", 0
s_ctls:     .byte       "#d/s/ctl", 0
s_sbin:     .byte       "#fs/bin", 0
s_slib:     .byte       "#fs/lib", 0
s_start:    .byte       "start "
ns_table:   .byte       MREPL                               ; bind '#/' /
            .word       s_hroot, s_root
            .byte       MAFTER                              ; bind -a '#c' /dev
            .word       s_hcons, s_dev
            .byte       MAFTER                              ; bind -a '#n' /dev
            .word       s_hnull, s_dev
            .byte       MAFTER                              ; bind -a '#t' /dev
            .word       s_htime, s_dev
            .byte       MREPL                               ; bind '#m' /dev/mod
            .word       s_hmod, s_devmod
            .byte       MREPL                               ; bind '#p' /proc
            .word       s_hproc, s_proc
            .byte       MREPL                               ; bind '#d' /dev/sd
            .word       s_hsd, s_devsd
            .byte       MREPL                               ; bind '#S' /dev/spi
            .word       s_hspi, s_devspi
            .byte       $FF
s_hroot:    .byte       "#/", 0
s_hcons:    .byte       "#c", 0
s_hnull:    .byte       "#n", 0
s_htime:    .byte       "#t", 0
s_hmod:     .byte       "#m", 0
s_hproc:    .byte       "#p", 0
s_hsd:      .byte       "#d", 0
s_hspi:     .byte       "#S", 0
s_root:     .byte       "/", 0
s_dev:      .byte       "/dev", 0
s_devmod:   .byte       "/dev/mod", 0
s_proc:     .byte       "/proc", 0
s_devsd:    .byte       "/dev/sd", 0
s_devspi:   .byte       "/dev/spi", 0

.include "nslib.s"
