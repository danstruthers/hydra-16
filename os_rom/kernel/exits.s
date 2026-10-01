.debuginfo

; ****************************************************************************
; Exit statuses (BIOS ROM page 5, included inside `.scope PAGE5`, see all.s), as Plan 9's exits and wait: a task
; ends with a code (0: success) and a message (up to EXIT_MSG_MAX characters; "" for none), and the task that
; started it gets them when it waits for it (TASK_JOIN).  They're kept in the system's shared bank, one record a
; task (EXIT_TABLE: io.inc), written as the task ends: before its parent is woken, so a wait always finds it.
;   TASK_EXITS ends the calling task with a code and a message; a task whose entry point returns ends with 0
; (TASK_EXIT: EXIT_NOTE); a break with no handler ends it with EXIT_BREAK, "interrupt", and a kill with
; EXIT_KILLED, "killed" (EXIT_SIGNALLED, from BREAK_ENTRY).
; The shell keeps the last program's in $status (/env/status: the message, or the code if there's none, or ""
; for success) and HyForth's status word (the code).

.segment "FP_P5"

EXIT_P          = ZP_SLEEP_SCAN                             ; (2) A record's address.  Only with IRQs off and
                                                            ;   no YIELD in between (as sem.s's SEM_Z)

; ****************************************************************************
; End this task with an exit status: .A = its code (0: success), ZP_IO_BUF = its message (zero-terminated; a
; high byte of 0: none).  Its files are closed and its memory freed (TASK_EXIT), and its parent, if it's waiting
; (TASK_JOIN), wakes.  Doesn't return.
TASK_EXITS:
            ldx         ZP_IO_BUF + 1                       ; (A message?)
            jsr         EXIT_NOTE
            jmp         TASK_EXIT_NOTED

; A break with no handler, or a kill, is ending this task (BREAK_ENTRY): its record says which.
; IN: .X = its TASK_STATUS_REG as the signal came (TASK_KILL_FLAG: a kill).  Doesn't return
EXIT_SIGNALLED:
            txa
            ldx         #<EXIT_S_KILLED
            ldy         #>EXIT_S_KILLED
            and         #TASK_KILL_FLAG
            bne         :+
            ldx         #<EXIT_S_BREAK
            ldy         #>EXIT_S_BREAK
:
            stx         ZP_IO_BUF
            sty         ZP_IO_BUF + 1
            ldx         #EXIT_KILLED
            cmp         #0
            bne         :+
            ldx         #EXIT_BREAK
:
            txa
            ldx         #1                                  ; (A message: page 5's own, seen here)
            jsr         EXIT_NOTE
            jmp         TASK_EXIT_NOTED

EXIT_S_BREAK:   .byte   "interrupt", 0
EXIT_S_KILLED:  .byte   "killed", 0

; As this task ends: the tasks it started (their ZP_TASK_OWNER: it) get its owner instead, as Unix gives its
; orphans to init, so a new task in its slot isn't taken for their parent: the RAM disk's areas (/ram/N, for N's
; family) and kills follow the owner chain (docs/plans/DISKS.md).  Then its own area goes (TASK_AREA_END).
; (From TASK_EXIT_NOTED, after MM_TASK_RESET has closed its fds; IRQs on.)  Modifies: .A, .X, .Y, ZP_TEMP, ZP_TEMP_2
TASK_ORPHANS:
            php
            sei
            ldy         T_REGISTER                          ; .Y = this task, all the way through
            lda         ZP_TASK_OWNER                       ; .A = its owner, theirs now
            ldx         #MAX_TASK_NUMBER

@task:
            stx         T_REGISTER                          ; Quick look (no stack use!)
            cpy         ZP_TASK_OWNER
            bne         :+
            sta         ZP_TASK_OWNER
:
            dex
            bne         @task                               ; (Tasks 15-1: task 0, the system's, has none)
            sty         T_REGISTER
            plp

; As this task ends: its area on the RAM disk (/ram/N), if it may have one, removed with all that's in it
; (HFS_AREA_END, run in the storage task: TASK_CALL).  Which areas there may be is the storage task's RAMD_AREAS:
; a quick look.  Modifies: .A, .X, .Y, ZP_TEMP, ZP_TEMP_2
TASK_AREA_END:
            php
            sei
            ldy         T_REGISTER                          ; .Y = this task
            lda         #STORAGE_TASK_NUM
            sta         T_REGISTER                          ; Quick look (no stack use!)
            lda         #0
            tax
            bbr3        TASK_STATUS_REG, :+                 ; (TASK_RESIDENT_FLAG: not started yet, no areas)
            lda         RAMD_AREAS
            ldx         RAMD_AREAS + 1
:
            sty         T_REGISTER
            plp
            sta         ZP_TEMP                             ; Its bit: bit .Y of .X.A
            stx         ZP_TEMP_2
            tya
            tax
:
            lsr         ZP_TEMP_2
            ror         ZP_TEMP
            dex
            bpl         :-
            bcc         @done                               ; (No area)
            LOAD_ADDR   ::HFS_AREA_END_G, ZP_TC_VEC
            lda         #STORAGE_TASK_NUM
            sta         ZP_TC_TASK
            tya
            jmp         TASK_CALL                           ; (.A = this task)

@done:
            rts

; May task .A (the one asking) use task .X's things?  .Y = 0: the way the RAM disk's areas go (/ram/X is X's
; family's): .A is .X, or a task .X started (directly or not).  .Y = 1: the other way (/proc, planned: a task, or
; a debugger, may reach the tasks it started): .X is .A, or a task .A started.  Task 0, the system's, may use
; anything.  OUT: C = 0: yes; C = 1: no.  Preserves .X, .Y.  Modifies: .A, ZP_TEMP, ZP_TEMP_2
TASK_MAY:
            cmp         #SYSTEM_TASK_NUM
            beq         @yes
            phx
            phy
            cpy         #0
            beq         :+
            pha                                             ; (The other way: .A and .X swapped)
            txa
            plx
:
            jsr         TASK_DESCENDS
            ply
            plx
            rts

@yes:
            clc
            rts

; Is task .A task .X, or one .X started (directly or not: up .A's owner chain, 16 steps at most)?
; OUT: C = 0: yes; C = 1: no.  Modifies: .A, .X, .Y, ZP_TEMP, ZP_TEMP_2
TASK_DESCENDS:
            stx         ZP_TEMP                             ; (The one to find)
            ldy         #MAX_TASK_NUMBER + 1
            sty         ZP_TEMP_2

@up:
            cmp         ZP_TEMP
            beq         @yes
            cmp         #MAX_TASK_NUMBER + 1
            bcs         @no                                 ; ($FF: nobody started it)
            tax
            php
            sei
            ldy         T_REGISTER                          ; Its owner: a quick look in its zero page (no
            stx         T_REGISTER                          ;   RAM but the registers while T is that task)
            ldx         ZP_TASK_OWNER
            sty         T_REGISTER
            plp
            txa
            dec         ZP_TEMP_2
            bne         @up

@no:
            sec
            rts

@yes:
            clc
            rts

; This task's exit record: .A = the code; .X = 0: no message, or its message is at ZP_IO_BUF (in task RAM, or on
; this page).  Preserves .X, .Y (and the caller's I flag).  Modifies: .A, ZP_TEMP, ZP_TEMP_2
EXIT_NOTE:
            php
            sei
            phx
            phy
            sta         ZP_TEMP                             ; (The code)
            stx         ZP_TEMP_2                           ; (A message?)
            lda         T_REGISTER
            jsr         EXIT_PTR
            _M_SYS_ENTER
            lda         ZP_TEMP
            sta         (EXIT_P)
            ldx         ZP_TEMP_2
            ldy         #0

@copy:
            lda         #0
            cpx         #0
            beq         :+
            lda         (ZP_IO_BUF),Y
:
            iny
            sta         (EXIT_P),Y
            cmp         #0
            beq         @done
            cpy         #EXIT_MSG_MAX
            bcc         @copy
            lda         #0                                  ; (Cut short)
            iny
            sta         (EXIT_P),Y

@done:
            _M_SYS_LEAVE
            ply
            plx
            plp
            rts

; ****************************************************************************
; Wait for task .A (one this task started) to end, and get its exit status: .A = its code (0: success), and its
; message (zero-terminated, up to EXIT_MSG_MAX characters) at ZP_IO_BUF, if its high byte isn't 0.  While it
; runs, it has the console if this task has it, and this task has it back after (as the shell's waits).  A task
; that's ended already, or isn't this task's any more, isn't waited for: its record is as it ended.
; OUT: C = 0, .A = the code; or C = 1, .A = ERR_BAD_TASK.  Preserves .X, .Y
TASK_JOIN:
            cmp         #MAX_TASK_NUMBER + 1
            bcc         :+
            lda         #ERR_BAD_TASK
            rts
:
            phx
            phy
            pha                                             ; (The task)
            jsr         JOIN_FG
            php                                             ; (Z = 1: we have the console: it's ours again after)
            bne         :+
            tsx
            lda         $0102,X                             ; (The task, under the php)
            jsr         CONS_SET_FG
:
            tsx
            lda         $0102,X
            php
            sei
            ldy         T_REGISTER
            sta         T_REGISTER                          ; Quick look at the task (no stack use!)
            bbr0        TASK_STATUS_REG, @ended             ; (Ended already: free, ...
            cpy         ZP_TASK_OWNER
            bne         @ended                              ;   or someone else's now)
            sty         TASK_PARENT                         ; It wakes us when it ends (TASK_EXIT) ...
            sty         T_REGISTER
            smb1        TASK_STATUS_REG                     ;   and till then we're paused (TASK_PAUSED_FLAG)
            jsr         YIELD
            bra         @waited

@ended:
            sty         T_REGISTER

@waited:
            plp
            plp                                             ; The console ours again, if we had it (the task
            bne         :+                                  ;   gives it back as it ends, CONS_RELEASE, but not
            lda         T_REGISTER                          ;   if it ended before it got it)
            and         #$0F
            jsr         CONS_SET_FG
:
            pla                                             ; Its record
            php
            sei
            jsr         EXIT_PTR
            _M_SYS_ENTER
            ldx         ZP_IO_BUF + 1
            beq         @code                               ; (No buffer for the message)
            ldy         #0
:
            iny
            lda         (EXIT_P),Y
            dey
            cpy         #EXIT_MSG_MAX                       ; (At most EXIT_MSG_MAX characters)
            bcc         :+
            lda         #0
:
            sta         (ZP_IO_BUF),Y
            iny
            cmp         #0
            bne         :--

@code:
            lda         (EXIT_P)
            tax
            _M_SYS_LEAVE
            plp
            txa
            ply
            plx
            clc
            rts

.assert     TASK_BUSY_FLAG = 1 .and TASK_PAUSED_FLAG = 2, error, "TASK_JOIN tests bit 0 and sets bit 1"

; Do we have the console (the serial driver's foreground task)?  OUT: Z = 1: yes.  Modifies: .A, .X
JOIN_FG:
            php
            sei
            ldx         T_REGISTER
            lda         #SERIAL_TASK_NUM
            sta         T_REGISTER                          ; Quick look (no stack use!)
            lda         ZP_SER_CAPTURE
            stx         T_REGISTER
            plp
            eor         T_REGISTER
            and         #$0F
            rts

; At boot (SEM_INIT): every record "" and 0.  Modifies: .A, .Y.  (In the system's shared bank)
EXIT_INIT:
            lda         #<EXIT_TABLE
            sta         EXIT_P
            lda         #>EXIT_TABLE
            sta         EXIT_P + 1
            ldy         #0
            tya
:
            sta         (EXIT_P),Y
            iny
            bne         :-
            inc         EXIT_P + 1
:
            sta         (EXIT_P),Y
            iny
            bne         :-
            rts

.assert     (MAX_TASK_NUMBER + 1) * EXIT_SIZE = 512, error, "EXIT_INIT: the table is two pages' worth"

; EXIT_P = task .A's record (EXIT_TABLE + task * EXIT_SIZE).  Modifies: .A
EXIT_PTR:
            and         #$0F
            pha
            lsr                                             ; (task * 32: its high byte, task / 8 ...
            lsr
            lsr
            clc
            adc         #>EXIT_TABLE
            sta         EXIT_P + 1
            pla                                             ;   and its low byte, (task & 7) * 32)
            asl
            asl
            asl
            asl
            asl
            clc
            adc         #<EXIT_TABLE
            sta         EXIT_P
            bcc         :+
            inc         EXIT_P + 1
:
            rts

.assert     EXIT_SIZE = 32, error, "EXIT_PTR: a record is 32 bytes"
.assert     EXIT_MSG_MAX + 2 <= EXIT_SIZE, error, "EXIT_NOTE: the code, the message, its 0"
