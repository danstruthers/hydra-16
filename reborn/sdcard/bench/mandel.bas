' Mandelbrot - after BBC BASIC's "FP + Strings" benchmark: HyBASIC's 32-bit integers (\ and MOD), no line numbers
PRINT "Mandelbrot - HyBASIC - integers + strings"
PRINT "Start"
t0 = TIMER
z$ = ".,'~=+:;*%&$OXB#@ "
f = 50
FOR y = -12 TO 12
  FOR x = -49 TO 29
    c = x * 229 \ 100
    d = y * 416 \ 100
    a = c: b = d: i = 0
    DO
      q = b \ f: s = b MOD f
      t = (a * a - b * b) \ f + c
      b = 2 * (a * q + a * s \ f) + d
      a = t: p = a \ f: q = b \ f
      IF p * p + q * q >= 5 THEN EXIT DO
      i = i + 1
    LOOP WHILE i < 16
    IF i < 16 THEN PRINT MID$(z$, i + 1, 1); ELSE PRINT " ";
  NEXT x
  PRINT
NEXT y
el = TIMER - t0
PRINT "Finished"
PRINT "Time: "; FIXED(el, 2); " secs."
