' flow.bas - FOR (exact steps), DO and LOOP in their forms, WHILE, EXIT, GOTO, GOSUB and RETURN, ON ... GOTO and GOSUB,
' labels and line numbers (in any order), a line's continuation
DIM SHARED checks, failed
SUB ck (got, want, what$)
    checks = checks + 1
    IF got <> want THEN failed = failed + 1: PRINT "FAIL "; what$; ":"; got; "not"; want
END SUB

' FOR
n = 0: FOR i = 1 TO 10: n = n + i: NEXT
ck n, 55, "FOR"
ck i, 11, "past its end"
n = 0: FOR i = 0 TO 1 STEP 0.1: n = n + 1: NEXT
ck n, 11, "STEP 0.1: 11 times, exactly"
n = 0: FOR i = 0 TO 1 STEP 1 / 3: n = n + 1: NEXT
ck n, 4, "STEP 1/3"
ck i, 4 / 3, "STEP 1/3: its end"
n = 0: FOR i = 10 TO 1 STEP -2: n = n + i: NEXT i
ck n, 30, "STEP -2"
n = 0: FOR i = 5 TO 1: n = n + 1: NEXT
ck n, 0, "none"
n = 0: FOR i = 2 ^ 64 TO 2 ^ 64 + 2: n = n + 1: NEXT
ck n, 3, "big"
n = 0
FOR i = 1 TO 3
    FOR j = 1 TO 4
        n = n + 1
NEXT j, i
ck n, 12, "NEXT j, i"
n = 0: e = 3
FOR i = 1 TO e: e = 10: n = n + 1: NEXT
ck n, 3, "its end read once"
' DO, WHILE
n = 0: DO WHILE n < 5: n = n + 1: LOOP
ck n, 5, "DO WHILE"
n = 0: DO UNTIL n = 7: n = n + 1: LOOP
ck n, 7, "DO UNTIL"
n = 10: DO: n = n + 1: LOOP WHILE n < 5
ck n, 11, "LOOP WHILE: once"
n = 0: DO: n = n + 2: LOOP UNTIL n >= 9
ck n, 10, "LOOP UNTIL"
n = 0
DO
    n = n + 1
    IF n = 6 THEN EXIT DO
LOOP
ck n, 6, "EXIT DO"
n = 0: WHILE n < 4: n = n + 1: WEND
ck n, 4, "WHILE"
n = 0
WHILE n < 100
    n = n + 1
    k = 0
    WHILE k < n: k = k + 1: WEND
WEND
ck n + k, 200, "WHILE in WHILE"
n = 0
FOR i = 1 TO 100
    IF i = 9 THEN EXIT FOR
    n = n + 1
NEXT
ck n * 100 + i, 809, "EXIT FOR"
n = 0
FOR i = 1 TO 3
    DO
        n = n + 1
        IF n MOD 2 = 0 THEN EXIT DO
    LOOP
NEXT
ck n, 6, "EXIT DO in FOR"
' GOTO
n = 0
again:
n = n + 1
IF n < 3 THEN GOTO again
ck n, 3, "GOTO back"
GOTO ahead
ck 0, 1, "GOTO ahead"
ahead: ck 1, 1, "GOTO ahead"
' GOSUB
n = 0
GOSUB addOne: GOSUB addOne
ck n, 2, "GOSUB"
GOSUB twice
ck n, 4, "GOSUB in GOSUB"
GOSUB elsewhere
ck 0, 1, "RETURN label"
back: ck n, 5, "RETURN label"
' ON
FOR i = 0 TO 4
    r = 0
    ON i GOTO o1, o2, o3
    r = -1
    GOTO onDone
o1: r = 1: GOTO onDone
o2: r = 2: GOTO onDone
o3: r = 3
onDone:
    IF i = 0 OR i = 4 THEN ck r, -1, "ON out of range" ELSE ck r, i, "ON GOTO"
NEXT
r = 0: ON 2 GOSUB s1, s2: ck r, 20, "ON GOSUB"
' Line numbers, in any order
GOTO 300
200 ck 0, 1, "line 200 passed"
300 ck 1, 1, "GOTO 300"
GOSUB 50
ck r, 50, "GOSUB 50"
' A line's continuation
x = 1 + _
    2 + _
    3
ck x, 6, "_"
PRINT "flow:"; checks; "checks,"; failed; "failed"
END

addOne: n = n + 1: RETURN
twice: GOSUB addOne: GOSUB addOne: RETURN
elsewhere: n = n + 1: RETURN back
s1: r = 10: RETURN
s2: r = 20: RETURN
50 r = 50: RETURN
