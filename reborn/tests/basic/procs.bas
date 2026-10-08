' procs.bas - SUBs and FUNCTIONs: arguments by reference (a variable, an element, a whole array) and by value (an
' expression, (x)), their own variables, RETURN, recursion, STATIC, SHARED, DIM SHARED, CONST, EXIT, CALL, INCLUDE
DIM SHARED checks, failed
SUB ck (got, want, what$)
    checks = checks + 1
    IF got <> want THEN failed = failed + 1: PRINT "FAIL "; what$; ":"; got; "not"; want
END SUB
DECLARE SUB swap2 (a, b)
INCLUDE "inc.bas"

SUB swap2 (a, b)
    t = a: a = b: b = t
END SUB
x = 1: y = 2: swap2 x, y
ck x * 10 + y, 21, "by reference"
ck t, 0, "its t its own"
x = 1: y = 2: swap2 (x), y
ck x * 10 + y, 11, "(x) a copy"
DIM a(5)
a(1) = 1: a(2) = 2: swap2 a(1), a(2)
ck a(1) * 10 + a(2), 21, "elements by reference"
i = 1: swap2 a(i), a(i + 1)
ck a(1) * 10 + a(2), 12, "elements by reference, i"
CALL swap2(x, y)
ck x * 10 + y, 11, "CALL"
' FUNCTIONs
FUNCTION sq (n)
    sq = n * n
END FUNCTION
FUNCTION half (n)
    RETURN n / 2
END FUNCTION
FUNCTION greet$ (who$)
    greet$ = "hi " + who$
END FUNCTION
ck sq(7), 49, "FUNCTION"
ck half(5), 5 / 2, "RETURN value"
ck greet$("bo") = "hi bo", -1, "a string FUNCTION"
ck sq(sq(2)) + half(sq(4)), 24, "calls in calls"
' Recursion
FUNCTION fact (n)
    IF n <= 1 THEN fact = 1 ELSE fact = n * fact(n - 1)
END FUNCTION
ck fact(10), 3628800, "recursion"
ck fact(30), VAL("265252859812191058636308480000000"), "fact 30"
FUNCTION fibr (n)
    IF n < 2 THEN RETURN n
    RETURN fibr(n - 1) + fibr(n - 2)
END FUNCTION
ck fibr(15), 610, "two calls deep"
SUB countDown (n, acc$)
    IF n = 0 THEN EXIT SUB
    acc$ = acc$ + STR$(n)
    countDown n - 1, acc$
END SUB
s$ = "": countDown 3, s$
ck s$ = " 3 2 1", -1, "a recursive SUB, EXIT SUB"
' Whole arrays
SUB fill (arr(), v)
    FOR i = LBOUND(arr) TO UBOUND(arr): arr(i) = v: NEXT
END SUB
FUNCTION total (arr())
    s = 0
    FOR i = LBOUND(arr) TO UBOUND(arr): s = s + arr(i): NEXT
    total = s
END FUNCTION
DIM b(2 TO 6)
fill b(), 3
ck total(b()), 15, "an array given"
SUB names (n$())
    n$(1) = "one"
END SUB
DIM nm$(3): names nm$()
ck nm$(1) = "one", -1, "a string array given"
' STATIC, SHARED
FUNCTION counter
    STATIC c
    c = c + 1
    counter = c
END FUNCTION
z = counter: z = counter
ck counter, 3, "STATIC"
SUB bump STATIC
    SHARED kk
    k = k + 1
    kk = k
END SUB
CALL bump: bump
SUB look
    SHARED kk, sh
    sh = kk
END SUB
look
ck sh, 2, "SUB STATIC, SHARED"
DIM SHARED glob
glob = 5
SUB useGlob
    glob = glob + 1
END SUB
useGlob
ck glob, 6, "DIM SHARED"
CONST ten = 10, word$ = "w"
FUNCTION useConst
    useConst = ten * 2
END FUNCTION
ck useConst, 20, "a CONST in a FUNCTION"
' EXIT FUNCTION, a parameter's type
FUNCTION firstOver (lim)
    firstOver = -1
    FOR i = 1 TO 100
        IF i * i > lim THEN firstOver = i: EXIT FUNCTION
    NEXT
END FUNCTION
ck firstOver(50), 8, "EXIT FUNCTION"
FUNCTION twice (n AS INTEGER)
    twice = n * 2
END FUNCTION
ck twice(2.5), 5, "AS INTEGER: exact"
' Its variables new each call
FUNCTION fresh
    fresh = v
    v = 9
END FUNCTION
ck fresh + fresh, 0, "locals new each call"
' A FUNCTION's call among a call's arguments, elements by reference
SUB inc2 (p, q)
    p = p + 1: q = q + 10
END SUB
a(1) = 0: a(4) = 0
inc2 a(sq(1)), a(sq(2))
ck a(1) + a(4), 11, "elements at a FUNCTION's index"
' A procedure that calls another: its own locals, its own return
SUB addTo (t, v)
    t = t + v
END SUB
FUNCTION sumTo (n)
    DIM acc
    FOR k = 1 TO n: addTo acc, k: NEXT
    sumTo = acc
END FUNCTION
ck sumTo(10), 55, "a FUNCTION calling a SUB"
SUB outer (r)
    loc2 = 5: addTo loc2, 1
    r = loc2
END SUB
outer res
ck res, 6, "a SUB calling a SUB, its locals"
' INCLUDE
ck triple(4), 12, "INCLUDE's FUNCTION"
ck incName$ = "inc", -1, "INCLUDE's CONST"
PRINT "procs:"; checks; "checks,"; failed; "failed"
