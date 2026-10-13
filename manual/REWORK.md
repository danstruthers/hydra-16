# The manual's rework: the plan

This plan answers the review of the first edition (October 2026), read once as a student of 12 and once as a
teacher. It is worked from top to bottom; each item is ticked when it is done, checked in the emulator, and in the
PDF. The branch is `reborn-manual-rework`.

## Where it stands

All five phases are done (12 October 2026), and the faults in HydraOS that the writing found are fixed (the last
section), with the books' notes about them taken out. The three books:

| Book | Build | PDF |
|---|---|---|
| The User's and Builder's Manual | `node manual/build.js` | `manual/hydra-16-manual.pdf`: 29 chapters, appendices A to K |
| The Student Companion | `node manual/build.js companion` | `manual/companion/hydra-16-companion.pdf`: a build log, 26 labs, 7 projects, 8 cards |
| The Educator's Course Guide | `node manual/build.js educator` | `manual/educator/hydra-16-course-guide.pdf`: 16 topics, with the answers |

To check them: `node manual/tools/check.js . companion educator` replays every transcript (about 450, and three
quarters of an hour; give it one book, or one file, at a time), and `node manual/tools/xref.js` checks every
cross-reference. Chapter numbers changed twice on the way (a new Chapter 6, First Steps, and a new Chapter 25, One
Program, Three Languages): `tools/renumber.js` did it, and will again.

What is left is in "Decisions that are yours", at the end.

## Three books, not one

The first edition tries to be a tutorial, a reference and a course at once, and the course is the part it does
badly. Putting checklists, lab sheets, exercises, lesson plans and answer keys into it would make a 320-page book a
500-page one, and would put the answers in the students' hands. So the rework makes three books that share one set
of tools, figures and checked examples:

| Book | Folder | For | What it is |
|---|---|---|---|
| **The User's and Builder's Manual** | `manual/` | Everyone | The tutorial and the reference. It must stand alone: a reader with only this book can build a Hydra and learn all of it. |
| **The Student Companion** | `manual/companion/` | Students (and anyone building) | A workbook that is written in: build checklists, a build log, lab sheets, exercises, projects, quick-reference cards. It points into the manual; it doesn't repeat it. |
| **The Educator's Course Guide** | `manual/educator/` | Teachers and professors | Planning: course shapes, a scope and sequence, lesson plans keyed to the manual's chapters and the companion's labs, room and kit logistics, safety, assessment, and the answers. |

Rules that keep the three in step:

* The manual never needs the other two. The companion and the guide always name the manual's chapter and section.
* Every transcript in all three is replayed by `tools/check.js`; every cross-reference is checked by `tools/xref.js`.
* An exercise's question is in the companion; its answer is only in the guide.

## Phase 1. The manual: put right what is wrong

- [x] **1.1 Errors.** The review's list: the joint counts and hours (Chapter 3's opening, section 4.1); "most chips
  are CMOS" (3.3); 4.3's pointer for the power wiring; "up to 15 cards" (1.3); the boot's one module against `free`'s
  three (5.3, 5.4); the stale task table in the boot's step 7; sixteen or fifteen interrupt lines; `free`'s 1024 KB
  of shared memory against the board's 2 MB; 16.1's pointer for I²C; "the monitor ... Part Five" (2.4); `hi.s`'s
  comment about window 0; the `TIP` box missing from the preface.
- [x] **1.2 The book's Hydra.** One box that says which Hydra the examples were typed into (three RAM modules, the
  sample SD card, no Vera X, no clock chip), how the web emulator differs as it comes, what a reader will see
  instead, and how to make it match. "All you need is a web browser" becomes true: say where `hydra-16.html` comes
  from.
- [x] **1.3 A first chapter for everyone.** A new **Chapter 6, First Steps**, opens Part Two: the prompt, typing and
  fixing a line, the first commands, what a mistake looks like and how to get out of one, looking around, making a
  file with `edit`, where work is kept, the sample card's programs, a taste of each language, and where to find
  help. Chapters 6 to 27 become 7 to 28. Chapter 5 keeps only what needs the real board.
- [x] **1.4 Chapter openings.** Every chapter starts by saying what the reader will be able to do at its end and
  what it expects them to know (the preface promises this).
- [x] **1.5 Signposts.** The boot chapter says what to skim on a first reading. Part Three says that its Forth
  one-liners need no Forth. Part Four opens with "which language first?".
- [x] **1.6 An honest build.** The real number of joints and hours; safety that a school needs (fumes, lead, burns,
  eye protection, washing hands); the SD card reader as something nearly everyone wants; the address decoders in
  the fault-finding list; where the Vera X's own instructions are.
- [x] **1.7 Machine language.** `stars` before the 70-line `hi.s`; the debugger's second listing shown as its
  three changed lines.
- [x] **1.8 Looking things up.** The glossary's missing words (baud, DIP, PLCC, FPGA, MIDI, MML, ZSM, CMOS, RS-232,
  LED, cluster ...); the index's missing everyday words (loop, save, copy, prompt, LED ...), from `index.txt`.
- [x] **1.9 Tools.** `tools/xref.js`: every "Chapter N", "section N.M", "Figure N-M" and "Appendix X" in the three
  books points at something that exists. `tools/renumber.js`: moves chapters, and everything that names them.

## Phase 2. The languages: teach more, look up more

The review's hardest words were for Part Four: eight pages do not teach a language, and a one-line entry does not
explain a word.

- [x] **2.1 The three "Learning" chapters** grow into short courses: each idea with an example to type and one to
  change, the mistakes a beginner makes and what the Hydra says, and a whole small program built up at the end.
  HyForth: what an unknown word and an unfinished definition look like. hylang: the prefix table after the reader
  has used three prefixes; `T` and `NIL`. HyBASIC: the example that can be `RUN` twice.
