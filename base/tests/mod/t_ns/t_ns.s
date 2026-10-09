; ****************************************************************************
; t_ns - namespaces (phase 2.5), run as init with t_srv (#T) and t_child: init's namespace, empty at first; a bind
; at /; names cleaned and relative to the current directory (CHDIR, GETCWD); unions, in order, and a union
; directory read whole; a mount point bound elsewhere (all its members), and a name under one (what it is now);
; UNMOUNT; MOUNT; a child's namespace, shared (and copied when it changes it) or new and empty; room for more than
; eight namespaces at once, and more than 128 mount entries.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "testlib.inc"

            HYX2_PROGRAM "t_ns", main

.zeropage
fd:         .res        1
child:      .res        1
total:      .res        2

.bss
buf:        .res        512

.code

; Open path (a label) for reading.  OUT: .A = the fd, C
.macro OPEN_  path
            LDR         r0, path
            lda         #O_READ
            jsr         OPEN
.endmacro

; Open path, read up to 64 bytes into buf, close.  OUT: .A = the count read, C (the open's error)
.macro CAT_   path
            LDR         r0, path
            jsr         cat
.endmacro

; Bind new at old with flags.  OUT: C, .A
.macro BIND_  new, old, flags
            LDR         r0, new
            LDR         r1, old
            lda         #flags
            jsr         BIND
.endmacro

; SPAWN "t_child" with the arguments at label and flags, and WAIT for it.  OUT: .A = its exit code
.macro CHILD_ label, flags
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
            LDR         r0, s_out                           ; Fds 0-2: #T/out (# names need no namespace)
            lda         #O_RDWR
            jsr         OPEN
            lda         #0
            jsr         DUP
            lda         #0
            jsr         DUP

; ---- An empty namespace, then #T at /
            OPEN_       s_hello
            EXPECT_ERR  E_NOENT, "init's namespace is empty: /hello is E_NOENT"
            BIND_       s_t, s_root, MREPL
            EXPECT_OK   "BIND #T /"
            CAT_        s_hello
            EXPECT_A    13, "/hello: hello, world (13 bytes)"
            lda         buf
            EXPECT_A    'h', "/hello: h first"
            CAT_        s_messy
            EXPECT_A    6, "//sub/./x/../inner: cleaned, /sub/inner (6 bytes)"

; ---- The current directory
            LDR         r0, s_sub
            jsr         CHDIR
            EXPECT_OK   "CHDIR /sub"
            LDR         r0, buf
            jsr         GETCWD
            EXPECT_A    4, "GETCWD: /sub (4)"
            CAT_        s_inner_rel
            EXPECT_A    6, "inner, relative: /sub/inner"
            LDR         r0, s_dotdot
            jsr         CHDIR
            EXPECT_OK   "CHDIR .."
            LDR         r0, buf
            jsr         GETCWD
            EXPECT_A    1, "GETCWD: / (1)"
            LDR         r0, s_hello
            jsr         CHDIR
            EXPECT_ERR  E_NOTDIR, "CHDIR /hello: E_NOTDIR"
            LDR         r0, s_tsub
            jsr         CHDIR
            EXPECT_OK   "CHDIR #T/sub (a # name)"
            CAT_        s_inner_rel
            EXPECT_A    6, "inner, relative to #T/sub"
            LDR         r0, s_root
            jsr         CHDIR

; ---- A union: #T/sub then #T at /u
            BIND_       s_tsub, s_u, MREPL
            EXPECT_OK   "BIND #T/sub /u"
            BIND_       s_t, s_u, MAFTER
            EXPECT_OK   "BIND -a #T /u"
            CAT_        s_u_inner
            EXPECT_A    6, "/u/inner: the first member's"
            CAT_        s_u_hello
            EXPECT_A    13, "/u/hello: the second member's (not in the first)"
            LDR         r0, s_u
            jsr         first
            EXPECT_A    'i', "/u read: its first record is #T/sub's inner"
            OPEN_       s_u                                 ; Read whole: 1 record, then 8
            sta         fd
            EXPECT_OK   "OPEN /u, a union directory"
            stz         total
            stz         total + 1
@read:
            LDR         r0, buf
            LDR         r1, 512
            lda         fd
            jsr         READ
            bcs         @read_end
            sta         r2
            stx         r2 + 1
            ora         r2 + 1
            beq         @read_end
            clc
            lda         total
            adc         r2
            sta         total
            lda         total + 1
            adc         r2 + 1
            sta         total + 1
            bra         @read

@read_end:
            lda         fd
            jsr         CLOSE
            lda         total + 1
            EXPECT_A    >(9 * SR_SIZE), "the union directory: 9 records, both members' (high byte)"
            lda         total
            EXPECT_A    <(9 * SR_SIZE), "the union directory: 9 records (576 bytes)"

; ---- Before, not after: #T first at /v
            BIND_       s_tsub, s_v, MREPL
            BIND_       s_t, s_v, MBEFORE
            EXPECT_OK   "BIND -b #T /v"
            LDR         r0, s_v
            jsr         first
            EXPECT_A    'h', "/v read: its first record is #T's hello (#T first)"
            CAT_        s_v_inner
            EXPECT_A    6, "/v/inner: not in #T, found in #T/sub"

; ---- A mount point bound elsewhere: all its members; a name under one: what it is now
            BIND_       s_u, s_x, MREPL
            EXPECT_OK   "BIND /u /x (a union: both members)"
            CAT_        s_x_hello
            EXPECT_A    13, "/x/hello, in its second member"
            BIND_       s_u_sub, s_y, MREPL
            EXPECT_OK   "BIND /u/sub /y (#T's sub, found in the second member)"
            CAT_        s_y_inner
            EXPECT_A    6, "/y/inner"

; ---- UNMOUNT
            LDR         r0, s_tsub
            LDR         r1, s_u
            jsr         UNMOUNT
            EXPECT_OK   "UNMOUNT #T/sub /u"
            OPEN_       s_u_inner
            EXPECT_ERR  E_NOENT, "/u/inner: gone"
            CAT_        s_u_hello
            EXPECT_A    13, "/u/hello: still there"
            stz         r0
            stz         r0 + 1
            LDR         r1, s_u
            jsr         UNMOUNT
            EXPECT_OK   "UNMOUNT /u: all of it"
            OPEN_       s_u_hello
            EXPECT_ERR  E_NOENT, "/u/hello: gone (/ is #T, which has no u)"
            LDR         r1, s_u
            jsr         UNMOUNT
            EXPECT_ERR  E_NOENT, "UNMOUNT /u again: E_NOENT"

; ---- MOUNT
            LDR         r0, s_spec
            LDR         r1, s_m
            ldx         #'T'
            lda         #MREPL
            jsr         MOUNT
            EXPECT_OK   "MOUNT #T with a spec at /m"
            CAT_        s_m_hello
            EXPECT_A    13, "/m/hello"
            stz         r0
            stz         r0 + 1
            LDR         r1, s_m
            ldx         #'Z'
            lda         #MREPL
            jsr         MOUNT
            EXPECT_ERR  E_NODEV, "MOUNT #Z: E_NODEV"

; ---- A child's namespace: its parent's (a bind of its own copies it first), or a new, empty one
            CHILD_      s_h, 0
            EXPECT_A    0, "a child opens /hello: its parent's namespace"
            CHILD_      s_h, SPAWN_NEWNS
            EXPECT_A    $E0 | E_NOENT, "with SPAWN_NEWNS, an empty one: E_NOENT"
            CHILD_      s_m_op, 0
            EXPECT_A    0, "a child binds #T/sub at /, and opens /inner"
            CAT_        s_hello
            EXPECT_A    13, "its parent's / is still #T: the child's bind was in its own copy"

; ---- Room: NS_MAX namespaces (16), MT_MAX entries (255) in all.  Eight children, each with a namespace of its own
; (SPAWN_NEWNS), pausing till woken: nine at once with this one's
            stz         total
@spawn:
            LDR         r0, s_child
            LDR         r1, s_p
            lda         #SPAWN_NEWNS
            jsr         SPAWN
            bcs         @spawned
            ldx         total
            sta         buf,X
            inc         total
            lda         total
            cmp         #8
            bne         @spawn
@spawned:
            lda         total
            EXPECT_A    8, "eight children, each with a namespace of its own: nine namespaces at once"
@wake:
            ldx         total
            beq         @woken
            dec         total
            lda         buf - 1,X
            jsr         WAKE
            stz         r0
            stz         r0 + 1
            ldx         total
            lda         buf,X
            jsr         WAIT
            bra         @wake
@woken:
            stz         total                               ; 100 members at /w and 100 at /w2: 200 entries more
@member:                                                    ;   (a union has 127 members after its first at most:
            lda         total                               ;   their order, 128-254)
            cmp         #100
            bcs         :+
            BIND_       s_t, s_w, MAFTER
            bra         :++
:
            BIND_       s_t, s_w2, MAFTER
:
            bcs         @full
            inc         total
            lda         total
            cmp         #200
            bne         @member
@full:
            lda         total
            EXPECT_A    200, "100 members at /w, 100 at /w2: the mount entries' room past 128"
            stz         r0
            stz         r0 + 1
            LDR         r1, s_w
            jsr         UNMOUNT
            EXPECT_OK   "UNMOUNT /w: all 100"
            stz         r0
            stz         r0 + 1
            LDR         r1, s_w2
            jsr         UNMOUNT
            EXPECT_OK   "UNMOUNT /w2: all 100"

            DONE        "t_ns"

; Directory r0's first record's name's first character (or C = 1, .A = the error)
first:
            jsr         cat
            bcs         :+
            lda         buf + SR_NAME
:
            rts

; Path r0 opened, up to 64 bytes read into buf, closed.  OUT: .A = the count read; or C = 1, .A = the error
cat:
            lda         #O_READ
            jsr         OPEN
            bcs         @done
            sta         fd
            LDR         r0, buf
            LDR         r1, 64
            lda         fd
            jsr         READ
            pha
            lda         fd
            jsr         CLOSE
            pla
            clc
@done:
            rts

.rodata
s_out:      .byte       "#T/out", 0
s_t:        .byte       "#T", 0
s_tsub:     .byte       "#T/sub", 0
s_root:     .byte       "/", 0
s_hello:    .byte       "/hello", 0
s_messy:    .byte       "//sub/./x/../inner", 0
s_sub:      .byte       "/sub", 0
s_inner_rel: .byte      "inner", 0
s_dotdot:   .byte       "..", 0
s_u:        .byte       "/u", 0
s_u_inner:  .byte       "/u/inner", 0
s_u_hello:  .byte       "/u/hello", 0
s_u_sub:    .byte       "/u/sub", 0
s_v:        .byte       "/v", 0
s_v_inner:  .byte       "/v/inner", 0
s_x:        .byte       "/x", 0
s_x_hello:  .byte       "/x/hello", 0
s_y:        .byte       "/y", 0
s_y_inner:  .byte       "/y/inner", 0
s_m:        .byte       "/m", 0
s_m_hello:  .byte       "/m/hello", 0
s_spec:     .byte       "abc", 0
s_child:    .byte       "#m/t_child", 0
s_h:        .byte       "h", 0, 0
s_m_op:     .byte       "m", 0, 0
s_p:        .byte       "p", 0, 0
s_w:        .byte       "/w", 0
s_w2:       .byte       "/w2", 0
