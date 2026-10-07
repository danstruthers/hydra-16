; ****************************************************************************
; debug.s - the debugger's half in the kernel: single steps and breakpoints (TASKSTEP), for /proc/N/ctl (kdev) and
; the debugger, db (docs/reimplementation-from-scratch.md, phase 9).
;
; A step runs a stopped task's next instruction out of line: a copy of it in the task's OS zero page (TK_STEPBUF),
; a BRK after it, its frame's PC there, and the task started.  The BRK comes through IRQ_STRAY to K_TRAP (the
; task's TF_TRAP), which puts its PC where the instruction went on to (TK_STEPNEXT) and stops it again.  A branch
; runs with 2 for its offset and two BRKs after it: the second (TK_STEPHIT: the low byte of the PC it pushes) is
; taken, and goes on to the branch's target (TK_STEPTGT).  JMP (abs) and JMP (abs,X) run as code that reads their
; target, in the task's own view of memory, into TK_STEPTGT.  JMP abs, JSR, RTS and RTI are done here, on its frame
; (moved down or up its stack page by what they push or pull), and the task stays stopped; but a JSR to the BIOS
; ROM ($E000 and up: the kernel's jump table), and any JSR with TS_NEXT, runs out of line, its subroutine whole: it
; returns to the BRK after it.  (A subroutine that reads bytes after its JSR, its arguments, can't be stepped over.)
;   A step is out while TK_STEPHIT isn't 0: another waits for it to end (E_BUSY).  A stop meanwhile leaves it out,
; and a start lets it end.  With TF_BREAKS (TS_BREAKS: the debugger's breakpoints, BRKs it writes into the
; program) any other BRK stops the task too, its PC back on the BRK; else a BRK is the program's own (the note
; sys: brk).  A task in the kernel can't be stepped (in a call: not ready, or W not 0, or its PC at $E000 or
; above).  SPAWN_STOPPED starts a program stopped at its entry point (task.s: TF_HOLD).

.include "kdefs.inc"

.assert     TF_TRAP = $40, error, "IRQ_STRAY tests TF_TRAP with bit (V)"
.assert     TK_STEPTGT = TK_STEPNEXT + 2 .and TK_STEPHIT = TK_STEPTGT + 2, error, "s_go writes them in a row"
.assert     TK_STEPBUF + S_JMPI_LEN + 2 <= TK_STEPNEXT, error, "TK_STEPBUF's code runs past it"

; TASKSTEP's scratch, in its KCALL (the kernel task's)
S_TASK          = K0_STEP + 0   ; The task
S_MODE          = K0_STEP + 1   ; TS_*
S_SP            = K0_STEP + 2   ; Its TK_SP (JSR, RTS and RTI move it)
S_STATE         = K0_STEP + 3   ; Its state; then a count
S_LEN           = K0_STEP + 4   ; The instruction's length
S_HIT           = K0_STEP + 5   ; TK_STEPHIT's value ($FF: no BRK means taken)
S_CODE          = K0_STEP + 6   ; The code's length so far
S_I             = S_STATE
S_NEXT          = K0_PTR        ; (2) Where the instruction goes on to: its PC + its length
S_TGT           = K0_PTR2       ; (2) And taken: a branch's target
S_FRAME         = K_XBUF        ; Its frame (S_FRAME + FR_U to FR_PCH) ...
S_OP            = K_XBUF + 9    ; (3)   the instruction ...
S_BUF           = K_XBUF + 12   ; (16)  and the code for TK_STEPBUF

; The instructions' kinds (s_ops: the high nibble; the low its length)
SK_RUN          = 0             ; Run out of line, as it is
SK_BRANCH       = 1             ; Bxx and BRA: rel
SK_BBX          = 2             ; BBR and BBS: zp, rel
SK_JMP          = 3
SK_JMPI         = 4             ; JMP (abs)
SK_JMPIX        = 5             ; JMP (abs,X)
SK_JSR          = 6
SK_RTS          = 7
SK_RTI          = 8
SK_NO           = 9             ; BRK, STP: not stepped

.segment "KCODE"

