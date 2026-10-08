' funcs.bas - the number functions: sizes and signs, to an integer, the math functions (exact when they can be, else
' DIGITS digits), RND, fractions and complex numbers' parts, GCD, FIB, shifts and bits, numbers and their text
DIM SHARED checks, failed
SUB ck (got, want, what$)
    checks = checks + 1
    IF got <> want THEN failed = failed + 1: PRINT "FAIL "; what$; ":"; got; "not"; want
END SUB
SUB cks (got$, want$, what$)
    checks = checks + 1
    IF got$ <> want$ THEN failed = failed + 1: PRINT "FAIL "; what$; ": "; got$; " not "; want$
END SUB

' Sizes and signs
ck ABS(-5), 5, "ABS -"
ck ABS(5), 5, "ABS"
ck ABS(-1 / 3), 1 / 3, "ABS of a fraction"
ck SGN(-2), -1, "SGN -"
ck SGN(0), 0, "SGN 0"
ck SGN(1 / 1000), 1, "SGN +"
' To an integer
ck INT(3.7), 3, "INT"
ck INT(-3.7), -4, "INT down"
ck INT(-3), -3, "INT whole"
ck INT(-1 / 3), -1, "INT a fraction"
ck FIX(3.7), 3, "FIX"
ck FIX(-3.7), -3, "FIX toward 0"
ck CINT(2.5), 2, "CINT half to even"
ck CINT(2.51), 3, "CINT"
ck CINT(-3.5), -4, "CINT -3.5"
ck CLNG(1 / 3), 0, "CLNG a third"
ck INT(2 ^ 80 + 0.5), 2 ^ 80, "INT big"
' The math functions: exact when they can be
ck SQR(16), 4, "SQR 16"
ck SQR(9 / 4), 3 / 2, "SQR 9/4"
ck SQR(2.25), 1.5, "SQR 2.25"
ck SQR(0), 0, "SQR 0"
ck EXP(0), 1, "EXP 0"
ck LOG(1), 0, "LOG 1"
ck SIN(0), 0, "SIN 0"
ck COS(0), 1, "COS 0"
ck ATN(0), 0, "ATN 0"
ck 8 ^ (1 / 3), 2, "8 ^ 1/3"
' Else DIGITS digits, correctly rounded
ck DIGITS(), 12, "DIGITS()"
ck PI, 3.14159265359, "PI"
ck SQR(2), 1.41421356237, "SQR 2"
ck EXP(1), 2.71828182846, "EXP 1"
ck LOG(10), 2.30258509299, "LOG 10"
ck SIN(1), 0.841470984808, "SIN 1"
ck COS(1), 0.540302305868, "COS 1"
ck TAN(1), 1.55740772465, "TAN 1"
ck ATN(1), 0.785398163397, "ATN 1"
ck SQR(-4), 2i, "SQR -4"
DIGITS 30
ck DIGITS(), 30, "DIGITS 30"
ck PI, 3.14159265358979323846264338328, "PI to 30"
DIGITS 12
' RND
r = RND
ck r >= 0 AND r < 1, -1, "RND from 0 to 1"
ck RND(0), r, "RND(0) the last"
RANDOMIZE 7: a = RND: b = RND
RANDOMIZE 7: ck RND, a, "RANDOMIZE again"
ck RND, b, "RANDOMIZE's next"
' Fractions, fixed decimals
ck FIXED(1 / 3, 4), 0.3333, "FIXED"
ck RATIONAL(0.75), 3 / 4, "RATIONAL"
ck NUMERATOR(6 / 8), 3, "NUMERATOR"
ck DENOMINATOR(6 / 8), 4, "DENOMINATOR"
ck DENOMINATOR(5), 1, "DENOMINATOR whole"
' Complex numbers
ck COMPLEX(1, 2), 1 + 2i, "COMPLEX"
ck REAL(3 - 4i), 3, "REAL"
ck IMAG(3 - 4i), -4, "IMAG"
ck IMAG(5), 0, "IMAG of a real number"
ck REAL(5), 5, "REAL of a real number"
' Integers
ck GCD(12, 18), 6, "GCD"
ck GCD(2 ^ 40, 6 ^ 20), 2 ^ 20, "GCD big"
ck FIB(10), 55, "FIB 10"
ck FIB(100), VAL("354224848179261915075"), "FIB 100"
ck SHL(1, 10), 1024, "SHL"
ck SHR(1024, 3), 128, "SHR"
ck SHL(1, 100), 2 ^ 100, "SHL big"
ck BIT(5, 0), 1, "BIT 0"
ck BIT(5, 1), 0, "BIT 1"
' Numbers and their text
ck VAL("42"), 42, "VAL"
ck VAL(" -1.5abc"), -1.5, "VAL's start"
ck VAL("x"), 0, "VAL none"
ck VAL("2/3"), 2 / 3, "VAL a fraction"
ck VAL("#xFF"), 255, "VAL #x"
ck VAL("&H10"), 16, "VAL &H"
cks STR$(42), " 42", "STR$"
cks STR$(-42), "-42", "STR$ -"
cks STR$(1 / 3), " 1/3", "STR$ a fraction"
cks STR$(0.5), " 0.5", "STR$ 0.5"
cks STR$(255, "x"), "FF", "STR$ in x"
cks STR$(255, "#x"), "#xFF", "STR$ in #x"
cks STR$(5, "b"), "101", "STR$ in b"
cks HEX$(255), "FF", "HEX$"
cks OCT$(8), "10", "OCT$"
ck CVN(MKN$(2 ^ 70 + 1 / 3)), 2 ^ 70 + 1 / 3, "MKN$ and CVN"
ck LEN(MKN$(5)), 1, "MKN$ 5: a byte"
PRINT "funcs:"; checks; "checks,"; failed; "failed"
