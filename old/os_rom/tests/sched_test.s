.debuginfo

; ****************************************************************************
; Scheduler self test (TH_SCHED_TEST, $F869; from WOZMON: F869R).  BIOS ROM page 4: included inside
; `.scope PAGE4` (see all.s), so calls go through the page 4 gates.
;
; Prints three lines:
;   1. Two background tasks print "a" and "b" while this task prints "m": the letters interleave
;      (preemption on the timer tick)
;   2. A background task prints "[cccccccccc]" holding NO_PREEMPT while this task prints "m": the
;      bracketed run is never split
;   3. A background task waits (TASK_WAIT); this task prints "w", then wakes it (IO_WAKE) and it prints
;      "W": "wW"
; then "done".  Uses two free tasks.

.segment "TESTS_P4"

NamedHString    S_SCHED_TEST, "Sched test:"
NamedHString    S_SCHED_DONE, "done"
NamedHString    S_SCHED_FAIL, "FAIL: no free task"

ST_COUNT        = 12                                        ; Letters per task

; About 10,000 cycles (~2.8 ms, a little over half a scheduler tick).  Modifies: .X, .Y
ST_DELAY:
            ldy         #8 * CPU_CLOCK_MULT
:
            ldx         #0
:
            dex
            bne         :-
            dey
            bne         :--
            rts

; Print a letter ST_COUNT times, with a delay after each.  IN: .A = letter.  Uses ZP_TEMP, ZP_TEMP_2
ST_PRINT_RUN:
            sta         ZP_TEMP_2
            lda         #ST_COUNT
            sta         ZP_TEMP
:
            lda         ZP_TEMP_2
            PRINT_CHAR
            jsr         ST_DELAY
            dec         ZP_TEMP
            bne         :-
            rts

; Background tasks (each has its own ZP, so they can all use ZP_TEMP)
ST_WORK_A:
            lda         #'a'
            jmp         ST_PRINT_RUN

ST_WORK_B:
            lda         #'b'
            jmp         ST_PRINT_RUN

ST_WORK_C:
            jsr         NO_PREEMPT
            PRINT_CHAR  #'['
            lda         #10
            sta         ZP_TEMP
:
            PRINT_CHAR  #'c'
            jsr         ST_DELAY
            dec         ZP_TEMP
            bne         :-
            PRINT_CHAR  #']'
            jmp         PREEMPT

ST_WORK_D:
            jsr         TASK_WAIT
            PRINT_CHAR  #'W'
            rts

; Start a background task on ROM page 4.  IN: .A.Y = entry point.  OUT: .A = task (C = 1: none free)
.macro _M_ST_RUN    entry
            lda         #<entry
            ldy         #>entry
            ldx         #4
            jsr         TASK_RUN
.endmacro

; Wait (yielding) until a task has finished.  IN: .A = task.  Uses ZP_TEMP_VEC
ST_WAIT_DONE:
            sta         ZP_TEMP_VEC
:
            lda         ZP_TEMP_VEC
            jsr         TASK_STATUS
            and         #TASK_BUSY_FLAG
            beq         :+
            jsr         YIELD
            bra         :-
:
            rts

SCHED_TEST:
            PUSH_AXY
            _M_WRITE_HSTRING    S_SCHED_TEST
            PRINT_CRLF

; 1. Preemption: three tasks printing at once
            _M_ST_RUN   ST_WORK_A
            bcs         @fail
            sta         ZP_TEMP_VEC3
            _M_ST_RUN   ST_WORK_B
            bcs         @fail
            sta         ZP_TEMP_VEC3 + 1
            lda         #'m'
            jsr         ST_PRINT_RUN
            lda         ZP_TEMP_VEC3
            jsr         ST_WAIT_DONE
            lda         ZP_TEMP_VEC3 + 1
            jsr         ST_WAIT_DONE
            PRINT_CRLF

; 2. NO_PREEMPT keeps "[cccccccccc]" together
            _M_ST_RUN   ST_WORK_C
            bcs         @fail
            sta         ZP_TEMP_VEC3
            lda         #'m'
            jsr         ST_PRINT_RUN
            lda         ZP_TEMP_VEC3
            jsr         ST_WAIT_DONE
            PRINT_CRLF

; 3. TASK_WAIT / IO_WAKE: "wW"
            _M_ST_RUN   ST_WORK_D
            bcs         @fail
            sta         ZP_TEMP_VEC3
            jsr         ST_DELAY                            ; Let it get to TASK_WAIT
            jsr         ST_DELAY
            jsr         ST_DELAY
            jsr         ST_DELAY
            PRINT_CHAR  #'w'
            lda         ZP_TEMP_VEC3
            jsr         IO_WAKE
            jsr         ST_WAIT_DONE
            PRINT_CRLF
            _M_WRITE_HSTRING    S_SCHED_DONE
            bra         @end

@fail:
            _M_WRITE_HSTRING    S_SCHED_FAIL

@end:
            PRINT_CRLF
            PULL_YXA
            rts
