# Lab 23. Three Languages

> **THIS LAB:** Manual: Chapter 25. About an hour. You need a Hydra, and the Learning chapter of at least one
> language behind you.

<p class="fields">Name <span></span> Date <span class="short"></span></p>

### Do

1. Type in the guessing game in your language (section 25.2, 25.3 or 25.4), as a file in `/sd/0/work`, and run
   it. Play five games. ✎ Write how many guesses each took.

| Game | 1 | 2 | 3 | 4 | 5 |
|---|---|---|---|---|---|
| Guesses | | | | | |

2. ✎ What's the best way to play? With that way, what's the *most* guesses you could ever need for a number from
   1 to 100? Why?

<div class="write l3"></div>

3. Change the game: give the player only seven guesses. If they run out, say what the number was. ✎ Write the
   lines you changed or added.

<div class="write l6"></div>

4. Now read the game in the two languages you *didn't* type. For each, find the line that does each job, and copy
   it.

| The job | In (language) | In (language) |
|---|---|---|
| Chooses the secret number | | |
| Reads your guess | | |
| Says "Too small." | | |
| Goes round again | | |
| Counts the guesses | | |

5. The race (section 25.6). Run the counting race in all three languages on *your* Hydra. ✎ Write the times.

| Language | Time to count to 10,000 | Times slower than the fastest |
|---|---|---|
| HyForth | | |
| hylang | | |
| HyBASIC | | |

6. ✎ Change each race to count to 40,000 instead. Predict the times first. Were you right? (One of the three will
   surprise you with the number it prints. Which, and why? Section 19.3's note has the reason.)

<div class="write l3"></div>

### Questions

1. ✎ hylang's game has no loop. How does it go round again?

<div class="write l2"></div>

2. ✎ HyForth's `ask` is the longest of the three ways to read a number. What do HyBASIC and hylang do for you that
   HyForth makes you do yourself?

<div class="write l2"></div>

3. ✎ Why is HyForth so much quicker in the race? What do you give up for that speed?

<div class="write l2"></div>

4. ✎ "The time that matters is yours." What does the manual mean by that?

<div class="write l2"></div>

5. ✎ Which language would you choose for each of these, and why: a quiz for your class; working out the 200th
   Fibonacci number; flashing a light a thousand times a second?

<div class="write l4"></div>

### Check yourself

| ☐ | I can |
|---|---|
| ☐ | read a small program in a language I haven't learned, and say roughly what each line does |
| ☐ | time a piece of a program |
| ☐ | give a reason for choosing one language over another |

> **CHALLENGE:** ★ Turn the game round: *you* think of a number, and the Hydra guesses. After each guess you type
> `s` (too small), `b` (too big) or `r` (right). The Hydra should never need more than seven guesses. Write it in
> your language.
