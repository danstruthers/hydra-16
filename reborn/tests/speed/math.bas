' math.bas - the math functions, 20 calls each, after a first SIN and EXP (sim/speed.js)
POKE &H6F, 1: Y = SIN(1)
POKE &H6F, 2: Y = EXP(1)
POKE &H6F, 3: FOR I = 1 TO 20: Y = EXP(I / 10): NEXT
POKE &H6F, 4: FOR I = 1 TO 20: Y = SIN(I): NEXT
POKE &H6F, 5: FOR I = 1 TO 20: Y = LOG(I): NEXT
POKE &H6F, 6: FOR I = 1 TO 20: Y = SQR(I): NEXT
POKE &H6F, 7: FOR I = 1 TO 20: Y = ATN(I): NEXT
POKE &H6F, 8: FOR I = 1 TO 20: Y = I ^ 0.37: NEXT
POKE &H6F, 9: DIGITS 30: FOR I = 1 TO 20: Y = SIN(I): NEXT
POKE &H6F, 0: PRINT Y
