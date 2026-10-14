# Project 8. A Game: Catch the Stars

> **THIS PROJECT:** HyBASIC. After Labs 10, 20 and 21. Two or three sittings. You'll use `INKEY$`, which reads a key
> without waiting for one; `LOCATE`, which puts the cursor anywhere on the screen; `TIMER` and `RND`; and a SUB that
> shares the main program's variables. It runs on any terminal: no Vera X needed.

<p class="fields">Name <span></span> Started <span class="short"></span> Finished <span class="short"></span></p>

### The starting point

Stars fall from the top of the screen, one at a time. Move your basket, `\_/`, under each one: <kbd>J</kbd> moves it
left and <kbd>L</kbd> right. Catch a star and you score; miss three, and the game's over. Each star falls a little
faster than the one before. Type this into `/sd/0/work/catch.bas` with `edit`, and play it with
`basic /sd/0/work/catch.bas`.

<!-- listing samples/catch.bas -->

```text
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
```

While you play, the screen looks like this:

```text
Catch the stars!  J: left   L: right   Q: stop

                    *



              \_/

Score: 4    Lives: 2
```

> **TIP:** In the web emulator, click in the serial console first, so that your keys go to the Hydra.

### Understand it first

1. ✎ `INKEY$` gives the key that's been pressed, or `""` if none has, and it doesn't wait. What would happen to the
   falling star if the game used `INPUT` instead?

<div class="write l2"></div>

2. ✎ The loop `DO WHILE TIMER - T < SPEED` does two jobs at once. What are they? Why does the star fall faster when
   `SPEED` is smaller?

<div class="write l2"></div>

3. ✎ `BASKET` is a SUB, called by its name alone. Why does it need the line `SHARED X, H, W`? (Take the line out,
   and see what HyBASIC says.)

<div class="write l2"></div>

4. ✎ A star is caught if `ABS(SX - X) <= 1`. Draw the basket, and mark the columns where a star counts as caught.

<div class="space s1"></div>

5. ✎ Why does the program print a space where the star was, before it moves the star down?

<div class="write"></div>

### Make it yours

Do at least four of these. Tick each when it works.

| ☐ | Change |
|---|---|
| ☐ | Make the last line say "1 star", not "1 stars" |
| ☐ | Let the arrow keys move the basket too. An arrow key comes from `INKEY$` as two characters: `CHR$(0)`, then `K` for ← or `M` for → |
| ☐ | Keep the best score in a file, and say when it's been beaten (section 24.1) |
| ☐ | Now and then, drop a `+` instead of a `*`: it's worth 5, and falls twice as fast |
| ☐ | Give the stars, the basket and the score colours of their own (`COLOR`, section 24.2) |
| ☐ | Play a happy note for a catch and a sad one for a miss (`SOUND`, section 24.5), instead of the `BEEP` |
| ☐ | Levels: after every 10 stars, a message, and the next level starts faster |
| ☐ | Two stars falling at once (you'll need arrays: `DIM SX(2), SY(2)`) |

✎ Which did you do? What was the hardest part, and how did you get past it?

<div class="write l5"></div>

### Show it

| ☐ | Before you call it finished |
|---|---|
| ☐ | Someone else has played it, without your help, and found the keys from the screen |
| ☐ | Nothing is left on the screen that shouldn't be: no trails of stars, no bits of old baskets |
| ☐ | The program has comments, and its loops and SUB are indented |
| ☐ | It's saved in `/sd/0/work` |

> **CHALLENGE:** ★ With a Vera X, move the game to the bitmap (section 24.6): draw the stars with `CIRCLE` and the
> basket with `LINE`, in colour. Or make a different game of your own with the same parts: a key read without
> waiting, a timer, a score, and a SUB for each thing that's drawn.
