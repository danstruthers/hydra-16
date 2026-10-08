' bench.bas - BASIC's side of the benchmarks (bench.hl is hylang's, bench.fs HyForth's: the same algorithms, the same
' sizes, the same results).  basic bench.bas [reps [q|f [name ...]]]: each benchmark run reps times (1), and a line for
' each, "bench basic NAME RESULT TICKS REPS": its value, and the ticks the reps took (200 a second).  q: smaller sizes
' (the regression test's); names: those alone.  Each in BASIC's own way: a FUNCTION each, its parameters (by
' reference) and locals its own; recursion where hylang and HyForth recurse; arrays where hylang has lists and
' HyForth memory; a FUNCTION's call where they call a function given (map, filter, foldl; EXECUTE).  sim/bench.js runs
' the three languages' and compares them.

DIM SHARED qcol(15), qd1(31), qd2(31), qn
reps = VAL(ARG$(1)): IF reps < 1 THEN reps = 1
quick = ARG$(2) = "q"
DO
    READ nm$, nf, nq
    IF nm$ = "done" THEN EXIT DO
    IF chosen(nm$) THEN
        n = nf: IF quick THEN n = nq
        t0 = ticks
        FOR rr = 1 TO reps
            SELECT CASE nm$
                CASE "calls": r = calls(n)
                CASE "fib": r = fibr(n)
                CASE "tak": r = taks(n)
                CASE "ack": r = acks(n)
                CASE "loop": r = countUp(n)
                CASE "while": r = whileSum(n)
                CASE "dotimes": r = forSum(n)
                CASE "nested": r = nested(n)
                CASE "gcd": r = gcdSum(n)
                CASE "collatz": r = collatz(n)
                CASE "hash": r = hash(n)
                CASE "sieve": r = sieve(n)
                CASE "sort": r = sortBytes(n)
                CASE "matrix": r = matrix(n)
                CASE "queens": r = queens(n)
                CASE "mapf": r = mapf(40, n)
                CASE "fold": r = fold(200, n)
                CASE "each": r = eachSum(200, n)
                CASE "chars": r = chars(n)
                CASE "digits": r = digitsLen(n)
            END SELECT
        NEXT
        d = (ticks - t0) MOD 32768: IF d < 0 THEN d = d + 32768
        PRINT "bench basic "; nm$; STR$(r); STR$(d); STR$(reps)
    END IF
LOOP
PRINT "bench basic done"
' Each benchmark's name, its size, its quick size (mapf's, fold's and each's: their reps)
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

' ---- Calls

' calls: n calls of a FUNCTION of two arguments
FUNCTION add2 (a, b)
    add2 = a + b
END FUNCTION
FUNCTION calls (n)
    c = 0
    FOR i = 1 TO n: c = add2(c, 1): NEXT
    calls = c
END FUNCTION

' fib: Fibonacci, recursively
FUNCTION fibr (n)
    IF n < 2 THEN fibr = n ELSE fibr = fibr(n - 1) + fibr(n - 2)
END FUNCTION

' tak: Takeuchi's function, three arguments, each call's three calls' values its arguments: tak(9, 6, 3), k times,
' summed
FUNCTION tak (x, y, z)
    IF y < x THEN tak = tak(tak(x - 1, y, z), tak(y - 1, z, x), tak(z - 1, x, y)) ELSE tak = z
END FUNCTION
FUNCTION taks (k)
    s = 0
    FOR i = 1 TO k: s = s + tak(9, 6, 3): NEXT
    taks = s
END FUNCTION

' ack: Ackermann's function, its calls nested deep: ack(2, 9), k times, summed
FUNCTION ack (m, n)
    IF m = 0 THEN
        ack = n + 1
    ELSEIF n = 0 THEN
        ack = ack(m - 1, 1)
    ELSE
        ack = ack(m - 1, ack(m, n - 1))
    END IF
END FUNCTION
FUNCTION acks (k)
    s = 0
    FOR i = 1 TO k: s = s + ack(2, 9): NEXT
    acks = s
END FUNCTION

' ---- Loops

' loop: n steps of a counting loop
FUNCTION countUp (n)
    r = 0
    FOR i = 1 TO n: r = r + 1: NEXT
    countUp = r
END FUNCTION

' while: the sum of i AND 3 for i below n, by WHILE
FUNCTION whileSum (n)
    i = 0: s = 0
    WHILE i < n
        s = s + (i AND 3): i = i + 1
    WEND
    whileSum = s
END FUNCTION

' dotimes: the same sum, by FOR
FUNCTION forSum (n)
    s = 0
    FOR i = 0 TO n - 1: s = s + (i AND 3): NEXT
    forSum = s
END FUNCTION

' nested: the pairs i, j below n whose i XOR j is even, by a FOR in a FOR
FUNCTION nested (n)
    c = 0
    FOR i = 0 TO n - 1
        FOR j = 0 TO n - 1
            IF ((i XOR j) AND 1) = 0 THEN c = c + 1
        NEXT
    NEXT
    nested = c
END FUNCTION

' ---- Arithmetic

' gcd: the sum of gcd(i, j) for i and j 1 to n, each by subtraction
FUNCTION gcdSum (n)
    r = 0
    FOR i = 1 TO n
        FOR j = 1 TO n
            a = i: b = j
            DO WHILE a <> b
                IF a > b THEN a = a - b ELSE b = b - a
            LOOP
            r = r + a
        NEXT
    NEXT
    gcdSum = r
END FUNCTION

' collatz: the steps to 1 of each n from 1 to m (halved if even, else 3n + 1), summed
FUNCTION collatz (m)
    t = 0
    FOR i = 1 TO m
        x = i
        DO WHILE x > 1
            IF x AND 1 THEN x = 3 * x + 1 ELSE x = x \ 2
            t = t + 1
        LOOP
    NEXT
    collatz = t
END FUNCTION

' hash: h = ((h AND 255) * 31 + i) AND 4095 for i below n: a multiplication a step
FUNCTION hash (n)
    h = 0
    FOR i = 0 TO n - 1: h = ((h AND 255) * 31 + i) AND 4095: NEXT
    hash = h
END FUNCTION

' ---- Arrays

' sieve: the primes below n, a flag each, a prime's multiples marked from its double
FUNCTION sieve (n)
    DIM f(n - 1)
    FOR i = 0 TO n - 1: f(i) = 1: NEXT
    r = 0
    FOR i = 2 TO n - 1
        IF f(i) THEN
            r = r + 1
            FOR j = i + i TO n - 1 STEP i: f(j) = 0: NEXT
        END IF
    NEXT
    sieve = r
END FUNCTION

' sort: n bytes (x' = 13x + 7, mod 256, from 1) sorted by insertion; the first, the middle and the last added
FUNCTION sortBytes (n)
    DIM b(n - 1)
    x = 1
    FOR i = 0 TO n - 1: b(i) = x: x = (x * 13 + 7) AND 255: NEXT
    FOR i = 1 TO n - 1
        v = b(i): j = i - 1
        DO WHILE j >= 0
            IF b(j) <= v THEN EXIT DO
            b(j + 1) = b(j): j = j - 1
        LOOP
        b(j + 1) = v
    NEXT
    sortBytes = b(0) + b(n - 1) + b(n \ 2)
