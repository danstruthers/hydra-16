' guess.bas - guess the number, in HyBASIC
RANDOMIZE TIMER
SECRET = INT(RND * 100) + 1
TRIES = 0
PRINT "I'm thinking of a number from 1 to 100."
DO
  INPUT "Your guess"; GUESS
  TRIES = TRIES + 1
  IF GUESS < SECRET THEN
    PRINT "Too small."
  ELSEIF GUESS > SECRET THEN
    PRINT "Too big."
  END IF
LOOP UNTIL GUESS = SECRET
PRINT "Right! You took"; TRIES; "guesses."
