' catch.bas - catch the falling stars in a basket: J moves it left, L right; Q stops the game
RANDOMIZE TIMER
W = 30 : H = 14
X = 15 : SCORE = 0 : LIVES = 3 : SPEED = 0.3
CLS
PRINT "Catch the stars!  J: left   L: right   Q: stop"
BASKET
DO
  SX = INT(RND * (W - 2)) + 2 : SY = 3
  DO
    LOCATE SY, SX : PRINT "*";
    T = TIMER
    DO WHILE TIMER - T < SPEED
      K$ = UCASE$(INKEY$)
      IF K$ = "J" AND X > 2 THEN X = X - 1 : BASKET
      IF K$ = "L" AND X < W - 1 THEN X = X + 1 : BASKET
      IF K$ = "Q" THEN LIVES = 0 : EXIT DO
    LOOP
    LOCATE SY, SX : PRINT " ";
    SY = SY + 1
  LOOP UNTIL SY = H OR LIVES = 0
  IF LIVES > 0 THEN
    IF ABS(SX - X) <= 1 THEN
      SCORE = SCORE + 1
      BEEP
    ELSE
      LIVES = LIVES - 1
    END IF
  END IF
  SPEED = SPEED * 0.9
  LOCATE H + 2, 1 : PRINT "Score:"; SCORE; "   Lives:"; LIVES; "  "
LOOP UNTIL LIVES = 0
LOCATE H + 4, 1 : PRINT "Game over! You caught"; SCORE; "stars."
END

' The basket, \_/, at column X of row H (the line cleared first)
SUB BASKET
  SHARED X, H, W
  LOCATE H, 1 : PRINT SPACE$(W + 2);
  LOCATE H, X - 1 : PRINT "\_/";
END SUB
