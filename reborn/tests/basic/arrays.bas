' arrays.bas - arrays: DIM's forms (TO, dimensions), one made by its use (0 to 10), numbers of every kind and strings in
' them, REDIM (PRESERVE), ERASE, LBOUND and UBOUND, an array and a variable of one name, SWAP, a big one
DIM SHARED checks, failed
SUB ck (got, want, what$)
    checks = checks + 1
    IF got <> want THEN failed = failed + 1: PRINT "FAIL "; what$; ":"; got; "not"; want
END SUB

DIM a(10)
FOR i = 0 TO 10: a(i) = i * i: NEXT
ck a(0) + a(10), 100, "DIM a(10)"
ck LBOUND(a), 0, "LBOUND"
ck UBOUND(a), 10, "UBOUND"
DIM b(-5 TO 5)
b(-5) = 1: b(5) = 2
ck b(-5) + b(5), 3, "-5 TO 5"
ck LBOUND(b), -5, "LBOUND -5"
DIM g(1 TO 3, 1 TO 4)
FOR i = 1 TO 3: FOR j = 1 TO 4: g(i, j) = i * 10 + j: NEXT j, i
ck g(2, 3), 23, "two dimensions"
ck g(3, 4), 34, "the last"
ck UBOUND(g, 2), 4, "UBOUND 2"
ck LBOUND(g, 1), 1, "LBOUND 1"
DIM c(2, 2, 2)
c(1, 2, 1) = 7: c(2, 2, 2) = 8
ck c(1, 2, 1) * c(2, 2, 2), 56, "three dimensions"
ck c(0, 0, 0), 0, "0 at the start"
' Numbers of every kind
DIM n(3)
n(0) = 1 / 3: n(1) = 2 ^ 100: n(2) = 1 + 2i: n(3) = 0.125
ck n(0) * 3, 1, "a fraction"
ck n(1) / 2 ^ 99, 2, "big"
ck n(2) * n(2), -3 + 4i, "complex"
ck n(3) * 8, 1, "a fixed decimal"
n(1) = 5
ck n(1), 5, "big replaced"
' Strings
DIM s$(5)
s$(0) = "zero": s$(5) = "five"
ck s$(0) + s$(5) = "zerofive", -1, "strings"
ck LEN(s$(3)), 0, "empty at the start"
DIM t(3) AS STRING
t(1) = "as string"
ck t(1) = "as string", -1, "AS STRING"
' Made by its use: 0 to 10
u(10) = 4
ck u(10), 4, "used before DIM"
ck UBOUND(u), 10, "0 to 10"
v$(2) = "x"
ck v$(2) = "x", -1, "a string array used"
w(3, 4) = 34
ck w(3, 4), 34, "two dimensions used"
' A variable and an array of one name
a = 99
ck a + a(3), 108, "a and a()"
' REDIM, ERASE
REDIM r(5)
r(5) = 1
REDIM r(20)
ck r(5), 0, "REDIM empties"
ck UBOUND(r), 20, "REDIM's size"
r(1) = 11: r(20) = 20
REDIM PRESERVE r(30)
ck r(1) + r(20), 31, "REDIM PRESERVE"
ck UBOUND(r), 30, "its new size"
REDIM PRESERVE r(2)
ck r(1), 11, "PRESERVE smaller"
ERASE r
REDIM r(3)
ck r(1), 0, "ERASE"
' SWAP
x = 1: y = 2: SWAP x, y
ck x * 10 + y, 21, "SWAP"
SWAP a(1), a(2)
ck a(1) * 10 + a(2), 41, "SWAP elements"
p$ = "p": q$ = "q": SWAP p$, q$
ck p$ + q$ = "qp", -1, "SWAP strings"
' Big
DIM big(5000)
FOR i = 0 TO 5000 STEP 500: big(i) = i: NEXT
sum = 0
FOR i = 0 TO 5000 STEP 500: sum = sum + big(i): NEXT
ck sum, 27500, "5001 elements"
' Out of range
ON ERROR GOTO bad
e = 0: z = a(11)
ck e, 9, "a(11): subscript out of range"
e = 0: z = g(0, 1)
ck e, 9, "g(0, 1)"
e = 0: z = a(1, 1)
ck e, 9, "too many indices"
ON ERROR GOTO 0
PRINT "arrays:"; checks; "checks,"; failed; "failed"
END
bad:
e = ERR
RESUME NEXT
