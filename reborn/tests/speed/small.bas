' small.bas - one operation 200 times a stretch, BASIC's cycles a pass (sim/speed.js)
A = 1.5: B = 2.25: C = 1 / 3: D = 1 / 7: E = 123456789012: F = 987654321098: G = 3: DIM AR(200)
POKE &H6F, 1: FOR I = 1 TO 200: NEXT
POKE &H6F, 2: FOR I = 1 TO 200: X = I: NEXT
POKE &H6F, 3: FOR I = 1 TO 200: X = I + G: NEXT
POKE &H6F, 4: FOR I = 1 TO 200: X = A + B: NEXT
POKE &H6F, 5: FOR I = 1 TO 200: X = A * B: NEXT
POKE &H6F, 6: FOR I = 1 TO 200: X = I / G: NEXT
POKE &H6F, 7: FOR I = 1 TO 200: X = C + D: NEXT
POKE &H6F, 8: FOR I = 1 TO 200: X = E * F: NEXT
POKE &H6F, 9: FOR I = 1 TO 200: IF A < B THEN X = 1
NEXT
POKE &H6F, 10: FOR I = 1 TO 200: X = INT(A): NEXT
POKE &H6F, 11: FOR I = 1 TO 200: X = E + F: NEXT
POKE &H6F, 12: FOR I = 1 TO 200: X = I \ G: NEXT
POKE &H6F, 13: FOR I = 1 TO 200: AR(I) = I: NEXT
POKE &H6F, 14: FOR I = 1 TO 200: X = A: NEXT
POKE &H6F, 15: FOR X = 0 TO 20 STEP 0.1: NEXT
POKE &H6F, 0: PRINT X