- [x] **2.2 The three references** (Appendices A, B and C): every word with its arguments, what it does, and a
  checked example; grouped by job; and every word in the index, so that a word met in an example can be found.
- [x] **2.3 The same program in three languages**, side by side, at the end of Part Four.
- [x] **2.4 The sample card.** A section for the programs on the SD card (`sketch`, `shapes`, `tones`, `jukebox`,
  `race`, `philo` ...): what each shows, and how to run and read it.

## Phase 3. The Student Companion

- [x] **3.1 `build.js` builds any of the three books** (`node manual/build.js companion`), with shared style,
  figures and tools; a workbook's own page furniture (tick boxes, write-in lines, tables to fill).
- [x] **3.2 The build log.** A checklist for every step of Chapters 3 to 5, with the parts and joint counts of each
  stage, inspection sign-offs, the first power-on's readings, and a fault-finding chart.
- [x] **3.3 Lab sheets**, one or more for each chapter of the manual: the aim, what you need, the steps, questions
  with room for answers, a "check yourself" list and a challenge.
- [x] **3.4 Projects.** Type-in programs that grow: a guessing game, a quiz, turtle pictures, a tune, a light on the
  I/O header, two tasks that talk, a machine-language routine called from BASIC.
- [x] **3.5 Quick-reference cards**: the keys, the commands, each language on two pages, the 65C02 on two.
- [x] **3.6 A logbook and a skills passport** ("I can ...").

## Phase 4. The Educator's Course Guide

- [x] **4.1 Course shapes**: a one-week camp, a nine-week unit, a semester and a year; a club; emulator only, one
  shared Hydra, or a Hydra for each group; and a university outline (architecture and operating systems).
- [x] **4.2 Scope and sequence**: lessons with aims, timings, the manual's sections, the companion's labs, what to
  demonstrate and what goes wrong.
- [x] **4.3 The room**: giving out the emulator, students' SD images, a soldering station's tools and rules, a
  safety policy, the build as a group project, parts and spares.
- [x] **4.4 Assessment**: rubrics, quizzes, project marking, and the answers to everything in the companion.
- [x] **4.5 Reaching everyone**: younger and older students, students who race ahead, students who can't solder.

## Phase 5. Finish

- [x] Three PDFs built; all transcripts checked; all cross-references checked; the review read again against all three.

## Decisions that are yours

1. **Three books or one.** Decided: three.
2. **The order of the parts.** Devices stay before languages, as you asked, with signposts. The other choice is to
   move Part Four in front of Part Three.
3. **Photographs.** The books have drawings only. Photographs of a built board, a good and a bad joint, and each
   building stage would help a 12-year-old more than any drawing; they need a camera and a real board.
4. **What a kit is.** Where a board, the parts and the programmed ROMs come from, and what they cost, is not in the
   repository. The Course Guide says so, and lists what a kit is made of (its Topic 3) so that it can be priced.
   A component list and buy sheet (everything but the main board and the breakout cards, from sellers such as
   Digi-Key) is to be a task of its own, after this one.
5. **Changes outside the manual** that would remove whole paragraphs of explanation (not done here):
   * a `help` command on the Hydra (there is none: `help` answers `rc: help: not found`);
   * the emulators starting as the same Hydra (the web page: a Vera X, a clock chip, three modules; the terminal:
     none, none, two);
   * `hydra-16.html` kept ready-made in the repository, beside the ROM images.

## The bugs found while writing: fixed

The first editions of the three books worked round these faults in HydraOS. They're fixed now, each with a
regression test (`node reborn/sim/test.js`: 128 tests), and the books' notes about them are gone.

| The fault | The fix | Its test |
|---|---|---|
| HyBASIC: the second `RUN` of a program with a `SUB` or a `FUNCTION` said `duplicate definition`, until HyBASIC was started again | A procedure's entry is marked "not defined yet" when a program's first pass makes it (`modules/basic/procs.inc`) | `bsuite` (`tests/basic/prompt.txt`) |
| rc: a pipeline of four programs said `too many fds`, and left rc's pipes broken | Pipes are made as each program starts, not all at once, and are closed if the line is abandoned (`modules/rc/exec.s`). The limit is now the Hydra's sixteen tasks: six programs at the prompt, then `no free task` | `rc` |
| rc: `while()` and `if()` with an empty test | An empty test is true, as in Plan 9's rc | `rc` |
| HyForth: `.` (and `u.`, `emit`, `type`, `spaces`) on an empty stack printed rubbish before `stack underflow` | The printing words look at the stack before they print (`modules/forth/ftext.inc`) | `fshell` |
| HyForth as the shell: a line that started with `2/3`, `100000` or `#xFF` went to rc | The shell asks the numbers library, if it's loaded, whether the first word is one of its numbers (`forthlib/shell.s`, `forthlib/numbers.s`) | `fshell` |
| db: after <kbd>CTRL</kbd>-<kbd>C</kbd> in an `n`, `c` stopped again, saying nothing, where the step would have ended. And a breakpoint inside a subroutine that `n` was running left `s`, `n` and `c` all refused | `n` runs a subroutine of the program's own in place, with a `BRK` of db's where it returns to, so no step of the kernel's is left unfinished; a system call's unfinished step is finished by `s` or `n`, and passed by `c` (`programs/db/db.c`) | `dbover` |
| The console: the Vera X's boot message was cut into the next line (`[a1s8kt 1: init`) | The console driver waits for its question to the terminal to be sent before its start-up ends (`modules/cons/cons.s`) | `screen` |
| The console: keys typed or pasted ahead of a program were dropped after the 63rd | A key with no room in its window's queue stays in the receive ring, with those behind it, till the program has read some: 255 more (`modules/cons/cons.s`) | `typeahead` |
