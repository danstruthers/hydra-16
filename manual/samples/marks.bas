' marks.bas - the average of some marks
DIM MARK(5)
TOTAL = 0
FOR I = 1 TO 5
  READ MARK(I)
  TOTAL = TOTAL + MARK(I)
NEXT I
PRINT "The average is"; TOTAL / 5
DATA 7, 9, 4, 8, 10
