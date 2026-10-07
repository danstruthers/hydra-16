; ****************************************************************************
; t_env - environments (phase 4.2: kernel/env.s, and kdev's #e), run as init (its fds 0-2 closed: its lines go out
; on the bring-up console; the fds it opens are moved to 5 on), with t_child.  The calls: ENV_PUT (made, set at an
; offset, cut), ENV_GET (parts, offsets, past the end), ENV_NAME, ENV_DEL, their errors (no such variable, bad
; names, a full environment: ENV_MAX, no such task); TASKREAD's TR_ENV (the variables that fit ENV_SIZE) and TR_ENVAT
; (a part); a child's copy (of a full one too), its changes its own, and SPAWN_NOENV's empty one.  #e:
; the directory, a variable read in parts, made, written, emptied (O_TRUNC), appended to, removed, its length in its
; stat record, a long one (in kdev's parts), and its errors.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "testlib.inc"

            HYX2_PROGRAM "t_env", main

.zeropage
fd:         .res        1
child:      .res        1
len:        .res        2

.bss
buf:        .res        512
rec:        .res        SR_SIZE

.code

; ENV_PUT name (a label), the bytes at label (count), at offset, in this task's
.macro PUT_     name, label, count, offset
            LDR         r0, name
            LDR         r1, label
            LDR         r2, count
            LDR         r3, offset
            lda         #$FF
            jsr         ENV_PUT
.endmacro

; ENV_GET name, into buf (size), from offset, in task t
.macro GET_     t, name, size, offset
            LDR         r0, name
            LDR         r1, buf
            LDR         r2, size
            LDR         r3, offset
            lda         #t
            jsr         ENV_GET
.endmacro

; OPEN path with mode, the fd moved to 5 (fd 1 stays closed).  OUT: .A = the fd, C
.macro OPEN_    path, mode
            LDR         r0, path
            lda         #mode
            jsr         open5
.endmacro

; SPAWN t_child with the arguments at label, with flags; its code
.macro CHILD_   label, flags
            LDR         r0, s_child
            LDR         r1, label
            lda         #flags
            jsr         SPAWN
            sta         child
            stz         r0
            stz         r0 + 1
            lda         child
            jsr         WAIT
            txa
.endmacro

main:
            stz         T_FAILS

; ---- The calls
            PUT_        s_x, s_hello, 5, 0
            EXPECT_OK   "ENV_PUT x hello"
            GET_        $FF, s_x, 16, 0
            EXPECT_A    5, "ENV_GET x: 5 bytes"
            lda         buf + 4
            EXPECT_A    'o', "hello"
            GET_        $FF, s_x, 2, 2
            EXPECT_A    3, "ENV_GET x from 2, 2 of them: the 3 there are from 2"
            lda         buf + 1
            EXPECT_A    'l', "and ll in the buffer"
            GET_        $FF, s_x, 16, 9
            EXPECT_A    0, "ENV_GET past the end: 0"
            PUT_        s_x, s_world, 6, 5
            GET_        $FF, s_x, 16, 0
            EXPECT_A    11, "ENV_PUT at 5: hello world (11)"
            lda         buf + 10
            EXPECT_A    'd', "its last byte"
            PUT_        s_x, s_j, 1, 0
            GET_        $FF, s_x, 16, 0
            EXPECT_A    1, "ENV_PUT at 0: the value cut first (J)"
            PUT_        s_y, s_two, 1, 0
            PUT_        s_zz, s_33, 2, 0
            LDR         r0, buf
            lda         #$FF
            ldx         #2
            jsr         ENV_NAME
            EXPECT_A    2, "ENV_NAME 2: zz's length"
            lda         buf + 1
            EXPECT_A    'z', "ENV_NAME 2: zz, the third made"
            LDR         r0, buf
            lda         #$FF
            ldx         #3
            jsr         ENV_NAME
            EXPECT_ERR  E_NOENT, "ENV_NAME 3: E_NOENT"
            LDR         r0, s_y
            lda         #$FF
            jsr         ENV_DEL
            EXPECT_OK   "ENV_DEL y"
            LDR         r0, buf
            lda         #$FF
            ldx         #1
            jsr         ENV_NAME
            lda         buf
            EXPECT_A    'z', "ENV_NAME 1: zz now (y gone, the ones after it moved down)"
            GET_        $FF, s_zz, 16, 0
            lda         buf + 1
            EXPECT_A    '3', "zz's value, moved with it"
            GET_        $FF, s_y, 16, 0
            EXPECT_ERR  E_NOENT, "ENV_GET y: E_NOENT"
            PUT_        s_x, s_j, 1, 5
            EXPECT_ERR  E_INVAL, "ENV_PUT past the value's end: E_INVAL"
            PUT_        s_y, s_j, 1, 3
            EXPECT_ERR  E_NOENT, "ENV_PUT at an offset, no such variable: E_NOENT"
            GET_        $FF, s_empty, 16, 0
            EXPECT_ERR  E_INVAL, "an empty name: E_INVAL"
            GET_        $FF, s_slash, 16, 0
            EXPECT_ERR  E_INVAL, "a name with a /: E_INVAL"
            GET_        $FF, s_long, 16, 0
            EXPECT_ERR  E_NAMETOOLONG, "a name of 32: E_NAMETOOLONG"
            GET_        16, s_x, 16, 0
            EXPECT_ERR  E_SRCH, "task 16: E_SRCH"
            GET_        9, s_x, 16, 0
            EXPECT_ERR  E_SRCH, "a task not in use: E_SRCH"
            PUT_        s_big, buf, 8170, 0                 ; (13 bytes in use: 8189 with it)
            EXPECT_OK   "ENV_PUT big (8170 bytes): it fits"
            GET_        $FF, s_big, 16, 8160
            EXPECT_A    10, "ENV_GET big from 8160: its last 10 bytes"
            lda         buf + 9
            eor         buf + 8169
            EXPECT_A    0, "and its last, as it was put"
            PUT_        s_big2, buf, 10, 0
            EXPECT_ERR  E_NOMEM, "and another: E_NOMEM (ENV_MAX: 8192 bytes an environment)"
            PUT_        s_x, buf, 100, 1
            EXPECT_ERR  E_NOMEM, "x made longer: E_NOMEM"
            lda         #$55
            sta         buf + 12
            LDR         r0, buf
            lda         #$FF
            ldx         #TR_ENV
            jsr         TASKREAD
            lda         buf + 12
            EXPECT_A    0, "TASKREAD's TR_ENV: the variables that fit ENV_SIZE, whole (x, zz), then a 0"
            lda         #$55
            sta         buf + 8
            LDR         r0, buf
            LDR         r1, 16
            LDR         r2, 8180
            lda         #$FF
            ldx         #TR_ENVAT
            jsr         TASKREAD
            stx         len + 1
            EXPECT_A    <8189, "TASKREAD's TR_ENVAT: the bytes in use (8189) ..."
            lda         len + 1
            EXPECT_A    >8189, "  (8189's high byte)"
            lda         buf + 8
            EXPECT_A    0, "  and from 8180, the last 9 of them: its 0 last"
            LDR         r0, s_big
            lda         #$FF
            jsr         ENV_DEL

; ---- A child's copy
            PUT_        s_x, s_q, 1, 0
            CHILD_      s_v, 0
            EXPECT_A    'Q', "a child's environment: a copy (x = Q)"
            GET_        $FF, s_x, 16, 0
            lda         buf
            EXPECT_A    'Q', "its x = C its own: this one's still Q"
            CHILD_      s_v, SPAWN_NOENV
            EXPECT_A    $EE, "SPAWN_NOENV: an empty one (no x)"
            LDR         r0, s_x                             ; x last in a full environment (zz, big, x): its
            lda         #$FF                                ;   copy is a page at a time, from one bank to another
            jsr         ENV_DEL
            PUT_        s_big, buf, 8000, 0
            PUT_        s_x, s_q, 1, 0
            CHILD_      s_v, 0
            EXPECT_A    'Q', "a child's copy of a full environment: x, in its last page, is Q there too"
            LDR         r0, s_big                           ; (As before: x, then zz)
            lda         #$FF
            jsr         ENV_DEL
            LDR         r0, s_zz
            lda         #$FF
            jsr         ENV_DEL
            PUT_        s_zz, s_33, 2, 0

; ---- /proc/N/env of an environment past ENV_SIZE: kdev makes it a read at a time (x=Q, zz=33, big=1500 As)
            ldx         #0
            lda         #'A'
:
            sta         buf,X
            sta         buf + 256,X
            inx
            bne         :-
            PUT_        s_big, buf, 500, 0
            PUT_        s_big, buf, 500, 500
            PUT_        s_big, buf, 500, 1000
            OPEN_       s_penv, O_READ
            sta         fd
            EXPECT_OK   "OPEN #p/1/env"
            stz         len
            stz         len + 1
@penv:
            LDR         r0, buf
            LDR         r1, 512
            lda         fd
            jsr         READ
            bcs         @penvend                            ; (An error: what came so far)
            sta         r2
            stx         r2 + 1
            ora         r2 + 1
            beq         @penvend                            ; (0: its end)
            clc
            lda         len
            adc         r2
            sta         len
            lda         len + 1
            adc         r2 + 1
            sta         len + 1
            bra         @penv

@penvend:
            lda         len
            EXPECT_A    <1515, "#p/1/env, 512 at a time: 1515 bytes (x=Q, zz=33, big=1500 As) ..."
            lda         len + 1
            EXPECT_A    >1515, "  (1515's high byte)"
            lda         fd
            jsr         CLOSE
            LDR         r0, s_big
            lda         #$FF
            jsr         ENV_DEL

; ---- #e
            OPEN_       s_he, O_READ
            sta         fd
            EXPECT_OK   "OPEN #e"
            LDR         r0, buf
            LDR         r1, 512
            lda         fd
            jsr         READ
            EXPECT_A    2 * SR_SIZE, "#e: a record a variable (x, zz)"
            lda         buf + SR_SIZE + SR_NAME
            EXPECT_A    'z', "zz second"
            lda         fd
            jsr         CLOSE
            OPEN_       s_hex, O_READ
            sta         fd
            LDR         r0, buf
            LDR         r1, 16
            lda         fd
            jsr         READ
            EXPECT_A    1, "#e/x reads as Q (1 byte)"
            lda         buf
            EXPECT_A    'Q', "Q"
            lda         fd
            jsr         CLOSE
            LDR         r0, s_hnew                          ; Made, written
            lda         #O_WRITE
            ldx         #0
            jsr         CREATE
            ldx         #5
            jsr         move
            sta         fd
            EXPECT_OK   "CREATE #e/new"
            LDR         r0, s_abc
            LDR         r1, 3
            lda         fd
            jsr         WRITE
            EXPECT_A    3, "write abc"
            lda         fd
            jsr         CLOSE
            GET_        $FF, s_new, 16, 0
            EXPECT_A    3, "new: abc (ENV_GET)"
            OPEN_       s_hnew, O_WRITE | O_TRUNC           ; Emptied, written
            sta         fd
            LDR         r0, s_de
            LDR         r1, 2
            lda         fd
            jsr         WRITE
            LDR         r0, s_z                             ; Appended: at its offset, 2
            LDR         r1, 1
            lda         fd
            jsr         WRITE
            lda         fd
            jsr         CLOSE
            LDR         r0, s_hnew
            LDR         r1, rec
            jsr         STAT
            lda         rec + SR_LENGTH
            EXPECT_A    3, "O_TRUNC, de, then Z: its stat record says 3"
            OPEN_       s_hnew, O_READ                      ; Read in parts
            sta         fd
            LDR         r0, buf
            LDR         r1, 2
            lda         fd
            jsr         READ
            EXPECT_A    2, "read 2: de"
            LDR         r0, buf + 2
            LDR         r1, 2
            lda         fd
            jsr         READ
            EXPECT_A    1, "read 2 more: Z, all there is"
            lda         buf + 2
            EXPECT_A    'Z', "Z"
            LDR         r0, buf
            LDR         r1, 2
            lda         fd
            jsr         READ
            EXPECT_A    0, "then the end"
            lda         fd
            jsr         CLOSE
            LDR         r0, s_hnew
            jsr         REMOVE
            EXPECT_OK   "REMOVE #e/new"
            GET_        $FF, s_new, 16, 0
            EXPECT_ERR  E_NOENT, "new: gone"
            ldx         #0                                  ; A long one: 300 bytes, in two writes and a read
:
            txa
            sta         buf,X
            sta         buf + 256,X
            inx
            bne         :-
            OPEN_       s_hlong, O_RDWR
            EXPECT_ERR  E_NOENT, "OPEN #e/long: E_NOENT (not made yet)"
            LDR         r0, s_hlong
            lda         #O_RDWR
            ldx         #0
            jsr         CREATE
            ldx         #5
            jsr         move
            sta         fd
            LDR         r0, buf
            LDR         r1, 300
            lda         fd
            jsr         WRITE
            sta         len
            stx         len + 1
            lda         len + 1
            EXPECT_A    >300, "300 bytes written (high byte)"
            lda         len
            EXPECT_A    <300, "300 bytes written"
            lda         fd
            jsr         CLOSE
            ldx         #0
:
            stz         buf,X
            stz         buf + 256,X
            inx
            bne         :-
            OPEN_       s_hlong, O_READ
            sta         fd
            LDR         r0, buf
            LDR         r1, 512
            lda         fd
            jsr         READ
            sta         len
            stx         len + 1
            lda         len
            EXPECT_A    <300, "read back: 300 bytes"
            lda         buf + 299
            EXPECT_A    <299, "its last byte"
            lda         fd
            jsr         CLOSE
            OPEN_       s_hnone, O_READ
            EXPECT_ERR  E_NOENT, "OPEN #e/nothing: E_NOENT"
            LDR         r0, s_hdir
            lda         #O_READ
            ldx         #DM_DIR
            jsr         CREATE
            EXPECT_ERR  E_PERM, "CREATE a directory in #e: E_PERM"
            OPEN_       s_he, O_WRITE
            EXPECT_ERR  E_ISDIR, "OPEN #e for writing: E_ISDIR"
            DONE        "t_env"

; OPEN r0 with .A, the fd moved to 5.  OUT: .A = 5; or C = 1, .A = the error
open5:
            jsr         OPEN
            bcs         @done
            ldx         #5
            jsr         move
            clc
@done:
            rts

; Fd .A moved to fd .X.  OUT: .A = .X
move:
            phx
            pha
            jsr         DUP2
            pla
            jsr         CLOSE
            pla
            rts

.rodata
s_child:    .byte       "#m/t_child", 0
s_v:        .byte       "v", 0, 0
s_x:        .byte       "x", 0
s_y:        .byte       "y", 0
s_zz:       .byte       "zz", 0
s_new:      .byte       "new", 0
s_big:      .byte       "big", 0
s_big2:     .byte       "big2", 0
s_empty:    .byte       0
s_slash:    .byte       "a/b", 0
s_long:     .byte       "abcdefghijklmnopqrstuvwxyz012345", 0
s_he:       .byte       "#e", 0
s_hex:      .byte       "#e/x", 0
s_penv:     .byte       "#p/1/env", 0
s_hnew:     .byte       "#e/new", 0
s_hlong:    .byte       "#e/long", 0
s_hnone:    .byte       "#e/nothing", 0
s_hdir:     .byte       "#e/d", 0
s_hello:    .byte       "hello"
s_world:    .byte       " world"
s_j:        .byte       "J"
s_q:        .byte       "Q"
s_two:      .byte       "2"
s_33:       .byte       "33"
s_abc:      .byte       "abc"
s_de:       .byte       "de"
s_z:        .byte       "Z"
