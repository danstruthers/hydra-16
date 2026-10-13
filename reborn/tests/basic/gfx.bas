' gfx.bas - the bitmap (a Vera X), checked by POINT: SCREEN 13, PSET, LINE (a line, B a box, BF a filled one),
' CIRCLE (and F a disc); the same through a WINDOW (which kept none of their kinds: every one was filled); GPRINT's
' whole text through a WINDOW (it was 4 characters)
DIM SHARED checks, failed
SUB ck (got, want, what$)
    checks = checks + 1
    IF got <> want THEN failed = failed + 1: PRINT "FAIL "; what$; ":"; got; "not"; want
END SUB

SCREEN 13
PSET (5, 5), 7
p1 = POINT(5, 5): p2 = POINT(6, 5)
LINE (10, 10)-(60, 60), 5
l1 = POINT(30, 30): l2 = POINT(40, 20)
LINE (100, 10)-(150, 60), 3, B
b1 = POINT(100, 30): b2 = POINT(120, 30)
LINE (160, 10)-(210, 60), 4, BF
f1 = POINT(180, 30)
CIRCLE (60, 150), 30, 6
c1 = POINT(90, 150): c2 = POINT(60, 150)
CIRCLE (160, 150), 30, 9, , , , F
d1 = POINT(160, 150)
' Through a WINDOW (y up): the same statements, the same kinds
SCREEN 13
WINDOW (0, 0)-(319, 239)
LINE (10, 10)-(60, 60), 5
wl1 = POINT(30, 30): wl2 = POINT(40, 20)
LINE (100, 10)-(150, 60), 3, B
wb1 = POINT(100, 30): wb2 = POINT(120, 30)
LINE (160, 10)-(210, 60), 4, BF
wf1 = POINT(180, 30)
CIRCLE (60, 150), 30, 6
wc1 = POINT(90, 150): wc2 = POINT(60, 150)
CIRCLE (160, 150), 30, 9, , , , F
wd1 = POINT(160, 150)
' GPRINT: its eight characters (the last three's cells have dots in them)
GPRINT (10, 100), "HHHHHHHH"
far = 0: near = 0
FOR y = 92 TO 101
    FOR x = 50 TO 73
        IF POINT(x, y) <> 0 THEN far = 1
    NEXT
    FOR x = 10 TO 25
        IF POINT(x, y) <> 0 THEN near = 1
    NEXT
NEXT
SCREEN 0
ck p1, 7, "PSET, POINT"
ck p2, 0, "POINT beside it"
ck l1, 5, "LINE: on it"
ck l2, 0, "LINE: beside it (not a box)"
ck b1, 3, "LINE B: its edge"
ck b2, 0, "LINE B: inside it (not filled)"
ck f1, 4, "LINE BF: inside it"
ck c1, 6, "CIRCLE: on it"
ck c2, 0, "CIRCLE: its centre (not a disc)"
ck d1, 9, "CIRCLE F: its centre"
ck wl1, 5, "WINDOW, LINE: on it"
ck wl2, 0, "WINDOW, LINE: beside it (not a box)"
ck wb1, 3, "WINDOW, LINE B: its edge"
ck wb2, 0, "WINDOW, LINE B: inside it (not filled)"
ck wf1, 4, "WINDOW, LINE BF: inside it"
ck wc1, 6, "WINDOW, CIRCLE: on it"
ck wc2, 0, "WINDOW, CIRCLE: its centre (not a disc)"
ck wd1, 9, "WINDOW, CIRCLE F: its centre"
ck near, 1, "WINDOW, GPRINT: its first characters"
ck far, 1, "WINDOW, GPRINT: its last three"
ck ERR, 0, "no error on the way"
PRINT "gfx:"; checks; "checks,"; failed; "failed"
