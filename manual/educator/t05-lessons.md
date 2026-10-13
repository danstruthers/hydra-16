# Topic 5. Lesson by Lesson

Notes for each of the Companion's twenty-six labs: what it's really for, how to open it, where students get
stuck, and a question to end on. The answers are in Part Three.

## The shape of a lab lesson

The same shape works for nearly all of them, in a 50-minute lesson:

| Minutes | What | Why |
|---|---|---|
| 5 | **Open** with a demonstration or a question, at the front (each lab's notes suggest one) | To give the lab a reason |
| 5 | **Predict.** Students read the first table of the lab, and fill in the "guess" column, in pencil, before touching a key | The guess is where the learning is. A wrong guess, corrected by the machine, sticks |
| 25 | **Do.** Students work through the steps. You circulate | |
| 10 | **Questions**, in writing, alone | To find out who understood, and who only typed |
| 5 | **Close**: one student shows one thing; you ask the closing question | To hear it said aloud |

Two habits make the difference between a class that learns from labs and a class that types them:

* **Pencils down, then keys.** Every lab has a prediction to make. Insist on it being written first.
* **"What did it say, exactly?"** When a student says "it doesn't work", don't look at the screen. Ask them to
  read you the message. Nine times in ten, reading it aloud is the fix.

Reading comes before the lesson, not during it. The Companion names the sections; set them as homework, and open
with a two-minute check that they were read.

## Labs 1 to 3: meeting the Hydra

### Lab 1. Meet the Computer

**It's for** the vocabulary (processor, memory, bus, ROM, RAM), and hexadecimal, which everything later leans on.

**Open with** a real board, or the manual's Figure 1-1 on the projector, and the question "which of these chips is
the computer?" (None of them is; all of them are.)

**Watch for** hex, which is new and slippery. The commonest error is reading `$10` as ten. Have the class count
aloud from `$08` to `$12`. Students who convert by long division get lost; the manual's way, a hex digit for each
four bits, is quicker and safer.

**Close with** "why do programmers bother with hex at all?" (Because two hex digits are exactly one byte.)

### Lab 2. A Hydra on Your PC

**It's for** getting every student to a prompt, and the one fact that saves a term's work: where files survive.

**Open with** nothing. Let them open the file and watch the start-up messages scroll. Then ask what they noticed.

**Watch for** the experiment in step 4. Students skip remaking the files after a reset, and then report that the
SD card "lost" one. Step 4 is also where the lesson's main point lands, so don't let it be rushed: take the
class's results on the board.

**Before the lesson**, read Topic 3's warning about browser storage, and decide how students will keep their
cards.

**Close with** "where will you keep your work, from now on?"

### Lab 3. First Steps

**It's for** everything else. A student who can't do this lab can't do any other. Give it two lessons if it needs
them.

**Open with** a deliberate mistake at the projector: type `dat`, and ask the class what the Hydra is telling you.
Establish at once that an error message is information, not a telling-off.

**Watch for:**

* Capitals. Students arriving from phones type `Ls` and `Date`. The Hydra says `not found`, and they conclude that
  the Hydra is broken.
* The unfinished definition in step 5. A student who types `: test` and then anything else is inside HyForth's
  compiler, with a prompt of spaces, and every command they try "does nothing". Teach the cure (`;`) to the
  whole class, because half of them will meet this by accident within the week.
* `edit`. Some terminals and browsers grab <kbd>CTRL</kbd>-<kbd>O</kbd> or <kbd>CTRL</kbd>-<kbd>W</kbd> for
  themselves. Try the editor on the students' own computers beforehand; in the web emulator, click in the
  console first.
* Saving into `/ram`. They will. Let one student lose a file to a reset, in public, early: it's the cheapest
  lesson of the course.

**Close with** "you've typed a command and nothing happens. What three things do you try?" (<kbd>CTRL</kbd>-<kbd>C</kbd>;
<kbd>CTRL</kbd>-<kbd>D</kbd>; look at the prompt.)

## Labs 4 to 9: inside HydraOS

### Lab 4. From Power to Prompt

**It's for** the idea of a task, and the picture of the system as a few programs co-operating.

**Open with** the terminal emulator at the projector, and <kbd>CTRL</kbd>-<kbd>A</kbd> <kbd>S</kbd>: every task and
what it's doing, live. Or with `top` on any Hydra.

**Watch for** the task numbers in step 4. The lab says `kill 2`, but the number is whatever *their* `ps` shows,
and killing the wrong one is instructive: kill the shell (task 1), and another starts (the kernel starts `init` again, which becomes it). Let it happen, then
ask why.

The reading (Chapter 7) is the hardest in Part Two. Don't set section 7.4 for everyone: the lab and its questions
need only 7.1 to 7.3 and 7.5.

**Close with** "is the Hydra really doing sixteen things at once?" (No: one thing at a time, two hundred turns a
second.)

### Lab 5. Talking to the Shell

**It's for** variables, redirection and pipes: the grammar of a command line.

**Open with** a question: "how many programs does the Hydra have?" Take guesses, then type
`ls /rom/bin | wc -l`, and let the answer raise the next question: what was that `|`?

**Watch for:**

* The two prompts. This lab is typed at rc's `%`, and a student who forgets to type `rc` first finds that
  `pets=(cat dog fish)` gives an error from HyForth.
* Spaces around `=`. `pets = (cat dog)` isn't an assignment.
* The backquote in `` `{date} ``. It's the key at the top left of most keyboards, and half the class will type
  an apostrophe.
* **The nine-program limit** on pipes (the manual's section 8.7). Each program in a pipeline is a task, and there
  are sixteen tasks for everything, so a tenth program gets `no free task`. Nothing breaks: the rest of the
  line runs without it, and the answer is wrong or missing. It's a good moment to run `ps` and count.

**Close with** "what's the difference between `>` and `|`?" (One sends output to a file; the other to a program.)

### Lab 6. Scripts

**It's for** the step from typing commands to keeping them: the first real programming of the course, for many.

**Open with** "what do you type most often? Wouldn't you rather type one word?"

**Watch for:**

* Tabs and braces in the script. The manual's scripts put the `{` on the `for`'s line and the `}` on a line of its
  own; rc accepts other layouts, but keep to that one and the shape stays easy to see. The editor's auto-indent
  helps.
* Step 5's question has a sting: the script copied to `/bin` is really in `/ram/bin`, and is gone at the next
  reset. Let them find that out next lesson, and then teach `/sd/0/bin`. (A directory made there joins `/bin`
  the next time the namespace is built: after a reset, or `newns`.)

**Close with** "when is a script better than a function?" (When you want it tomorrow.)

### Lab 7. The Tree of Files

**It's for** paths, and the Hydra's best idea: one tree of names for everything.

**Open with** a drawing on the board of the tree as far as the class can dictate it from Lab 3, and add to it as
the lesson goes.

**Watch for** relative paths. `cd ../songs` is the first time many students meet `..`, and the commonest wrong
picture is that `..` means "back to where I was before". It means "the directory above".

Step 6 (`bind`) is the conceptual high point. A student who sees that `/mnt` showed the songs *without anything
being copied* has understood what a name is.

**Close with** "is `/dev/time` a file?" (It behaves like one. That's all "file" means, here.)

### Lab 8. Windows

**It's for** seeing multitasking, rather than being told about it.

**Open with** two windows side by side at the projector, a slow command running in one while you type in the
other.

**Watch for** the window key. <kbd>CTRL</kbd>-<kbd>]</kbd> is awkward on some keyboard layouts (on many European
layouts `]` needs another modifier), and a few terminal programs take it for themselves. Try it on the students'
machines first. (Section 10.3 lists the window key's commands; <kbd>CTRL</kbd>-<kbd>]</kbd> twice sends one through.)

Students with small browser windows can't see tiled windows properly. Set the console's size to at least 80
columns first.

**Close with** "you have two windows. How many shells are running? How many tasks?"

### Lab 9. A Command Hunt

**It's for** learning to look things up: the skill that outlasts every particular command.

**Open with** nothing but the rule: the manual is allowed, asking the teacher is not.

**Watch for** students who guess options from other systems (`head -n 3`, `grep -r`). The Hydra's commands are
smaller than Linux's, and the usage line is the authority: type the command wrongly, and it prints its usage.

This lab works well as a race in pairs, one with the manual and one at the keyboard, swapping at half time.

**Close with** "which command did you find that you didn't know you wanted?"

## Labs 10 to 14: devices

### Lab 10. Devices Are Files

**It's for** the idea that reading and writing files is the *only* interface there is.

**Open with** `echo b115200 >/dev/serctl` on a real Hydra at the projector, and the terminal turning to nonsense.
(Then change the terminal's speed, and it comes back.) A device was reconfigured by writing a word into a file.

**Watch for** step 5, the escape sequences: this is the first HyForth that students type, and they must type it
*exactly*. The space after `."` is the usual casualty. Tell them it's a spell for now; Lab 16 explains it.

**Close with** "what would you have to know to control a device nobody's told you about?" (The names of its
files, and the words its control file understands.)

### Lab 11. Disks

**It's for** what "saving" really means: where bytes go, how much room they take, and what survives.

**Open with** the question "your file is two bytes long. How much room does it take up?" and let step 3 answer.

**Watch for** the emulator: its SD card "is" a file on the PC, and students ask whether it's real. It's a good
question. The honest answer is that the Hydra can't tell, and that this is what "emulate" means.

**Close with** "which of the Hydra's disks would you trust with your project? In what order?"

### Lab 12. Sound

**It's for** fun, mostly, and it earns its place: it's the lab that students show their friends. Underneath,
it's about representing something that isn't a number (a tune) as numbers and text.

**Open with** `play /rom/songs/scom.zsm`, loud.

**Watch for:**

* No sound. In the web emulator the **Sound** button must be pressed (browsers won't play until the user
  clicks), and the speed must be **Real time**. On a real Hydra, check the plug is fully in J26.
* Headphones. Insist.
* Bars that don't add up, in step 6. A student whose two voices drift apart has miscounted beats: have them
  count each bar aloud.

Students who play an instrument will race ahead; pair them with those who don't, and let the musician dictate
while the other types.

**Close with** "what's the difference between writing down a tune and recording one?" (The score, and the ZSM
file: a description, and a list of events.)

### Lab 13. Pictures

**It's for** coordinates, and again the same idea: a picture described in text.

**Open with** the question "where is the point (0, 0)?" Most will say the bottom left, from mathematics. On a
screen it's the top left, and *y* grows downwards.

**Watch for** the house in step 5. Students who plan on squared paper first finish in half the time of those who
type and hope: hand out squared paper.

When the bitmap goes on, the console's text becomes 40 columns wide. It isn't broken, and `bitmap off` puts it
back.

**Close with** "why keep the drawing as a file of commands, when you could save the picture?"

### Lab 14. Pins, and the System's Own Files

**It's for** the moment when software moves the physical world: Part A. And for seeing the operating system
through its own files: Part B.

**Open with** an LED on the front-bench Hydra, lit by `echo 0 >/dev/gpio/2` at the projector.

**Watch for:**

* The wiring. Power *off* while wiring. An LED straight across a pin and ground, with no resistor, or a wire that
  shorts +5 V to ground: check each circuit before its power goes on. The manual's caution in section 17.1 is
  not decoration.
* "It lights when I write 0." Yes: the LED is wired from +5 V to the pin, so the pin has to go *low* to light
  it. The manual explains why (a pin can sink more current than it can supply). It's worth five minutes at the
  board.
* On an emulator, Part A stops at step 2. Those students go straight to Part B.

**Close with** "the LED, the clock and the task list are all files. What isn't a file, on the Hydra?"

## Labs 15 to 23: programming languages

### Choosing the languages

Few courses have time for all three. Some advice:

* **One language, for everybody: HyBASIC.** The gentlest start, readable programs, and exact fractions that
  delight mathematics teachers. Labs 20 and 21.
* **A second, for contrast: HyForth.** It's the shell, so it costs no extra set-up, and nothing else shows so
  plainly that a language is a *way of thinking*. Labs 16 and 17.
* **hylang** for a class that already programs, or one with a mathematical bent. Its ideas (functions as values,
  lists, recursion) are the ones a later computer science course will build on. Labs 18 and 19.

If you let students choose, do it after a taster of each: the manual's section 6.9 is ten minutes, and Chapter
25 sets the three side by side.

### Lab 15. Numbers and Bases

**It's for** breaking the belief that a computer's numbers are the calculator's numbers.

**Open with** "what's 0.1 + 0.2?" Have someone try it in a spreadsheet, or in another language, if one's to
hand, and then on the Hydra.

**Watch for** the quotes around `calc`'s sums. Without them, the shell takes `*` and `(` for its own, and the
error comes from rc, not from `calc`.

**Close with** "is a third a number the Hydra can hold exactly? Is the square root of two?"

### Lab 16. HyForth: The Stack

**It's for** the stack: a simple idea that takes an hour to believe.

**Open with** a real stack: five numbered cards or plates. Act out `1 2 3 swap` with a volunteer. Then act out
`+`, with the volunteer taking two cards and putting one back. It looks childish, and it works on university
students too.

**Watch for:**

* Missing spaces: `2 3+`. The message `3+ ?` is HyForth's "I don't know that word".
* Leftovers. A student whose answers are wrong "for no reason" has numbers on the stack from an earlier line.
  Teach `.s` as the first thing to type when puzzled.
* Step 1's table is the lab. Don't let them type before they've filled in the guesses.

**Close with** "why doesn't Forth need brackets?"

### Lab 17. HyForth: Decisions, Loops and Programs

**It's for** building a program out of small tested words: the most transferable habit in programming.

**Open with** FizzBuzz, played aloud round the room, to 30. Then: "how would you teach the Hydra to play?"

**Watch for:**

* `if` at the prompt (`if: compile only`). Decisions and loops live inside definitions.
* Stack imbalance across `if` ... `else` ... `then`: one branch uses the number and the other doesn't. The
  discipline that cures it is the stack diagram: have them write one for every word, and check both branches
  against it.
* `do` loops with the limit and the start the wrong way round.

**Close with** "you've written `triangle` using `stars`. Which did you test first? Why?"

### Lab 18. hylang: Expressions and Functions

**It's for** the idea of a *value*: everything you type is worth something.

**Open with** `(+ 1 2 3 4 5)`, and "what's odd about that plus sign?"

**Watch for:**

* Parentheses, of course. Teach counting them from the inside out, and teach the prompt of spaces as the sign
  of one missing.
* Braces against parentheses in `fun`. The body goes in braces because it mustn't be worked out yet. A body in
  parentheses gives `Unbound Symbol 'n'` at once, which is baffling until explained.
* `(square)` returning a function. Students think it's an error. It's one of the best ideas in the language,
  and a quick student can be set loose on it.

**Close with** "what's the value of `(def {x} 5)`? And of `x`, afterwards?"

### Lab 19. hylang: Lists, Decisions and Programs

**It's for** working on a whole collection at once, and for recursion.

**Open with** "add up the numbers from 1 to 100", by any means, timed. Then `(sum (range 1 101))`.

**Watch for** recursion without a stopping case. `fact` without its `if` runs until memory is gone. Ask every
student, of every recursive function: "which line stops it?"

**Close with** "you wrote no loop in step 3. Where did the loop go?"

### Lab 20. HyBASIC: First Programs

**It's for** the fundamentals: a variable, a sequence, a loop, a decision, input and output.

**Open with** a three-line program that asks your name and insults you politely. Then change one line, in front
of them, and run it again. The edit-run cycle is the lesson.

**Watch for:**

* `AGE = AGE + 1` read as an equation ("that's impossible"). Say "becomes", never "equals".
* A variable's name misspelled. It isn't an error in BASIC: the new name is a new variable, worth 0. The
  symptom is a mysterious 0.
* Students who learned to renumber by retyping. Show them that a line typed again replaces the old one, and
  that `EDIT` exists.

**Close with** "what's the difference between `PRINT A` and `PRINT "A"`?"

### Lab 21. HyBASIC: Bigger Programs

**It's for** structure: loops that end on a condition, data in arrays, and procedures.

**Open with** the question "how many times do you have to double 1 to pass a million?" Take guesses. (Twenty.)

**Watch for** whose variable it is. A `SUB`'s or a `FUNCTION`'s variables are its own (the manual's section
23.12), so a student who adds to `TOTAL` inside a `SUB` finds the main program's `TOTAL` unchanged, and decides
that HyBASIC is broken. `SHARED TOTAL` in the `SUB` is the cure; passing the value in and giving the answer back
with a `FUNCTION` is the better one. The lab's programs have no line numbers: they're kept in files and run from
the shell with `basic file.bas`. Make that the class's habit from this lab on.

**Close with** "what does a `SUB` give you that a `GOSUB` doesn't?" (Its own variables, and a name.)

### Lab 22. Your Language Meets the Hydra

**It's for** using a manual as a programmer does: the job is set, and the student finds the means.

**Open with** the list of seven jobs, and nothing else. This lab is deliberately unscaffolded.

**Watch for** frustration, at first, from students used to being given the code. Hold your nerve. Point at the
chapter, not at the line.

Mixed-language classes can compare their answers to each job: it becomes Lab 23 by itself.

**Close with** "which job did your language make easy? Which did it make you work for?"

### Lab 23. Three Languages

**It's for** reading code you didn't write, in a language you don't know: what programmers actually do all day.

**Open with** the guessing game, played against the class at the projector. Then ask for the best strategy, and
why seven guesses is always enough for a hundred numbers.

**Watch for** step 6, where HyForth counts to 40,000 and prints -25536. It's the 16-bit cell overflowing into
the sign bit, and a perfect, unplanned-looking introduction to Lab 24.

**Close with** "is there a best programming language?"

## Labs 24 to 26: machine language

### Lab 24. The 65C02, on Paper

**It's for** the bottom layer: there is nothing underneath this but electronics.

**Open with** "be the processor": three students at the front, as A, X and the program counter, each holding a
card with a number, and a fourth reading out instructions.

**Watch for:**

* `#`. `lda #$03` loads the number 3; `lda $03` loads whatever is in memory at address 3. It's the classic
  beginner's bug, and Lab 25 has them make it on purpose.
* The carry. `$FF + $01` gives `$00` and a carry of 1: an odometer rolling over.
* `bne` after `dex`: "branch if the result wasn't zero". Students need to say that sentence aloud a few times.

No Hydra is needed, which makes this a good lab for a day when the computers are down.

**Close with** "the 65C02 has no multiply instruction. Is that a problem?"

### Lab 25. Your First Machine Code

**It's for** the whole cycle: source, assembler, program, run.

**Open with** `./stars`, and then `xd stars`: the program as bytes. "Somewhere in there is the instruction that
prints a star. Next lesson we'll find it."

**Watch for** typing. `stars.s` is thirty lines, and one wrong character stops it. Have students assemble after
the first few lines to catch errors early, or give the file out on a card image (Topic 3) to those who would
otherwise lose the whole lesson to typing.

Step 4, the deliberate mistakes, is the centre of the lab. The one that assembles without complaint (`lda 1`)
matters most: the assembler can't tell you about a program that's valid but wrong.

**Close with** "what's in `stars.s` that isn't in `stars`?" (The names and the comments: everything that's for
people.)

### Lab 26. Taking Programs Apart

**It's for** closing the loop: the program from Lab 25, opened up and watched from inside.

**Open with** the single-step at the projector: `db ./stars`, and `s`, `s`, `s`, with the class calling out what
A will hold next. They traced by hand in Lab 24; now the machine shows its working.

**Watch for** `c` with no breakpoint set: the program runs to its end, `db` exits, and the student thinks it
crashed.

In step 8, the students change a running program's memory. Make sure the questions that follow are answered:
the file on disk hasn't changed, and neither has the source.

**Close with** "you've now seen the same program as BASIC-like source, as assembly, as bytes, and running. Which
one *is* the program?"
