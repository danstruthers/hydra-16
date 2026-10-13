' double.bas - how many doublings to a million?
N = 1
COUNT = 0
DO WHILE N < 1000000
  N = N * 2
  COUNT = COUNT + 1
LOOP
PRINT COUNT; "doublings make"; N
