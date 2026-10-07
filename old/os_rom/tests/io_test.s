.debuginfo

; ****************************************************************************
; IO self test (TH_IO_TEST, $F88A; from WOZMON: F88AR).  BIOS ROM page 4, included inside `.scope PAGE4`
; (see all.s).  Runs in the current task and prints "IO test: ok", or "IO test: FAIL x ee" (x = failing
; step, ee = error code or value).  Uses /dev/null, /dev/zero and /dev/cons (it writes "cons "), and a
; 300-byte buffer from the MMU.  The task's other fds (e.g. stdin, stdout, stderr) are left alone.

.segment "TESTS_P4"

NamedHString    S_IO_TEST, "IO test: "
S_DEV_NULL:     .byte "/dev/null", 0
S_DEV_ZERO:     .byte "/dev/zero", 0
S_DEV_ZERO_SUB: .byte "/dev/zero/sub", 0
S_DEV_NOTHERE:  .byte "/dev/nothere", 0
S_DEV_ZEROO:    .byte "/dev/zeroo", 0
S_NOT_DEV:      .byte "/foo", 0
S_DEV_CONS_T:   .byte "/dev/cons", 0
S_ZERO_DEV:     .byte "zero", 0
S_ROM_PATH:     .byte "/romz", 0
S_CONS_MSG:     .byte "cons "
CONS_MSG_LEN    = * - S_CONS_MSG
S_NS_N:         .byte "/n", 0
S_NS_Z:         .byte "/z", 0
S_NS_Z_SUB:     .byte "/z/sub", 0
S_NS_X:         .byte "/x", 0
S_NS_Y:         .byte "/y", 0
S_ZERO:         .byte "zero", 0
S_DEV_RAM:      .byte "/dev/ram", 0

; The IO layer runs on page 2 and reads names through their pointers, so a name has to be in RAM: copy
; it to path slot `slot` (0 or 1) after the buffer.  OUT: .A.Y = the copy.  Uses ZP_HS_TEMP
.macro _M_IT_PATH       str, slot
            ldx         #slot * IT_PATH_SIZE
            jsr         IT_PATH_SLOT                        ; ZP_HS_TEMP = the slot
            ldy         #0
:
            lda         str,Y
            sta         (ZP_HS_TEMP),Y
            beq         :+
            iny
            bra         :-
:
            lda         ZP_HS_TEMP
            ldy         ZP_HS_TEMP + 1
.endmacro

.macro _M_IT_NS         call, path, other               ; IO_MOUNT / IO_BIND path, other
            _M_IT_PATH  other, 1
            sta         ZP_IO_BUF
            sty         ZP_IO_BUF + 1
            _M_IT_PATH  path, 0
            ldx         #0                                  ; (No flags)
            jsr         call
.endmacro

.macro _M_IT_UNMOUNT    path
            _M_IT_PATH  path, 0
            ldx         #0                                  ; (All of its entries)
            jsr         IO_UNMOUNT
.endmacro

IT_BUF_SIZE     = 300
IT_PATH_SIZE    = 32                                    ; Each of the two path slots after the buffer

.macro _M_IT_FAIL_IF_C  step                                ; Fail if the call returned an error
            bcc         :+
            ldx         #step
            jmp         @fail
:
.endmacro

.macro _M_IT_FAIL_IF_NC step, error                         ; Fail unless the call failed with this error
            bcs         :+
            lda         #0
            ldx         #step
            jmp         @fail
:
            cmp         #error
            beq         :+
            ldx         #step
            jmp         @fail
:
.endmacro

.macro _M_IT_OPEN       name, mode                          ; .A = fd
            _M_IT_PATH  name, 0
            ldx         #mode
            jsr         IO_OPEN
.endmacro

.macro _M_IT_COUNT      fd, count                           ; Set up a transfer of count bytes to/from the buffer
            lda         ZP_TEMP_VEC3
            sta         ZP_IO_BUF
            lda         ZP_TEMP_VEC3 + 1
            sta         ZP_IO_BUF + 1
            lda         #<count
            sta         ZP_IO_CNT
            lda         #>count
            sta         ZP_IO_CNT + 1
            lda         fd
.endmacro

.macro _M_IT_EXPECT_CNT step, count                         ; Fail unless ZP_IO_CNT = count
            lda         ZP_IO_CNT
            cmp         #<count
            bne         :+
            lda         ZP_IO_CNT + 1
            cmp         #>count
            beq         :++
