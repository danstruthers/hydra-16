' files.bas - files: OPEN FOR OUTPUT, APPEND, INPUT, BINARY; PRINT #, WRITE #, PRINT # USING; INPUT #, LINE INPUT #,
' INPUT$; EOF, LOF, LOC, SEEK; GET and PUT; FREEFILE; several open; KILL, NAME, MKDIR, CHDIR, RMDIR, DIR$ (written on
' the card)
DIM SHARED checks, failed
SUB ck (got, want, what$)
    checks = checks + 1
    IF got <> want THEN failed = failed + 1: PRINT "FAIL "; what$; ":"; got; "not"; want
END SUB
SUB cks (got$, want$, what$)
    checks = checks + 1
    IF got$ <> want$ THEN failed = failed + 1: PRINT "FAIL "; what$; ": ["; got$; "] not ["; want$; "]"
END SUB

' Written, read back
OPEN "f1.txt" FOR OUTPUT AS #1
FOR i = 1 TO 3: PRINT #1, i; i * i: NEXT
PRINT #1, "a line, with a comma"
WRITE #1, "quoted", 1 / 3, -2
PRINT #1, USING "##.##"; 3.14159
CLOSE #1
OPEN "f1.txt" FOR INPUT AS #1
s = 0
FOR i = 1 TO 3: INPUT #1, a, b: s = s + a + b: NEXT
ck s, 20, "INPUT # numbers"
LINE INPUT #1, l$
cks l$, "a line, with a comma", "LINE INPUT #"
INPUT #1, q$, f, n
cks q$, "quoted", "WRITE #'s string"
ck f, 1 / 3, "WRITE #'s number, exact"
ck n, -2, "WRITE #'s -2"
LINE INPUT #1, u$
cks u$, " 3.14", "PRINT # USING"
ck EOF(1), -1, "EOF at the end"
CLOSE #1
' APPEND
OPEN "f1.txt" FOR APPEND AS #2
PRINT #2, "more"
CLOSE #2
OPEN "f1.txt" FOR INPUT AS #2
k = 0
DO UNTIL EOF(2): LINE INPUT #2, l$: k = k + 1: LOOP
ck k, 7, "APPEND: a line more"
cks l$, "more", "its last"
CLOSE
' INPUT$, LOF, LOC, SEEK
OPEN "f2.txt" FOR OUTPUT AS #3
PRINT #3, "abcdefghij";
CLOSE #3
OPEN "f2.txt" FOR BINARY AS #3
ck LOF(3), 10, "LOF"
cks INPUT$(3, #3), "abc", "INPUT$"
ck LOC(3), 3, "LOC"
ck SEEK(3), 4, "SEEK()"
SEEK #3, 8
cks INPUT$(2, #3), "hi", "SEEK #"
' GET and PUT
s$ = "XYZ"
PUT #3, 1, s$
SEEK #3, 1
g$ = SPACE$(4)
GET #3, , g$
cks g$, "XYZd", "PUT a string, GET it"
big = 2 ^ 70 + 1 / 3: PUT #3, 11, big
GET #3, 11, back
ck back, 2 ^ 70 + 1 / 3, "PUT a number, GET it"
ck LOF(3), 10 + LEN(MKN$(2 ^ 70 + 1 / 3)), "its bytes: MKN$'s"
CLOSE #3
' FREEFILE, several open
n1 = FREEFILE
OPEN "f1.txt" FOR INPUT AS n1
n2 = FREEFILE
OPEN "f2.txt" FOR INPUT AS #n2
ck n2 <> n1, -1, "FREEFILE: the next"
LINE INPUT #n1, a$: LINE INPUT #n2, b$
cks a$ + "|" + LEFT$(b$, 3), " 1  1 |XYZ", "two open"
CLOSE n1, n2
' Past the end
OPEN "f2.txt" FOR INPUT AS #4
LINE INPUT #4, a$
ON ERROR GOTO bad
e = 0: LINE INPUT #4, a$
ck e, 62, "input past end of file"
ON ERROR GOTO 0
CLOSE #4
' Files and directories
MKDIR "sub"
OPEN "sub/in.txt" FOR OUTPUT AS #1: PRINT #1, "inside": CLOSE #1
NAME "sub/in.txt" AS "sub/moved.txt"
cks DIR$("sub"), "moved.txt", "DIR$: NAME's"
cks DIR$, "", "DIR$: no more"
CHDIR "sub"
OPEN "moved.txt" FOR INPUT AS #1: LINE INPUT #1, a$: CLOSE #1
cks a$, "inside", "CHDIR"
KILL "moved.txt"
CHDIR ".."
RMDIR "sub"
ON ERROR GOTO bad
e = 0: CHDIR "sub"
ck e, 53, "RMDIR: gone"
ON ERROR GOTO 0
KILL "f1.txt": KILL "f2.txt"
PRINT "files:"; checks; "checks,"; failed; "failed"
END
bad:
e = ERR
RESUME NEXT
