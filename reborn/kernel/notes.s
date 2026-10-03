; ****************************************************************************
; notes.s - notes: Plan 9's notes (signals), with numbers (docs/reimplementation-from-scratch.md, §10.7).
;
; NOTE marks a note pending in a task (TK_NOTES and TK_NOTED, in its zero page) and wakes it if it waits (WAIT,
; SLEEP, BLOCKED); a note to a note group goes to each task in it.  A task takes its notes in its own code, never
; inside the kernel: when the switch into it finds one pending and its frame in its own code (W = 0, its PC below
; $E000: sched.s), and when a system call that waits (WAIT, PAUSE, SLEEP, SLEEP_UNTIL, GETC) ends early for one,
; with E_INTR (K_NOTE_RETURN).  Either way its state is a frame on its stack (U Y W X A P PC), and K_NOTE_TRAMP
; runs in the task: each note to its handler (NOTIFY) with .A = the note, which returns C = 0 to go on (the next
; note, then the frame is resumed) or C = 1 for the default.  With no handler, and for NOTE_KILL, the default:
; EXITS, with 128 + the note's Unix number, as before (130 for Ctrl-C, 137 for kill).  A note that comes while
; its handler runs waits for it (but a kill).  A BRK in a program is the note NOTE_BRK (irq.s).

.include "kdefs.inc"

.segment "KCODE"

; NOTE: send a note.  IN: .A = a task, or NOTE_GROUP | a note group; .X = the note (1-31).  OUT: C = 0; or C = 1,
; .A = E_INVAL (no such note), E_SRCH (no such task, or nobody in the group), E_PERM (the kernel task, or a
; driver).  A note to this task (or its group) is taken on the way out
K_NOTE:
            jsr         K_NOTE_POST
            jmp         K_NOTE_CHECK

; NOTE_POST: NOTE's work, without taking the caller's own notes: from a task, or an irq entry (quick looks only,
; IRQs off for each task; but a note to a group is too long for an irq entry: NOTE_QUEUE).  IN, OUT: as NOTE's.
; Its scratch: TQ_*, in the calling task's OS zero page (an irq entry's own: they don't nest)
K_NOTE_POST:
            cpx         #1
            bcc         @inval
            cpx         #NOTE_MAX + 1
            bcs         @inval
            sta         TQ_WHO
            txa                                             ; Its bit, and its byte in TK_NOTES
            and         #7
            tay
            lda         N_BIT8,Y
            sta         TQ_MASK
            txa
            lsr
            lsr
            lsr
            sta         TQ_INDEX
            lda         TQ_WHO
            bmi         @group
            cmp         #TASKS
            bcs         @srch
            tax
            jmp         q_post

@group:
            and         #TASKS - 1
            sta         TQ_WHO
            stz         TQ_POSTED
            ldx         #TASKS - 1
@member:
            ldy         T_REGISTER                          ; Its group: the kernel task's table (a quick look)
            php
            sei
            stz         T_REGISTER
            lda         K_NGROUP,X
            sty         T_REGISTER
            plp
            cmp         TQ_WHO
            bne         @next
            jsr         q_post
            bcs         @next                               ; (Not one to take notes: the next)
            inc         TQ_POSTED
@next:
            dex
            bne         @member                             ; (Not the kernel task)
            lda         TQ_POSTED
            beq         @srch
            clc
            rts

@inval:
            FAIL        E_INVAL

@srch:
            FAIL        E_SRCH

