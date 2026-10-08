' hydra.bas - the Hydra's own: SYS by name and RREG; a bank of the program's own (BANK, PEEK, POKE) and machine code
' in it (SYS, CALL ABSOLUTE); FRE; SLEEP and the ticks, TIMER, DATE$, TIME$; ENV$, ENVIRON$ and ENVIRON; ARG$ and
' COMMAND$ (rc's $greet: hi; its arguments: one two); SHELL, SHELL$ and STATUS
DIM SHARED checks, failed
SUB ck (got, want, what$)
    checks = checks + 1
    IF got <> want THEN failed = failed + 1: PRINT "FAIL "; what$; ":"; got; "not"; want
END SUB
SUB cks (got$, want$, what$)
    checks = checks + 1
    IF got$ <> want$ THEN failed = failed + 1: PRINT "FAIL "; what$; ": ["; got$; "] not ["; want$; "]"
END SUB

' SYS by name, RREG
SYS "GETPID": RREG p
ck p > 0 AND p < 16, -1, "GETPID"
SYS "getpid": RREG q
ck q, p, "a name in either case"
SYS "BANKS_ALLOC", 1: RREG b, , , st
ck st AND 1, 0, "BANKS_ALLOC: done"
ck b > 0 AND b < 256, -1, "its bank"
' A bank of its own: BANK, PEEK, POKE; machine code in it
BANK b
ck BANK(), b, "BANK()"
POKE &H8000, &HA9: POKE &H8001, 42: POKE &H8002, &H60
ck PEEK(&H8000) * 1000 + PEEK(&H8001), 169042, "POKE, PEEK"
SYS &H8000: RREG a
ck a, 42, "SYS addr: LDA #42"
POKE &H8000, &HE8: POKE &H8001, &HC8: POKE &H8002, &H60
SYS &H8000, 7, 1, 2: RREG a, x, y
ck a * 100 + x * 10 + y, 723, "SYS's registers in and out: INX, INY"
RREG , , y2
ck y2, 3, "RREG's places"
POKE &H8000, &HEE: POKE &H8001, &H10: POKE &H8002, &H80: POKE &H8003, &H60
POKE &H8010, 5
CALL ABSOLUTE(&H8000)
ck PEEK(&H8010), 6, "CALL ABSOLUTE: INC $8010"
SYS "BANKS_FREE", b, 1: RREG , , , st
ck st AND 1, 0, "BANKS_FREE"
ck FRE() > 10000, -1, "FRE()"
' The clock
SYS "TICKS": RREG l, h: t1 = h * 256 + l
SLEEP 0.5
SYS "TICKS": RREG l, h: d = h * 256 + l - t1: IF d < 0 THEN d = d + 65536
ck d >= 98 AND d <= 103, -1, "SLEEP 0.5: 100 ticks"
t = TIMER
SLEEP 0.25
d = TIMER - t: IF d < 0 THEN d = d + 86400
ck d >= 0.25 AND d <= 0.3, -1, "TIMER: a quarter of a second"
ck DENOMINATOR(t * 200), 1, "TIMER: to a tick"
d$ = DATE$: t$ = TIME$
ck LEN(d$) = 10 AND MID$(d$, 5, 1) = "-" AND MID$(d$, 8, 1) = "-", -1, "DATE$: yyyy-mm-dd"
ck LEN(t$) = 8 AND MID$(t$, 3, 1) = ":" AND MID$(t$, 6, 1) = ":", -1, "TIME$: hh:mm:ss"
' The environment, the arguments
cks ENV$("greet"), "hi", "ENV$"
cks ENVIRON$("greet"), "hi", "ENVIRON$"
cks ENV$("nosuch"), "", "ENV$ not there"
ENVIRON "mine=yes"
cks ENV$("mine"), "yes", "ENVIRON"
cks ARG$(0), "hydra.bas", "ARG$(0)"
cks ARG$(1) + "/" + ARG$(2), "one/two", "ARG$"
cks ARG$(3) + ARG$(255), "", "ARG$ past them"
cks COMMAND$, "one two", "COMMAND$"
' The shell
SHELL "echo from rc >sh.txt"
ck STATUS, 0, "SHELL: STATUS 0"
OPEN "sh.txt" FOR INPUT AS #1: LINE INPUT #1, a$: CLOSE #1
cks a$, "from rc", "SHELL's output"
SHELL "exit 3"
ck STATUS, 3, "STATUS: exit 3"
cks SHELL$("echo a  b"), "a b", "SHELL$"
OPEN "st.bas" FOR OUTPUT AS #1: PRINT #1, "END 3": CLOSE #1
SHELL "basic st.bas"
ck STATUS, 3, "a script's END 3: its status"
OPEN "st.bas" FOR OUTPUT AS #1: PRINT #1, "x = 1 / 0": CLOSE #1
SHELL "basic st.bas >[2] /dev/null"
ck STATUS, 1, "a script's error: status 1"
KILL "st.bas"
cks SHELL$("echo $mine"), "yes", "ENVIRON's for rc too"
KILL "sh.txt"
PRINT "hydra:"; checks; "checks,"; failed; "failed"