END FUNCTION

' matrix: two n by n matrices (a: (i + j) AND 3, b: i * j AND 3) multiplied, the product's elements summed
FUNCTION matrix (n)
    DIM a(n - 1, n - 1), b(n - 1, n - 1)
    FOR i = 0 TO n - 1
        FOR j = 0 TO n - 1: a(i, j) = (i + j) AND 3: b(i, j) = (i * j) AND 3: NEXT
    NEXT
    s = 0
    FOR i = 0 TO n - 1
        FOR j = 0 TO n - 1
            FOR k = 0 TO n - 1: s = s + a(i, k) * b(k, j): NEXT
        NEXT
    NEXT
    matrix = s
END FUNCTION

' queens: the ways n queens can stand on an n by n board, none taking another, counted by backtracking (a flag each
' for the columns and the two ways of diagonals: SHARED arrays)
FUNCTION place (r)
    IF r = qn THEN RETURN 1
    c = 0
    FOR i = 0 TO qn - 1
        IF qcol(i) = 0 AND qd1(r + i) = 0 AND qd2(r - i + qn) = 0 THEN
            qcol(i) = 1: qd1(r + i) = 1: qd2(r - i + qn) = 1
            c = c + place(r + 1)
            qcol(i) = 0: qd1(r + i) = 0: qd2(r - i + qn) = 0
        END IF
    NEXT
    place = c
END FUNCTION
FUNCTION queens (n)
    qn = n
    FOR i = 0 TO 15: qcol(i) = 0: NEXT
    FOR i = 0 TO 31: qd1(i) = 0: qd2(i) = 0: NEXT
    queens = place(0)
END FUNCTION

' ---- FUNCTIONs called for each (hylang: map, filter, foldl, each; HyForth: EXECUTE)

' mapf: the sum of the squares of the evens below n, a FUNCTION called for each test and each square, k times
FUNCTION sq (x)
    sq = x * x
END FUNCTION
FUNCTION isEven (x)
    isEven = (x AND 1) = 0
END FUNCTION
FUNCTION mapf (n, k)
    FOR rep = 1 TO k
        r = 0
        FOR i = 0 TO n - 1
            IF isEven(i) THEN r = r + sq(i)
        NEXT
    NEXT
    mapf = r
END FUNCTION

' fold: a = (3a + x) AND 1023 over 0 to n - 1, a FUNCTION called for each, k times
FUNCTION f3 (a, x)
    f3 = (a * 3 + x) AND 1023
END FUNCTION
FUNCTION fold (n, k)
    FOR rep = 1 TO k
        a = 0
        FOR x = 0 TO n - 1: a = f3(a, x): NEXT
    NEXT
    fold = a
END FUNCTION

' each: the sum of x AND 7 over an array of 0 to n - 1, k times
FUNCTION eachSum (n, k)
    DIM l(n - 1)
    FOR i = 0 TO n - 1: l(i) = i: NEXT
    FOR rep = 1 TO k
        r = 0
        FOR i = 0 TO n - 1: r = r + (l(i) AND 7): NEXT
    NEXT
    eachSum = r
END FUNCTION

' ---- Text

' chars: the a's in a string of 64 characters (MID$, ASC), k times
FUNCTION chars (k)
    t$ = "the quick brown fox jumps over a lazy dog and a cat at the gate."
    FOR rep = 1 TO k
        c = 0
        FOR i = 1 TO LEN(t$)
            IF ASC(MID$(t$, i, 1)) = 97 THEN c = c + 1
        NEXT
    NEXT
    chars = c
END FUNCTION

' digits: the numbers below n written out (STR$, less its sign's space), their lengths summed
FUNCTION digitsLen (n)
    s = 0
    FOR i = 0 TO n - 1: s = s + LEN(STR$(i)) - 1: NEXT
    digitsLen = s
END FUNCTION
