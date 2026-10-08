' errors.bas - ON ERROR GOTO and a handler: ERR, ERL, ERR$; RESUME, RESUME NEXT, RESUME label; ERROR n; an error in a
' SUB; the codes of BASIC's errors and the system's (a file not found, 256 + the system's code)
DIM SHARED checks, failed
SUB ck (got, want, what$)
    checks = checks + 1
    IF got <> want THEN failed = failed + 1: PRINT "FAIL "; what$; ":"; got; "not"; want
END SUB
SUB cks (got$, want$, what$)
    checks = checks + 1
    IF got$ <> want$ THEN failed = failed + 1: PRINT "FAIL "; what$; ": ["; got$; "] not ["; want$; "]"
END SUB

ON ERROR GOTO handler
' RESUME NEXT: on after it
e = 0: x = 1 / 0
ck e, 11, "division by zero"
ck where, 15, "ERL: its line in the file"
cks msg$, "division by zero", "ERR$"
' RESUME: the statement again
d = 0: tries = 0
mode = 1: x = 10 / d
ck x, 5, "RESUME: again, d made 2"
ck tries, 1, "once"
mode = 0
' RESUME label
mode = 2: x = 1 / 0
ck 0, 1, "RESUME label"
afterIt:
ck mode, 0, "RESUME label"
' ERROR n
e = 0: ERROR 5
ck e, 5, "ERROR 5"
e = 0: ERROR 200
ck e, 200, "ERROR 200"
' The errors' codes
e = 0: x = SQR(-1): ck e, 0, "SQR(-1) no error: i"
e = 0: x$ = CHR$(300): ck e, 5, "illegal function call"
e = 0: x = VAL("1E999"): ck e, 6, "overflow"
DIM a(3): e = 0: x = a(4): ck e, 9, "subscript out of range"
e = 0: x = LOG(0): ck e, 5, "LOG(0)"
e = 0: RESTORE noData: READ x: ck e, 4, "out of DATA"
e = 0: RETURN: ck e, 3, "RETURN without GOSUB"
e = 0: x$ = SPACE$(-1): ck e, 5, "SPACE$(-1)"
' The system's: a file not there, as QuickBASIC's; the rest 256 + its code, ERR$ its text
e = 0: OPEN "nosuch.txt" FOR INPUT AS #1: ck e, 53, "file not found"
e = 0: KILL "nosuch.txt": ck e, 53, "KILL: file not found"
MKDIR "edir"
e = 0: OPEN "edir" FOR OUTPUT AS #1: ck e, 256 + 35, "a system error: 256 + its code"
cks msg$, "is a directory", "its text"
RMDIR "edir"
e = 0: CHDIR "errors.bas": ck e, 76, "CHDIR to a file: path not found"
e = 0: CLOSE #7: ck e, 0, "CLOSE of one not open: none"
e = 0: PRINT #9, "x": ck e, 52, "bad file number"
' An error in a SUB: the main program's handler; RESUME NEXT in the SUB
SUB risky (r)
    r = 1
    r = r / 0
    r = r + 10
END SUB
e = 0: risky v
ck e, 11, "in a SUB"
ck v, 11, "RESUME NEXT in the SUB"
' Numbered lines: ERL their number
100 e = 0
110 x = 1 / 0
120 ck where, 110, "ERL: the line's number"
130 e = 0: GOSUB 500
140 ck where, 500, "ERL: the nearest number before it"
ON ERROR GOTO 0
PRINT "errors:"; checks; "checks,"; failed; "failed"
END

500 y = 1
x = 1 / 0
RETURN

handler:
e = ERR: where = ERL: msg$ = ERR$
SELECT CASE mode
    CASE 1
        tries = tries + 1: d = 2: RESUME
    CASE 2
        mode = 0: RESUME afterIt
END SELECT
RESUME NEXT
noData:
