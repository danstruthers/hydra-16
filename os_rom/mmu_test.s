.debuginfo

; ****************************************************************************
; BIOS ROM page 1: included inside `.scope PAGE1` (see all.s), so MM_* / SH_* / TASK_CALL calls go through
; the page 1 gates (page1.s).  Routines passed to TASK_CALL are named with :: (their page 0 addresses),
; because TASK_CALL runs them on page 0.

.segment "TESTS_P1"

; ****************************************************************************
; MMU self test (TH_MMU_TEST, $F833; from WOZMON: F833R; runs on ROM page 1).  Runs the handle calls in the current task and
; prints "MMU test: ok", or "MMU test: FAIL x ee" (x = failing step, ee = error code or value read).
; Allocates and frees task RAM pages (at the top of the free area), chunks, 2 RAM banks and shared memory,
; and borrows (and resets) task MMU_TEST_TASK.

.macro _M_MT_FAIL_IF_C  step                                ; Fail if the call returned an error
            bcc         :+
            ldx         #step
            jmp         @fail
:
.endmacro

.macro _M_MT_FAIL_IF_NC step                                ; Fail if the call should have failed but didn't
            bcs         :+
            ldx         #step
            jmp         @fail
:
.endmacro

.macro _M_MT_EXPECT     step, value                         ; Fail if .A <> value
            cmp         #value
            beq         :+
            ldx         #step
            jmp         @fail
:
.endmacro

NamedHString    S_MMU_TEST, "MMU test: "

MMU_TEST_TASK   = 2                                         ; An idle task the test borrows (and resets)

