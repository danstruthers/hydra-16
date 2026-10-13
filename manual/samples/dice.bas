' dice.bas - throw two dice
RANDOMIZE TIMER
A = INT(RND * 6) + 1
B = INT(RND * 6) + 1
PRINT A; B; "makes"; A + B
