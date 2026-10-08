' numb.bas - BASIC's number work, by stretch (sim/speed.js: POKE &H6F, k starts the kth, 0 ends them)
POKE &H6F, 1: X = 0: FOR I = 1 TO 2000: X = X + 1: NEXT
POKE &H6F, 2: X = 0: FOR I = 1 TO 500: X = X + 0.1: NEXT
POKE &H6F, 3: X = 0: FOR I = 1 TO 300: X = X + I / 7: NEXT
POKE &H6F, 4: X = 1: FOR I = 1 TO 300: X = X * 1.5: X = X / 1.5: NEXT
POKE &H6F, 5: X = 1: FOR I = 1 TO 100: X = X * 2: NEXT
POKE &H6F, 6: FOR I = 1 TO 100: Y = SQR(I): NEXT
POKE &H6F, 7: FOR I = 1 TO 50: Y = SIN(I): NEXT
POKE &H6F, 8: FOR I = 1 TO 50: Y = EXP(I / 10): NEXT
POKE &H6F, 9: FOR I = 1 TO 50: Y = LOG(I): NEXT
POKE &H6F, 10: FOR I = 1 TO 300: S$ = STR$(I * 1.5): NEXT
POKE &H6F, 11: FOR I = 1 TO 300: Y = VAL("123.25"): NEXT
POKE &H6F, 0: PRINT X; Y; S$
