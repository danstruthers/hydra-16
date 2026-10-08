' bench.bas - BASIC's side of the benchmarks (bench.hl is hylang's, bench.fs HyForth's: the same algorithms, the same
' sizes, the same results).  basic bench.bas [reps [q|f]]: each benchmark run reps times (1), and a line for each,
' "bench basic NAME RESULT TICKS REPS": its value, and the ticks the reps took (200 a second).  q: smaller sizes
' (the regression test's).  Each in BASIC's own way: a FUNCTION each, its parameters and locals its own, recursion
' for Fibonacci, an array DIMmed in its FUNCTION.  sim/bench.js runs the three languages' and compares them.

reps = VAL(ARG$(1)): IF reps < 1 THEN reps = 1
quick = ARG$(2) = "q"
DO
    READ nm$, nf, nq
    IF nm$ = "done" THEN EXIT DO
    n = nf: IF quick THEN n = nq
    t0 = ticks
    FOR rr = 1 TO reps
        SELECT CASE nm$
            CASE "loop": r = countUp(n)
            CASE "calls": r = calls(n)
            CASE "fib": r = fibr(n)
            CASE "sieve": r = sieve(n)
            CASE "sort": r = sortBytes(n)
            CASE "gcd": r = gcdSum(n)
        END SELECT
    NEXT
    d = (ticks - t0) MOD 32768: IF d < 0 THEN d = d + 32768
    PRINT "bench basic "; nm$; STR$(r); STR$(d); STR$(reps)
LOOP
PRINT "bench basic done"
DATA loop, 4000, 1000, calls, 2000, 500, fib, 16, 12, sieve, 1024, 512, sort, 100, 40, gcd, 20, 10, done, 0, 0

' The clock: the ticks (200 a second, 16 bits)
FUNCTION ticks
    SYS "TICKS": RREG l, h
    ticks = h * 256 + l
END FUNCTION

' loop: n steps of a counting loop
FUNCTION countUp (n)
    r = 0
    FOR i = 1 TO n: r = r + 1: NEXT
    countUp = r
END FUNCTION

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
