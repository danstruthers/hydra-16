; ****************************************************************************
; t_step - the debugger's steps and breakpoints (phase 9: the kernel's TASKSTEP, /proc/N/ctl's step, next, break and
; nobreak, SPAWN_STOPPED), run as init with a card: t_steppee (a RAM program, tests/ram) started stopped at its entry
; point, then stepped through each kind of instruction (run out of line: an ordinary one, branches taken and not,
; BBR and BBS, JMP (abs) and JMP (abs,X), a JSR to the jump table; done on its frame: JMP, JSR, RTS, RTI), its PC
; and registers read after each (TASKREAD's frame); a JSR stepped over (next); a breakpoint (a BRK written into
; it through /proc/N/mem, break: stopped on it; a step there refused); then run on to its end.  t_child's own BRK,
; with break (a module in place: stopped on it); a step of a task not stopped, and of the kernel task, refused.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "testlib.inc"

            HYX2_PROGRAM "t_step", main

; t_steppee's instructions, from its entry point (its comments')
OFS_STA         = 2
OFS_LDX0        = 4
OFS_CLC         = 6
OFS_BCC         = 7
OFS_SEC         = 10
OFS_BCC2        = 11
OFS_BBR         = 13
OFS_BBS         = 17
OFS_JMP         = 20
OFS_JMPI        = 23
OFS_LDX2        = 26
OFS_JMPIX       = 28
OFS_JSR         = 31
OFS_JSR2        = 34
OFS_LDA_HI      = 37
OFS_PHA         = 39
OFS_LDA_LO      = 40
OFS_PHA2        = 42
OFS_PHP         = 43
OFS_RTI         = 44
OFS_GETPID      = 45
OFS_NOP         = 48
OFS_BRK         = 49
OFS_END         = 50
OFS_SUB         = 59
OFS_RTS         = 60

.zeropage
child:      .res        1
fd:         .res        1                                   ; Its ctl
memfd:      .res        1                                   ; Its mem
entry:      .res        2
s0:         .res        1                                   ; Its S before the JSR
ptr:        .res        2
tries:      .res        2

.bss
frame:      .res        TF_SIZE
info:       .res        TI_SIZE
pbuf:       .res        32                                  ; "#p/N/name"
byte:       .res        1

.code

; A step: cmd (a label, len bytes) to its ctl, then (once it's stopped again) its PC = its entry point + ofs.
; OUT: C = 0; or C = 1, .A = the write's error, $EE (it didn't stop), or its PC's offset
.macro STEP_    cmd, len, ofs, text
            LDR         r0, cmd
            LDR         r1, len
            lda         #ofs
            jsr         step
            EXPECT_OK   text
.endmacro

; cmd (a label, len bytes) to ctl fd f.  OUT: C, .A
.macro CTL_     f, cmd, len
            LDR         r0, cmd
            LDR         r1, len
            lda         f
            jsr         WRITE
.endmacro

main:
            stz         T_FAILS
            LDR         r0, s_cons                          ; fds 0-2: the console
            lda         #O_RDWR
            jsr         OPEN
            lda         #0
            jsr         DUP
            lda         #0
            jsr         DUP
            stz         r0                                  ; The card at /sd
            stz         r0 + 1
            LDR         r1, s_sd
            ldx         #'f'
            lda         #MREPL
            jsr         MOUNT
            EXPECT_OK   "the card at /sd"

; ---- t_steppee, stopped at its entry point
            LDR         r0, s_steppee
            stz         r1
            stz         r1 + 1
            lda         #SPAWN_STOPPED
            jsr         SPAWN
            sta         child
            EXPECT_OK   "SPAWN /sd/0/bin/t_steppee, SPAWN_STOPPED"
            jsr         stopped
            EXPECT_OK   "it stops (TF_STOPPED)"
            jsr         rdframe
            lda         frame + TF_PC
            sta         entry
            lda         frame + TF_PC + 1
            sta         entry + 1
            EXPECT_A    $08, "its PC its entry point, in its RAM ($08xx)"
            lda         frame + TF_STATE
            EXPECT_A    1, "ready (not in a call)"
            lda         frame + TF_W
            EXPECT_A    0, "W 0"
            LDR         ptr, s_ctl
            jsr         ppath
            LDR         r0, pbuf
            lda         #O_WRITE
            jsr         OPEN
            sta         fd
            EXPECT_OK   "its ctl opened"

