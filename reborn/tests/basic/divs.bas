' divs.bas - integers' division and product, worked by the interpreter where it can (FIX(a / b) and INT(a / b) of
' two integers, a / b that's whole, a * b of 16 bits, a \ b, a MOD b) and by the numbers library past that: each
' result's bytes (MKN$) as the build before those (7dfff75a) gave them, errors too ("E11": division by zero), in
' divs.dat: a, b, then the six results, a line each.  Made by a generator from that build's output: integers of every
' size (0, 1, -1, -2^31, past 32 bits), fixed decimals, fractions, complex numbers
DIM SHARED checks, failed
FUNCTION hx$ (x)
    m$ = MKN$(x): h$ = ""
    FOR k = 1 TO LEN(m$): h$ = h$ + RIGHT$("0" + HEX$(ASC(MID$(m$, k, 1))), 2): NEXT
    hx$ = h$
END FUNCTION
ON ERROR GOTO oops
OPEN "divs.dat" FOR INPUT AS #1
DO
    INPUT #1, a$: IF a$ = "end" THEN EXIT DO
    INPUT #1, b$: INPUT #1, w1$: INPUT #1, w2$: INPUT #1, w3$: INPUT #1, w4$: INPUT #1, w5$: INPUT #1, w6$
    a = VAL(a$): b = VAL(b$)
    r$ = "": r$ = hx$(FIX(a / b))
    w$ = w1$: checks = checks + 1: IF r$ <> w$ THEN failed = failed + 1: PRINT "FAIL "; a$; " "; b$; " FIX(a / b): "; r$; " not "; w$
    r$ = "": r$ = hx$(INT(a / b))
    w$ = w2$: checks = checks + 1: IF r$ <> w$ THEN failed = failed + 1: PRINT "FAIL "; a$; " "; b$; " INT(a / b): "; r$; " not "; w$
    r$ = "": r$ = hx$(a / b)
    w$ = w3$: checks = checks + 1: IF r$ <> w$ THEN failed = failed + 1: PRINT "FAIL "; a$; " "; b$; " a / b: "; r$; " not "; w$
    r$ = "": r$ = hx$(a * b)
    w$ = w4$: checks = checks + 1: IF r$ <> w$ THEN failed = failed + 1: PRINT "FAIL "; a$; " "; b$; " a * b: "; r$; " not "; w$
    r$ = "": r$ = hx$(a \ b)
    w$ = w5$: checks = checks + 1: IF r$ <> w$ THEN failed = failed + 1: PRINT "FAIL "; a$; " "; b$; " a \ b: "; r$; " not "; w$
    r$ = "": r$ = hx$(a MOD b)
    w$ = w6$: checks = checks + 1: IF r$ <> w$ THEN failed = failed + 1: PRINT "FAIL "; a$; " "; b$; " a MOD b: "; r$; " not "; w$
LOOP
CLOSE #1
PRINT "divs:"; checks; "checks,"; failed; "failed"
END
oops:
    r$ = "E" + MID$(STR$(ERR), 2): RESUME NEXT
