' benchasm.bas - the benchmarks in BASIC's inline assembly: bench.bas's twenty (the same algorithms, sizes and
' results), each one's work in an ASM block, called with CALL ASM; the program around them is bench.bas's.  basic
' benchasm.bas [reps [q|f [name ...]]]: each run reps times (1), a line for each, "bench basm NAME RESULT TICKS REPS".
' sim/bench.js runs it beside bench.bas, bench.hl and bench.fs; each benchmark's reps are a loop of their own, so a
' rep's time is its code's and the CALL ASM's (bench.bas's SELECT CASE in the loop, its strings compared, would be
' more than some of them).  The code works in 16 bits (every value of these sizes fits): a size in .A (its low byte)
' and .X (its high), the result back in .A and .X; r0-r15 its zero page; recursion where the others recurse (fib, tak,
' ack, queens: the 6502's stack), a subroutine called where they call a function (calls, mapf, fold), a multiplication
' by a routine of shifts and adds (hash, sort, matrix, mapf), the arrays bytes in the blocks' .bss, the text in its
' .rodata, numbers written out by division by 10 (digits).

CONST MAPN = 40, FOLDN = 200, EACHN = 200
reps = VAL(ARG$(1)): IF reps < 1 THEN reps = 1
quick = ARG$(2) = "q"
DO
    READ nm$, nf, nq
    IF nm$ = "done" THEN EXIT DO
    IF chosen(nm$) THEN
        n = nf: IF quick THEN n = nq
        nl = n AND 255: nh = n \ 256
        t0 = ticks
        SELECT CASE nm$                 ' (Chosen once: each its own loop of reps)
            CASE "calls":   FOR rr = 1 TO reps: CALL ASM calls, nl, nh: NEXT
            CASE "fib":     FOR rr = 1 TO reps: CALL ASM fibr, nl: NEXT
            CASE "tak":     FOR rr = 1 TO reps: CALL ASM taks, nl: NEXT
            CASE "ack":     FOR rr = 1 TO reps: CALL ASM acks, nl: NEXT
            CASE "loop":    FOR rr = 1 TO reps: CALL ASM countup, nl, nh: NEXT
            CASE "while":   FOR rr = 1 TO reps: CALL ASM whilesum, nl, nh: NEXT
            CASE "dotimes": FOR rr = 1 TO reps: CALL ASM forsum, nl, nh: NEXT
            CASE "nested":  FOR rr = 1 TO reps: CALL ASM nested, nl: NEXT
            CASE "gcd":     FOR rr = 1 TO reps: CALL ASM gcdsum, nl: NEXT
            CASE "collatz": FOR rr = 1 TO reps: CALL ASM collatz, nl: NEXT
            CASE "hash":    FOR rr = 1 TO reps: CALL ASM hash, nl, nh: NEXT
            CASE "sieve":   FOR rr = 1 TO reps: CALL ASM sieve, nl, nh: NEXT
            CASE "sort":    FOR rr = 1 TO reps: CALL ASM sortb, nl: NEXT
            CASE "matrix":  FOR rr = 1 TO reps: CALL ASM matrix, nl: NEXT
            CASE "queens":  FOR rr = 1 TO reps: CALL ASM queens, nl: NEXT
            CASE "mapf":    FOR rr = 1 TO reps: CALL ASM mapf, nl: NEXT
            CASE "fold":    FOR rr = 1 TO reps: CALL ASM fold, nl: NEXT
            CASE "each":    FOR rr = 1 TO reps: CALL ASM eachsum, nl: NEXT
            CASE "chars":   FOR rr = 1 TO reps: CALL ASM chars, nl: NEXT
            CASE "digits":  FOR rr = 1 TO reps: CALL ASM digitslen, nl, nh: NEXT
        END SELECT
        RREG rl, rh: r = rh * 256 + rl
        d = (ticks - t0) MOD 32768: IF d < 0 THEN d = d + 32768
        PRINT "bench basm "; nm$; STR$(r); STR$(d); STR$(reps)
    END IF