; ---- Each kind, a step at a time
            STEP_       s_step, 5, OFS_STA, "step: LDA #, run out of line"
            lda         frame + TF_A
            EXPECT_A    $FE, "its A: $FE"
            STEP_       s_step, 5, OFS_LDX0, "step: STA zp"
            STEP_       s_step, 5, OFS_CLC, "step: LDX #"
            lda         frame + TF_X
            EXPECT_A    0, "its X: 0"
            STEP_       s_step, 5, OFS_BCC, "step: CLC"
            lda         frame + TF_P
            and         #$01
            EXPECT_A    0, "its C: 0"
            STEP_       s_step, 5, OFS_SEC, "step: BCC, taken"
            STEP_       s_step, 5, OFS_BCC2, "step: SEC"
            STEP_       s_step, 5, OFS_BBR, "step: BCC, not taken"
            STEP_       s_step, 5, OFS_BBS, "step: BBR0, taken"
            STEP_       s_step, 5, OFS_JMP, "step: BBS0, not taken"
            STEP_       s_step, 5, OFS_JMPI, "step: JMP abs (on its frame)"
            STEP_       s_step, 5, OFS_LDX2, "step: JMP (abs)"
            STEP_       s_step, 5, OFS_JMPIX, "step: LDX #2"
            lda         frame + TF_S
            sta         s0
            STEP_       s_step, 5, OFS_JSR, "step: JMP (abs,X)"
            lda         frame + TF_S
            sec
            sbc         s0
            EXPECT_A    0, "its S kept by the out-of-line code (PHP, PHA, PLA, PLP)"
            STEP_       s_step, 5, OFS_SUB, "step: JSR, into its subroutine (on its frame)"
            lda         s0
            sec
            sbc         frame + TF_S
            EXPECT_A    2, "its S 2 down"
            STEP_       s_step, 5, OFS_RTS, "step: INX"
            STEP_       s_step, 5, OFS_JSR2, "step: RTS (on its frame)"
            lda         frame + TF_S
            sec
            sbc         s0
            EXPECT_A    0, "its S back"
            STEP_       s_next, 5, OFS_LDA_HI, "next: JSR, its subroutine run whole"
            lda         frame + TF_X
            EXPECT_A    4, "its X: 4 (its subroutine run twice)"
            lda         frame + TF_S
            sec
            sbc         s0
            EXPECT_A    0, "its S back"
            STEP_       s_step, 5, OFS_PHA, "step: LDA #"
            STEP_       s_step, 5, OFS_LDA_LO, "step: PHA"
            STEP_       s_step, 5, OFS_PHA2, "step: LDA #"
            STEP_       s_step, 5, OFS_PHP, "step: PHA"
            STEP_       s_step, 5, OFS_RTI, "step: PHP"
            STEP_       s_step, 5, OFS_GETPID, "step: RTI (on its frame)"
            lda         frame + TF_S
            sec
            sbc         s0
            EXPECT_A    0, "its S back"
            STEP_       s_step, 5, OFS_NOP, "step: JSR GETPID (the jump table's: run out of line, whole)"
            lda         frame + TF_A
            sec
            sbc         child
            EXPECT_A    0, "its A: its task"

; ---- A breakpoint: a BRK written at +49, break, start: stopped on it
            CTL_        fd, s_break, 6
            EXPECT_OK   "break"
            LDR         ptr, s_mem
            jsr         ppath
            LDR         r0, pbuf
            lda         #O_RDWR
            jsr         OPEN
            sta         memfd
            EXPECT_OK   "its mem opened"
            lda         #0                                  ; (BRK)
            jsr         poke
            EXPECT_OK   "a BRK written at +49"
            CTL_        fd, s_start, 6
            EXPECT_OK   "start"
            jsr         stopped
            EXPECT_OK   "it stops"
            jsr         rdframe
            lda         #OFS_BRK
            jsr         pcis
            EXPECT_OK   "at the breakpoint: its PC on the BRK"
            CTL_        fd, s_step, 5
            EXPECT_ERR  E_INVAL, "a step at a BRK: E_INVAL"
            lda         #$EA                                ; (NOP, back)
            jsr         poke
            EXPECT_OK   "its byte back"
            CTL_        fd, s_nobreak, 8
            EXPECT_OK   "nobreak"
            STEP_       s_step, 5, OFS_END, "step: the NOP"
            CTL_        fd, s_start, 6
            EXPECT_OK   "start"
            jsr         reap
            EXPECT_A    0, "it runs on to its end: code 0"
            lda         memfd
            jsr         CLOSE
            lda         fd
            jsr         CLOSE

; ---- t_child's own BRK (a module in place), with break: stopped on it
            LDR         r0, s_child
            LDR         r1, s_b
            lda         #SPAWN_STOPPED
            jsr         SPAWN
            sta         child
            EXPECT_OK   "SPAWN #m/t_child b, SPAWN_STOPPED"
            jsr         stopped
            EXPECT_OK   "it stops"
            jsr         rdframe
            lda         frame + TF_PC + 1
            and         #$E0
            EXPECT_A    $A0, "its PC its entry point, in the paged ROM"
            LDR         ptr, s_ctl
            jsr         ppath
            LDR         r0, pbuf
            lda         #O_WRITE
            jsr         OPEN
            sta         fd
            CTL_        fd, s_break, 6
            EXPECT_OK   "break"
            CTL_        fd, s_start, 6
            EXPECT_OK   "start"
            jsr         stopped
            EXPECT_OK   "it stops"
            jsr         rdframe
            jsr         peek                                ; (Its byte at its PC: the BRK)
            lda         byte
            EXPECT_A    0, "at its BRK"
            CTL_        fd, s_kill, 5
            EXPECT_OK   "kill"
            jsr         reap
            EXPECT_A    137, "killed: 137"
            lda         fd
            jsr         CLOSE