:
            lda         ZP_IO_CNT
            ldx         #step
            jmp         @fail
:
.endmacro

IO_TEST:
            PUSH_AXY
            _M_WRITE_HSTRING    S_IO_TEST
            lda         #<(IT_BUF_SIZE + 2 * IT_PATH_SIZE)  ; The buffer and the path slots (task RAM
            ldy         #>(IT_BUF_SIZE + 2 * IT_PATH_SIZE)  ;   pages)
            ldx         #0
            jsr         MM_ALLOC
            _M_IT_FAIL_IF_C     '0'
            sta         ZP_TEMP_VEC4                        ; Its handle
            jsr         MM_LOCK                             ; .A.Y = address (page blocks don't move)
            sta         ZP_TEMP_VEC3
            sty         ZP_TEMP_VEC3 + 1
            lda         ZP_TEMP_VEC4
            jsr         MM_UNLOCK

; /dev/null: writes take everything (split over two transfers), reads are empty
            _M_IT_OPEN  S_DEV_NULL, IO_MODE_RDWR
            _M_IT_FAIL_IF_C     'a'
            sta         ZP_TEMP                             ; (The first free fd)
            _M_IT_COUNT ZP_TEMP, IT_BUF_SIZE
            jsr         IO_WRITE
            _M_IT_FAIL_IF_C     'b'
            _M_IT_EXPECT_CNT    'b', IT_BUF_SIZE
            _M_IT_COUNT ZP_TEMP, 10
            jsr         IO_READ
            _M_IT_FAIL_IF_C     'c'
            _M_IT_EXPECT_CNT    'c', 0

; /dev/zero: reads fill the buffer with zeros; it's read-only here
            ldy         #0                                  ; Fill the buffer with $FF first
            lda         #$FF
:
            sta         (ZP_TEMP_VEC3),Y
            iny
            bne         :-
            inc         ZP_TEMP_VEC3 + 1
:
            sta         (ZP_TEMP_VEC3),Y
            iny
            cpy         #<IT_BUF_SIZE
            bne         :-
            dec         ZP_TEMP_VEC3 + 1
            _M_IT_OPEN  S_DEV_ZERO, IO_MODE_READ
            _M_IT_FAIL_IF_C     'd'
            sta         ZP_TEMP_2                           ; (The next one)
            _M_IT_COUNT ZP_TEMP_2, IT_BUF_SIZE
            jsr         IO_READ
            _M_IT_FAIL_IF_C     'e'
            _M_IT_EXPECT_CNT    'e', IT_BUF_SIZE
            ldy         #0                                  ; All zeros?
:
            lda         (ZP_TEMP_VEC3),Y
            bne         @not_zero
            iny
            bne         :-
            inc         ZP_TEMP_VEC3 + 1
:
            lda         (ZP_TEMP_VEC3),Y
            bne         @not_zero
            iny
            cpy         #<IT_BUF_SIZE
            bne         :-
            dec         ZP_TEMP_VEC3 + 1
            _M_IT_COUNT ZP_TEMP_2, 5
            jsr         IO_WRITE                            ; Read-only: must fail
            _M_IT_FAIL_IF_NC    'f', ERR_IO_MODE
            bra         @getc

@not_zero:
            ldx         #'e'
            jmp         @fail

; One byte at a time
@getc:
            ldx         ZP_TEMP_2
            jsr         IO_GETC
            _M_IT_FAIL_IF_C     'g'
            cmp         #0
            beq         :+
            ldx         #'g'
            jmp         @fail
:
            ldx         ZP_TEMP
            lda         #'x'
            jsr         IO_PUTC
            _M_IT_FAIL_IF_C     'h'
            ldx         ZP_TEMP
            jsr         IO_GETC                             ; /dev/null: end of file
            _M_IT_FAIL_IF_NC    'h', ERR_IO_EOF

; Closing
            lda         ZP_TEMP
            jsr         IO_CLOSE
            _M_IT_FAIL_IF_C     'i'
            lda         ZP_TEMP_2
            jsr         IO_CLOSE
            _M_IT_FAIL_IF_C     'i'
            lda         ZP_TEMP
            jsr         IO_CLOSE                            ; Closed already
            _M_IT_FAIL_IF_NC    'j', ERR_IO_BAD_FD
            _M_IT_COUNT ZP_TEMP, 1
            jsr         IO_READ                             ; Closed
            _M_IT_FAIL_IF_NC    'j', ERR_IO_BAD_FD
            lda         #IO_MAX_FDS                         ; Out of range
            jsr         IO_CLOSE
            _M_IT_FAIL_IF_NC    'j', ERR_IO_BAD_FD

; Names
            _M_IT_OPEN  S_DEV_NOTHERE, IO_MODE_READ
            _M_IT_FAIL_IF_NC    'k', ERR_IO_NOT_FOUND
            _M_IT_OPEN  S_DEV_ZEROO, IO_MODE_READ
            _M_IT_FAIL_IF_NC    'k', ERR_IO_NOT_FOUND
            _M_IT_OPEN  S_NOT_DEV, IO_MODE_READ
            _M_IT_FAIL_IF_NC    'k', ERR_IO_NOT_FOUND
            lda         #<S_DEV_NULL                        ; A name straight from the BIOS ROM (this page):
            ldy         #>S_DEV_NULL                        ;   IO_OPEN reads it here (a far pointer)
            ldx         #IO_MODE_READ
            jsr         IO_OPEN
            _M_IT_FAIL_IF_C     'k'
            jsr         IO_CLOSE
            _M_IT_FAIL_IF_C     'k'
            lda         #<S_ZERO_DEV                        ; IO_MOUNT and IO_UNMOUNT: both names from here too
            sta         ZP_IO_BUF
            lda         #>S_ZERO_DEV
            sta         ZP_IO_BUF + 1
            lda         #<S_ROM_PATH
            ldy         #>S_ROM_PATH
            ldx         #0
            jsr         IO_MOUNT
            _M_IT_FAIL_IF_C     'k'
            lda         #<S_ROM_PATH
            ldy         #>S_ROM_PATH
            ldx         #0
            jsr         IO_UNMOUNT
            _M_IT_FAIL_IF_C     'k'
            _M_IT_OPEN  S_DEV_ZERO_SUB, IO_MODE_READ            ; A path inside the device: the server's
            _M_IT_FAIL_IF_C     'l'                                 ;   business (zero ignores it)
            jsr         IO_CLOSE
            _M_IT_FAIL_IF_C     'l'

; /dev/cons: writes show up; an unknown control code is refused
            _M_IT_OPEN  S_DEV_CONS_T, IO_MODE_WRITE
            _M_IT_FAIL_IF_C     'q'
            sta         ZP_TEMP
            ldy         #CONS_MSG_LEN - 1                   ; The message, into the buffer (in task RAM)
:
            lda         S_CONS_MSG,Y
            sta         (ZP_TEMP_VEC3),Y
            dey
            bpl         :-
            _M_IT_COUNT ZP_TEMP, CONS_MSG_LEN
            jsr         IO_WRITE
            _M_IT_FAIL_IF_C     'r'
            _M_IT_EXPECT_CNT    'r', CONS_MSG_LEN
            lda         ZP_TEMP
            ldx         #$7F
            jsr         IO_CTL
            _M_IT_FAIL_IF_NC    's', ERR_IO_BAD_REQ
            lda         ZP_TEMP
            jsr         IO_CLOSE
            _M_IT_FAIL_IF_C     's'

; A pipe between two tasks: a child writes IT_PIPE_SIZE bytes (more than the ring holds, so both sides
; wait for each other) and ends, which closes its write end; we read to end of file.  The child gets the
; write end as fd IO_MAX_FDS - 1 (IO_DUP2), and inherits it.
            jsr         IO_PIPE                             ; .A = read fd, .X = write fd
            _M_IT_FAIL_IF_C     't'
            sta         ZP_TEMP_VEC2                        ; (TASK_RUN uses ZP_TEMP and ZP_TEMP_VEC)
            stx         ZP_TEMP_2
            txa
            ldx         #IO_MAX_FDS - 1
            jsr         IO_DUP2
            _M_IT_FAIL_IF_C     't'
            lda         ZP_TEMP_2
            jsr         IO_CLOSE
            _M_IT_FAIL_IF_C     't'
            lda         #<IT_CHILD
            ldy         #>IT_CHILD
            ldx         #4                                  ; (ROM page 4)
            jsr         TASK_RUN
            _M_IT_FAIL_IF_C     'u'
            lda         #IO_MAX_FDS - 1                     ; Only the child's write end is left
            jsr         IO_CLOSE
            _M_IT_FAIL_IF_C     'u'
            stz         ZP_TEMP_VEC                         ; Bytes read
            stz         ZP_TEMP_VEC + 1

@pipe_read:
            _M_IT_COUNT ZP_TEMP_VEC2, IT_BUF_SIZE
            jsr         IO_READ
            _M_IT_FAIL_IF_C     'v'
            lda         ZP_IO_CNT
            ora         ZP_IO_CNT + 1
            beq         @pipe_eof
            lda         ZP_TEMP_VEC
            clc
            adc         ZP_IO_CNT
            sta         ZP_TEMP_VEC
            lda         ZP_TEMP_VEC + 1
            adc         ZP_IO_CNT + 1
            sta         ZP_TEMP_VEC + 1
            bra         @pipe_read

@pipe_eof:
            lda         ZP_TEMP_VEC
            cmp         #<IT_PIPE_SIZE
            bne         :+
            lda         ZP_TEMP_VEC + 1
            cmp         #>IT_PIPE_SIZE
            beq         :++
:
            lda         ZP_TEMP_VEC
            ldx         #'w'
            jmp         @fail
:
            lda         ZP_TEMP_VEC2
            jsr         IO_CLOSE
            _M_IT_FAIL_IF_C     'w'

; The namespace: a bind, a mount (the server gets the rest of the name), a loop of binds, and unmounting.
; Each step's entries go before the next, so it needs only 2 of the task's free entries (NS_ENTRIES: the
; shell's, /sd, has one already)
            _M_IT_NS    IO_BIND, S_NS_N, S_DEV_NULL
            _M_IT_FAIL_IF_C     'x'
            _M_IT_OPEN  S_NS_N, IO_MODE_RDWR
            _M_IT_FAIL_IF_C     'x'
            jsr         IO_CLOSE
            _M_IT_UNMOUNT       S_NS_N
            _M_IT_FAIL_IF_C     '1'
            _M_IT_NS    IO_MOUNT, S_NS_Z, S_ZERO
            _M_IT_FAIL_IF_C     'y'
            _M_IT_OPEN  S_NS_Z_SUB, IO_MODE_READ
            _M_IT_FAIL_IF_C     'y'
            jsr         IO_CLOSE
            _M_IT_UNMOUNT       S_NS_Z
            _M_IT_FAIL_IF_C     '1'
            _M_IT_NS    IO_BIND, S_NS_X, S_NS_Y
            _M_IT_FAIL_IF_C     'z'
            _M_IT_NS    IO_BIND, S_NS_Y, S_NS_X
            _M_IT_FAIL_IF_C     'z'
            _M_IT_OPEN  S_NS_X, IO_MODE_READ
            _M_IT_FAIL_IF_NC    'z', ERR_IO_NS_LOOP
            _M_IT_UNMOUNT       S_NS_X
            _M_IT_FAIL_IF_C     '1'
            _M_IT_UNMOUNT       S_NS_Y
            _M_IT_FAIL_IF_C     '1'
            _M_IT_OPEN  S_NS_N, IO_MODE_READ
            _M_IT_FAIL_IF_NC    '1', ERR_IO_NOT_FOUND

; Running out of fds: fill the free ones (ZP_TEMP_VEC: bit n = fd n was free), then close them again
            stz         ZP_TEMP_VEC
            stz         ZP_TEMP_VEC + 1
            ldy         #(IO_MAX_FDS - 1) * IO_FD_SIZE

@scan:
            lda         IO_FD_SERVER,Y
            cmp         #IO_FD_CLOSED                       ; C = 1: free
            rol         ZP_TEMP_VEC
            rol         ZP_TEMP_VEC + 1
            tya
            sec
            sbc         #IO_FD_SIZE
            tay
            bcs         @scan

@open_all:
            _M_IT_OPEN  S_DEV_NULL, IO_MODE_RDWR
            bcc         @open_all
            cmp         #ERR_IO_NO_FDS
            beq         :+
            ldx         #'n'
            jmp         @fail
:
            ldx         #0

@close_all:
            lsr         ZP_TEMP_VEC + 1
            ror         ZP_TEMP_VEC
            bcc         @next_fd
            txa
            jsr         IO_CLOSE
            _M_IT_FAIL_IF_C     'o'

@next_fd:
            inx
            cpx         #IO_MAX_FDS
            bne         @close_all

; /dev/ram: not for this task (only task 0's) ...
            _M_IT_OPEN  S_DEV_RAM, IO_MODE_READ
            _M_IT_FAIL_IF_NC    'r', ERR_IO_PERM
; ... but task 0 reads this task's RAM through it (IT_RAW: the buffer's first bytes, IT_RAW_BYTES) ...
            ldy         #2
:
            lda         IT_RAW_BYTES,Y
            sta         (ZP_TEMP_VEC3),Y
            dey
            bpl         :-
            lda         T_REGISTER
            ldx         ZP_TEMP_VEC3
            ldy         ZP_TEMP_VEC3 + 1
            jsr         IT_RAW
            _M_IT_FAIL_IF_C     's'
; ... a bank of this task's on a RAM module ...
            lda         #<8192
            ldy         #>8192
            ldx         #AI_PAGED
            jsr         MM_ALLOC
            _M_IT_FAIL_IF_C     't'
            sta         ZP_TEMP_VEC2                        ; (Its handle)
            jsr         MM_LOCK                             ; (Its bank at $8000: .X = what to put back)
            jsr         IT_RAW_FILL                         ; .A = the bank
            sta         ZP_TEMP_VEC2 + 1
            lda         ZP_TEMP_VEC2
            jsr         MM_UNLOCK
            ldx         ZP_TEMP_VEC2 + 1
            lda         T_REGISTER
            ora         #IT_RAW_MODULE
            jsr         IT_RAW
            _M_IT_FAIL_IF_C     'u'
            lda         ZP_TEMP_VEC2
            jsr         MM_FREE
            _M_IT_FAIL_IF_C     'u'
; ... and a shared bank
            lda         #<8192
            ldy         #>8192
            jsr         SH_ALLOC
            _M_IT_FAIL_IF_C     'v'
            sta         ZP_TEMP_VEC2
            jsr         SH_FP                               ; Its shared bank ID (its far pointer's sel)
            _M_IT_FAIL_IF_C     'v'
            lda         ZP_FP + FarPtr::sel
            sta         ZP_TEMP_VEC2 + 1
            lda         ZP_TEMP_VEC2
            jsr         SH_LOCK                             ; (Mapped: .X, .Y = what to put back)
            _M_IT_FAIL_IF_C     'v'
            phx
            phy
            jsr         IT_RAW_FILL
            ply
            plx
            jsr         SH_UNLOCK
            ldx         ZP_TEMP_VEC2 + 1
            lda         #IT_RAW_SHARED
            jsr         IT_RAW
            _M_IT_FAIL_IF_C     'w'
            lda         ZP_TEMP_VEC2
            jsr         SH_DETACH
            _M_IT_FAIL_IF_C     'w'
            lda         ZP_TEMP_VEC4
            jsr         MM_FREE
            _M_IT_FAIL_IF_C     'p'
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

; The /dev/ram step's bytes, and what IT_RAW reads (IT_RAW_T0's .A: the kind | the task)
IT_RAW_BYTES:   .byte   $5A, $A5, $3C
IT_RAW_TASK     = $00                                   ; Task RAM: .Y.X = the address
IT_RAW_MODULE   = $10                                   ; A bank on a RAM module: .X = its bank ID
IT_RAW_SHARED   = $20                                   ; A shared bank: .X = its shared bank ID

; IT_RAW_BYTES at $8000 (the bank mapped there).  OUT: .A = the bank ($00).  Preserves .X
IT_RAW_FILL:
            ldy         #2
:
            lda         IT_RAW_BYTES,Y
            sta         PAGED_RAM_BASE,Y
            dey
            bpl         :-
            lda         RAM_BANK_REG
            rts

; Have task 0 read 3 bytes through /dev/ram (IT_RAW_T0, by TASK_CALL), and check they're IT_RAW_BYTES.
; IN: as IT_RAW_T0.  OUT: C = 0; or C = 1, .A = the error, or the first byte read
IT_RAW:
            pha
            LOAD_ADDR   ::IT_RAW_T0, ZP_TC_VEC              ; (Its page 0 gate: thunks.s)
            stz         ZP_TC_TASK
            pla
            jsr         TASK_CALL
            bcs         @done
            cmp         IT_RAW_BYTES
            bne         @wrong
            cpx         IT_RAW_BYTES + 1
            bne         @wrong
            cpy         IT_RAW_BYTES + 2
            bne         @wrong
            clc
            rts

@wrong:
            sec

@done:
            rts

; The /dev/ram step's part in task 0 (TASK_CALL): 3 bytes, read through /dev/ram as only task 0 can.  WOZMON's line
; buffer (IN) is task 0's scratch: it never runs WOZMON.
; IN: .A = IT_RAW_TASK | the task, .Y.X = an address in its RAM; IT_RAW_MODULE | the task, .X = a bank ID of its;
;     or IT_RAW_SHARED, .X = a shared bank ID (the bank's first bytes).  OUT: C = 0, .A .X .Y = the bytes; or C = 1,
;     .A = the error
IT_RAW_T0:
            sta         IN + 16                             ; (What)
            stx         IN + 17
            sty         IN + 18
            ldx         #0
:
            lda         S_DEV_RAM,X
            sta         IN,X
            beq         :+
            inx
            bra         :-
:
            lda         #<IN
            ldy         #>IN
            ldx         #IO_MODE_READ
            jsr         IO_OPEN
            bcc         :+
            rts
:
            sta         IN + 19                             ; (The fd)
            stz         ZP_IO_OFS
            stz         ZP_IO_OFS + 3
            lda         IN + 16
            and         #$F0
            beq         @task_ram
            cmp         #IT_RAW_MODULE
            beq         @module
            lda         IN + 17                             ; A shared bank: $80000 + the ID * $2000
            asl
            asl
            asl
            asl
            asl
            sta         ZP_IO_OFS + 1
            lda         IN + 17
            lsr
            lsr
            lsr
            clc
            adc         #8
            sta         ZP_IO_OFS + 2
            bra         @seek

@task_ram:                                                  ; Task RAM: the task * $8000 + the address
            lda         IN + 17
            sta         ZP_IO_OFS
            lda         IN + 16
            lsr
            sta         ZP_IO_OFS + 2
            lda         IN + 18
            and         #$7F
            bcc         :+
            ora         #$80
:
            sta         ZP_IO_OFS + 1
            bra         @seek

@module:                                                    ; Module m's bank b, the task's: $280000 +
            lda         IN + 17                             ;   m * $200000 + the task * $20000 + b * $2000
            asl
            asl
            asl
            asl
            asl
            sta         ZP_IO_OFS + 1                       ; (b's low 3 bits)
            lda         IN + 17
            and         #$08
            lsr
            lsr
            lsr
            sta         IN + 23                             ; (b's top bit)
            lda         IN + 16
            and         #$0F
            asl
            ora         IN + 23
            sta         IN + 23
            lda         IN + 17                             ; m: m * 32 (in 64K units)
            lsr
            lsr
            lsr
            lsr
            pha
            asl
            asl
            asl
            asl
            asl
            ora         IN + 23
            clc
            adc         #$28
            sta         ZP_IO_OFS + 2
            pla
            lsr
            lsr
            lsr
            adc         #0
            sta         ZP_IO_OFS + 3

@seek:
            lda         IN + 19
            jsr         IO_SEEK
            bcs         @close
            LOAD_ADDR   (IN + 20), ZP_IO_BUF
            lda         #3
            sta         ZP_IO_CNT
            stz         ZP_IO_CNT + 1
            lda         IN + 19
            jsr         IO_READ
            bcs         @close
            lda         IN + 19
            jsr         IO_CLOSE
            lda         IN + 20
            ldx         IN + 21
            ldy         IN + 22
            clc

@done:
            rts

@close:
            pha
            lda         IN + 19
            jsr         IO_CLOSE
            pla
            sec
            rts

; ZP_HS_TEMP = path slot .X / IT_PATH_SIZE: ZP_TEMP_VEC3 (the buffer) + IT_BUF_SIZE + .X
IT_PATH_SLOT:
            txa
            clc
            adc         ZP_TEMP_VEC3
            sta         ZP_HS_TEMP
            lda         ZP_TEMP_VEC3 + 1
            adc         #0
            sta         ZP_HS_TEMP + 1
            lda         ZP_HS_TEMP
            clc
            adc         #<IT_BUF_SIZE
            sta         ZP_HS_TEMP
            lda         ZP_HS_TEMP + 1
            adc         #>IT_BUF_SIZE
            sta         ZP_HS_TEMP + 1
            rts

; The pipe test's child task: write IT_PIPE_SIZE bytes (any: this ROM page) to fd IO_MAX_FDS - 1, and end
IT_PIPE_SIZE    = 600

IT_CHILD:
            lda         #<RESET_ENTRY
            sta         ZP_IO_BUF
            lda         #>RESET_ENTRY
            sta         ZP_IO_BUF + 1
            lda         #<IT_PIPE_SIZE
            sta         ZP_IO_CNT
            lda         #>IT_PIPE_SIZE
            sta         ZP_IO_CNT + 1
            lda         #IO_MAX_FDS - 1
            jmp         IO_WRITE
