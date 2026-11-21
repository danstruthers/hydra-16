' Mandelbrot - after BBC BASIC's "FP + Strings" benchmark: HyBASIC's exact numbers (/ gives a fraction, FIX makes it
' whole), no line numbers.  The same picture as mandel.bas, which keeps to 32-bit integers (\ and MOD)
PRINT "Mandelbrot - HyBASIC - exact numbers + strings"
PRINT "Start"
t0 = TIMER
z$ = ".,'~=+:;*%&$OXB#@ "
f = 50
FOR y = -12 TO 12
  FOR x = -49 TO 29
    c = FIX(x * 229 / 100)
    d = FIX(y * 416 / 100)
    a = c: b = d: i = 0
    DO
      q = FIX(b / f): s = b - (q * f)
      t = FIX(((a * a) - (b * b)) / f) + c
      b = 2 * ((a * q) + FIX(a * s / f)) + d
      a = t: p = FIX(a / f): q = FIX(b / f)
      IF ((p * p) + (q * q)) >= 5 THEN EXIT DO
      i = i + 1
    LOOP WHILE i < 16
    IF i < 16 THEN PRINT MID$(z$, i + 1, 1); ELSE PRINT " ";
  NEXT x
  PRINT
NEXT y
el = TIMER - t0
PRINT "Finished"
PRINT "Time: "; FIXED(el, 2); " secs."
