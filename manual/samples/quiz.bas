' quiz.bas - a quiz: its questions and answers are in the DATA lines
SCORE = 0
READ COUNT
FOR Q = 1 TO COUNT
  READ QUESTION$, ANSWER$
  PRINT Q; QUESTION$
  LINE INPUT "   Your answer: "; REPLY$
  IF UCASE$(REPLY$) = UCASE$(ANSWER$) THEN
    PRINT "   Right!"
    SCORE = SCORE + 1
  ELSE
    PRINT "   No: it's "; ANSWER$; "."
  END IF
NEXT Q
PRINT
PRINT "You scored"; SCORE; "out of"; COUNT

DATA 4
DATA "What chip is the Hydra's processor?", "65C02"
DATA "How many tasks can a Hydra run at once?", "16"
DATA "CTRL and which key stops a program?", "C"
DATA "What is the Hydra's shell called?", "HyForth"