; A BRK in a task with TF_TRAP, from IRQ_STRAY: IRQs off, in the task; .X = TK_SP, its frame's Y, W, X, A, P and PC
; on its stack.  A step's BRK (in TK_STEPBUF: its PC's high byte 0): its PC where the instruction went on to; else,
; with TF_BREAKS, a breakpoint: its PC back on the BRK; else it's the program's own BRK.  Stopped, its P's B bit
; cleared (P as the program had it), and switched out.  In a BRK's IRQs-off stretch: kept short
K_TRAP:
            lda         TK_STEPHIT                          ; A step's?
            beq         @other
            ldy         $0107,X
            bne         @other
            cmp         $0106,X
            beq         @taken
            lda         TK_STEPNEXT
            ldy         TK_STEPNEXT + 1
            bra         @pc

@taken:
            lda         TK_STEPTGT
            ldy         TK_STEPTGT + 1
@pc:
            sta         $0106,X
            tya
            sta         $0107,X
            stz         TK_STEPHIT                          ; (The step's done)
            lda         #TF_BREAKS                          ; TF_TRAP kept for breakpoints only
            and         TK_FLAGS
            bne         @stop
            lda         #TF_TRAP
            trb         TK_FLAGS
            bra         @stop

@other:
            lda         #TF_BREAKS
            and         TK_FLAGS
            beq         @note
            lda         $0106,X                             ; A breakpoint: its PC back on the BRK
            sec
            sbc         #2
            sta         $0106,X
            bcs         @stop
            dec         $0107,X
@stop:
            lda         $0105,X
            and         #$FF ^ $10
            sta         $0105,X
            lda         #TF_STOPPED
            tsb         TK_FLAGS
            jmp         IRQ_SWITCH

@note:
            jmp         IRQ_BRK_NOTE

.segment "KCODE_P4"

; TASKSTEP: step task .A (stopped) one instruction (.X = TS_STEP), or one with a JSR's subroutine run whole
; (TS_NEXT), and stop it again; or from now its BRKs stop it (TS_BREAKS: the debugger's breakpoints), or they're
; its own again (TS_NOBREAKS).  A driver's call (kdev's, for /proc/N/ctl); not of the kernel task or a driver.
; OUT: C = 0; or C = 1, .A = E_PERM, E_SRCH (free, or not started), E_INVAL (another .X; a BRK or STP to step),
; E_BUSY (to step: not stopped, a step still out, or in the kernel)
K_TASKSTEP:
            tay
            lda         TK_FLAGS                            ; (A driver's call only)
            and         #TF_DRIVER
            bne         :+
            FAIL        E_PERM

:
            cpx         #TS_NOBREAKS + 1
            bcc         :+
            FAIL        E_INVAL

:
            tya
            KCALL_FAR   K_TASKSTEP_K
            rts

; TASKSTEP's, in the kernel task (KCALL): the task can't run till it's done.  .A = the task, .X = TS_*
K_TASKSTEP_K:
            sta         S_TASK
            stx         S_MODE
            tax
            beq         @perm                               ; (The kernel task)
            cpx         #TASKS
            bcs         @srch
            ldy         #TK_STATE
            jsr         s_zget
            beq         @srch                               ; (Free)
            cmp         #ST_NEW
            beq         @srch
            sta         S_STATE
            ldy         #TK_FLAGS
            jsr         s_zget
            and         #TF_DRIVER
            bne         @perm
            lda         S_MODE
            cmp         #TS_BREAKS
            bcc         @step
            bne         @nobreaks
            ldy         #TK_FLAGS                           ; TS_BREAKS: TF_BREAKS and TF_TRAP
            jsr         s_zget
            ora         #TF_BREAKS | TF_TRAP
            jsr         s_zput
            clc
            rts

@nobreaks:                                                  ; TS_NOBREAKS: neither (TF_TRAP kept while a step's out)
            ldy         #TK_STEPHIT
            jsr         s_zget
            beq         :+
            lda         #TF_TRAP
:
            ora         #$FF ^ (TF_BREAKS | TF_TRAP)
            sta         S_I
            ldy         #TK_FLAGS
            jsr         s_zget
            and         S_I
            jsr         s_zput
            clc
            rts

@perm:
            FAIL        E_PERM

@srch:
            FAIL        E_SRCH

@busy:
            FAIL        E_BUSY

@step:
            lda         S_STATE                             ; Ready (not in a call) ...
            cmp         #ST_READY
            bne         @busy
            ldy         #TK_STEPHIT                         ;   with no step out ...
            jsr         s_zget
            bne         @busy
            ldy         #TK_FLAGS                           ;   and stopped
            jsr         s_zget
            bpl         @busy
            ldy         #TK_SP                              ; Its frame
            jsr         s_zget
            sta         S_SP
            ldy         #FRAME_SIZE
            sty         S_I
:
            ldy         S_I
            jsr         s_sget
            ldy         S_I
            sta         S_FRAME,Y
            dec         S_I
            bne         :-
            lda         S_FRAME + FR_W                      ; In its own code?  (W 0, its PC below the BIOS ROM)
            bne         @busy
            lda         S_FRAME + FR_PCH
            cmp         #>BIOS_BASE
            bcs         @busy
            lda         S_FRAME + FR_PCL                    ; Its instruction: 3 bytes from its PC, as it sees
            ldy         #TM_PTR                             ;   them (TM_PTR: its TASKMEM pointer, not in use: it's
            jsr         s_zput                              ;   no driver)
            lda         S_FRAME + FR_PCH
            iny
            jsr         s_zput
            ldx         S_TASK
            ldy         #2
:
            php
            sei
            stx         T_REGISTER                          ; ---- The task
            lda         (TM_PTR),Y
            stz         T_REGISTER                          ; ---- Back
            plp
            sta         S_OP,Y
            dey
            bpl         :-
            stz         S_CODE
            ldy         S_OP                                ; Its length, and where it goes on to
            lda         s_ops,Y
            and         #$0F
            sta         S_LEN
            clc
            adc         S_FRAME + FR_PCL
            sta         S_NEXT
            lda         S_FRAME + FR_PCH
            adc         #0
            sta         S_NEXT + 1
            lda         s_ops,Y                             ; Its kind
            lsr
            lsr
            lsr
            and         #$1E
            tax
            jmp         (s_kinds,X)

s_kinds:    .word       s_run, s_branch, s_bbx, s_jmp, s_jmpi, s_jmpi, s_jsr, s_rts, s_rti, s_no

; SK_JSR: done here (its return address pushed, the frame 2 down); but out of line to the BIOS ROM, or with TS_NEXT
s_jsr:
            lda         S_OP + 2
            cmp         #>BIOS_BASE
            bcs         s_run
            lda         S_MODE
            cmp         #TS_NEXT
            beq         s_run
            lda         S_NEXT                              ; (Its last byte's address: S_NEXT - 1)
            sec
            sbc         #1
            sta         S_TGT
            lda         S_NEXT + 1
            sbc         #0
            ldy         #FRAME_SIZE                         ; Pushed where the program's S is: high, then low
            jsr         s_sput
            lda         S_TGT
            ldy         #FRAME_SIZE - 1
            jsr         s_sput
            dec         S_SP
            dec         S_SP
            lda         S_OP + 1
            ldy         S_OP + 2
            jmp         s_jump

; SK_RUN: its bytes, then a BRK
s_run:
            ldy         #0
:
            lda         S_OP,Y
            jsr         s_put
            iny
            cpy         S_LEN
            bne         :-
            lda         #$FF                                ; (None taken)
            sta         S_HIT
s_brk:                                                      ; A BRK (and its signature byte), then it runs
            lda         #0
            jsr         s_put
            jsr         s_put
            jmp         s_go

; SK_BRANCH: Bxx 2, then two BRKs (the second, taken, at its target: 2 on)
s_branch:
            lda         S_OP
            jsr         s_put
            lda         S_OP + 1
            jsr         s_target
            bra         s_two

; SK_BBX: BBR or BBS zp, 2, then two BRKs
s_bbx:
            lda         S_OP
            jsr         s_put
            lda         S_OP + 1
            jsr         s_put
            lda         S_OP + 2
            jsr         s_target
s_two:
            lda         #2
            jsr         s_put
            lda         S_CODE                              ; The second BRK's PC, as it pushes it (its address + 2)
            clc
            adc         #<TK_STEPBUF + 4
            sta         S_HIT
            lda         #0
            jsr         s_put
            jsr         s_put
            bra         s_brk

; SK_JMP: its PC, here
s_jmp:
            lda         S_OP + 1
            ldy         S_OP + 2
s_jump:
            sta         S_FRAME + FR_PCL
            sty         S_FRAME + FR_PCH
            jmp         s_done

; SK_JMPI, SK_JMPIX: code that reads its target (s_jmpi's, its operand put in), then a BRK, taken
s_jmpi:
            ldy         #0
:
            lda         s_jmpicode,Y
            jsr         s_put
            iny
            cpy         #S_JMPI_LEN
            bne         :-
            lda         S_OP                                ; (LDA abs,X for JMP (abs,X))
            cmp         #$6C
            beq         :+
            lda         #$BD
            sta         S_BUF + 2
            sta         S_BUF + 7
:
            lda         S_OP + 1
            sta         S_BUF + 3
            clc
            adc         #1
            sta         S_BUF + 8
            lda         S_OP + 2
            sta         S_BUF + 4
            adc         #0
            sta         S_BUF + 9
            lda         #<TK_STEPBUF + S_JMPI_LEN + 2
            sta         S_HIT
            jmp         s_brk

; SK_RTS: its return address pulled, + 1 (the frame 2 up)
s_rts:
            ldy         #FRAME_SIZE + 1
            jsr         s_sget
            sta         S_TGT
            ldy         #FRAME_SIZE + 2
            jsr         s_sget
            sta         S_TGT + 1
            inc         S_SP
            inc         S_SP
            lda         S_TGT
            clc
            adc         #1
            sta         S_FRAME + FR_PCL
            lda         S_TGT + 1
            adc         #0
            sta         S_FRAME + FR_PCH
            bra         s_done

; SK_RTI: P and its PC pulled (the frame 3 up)
s_rti:
            ldy         #FRAME_SIZE + 1
            jsr         s_sget
            and         #$FF ^ $10                          ; (B: not a flag)
            sta         S_FRAME + FR_P
            ldy         #FRAME_SIZE + 2
            jsr         s_sget
            sta         S_FRAME + FR_PCL
            ldy         #FRAME_SIZE + 3
            jsr         s_sget
            sta         S_FRAME + FR_PCH
            lda         S_SP
            clc
            adc         #3
            sta         S_SP
            bra         s_done

; SK_NO: BRK, STP
s_no:
            FAIL        E_INVAL

; Done here: its frame, where it is now (S_SP), and its TK_SP.  It stays stopped
s_done:
            ldy         #FRAME_SIZE
            sty         S_I
:
            ldy         S_I
            lda         S_FRAME,Y
            jsr         s_sput
            dec         S_I
            bne         :-
            lda         S_SP
            ldy         #TK_SP
            jsr         s_zput
            clc
            rts

; Run out of line: the code into TK_STEPBUF; TK_STEPNEXT, TK_STEPTGT and TK_STEPHIT; its PC there; and it's started,
; with TF_TRAP
s_go:
            ldy         #0
:
            lda         S_BUF,Y
            ldx         S_TASK
            php
            sei
            stx         T_REGISTER                          ; ---- The task
            sta         TK_STEPBUF,Y
            stz         T_REGISTER                          ; ---- Back
            plp
            iny
            cpy         S_CODE
            bne         :-
            ldy         #TK_STEPNEXT
            lda         S_NEXT
            jsr         s_zput
            iny
            lda         S_NEXT + 1
            jsr         s_zput
            iny
            lda         S_TGT
            jsr         s_zput
            iny
            lda         S_TGT + 1
            jsr         s_zput
            iny
            lda         S_HIT
            jsr         s_zput
            lda         #<TK_STEPBUF
            ldy         #FR_PCL
            jsr         s_sput
            lda         #>TK_STEPBUF
            ldy         #FR_PCH
            jsr         s_sput
            ldy         #TK_FLAGS
            jsr         s_zget
            and         #$FF ^ TF_STOPPED
            ora         #TF_TRAP
            jsr         s_zput
            clc
            rts

; S_TGT = S_NEXT + .A (a branch's offset, -128 to 127)
s_target:
            ldy         #$FF
            cmp         #$80
            bcs         :+
            iny
:
            clc
            adc         S_NEXT
            sta         S_TGT
            tya
            adc         S_NEXT + 1
            sta         S_TGT + 1
            rts

; .A on the end of the code (S_BUF, S_CODE).  Keeps .A, .Y, C
s_put:
            ldx         S_CODE
            sta         S_BUF,X
            inc         S_CODE
            rts

; .A = the task's zero page byte .Y (N and Z: its).  Keeps .Y
s_zget:
            ldx         S_TASK
            php
            sei
            stx         T_REGISTER                          ; ---- The task
            lda         a:$0000,Y
            stz         T_REGISTER                          ; ---- Back
            plp
            ora         #0
            rts

; The task's zero page byte .Y = .A.  Keeps .A, .Y
s_zput:
            ldx         S_TASK
            php
            sei
            stx         T_REGISTER                          ; ---- The task
            sta         a:$0000,Y
            stz         T_REGISTER                          ; ---- Back
            plp
            rts

; .A = the task's stack byte S_SP + .Y (its stack page, from its TK_SP: the frame at 1-8, what the program pushed
; from 9).  Modifies .X, .Y
s_sget:
            tya
            clc
            adc         S_SP
            tay
            ldx         S_TASK
            php
            sei
            stx         T_REGISTER                          ; ---- The task
            lda         $0100,Y
            stz         T_REGISTER                          ; ---- Back
            plp
            rts

; The task's stack byte S_SP + .Y = .A.  Modifies .X, .Y
s_sput:
            pha
            tya
            clc
            adc         S_SP
            tay
            pla
            ldx         S_TASK
            php
            sei
            stx         T_REGISTER                          ; ---- The task
            sta         $0100,Y
            stz         T_REGISTER                          ; ---- Back
            plp
            rts

.segment "KRODATA_P4"

; JMP (abs)'s code: PHP, PHA, LDA abs, STA TK_STEPTGT, LDA abs + 1, STA TK_STEPTGT + 1, PLA, PLP (abs put in)
s_jmpicode: .byte       $08, $48, $AD, 0, 0, $85, TK_STEPTGT, $AD, 0, 0, $85, TK_STEPTGT + 1, $68, $28
S_JMPI_LEN  = * - s_jmpicode

; Each opcode's kind (SK_*: the high nibble) and length (the low): the W65C02S's, every opcode (those it doesn't
; define are NOPs of 1, 2 or 3 bytes)
s_ops:
            .byte       $91, $02, $02, $01, $02, $02, $02, $02, $01, $02, $01, $01, $03, $03, $03, $23      ; $0x
            .byte       $12, $02, $02, $01, $02, $02, $02, $02, $01, $03, $01, $01, $03, $03, $03, $23      ; $1x
            .byte       $63, $02, $02, $01, $02, $02, $02, $02, $01, $02, $01, $01, $03, $03, $03, $23      ; $2x
            .byte       $12, $02, $02, $01, $02, $02, $02, $02, $01, $03, $01, $01, $03, $03, $03, $23      ; $3x
            .byte       $81, $02, $02, $01, $02, $02, $02, $02, $01, $02, $01, $01, $33, $03, $03, $23      ; $4x
            .byte       $12, $02, $02, $01, $02, $02, $02, $02, $01, $03, $01, $01, $03, $03, $03, $23      ; $5x
            .byte       $71, $02, $02, $01, $02, $02, $02, $02, $01, $02, $01, $01, $43, $03, $03, $23      ; $6x
            .byte       $12, $02, $02, $01, $02, $02, $02, $02, $01, $03, $01, $01, $53, $03, $03, $23      ; $7x
            .byte       $12, $02, $02, $01, $02, $02, $02, $02, $01, $02, $01, $01, $03, $03, $03, $23      ; $8x
            .byte       $12, $02, $02, $01, $02, $02, $02, $02, $01, $03, $01, $01, $03, $03, $03, $23      ; $9x
            .byte       $02, $02, $02, $01, $02, $02, $02, $02, $01, $02, $01, $01, $03, $03, $03, $23      ; $Ax
            .byte       $12, $02, $02, $01, $02, $02, $02, $02, $01, $03, $01, $01, $03, $03, $03, $23      ; $Bx
            .byte       $02, $02, $02, $01, $02, $02, $02, $02, $01, $02, $01, $01, $03, $03, $03, $23      ; $Cx
            .byte       $12, $02, $02, $01, $02, $02, $02, $02, $01, $03, $01, $91, $03, $03, $03, $23      ; $Dx
            .byte       $02, $02, $02, $01, $02, $02, $02, $02, $01, $02, $01, $01, $03, $03, $03, $23      ; $Ex
            .byte       $12, $02, $02, $01, $02, $02, $02, $02, $01, $03, $01, $01, $03, $03, $03, $23      ; $Fx

.assert     SK_NO = 9 .and SK_RTI = 8 .and SK_JSR = 6 .and SK_BRANCH = 1 .and SK_BBX = 2, error, "s_ops' kinds"
