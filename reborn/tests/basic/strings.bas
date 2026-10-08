' strings.bas - strings: joined, compared, the functions, MID$ as a statement, fixed-length strings, long ones (past
' 255 characters), many (the garbage collector)
DIM SHARED checks, failed
SUB ck (got, want, what$)
    checks = checks + 1
    IF got <> want THEN failed = failed + 1: PRINT "FAIL "; what$; ":"; got; "not"; want
END SUB
SUB cks (got$, want$, what$)
    checks = checks + 1
    IF got$ <> want$ THEN failed = failed + 1: PRINT "FAIL "; what$; ": ["; got$; "] not ["; want$; "]"
END SUB

a$ = "Hydra": b$ = "-16"
cks a$ + b$, "Hydra-16", "+"
cks "" + "", "", "empty +"
ck LEN(a$ + b$), 8, "LEN"
ck LEN(""), 0, "LEN empty"
' The functions
cks LEFT$(a$, 2), "Hy", "LEFT$"
cks LEFT$(a$, 0), "", "LEFT$ 0"
cks LEFT$(a$, 99), "Hydra", "LEFT$ past"
cks RIGHT$(a$, 3), "dra", "RIGHT$"
cks RIGHT$(a$, 99), "Hydra", "RIGHT$ past"
cks MID$(a$, 2, 3), "ydr", "MID$"
cks MID$(a$, 3), "dra", "MID$ to the end"
cks MID$(a$, 9, 2), "", "MID$ past"
cks MID$(a$, 4, 99), "ra", "MID$ long"
ck INSTR(a$, "dr"), 3, "INSTR"
ck INSTR(a$, "x"), 0, "INSTR none"
ck INSTR("abcabc", "bc"), 2, "INSTR first"
ck INSTR(3, "abcabc", "bc"), 5, "INSTR from"
ck INSTR(a$, ""), 1, "INSTR empty"
cks UCASE$("Hydra 16!"), "HYDRA 16!", "UCASE$"
cks LCASE$("Hydra 16!"), "hydra 16!", "LCASE$"
cks LTRIM$("  x  "), "x  ", "LTRIM$"
cks RTRIM$("  x  "), "  x", "RTRIM$"
cks SPACE$(3), "   ", "SPACE$"
cks SPACE$(0), "", "SPACE$ 0"
cks STRING$(4, "*"), "****", "STRING$"
cks STRING$(3, 65), "AAA", "STRING$ code"
cks STRING$(2, "xyz"), "xx", "STRING$ first"
cks CHR$(65) + CHR$(97), "Aa", "CHR$"
ck ASC("A"), 65, "ASC"
ck ASC("abc"), 97, "ASC first"
ck ASC(CHR$(200)), 200, "ASC 200"
cks STR$(7) + STR$(-7), " 7-7", "STR$"
ck VAL("  12  "), 12, "VAL spaces"
ck VAL(STR$(1 / 7)), 1 / 7, "VAL STR$"
cks HEX$(4096), "1000", "HEX$"
' Comparisons
ck "a" < "b", -1, "<"
ck "abc" > "abd", 0, ">"
ck "Z" < "a", -1, "bytes"
ck a$ = "Hydra", -1, "="
ck a$ <> "hydra", -1, "case counts"
' MID$ as a statement
s$ = "abcdef"
MID$(s$, 2, 3) = "XYZ"
cks s$, "aXYZef", "MID$ ="
MID$(s$, 5) = "123456"
cks s$, "aXYZ12", "MID$ = past the end"
MID$(s$, 1, 1) = "!?"
cks s$, "!XYZ12", "MID$ = its n"
' Fixed-length strings
DIM f AS STRING * 5
cks f, "     ", "STRING * 5 at the start"
f = "ab"
cks f, "ab   ", "padded"
f = "abcdefgh"
cks f, "abcde", "cut"
ck LEN(f), 5, "its length"
' Long strings
l$ = STRING$(300, "x") + "end"
ck LEN(l$), 303, "past 255"
cks RIGHT$(l$, 4), "xend", "RIGHT$ of it"
ck INSTR(l$, "end"), 301, "INSTR in it"
l$ = l$ + l$ + l$ + l$
ck LEN(l$), 1212, "longer"
cks MID$(l$, 1210, 3), "end", "MID$ far"
' Many: the garbage collector
DIM w$(200)
FOR i = 0 TO 200
    w$(i) = STR$(i) + STRING$(20, CHR$(65 + i MOD 26))
NEXT
FOR k = 1 TO 20
    FOR i = 0 TO 200 STEP 7
        w$(i) = STR$(i) + STRING$(20 + k, CHR$(65 + i MOD 26))
    NEXT
NEXT
ok = -1
FOR i = 0 TO 200
    want$ = STR$(i) + STRING$(20, CHR$(65 + i MOD 26))
    IF i MOD 7 = 0 THEN want$ = STR$(i) + STRING$(40, CHR$(65 + i MOD 26))
    IF w$(i) <> want$ THEN ok = 0
NEXT
ck ok, -1, "after the collector"
t$ = ""
FOR i = 1 TO 500
    t$ = t$ + CHR$(48 + i MOD 10)
NEXT
ck LEN(t$), 500, "built up"
cks MID$(t$, 491, 10), "1234567890", "its end"
PRINT "strings:"; checks; "checks,"; failed; "failed"