; ---- Refused: a step of a task not stopped, and of the kernel task
            LDR         r0, s_child
            LDR         r1, s_p
            lda         #0
            jsr         SPAWN
            sta         child
            EXPECT_OK   "SPAWN #m/t_child p (it pauses)"
            ldy         #20
:
            phy
            jsr         YIELD
            ply
            dey
            bne         :-
            LDR         ptr, s_ctl
            jsr         ppath
            LDR         r0, pbuf
            lda         #O_WRITE
            jsr         OPEN
            sta         fd
            CTL_        fd, s_step, 5
            EXPECT_ERR  E_BUSY, "a step of a task not stopped: E_BUSY"
            CTL_        fd, s_kill, 5
            jsr         reap
            lda         fd
            jsr         CLOSE
            stz         child                               ; (The kernel task's ctl)
            LDR         ptr, s_ctl
            jsr         ppath
            LDR         r0, pbuf
            lda         #O_WRITE
            jsr         OPEN
            sta         fd
            CTL_        fd, s_step, 5
            EXPECT_ERR  E_PERM, "a step of the kernel task: E_PERM"
            DONE        "t_step"

; Write cmd (r0, r1 bytes) to its ctl, then wait for it to stop: its PC = its entry point + .A.  OUT: C = 0; or
; C = 1, .A = the write's error, $EE (it didn't stop) or its PC's offset
step:
            pha
            lda         fd
            jsr         WRITE
            bcc         :+
            ply
            rts
:
            jsr         stopped
            bcc         :+
            pla
            lda         #$EE
            rts
:
            jsr         rdframe
            pla
            ; (on into pcis)

; Its PC (frame) = its entry point + .A?  OUT: C = 0; or C = 1, .A = its PC's offset
pcis:
            clc
            adc         entry
            tax
            lda         entry + 1
            adc         #0
            cmp         frame + TF_PC + 1
            bne         @no
            cpx         frame + TF_PC
            bne         @no
            clc
            rts
@no:
            lda         frame + TF_PC
            sec
            sbc         entry
            sec
            rts

; Wait for the child to stop (TF_STOPPED), a YIELD at a time.  OUT: C = 0; or C = 1 (not after 10000)
stopped:
            LDR         tries, 10000
@look:
            LDR         r0, info
            lda         child
            jsr         TASKINFO
            lda         info + TI_FLAGS
            bmi         @yes
            jsr         YIELD
            lda         tries
            bne         :+
            dec         tries + 1
:
            dec         tries
            lda         tries
            ora         tries + 1
            bne         @look
            sec
            rts
@yes:
            clc
            rts

; Its frame (TASKREAD)
rdframe:
            LDR         r0, frame
            lda         child
            ldx         #TR_FRAME
            jmp         TASKREAD

; .A at its entry point + OFS_BRK, through its mem.  OUT: C, .A
poke:
            sta         byte
            jsr         brkat
            LDR         r0, byte
            LDR         r1, 1
            lda         memfd
            jmp         WRITE

; byte = its byte at its PC (frame), through its mem (opened read only here, and closed)
peek:
            LDR         ptr, s_mem
            jsr         ppath
            LDR         r0, pbuf
            lda         #O_READ
            jsr         OPEN
            sta         memfd
            MOVR        r0, frame + TF_PC
            stz         r1
            stz         r1 + 1
            ldx         #0
            lda         memfd
            jsr         SEEK
            LDR         r0, byte
            LDR         r1, 1
            lda         memfd
            jsr         READ
            lda         memfd
            jmp         CLOSE

; memfd's offset: its entry point + OFS_BRK
brkat:
            lda         entry
            clc
            adc         #OFS_BRK
            sta         r0
            lda         entry + 1
            adc         #0
            sta         r0 + 1
            stz         r1
            stz         r1 + 1
            ldx         #0
            lda         memfd
            jmp         SEEK

; Wait for the child: .A = its exit code
reap:
            stz         r0
            stz         r0 + 1
            lda         child
            jsr         WAIT
            txa
            rts

; "#p/N/name" in pbuf: N the child (0-15), name at ptr
ppath:
            ldx         #0
:
            lda         s_pre,X
            sta         pbuf,X
            inx
            cpx         #3
            bne         :-
            lda         child
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
s_sd:       .byte       "/sd", 0
s_steppee:  .byte       "/sd/0/bin/t_steppee", 0
s_child:    .byte       "#m/t_child", 0
s_b:        .byte       "b", 0, 0
s_p:        .byte       "p", 0, 0
s_pre:      .byte       "#p/"
s_ctl:      .byte       "ctl", 0
s_mem:      .byte       "mem", 0
s_step:     .byte       "step", LF
s_next:     .byte       "next", LF
s_break:    .byte       "break", LF
s_nobreak:  .byte       "nobreak", LF
s_start:    .byte       "start", LF
s_kill:     .byte       "kill", LF
