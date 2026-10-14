# Topic 15. The Projects

For each of the Companion's eight projects: the answers to "Understand it first", what a good finished project
looks like, the snags to expect, and a worked solution to one or two of the extensions. Topic 7 has the rubric.

A general rule for all seven: **the starting listing is not the project.** A student who has typed it in and run it
has done the equivalent of a lab. The project is what they do to it next.

## Project 1. A Quiz

**Understand it first.**

1. It's the number of questions: `READ COUNT` takes it, and the loop runs that many times. Add a question, and the
   4 must become a 5, or the new one is never asked. (A better design, and a good extension: end the data with a
   marker, such as `"end"`, and loop until it's read.)
2. So that capitals don't matter: `65c02`, `65C02` and `65c02` typed with the shift key stuck all count.
3. `INPUT` treats a comma as the end of an answer, so "Paris, France" would be cut short. `LINE INPUT` takes the
   whole line, commas and all.

**Look for** questions the student cares about; messages that have a voice; and input that's been thought
about (a blank answer, an answer with spaces round it: `LTRIM$` and `RTRIM$`).

**Expect** the `DATA` count to be wrong after questions are added (an error from `READ`, or a question that's never
asked); and
commas *inside* a question, which split it in two unless the question is in quotes.

**A worked extension**: an answer that's right if it's anywhere in the reply.

```session
> A$ = "it is the 65c02, I think" : B$ = "65C02"
> IF INSTR(UCASE$(A$), UCASE$(B$)) > 0 THEN PRINT "Right!" ELSE PRINT "No."
Right!
```

In the program, that's one changed line:
`IF INSTR(UCASE$(REPLY$), UCASE$(ANSWER$)) > 0 THEN`.

## Project 2. Turtle Pictures

**Understand it first.**

1. 120 degrees for a triangle; 60 for a hexagon. The rule: the turtle turns through a full circle, 360 degrees, on
   its way round any closed shape, so each corner is 360 divided by the number of sides. In `polygon`, that's
   `360 over /`.
2. For `60 4 polygon`:

| The word | The stack afterwards |
|---|---|
| (to begin with) | 60 4 |
| `360` | 60 4 360 |
| `over` | 60 4 360 4 |
| `/` | 60 4 90 |
| `swap` | 60 90 4 |
| `0` | 60 90 4 0 |
| `do` | 60 90 (the loop has taken the 4 and the 0) |

3. It chooses the pen's colour from the loop's count: `i 15 mod` runs from 0 to 14 and round again, and `1+`
   makes that 1 to 15. Without the `1+`, one square in fifteen would be drawn in colour 0, which is black, and
   wouldn't be seen.

**Look for** words built on words (`flower` uses `square` uses `polygon`), a stack diagram on every one, and a
picture that the student planned rather than stumbled on.

**Expect** stack trouble inside loops: a word that leaves a number behind each time round overflows the stack by
the end of a flower. The cure is the stack diagram, checked against the word a step at a time, as in the table
above.

**Worked extensions.**

```text
: star ( size -- ) 5 0 do dup fd 144 rt loop drop ;
: spiral ( turn -- ) 100 1 do i 2 * fd dup rt loop drop ;
cs 80 star
cs 91 spiral
```

A five-pointed star turns 144 degrees at each point, because it goes round *twice* on its way: 720 divided by 5.
That's the answer to the challenge's "why": a turn that divides evenly into 360 makes a polygon, one that
divides into a multiple of 360 makes a star, and one that divides into neither never closes, and makes a
spiral that slowly rotates.

## Project 3. A Round for Two Voices

**Understand it first.**

1. Line `B` starts with `r1 r1`, two whole-note rests, and it has another instrument. So the second voice comes
   in two bars after the first, with the same tune.
2. `l4`: notes are quarter notes unless they say otherwise. `g2`: a G for a half note. `g8`: a G for an eighth.
   `[c d e c]2`: play what's in the brackets twice. `<g>`: down an octave, a G, and up again. `r1`: a rest for a
   whole note, four beats.
3. Four eighths are two beats, and two quarters are two more: four beats. A whole bar.
4. 73 is the flute and 71 the clarinet (General MIDI's numbers, counted from 0: section 15.4).

**Look for** bars that add up, in every voice, all the way through; a score that's laid out so the bars can be
seen (the `|` is there for that); and, in a tune of the student's own, some shape: a phrase that comes back.

**Expect** voices that drift apart, from a bar with too many or too few beats in it. Have the student count each
bar aloud. `play -o` will turn a score into a ZSM file without playing it, which catches spelling mistakes in
the MML (it reports the line) before a class of thirty all press play.

**A worked extension**: a third voice, two bars later still, and each voice in its own speaker.

```text
A p l I73 o4 l4 [c d e c]2 [e f g2]2 [g8 a8 g8 f8 e c]2 [c <g> c2]2
B p r I71 o4 l4 r1 r1 [c d e c]2 [e f g2]2 [g8 a8 g8 f8 e c]2 [c <g> c2]2
C p c I0 o3 l4 r1 r1 r1 r1 [c d e c]2 [e f g2]2 [g8 a8 g8 f8 e c]2 [c <g> c2]2
```

## Project 4. Your Own Command

**Understand it first.**

1. `$*` is the list of the script's arguments, and `$#*` is how many there are: 0, when `jot` is typed alone.
2. It runs `date`, and puts its output into the line as words: the date, and the time. `>>` adds the line to the
   end of the notebook. `>` would empty the notebook first, every time.
3. Into `/ram/bin`: new files in `/bin` go to the RAM disk, and are gone at the next reset. To keep the command,
   make the directory `/sd/0/bin` and put the script there. It joins `/bin` when the namespace is next built: at
   a reset, or straight away with the command `newns`.

**Look for** a first comment that says how to use the command; behaviour that's sensible when the notebook
doesn't exist yet; and a usage message.

**Expect** the usage message to want quotes. In rc, `[` and `]` make a pattern, and `[-n]` would be replaced by the
name of any file in the current directory that it matched (when nothing matches, rc leaves the word alone, so the
mistake hides until it bites). Keep the message in quotes. It's a good moment to ask what else rc has been doing to
their words all along.

**A worked solution**, with three options and a usage line:

<!-- listing samples/jot2 -->

```text
#!/bin/rc
# jot - a notebook.  jot words ... adds a line; jot alone shows them all;
# jot -n counts them; jot -l shows the last five; jot -f word finds the ones with a word in
book=/sd/0/work/notebook
if(~ $#* 0) {
	cat $book
	exit
}
switch($1){
case -n
	wc -l <$book
case -l
	tail -5 $book
case -f
	grep $2 $book
case -*
	echo 'usage: jot [-n] [-l] [-f word] [words ...]'
case *
	echo `{date} $* >>$book
}
```

<!-- session
mkdir /sd/0/work
cd /ram
!file samples/jot2
rc jot2 Feed the cat
rc jot2 Buy solder
-->

```session
/ram> rc jot2 -n
      2
/ram> rc jot2 -f solder
2000-01-01 00:01:12 Buy solder
/ram> rc jot2 -z
usage: jot [-n] [-l] [-f word] [words ...]
```

## Project 5. A Program in the Background

**Understand it first.** The two lines of `ps` are an `rc`, running the script, in the state `wait`; and a `sleep`,
its child, in the state `sleep`. The script spends nearly all its time waiting for its `sleep` to finish.

1. The kernel gives each task that's ready a short turn at the processor, two hundred times a second, and takes
   it away again at the next tick. The shell gets turns between the ticker's.
2. Almost none: a task that's asleep or waiting isn't given turns at all. The `cpu` column for both stays at 0.0.
3. <kbd>CTRL</kbd>-<kbd>C</kbd> at the prompt goes to the program in the foreground, and a command started with
   `&` isn't that: the ticker carries on. A reset stops everything, the ticker with it.

**Look for** a student who can start it, find it and stop it without help, and who can say what `&` does. The
written explanation in the "two tasks, one file" step is the heart of it.

**Expect** forgotten tickers: three of them still running, a week later, in a student's long-lived emulator.
`ps` at the start of each lesson is a good habit. And expect "which number do I kill?": the `rc`, not the `sleep`
(kill the `sleep`, and the loop just starts the next one).

**Two tickers, one file.** In practice their lines don't get mixed: each `date` writes its whole line in one go, and
the file is opened for adding to its end. That's the contrast with `chorus`, where each task writes a *letter* at a
time. Whether tasks tread on each other depends on how big a step each one takes with the thing they share:
which is the whole idea of a lock.

**The challenge** (`race`). Adding 1 takes three steps: read the counter, add 1, write it back. If one task is
stopped after its read, and another task then reads the same value, both write back the same number, and two
additions have counted as one. A lock makes the three steps into one: a task that holds it finishes its read,
add and write before any other task can begin.

## Project 6. A Traffic Light

**Understand it first.**

1. `on 2` writes a 0 to `/dev/gpio/2`. The LEDs are wired from +5 V to the pins, so current flows, and an LED
   lights, when its pin is *low*.
2. It makes pins 2, 3 and 4 outputs, and switches all three LEDs off, so that the lights start from a known
   state whatever was there before.
3. Red, for 4 seconds; red and amber together, for 1; green, for 4; amber, for 1; and round again.

**Before any power goes on,** check each circuit yourself (Topic 4, and the lesson notes for Lab 14): a resistor
for every LED, nothing on the header's pins 3 and 4, and no stray wire across +5 V and ground.

**Look for** a drawing that matches the wiring; a `lightsoff`; and, in the crossroads version, a student who has
*argued* that the two roads can never both be green, and not just watched it for a while.

**Expect** an LED in backwards (it simply doesn't light: turn it round), and lights left on when the script is
stopped with <kbd>CTRL</kbd>-<kbd>C</kbd> half way through.

**A worked extension**: `lightsoff`.

```text
#!/bin/rc
# lightsoff - put the traffic light out
for(p in 2 3 4) echo 1 >/dev/gpio/$p
```

## Project 7. Faster, with Machine Code

**Understand it first.**

1. On the manual's Hydra, about fifty times: 2.7 seconds against a twentieth of a second.
2. With two registers together: X is the low byte of a 16-bit count, and Y the high byte.
3. `inx` adds 1 to X every time round. `iny` happens only when X goes round from $FF to $00: the 256th time.
4. 10,000 is $2710: the count has reached it when the low byte is $10 and the high byte is $27. X is $10 once in
   every 256 counts, so the routine must also check that Y is $27: only then is it the right $10.
5. It copies the registers, as the routine left them, into four HyBASIC variables. `Y * 256 + X` puts the two
   bytes back together: the high byte is worth 256 times as much.

**Look for** routines that have been run by hand, on paper, before they were typed; a comment on every line; and
a student who can say what each register is for.

**Expect** crashes. A machine-code routine that never reaches its `rts`, or that writes where it shouldn't, can
take HyBASIC down with it, and sometimes needs a reset. That's normal at this level, and it's why the work
should be saved before every run. A loop that never ends is stopped with <kbd>CTRL</kbd>-<kbd>C</kbd> as usual.

**A worked extension**: the number to count to, passed in. The registers arrive already loaded, so the routine
keeps the target in two bytes of a HyBASIC variable, and counts until it gets there.

```text
T = 0
LIMIT = 50000
ASM
count:  stx t + 1       ; The target's low and high bytes, kept in the variable T
        sty t + 2
        ldx #0
        ldy #0
again:  inx
        bne skip
        iny
skip:   cpx t + 1
        bne again
        cpy t + 2
        bne again
        rts
END ASM
CALL ASM count, 0, LIMIT MOD 256, LIMIT \ 256
```

**The challenge.** A loop that runs once at start-up: HyBASIC, since nobody will notice the time. A loop over
every dot of a picture (76,800 of them): machine code, or the wait is minutes. A program to run once and throw
away: HyBASIC, every time. The time that matters is the programmer's.

## Project 8. A Game: Catch the Stars

**Understand it first.**

1. `INPUT` waits for a whole line and <kbd>RETURN</kbd>: the star would hang in the air until the player typed one.
   `INKEY$` looks, and goes on at once, so the star keeps falling whether a key has been pressed or not.
2. It waits for the star's next step (`SPEED` seconds), and while it waits, it reads the keys and moves the
   basket. A smaller `SPEED` is a shorter wait between steps, so the star falls faster: each star's is 9/10 of the
   last one's.
3. A SUB's variables are its own (section 23.12). Without `SHARED`, `X`, `H` and `W` inside `BASKET` are new
   variables, all 0, and `LOCATE 0, ...` stops the program with `illegal function call`.
4. The basket's middle is column `X`, and it covers `X - 1` to `X + 1`: a star in any of those three columns is
   caught.
5. So that the star doesn't leave a trail. The screen keeps whatever was printed on it until something is printed
   over it.

**Look for** a game that someone else can play from what's on the screen, and a screen that's tidy as it's played.
The "1 star" change is a small `IF`, and a good first one. The arrow keys need the student to find out what
`INKEY$` gives for them (two characters), which is worth doing at the prompt first:
`10 K$ = INKEY$ : IF K$ = "" THEN 10`, then `20 PRINT LEN(K$), ASC(K$), ASC(RIGHT$(K$, 1))`.

**Expect** a basket that flickers or lags on a slow terminal (each move prints a whole row); students who move the
basket by more than one column at a time, and then wonder why it leaves pieces behind; and a SUB written without
`SHARED`. The game's timing is in seconds, so it plays the same on a real Hydra and in the emulator at **Real time**,
but faster than intended at **As fast as it goes**.

**A worked extension**: the arrow keys, and "1 star".

```text
      K$ = UCASE$(INKEY$)
      IF K$ = CHR$(0) + "K" THEN K$ = "J"
      IF K$ = CHR$(0) + "M" THEN K$ = "L"
      ...
IF SCORE = 1 THEN S$ = "star." ELSE S$ = "stars."
LOCATE H + 4, 1 : PRINT "Game over! You caught"; SCORE; S$
```