; NOTE_QUEUE: a note to a note group, from an irq entry (a console's Ctrl-C): queued in the kernel task, which posts
; it as soon as it runs (the scheduler runs it then: sched.s).  As short as can be: nothing is checked.  IN: IRQs
; off; .X = the group (0-15); .A = the note's bit: 1 << (note - 1), for a note 1-8.  Keeps .X
K_NOTE_QUEUE:
            ldy         T_REGISTER
            stz         T_REGISTER                          ; ---- The kernel task
            ora         K0_NQ,X
            sta         K0_NQ,X
            sta         K0_NQ_ANY                           ; (Not 0)
            sty         T_REGISTER                          ; ---- Back
            rts

; The notes irq entries queued, posted (and the tasks woken): in the kernel task, IRQs on (its idle loop)
K_NOTE_QUEUED:
            stz         K0_NQ_ANY
            ldx         #TASKS - 1                          ; Each group's ...
@group:
            sei
            lda         K0_NQ,X
            stz         K0_NQ,X
            cli
            sta         TN_A
            phx
            ldx         #1                                  ; ... notes, from 1
@note:
            lsr         TN_A
            bcc         @next
            pla                                             ; (The group)
            pha
            phx
            ora         #NOTE_GROUP
            jsr         K_NOTE_POST                         ; (Its errors: nobody to tell)
            plx
@next:
            inx
            lda         TN_A
            bne         @note
            plx
            dex
            bpl         @group
            rts

; The note (TQ_MASK in byte TQ_INDEX) to task .X: pending, and the task woken if it waits (or its next PAUSE won't:
; between a look for notes and a PAUSE).  OUT: C = 0; or C = 1, .A = E_PERM (the kernel task, a driver), E_SRCH
; (free, or not started).  Keeps .X
q_post:
            cpx         #KERNEL_TASK
            beq         @perm
            ldy         T_REGISTER
            php
            sei
            stx         T_REGISTER                          ; ---- The task: one to take notes?
            lda         TK_STATE
            beq         @srch
            cmp         #ST_NEW
            beq         @srch
            lda         TK_FLAGS
            and         #TF_DRIVER
            bne         @perm_back
            sty         T_REGISTER                          ; ---- Back: its byte, its bit
            lda         TQ_INDEX
            beq         @b0
            cmp         #2
            bcc         @b1
            beq         @b2
            lda         TQ_MASK
            stx         T_REGISTER                          ; ---- The task
            ora         TK_NOTES + 3
            sta         TK_NOTES + 3
            bra         @noted

@b0:
            lda         TQ_MASK
            stx         T_REGISTER
            ora         TK_NOTES
            sta         TK_NOTES
            bra         @noted

@b1:
            lda         TQ_MASK
            stx         T_REGISTER
            ora         TK_NOTES + 1
            sta         TK_NOTES + 1
            bra         @noted

@b2:
            lda         TQ_MASK
            stx         T_REGISTER
            ora         TK_NOTES + 2
            sta         TK_NOTES + 2
@noted:
            lda         #1
            sta         TK_NOTED
            lda         TK_STATE                            ; Woken, if it waits
            cmp         #ST_WAIT
            beq         @wake
            cmp         #ST_SLEEP
            beq         @wake
            cmp         #ST_BLOCKED
            beq         @wake
            cmp         #ST_EVENT
            beq         @wake
            lda         #1
            sta         TK_WOKEN
            bra         @done

@wake:
            lda         #ST_READY
            sta         TK_STATE
@done:
            sty         T_REGISTER                          ; ---- Back
            plp
            clc
            rts

@srch:
            sty         T_REGISTER
            plp
            FAIL        E_SRCH

@perm_back:
            sty         T_REGISTER
            plp
@perm:
            FAIL        E_PERM

; PAUSE, as programs call it: a note pending (it wakes the task) is taken on the way out
K_UPAUSE:
            jsr         K_PAUSE
            jmp         K_NOTE_CHECK

; The end of a system call, at the program's return address (the stack's top): through the notes pending, if
; any (and its handler isn't running), else back.  Keeps .A, .X, .Y and the flags
K_NOTE_CHECK:
            php
            pha
            lda         TK_INNOTE
            bne         @back
            lda         TK_NOTED
            beq         @back
            pla
            plp
            bra         K_NOTE_RETURN

@back:
            pla
            plp
            rts

; Return from a system call through the notes: the call's results (.A, .X, .Y, P) and the program's return
; address (the stack's top: its jsr's) become a frame, as a switch leaves it, and the trampoline takes the notes
; then resumes it
K_NOTE_RETURN:
            php
            sta         TN_A
            stx         TN_X
            sty         TN_Y
            pla
            sta         TN_P
            pla                                             ; The return address: its jsr's last byte ...
            clc
            adc         #1                                  ;   ... + 1, for RTI
            tax
            pla
            adc         #0
            pha                                             ; PCH
            phx                                             ; PCL
            lda         TN_P
            pha                                             ; P
            lda         TN_A
            pha                                             ; A
            lda         TN_X
            pha                                             ; X
            lda         W_REGISTER
            pha                                             ; W
            lda         TN_Y
            pha                                             ; Y
            lda         U_REGISTER
            pha                                             ; U

; The trampoline: in the noted task, its frame on its stack's top.  Its notes, then the frame
K_NOTE_TRAMP:
            cli
@next:
            jsr         K_NOTE_TAKE
            beq         @resume
            pha                                             ; (The note, for the default)
            cmp         #NOTE_KILL
            beq         @default
            ldx         TA_NOTIFY + 1
            beq         @default                            ; (No handler)
            inc         TK_INNOTE
            jsr         @handler                            ; (.A: the note)
            dec         TK_INNOTE
            bcs         @default
            pla
            bra         @next

@default:                                                   ; EXITS: 128 + its Unix number, and its name
            pla
            tay                                             ; (.Y: the note)
            cmp         #NOTE_BRK + 1
            bcc         :+
            lda         #0                                  ; (No name: "note", and 128 + the note)
:
            tax
            lda         N_MSG_LO,X
            sta         r0
            lda         N_MSG_HI,X
            sta         r0 + 1
            lda         N_CODES,X
            bne         :+
            tya
            ora         #$80
:
            jmp         K_EXITS

@resume:
            sei
            jmp         K_SCHED_RESUME                      ; (U, Y, W, X, A, P, PC: the frame)

@handler:
            jmp         (TA_NOTIFY)

; The note to take: the kill first, then the lowest; its bit cleared, and TK_NOTED with the last.  OUT: .A = it,
; Z = 1 if there's none.  Modifies .X, .Y
K_NOTE_TAKE:
            php
            sei
            lda         #1 << NOTE_KILL
            and         TK_NOTES
            beq         :+
            trb         TK_NOTES
            lda         #NOTE_KILL
            bra         @took
:
            ldy         #0                                  ; The lowest: its byte ...
@byte:
            lda         TK_NOTES,Y
            bne         @found
            iny
            cpy         #4
            bne         @byte
            stz         TK_NOTED                            ; (None)
            plp
            lda         #0
            rts

@found:
            ldx         #0                                  ; ... and its bit
:
            lsr
            bcs         :+
            inx
            bra         :-
:
            lda         N_BIT8,X
            eor         #$FF
            and         TK_NOTES,Y
            sta         TK_NOTES,Y
            tya                                             ; The note: byte * 8 + bit
            asl
            asl
            asl
            stx         TN_A
            ora         TN_A
@took:
            tax
            lda         TK_NOTES                            ; The last?
            ora         TK_NOTES + 1
            ora         TK_NOTES + 2
            ora         TK_NOTES + 3
            bne         :+
            stz         TK_NOTED
:
            plp
            txa                                             ; (Z: 0, a note)
            rts

.segment "KRODATA"
N_BIT8:     .byte       $01, $02, $04, $08, $10, $20, $40, $80
; By note (0: a program's own, 16-31): its exit code (0: 128 + the note) and name
N_CODES:    .byte       0, 130, 137, 129, 142, 133
N_MSG_LO:   .byte       <N_S_NOTE, <N_S_INTR, <N_S_KILL, <N_S_HUP, <N_S_ALRM, <N_S_BRK
N_MSG_HI:   .byte       >N_S_NOTE, >N_S_INTR, >N_S_KILL, >N_S_HUP, >N_S_ALRM, >N_S_BRK
N_S_NOTE:   .byte       "note", 0
N_S_INTR:   .byte       "interrupt", 0
N_S_KILL:   .byte       "killed", 0
N_S_HUP:    .byte       "hangup", 0
N_S_ALRM:   .byte       "alarm", 0
N_S_BRK:    .byte       "sys: brk", 0

; ****************************************************************************
.segment "KCODE_P1"

; NOTIFY: the note handler.  IN: r0 = it, or 0 for none (the defaults).  OUT: C = 0
K_NOTIFY:
            lda         r0
            sta         TA_NOTIFY
            lda         r0 + 1
            sta         TA_NOTIFY + 1
            clc
            rts

