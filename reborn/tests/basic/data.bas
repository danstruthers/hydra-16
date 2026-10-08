' data.bas - DATA, READ and RESTORE (numbers of every kind, strings quoted and bare, RESTORE to a label, past the
' end); the declarations: CONST, OPTION BASE, DEFSTR and DEFINT, a suffix's variable
OPTION BASE 1
DIM SHARED checks, failed
SUB ck (got, want, what$)
    checks = checks + 1
    IF got <> want THEN failed = failed + 1: PRINT "FAIL "; what$; ":"; got; "not"; want
END SUB

DATA 1, -2.5, 1/3, 2i, &HFF, #b101, 1E3
READ a, b, c, d, e, f, g
ck a, 1, "READ"
ck b, -2.5, "a decimal"
ck c, 1 / 3, "a fraction"
ck d, 2i, "complex"
ck e, 255, "&HFF"
ck f, 5, "#b101"
ck g, 1000, "1E3"
DATA "quoted, with a comma", bare words ,  spaced  , ""
READ s1$, s2$, s3$, s4$
ck s1$ = "quoted, with a comma", -1, "a quoted string"
ck s2$ = "bare words", -1, "a bare string"
ck s3$ = "spaced", -1, "spaces around it dropped"
ck s4$ = "", -1, "an empty string"
DATA 42, 1180591620717411303424, 2 ^ 70
READ n$, big, two
ck n$ = "42", -1, "a number read as a string"
ck big, 2 ^ 70, "a big number"
ck two, 2, "2 ^ 70: as VAL reads it"
' RESTORE
RESTORE
READ a
ck a, 1, "RESTORE"
RESTORE more
READ x, y
ck x * 10 + y, 78, "RESTORE label"
more:
DATA 7, 8
DATA 9
READ z
ck z, 9, "on after it"
ON ERROR GOTO bad
e = 0: READ z
ck e, 4, "out of DATA"
ON ERROR GOTO 0
' CONST
CONST limit = 10, title$ = "Hydra", twoPi = 2 * PI
ck limit * 2, 20, "CONST"
ck title$ = "Hydra", -1, "a string CONST"
ck twoPi > 6.28, -1, "a CONST of an expression"
' OPTION BASE 1
DIM ob(3)
ck LBOUND(ob), 1, "OPTION BASE 1"
ck UBOUND(ob), 3, "its highest"
DIM ob0(0 TO 2)
ck LBOUND(ob0), 0, "0 TO 2 all the same"
' DEFSTR, DEFINT: the names' kinds
DEFSTR s
sx = "a string"
ck LEN(sx), 8, "DEFSTR s"
DEFINT i-k
ivar = 2.5
ck ivar, 2.5, "DEFINT changes nothing"
' Suffixes: one variable
q = 3
ck q% + q& + q! + q#, 12, "q% q& q! q#"
PRINT "data:"; checks; "checks,"; failed; "failed"
END
bad:
e = ERR
RESUME NEXT