LOOP
PRINT "bench basm done"
' Each benchmark's name, its size, its quick size (mapf's, fold's and each's: their reps): bench.bas's
DATA calls, 2000, 500, fib, 16, 12, tak, 6, 2, ack, 8, 2, loop, 4000, 1000, while, 4000, 1000
DATA dotimes, 4000, 1000, nested, 60, 30, gcd, 20, 10, collatz, 60, 30, hash, 2000, 500, sieve, 1024, 512
DATA sort, 100, 40, matrix, 10, 6, queens, 7, 6, mapf, 20, 5, fold, 10, 3, each, 10, 3, chars, 40, 10
DATA digits, 1000, 300, done, 0, 0

' Is a benchmark chosen: named after q|f, or none named?
FUNCTION chosen (nm$)
    IF ARG$(3) = "" THEN RETURN -1
    FOR a = 3 TO 30
        IF ARG$(a) = "" THEN EXIT FOR
        IF ARG$(a) = nm$ THEN RETURN -1
    NEXT
    RETURN 0
END FUNCTION

' The clock: the ticks (200 a second, 16 bits)
FUNCTION ticks
    SYS "TICKS": RREG l, h
    ticks = h * 256 + l
END FUNCTION

' ---- The code's zero page (r0-r15), its helpers

ASM
zn      = $02                   ; n
zi      = $04                   ; i
zj      = $06                   ; j
zk      = $08                   ; k (a count of reps)
zr      = $0A                   ; the result
za      = $0C                   ; (scratch)
zb      = $0E
zt      = $10
zs      = $12
zp      = $14                   ; (a pointer)
zv      = $16                   ; (a value kept)
zm1     = $18                   ; (mul8's)
zm2     = $19

; 16 bits less one, at v
.macro  dec16 v
        lda v
        bne :+
        dec v + 1
:       dec v
.endmacro

; C = 0 if the 16 bits at v are below those at w
.macro  below16 v, w
        lda v
        cmp w
        lda v + 1
        sbc w + 1
.endmacro

; .A times .Y (8 bits each): the product in .A (its low byte) and .X (its high), by shifts and adds
mul8:   sta zm1
        sty zm2
        lda #0
        ldx #8
        lsr zm1
@bit:   bcc @no
        clc
        adc zm2
@no:    ror a
        ror zm1
        dex
        bne @bit
        tax
        lda zm1
        rts

; The result (zr) in .A and .X
result: lda zr
        ldx zr + 1
        rts
END ASM

' ---- Calls

ASM
; calls: n calls of a subroutine of two arguments (.A and .X its first, .Y its second)
calls:  sta zn
        stx zn + 1
        stz zr
        stz zr + 1
@loop:  lda zn
        ora zn + 1
        beq @done
        lda zr
        ldx zr + 1
        ldy #1
        jsr add2
        sta zr
        stx zr + 1
        dec16 zn
        bra @loop
@done:  jmp result
add2:   sty zt
        clc
        adc zt
        bcc :+
        inx
:       rts

; fib: Fibonacci, recursively (n in .A)
fibr:   cmp #2
        bcs @rec
        ldx #0
        rts
@rec:   pha                     ; (n)
        dec a
        jsr fibr                ; fib(n - 1)
        tay
        pla
        phx                     ; (kept)
        phy
        sec
        sbc #2
        jsr fibr                ; fib(n - 2)
        sta zt
        stx zt + 1
        pla
        clc
        adc zt
        sta zt
        pla
        adc zt + 1
        tax
        lda zt
        rts

; tak: Takeuchi's function (x in .A, y in .X, z in .Y): tak(9, 6, 3), k times, summed
tak:    stx zt
        cmp zt
        beq @z                  ; (y < x: else z)
        bcc @z
        pha                     ; (x, y, z kept)
        phx
        phy
        dec a                   ; tak(x - 1, y, z)
        jsr tak
        pha
        tsx                     ; ($101: it; $102: z; $103: y; $104: x)
        ldy $104,x              ; tak(y - 1, z, x)
        lda $102,x
        pha
        lda $103,x
        dec a
        plx
        jsr tak
        pha
        tsx                     ; ($101: it; $102: the first; $103: z; $104: y; $105: x)
        ldy $104,x              ; tak(z - 1, x, y)
        lda $105,x
        pha
        lda $103,x
        dec a
        plx
        jsr tak
        tay                     ; tak(the first, the second, the third)
        plx
        pla
        jsr tak
        sta zt
        pla
        pla
        pla
        lda zt
        rts
@z:     tya
        rts
taks:   sta zk
        stz zr
        stz zr + 1
@rep:   lda #9
        ldx #6
        ldy #3
        jsr tak
        clc
        adc zr
        sta zr
        bcc :+
        inc zr + 1
:       dec zk
        bne @rep
        jmp result

; ack: Ackermann's function (m in .A, n in .X), a tail call in each: ack(2, 9), k times, summed
ack:    cmp #0
        bne @m
        inx
        txa
        rts
@m:     cpx #0
        bne @both
        dec a                   ; ack(m - 1, 1)
        ldx #1
        jmp ack
@both:  pha                     ; ack(m - 1, ack(m, n - 1))
        dex
        jsr ack
        tax
        pla
        dec a
        jmp ack
acks:   sta zk
        stz zr
        stz zr + 1
@rep:   lda #2
        ldx #9
        jsr ack
        clc
        adc zr
        sta zr
        bcc :+
        inc zr + 1
:       dec zk
        bne @rep
        jmp result
END ASM

' ---- Loops

ASM
; loop: n steps of a counting loop
countup: sta zn
        stx zn + 1
        stz zr
        stz zr + 1
@loop:  lda zn
        ora zn + 1
        beq @done
        inc zr
        bne :+
        inc zr + 1
:       dec16 zn
        bra @loop
@done:  jmp result

; while: the sum of i AND 3 for i below n, while i < n
whilesum: sta zn
        stx zn + 1
        stz zi
        stz zi + 1
        stz zr
        stz zr + 1
@loop:  below16 zi, zn
        bcs @done
        lda zi
        and #3
        clc
        adc zr
        sta zr
        bcc :+
        inc zr + 1
:       inc zi
        bne @loop
        inc zi + 1
        bra @loop
@done:  jmp result

; dotimes: the same sum, i counted up as n counts down
forsum: sta zn
        stx zn + 1
        stz zi
        stz zi + 1
        stz zr
        stz zr + 1
@loop:  lda zn
        ora zn + 1
        beq @done
        lda zi
        and #3
        clc
        adc zr
        sta zr
        bcc :+
        inc zr + 1
:       inc zi
        bne :+
        inc zi + 1
:       dec16 zn
        bra @loop
@done:  jmp result

; nested: the pairs i, j below n whose i XOR j is even, a loop in a loop
nested: sta zn
        stz zr
        stz zr + 1
        ldx #0
@i:     ldy #0
@j:     stx zt
        tya
        eor zt
        and #1
        bne :+
        inc zr
        bne :+
        inc zr + 1
:       iny
        cpy zn
        bne @j
        inx
        cpx zn
        bne @i
        jmp result
END ASM

' ---- Arithmetic

ASM
; gcd: the sum of gcd(i, j) for i and j 1 to n, each by subtraction
gcdsum: sta zn
        stz zr
        stz zr + 1
        lda #1
        sta zi
@i:     lda #1
        sta zj
@j:     lda zi
        sta za
        lda zj
        sta zb
@g:     lda za
        cmp zb
        beq @eq
        bcc @less
        sbc zb                  ; (a > b: a - b)
        sta za
        bra @g
@less:  lda zb                  ; (b - a)
        sec
        sbc za
        sta zb
        bra @g
@eq:    clc
        adc zr
        sta zr
        bcc :+
        inc zr + 1
:       lda zj
        cmp zn
        beq @nexti
        inc zj
        bra @j
@nexti: lda zi
        cmp zn
        beq @done
        inc zi
        bra @i
@done:  jmp result

; collatz: the steps to 1 of each n from 1 to m (halved if even, else 3n + 1), summed
collatz: sta zn
        stz zr
        stz zr + 1
        lda #1
        sta zi
@i:     lda zi
        sta za
        stz za + 1
@step:  lda za + 1              ; (while x > 1)
        bne @go
        lda za
        cmp #2
        bcc @next
@go:    lda za
        lsr a
        bcc @even
        lda za                  ; (3x + 1: x + 2x + 1)
        asl a
        sta zt
        lda za + 1
        rol a
        sta zt + 1
        sec
        lda za
        adc zt
        sta za
        lda za + 1
        adc zt + 1
        sta za + 1
        bra @count
@even:  lsr za + 1
        ror za
@count: inc zr
        bne @step
        inc zr + 1
        bra @step
@next:  lda zi
        cmp zn
        beq @done
        inc zi
        bra @i
@done:  jmp result

; hash: h = ((h AND 255) * 31 + i) AND 4095 for i below n: a multiplication a step
hash:   sta zn
        stx zn + 1
        stz zi
        stz zi + 1
        stz zr
        stz zr + 1
@loop:  below16 zi, zn
        bcs @done
        lda zr
        ldy #31
        jsr mul8
        clc
        adc zi
        sta zr
        txa
        adc zi + 1
        and #$0F
        sta zr + 1
        inc zi
        bne @loop
        inc zi + 1
        bra @loop
@done:  jmp result
END ASM

' ---- Arrays (bytes, in the blocks' .bss)

ASM
; sieve: the primes below n, a flag each, a prime's multiples marked from its double
sieve:  sta zn
        stx zn + 1
        lda #<sflags            ; (Each flag 1)
        sta zp
        lda #>sflags
        sta zp + 1
        lda zn
        sta zk
        lda zn + 1
        sta zk + 1
@fill:  lda #1
        sta (zp)
        inc zp
        bne :+
        inc zp + 1
:       dec16 zk
        lda zk
        ora zk + 1
        bne @fill
        stz zr
        stz zr + 1
        lda #2
        sta zi
        stz zi + 1
@i:     below16 zi, zn
        bcs @done
        clc
        lda #<sflags
        adc zi
        sta zp
        lda #>sflags
        adc zi + 1
        sta zp + 1
        lda (zp)
        beq @nexti
        inc zr
        bne :+
        inc zr + 1
:       lda zi                  ; (j from i + i, by i)
        asl a
        sta zj
        lda zi + 1
        rol a
        sta zj + 1
@j:     below16 zj, zn
        bcs @nexti
        clc
        lda #<sflags
        adc zj
        sta zp
        lda #>sflags
        adc zj + 1
        sta zp + 1
        lda #0
        sta (zp)
        clc
        lda zj
        adc zi
        sta zj
        lda zj + 1
        adc zi + 1
        sta zj + 1
        bra @j
@nexti: inc zi
        bne @i
        inc zi + 1
        bra @i
@done:  jmp result

; sort: n bytes (x' = 13x + 7, mod 256, from 1) sorted by insertion; the first, the middle and the last added
sortb:  sta zn
        lda #1
        ldx #0
@gen:   sta sbuf,x
        phx
        ldy #13
        jsr mul8
        plx
        clc
        adc #7
        inx
        cpx zn
        bne @gen
        ldx #1
@i:     cpx zn
        bcs @sorted
        lda sbuf,x
        sta zv
        txa                     ; (.Y: j + 1)
        tay
@j:     cpy #0
        beq @put
        lda sbuf - 1,y
        cmp zv
        bcc @put
        beq @put
        sta sbuf,y
        dey
        bra @j
@put:   lda zv
        sta sbuf,y
        inx
        bra @i
@sorted: lda sbuf
        sta zr
        stz zr + 1
        ldx zn
        lda sbuf - 1,x
        clc
        adc zr
        sta zr
        bcc :+
        inc zr + 1
:       lda zn
        lsr a
        tax
        lda sbuf,x
        clc
        adc zr
        sta zr
        bcc :+
        inc zr + 1
:       jmp result

; matrix: two n by n matrices (a: (i + j) AND 3, b: i * j AND 3) multiplied, the product's elements summed; a row
; each 16 bytes
matrix: sta zn
        stz zi
@fi:    stz zj
@fj:    lda zi
        asl a
        asl a
        asl a
        asl a
        ora zj
        sta zv
        lda zi
        clc
        adc zj
        and #3
        ldx zv
        sta ma,x
        lda zi
        ldy zj
        jsr mul8
        and #3
        ldx zv
        sta mb,x
        inc zj
        lda zj
        cmp zn
        bne @fj
        inc zi
        lda zi
        cmp zn
        bne @fi
        stz zr
        stz zr + 1
        stz zi
@mi:    stz zj
@mj:    stz zk
@mk:    lda zi                  ; (a(i, k) * b(k, j))
        asl a
        asl a
        asl a
        asl a
        ora zk
        tax
        lda ma,x
        sta zt
        lda zk
        asl a
        asl a
        asl a
        asl a
        ora zj
        tax
        lda mb,x
        ldy zt
        jsr mul8
        clc
        adc zr
        sta zr
        bcc :+
        inc zr + 1
:       inc zk
        lda zk
        cmp zn
        bne @mk
        inc zj
        lda zj
        cmp zn
        bne @mj
        inc zi
        lda zi
        cmp zn
        bne @mi
        jmp result

; queens: the ways n queens can stand on an n by n board, none taking another, counted by backtracking (a flag
; each for the columns and the two ways of diagonals)
queens: sta zn
        ldx #31
:       stz qd1,x
        stz qd2,x
        dex
        bpl :-
        ldx #15
:       stz qcol,x
        dex
        bpl :-
        stz zr
        stz zr + 1
        lda #0
        jsr place
        jmp result
place:  cmp zn                  ; (row r in .A)
        bne @go
        inc zr
        bne :+
        inc zr + 1
:       rts
@go:    ldy #0                  ; (.Y: the column i)
@col:   sta zv
        lda qcol,y
        bne @next
        tya                     ; (r + i)
        clc
        adc zv
        tax
        lda qd1,x
        bne @next
        lda zv                  ; (r - i + n)
        clc
        adc zn
        sty zt
        sec
        sbc zt
        tax
        lda qd2,x
        bne @next
        lda #1
        sta qcol,y
        sta qd2,x
        tya
        clc
        adc zv
        tax
        lda #1
        sta qd1,x
        lda zv                  ; (row r + 1: r and i kept)
        pha
        phy
        inc a
        jsr place
        ply
        pla
        sta zv
        lda #0
        sta qcol,y
        tya
        clc
        adc zv
        tax
        stz qd1,x
        lda zv
        clc
        adc zn
        sty zt
        sec
        sbc zt
        tax
        stz qd2,x
@next:  lda zv
        iny
        cpy zn
        bne @col
        rts

.bss
sflags: .res 1024
sbuf:   .res 100
ma:     .res 256
mb:     .res 256
qcol:   .res 16
qd1:    .res 32
qd2:    .res 32
earr:   .res EACHN
dbuf:   .res 8
.code
END ASM

' ---- Subroutines called for each (hylang: map, filter, foldl, each; HyForth: EXECUTE)

ASM
; mapf: the sum of the squares of the evens below MAPN, a subroutine called for each test and each square, k times
mapf:   sta zk
@rep:   stz zr
        stz zr + 1
        stz zi
@i:     lda zi
        jsr iseven
        beq @next
        lda zi
        jsr sq
        clc
        adc zr
        sta zr
        txa
        adc zr + 1
        sta zr + 1
@next:  inc zi
        lda zi
        cmp #MAPN
        bne @i
        dec zk
        bne @rep
        jmp result
iseven: and #1                  ; (1 if .A is even)
        eor #1
        rts
sq:     tay
        jmp mul8

; fold: a = (3a + x) AND 1023 over 0 to FOLDN - 1, a subroutine called for each, k times
fold:   sta zk
@rep:   stz za
        stz za + 1
        stz zi
@x:     lda za
        ldx za + 1
        ldy zi
        jsr f3
        sta za
        stx za + 1
        inc zi
        lda zi
        cmp #FOLDN
        bne @x
        dec zk
        bne @rep
        lda za
        ldx za + 1
        rts
f3:     sta zt                  ; ((a in .A, .X) * 3 + .Y) AND 1023
        stx zt + 1
        asl a
        sta zs
        txa
        rol a
        sta zs + 1
        clc
        lda zs
        adc zt
        sta zs
        lda zs + 1
        adc zt + 1
        sta zs + 1
        tya
        clc
        adc zs
        pha
        lda zs + 1
        adc #0
        and #3
        tax
        pla
        rts

; each: the sum of x AND 7 over an array of 0 to EACHN - 1, k times
eachsum: sta zk
        ldx #0
:       txa
        sta earr,x
        inx
        cpx #EACHN
        bne :-
@rep:   stz zr
        stz zr + 1
        ldx #0
@i:     lda earr,x
        and #7
        clc
        adc zr
        sta zr
        bcc :+
        inc zr + 1
:       inx
        cpx #EACHN
        bne @i
        dec zk
        bne @rep
        jmp result
END ASM

' ---- Text

ASM
; chars: the a's in a string of 64 characters, k times
chars:  sta zk
@rep:   stz zr
        stz zr + 1
        ldx #0
@c:     lda text,x
        cmp #'a'
        bne :+
        inc zr
:       inx
        cpx #TEXTLEN
        bne @c
        dec zk
        bne @rep
        jmp result

; digits: the numbers below n written out in decimal (a digit at a time, by division by 10), their lengths summed
digitslen: sta zn
        stx zn + 1
        stz zi
        stz zi + 1
        stz zr
        stz zr + 1
@i:     below16 zi, zn
        bcs @done
        lda zi
        sta za
        lda zi + 1
        sta za + 1
        ldy #0
@d:     jsr div10
        ora #'0'
        sta dbuf,y
        iny
        lda za
        ora za + 1
        bne @d
        tya
        clc
        adc zr
        sta zr
        bcc :+
        inc zr + 1
:       inc zi
        bne @i
        inc zi + 1
        bra @i
@done:  jmp result
div10:  lda #0                  ; (za / 10 into za, the remainder in .A; .Y kept)
        ldx #16
@bit:   asl za
        rol za + 1
        rol a
        cmp #10
        bcc :+
        sbc #10
        inc za
:       dex
        bne @bit
        rts

.rodata
text:   .byte "the quick brown fox jumps over a lazy dog and a cat at the gate."
TEXTLEN = * - text
.code
END ASM
