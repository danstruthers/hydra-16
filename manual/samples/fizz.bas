' fizz.bas - the game of FizzBuzz: count, but say Fizz for the threes,
' Buzz for the fives, and FizzBuzz for both
FOR N = 1 TO 15
  IF N MOD 15 = 0 THEN
    PRINT "FizzBuzz ";
  ELSEIF N MOD 3 = 0 THEN
    PRINT "Fizz ";
  ELSEIF N MOD 5 = 0 THEN
    PRINT "Buzz ";
  ELSE
    PRINT N;
  END IF
NEXT N
PRINT
