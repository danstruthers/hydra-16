; ****************************************************************************
; t_proc - /proc's files for a task's memory and state (phase 5.7: kdev's #p, the kernel's TASKMEM and TASKREAD),
; run as init with t_child: a child that keeps its notes, paused, and its mem (its arguments where they are, a write
; read back; its bank at $8000 as its ram has it; its module's header in the paged ROM; page 0 of the BIOS ROM;
; zeros for the I/O area; nothing past $FFFF; no writes to the ROMs), its ram (a write to a bank read back; the
; end), their lengths, its regs, its env (the variable it was given), its note (by name: its handler's, its code);
; the kernel task's and a driver's refused.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "testlib.inc"

            HYX2_PROGRAM "t_proc", main

.zeropage
fd:         .res        1
child:      .res        1
ptr:        .res        2

.bss
buf:        .res        64
stat:       .res        SR_SIZE
pbuf:       .res        32                                  ; "#p/N/name"

.code

; "#p/N/name" in pbuf: N the child (.Y: 0 the child, else task .Y), name at .A/.X.  Keeps nothing
.macro PPATH_ name, task
            ldy         #task
            lda         #<name
            ldx         #>name
            jsr         ppath
.endmacro

; Open pbuf with mode: fd
.macro POPEN_ mode
            LDR         r0, pbuf
            lda         #mode
            jsr         OPEN
            sta         fd
.endmacro

; fd's offset = a 24-bit offset
.macro SEEK_ ofs
            LDR         r0, ofs & $FFFF
            LDR         r1, ofs >> 16
            ldx         #0
            lda         fd
            jsr         SEEK
.endmacro

; Read count bytes from fd into buf.  OUT: .A/.X, C
.macro READ_  count
            LDR         r0, buf
            LDR         r1, count
            lda         fd
            jsr         READ
.endmacro

; Write len bytes at label to fd.  OUT: .A/.X, C
.macro WRITE_ label, len
            LDR         r0, label
            LDR         r1, len
            lda         fd
            jsr         WRITE
.endmacro

main:
            stz         T_FAILS
            LDR         r0, s_cons
            lda         #O_RDWR
            jsr         OPEN
            lda         #0
            jsr         DUP
            lda         #0
            jsr         DUP
            LDR         r0, s_x                             ; x = hello, for the child's environment
            LDR         r1, s_hello
            LDR         r2, 5
            stz         r3
            stz         r3 + 1
            lda         #$FF
            jsr         ENV_PUT
            EXPECT_OK   "ENV_PUT x"
            LDR         r0, s_child                         ; The child: it keeps its notes, and pauses
            LDR         r1, s_n
            lda         #0
            jsr         SPAWN
            sta         child
            EXPECT_OK   "SPAWN t_child n"
            lda         #3
            ldx         #0
            jsr         SLEEP

; ---- mem
            PPATH_      s_mem, 0
            POPEN_      O_RDWR
            EXPECT_OK   "OPEN #p/N/mem"
            SEEK_       $0350                               ; (TA_ARGS)
            READ_       3
            EXPECT_A    3, "mem: 3 bytes at $0350"
            lda         buf
            cmp         #'n'
            bne         :+
            lda         buf + 1
            ora         buf + 2
:
            EXPECT_A    0, "mem: its arguments at $0350 (n, and the empty one)"
            SEEK_       $7F00
            WRITE_      s_xyz, 3
            EXPECT_A    3, "mem: 3 bytes written at $7F00"
            SEEK_       $7F00
            READ_       3
            lda         buf
            eor         #'x'
            sta         ptr
            lda         buf + 2
            eor         #'z'
            ora         ptr
            EXPECT_A    0, "mem: read back"
            SEEK_       $A000                               ; Its module's header, in its paged ROM bank
            READ_       4
            EXPECT_A    4, "mem: 4 bytes at $A000"
            lda         buf
            eor         #'H'
            sta         ptr
            lda         buf + 3
            eor         #'2'
            ora         ptr
            EXPECT_A    0, "mem: its module's header at $A000 (HYX2)"
            SEEK_       $A000
            WRITE_      s_xyz, 1
            EXPECT_ERR  E_PERM, "mem: no write to the paged ROM"
            SEEK_       $E000                               ; The BIOS ROM's page 0: as ours
            READ_       2
            lda         buf
            eor         $E000
            sta         ptr
            lda         buf + 1
            eor         $E001
            ora         ptr
            EXPECT_A    0, "mem: the BIOS ROM's page 0 at $E000"
            SEEK_       $FFFC                               ; The I/O area: zeros
            lda         #$5A
            sta         buf
            sta         buf + 3
            READ_       16
            EXPECT_A    4, "mem: 4 bytes to its end, at $FFFC"
            lda         buf
            ora         buf + 3
            EXPECT_A    0, "mem: the I/O area as zeros"
            SEEK_       $10000
            READ_       16
            EXPECT_A    0, "mem: nothing past $FFFF"
            lda         fd
            jsr         CLOSE

; ---- ram: its banks, and its bank at $8000 in mem
            PPATH_      s_ram, 0
            POPEN_      O_RDWR
            EXPECT_OK   "OPEN #p/N/ram"
            SEEK_       $2010                               ; (Bank 1, $10)
            WRITE_      s_ram4, 4
            EXPECT_A    4, "ram: 4 bytes written to bank 1"
            SEEK_       $2010
            READ_       4
            lda         buf
            eor         #'r'
            sta         ptr
            lda         buf + 3
            eor         #'!'
            ora         ptr
            EXPECT_A    0, "ram: bank 1's read back"
            SEEK_       $0020                               ; (Bank 0, $20: its bank as it starts)
            WRITE_      s_xyz, 3
            EXPECT_A    3, "ram: 3 bytes written to bank 0"
            SEEK_       $40000                              ; (Two modules: 32 banks)
            READ_       16
            EXPECT_A    0, "ram: nothing past the last module's banks"
            lda         fd
            jsr         CLOSE
            PPATH_      s_mem, 0                            ; Its bank 0 at $8000, in mem
            POPEN_      O_READ
            SEEK_       $8020
            READ_       3
            lda         buf
            eor         #'x'
            sta         ptr
            lda         buf + 2
            eor         #'z'
            ora         ptr
            EXPECT_A    0, "mem: its bank at $8000, as its ram has it"
            lda         fd
            jsr         CLOSE
            PPATH_      s_mem, 0                            ; Their lengths
            LDR         r0, pbuf
            LDR         r1, stat
            jsr         STAT
            lda         stat + SR_LENGTH
            ora         stat + SR_LENGTH + 1
            ora         stat + SR_LENGTH + 3
            eor         stat + SR_LENGTH + 2
            EXPECT_A    1, "mem: 64K long"
            PPATH_      s_ram, 0
            LDR         r0, pbuf
            LDR         r1, stat
            jsr         STAT
            lda         stat + SR_LENGTH + 2
            EXPECT_A    4, "ram: 256K long (two modules)"

; ---- regs, env
            PPATH_      s_regs, 0
            POPEN_      O_READ
            READ_       63
            lda         buf
            eor         #'P'
            sta         ptr
            lda         buf + 2
            eor         #'='
            ora         ptr
            EXPECT_A    0, "regs: PC= first"
            lda         fd
            jsr         CLOSE
            PPATH_      s_env, 0
            POPEN_      O_READ
            READ_       63
            EXPECT_A    8, "env: 8 bytes"
            ldx         #7
:
            lda         buf,X
            cmp         s_xline,X
            bne         :+
            dex
            bpl         :-
            lda         #0
:
            EXPECT_A    0, "env: x=hello"
            lda         fd
            jsr         CLOSE

; ---- refused: the kernel task's, a driver's (cons, task F)
            PPATH_      s_mem, $FF
            POPEN_      O_READ
            EXPECT_ERR  E_PERM, "#p/0/mem: refused"
            PPATH_      s_regs, $FE
            POPEN_      O_READ
            READ_       63
            EXPECT_ERR  E_PERM, "#p/15/regs (a driver's): refused"
            lda         fd
            jsr         CLOSE

; ---- note: by name; its handler keeps it, and it ends with it
            PPATH_      s_note, 0
            POPEN_      O_WRITE
            EXPECT_OK   "OPEN #p/N/note"
            WRITE_      s_alarm, 6
            EXPECT_A    6, "note: alarm written"
            lda         fd
            jsr         CLOSE
            stz         r0
            stz         r0 + 1
            lda         child
            jsr         WAIT
            txa
            EXPECT_A    NOTE_ALARM, "the child ends with the note it kept (alarm)"
            DONE        "t_proc"

; pbuf = "#p/N/" and the name at .A/.X: N the child (.Y = 0), the kernel task ($FF) or task 15 ($FE)
ppath:
            sta         ptr
            stx         ptr + 1
            ldx         #0
:
            lda         s_pre,X
            sta         pbuf,X
            inx
            cpx         #3
            bne         :-
            lda         child
            cpy         #$FF
            bne         :+
            lda         #0
:
            cpy         #$FE
            bne         :+
            lda         #15
:
            cmp         #10
            bcc         :+
            pha
            lda         #'1'
            sta         pbuf,X
            inx
            pla
            sbc         #10
:
            ora         #'0'
            sta         pbuf,X
            inx
            lda         #'/'
            sta         pbuf,X
            inx
            ldy         #0
:
            lda         (ptr),Y
            sta         pbuf,X
            inx
            iny
            cmp         #0
            bne         :-
            rts

.rodata
s_cons:     .byte       "#c/cons", 0
s_child:    .byte       "#m/t_child", 0
s_n:        .byte       "n", 0, 0
s_pre:      .byte       "#p/"
s_mem:      .byte       "mem", 0
s_ram:      .byte       "ram", 0
s_regs:     .byte       "regs", 0
s_env:      .byte       "env", 0
s_note:     .byte       "note", 0
s_x:        .byte       "x", 0
s_hello:    .byte       "hello"
s_xline:    .byte       "x=hello", LF
s_xyz:      .byte       "xyz"
s_ram4:     .byte       "ram!"
s_alarm:    .byte       "alarm", LF