MMU_TEST:
            PUSH_AXY
            _M_WRITE_HSTRING    S_MMU_TEST
            lda         MMU_LOW_WATER                       ; Lowest allocated page before the test
            sta         ZP_TEMP_VEC4 + 1                    ; (e.g. HyForth's arena)

; Small (in-entry) allocation
            lda         #2                                  ; 2 bytes
            ldy         #0
            ldx         #0
            jsr         MM_ALLOC
            _M_MT_FAIL_IF_C     'a'
            sta         ZP_TEMP                             ; h1
            ldx         #$5A
            ldy         #1
            jsr         MM_WRITE
            _M_MT_FAIL_IF_C     'b'
            lda         ZP_TEMP
            ldy         #1
            jsr         MM_READ
            _M_MT_FAIL_IF_C     'c'
            _M_MT_EXPECT        'c', $5A
            lda         ZP_TEMP
            ldy         #2                                  ; Past the end
            jsr         MM_READ
            _M_MT_FAIL_IF_NC    'd'

; Task RAM pages
            lda         #<300                               ; 2 pages
            ldy         #>300
            ldx         #0
            jsr         MM_ALLOC
            _M_MT_FAIL_IF_C     'e'
            sta         ZP_TEMP_2                           ; h2
            jsr         MM_LOCK
            _M_MT_FAIL_IF_C     'f'
            sta         ZP_TEMP_VEC3
            sty         ZP_TEMP_VEC3 + 1
            _M_MT_EXPECT        'f', 0                      ; Page aligned
            tya
            cmp         MMU_LOW_WATER                       ; Lowest allocated page
            beq         :+
            ldx         #'f'
            jmp         @fail
:
            lda         #$A5                                ; Write through the raw pointer
            ldy         #255
            sta         (ZP_TEMP_VEC3),Y
            lda         ZP_TEMP_2
            jsr         MM_FREE                             ; Locked: must fail (preserves .X = old bank)
            _M_MT_FAIL_IF_NC    'g'
            lda         ZP_TEMP_2
            jsr         MM_UNLOCK
            _M_MT_FAIL_IF_C     'h'
            lda         ZP_TEMP_2
            ldy         #255
            jsr         MM_READ
            _M_MT_FAIL_IF_C     'i'
            _M_MT_EXPECT        'i', $A5

; RAM banks
            lda         #<$2001                             ; 2 banks
            ldy         #>$2001
            ldx         #AI_PAGED
            jsr         MM_ALLOC
            _M_MT_FAIL_IF_C     'j'
            sta         ZP_TEMP_VEC4                        ; h3
            ldx         #$3C
            ldy         #7
            jsr         MM_WRITE
            _M_MT_FAIL_IF_C     'k'
            lda         ZP_TEMP_VEC4
            ldy         #7
            jsr         MM_READ
            _M_MT_FAIL_IF_C     'k'
            _M_MT_EXPECT        'k', $3C

; Chunks: two 16-byte chunks share a page
            lda         #10                                 ; 16-byte class
            ldy         #0
            ldx         #0
            jsr         MM_ALLOC
            _M_MT_FAIL_IF_C     'o'
            sta         ZP_TEMP_VEC3                        ; c1
            lda         #16
            ldy         #0
            ldx         #0
            jsr         MM_ALLOC
            _M_MT_FAIL_IF_C     'o'
            sta         ZP_TEMP_VEC3 + 1                    ; c2
            lda         ZP_TEMP_VEC3
            jsr         MM_LOCK
            _M_MT_FAIL_IF_C     'p'
            sta         ZP_TEMP_VEC                         ; c1's address
            sty         ZP_TEMP_VEC + 1
            lda         ZP_TEMP_VEC3
            jsr         MM_UNLOCK
            lda         ZP_TEMP_VEC3 + 1
            jsr         MM_LOCK
            _M_MT_FAIL_IF_C     'p'
            cpy         ZP_TEMP_VEC + 1                     ; Same page...
            beq         :+
            tya
            ldx         #'p'
            jmp         @fail
:
            cmp         ZP_TEMP_VEC                         ; ...different chunk
            bne         :+
            ldx         #'p'
            jmp         @fail
:
            lda         ZP_TEMP_VEC3 + 1
            jsr         MM_UNLOCK
            lda         ZP_TEMP_VEC3
            ldx         #$77
            ldy         #15                                 ; Last byte of the chunk
            jsr         MM_WRITE
            _M_MT_FAIL_IF_C     'q'
            lda         ZP_TEMP_VEC3
            ldy         #15
            jsr         MM_READ
            _M_MT_FAIL_IF_C     'q'
            _M_MT_EXPECT        'q', $77
            lda         ZP_TEMP_VEC3
            ldy         #16                                 ; Past the end of the chunk
            jsr         MM_READ
            _M_MT_FAIL_IF_NC    'r'

; Chunks: four 64-byte chunks need two chunk pages (3 per page)
            ldx         #3

@alloc_64:
            phx
            lda         #64
            ldy         #0
            ldx         #0
            jsr         MM_ALLOC
            plx
            _M_MT_FAIL_IF_C     's'
            sta         ZP_TEMP_VEC,X                       ; ZP_TEMP_VEC .. ZP_TEMP_VEC2 + 1
            dex
            bpl         @alloc_64
            ldx         #3

@free_64:
            lda         ZP_TEMP_VEC,X
            jsr         MM_FREE
            _M_MT_FAIL_IF_C     't'
            dex
            bpl         @free_64
            lda         ZP_TEMP_VEC3
            jsr         MM_FREE
            _M_MT_FAIL_IF_C     'u'
            lda         ZP_TEMP_VEC3 + 1
            jsr         MM_FREE
            _M_MT_FAIL_IF_C     'u'

; Free everything; the page map must be back where it started
            lda         ZP_TEMP
            jsr         MM_FREE
            _M_MT_FAIL_IF_C     'l'
            lda         ZP_TEMP_2
            jsr         MM_FREE
            _M_MT_FAIL_IF_C     'l'
            lda         ZP_TEMP_VEC4
            jsr         MM_FREE
            _M_MT_FAIL_IF_C     'l'
            lda         MMU_LOW_WATER                       ; Back to where it was
            cmp         ZP_TEMP_VEC4 + 1
            beq         :+
            ldx         #'m'
            jmp         @fail
:
            lda         ZP_TEMP_2
            jsr         MM_FREE                             ; Already freed: must fail
            _M_MT_FAIL_IF_NC    'n'

; Shared memory
            lda         #100
            ldy         #0
            jsr         SH_ALLOC
            _M_MT_FAIL_IF_C     'v'
            sta         ZP_TEMP                             ; sh
            ldx         #$C3
            ldy         #5
            jsr         SH_WRITE
            _M_MT_FAIL_IF_C     'w'
            lda         ZP_TEMP
            ldy         #5
            jsr         SH_READ
            _M_MT_FAIL_IF_C     'w'
            _M_MT_EXPECT        'w', $C3
            lda         ZP_TEMP
            jsr         SH_LOCK                             ; .X = old bank, .Y = old U
            _M_MT_FAIL_IF_C     'x'
            lda         SH_WINDOW + 5
            jsr         SH_UNLOCK
            _M_MT_EXPECT        'x', $C3

; ...a second task (MMU_TEST_TASK) takes a reference; ours goes
            LOAD_ADDR   ::SH_ATTACH, ZP_TC_VEC
            lda         #MMU_TEST_TASK
            sta         ZP_TC_TASK
            lda         ZP_TEMP
            jsr         TASK_CALL                           ; SH_ATTACH in the other task
            _M_MT_FAIL_IF_C     'y'
            lda         ZP_TEMP
            jsr         SH_DETACH
            _M_MT_FAIL_IF_C     'z'
            lda         ZP_TEMP
            ldy         #5
            jsr         SH_READ                             ; No reference any more: must fail
            _M_MT_FAIL_IF_NC    'z'
            LOAD_ADDR   ::SH_READ, ZP_TC_VEC
            lda         #MMU_TEST_TASK
            sta         ZP_TC_TASK
            lda         ZP_TEMP
            ldy         #5
            jsr         TASK_CALL                           ; The other task can still read it
            _M_MT_FAIL_IF_C     '1'
            _M_MT_EXPECT        '1', $C3

; ...resetting the other task drops its reference: the shared memory is freed
            lda         #MMU_TEST_TASK
            jsr         MM_TASK_RESET
            lda         ZP_TEMP
            jsr         SH_ATTACH                           ; Freed: must fail
            _M_MT_FAIL_IF_NC    '2'

; MM_TASK_RESET frees a task's own allocations (its handles start over at 1)
            LOAD_ADDR   ::MM_ALLOC, ZP_TC_VEC
            lda         #MMU_TEST_TASK
            sta         ZP_TC_TASK
            lda         #2
            ldy         #0
            ldx         #0
            jsr         TASK_CALL                           ; Handle 1 in the other task
            _M_MT_FAIL_IF_C     '3'
            lda         #2
            ldy         #0
            ldx         #0
            jsr         TASK_CALL                           ; Handle 2
            _M_MT_FAIL_IF_C     '3'
            _M_MT_EXPECT        '3', 2
            lda         #MMU_TEST_TASK
            jsr         MM_TASK_RESET
            LOAD_ADDR   ::MM_ALLOC, ZP_TC_VEC
            lda         #MMU_TEST_TASK
            sta         ZP_TC_TASK
            lda         #2
            ldy         #0
            ldx         #0
            jsr         TASK_CALL
            _M_MT_FAIL_IF_C     '4'
            _M_MT_EXPECT        '4', 1
            lda         #MMU_TEST_TASK
            jsr         MM_TASK_RESET

            PRINT_CHAR  #'o', #'k'
            bra         @end

@fail:                                                      ; .X = step, .A = error / value
            pha
            phx
            PRINT_CHAR  #'F', #'A', #'I', #'L', #' '
            pla
            PRINT_CHAR
            PRINT_SPACE
            pla
            PRINT_BYTE

@end:
            PRINT_CRLF
            PULL_YXA
            rts
