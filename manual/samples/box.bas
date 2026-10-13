' box.bas - boxes of stars
BOX 3, 5
BOX 2, 10
END

SUB BOX (ROWS, COLS)
  FOR R = 1 TO ROWS
    PRINT STRING$(COLS, "*")
  NEXT R
  PRINT
END SUB
