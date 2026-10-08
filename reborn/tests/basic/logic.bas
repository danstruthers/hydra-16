' logic.bas - comparisons (numbers of every kind, strings), the bitwise operators on integers of any size, IF in its
' forms, SELECT CASE
DIM SHARED checks, failed
SUB ck (got, want, what$)
    checks = checks + 1
    IF got <> want THEN failed = failed + 1: PRINT "FAIL "; what$; ":"; got; "not"; want
END SUB

' Comparisons: -1 or 0
ck 1 < 2, -1, "<"
ck 2 < 1, 0, "< false"
ck 2 <= 2, -1, "<="
ck 3 > 2, -1, ">"
ck 2 >= 3, 0, ">="
ck 2 = 2, -1, "="
ck 2 <> 2, 0, "<>"
ck 1 / 3 < 0.34, -1, "a fraction and a decimal"
ck 1 = 1.0, -1, "1 = 1.0"
ck 0.5 = 1 / 2, -1, "0.5 = 1/2"
ck 2 ^ 100 > 2 ^ 99, -1, "big"
ck -(2 ^ 100) < 1, -1, "big below 0"
ck 1 + 2i = 1 + 2i, -1, "complex ="
ck 1 + 2i < 1 + 3i, -1, "complex by the imaginary part"
ck 2 + 0i > 1 + 5i, -1, "complex by the real part"
ck "abc" = "abc", -1, "strings ="
ck "abc" < "abd", -1, "strings <"
ck "ab" < "abc", -1, "a start <"
ck "B" < "a", -1, "by the bytes"
ck "" < "a", -1, "empty <"
' The bitwise operators
ck NOT 0, -1, "NOT 0"
ck NOT -1, 0, "NOT -1"
ck NOT 5, -6, "NOT 5"
ck 12 AND 10, 8, "AND"
ck 12 OR 10, 14, "OR"
ck 12 XOR 10, 6, "XOR"
ck 12 EQV 10, -7, "EQV"
ck 12 IMP 10, -5, "IMP"
ck -1 AND 255, 255, "AND -1"
ck (2 ^ 70 + 5) AND 7, 5, "AND big"
ck (2 ^ 70) OR 1, 2 ^ 70 + 1, "OR big"
ck NOT (2 ^ 70), -(2 ^ 70) - 1, "NOT big"
ck 6.6 AND 7, 7, "a fraction rounded first"
' Their order: comparisons, NOT, AND, OR
ck 1 < 2 AND 3 < 4, -1, "< before AND"
ck NOT 1 = 2, -1, "= before NOT"
ck 0 OR -1 AND 0, 0, "AND before OR"
ck -1 OR 0 AND 0, -1, "AND before OR 2"
ck 1 + 1 = 2 AND 2 * 2 = 4, -1, "arithmetic first"
' Truth: any number not 0
t = 0
IF 5 THEN t = 1
ck t, 1, "5 is true"
IF 1 / 1000 THEN t = 2
ck t, 2, "1/1000 is true"
IF 0 THEN t = 3
ck t, 2, "0 is false"
' IF on one line
x = 5
IF x > 3 THEN y = 1 ELSE y = 2
ck y, 1, "IF THEN"
IF x > 9 THEN y = 1 ELSE y = 2
ck y, 2, "IF ELSE"
IF x > 3 THEN y = 10: z = 20 ELSE y = 30: z = 40
ck y + z, 30, "IF statements"
IF x > 9 THEN y = 10: z = 20 ELSE y = 30: z = 40
ck y + z, 70, "ELSE statements"
IF x = 5 THEN IF y = 30 THEN w = 1 ELSE w = 2
ck w, 1, "IF in IF"
IF x = 5 THEN 100
ck 0, 1, "IF THEN a line number"
100 ck 1, 1, "IF THEN a line number"
IF x <> 5 THEN GOTO 110 ELSE GOTO there
110 ck 0, 1, "IF ELSE GOTO"
there:
ck 1, 1, "IF ELSE GOTO"
' IF blocks
FOR i = 1 TO 4
    IF i = 1 THEN
        r = 10
    ELSEIF i = 2 THEN
        r = 20
    ELSEIF i = 3 THEN
        IF x = 5 THEN
            r = 30
        ELSE
            r = -30
        END IF
    ELSE
        r = 40
    END IF
    ck r, i * 10, "IF block"
NEXT
IF x = 1 THEN
    r = 0
END IF
ck r, 40, "IF block not taken"
' SELECT CASE
FUNCTION kind$ (n)
    SELECT CASE n
        CASE 0
            kind$ = "zero"
        CASE 1, 3, 5
            kind$ = "odd small"
        CASE 2 TO 4, 6
            kind$ = "even small"
        CASE IS < 0
            kind$ = "below"
        CASE IS >= 100
            kind$ = "big"
        CASE ELSE
            kind$ = "other"
    END SELECT
END FUNCTION
ck kind$(0) = "zero", -1, "CASE 0"
ck kind$(3) = "odd small", -1, "CASE list"
ck kind$(4) = "even small", -1, "CASE TO"
ck kind$(6) = "even small", -1, "CASE TO, list"
ck kind$(-5) = "below", -1, "CASE IS <"
ck kind$(2 ^ 80) = "big", -1, "CASE IS >="
ck kind$(50) = "other", -1, "CASE ELSE"
ck kind$(7 / 2) = "even small", -1, "CASE TO a fraction"
s$ = "pear": n = 0
SELECT CASE s$
    CASE "apple": n = 1
    CASE "orange", "pear": n = 2
    CASE ELSE: n = 3
END SELECT
ck n, 2, "CASE strings"
SELECT CASE "m"
    CASE "a" TO "f": n = 1
    CASE "g" TO "z": n = 2
END SELECT
ck n, 2, "CASE strings TO"
SELECT CASE 9
    CASE 1: n = 5
END SELECT
ck n, 2, "no CASE"
PRINT "logic:"; checks; "checks,"; failed; "failed"
